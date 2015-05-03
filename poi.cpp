#include "poi.hpp"

// Constructor & Destructor
POIFS::POIFS() {
	time(&mount_time);
}
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
	if (mountname == NULL)
		fname = "POI!";
	else
		fname = mountname;
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
	rootdir = rootname;
	memcpy(buffer + 0x30, rootdir, 32); // BELUM SELESAI

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
		target.write(buffer, BLOCK_SIZE);
}

/* Read .poi */
void readPoi(const char *filename){
	target.open(filename, fstream::in | fstream::out | fstream::binary);
	/* cek apakah file ada */
	try {
		target.open(filename, fstream::in | fstream::out | fstream::binary);
	}
	catch(exception& e){
		target.close();
		throw runtime_error("File not found");
		cout << e.what();
	}
	/* periksa Volume Information */
	readVolumeInformation();
	/* baca Allocation Table */
	readAllocationTable();
}

/* Read the .poi volume information */
void readVolumeInformation(){
	char buffer[BLOCK_SIZE];
	target.seekg(0);

	target.read(buffer, BLOCK_SIZE);

	if (string(buffer, 4) != "poi!") {
		target.close();
		throw runtime_error("File is not a valid POI file");
	}
	/* baca capacity */
	memcpy((char*)&blockCapacity, buffer + 0x24, 4);
	/* baca available */
	memcpy((char*)&availBlock, buffer + 0x28, 4);
	/* baca firstEmpty */
	memcpy((char*)&firstAvail, buffer + 0x2C, 4);
}

/* Read the .poi allocation table */
void readAllocTable(){
	char buffer[3];
	/* pindah posisi ke awal Allocation Table */
	target.seekg(0x200);
	/* baca nilai nextBlock */
	for (int i = 0; i < N_BLOCK; i++) {
		target.read(buffer, 2);
		memcpy((char*)&nextBlock[i], buffer, 2);
	}
}

void writeVolumeInformation(){
	target.seekp(0x00);
	/* buffer untuk menulis ke file */
	char buffer[BLOCK_SIZE];
	memset(buffer, 0, BLOCK_SIZE);
	/* Magic string "CCFS" */
	memcpy(buffer + 0x00, "poi!", 4);
	/* Nama volume */
	memcpy(buffer + 0x04, filename.c_str(), filename.length());
	/* Kapasitas filesystem, dalam little endian */
	memcpy(buffer + 0x24, (char*)&blockCapacity, 4);
	/* Jumlah blok yang belum terpakai, dalam little endian */
	memcpy(buffer + 0x28, (char*)&availBlock, 4);
	/* Indeks blok pertama yang bebas, dalam little endian */
	memcpy(buffer + 0x2C, (char*)&firstAvail, 4);
	/* String "!iop" */
	memcpy(buffer + 0x1FC, "!iop", 4);
	handle.write(buffer, BLOCK_SIZE);
}

void writeAllocationTable(ptr_block position){
	target.seekp(BLOCK_SIZE + sizeof(ptr_block) * position);
	target.write((char*)&nextBlock[position], sizeof(ptr_block));
}

/* bagian alokasi block */
void setNextBlock(ptr_block position, ptr_block next){
	nextBlock[position] = next;
	writeAllocationTable(position);
}

ptr_block allocateBlock(){
	ptr_block result = firstEmpty;
	setNextBlock(result, END_BLOCK);
	while (nextBlock[firstEmpty] != 0x0000) {
		firstEmpty++;
	}
	available--;
	writeVolumeInformation();
	return result;
}

void freeBlock(ptr_block position){
	f (position == EMPTY_BLOCK) {
	return;
	}
	while (position != END_BLOCK) {
		ptr_block temp = nextBlock[position];
		setNextBlock(position, EMPTY_BLOCK);
		position = temp;
		available--;
	}
	writeVolumeInformation();
}

/* bagian baca/tulis block */
int readBlock(ptr_block position, char *buffer, int size, int offset = 0){
	/* kalau sudah di END_BLOCK, return */
	if (position == END_BLOCK) {
		return 0;
	}
	/* kalau offset >= BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		return readBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}
	target.seekg(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset);
	int size_now = size;
	/* cuma bisa baca sampai sebesar block size */
	if (offset + size_now > BLOCK_SIZE) {
		size_now = BLOCK_SIZE - offset;
	}
	handle.read(buffer, size_now);
	/* kalau size > block size, lanjutkan di nextBlock */
	if (offset + size > BLOCK_SIZE) {
		return size_now + readBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}
	return size_now;
}
int writeBlock(ptr_block position, const char *buffer, int size, int offset = 0){
	/* kalau sudah di END_BLOCK, return */
	if (position == END_BLOCK) {
		return 0;
	}
	/* kalau offset >= BLOCK_SIZE */
	if (offset >= BLOCK_SIZE) {
		/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return writeBlock(nextBlock[position], buffer, size, offset - BLOCK_SIZE);
	}
	handle.seekp(BLOCK_SIZE * DATA_POOL_OFFSET + position * BLOCK_SIZE + offset);
	int size_now = size;
	if (offset + size_now > BLOCK_SIZE) {
		size_now = BLOCK_SIZE - offset;
	}
	handle.write(buffer, size_now);
	/* kalau size > block size, lanjutkan di nextBlock */
	if (offset + size > BLOCK_SIZE) {
	/* kalau nextBlock tidak ada, alokasikan */
		if (nextBlock[position] == END_BLOCK) {
			setNextBlock(position, allocateBlock());
		}
		return size_now + writeBlock(nextBlock[position], buffer + BLOCK_SIZE, offset + size - BLOCK_SIZE);
	}
	return size_now;
}

/******* NOT FINISHED YET *******/
