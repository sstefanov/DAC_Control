#ifndef IR_Uni_AUX3_h
#define IR_Uni_AUX3_h

#define IRNone 0xffff
// AZtech sat receiver remote
// TV code 088

#include <IRremote.h>
#include "commands.h"

#define IRADDR 0x0008
// NEC
#define IRPROTO 1

#define IR_RPT_MSG 0xFFFFFFFF

// keys
#define IR_KB_POWER 0xff
#define IR_KB_TV_VCR 0xbf
#define IR_KB_CH_PC 0x4f	// notes
#define IR_KB_I_II 0x67		// timer
#define IR_KB_PAUSE 0x17	// ?
#define IR_KB_PLAY 0x27		// EPG
#define IR_KB_REC 0x7d		// RCL
#define IR_KB_STOP 0x37		// OK
#define IR_KB_REW 0xb7		// Left
#define IR_KB_FF 0xd7		// Right
#define IR_KB_R 0xc7
#define IR_KB_G 0x47
#define IR_KB_Y 0xa7
#define IR_KB_B 0x87
#define IR_KB_Mute 0xe7
#define IR_KB_Ch_Up 0xf7
#define IR_KB_Vol_Up 0x07
#define IR_KB_Ch_Down 0x0f
#define IR_KB_Vol_Dn 0xfd
#define IR_KB_1 0x3f
#define IR_KB_2 0xdf
#define IR_KB_3 0x5f
#define IR_KB_4 0x9f
#define IR_KB_5 0x1f
#define IR_KB_6 0xef
#define IR_KB_7 0x6f
#define IR_KB_8 0xaf
#define IR_KB_9 0x2f
#define IR_KB_0 0xcf
#define IR_KB_11 0x77			// Up
#define IR_KB_12 0x57			// Dn

#define IR_KB_EXIT 0x97

// keys to cmd
bool IR_check_RPT(unsigned long value);
bool IR_check_PROTO(unsigned long value);
bool IR_check_ADDRESS(unsigned long value);
unsigned int IR_checkCMD(unsigned long value);
_COMMAND IR_decode(unsigned int value);


#endif