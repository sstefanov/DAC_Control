#include <Arduino.h>
#include "commands.h"
#include "IR_Uni088.h"
// 088
//static unsigned int lastKey;

bool IR_check_RPT(unsigned long value) {
	return (value==IR_RPT_MSG ? true : false);
}

bool IR_check_PROTO(unsigned long value) {
	return (value==IRPROTO ? true : false);
}

bool IR_check_ADDRESS(unsigned long value){
        return ((value)>>16==IRADDR ? true : false);
}

unsigned int IR_checkCMD(unsigned long value) {
    int k=(int) value&0xFF;
    unsigned int b=(int) (value>>8)&0xFF;
    if ((k^b)!=0xFF){  // CRC Not OK
        return IRNone;
    }
    else {
        return k;
    }
}

//#define debug

_COMMAND IR_decode(unsigned int value) {
//    Serial.print("IR_decode = ");
//    Serial.println(value);
#ifdef DEBUGIR
	          Serial.print("IR value=0x");
	          Serial.println(value,HEX);
#endif

	if (value!=IRNone) {
//	    Serial.print("IR_decode value ");
//	      Serial.println(value, HEX);
		switch (value) {
	        case IR_KB_POWER:
#ifdef DEBUGIR
	          Serial.println("IR_KB_POWER");
#endif
	          return C_PWR;
	          break;
	        case IR_KB_R:
	          return C_SEL_IN_1;	// I2S 1
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_1");
#endif
	          break;
	        case IR_KB_G:
	          return C_SEL_IN_2;
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_2");
#endif
	          break;
	        case IR_KB_Y:
	          return C_SEL_IN_3;
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_3");
#endif
	          break;
	        case IR_KB_B:
	          return C_SEL_IN_4;
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_4");
#endif
	          break;
	        case IR_KB_0:
	          return C_SEL_IN_5;	// RPI
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_5");
#endif
	          break;
	        case IR_KB_11:
	          return C_SEL_IN_6;	// RPI
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_6");
#endif
	          break;
	        case IR_KB_12:
	          return C_SEL_IN_7;	// RPI
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_7");
#endif
	          break;
	        case IR_KB_1:
	          return C_SEL_IN_9;	// SPDIF 1
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_9");
#endif
	          break;
	        case IR_KB_2:
	          return C_SEL_IN_10;	// SPDIF 2
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_10");
#endif
	          break;
	        case IR_KB_3:
	          return C_SEL_IN_11;	// SPDIF 3
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_11");
#endif
	          break;
	        case IR_KB_4:
	          return C_SEL_IN_12;	// SPDIF 4
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_12");
#endif
	          break;
	        case IR_KB_5:
	          return C_SEL_IN_13;	// SPDIF 5
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_13");
#endif
	          break;
	        case IR_KB_6:
	          return C_SEL_IN_14;	// SPDIF 6
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_14");
#endif
	          break;
	        case IR_KB_7:
	          return C_SEL_IN_15;	// SPDIF 7
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_15");
#endif
	          break;
	        case IR_KB_8:
	          return C_SEL_IN_16;	// SPDIF 8
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_16");
#endif
	          break;
	        case IR_KB_REW:
	          return C_P_PREV_TRACK;
	//          Serial.println("Left B");
#ifdef DEBUGIR
		       Serial.println("C_P_PREV_TRACK");
#endif
	          break;
	        case IR_KB_PAUSE:
	          return C_P_PAUSE;
	//          Serial.println("Up");
#ifdef DEBUGIR
		       Serial.println("C_P_PAUSE");
#endif
	          break;
	        case IR_KB_PLAY:
	          return C_P_PLAY;
	//          Serial.println("Right");
#ifdef DEBUGIR
		       Serial.println("C_P_PLAY");
#endif
	          break;
	        case IR_KB_FF:
	          return C_P_NEXT_TRACK;
	//          Serial.println("Right B");
#ifdef DEBUGIR
		       Serial.println("C_P_NEXT_TRACK");
#endif
	          break;
	        case IR_KB_STOP:
	          return C_P_STOP;
	//          Serial.println("Down");
#ifdef DEBUGIR
		       Serial.println("C_P_STOP");
#endif
	          break;
	        case IR_KB_Mute:
	          return C_VOL_MUTE;
	//          Serial.println("Mute");
#ifdef DEBUGIR
		       Serial.println("C_VOL_MUTE");
#endif
	          break;
	        case IR_KB_Ch_Up:
	          return C_SEL_IN_NEXT;
	//          Serial.println("Ch Up");
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_NEXT");
#endif
	          break;
	        case IR_KB_Vol_Up:
	          return C_VOL_UP;
	//          Serial.println("Vol Up");
#ifdef DEBUGIR
		       Serial.println("C_VOL_UP");
#endif
	          break;
	        case IR_KB_Ch_Down:
	          return C_SEL_IN_PREV;
	//          Serial.println("Ch Down");
#ifdef DEBUGIR
		       Serial.println("C_SEL_IN_PREV");
#endif
	          break;
	        case IR_KB_Vol_Dn:
	          return C_VOL_DN;
	//          Serial.println("Vol Dn");
#ifdef DEBUGIR
		       Serial.println("C_VOL_DN");
#endif
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
