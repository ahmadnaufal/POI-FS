#include "poi.hpp"

// Implementasi class POIFS

// Constructor & Destructor
POIFS::POIFS() {
	time(&mount_time);
}
POIFS::~POIFS() {
	target.close();
}

/* Create new .poi */
void POIFS::createPoi(const char *filename, const char *rootname) {
	target.open(filename, fstream::in | fstream::out | fstream::binary | fstream::trunc);

	/* Initialize the .poi volume information */
	initVolumeInformation(filename, rootname);

	/* Initialize the .poi allocation table */
	initAllocTable();

	/* Initialize the .poi data pool */
	initDataPool();

	target.close();
}

/* Define the newly created .poi volume information */
void POIFS::initVolumeInformation(const char *filename, const char *rootname) {
	char buffer[BLOCK_SIZE];

	memset(buffer, '\0', BLOCK_SIZE);

	/* Set the magic string "poi!" */
	memcpy(buffer + 0x00, "poi!", 4);

	/* Set the filename */
	if (filename == NULL)
		mountname = "POI!";
	else
		mountname = string(filename);
	memcpy(buffer + 0x04, mountname.c_str(), mountname.length());

	/* initialize capacity of the filesystem */
	blockCapacity = DATAPOOL_BLOCK_N;
	memcpy(buffer + 0x24, (char*)&blockCapacity, 4);

	availBlock = DATAPOOL_BLOCK_N - 1;
	memcpy(buffer + 0x28, (char*)&availBlock, 4);

	/* initialize the first availBlock block */
	firstAvail = 1;
	memcpy(buffer + 0x2C, (char*)&firstAvail, 4);

	/* Entry root directory block */
	rootdir = string(rootname);
	memcpy(buffer + 0x30, rootdir.c_str(), rootdir.length()); // BELUM SELESAI

	/* Closing "!iop" statement */
	memcpy(buffer + 0x1FC, "!iop", 4);

	target.write(buffer, BLOCK_SIZE);
}

/* Define the newly created .poi allocation table (which refer to data pool) */
void POIFS::initAllocTable() {
	short buffer = 0xFFFF;
	char allocTable[DATAALLOCTABLE*BLOCK_SIZE];

	memset(allocTable, 0, sizeof(allocTable));

	/* Allocation for root */
	memcpy(allocTable, (char*)&buffer, sizeof(buffer));

	target.write(allocTable, sizeof(allocTable));
}

/* Define the newly created .poi data pool */
void POIFS::initDataPool() {
	char buffer[DATAPOOL_BLOCK_N * BLOCK_SIZE];

	/* Initialize data pool */
	memset(buffer, 0, sizeof(buffer));

	target.write(buffer, sizeof(buffer));
}

/* Read .poi */
void POIFS::readPoi(const char *filename){
	/* cek apakah file ada */
	try {
		target.open(filename, fstream::in | fstream::out | fstream::binary);
	}
	catch (exception& e) {
		target.close();
		throw runtime_error("File not found");
		cout << e.what();

		return;
	}
	/* periksa Volume Information */
	readVolumeInformation();
	/* baca Allocation Table */
	readAllocTable();
}

/* Read the .poi volume information */
void POIFS::readVolumeInformation(){
	char buffer[BLOCK_SIZE];
	target.seekg(0x00);

	target.read(buffer, BLOCK_SIZE);

	if (string(buffer, 4) != "poi!") {
		target.close();
		throw runtime_error("File is not a valid POI file");
		return;
	}

	/* read blockCapacity */
	memcpy((char*)&blockCapacity, buffer + 0x24, 4);
	/* read availBlock */
	memcpy((char*)&availBlock, buffer + 0x28, 4);
	/* read firstAvail */
	memcpy((char*)&firstAvail, buffer + 0x2C, 4);
	/* read the root directory */
	memcpy((char*)&rootdir, buffer + 0x30, 32);
}

/* Read the .poi allocation table */
void POIFS::readAllocTable(){
	char buffer[3];

	/* pindah posisi ke awal Allocation Table */
	target.seekg(0x200);

	/* baca nilai nextBlock */
	for (int i = 0; i < DATAPOOL_BLOCK_N; i++) {
		target.read(buffer, 2);
		memcpy((char*)&nextBlock[i], buffer, 2);
	}
}

void POIFS::writeVolumeInformation() {
	target.seekp(0x00);

	/* buffer untuk menulis ke file */
	char buffer[BLOCK_SIZE];
	memset(buffer, '\0', BLOCK_SIZE);

	/* Set the magic string "poi!" */
	memcpy(buffer + 0x00, "poi!", 4);

	/* Set the filename */
	memcpy(buffer + 0x04, mountname.c_str(), mountname.length());

	/* initialize capacity of the filesystem */
	memcpy(buffer + 0x24, (char*)&blockCapacity, 4);

	memcpy(buffer + 0x28, (char*)&availBlock, 4);

	/* initialize the first availBlock block */
	memcpy(buffer + 0x2C, (char*)&firstAvail, 4);

	/* Entry root directory block */
	memcpy(buffer + 0x30, rootdir.c_str(), rootdir.length());

	/* Closing "!iop" statement */
	memcpy(buffer + 0x1FC, "!iop", 4);

	target.write(buffer, BLOCK_SIZE);
}

/* bagian alokasi block */
void POIFS::writeAllocationTable(short position){
	/* Go to the position of the file */
	target.seekp(BLOCK_SIZE + (sizeof(position) * position));
	target.write((char*)&nextBlock[position], sizeof(position));
}

/* pengaturan allocation table untuk next block */
void POIFS::setNextBlock(short position, short next){
	nextBlock[position] = next;
	writeAllocationTable(position);
}

short POIFS::allocateBlock(){
	short res = firstAvail;
	setNextBlock(res, END_BLOCK);

	/* iterate through the block until found an empty block */
	while (nextBlock[firstAvail] != 0x0000)
		firstAvail++;

	availBlock--;
	writeVolumeInformation();
	return res;
}

void POIFS::releaseBlock(short position){
	/* if block is empty */
	if (position == ROOT_BLOCK)
		return;

	/* release the outer block */
	while (position != END_BLOCK) {
		short temp = nextBlock[position];
		setNextBlock(position, ROOT_BLOCK);
		position = temp;
		availBlock--;
	}

	writeVolumeInformation();
}

/* bagian baca/tulis block */
int POIFS::readBlock(short position, char *buffer, int size, int offset = 0){
	/* If at the end of file / END_BLOCK */
	if (position == END_BLOCK)
		return 0;

	/* if OFFSET is bigger than default BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		return readBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}

	target.seekg((BLOCK_SIZE * 257) + (position * BLOCK_SIZE) + offset);
	int curSize = size;

	/* put offset outside BLOCK_SIZE */
	if (offset + curSize > BLOCK_SIZE) {
		curSize = BLOCK_SIZE - offset;
	}

	/* read data with curSize */
	target.read(buffer, curSize);
	
	if (offset + size > BLOCK_SIZE) {
		return curSize + readBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}

	return curSize;
}

int POIFS::writeBlock(short position, const char *buffer, int size, int offset = 0){
	/* If at the end of file / END_BLOCK */
	if (position == END_BLOCK)
		return 0;

	/* if OFFSET is bigger than default BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return writeBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}

	target.seekp(BLOCK_SIZE * 257 + position * BLOCK_SIZE + offset);
	int curSize = size;
	if (offset + curSize > BLOCK_SIZE) {
		curSize = BLOCK_SIZE - offset;
	}
	target.write(buffer, curSize);
	/* kalau size > block size, lanjutkan di nextBlock */
	if (offset + size > BLOCK_SIZE) {
	/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return curSize + writeBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}
	return curSize;
}
