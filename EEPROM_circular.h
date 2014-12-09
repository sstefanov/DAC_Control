#include "Arduino.h"
#include <EEPROM.h>
#ifndef EEPROM_C_H
#define EEPROM_C_H


//#define PTRBUFSTART 0
//#define PTRBUFEND(x) PTRBUFSTART+x-1
//#define BBUFEND(x,y,z) PTRBUFEND(x)+y*z

// EEPROM structure:
// 0 - (pointerbufsize-1)	- pointer buffer
// pointerbufsize - EESIZE 	- data buffer
// Number of elemnts in buffer:
// bufsize=(EESIZE-PTRBUFSIZE)/ELEMENTSIZE


class EEPROM_C {
// buffer pointers
	public:
		EEPROM_C(byte b, byte p, byte s);
		byte pptr;  		// index of next element in pointer buffer
//		unsigned int bufaddr;  // address in data buffer for next element (read from pointer buffer)
		byte *bufelement;	// pointer to the current elememt array
		byte elementsize;	// size of one element
		byte bufsize;		// size (elements count) of buffer
		byte pointerbufsize;	// size (elements count) of pointer buffer
		byte indexbuffer;	// index of buffer element
// find active pointer in pointer buffer 
		void FindPtr();
// read element from address in pointer buffer 
		void ReadBufElement();
// write element from address in pointer buffer 
		void WriteBufElement();
};

#endif

