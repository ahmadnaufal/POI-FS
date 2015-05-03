#ifndef POIFS_H
#define POIFS_H

 /* Define constants used by the filesystem */
#define BLOCK_SIZE 512
#define ENTRY_SIZE 32
#define DATAPOOL_BLOCK_N 65536
#define DATAALLOCTABLE 256

#define ROOT_BLOCK 0x0000
#define END_BLOCK 0xFFFF

#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <ctime>

class POIFS {
public:
	// inner class
	class Entry{
	public:
		// ctor
		Entry();

		// entry attribute getter
		char* getNama();
		char getAttribut();
		short getTime();
		short getDate();
		short getIndex();
		int getSize();

		// entry attribute setter
		void setNama(const char* nama);
		void setAttribut(const char attribut);
		void setTime(const short time);
		void setDate(const short date);
		void setIndex(const short index);
		void setSize(const int size);
		
		// fungsi & method
		bool isEmpty();
		void makeEmpty();
		

	private:
		char blockEntry[ENTRY_SIZE];
		short entryPosition;
	};

	// Constructor & Destructor
	POIFS();
	~POIFS();

	/* Create new .poi */
	void createPoi(const char *filename);
	/* Define the newly created .poi volume information */
	void initVolumeInformation(const char *filename, const char *rootname);
	/* Define the newly created .poi allocation table (which refer to data pool) */
	void initAllocTable();
	/* Define the newly created .poi data pool */
	void initDataPool();

	/* Read .poi */
	void readPoi(const char *filename);
	/* Read the .poi volume information */
	void readVolumeInformation();
	/* Read the .poi allocation table */
	void readAllocTable();
	
	void writeVolumeInformation();
	void writeAllocationTable(short position);
	
	/* bagian alokasi block */
	void setNextBlock(short position, short next);
	short allocateBlock();
	void releaseBlock(short position);
	
	/* bagian baca/tulis block */
	int readBlock(short position, char *buffer, int size, int offset = 0);
	int writeBlock(short position, const char *buffer, int size, int offset = 0);


	/*** NOT FINISHED YET ***/

private:
	fstream target;			/* current .poi file to be handled by POIFS */
	
	string rootdir;			/* root directory for mounting */
	string mountname;		/* the file name of the mounted .poi */
	short nextBlock[DATAPOOL_BLOCK_N];

	int blockCapacity;		/* total size of the filesystem */
	int availBlock;			/* number of empty blocks in the filesystem */
	int firstAvail;			/* number of the first available block in the data pool */
	time_t mount_time;		/* time mounting .poi file */
};

#endif
