#ifndef IR_Uni_AUX3_h
#define IR_Uni_AUX3_h

// DVR Remote control

#include <IRremote.h>
#include "commands.h"

#define IRADDR 0x80ff
// NEC
#define IRPROTO 1


// keys
#define IR_KB_POWER 0xcf
#define IR_KB_PLAY 0x0f	// ENTER
#define IR_KB_STOP 0xbd
#define IR_KB_PAUSE 0xfd
#define IR_KB_REC 0x27
#define IR_KB_TV_VCR 0x07	//DEV
#define IR_KB_CH_PC 0x87	// A
#define IR_KB_I_II 0x57		// VOIP/mon
#define IR_KB_REW 0xaf
#define IR_KB_FF 0x2f
#define IR_KB_R 0x6d	// IRIS -
#define IR_KB_G 0xad	// FOCUS -
#define IR_KB_Y 0x2d	// ZOOM -
#define IR_KB_B 0xcd	// F2
#define IR_KB_Mute 0xc7	// PTZ
#define IR_KB_Ch_Up 0xa7	// PLAY
#define IR_KB_Vol_Up 0x3d
#define IR_KB_Ch_Down 0x47	// PREV
#define IR_KB_Vol_Dn 0xdd
#define IR_KB_1 0x7f
#define IR_KB_2 0xbf
#define IR_KB_3 0x3f
#define IR_KB_4 0xdf
#define IR_KB_5 0x5f
#define IR_KB_6 0x9f
#define IR_KB_7 0x1f
#define IR_KB_8 0xef
#define IR_KB_9 0x6f
#define IR_KB_0 0xff
#define IR_KB_11 0x37	// EDIT
#define IR_KB_12 0xd7	// ESC

// keys to cmd
bool IR_check_RPT(unsigned long value);
bool IR_check_PROTO(unsigned long value);
bool IR_check_ADDRESS(unsigned long value);
unsigned int IR_checkCMD(unsigned long value);
_COMMAND IR_decode(unsigned int value);


#endif