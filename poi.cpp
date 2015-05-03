#include "poi.hpp"

// Implementasi Inner class Entry

// ctor
POIFS::Entry::Entry() : entryPosition(0), off(0) {
	memset(blockEntry, 0, ENTRY_SIZE);
}

POIFS::Entry::Entry(ushort enPos, char offs) : entryPosition(enPos), off(offs) {
	target.seekg((BLOCK_SIZE * 257) + (entryPosition * BLOCK_SIZE) + (off * ENTRY_SIZE));
	target.read(blockEntry, ENTRY_SIZE);
}

// getter
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
	memcpy((char*)&_time, (blockEntry + 0x16), 2);
	return _time;
}

short POIFS::Entry::getDate(){
	short date;
	memcpy((char*)&date, blockEntry + 0x18, 2);
	return date;
}

short POIFS::Entry::getIndex(){
	short index;
	memcpy((char*)&index, blockEntry + 0x1A, 2);
	return index;
}

int POIFS::Entry::getSize(){
	int size;
	memcpy((char*)&size, blockEntry + 0x1C, 4);
	return size;
}

// entry methods
POIFS::Entry POIFS::Entry::getNextEntry() {
	if (off < 15)
		return Entry(entryPosition, off + 1);
	else
		return Entry(nextBlock[entryPosition], 0);
}

POIFS::Entry POIFS::Entry::getEntryfromPath(const char* path) {
	// find root path
	uint ed = 1;
	while (path[ed] != '/' && string(path).length() > ed)
		ed++;
	
	string rootPath = string(path + 1, ed - 1);
	
	// find the root directory's entry in entry
	while (getNama() != rootPath && entryPosition != END_BLOCK)
		*this = getNextEntry();
	
	// if root's entry is not found, pass empty entry
	if (isEmpty())
		return Entry();
	else {
		if (ed == string(path).length()) {
			return *this;
		}
		else {
			if (getAttribut() & 0x8) {
				// the path is a directory
				ushort idx;
				memcpy((char*)&idx, blockEntry + 0x1A, 2);
				Entry next(index, 0);

				return next.getEntryfromPath(path + ed);
			}
			else {
				// return empty entry
				return Entry();
			}
		}
	}
}

POIFS::Entry POIFS::Entry::getEmptyEntry() {
	Entry tmp(*this);
	
	while (!tmp.isEmpty())
		tmp = tmp.getNextEntry();
	
	if (tmp.entryPosition == END_BLOCK) {
		// if all blocks are full, allocate new empty block for it
		ushort newPos = allocateBlock();
		ushort lastPos = entryPosition;

		while (nextBlock[lastPos] != END_BLOCK)
			lastPos = nextBlock[lastPos];
		
		setNextBlock(lastPos, newPos);
		tmp.entryPosition = newPos;
		tmp.off = 0;
	}
	
	return tmp;
}

POIFS::Entry POIFS::Entry::getNewEntryfromPath(const char *path) {
	// find root path
	uint ed = 1;
	while (path[ed] != '/' && string(path).length() > ed)
		ed++;
	
	string rootPath = string(path + 1, ed - 1);
	
	// find the root directory's entry in entry
	Entry tmp(entryPosition, off);
	while (getNama() != rootPath && entryPosition != END_BLOCK)
		*this = getNextEntry();
	
	// if root's entry is not found, pass another new entry
	if (isEmpty()) {
		while (!tmp.isEmpty()) {
			if (tmp.getNextEntry().entryPosition == END_BLOCK)
				tmp = Entry(allocateBlock(), 0);
			else
				tmp = tmp.getNextEntry();
		}
		
		tmp.setNama(rootPath.c_str());
		tmp.setAttribut(0xF);
		tmp.setIndex(allocateBlock());
		tmp.setSize(BLOCK_SIZE);
		tmp.setTime(0);
		tmp.setDate(0);

		// write to file
		tmp.writeEntry();
		
		*this = tmp;
	}
	
	if (string(path).length() == ed)
		return *this;
	else {
		/* cek apakah direktori atau bukan */
		if (getAttribut() & 0x8) {
			ushort index;
			memcpy((char*)&index, blockEntry + 0x1A, 2);

			Entry next(index, 0);
			return next.getNewEntryfromPath(path + ed);
		}
		else
			return Entry();
	}
}

// setter
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

// fungsi dan method
bool POIFS::Entry::isEmpty(){
	return *(blockEntry) == 0;
}

void POIFS::Entry::makeEmpty(){
	memset(blockEntry, 0, ENTRY_SIZE);
}

time_t POIFS::Entry::getEntryTime() {
	uint dt;
	memcpy((char*)&dt, blockEntry + 0x16, 4);
	
	time_t tmp;
	time(&tmp);
	struct tm *res = localtime(&tmp);
	
	// get the time of the entry
	res->tm_sec = dt & 0x1F;
	res->tm_min = (dt >> 5u) & 0x3F;
	res->tm_hour = (dt >> 11u) & 0x1F;
	res->tm_mday = (dt >> 16u) & 0x1F;
	res->tm_mon = (dt >> 21u) & 0xF;
	res->tm_year = ((dt >> 25u) & 0x7F) + 10;
	
	time_t final = mktime(res);
	return final;
}

void POIFS::Entry::setCurrentTime() {
	time_t nowtime;
	time(&now);
	struct tm *now = localtime(&nowtime);
	
	int sc = now->tm_sec;
	int mn = now->tm_min;
	int hr = now->tm_hour;
	int d = now->tm_mday;
	int m = now->tm_mon;
	int y = now->tm_year;
	
	int curtime = (hr << 11) | (mn << 5) | (sc >> 1);
	int curdate = (d) | (m << 5) | ((y - 10) << 9);
	
	memcpy(blockEntry + 0x16, (char*)&curtime, 2);

	memcpy(blockEntry + 0x18, (char*)&curdate, 2);
}

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
