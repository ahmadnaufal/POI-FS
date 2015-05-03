#include "poi.hpp"

POIFS::Entry::Entry(){
	memset(blockEntry,'\0',ENTRY_SIZE);
}

char* POIFS::Entry::getNama(){
	char* nama;
	memcpy(nama, blockEntry + 0x00, 21);
	return nama;
}

char POIFS::Entry::getAttribut(){
	return blockEntry[22];
}

short POIFS::Entry::getTime(){
	short _time;
	memcpy(_time, (short) &(blockEntry + 0x16), 2);
	return _time;
}

short POIFS::Entry::getDate(){
	short date;
	memcpy(date, (short) &(blockEntry + 0x18), 2);
	return date;
}

short POIFS::Entry::getIndex(){
	short index;
	memcpy(index, (short) &(blockEntry + 0x1A), 2);
	return index;
}

int POIFS::Entry::getSize(){
	int size;
	memcpy(size, (int) &(blockEntry + 0x1C), 4);
	return size;
}

void POIFS::Entry::setNama(const char* nama){
	memcpy(blockEntry + 0x00, nama, strlen(nama));
}

void POIFS::Entry::setAttribut(const char attribut){
	memcpy(blockEntry + 0x15, attribut, 1);
}

void POIFS::Entry::setTime(const short _time){
	memcpy(blockEntry + 0x16, (char*) &_time, 2);
}

void POIFS::Entry::setDate(const short date){
	memcpy(blockEntry + 0x18, (char*) &date, 2);
}

void POIFS::Entry::setIndex(const short index){
	memcpy(blockEntry + 0x1A, (char*) &index, 2);
}

void POIFS::Entry::setSize(const int size){
	memcpy(blockEntry + 0x1C, (char*) &size, 4);
}


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
	initVolumeInformation(filename, rootname);

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
	memcpy(buffer + 0x24, (char*)&blockCapacity, 4);

	availBlock = DATAPOOL_BLOCK_N;
	memcpy(buffer + 0x28, (char*)&availBlock, 4);

	/* initialize the first availBlock block */
	firstAvail = 1;
	memcpy(buffer + 0x2C, (char*)&firstAvail, 4);

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
	/* baca availBlock */
	memcpy((char*)&availBlock, buffer + 0x28, 4);
	/* baca firstAvail */
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
	ptr_block result = firstAvail;
	setNextBlock(result, END_BLOCK);
	while (nextBlock[firstAvail] != 0x0000) {
		firstAvail++;
	}
	availBlock--;
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
		availBlock--;
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
