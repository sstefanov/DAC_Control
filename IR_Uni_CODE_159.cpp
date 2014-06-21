#include <Arduino.h>
#include "commands.h"
#include "IR_Uni_CODE_159.h"

static unsigned int lastKey;

bool IR_check_RPT(unsigned long value) {
	bool r;
	unsigned int k=(unsigned int) value&0xfFF;
//	Serial.print("CHK RPT ");
//	Serial.print(k, HEX);
//	Serial.print(", ");
//	Serial.println(lastKey, HEX);
//	Serial.println(k^lastKey, HEX);
	r=(k==lastKey ? true : false);
/*	if (r) {
		Serial.println("RPT");
	}
	else {
		Serial.println("NO RPT");
	}
*/
	lastKey=k;
	return r;
}

bool IR_check_PROTO(unsigned long value) {
	return (value==IRPROTO ? true : false);
}

bool IR_check_ADDRESS(unsigned long value){
//	return ((value)>>16==a ? true : false);
	return true;
}

unsigned int IR_checkCMD(unsigned long value) {
    return (unsigned int) value&0x7FF;
}

//#define debug

_COMMAND IR_decode(unsigned int value) {
//    Serial.print("IR_decode = ");
//    Serial.println(value);

	if (value!=IRNone) {
			    Serial.print("IR_decode value ");
	          Serial.println(value, HEX);
		switch (value) {
	        case IR_KB_POWER:
//#ifdef debug
//	          Serial.println("IR_KB_POWER");
//#endif
	          return C_PWR;
	          break;
	        case IR_KB_R:
	          return C_SEL_IN_1;	// I2S 1
	//          Serial.println("CD");
	          break;
	        case IR_KB_G:
	          return C_SEL_IN_2;
	//          Serial.println("Aux1");
	          break;
	        case IR_KB_Y:
	          return C_SEL_IN_3;
	//          Serial.println("Aux2");
	          break;
	        case IR_KB_B:
	          return C_SEL_IN_4;
	//          Serial.println("Aux3");
	          break;
	        case IR_KB_0:
	          return C_SEL_IN_5;	// RPI
	//          Serial.println("Phone");
	          break;
	        case IR_KB_11:
	          return C_SEL_IN_6;	// RPI
	//          Serial.println("Phone");
	          break;
	        case IR_KB_12:
	          return C_SEL_IN_7;	// RPI
	//          Serial.println("Phone");
	          break;
	        case IR_KB_1:
	          return C_SEL_IN_9;	// SPDIF 1
	//          Serial.println("Phone");
	          break;
	        case IR_KB_2:
	          return C_SEL_IN_10;	// SPDIF 2
	//          Serial.println("Message");
	          break;
	        case IR_KB_3:
	          return C_SEL_IN_11;	// SPDIF 3
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_4:
	          return C_SEL_IN_12;	// SPDIF 4
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_5:
	          return C_SEL_IN_13;	// SPDIF 5
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_6:
	          return C_SEL_IN_14;	// SPDIF 6
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_7:
	          return C_SEL_IN_15;	// SPDIF 7
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_8:
	          return C_SEL_IN_16;	// SPDIF 8
	//          Serial.println("SRS ");
	          break;
	        case IR_KB_REW:
	          return C_P_PREV_TRACK;
	//          Serial.println("Left B");
	          break;
	        case IR_KB_PAUSE:
	          return C_P_PAUSE;
	//          Serial.println("Up");
	          break;
	        case IR_KB_PLAY:
	          return C_P_PLAY;
	//          Serial.println("Right");
	          break;
	        case IR_KB_FF:
	          return C_P_NEXT_TRACK;
	//          Serial.println("Right B");
	          break;
	        case IR_KB_STOP:
	          return C_P_STOP;
	//          Serial.println("Down");
	          break;
	        case IR_KB_Mute:
	          return C_VOL_MUTE;
	//          Serial.println("Mute");
	          break;
	        case IR_KB_Ch_Up:
	          return C_SEL_IN_NEXT;
	//          Serial.println("Ch Up");
	          break;
	        case IR_KB_Vol_Up:
	          return C_VOL_UP;
	//          Serial.println("Vol Up");
	          break;
	        case IR_KB_Ch_Down:
	          return C_SEL_IN_PREV;
	//          Serial.println("Ch Down");
	          break;
	        case IR_KB_Vol_Dn:
	          return C_VOL_DN;
	//          Serial.println("Vol Dn");
	          break;
	        default:
	//          Serial.println(results.value, HEX);
	//          Serial.println(value, HEX);
		        return C_NONE;
	    	    break;
		}
	}
	else {
		return C_NONE;
	}
}
//#undef debug
