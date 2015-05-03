#include "fuse_imp.hpp"
#include "entry.hpp"
#include "poi.hpp"

POIFS POI;

/*** IMPLEMENTING FUSE OPERATIONS DEFINED ON THE API ***/

/* KALO BUTUH INFO TENTANG STAT BISA KE SINI http://codewiki.wikidot.com/c:struct-stat */

int poi_getattr(const char *path, struct stat *stbuf) {
	if (string(path) == "/"){
		/* if the path is a root path */
		stbuf->st_nlink = 1;
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_mtime = POI.mount_time;
	} else {
		Entry entry = Entry(0, 0).getEntryfromPath(path);
		
		// if the path isn't found
		if (entry.isEmpty())
			return -ENOENT;
		
		stbuf->st_nlink = 1;

		// if the entry is a directory
		if (entry.getAttribut() & 0x8)
			stbuf->st_mode = S_IFDIR | (0770 + (entry.getAttribut() & 0x7));
		else
			stbuf->st_mode = S_IFREG | (0660 + (entry.getAttribut() & 0x7));
		
		stbuf->st_size = entry.getSize();
		stbuf->st_mtime = entry.getEntryTime();
	}

	return 0;
}

int poi_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	// current working directory and root directory was defined to filler function
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	
	Entry tmp = Entry(0,0).getEntryfromPath(path);
	ushort index = tmp.getIndex();
	tmp = Entry(index,0);
	
	// filler is applied to the dir
	while (tmp.entryPosition != END_BLOCK) {
		if(!tmp.isEmpty())
			filler(buf, tmp.getNama(), NULL, 0);
		tmp = tmp.getNextEntry();
	}
	
	return 0;
}

int poi_mkdir(const char *path, mode_t mode){
	int i;
	for (i = string(path).length() - 1; path[i] != '/'; --i) {}
	
	/* get root directory */
	string root = string(path, i);
	
    Entry tmp;
    if (root == "")
    	// when the path is already the root path
		tmp = Entry(0, 0);
	else {
		tmp = Entry(0,0).getEntryfromPath(root.c_str());
		ushort index = tmp.getIndex();
		tmp = Entry(index, 0);
    }
    
    tmp = tmp.getEmptyEntry();		// find empty entry from root path
    
	tmp.setNama(path + i + 1);
	tmp.setAttribut(0x0F);
	tmp.setCurrentTime();
	tmp.setIndex(POI.allocateBlock());
	tmp.setSize(0);

	tmp.writeEntry();

	return 0;
}

int poi_mknod(const char *path, mode_t mode, dev_t dev){
	int i;
	for (i = string(path).length() - 1; path[i] != '/'; --i) {}
	
	/* get root directory */
	string root = string(path, i);
	
    Entry tmp;
    if (root == "")
    	// when the path is already the root path
		tmp = Entry(0, 0);
	else {
		tmp = Entry(0,0).getEntryfromPath(root.c_str());
		ushort index = tmp.getIndex();
		tmp = Entry(index, 0);
    }
    
    tmp = tmp.getEmptyEntry();		// find empty entry from root path
    
	tmp.setNama(path + i + 1);
	tmp.setAttribut(0x06);
	tmp.setCurrentTime();
	tmp.setIndex(POI.allocateBlock());
	tmp.setSize(0);

	tmp.writeEntry();

	return 0;
}

int poi_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
	Entry tmp = Entry(0,0).getEntryfromPath(path);
	ushort idx = tmp.getIndex();
	
	
	if(tmp.isEmpty()) // if the entry is empty
		return -ENOENT;
	
	return POI.readBlock(idx, buf, size, offset); // pass the reading to POIFS method readBlock
}

int poi_rmdir(const char *path){
	Entry tmp = Entry(0,0).getEntryfromPath(path);
	
	if(tmp.isEmpty())
		return -ENOENT; // if the entry is empty, let it be
	
	// free the block from the entry index
	POI.releaseBlock(tmp.getIndex());
	
	// make the entry becomes empty
	tmp.makeEmpty();
	
	return 0;
}

int poi_unlink(const char *path){
	Entry tmp = Entry(0,0).getEntryfromPath(path);

	if (tmp.getAttribut() & 0x8)	
		return -ENOENT;		// if the entry is from a directory, let it be
	else {
		// free the block from the entry index
		POI.releaseBlock(tmp.getIndex());	
		// make the entry becomes empty
		tmp.makeEmpty();
	}

	return 0;
}

int poi_rename(const char *oldpath, const char *newpath) {
	Entry oldentry = Entry(0,0).getEntryfromPath(oldpath);
	Entry newentry = Entry(0,0).getNewEntryfromPath(newpath);

	if(!oldentry.isEmpty()){
		// set all 
		newentry.setNama(oldentry.getNama());
		newentry.setAttribut(oldentry.getAttribut());
		newentry.setIndex(oldentry.getIndex());
		newentry.setSize(oldentry.getSize());
		newentry.setTime(oldentry.getTime());
		newentry.setDate(oldentry.getDate());
		newentry.writeEntry();
		
		oldentry.makeEmpty();
	}
	else
		return -ENOENT;
	
	return 0;
}

int poi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	Entry tmp = Entry(0,0).getEntryfromPath(path);

	ushort idx = tmp.getIndex();
	
	// if the entry is empty, let it be
	if(tmp.isEmpty())
		return -ENOENT;
	
	tmp.setSize(offset + size);
	tmp.writeEntry();
	
	return POI.writeBlock(idx, buf, size, offset);;
}

int poi_truncate(const char *path, off_t newoff){
	Entry tmp = Entry(0,0).getEntryfromPath(path);
	
	tmp.setSize(newoff);
	tmp.writeEntry();
	
	// the rest of using volume information
	ushort position = tmp.getIndex();
	while (newoff > 0) {
		newoff -= BLOCK_SIZE;
		if (newoff > 0) {	// allocate new entry when full
			if (POI.nextBlock[tmp.entryPosition] == END_BLOCK)
				POI.setNextBlock(tmp.entryPosition, POI.allocateBlock());
			tmp.entryPosition = POI.nextBlock[tmp.entryPosition];
		}
	}

	POI.releaseBlock(POI.nextBlock[tmp.entryPosition]);
	POI.setNextBlock(tmp.entryPosition, END_BLOCK);
	
	return 0;
}

 /************************************/
 /**	 BONUS IMPLEMENTATION		**/
 /************************************/
int poi_chmod(const char *, mode_t);

int poi_link(const char *, const char *);

int poi_open(const char *, struct fuse_file_info *);
