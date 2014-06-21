#ifndef IR_Uni_AUX3_h
#define IR_Uni_AUX3_h

#define IRNone 0xffff
// Universal Remote control
// TV code 159

#include <IRremote_INT.h>
#include "commands.h"

//#define IRADDR 0xF50A
// RC5
#define IRPROTO 3

#define IR_RPT_MSG 0xFFFFFFFF

// keys
#define IR_KB_POWER 0x0c
#define IR_KB_TV_VCR 0x38
#define IR_KB_CH_PC 0x2d
#define IR_KB_I_II 0x23
#define IR_KB_PAUSE 0x3c
#define IR_KB_PLAY 0x2b
#define IR_KB_REC 0x2c
#define IR_KB_STOP 0x2d
#define IR_KB_REW 0x3f
#define IR_KB_FF 0x2e
#define IR_KB_R 0x15
#define IR_KB_G 0x14
#define IR_KB_Y 0x13
#define IR_KB_B 0x12
#define IR_KB_Mute 0x0d
#define IR_KB_Ch_Up 0x20
#define IR_KB_Vol_Up 0x10
#define IR_KB_Ch_Down 0x21
#define IR_KB_Vol_Dn 0x11
#define IR_KB_1 0x01
#define IR_KB_2 0x02
#define IR_KB_3 0x03
#define IR_KB_4 0x04
#define IR_KB_5 0x05
#define IR_KB_6 0x06
#define IR_KB_7 0x07
#define IR_KB_8 0x08
#define IR_KB_9 0x09
#define IR_KB_0 0x00
#define IR_KB_11 0x0a
#define IR_KB_12 0x22

// keys to cmd
bool IR_check_RPT(unsigned long value);
bool IR_check_PROTO(unsigned long value);
bool IR_check_ADDRESS(unsigned long value);
unsigned int IR_checkCMD(unsigned long value);
_COMMAND IR_decode(unsigned int value);


#endif