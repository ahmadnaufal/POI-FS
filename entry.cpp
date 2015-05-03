#include "entry.hpp"
#include "poi.hpp"

// Implementasi Inner class Entry
POIFS POI;

// ctor
Entry::Entry() : entryPosition(0), off(0) {
	memset(blockEntry, 0, ENTRY_SIZE);
}

Entry::Entry(ushort enPos, char offs) : entryPosition(enPos), off(offs) {
	target.seekg((BLOCK_SIZE * 257) + (entryPosition * BLOCK_SIZE) + (off * ENTRY_SIZE));
	target.read(blockEntry, ENTRY_SIZE);
}

// getter
char* Entry::getNama(){
	char* nama;
	memcpy(nama, blockEntry + 0x00, 21);
	return nama;
}

char Entry::getAttribut(){
	return blockEntry[22];
}

short Entry::getTime(){
	short _time;
	memcpy((char*)&_time, (blockEntry + 0x16), 2);
	return _time;
}

short Entry::getDate(){
	short date;
	memcpy((char*)&date, blockEntry + 0x18, 2);
	return date;
}

short Entry::getIndex(){
	short index;
	memcpy((char*)&index, blockEntry + 0x1A, 2);
	return index;
}

int Entry::getSize(){
	int size;
	memcpy((char*)&size, blockEntry + 0x1C, 4);
	return size;
}

// entry methods
Entry Entry::getNextEntry() {
	if (off < 15)
		return Entry(entryPosition, off + 1);
	else
		return Entry(nextBlock[entryPosition], 0);
}

Entry Entry::getEntryfromPath(const char* path) {
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

Entry Entry::getEmptyEntry() {
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

Entry Entry::getNewEntryfromPath(const char *path) {
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
void Entry::setNama(const char* nama){
	memcpy(blockEntry + 0x00, nama, strlen(nama));
}

void Entry::setAttribut(const char attribut){
	memcpy(blockEntry + 0x15, attribut, 1);
}

void Entry::setTime(const short _time){
	memcpy(blockEntry + 0x16, (char*) &_time, 2);
}

void Entry::setDate(const short date){
	memcpy(blockEntry + 0x18, (char*) &date, 2);
}

void Entry::setIndex(const short index){
	memcpy(blockEntry + 0x1A, (char*) &index, 2);
}

void Entry::setSize(const int size){
	memcpy(blockEntry + 0x1C, (char*) &size, 4);
}

// fungsi dan method
bool Entry::isEmpty(){
	return *(blockEntry) == 0;
}

void Entry::makeEmpty(){
	memset(blockEntry, 0, ENTRY_SIZE);
}

time_t Entry::getEntryTime() {
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

void Entry::setCurrentTime() {
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
