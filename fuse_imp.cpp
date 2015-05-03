#include "fuse_imp.hpp"


/*** IMPLEMENTING FUSE OPERATIONS DEFINED ON THE API ***/

/* KALO BUTUH INFO TENTANG STAT BISA KE SINI http://codewiki.wikidot.com/c:struct-stat */

int poi_getattr(const char *path, struct stat *stbuf) {
	if (string(path) == "/") {
		/* if path is the root path */
		stbuf->st_nlink = 1;
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st
		return 0;
	} else {
		
	}
}

int poi_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *){
	log_msg("\nimplement_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n", path, buf, filler, offset, fi);
	
	filler(buf, ".", NULL, 0); // current directory
	filler(buf, "..", NULL, 0); // parent directory
		
	int idx = searchFile(path);
	
	idx = file[idx].getPointer(); // inside directory
	while (idx != END_BLOCK) {
		filler(buf, file[idx].getName().c_str(), NULL, 0);
		idx = file[idx].getNextPointer();
	}
	return 0;
}

int poi_mkdir(const char *path, mode_t mode){
    if (getLastPath(std::string(path+1)).length() > 20) // the length of the name is longer than 20 char, it may be invoked by terminal
		return -ENAMETOOLONG;
	if (volume_information.getNumbreleaseBlock() == 0) // check whether there is a block left
		return -EDQUOT;
	
	int idx = searchFile(path);
	
	if (idx >= 0) // the folder already exists
		return -EEXIST;
		
	// alocate new block
	idx = volume_information.frontBlock();
	volume_information.popBlock();
	
	// edit the previous entry pointer
	int prev_idx = searchFile(std::string("/"+removeLastPath(std::string(path+1))).c_str());
	if (file[prev_idx].getPointer() != END_BLOCK) { // if inside pointer is already filled, add to allocation table
		prev_idx = file[prev_idx].getPointer();
		while (file[prev_idx].getNextPointer() != END_BLOCK)
			prev_idx = file[prev_idx].getNextPointer();
		file[prev_idx].setNextPointer(idx);
	}
	else
		file[prev_idx].setPointer(idx);
	
	// set up the folder entry
	file[idx].setName(getLastPath(std::string(path+1)));
	file[idx].setSize(0);
	file[idx].setPointer(END_BLOCK);
	file[idx].setNextPointer(END_BLOCK);
	file[idx].setAttr(S_IFDIR | 0777);
	file[idx].setDateTime(time(NULL));
	log_msg("Folder's index : %d, pointing to %d\n", idx, file[idx].getPointer());
	
	return 0;
}

int poi_mknod(const char *path, mode_t mode, dev_t dev){
	log_msg("\nimplement_mknod(path=\"%s\")\n", path);
	
	if (getLastPath(std::string(path+1)).length() > 20) // the length of the name is longer than 20 char, it may be invoked by terminal
		return -ENAMETOOLONG;
	if (volume_information.getNumbreleaseBlock() == 0) // check whether there is a block left
		return -EDQUOT;
	
	int idx = searchFile(path);
	
	if (idx >= 0) // the file already exists
		return -EEXIST;
		
	// alocate new block
	idx = volume_information.frontBlock();
	volume_information.popBlock();
	
	// edit the previous entry pointer
	int prev_idx = searchFile(std::string("/"+removeLastPath(std::string(path+1))).c_str());
	if (file[prev_idx].getPointer() != END_BLOCK) { // if inside pointer is already filled, add to allocation table
		prev_idx = file[prev_idx].getPointer();
		while (file[prev_idx].getNextPointer() != END_BLOCK)
			prev_idx = file[prev_idx].getNextPointer();
		file[prev_idx].setNextPointer(idx);
	}
	else
		file[prev_idx].setPointer(idx);
	
	// set up the file entry
	file[idx].setName(getLastPath(std::string(path+1)));
	file[idx].setSize(0);
	file[idx].setPointer(0xFFFF);
	file[idx].setNextPointer(0xFFFF);
	file[idx].setAttr(S_IFREG | 0777);
	file[idx].setDateTime(time(NULL));
	log_msg("The file's index : %d, pointing to %d\n", idx, file[idx].getPointer());
	
	return 0;
}

int poi_read(const char *, char *, size_t, off_t, struct fuse_file_info *){
	log_msg("\nimplement_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n", path, buf, filler, offset, fi);
	
	filler(buf, ".", NULL, 0); // current directory
	filler(buf, "..", NULL, 0); // parent directory
		
	int idx = searchFile(path);
	
	idx = file[idx].getPointer(); // inside directory
	while (idx != END_BLOCK) {
		filler(buf, file[idx].getName().c_str(), NULL, 0);
		idx = file[idx].getNextPointer();
	}
	
	return 0;
}

int poi_rmdir(const char *path){
	log_msg("\nimplement_rmdir(path=\"%s\")\n", path);
    
    int idx = searchFile(path);
    
	if (file[idx].getPointer() != END_BLOCK) // the folder is not empty
		return -ENOTEMPTY;
	
	if (idx >= 0) {
		int ptr = searchPrevFile(path);
		if (ptr != END_BLOCK) {
			file[ptr].setNextPointer(file[idx].getNextPointer());
		}
		else {
			ptr = searchParentFolder(path);
			file[ptr].setPointer(file[idx].getNextPointer());
		}
		volume_information.pushBlock(idx);
	}
	
	return 0;
}

int poi_unlink(const char *){
	log_msg("\nimplement_rmdir(path=\"%s\")\n", path);
    
    int idx = searchFile(path);
    
	if (file[idx].getPointer() != END_BLOCK) // the folder is not empty
		return -ENOTEMPTY;
	
	if (idx >= 0) {
		int ptr = searchPrevFile(path);
		if (ptr != END_BLOCK) {
			file[ptr].setNextPointer(file[idx].getNextPointer());
		}
		else {
			ptr = searchParentFolder(path);
			file[ptr].setPointer(file[idx].getNextPointer());
		}
		volume_information.pushBlock(idx);
	}
	
	return 0;
}

int poi_rename(const char *, const char *){
	log_msg("\nimplement_rename(path=\"%s\", path_new=\"%s\")\n", path, path_new);
    
    std::string rename = getLastPath(std::string(path_new+1));
	if (rename.length() > 20) // the length of the name is longer than 20 char
		return -ENAMETOOLONG;
	
    int idx = searchFile(path); // the current block
    int prev = searchPrevFile(path);
    if (prev != END_BLOCK) {
    	log_msg("Prev file : %d\n", prev);
    	file[prev].setNextPointer(file[idx].getNextPointer());
    }
    else {
    	prev = searchParentFolder(path);
    	log_msg("Prev folder : %d\n", prev);
    	file[prev].setPointer(file[idx].getNextPointer());
    }

    int prev_new = searchParentFolder(path_new);
    log_msg("Parent folder : %d\n", prev_new);
    file[idx].setNextPointer(file[prev_new].getPointer()); // change the next pointer
    file[prev_new].setPointer(idx); // set to first pointer only
	
	file[idx].setName(rename); // rename it
	return 0;
}

int poi_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	log_msg("\nimplement_write(path=\"%s\")\n", path);
	log_msg("Size : %d, Offset : %d\n", size, offset);
	
	int init_size = size;
	
	int idx = searchFile(path); // search the file
	file[idx].setSize(file[idx].getSize()+size); // add to the file's size first
	
	if (idx < 0) // error no such file
		return -ENOENT;
	
	int ptr = file[idx].getPointer(); // point to file's data
	log_msg("Index mula-mula : %d, Pointer mula-mula : %d\n", idx, ptr);
	if (ptr == END_BLOCK) { // new file needs a new block
		ptr = volume_information.frontBlock();
		volume_information.popBlock();

		file[idx].setPointer(ptr);
		file[ptr].setNextPointer(END_BLOCK); // set new data's next pointer to END_BLOCK
	}
	
	// switch block until offset is less than a BLOCK_SIZE 
	while (offset >= BLOCK_SIZE) {
		idx = ptr;
		ptr = file[idx].getNextPointer();
		offset -= BLOCK_SIZE;
	}

	log_msg("Pointing to data : %d w/ Pointer : %d\n", idx, ptr);
	
	int pos = 0;
	while (size > 0) {
		if (ptr == END_BLOCK) { // if the next block hasn't been created yet
			if (volume_information.getNumbreleaseBlock() != 0) {
				ptr = volume_information.frontBlock();
				volume_information.popBlock();
				
				log_msg("Create Index %d pointing to %d\n", idx, ptr);

				file[idx].setNextPointer(ptr);
				file[ptr].setNextPointer(END_BLOCK);
			}
			else
				return -EFBIG; // File too large 
		}
		
		idx = ptr; // switch to idx
		ptr = file[idx].getNextPointer(); // ptr still becomes idx's next pointer
		log_msg("Index %d pointing to %d\n", idx, ptr);

		if (size+offset >= BLOCK_SIZE) {
			memcpy(file[idx].currentPosHandler()+offset, buf+pos, BLOCK_SIZE-offset);
			pos += BLOCK_SIZE-offset;
			size -= BLOCK_SIZE-offset;
		}
		else {
			memcpy(file[idx].currentPosHandler()+offset, buf+pos, size);
			pos += size;
			size = 0;
		}
		offset = 0;
	}
	log_msg("End of write\n");
	return init_size;
}

int poi_truncate(const char *path, off_t newsize){
	log_msg("\nimplement_truncate(path=\"%s\", offset = %d)\n", path, (int) newsize);
	
	int idx = searchFile(path);
	
	file[idx].setSize(newsize);
	int ptr = file[idx].getPointer();
	
	int count = 0;
	while (count*BLOCK_SIZE < newsize) {
		if (ptr == END_BLOCK) {
			ptr = volume_information.frontBlock();
			volume_information.popBlock();
			file[idx].setNextPointer(ptr);
			file[ptr].setNextPointer(END_BLOCK);
		}
		idx = ptr;
		ptr = file[idx].getNextPointer();
		count++;
	}

	if (count == 0) // set to END_BLOCK the previous block
		file[idx].setPointer(END_BLOCK);
	else
		file[idx].setNextPointer(END_BLOCK);

	std::stack<int> data;
	while (ptr != END_BLOCK) {
		data.push(ptr);
		log_msg("Push to stack : %d\n", ptr);
		ptr = file[ptr].getNextPointer();
	}
	while (!data.empty()) {
		volume_information.pushBlock(data.top());
		data.pop();
	}

	return newsize;
}

 /************************************/
 /**	 BONUS IMPLEMENTATION		**/
 /************************************/
int poi_chmod(const char *, mode_t);

int poi_link(const char *, const char *);

int poi_open(const char *, struct fuse_file_info *);
