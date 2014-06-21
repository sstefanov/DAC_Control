#include "Arduino.h"
#include <EEPROM.h>
#ifndef EEPROM_C_H
#define EEPROM_C_H


#define PTRBUFSTART 0
#define PTRBUFEND(x) PTRBUFSTART+x-1
#define BBUFEND(x,y,z) PTRBUFEND(x)+y*z


class EEPROM_C {
// buffer pointers
	public:
		EEPROM_C(byte b, byte p, byte s);
		byte pptr;  // pointer to next element in pointer buffer
		byte bufaddr;  // address in data buffer for next element (read from pointer buffer)
		byte *bufelement;	// pointer to the current elememt array
		byte elementsize;	// dimension of the element
		byte pointersize;
		byte bufsize;
// find active pointer in pointer buffer 
		void FindPtr();
// read element from address in pointer buffer 
		void ReadBufElement();
// write element from address in pointer buffer 
		void WriteBufElement();
};

#endif

