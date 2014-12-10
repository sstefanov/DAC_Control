#ifndef IR_Uni_h
#define IR_Uni_h

#include <IRremote.h>
#include "commands.h"

#define IRNone 0xffff
#define IR_RPT_MSG 0xFFFFFFFF

// keys to cmd
bool IR_check_RPT(unsigned long value);
bool IR_check_PROTO(unsigned long value);
bool IR_check_ADDRESS(unsigned long value);
unsigned int IR_checkCMD(unsigned long value);
_COMMAND IR_decode(unsigned int value);

#endif