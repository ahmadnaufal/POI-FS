#include "poi.hpp"

// Constructor & Destructor
POIFS::POIFS() { }
POIFS::~POIFS() {
	target.close()
}

/* Create new .poi */
void createPoi(const char *filename, const char *rootname) {
	target.open(filename, fstream::in | fstream::out | fstream::binary | fstream::trunc);

	/* Initialize the .poi volume information */
	initVolumeInformation(filename);

	/* Initialize the .poi allocation table */
	initAllocTable();

	/* Initialize the .poi data pool */
	initDataPool();

	target.close();
}

/* Define the newly created .poi volume information */
void initVolumeInformation(const char *filename, const char *rootname) {
	char buffer[BLOCK_SIZE];

	memset(buffer, '\0', BLOCK_SIZE);

	/* Set the magic string "poi!" */
	memcpy(buffer + 0x00, "poi!", 4);

	/* Set the filename */
	char *fname;
	if (filename == NULL)
		fname = "POI!";
	else
		fname = filename;
	memcpy(buffer + 0x04, fname, strlen(fname));

	/* initialize capacity of the filesystem */
	blockCapacity = DATAPOOL_BLOCK_N + 1;
	memcpy(buffer + 0x24, &blockCapacity, 4);

	availBlock = DATAPOOL_BLOCK_N;
	memcpy(buffer + 0x28, &availBlock, 4);

	/* initialize the first available block */
	firstAvail = 1;
	memcpy(buffer + 0x2C, &firstAvail, 4);

	/* Entry root directory block */
	memcpy(buffer + 0x30, path, 32); // BELUM SELESAI

	/* Closing "!iop" statement */
	memcpy(buffer + 0x1FC, "!iop", 4);

	target.write(buffer, BLOCK_SIZE);
}

/* Define the newly created .poi allocation table (which refer to data pool) */
void initAllocTable() {
	short buffer = 0xFFFF;

	/* Allocation for root */
	target.write((char*)&buffer, sizeof(buffer));
	for (int i = 1; i < DATAPOOL_BLOCK_N + 1; ++i)
		target.write((char*)&buffer, sizeof(buffer));
}

/* Define the newly created .poi data pool */
void initDataPool() {
	char buffer[BLOCK_SIZE];

	memset(buffer, 0, BLOCK_SIZE);

	for (int i = 0; i < DATAPOOL_BLOCK_N + 1; ++i)
		handle.write(buffer, BLOCK_SIZE);
}

/* 
/******* NOT FINISHED YET *******/