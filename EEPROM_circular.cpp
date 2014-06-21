#include <LiquidCrystal.h>
#include "EEPROM_circular.h"

EEPROM_C::EEPROM_C(byte b, byte p, byte s) {
  //  init
  pptr=0;
  elementsize = b;
  pointersize = p;
  bufsize = s;
  bufelement = (byte *)malloc(sizeof(byte) * elementsize);
}

// find active pointer in pointer buffer 
void EEPROM_C::FindPtr(){
  byte p = PTRBUFSTART;
  byte i = p+1;
  byte d1 = 0;
  byte data;
    do {
    	data=EEPROM.read(i);
        if (data <= bufsize) {
            d1 = data - EEPROM.read(p);
            if (d1 == 1) {
                p++;
                if (p > PTRBUFEND(pointersize)) {
                    p = PTRBUFSTART;
                }
                i++;
                if (i > PTRBUFEND(pointersize)) {
                    i = PTRBUFSTART;
                }
            }
        }
        else {
            d1 = 0;
           	data=EEPROM.read(p);
            if ((p == PTRBUFSTART) && (data>=bufsize)) {
                p = PTRBUFEND(pointersize);
            }
        }
    }
    while (!((p > PTRBUFEND(pointersize)) || (d1 != 1) ));
    p++;
    if (p > PTRBUFEND(pointersize)) {
        p = PTRBUFSTART;
    }
    pptr = p;
}

// read element from address in pointer buffer 
void EEPROM_C::ReadBufElement() {
  byte a;
  byte p=pptr;
  if (p==PTRBUFSTART){
    p=PTRBUFEND(pointersize);
  }
  else {
    p--;
  }
  bufaddr=EEPROM.read(p);
  a=PTRBUFEND(pointersize)+1+bufaddr*3;
  for (byte i=0; i<elementsize; i++) {
    bufelement[i]=EEPROM.read(a);
    a++;
    if (a > BBUFEND(pointersize,bufsize,elementsize)) {
         a=PTRBUFEND(pointersize)+1;
    }
  }
  bufaddr++;
  if (bufaddr >= bufsize) {
    bufaddr = 0;
  }
}

// write element from address in pointer buffer 
void EEPROM_C::WriteBufElement() {
  byte i;
  byte a;
  a=PTRBUFEND(pointersize)+1+bufaddr*3;
  for (i=0; i<elementsize; i++) {
    EEPROM.write(a,bufelement[i]);
    a++;
    if (a > BBUFEND(pointersize,bufsize,elementsize)) {
         a=PTRBUFEND(pointersize)+1;
    }
  }
  EEPROM.write(pptr,bufaddr);
  bufaddr++;
  if (bufaddr >= bufsize) {
    bufaddr = 0;
  }
  pptr++;
  if (pptr > PTRBUFEND(pointersize)) {
    pptr = PTRBUFSTART;
  }
}
