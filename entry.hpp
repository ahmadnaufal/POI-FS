#ifndef ENTRY_H
#define ENTRY_H

#include "poi.hpp"

class Entry{
	public:
		// ctor
		Entry();
		Entry(ushort enPos, char offs);

		// entry attribute getter
		char* getNama();
		char getAttribut();
		short getTime();
		short getDate();
		short getIndex();
		int getSize();

		// entry method for the block data
		Entry getNextEntry();
		Entry getEntryfromPath(const char* path);
		Entry getEmptyEntry();
		Entry getNewEntryfromPath(const char* path);

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

		time_t getEntryTime();
		void setCurrentTime();	

		void writeEntry();

	/* attributes */
		char blockEntry[ENTRY_SIZE];	/* the size of the entry block */
		ushort entryPosition;			/* the position of the block */
		char off;						/* offset of the block (from 0-15) */
};

#endif
