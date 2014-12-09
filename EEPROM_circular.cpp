#include <LiquidCrystal.h>
#include "EEPROM_circular.h"

//#define DEBUGEEPROM

EEPROM_C::EEPROM_C(byte b, byte p, byte s) {
  //  init
  pptr=0;
  elementsize = b;
  pointerbufsize = p;
  bufsize = s;
  bufelement = (byte *)malloc(sizeof(byte) * elementsize);
}

// find active pointer in pointer buffer
// pointers are indexes of the last element in the buffer 
void EEPROM_C::FindPtr(){
  byte p = 0;
  byte i = p+1;
  byte data,d1,d0;
#ifdef DEBUGEEPROM
  Serial.println("EEPROM_C FindPtr");
#endif    
    do {
    	data=EEPROM.read(i);
      if (data <= bufsize) {
            d0=EEPROM.read(p);
            d1 = data - d0;
/*
#ifdef DEBUGEEPROM
  Serial.print("i=");
  Serial.print(i);
  Serial.print(", p=");
  Serial.print(p);
  Serial.print(", data=");
  Serial.println(data);
  Serial.print(", d0=");
  Serial.print(d0);
  Serial.print(", d1=");
  Serial.println(d1);
#endif    
*/
            if (d1 == 1) {
                p++;
                if (p > pointerbufsize) {
                    p = 0;
                }
                i++;
                if (i > pointerbufsize) {
                    i = 0;
                }
            }
        }
        else {
            d1 = 0;
           	data=EEPROM.read(p);
            if ((p == 0) && (data>=bufsize)) {
                p = pointerbufsize;
            }
        }
    }
    while (!((p > pointerbufsize) || (d1 != 1) ));
    p++;
    if (p > pointerbufsize) {
        p = 0;
    }
//    pptr = d0;
    pptr = p;
    indexbuffer=d0;
/*
#ifdef DEBUGEEPROM
  Serial.print("   pptr=");
  Serial.println(pptr);
  Serial.println("PTR buffer:");
  for (i=0;i<pointerbufsize;i++) {
    data=EEPROM.read(i);
    Serial.print(data);
    Serial.print(", ");
  }
  Serial.println("");
#endif    
*/
}

// read element[pptr-1] 
void EEPROM_C::ReadBufElement() {
  unsigned int a;
/*  byte p=pptr;
  if (p==0){
    p=pointerbufsize;
  }
  else {
    p--;
  }*/
  a=pointerbufsize+indexbuffer*elementsize;  // address in EEPROM
#ifdef DEBUGEEPROM
  Serial.print("EEPROM_C ReadBufElement[");
  Serial.print(indexbuffer);
  Serial.println("]");
//  Serial.print("   addr 0x");
//  Serial.println(a, HEX);  
//  Serial.print(" buffer (0x) ");
#endif

//  a=pointerbufsize+1+bufaddr*3;
  for (byte i=0; i<elementsize; i++) {
    bufelement[i]=EEPROM.read(a);

#ifdef DEBUGEEPROM
      if (bufelement[i]<10) {
        Serial.print("0");
      }
      Serial.print(bufelement[i], HEX);
      Serial.print(" ");
#endif    

    a++;
/*    if (a > BBUFEND(pointersize,bufsize,elementsize)) {
         a=pointerbufsize+1;
    }*/
  }
#ifdef DEBUGEEPROM
  Serial.println();
#endif    

/*  bufaddr++;
  if (bufaddr >= bufsize) {
    bufaddr = 0;
  }
*/
}

// write element[pptr] 
void EEPROM_C::WriteBufElement() {
  byte i,data;
  unsigned int a;

  indexbuffer++;
  if (indexbuffer>bufsize)
    indexbuffer=0;
  EEPROM.write(pptr,indexbuffer); // save current element index

// write buffer
  a=pointerbufsize+indexbuffer*elementsize;
#ifdef DEBUGEEPROM
  Serial.print("EEPROM_C WriteBufElement[");
  Serial.print(indexbuffer);
  Serial.println("]");
//  Serial.print("   addr 0x");
//  Serial.println(a, HEX);  
//  Serial.print(" buffer (0x) ");
#endif
  for (i=0; i<elementsize; i++) {
#ifdef DEBUGEEPROM
      if (bufelement[i]<16) {
        Serial.print("0");
      }
      Serial.print(bufelement[i], HEX);
      Serial.print(" ");
#endif    
    if (EEPROM.read(a) != bufelement[i]) {
      EEPROM.write(a,bufelement[i]);
    }
    a++;
/*    if (a > BBUFEND(pointersize,bufsize,elementsize)) {
         a=pointerbufsize+1;
    }*/
  }

// next pptr
  pptr++;
  if (pptr > pointerbufsize) {
    pptr = 0;
  }
/*
#ifdef DEBUGEEPROM
  Serial.println();
  Serial.print("pptr=");
  Serial.println(pptr);  
  Serial.println("PTR buffer:");
  for (i=0;i<pointerbufsize;i++) {
    data=EEPROM.read(i);
    Serial.print(data);
    Serial.print(", ");
  }
  Serial.println("");
#endif    
#ifdef DEBUGEEPROM
  Serial.println();
#endif    
*/
}
