// DAC_Control.ino
#include <TimerOne.h>
//#include <LiquidCrystal.h>
#include <IRremote.h>
#include <EEPROM.h>
#include <I2C.h>

#include <ClickEncoder.h>
#include <Wire.h>
#include <ST7032.h>

#include "I_O.h"
#include "EEPROM_circular.h"

//#include "IR_Packard_Bell.h"
#include "IR_Uni088.h"
#include "commands.h"

#include "TCA6424A/TCA6424A.h"
#include "ArduinoWrapper.h"
#include "I2Cdev.h"
//#include "msp430_i2c.h"

#define T1TICKS  5  // 5 ticks for 10 ms
#define LED_FADE  600   // 6s fade - start of fading, must be greater of FADESTEPS
#define FADEVALUE 1
#define FADEMIN 0
#define FADEMAX 255
//#define FADESTEPS 256/FADEVALUE

//#define RPI_PRESENT

//#define DEBUGSERIAL
//#define DEBUGSERIALCMD
//#define DEBUGSERIAL2
#define DEBUGIR
#define DEBUGCMD
//#define DEBUGEEPROM
//#define DEBUGIR
//#define DEBUGENC
//#define DEBUGRELAY
//#define DEBUGINPUTS



// delay 62.5ns on a 16MHz AtMega
#define NOP __asm__ __volatile__ ("nop\n\t")
#define pulse_pin(x)    delayMicroseconds(1); digitalWrite(x, HIGH); delayMicroseconds(1); digitalWrite(x, LOW);

// constants

// EEPROM structure
// 1k for Atmega328

// 1 byte for selected input
// for each input - 2 bytes for volume

#define EESIZE 1024 // for atmega328

#define ELEMENTSIZE (MAX_IN_ANALOG+MAX_IN_SPDIF+MAX_IN_I2S+1)*2+1
#define PTRBUFSIZE 32 // 2 bytes for each
//#define BBUFSIZE (EESIZE-PTRBUFSIZE*sizeof(unsigned int))/ELEMENTSIZE
#define BBUFSIZE (EESIZE-PTRBUFSIZE)/ELEMENTSIZE-1

#define DELAY1  200
#define DELAY2  2

#define vol_RPT	50
#define MAXATT  63

// EEPROM addresses
#define EEADDR_Input  0
//#define EEADDR_Volume_L	1
//#define EEADDR_Volume_R 2

// bytes for EEPPROM
#define PARAMS 0
#define P_VOL_L 1
//#define P_VOL_R 2

#define btnRIGHT 1
#define btnUP 2
#define btnDOWN 3
#define btnLEFT 4
#define btnSELECT 5
#define btnNONE 0
#define btnReleased 128

// variables

int PWR_ON=0;
TCA6424A i2c_p1(0x22);
IRrecv irrecv(IR_RECV_PIN);
EEPROM_C eec(ELEMENTSIZE,PTRBUFSIZE,BBUFSIZE);
static unsigned int oldIRKey;
static unsigned int IRKey=IRNone;
static boolean IR_Recv;
static boolean IR_Pressed;
decode_results results;
unsigned int IR_TimerCount=0;  // counter
unsigned int LCD_TimerCount=0;  // lcd counter
int fader;
unsigned long time=0;
_COMMAND cmd=C_NONE;
_COMMAND oldcmd=C_NONE;
bool PWR_STAT=false;
bool EEPROM_Save=false; // flag to save to EEPROM

// Raspberry PI
bool RPI_WaitAnswer;
bool RPI_Ready;

bool I2C_Present=false;
bool fade;

//rotary encoder
ClickEncoder *encoder;
bool sel_in_mode=false,sel_in_led=false,in_transition=false;
int16_t enc_last, enc_value, enc_v;
unsigned int sel_in_timer=0;

// serial input
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

byte vol_l; // L volume
//byte vol_r; // R volume
byte sel_in;  // selected input
byte curvol_l; // current L volume
//byte curvol_r; // current R volume
byte cur_in; // current selected input
ST7032 lcd;

unsigned int lcd_key = 0;

// read serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
      inputString += inChar;
    if (inChar == '\n' || inChar == '!') {
      stringComplete = true;
    }
  }
}

// read the buttons
int read_LCD_buttons() {
  int adc_key_in;
  adc_key_in = analogRead(0);
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50) return btnRIGHT;
  if (adc_key_in < 195) return btnUP;
  if (adc_key_in < 380) return btnDOWN;
  if (adc_key_in < 555) return btnLEFT;
  if (adc_key_in < 790) return btnSELECT;
  return btnNONE; // when all others fail, return this...
}

int read_KBD() {
  static int oldKey;
  int Key;
  Key=read_LCD_buttons();
  if (Key==btnNONE) {  // not pressed
    if (oldKey!=btnNONE) { // was pressed - release
      Key=oldKey+btnReleased;
    }
    oldKey=btnNONE;
    return Key;
  }
  else {
    if (Key!=oldKey) {  // pressed key
      oldKey=Key;
      return Key;
    }
    else {
      return btnNONE;
    }
  }
}

void LCD_Light(bool f=true) {  // light LCD
  fader=FADEMAX;
  LCD_TimerCount=0; // start
  fade=f;
}

void IRLED_Blink() {
  #ifdef DEBUGCMD
      Serial.println("IRLED_Blink IR_Pressed IR_Recv");
  #endif
  Serial.print("^");
  IR_TimerCount=0;
  IR_Recv=true;
  IR_Pressed=true;

  if (PWR_STAT) {
    digitalWrite(IR_LED_PIN, HIGH); 
  }
  else {
    digitalWrite(IR_LED_PIN, LOW); 
  }
}

void timerIsr() {
  static byte t1tick;
  encoder->service();
  if (sel_in_mode) {
    sel_in_timer++;
    if (sel_in_timer>SELINDELAY) {
      sel_in_timer=0;
      sel_in_mode=false;
    }
  }
  if (t1tick<T1TICKS)
    t1tick++;
  else {
    t1tick=0;
    if (IR_TimerCount>=5) {  // end of blink led
//      Serial.print(".");
      if (PWR_STAT) {
        digitalWrite(IR_LED_PIN, LOW); 
      }
      else {
        digitalWrite(IR_LED_PIN, HIGH); 
      }
    }
    if (IR_Recv) {
      IR_TimerCount++;
    }
    if (IR_TimerCount>20) { // end of IR receiving
      IR_Recv=false;
      IR_TimerCount=0;  
//      oldcmd=C_NONE;
    }

  // LCD light timer
    if (PWR_STAT) {
      if (LCD_TimerCount<LED_FADE) {  // wait 
        LCD_TimerCount++;
      }
      else {  // fade in process
        if (fader> FADEMIN + FADEVALUE) {
          fader-=FADEVALUE;
          fade=true;
        }
        else if (fader>FADEMIN) {
          fader=FADEMIN;
          fade=true;
        }
      }
    }
  }
}

// read encoder
_COMMAND read_ENC() {
  static bool enc_btn, enc_rot;
  ClickEncoder::Button enc_b = encoder->getButton();
  if (!enc_btn && (enc_b == ClickEncoder::Held) ) {
#ifdef DEBUGENC
    Serial.println("ClickEncoder::Held");
#endif
    enc_btn=true;
    return C_PWR;
  }
  if (!enc_btn && (enc_b == ClickEncoder::DoubleClicked)) { //select input
    enc_btn=true;
    sel_in_mode=!sel_in_mode;
#ifdef DEBUGENC
    Serial.println("ClickEncoder::DoubleClicked");
#endif
  }
  if (enc_btn && (enc_b==ClickEncoder::Open)) {
#ifdef DEBUGENC
    Serial.println("ClickEncoder C_K_KEYRELEASED");
#endif
    enc_btn=false;
    return C_K_KEYRELEASED;
  }

  enc_v = encoder->getValue();  
  enc_value += enc_v;

  if (enc_value-enc_last>ENC_STEPS || enc_last-enc_value>ENC_STEPS) {
#ifdef DEBUGENC
    Serial.print("Encoder Value: ");
    Serial.println(enc_value);
    Serial.print("Encoder getValue: ");
    Serial.println(enc_v);
#endif
    enc_rot=true;
    enc_last = enc_value;
    if (sel_in_mode)
    // select input
      if (enc_v<0) {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_SEL_IN_PREV");
#endif
        return C_SEL_IN_PREV;
      }
      else if (enc_v>0) {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_SEL_IN_NEXT");
#endif
        return C_SEL_IN_NEXT;
      }
      else {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_NONE");
#endif
       return C_NONE;
      }
    else
    // set volume
      if (enc_v<0) {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_VOL_DN");
#endif
        return C_VOL_DN;
      }
      else if (enc_v>0) {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_VOL_UP");
#endif
        return C_VOL_UP;
      }
      else {
#ifdef DEBUGENC
        Serial.println("ClickEncoder C_NONE");
#endif
        return C_NONE;
      }
  }   
  else {
    if (enc_rot) {
#ifdef DEBUGENC
      Serial.println("ClickEncoder C_K_KEYRELEASED");
#endif
      enc_rot=false;
      return C_K_KEYRELEASED;
    }
  }
  return C_NONE;
}

unsigned int read_IR() {
  unsigned long v;  // place for the value
  int t;
  if (irrecv.decode(&results)) {
    irrecv.resume(); // Receive the next value
    t=results.decode_type;
  
  #ifdef DEBUGIR0
    Serial.print("read_IR, value, 0x");
    Serial.println(results.value, HEX); 
    Serial.print("read_IR, type ");
    Serial.println(t);
  #endif

    if (t==-1) {
      return IRNone;
    }
    v=results.value;
    IR_TimerCount=0;
    if (PWR_STAT && IR_check_RPT(v) && (oldcmd!=C_NONE)) {  // repeat command
#ifdef DEBUGIR
        Serial.print("IRKey=IR_RPT_MSG");
        Serial.println(IRKey);
#endif
      IRKey=IR_RPT_MSG;  // wait to stop repeat, no new key is received
      IRLED_Blink();
    }
    else if (IR_check_PROTO(t)) {
      if (IR_check_ADDRESS(v)) {
        IRKey=IR_checkCMD(v);
        if (IRKey!=IRNone) {
          oldIRKey=IRKey;  // store for repeat calculation
          IRLED_Blink();
        }
#ifdef DEBUGIR
        Serial.print("read_IR, key ");
        Serial.println(IRKey);
#endif
      }
      else {
        IRKey=IRNone;
      }
    }
  }
  else {  
    IRKey=IRNone;   
  } 
  return IRKey;
}

_COMMAND KBD_Decode(int value){
  if (value==btnNONE){
    return C_NONE;
  }
  else {
    switch (value) {
      case btnRIGHT:
        cmd=C_K_RIGHT;
        break;
      case btnLEFT:
        cmd=C_K_LEFT;
        break;
      case btnUP:
        cmd=C_K_UP;
        break;
      case btnDOWN:
        cmd=C_K_DOWN;
        break;
      case btnSELECT:
        cmd=C_PWR;
        break;
      case btnSELECT+btnReleased:
      case btnRIGHT+btnReleased:
      case btnLEFT+btnReleased:
      case btnUP+btnReleased:
      case btnDOWN+btnReleased:
        cmd=C_K_KEYRELEASED;
        break;
    }
  }
}


void SetVolume (void) {
#ifdef DEBUGINPUTS
  return;
#endif

	uint8_t t;
  byte tl,o;
//  byte tr;
#ifdef DEBUGSERIAL
    Serial.print("\nSetVolume\ncurvol_l=");
    Serial.println(curvol_l);
#endif
#ifdef DEBUGSERIAL2
    Serial.print(", vol_l=");
    Serial.print(vol_l);
    Serial.print(", cur_in=");
    Serial.print(cur_in);
    Serial.print(", sel_in=");
    Serial.println(sel_in);
    Serial.print("EEPROM_Save = ");
    Serial.println (EEPROM_Save ? "true" : "false");
#endif
/*
  vol_r=vol_l;
  curvol_r=curvol_l;
*/
  if ((sel_in>>4)==T_IN_LAN) { // selected RPI
// all is set from PRI
    if (vol_l > 100) {
      vol_l=100;
  //    vol_r=vol_l;
    }
    if (cur_in=sel_in) {
      delay(vol_RPT);
    }
    else {
      delay(vol_RPT/8);      // faster when input is switched
    }
  }
  else {
  	if (vol_l > MAXATT){ // || vol_r > MAXATT) {
#ifdef DEBUGSERIAL2
      Serial.print("wrong volume=");
      Serial.println(vol_l);
      Serial.print(", set to 0");
#endif
      vol_l=0;
//  		vol_r=vol_l;
  	}
#ifdef DEBUGSERIAL2
    Serial.print("before change EEPROM_Save = ");
    Serial.println (EEPROM_Save ? "true" : "false");
#endif
    EEPROM_Save=false; // don't save while cycling
  	while ((curvol_l != vol_l)) { // || (curvol_r != vol_r)) {
  		tl=curvol_l;
//  		tr=curvol_r;

#ifdef DEBUGSERIAL2
    Serial.print("current volume=");
    Serial.print(curvol_l);
    Serial.print(", target volume=");
    Serial.println(vol_l);
#endif

  		if (curvol_l < vol_l) {
  			curvol_l++;
#ifdef DEBUGSERIAL2
        Serial.println("curvol_l++ ");
#endif
  		}
  		else if (curvol_l > vol_l) {
  			curvol_l--;
#ifdef DEBUGSERIAL2
        Serial.println("curvol_l--");
#endif
  		}
/*
  		if (curvol_r < vol_r) {
  			curvol_r++;
#ifdef DEBUGSERIAL2
      Serial.println("curvol_r++ ");
#endif
      }
  		else if (curvol_r > vol_r) {
  			curvol_r--;
#ifdef DEBUGSERIAL2
        Serial.println("curvol_r--");
#endif
  		}
*/
      LCD_Light();

//make-before-break
// set all ones
      if (I2C_Present) {
        t=i2c_p1.readBank(VOLUMEBANK); // read port status in temp byte
#ifdef INVERTED_RELAY      
        t|=MASK_VOL; // set relay bytes
        t&=~((curvol_l & tl) & MASK_VOL);
#else
        t&=~MASK_VOL; // clear relay bytes
        t|=(curvol_l & tl) & MASK_VOL;
#endif
        i2c_p1.writeBank(VOLUMEBANK, t);
        delay(5);
// set all
        t&=~MASK_VOL;
#ifdef INVERTED_RELAY      
        t|=MASK_VOL; // set relay bytes
        t&=~((curvol_l) & MASK_VOL);
#else
        t|=curvol_l & MASK_VOL;
#endif
        i2c_p1.writeBank(VOLUMEBANK, t);
        delay(5);
      }
      if (PWR_STAT) {
        lcd.setCursor(0,1);
        if (curvol_l==0) { // && curvol_r==0) {
          if (cur_in=sel_in) {
            lcd.print("Mute        ");
            i2c_p1.writePin(MUTE_PIN,RELAY_OFF); // mute
          }
        }
        else {
          i2c_p1.writePin(MUTE_PIN,RELAY_ON); // unmute
          lcd.print("Vol ");
          lcd.print(curvol_l);
          lcd.print(" ");
//          lcd.print(curvol_r);
//          lcd.print("  ");
        }
      }
      if (cur_in==sel_in) {
        delay(vol_RPT);
      }
      else {
        delay(vol_RPT/4);      // faster when input is switched
      }
    }
    EEPROM_Save=(cur_in==sel_in);
  }
#ifdef DEBUGSERIAL2
    Serial.print("SetVolume EEPROM_Save = ");
    Serial.println(EEPROM_Save ? "true" : "false");
#endif

}

bool isInputActive (byte i) {
// return true if input is active
#ifndef ACTIVE_IN_CHECK
  return true;
#else
// works if hardware is set
  byte inpno, inptype;
  inptype=sel_in>>4;
  inpno=cur_in&0x0f;
  switch (inptype) {
      case T_IN_AN:
        return true;  // always active inputs
        break;
      case T_IN_SPDIF:
        // set second multiplexer
        // clear check bit
        // wait for check
        delay(10);
        // read input
        // if found try again to be sure
        // clear check bit
        // wait for check
        delay(10);
        // read input

        break;
      case T_IN_I2S:
        break;
      case T_IN_LAN:
        return RPI_Ready;
        break;
  }
#endif
}

void SelectInput (void) {
// global input numeration
// 0x00-0x03  - analog input
// 0x10-0x17  - spdif input
// 0x20-0x21  - I2S external transport
// 0x30       - RPI
// 0xff       - switch off all inputs
  uint8_t t, t0;
  byte inptype, oldinptype, o;
#ifdef DEBUGINPUTS
    Serial.print("SelectInput cur_in = 0x");
    Serial.print(cur_in,HEX);
    Serial.print(", sel_in = 0x");
    Serial.println(sel_in,HEX);
#endif


  if (sel_in==0xff) { // switch off all inputs
    vol_l=0;
//    vol_r=0;
    SetVolume();
// power off all analogue relays
    if (I2C_Present) {
      t=i2c_p1.readBank(ANALOGINBANK);
      #ifdef INVERTED_RELAY      
        t|=MASK_ANALOGMUX;
      #else
        t&=~MASK_ANALOGMUX;
      #endif
      i2c_p1.writeBank(ANALOGINBANK,t);
      delay(20);
    }
    cur_in=sel_in;
    return;
  }

  LCD_Light();
  t=sel_in&0x0f;
  oldinptype=cur_in>>4;
  inptype=sel_in>>4; // type of input is in high nibble
  switch (inptype) {
      case T_IN_AN:
        if (t>MAX_IN_ANALOG) {   // wrong input, initialize        
          sel_in=T_IN_AN<<4;  // first analog
          cur_in=vol_l=0;
//          vol_r=0;
        }
      break;
      case T_IN_SPDIF:
        if (t>MAX_IN_SPDIF) {   // wrong input, initialize        
          sel_in=T_IN_SPDIF<<4;  // first SPDIF
          cur_in=vol_l=0;
//          vol_r=0;
        }
        break;
      case T_IN_I2S:
        if (t>MAX_IN_I2S) {   // wrong input, initialize        
          sel_in=T_IN_I2S<<4;  // first SPDIF
          cur_in=vol_l=0;
//          vol_r=0;
        }
        break;
      case T_IN_LAN:
        if (t>1) {   // wrong input, initialize        
          sel_in=T_IN_LAN<<4;  // first SPDIF
          cur_in=vol_l=0;
//          vol_r=0;
        }
        break;
      default:
        sel_in=T_IN_AN<<4;  // wrong type, set analog
        cur_in=vol_l=0;
//        vol_r=0;
      break;
  }

  if (sel_in != cur_in ) {
    lcd.setCursor(0,0);
    switch (inptype) {
      case T_IN_AN:
        if (sel_in>0) {
          lcd.print("Analog ");
        }
        else {
          lcd.clear();          
        }
        break;
      case T_IN_SPDIF:
        lcd.print("SPDIF  ");
        break;
      case T_IN_I2S:
        lcd.print("I2S    ");
        break;
      case T_IN_LAN:
        lcd.print("LAN ");
        if (!RPI_Ready) {
          lcd.print("...");
          Serial.println("RPC_STATUS");
        }
        else {
          lcd.print("   ");
        }
        break;
    }
    lcd.print(t);
    lcd.print("  ");

    t0=i2c_p1.readBank(MUXBANK);
    t=t0;
#ifdef DEBUGINPUTS
    Serial.print("Read MUXBANK, t0 = B ");
    Serial.println(t0,BIN);
#endif

// save old input volume
    o=inputoffset(cur_in);

    eec.bufelement[o]=vol_l;
  #ifdef DEBUGEEPROM
    Serial.print("SelectInput EEPROM write vol_l, eec.bufelement[");
    Serial.print(o);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o], HEX);
  #endif    
/*
    eec.bufelement[o+1]=vol_r;
  #ifdef DEBUGEEPROM
    Serial.print("SelectInput EEPROM write vol_r, eec.bufelement[");
    Serial.print(o+1);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o+1], HEX);
  #endif    
*/  
    // mute when switching
    vol_l=0;
//    vol_r=0;
    SetVolume();

// power off all analogue relays
    if (I2C_Present) {
      t=i2c_p1.readBank(ANALOGINBANK);
      #ifdef INVERTED_RELAY      
        t|=MASK_ANALOGMUX;
      #else
        t&=~MASK_ANALOGMUX;
      #endif
      i2c_p1.writeBank(ANALOGINBANK,t);
// set AN / DIGI
      if (inptype!=T_IN_AN)
        i2c_p1.writePin(ANALOG_DIGI,RELAY_ON);
      delay(20);
      t0=t;
    }

// switch input type
    if (inptype != oldinptype ) {
// set input type
      switch (inptype) {
        case T_IN_SPDIF:
          t=t0&~I2S_IN_MASK;  // set I2SMUX to channel 0
          t|=I2S_IN_SPDIF<<I2S_IN_SHIFT;
        break;
        case T_IN_I2S:
          // do nothing, all will bi set later, when input is selected
        break;
        case T_IN_LAN:
          t=t0&~I2S_IN_MASK;
          t|=I2S_IN_RPI<<I2S_IN_SHIFT;
        break;
      }
    } // input type set

// select input number
    if (inptype==T_IN_AN) {
      switch (sel_in) {
        case 0:
          i2c_p1.writePin(ANALOG_IN_0,RELAY_ON);
        break;
        case 1:
          i2c_p1.writePin(ANALOG_IN_1,RELAY_ON);
        break;
        case 2:
          i2c_p1.writePin(ANALOG_IN_2,RELAY_ON);
        break;
        case 3:
          i2c_p1.writePin(ANALOG_IN_3,RELAY_ON);
        break;
      }
      delay(20);
    }
    else {
      switch (inptype) {
        case T_IN_SPDIF:
          t&=~SPDIFMUX_MASK;  // clear SPDIFMUX bytes
          t|=(sel_in&0x07)<<SPDIFMUX_SHIFT;
        break;
        case T_IN_I2S:
          t&=~I2S_IN_MASK;  // clear SPDIFMUX bytes
          t|=((sel_in + I2S_IN_TRANSPORT1) & I2S_IN_MASK)<<I2S_IN_SHIFT;
        break;
        case T_IN_LAN:
          t&=~I2S_IN_MASK;  // clear SPDIFMUX bytes
          t|=I2S_IN_RPI<<I2S_IN_SHIFT;
        break;
      }
    }
    if (t!=t0) {
      i2c_p1.writeBank(MUXBANK,t);
    }
    cur_in=sel_in;
    o=inputoffset(cur_in);
// get selected input volumes
    vol_l=eec.bufelement[o];
  #ifdef DEBUGEEPROM
    Serial.print("SelectInput EEPROM read vol_l, eec.bufelement[");
    Serial.print(o);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o], HEX);
  #endif    
/*
    vol_r=eec.bufelement[o+1];
  #ifdef DEBUGEEPROM
    Serial.print("SelectInput EEPROM read vol_r, eec.bufelement[");
    Serial.print(o+1);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o+1], HEX);
  #endif    
*/
    SetVolume();
//    cur_in=sel_in;
    EEPROM_Save=true;
//    SaveStatus();
    sel_in_timer=0;
    sel_in_mode=true;
  } 
}

void SelectNextInput (void) {
byte inpno, inptype, vl;
//byte vr;
unsigned int s;
bool spdif_bad=true;
// TODO check if SPDIF is active
// If not active - skip
  inptype=cur_in>>4; // type of input is in high nibble
  inpno=cur_in&0x0f;  // number of input
  switch (inptype) {
    case T_IN_AN:
      if (inpno>=MAX_IN_ANALOG-1) { 
        // select first SPDIF
        sel_in=(T_IN_SPDIF<<4) + 0;
      }
      else {
        sel_in++;
      }
      break;
    case T_IN_SPDIF:
      if (inpno>=MAX_IN_SPDIF-1) { 
        // select first I2S
        sel_in=(T_IN_I2S<<4) + 0;
      }
      else {
        // mute
        vl=vol_l;  // mute when switching
//        vr=vol_r;
        vol_l=0;
//        vol_r=0;
        SetVolume();

        while (spdif_bad && (inpno<MAX_IN_SPDIF)) {
          sel_in++;
          SelectInput();
          delay(PLL_LOCK_UP);  // PLL lock up time
//          s=analogRead(SPDIF_STAT_PIN);
//          spdif_bad=(s>SPDIF_BAD ? true : false);
          spdif_bad=i2c_p1.readPin(SPDIF_PIN);
          inpno=cur_in&0x0f;  // number of input
        }
        if (spdif_bad && inpno>=MAX_IN_SPDIF-1) { // no SPDIF input active
          sel_in=(T_IN_I2S<<4) + 0;
        }
        if (!spdif_bad) {
          // unmute
          vol_l=vl;  // unmute
//          vol_r=vr;
          SetVolume();
        }
      }
      break;
    case T_IN_I2S:
      if (inpno>=MAX_IN_I2S-1) { 
        // select first I2S
        sel_in=(T_IN_LAN<<4) + 0;
      }
      else {
        sel_in++;
      }
      break;
    case T_IN_LAN:
    // select first Analog
        sel_in=(T_IN_AN<<4) + 0;
      break;
  }  
  SelectInput();
}

void SelectPrevInput (void) {
byte inpno, inptype, vl, vr;
unsigned int s;
bool spdif_bad=true;

  inptype=cur_in>>4; // type of input is in high nibble
  inpno=cur_in&0x0f;  // number of input
  if (inpno==0) { 
    switch (inptype) {
      case T_IN_AN:
          // select LAN
          sel_in=(T_IN_LAN<<4) + 0;
        break;
      case T_IN_SPDIF:
          // select last analog
          sel_in=(T_IN_AN<<4) + MAX_IN_ANALOG - 1;
        break;
      case T_IN_I2S:
          // select last SPDIF
          sel_in=(T_IN_SPDIF<<4) + MAX_IN_SPDIF - 1;
        break;
      case T_IN_LAN:
      // select last I2S
          sel_in=(T_IN_I2S<<4) + MAX_IN_I2S - 1;
        break;
    }
  }
  else {
    if (inptype==T_IN_SPDIF) {
          // mute
      vl=vol_l;  // mute when switching
//      vr=vol_r;
      vol_l=0;
//      vol_r=0;
      SetVolume();

      while (spdif_bad && (inpno>0)) {
        sel_in--;
        SelectInput();
        delay(PLL_LOCK_UP);  // PLL lock up time
//        s=analogRead(SPDIF_STAT_PIN);
//        spdif_bad=(s>SPDIF_BAD ? true : false);
          spdif_bad=i2c_p1.readPin(SPDIF_PIN);
        inpno=cur_in&0x0f;  // number of input
      }
      if (spdif_bad && inpno==0) {  // no SPDIF input is active
          // select last analog
          sel_in=(T_IN_AN<<4) + MAX_IN_ANALOG - 1;
      }
      if (!spdif_bad) {
        // unmute
        vol_l=vl;  // unmute
//        vol_r=vr;
        SetVolume();
      }
    } 
    else {
      sel_in--;
    }

  }  
  SelectInput();
}


void Power_Off() {
  oldcmd=C_NONE;
  cmd=C_NONE;
  vol_l=0;
//  vol_r=0;
  if (PWR_STAT) {
    SetVolume();
    EEPROM_Save=false;
  }
  sel_in=0xff;
  if (PWR_STAT) {
    SelectInput();
    EEPROM_Save=false;
  }
  digitalWrite(IR_LED_PIN, HIGH);
  Serial.println("RPC_POWEROFF");
  if (PWR_STAT) {
    if (RPI_Ready) {
      lcd.setCursor(0,0);
      lcd.print("Wait for RPI...");
      delay(RPI_POWEROFF_DELAY);
      RPI_Ready=false;
    }
    LCD_TimerCount=LED_FADE;
    fader=FADEMIN;
    Timer1.pwm(LCD_BL_PIN, fader<<2);
    lcd.noDisplay();
  }

  I2c.end();
  TWCR &= !_BV(TWEN); // release I2C bus
  Serial.println("PWR_PIN LOW");
  digitalWrite(PWR_PIN, LOW);
  digitalWrite(LCD_BL_PIN, LOW);
  digitalWrite(SCL, LOW);
  digitalWrite(SDA, LOW);
  PWR_STAT=false;
}

void Power_On() {
  uint8_t returnStatus;
  Serial.println("Power on");
  oldcmd=C_NONE;
  cmd=C_NONE;
  digitalWrite(LCD_BL_PIN, HIGH);
  Serial.println("PWR_PIN HIGH");
  digitalWrite(PWR_PIN, HIGH);
  delay(150);
  digitalWrite(IR_LED_PIN, LOW);
  I2c.begin();
  I2c.timeOut(100);

  i2c_p1.initialize();
  i2c_p1.setAllDirection(B00000000,B00000000,B10000000);  // all outputs, only C27 input

  lcd.begin(16, 2);
  lcd.setContrast(30);
  I2C_Present=i2c_p1.testConnection();
#ifdef DEBUGSERIAL
  Serial.print("I2C_Present= ");
  Serial.println(I2C_Present);
#endif

  if (I2C_Present) {
    i2c_p1.initialize();
    i2c_p1.setAllDirection(B00000000,B00000000,B10000000);  // all outputs, only C27 input
  }
  PWR_STAT=true;
  RPI_Ready=false;
  Serial.println("RPC_POWERON");
  RPI_WaitAnswer=true;  
  cur_in=0xff;
  eec.FindPtr();
  #ifdef DEBUGEEPROM
    Serial.print("EEPROM ptr = 0x");
    Serial.println(eec.pptr, HEX);
  #endif
  ReadStatus();
  SelectInput();
  EEPROM_Save=false;
}

uint8_t inputoffset (byte i) {
// calculate input offset in EEPROM element
// order of inputs is:
// analog, spdif, i2s, lan

  byte inptype, n, o;

#ifdef DEBUGSERIAL2
  Serial.print("Inputoffset: cur_in = 0x");
  Serial.println(cur_in,HEX);
#endif

  inptype=i>>4; // type of input is in high nibble
  n=(i & 0x0f);  // low nibble is selected input, 2bytes for vol_l and vol_r

  switch (inptype) {
    case T_IN_AN:
      o=n;
      break;
    case T_IN_SPDIF:
      o=n+MAX_IN_ANALOG;
      break;
    case T_IN_I2S:
      o=n+MAX_IN_ANALOG+MAX_IN_SPDIF;
      break;
    case T_IN_LAN:
      o=MAX_IN_ANALOG+MAX_IN_SPDIF+MAX_IN_I2S;
      break;
  }
  o=o*2+1; // 1 byte for selected input, and next 2 bytes for each input volume
#ifdef DEBUGSERIAL2
  Serial.print("o = 0x");
  Serial.println(o,HEX);
#endif
  return o;
}

void SaveStatus(){
  byte o;
  if (EEPROM_Save) {
#ifdef DEBUGSERIAL2
    Serial.println("SaveStatus:");
#endif
    o=inputoffset(cur_in);
    eec.bufelement[o]=vol_l;
  #ifdef DEBUGEEPROM
    Serial.print("SaveStatus EEPROM write vol_l, eec.bufelement[0x");
    Serial.print(o, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o], HEX);
  #endif    
/*
    eec.bufelement[o+1]=vol_r;
  #ifdef DEBUGEEPROM
    Serial.print("SaveStatus EEPROM write vol_r, eec.bufelement[0x");
    Serial.print(o+1, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o+1], HEX);
  #endif    
*/
    eec.bufelement[EEADDR_Input]=sel_in;
  #ifdef DEBUGEEPROM
    Serial.print("SaveStatus EEPROM write sel_in, eec.bufelement[0x");
    Serial.print(EEADDR_Input, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[EEADDR_Input], HEX);
  #endif    
    eec.WriteBufElement();

    EEPROM_Save=false;
  }
}

void ReadStatus () {
#ifdef DEBUGSERIAL2
  Serial.println("ReadStatus");
#endif
  byte o;
  eec.ReadBufElement();
  sel_in=eec.bufelement[EEADDR_Input];
  if (sel_in==0) {
    sel_in=1;
  }
  o=inputoffset(sel_in);
  #ifdef DEBUGEEPROM
    Serial.print("ReadStatus EEPROM read sel_in, eec.bufelement[0x");
    Serial.print(EEADDR_Input, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[EEADDR_Input], HEX);
  #endif    

  vol_l=eec.bufelement[o];
  #ifdef DEBUGEEPROM
    Serial.print("ReadStatus EEPROM read vol_l, eec.bufelement[0x");
    Serial.print(o, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o], HEX);
  #endif    
/*
  vol_r=eec.bufelement[o+1];
  #ifdef DEBUGEEPROM
    Serial.print("ReadStatus EEPROM read vol_r, eec.bufelement[0x");
    Serial.print(o+1, HEX);  
    Serial.print("] = 0x");
    Serial.println(eec.bufelement[o+1], HEX);
  #endif    
*/
  EEPROM_Save=false;
}

unsigned int read_RPI() {
  if (!RPI_WaitAnswer) return 0;  // nothing waiting
// read serial
// update status
  return 0;
}

void setup() {
  Serial.begin(115200);
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(LCD_BL_PIN, OUTPUT);
  pinMode(PWR_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);
  pinMode(SDA, OUTPUT);
  pinMode(SCL, OUTPUT);
  PWR_STAT=false;
  inputString.reserve(40);
  eec.FindPtr();
  encoder = new ClickEncoder(A1, A2, 11);
  encoder->setAccelerationEnabled(false);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); //,10000); // 10 ms
  IR_Recv=false;
  IR_TimerCount=0;
  enc_last = -1;
  Power_Off();
  irrecv.blink13(0);
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Ready");
}

bool irr;

void loop() {
  byte t;
  byte cmd_rpt;

  lcd_key=read_IR();
  if (lcd_key!=IRNone) {
#ifdef DEBUGSERIAL
    Serial.print("IR key = ");
  	Serial.println(lcd_key, HEX);
#endif
  }
  if (fade) {
    fade=false;
    Timer1.pwm(LCD_BL_PIN, fader<<2);
//    analogWrite(LCD_BL_PIN, fader); 
//#ifdef DEBUGSERIAL
//    Serial.print("fader=");
//    Serial.println(fader);
//#endif
  }

#ifdef DEBUGCMD0
  if (irr!=IR_Recv) {
      Serial.print("IR_Recv = ");
      Serial.println(IR_Recv);
      Serial.print("IR_TimerCount = ");
      Serial.println(IR_TimerCount);
      irr=IR_Recv;
  }
#endif

  if (cmd==C_NONE) {
    cmd=IR_decode(lcd_key);
    if (IR_Pressed && !IR_Recv) {
      cmd=C_K_KEYRELEASED;
#ifdef DEBUGCMD0
      Serial.println("cmd = C_K_KEYRELEASED");
#endif
      IR_Pressed=false;
      oldcmd=C_NONE;
    }
  }
  if (cmd==C_NONE) {  // read keyboard
    lcd_key = read_KBD(); // read the buttons
/*    if (lcd_key!=btnNONE){
      Serial.println("KBD key");
      Serial.println(lcd_key, HEX);
    } */
    cmd=KBD_Decode(lcd_key);
  }
  if (cmd==C_NONE) {  // read encoder
    cmd=read_ENC();
  }

  if (IR_Recv || (cmd!=C_NONE)) {  // process command
    cmd_rpt=(IR_Recv && (cmd==C_NONE) ? oldcmd: cmd);
#ifdef DEBUGSERIAL
    if ((cmd!=C_NONE)) {
      Serial.print("PWR_STAT = ");
      Serial.println(PWR_STAT);
      Serial.print("IR_Recv = ");
      Serial.println(IR_Recv);
    }
#endif
#ifdef DEBUGCMD
      Serial.print("oldcmd = ");
      Serial.print(oldcmd);
      Serial.print(". cmd = ");
      Serial.print(cmd);
      Serial.print(". cmd_rpt = ");
      Serial.println(cmd_rpt);
#endif
  // process command
    if (PWR_STAT) {  // powered
      if (!sel_in_mode && sel_in_led) {
        sel_in_led=false;
#ifdef LED_INP
        digitalWrite(LED_INP,LOW);
#endif
#ifdef DEBUGENC
        Serial.println("Select Input END");
#endif
      }
      if (sel_in_mode && !sel_in_led) {
        sel_in_led=true;
#ifdef LED_INP
        digitalWrite(LED_INP,HIGH);
#endif
#ifdef DEBUGENC
        Serial.println("Select Input Start");
#endif
      }
      LCD_Light();
      (cmd==C_K_KEYRELEASED ? lcd.setCursor(14,1) : lcd.setCursor(11,1));
      lcd.print(cmd,HEX);
      lcd.print("   ");
      if (oldcmd==C_PWR && cmd==C_K_KEYRELEASED) {
#ifdef DEBUGSERIAL
        Serial.println("cmd to power off");
#endif
        Power_Off();
      }
      else {
// repeated commands
//        do {
          switch (cmd_rpt) {
            case C_VOL_UP:
              if ((sel_in>>4)==T_IN_LAN) {
                Serial.println("RPC_VOLUP");
              }
              else {
  //              Serial.println("Volume up");
                if (vol_l>=MAXATT) {
                  vol_l=MAXATT;
                }
                else {
                  vol_l++;
                }
/*
                if (vol_r>=MAXATT) {
                  vol_r=MAXATT;
                }
                else {
                  vol_r++;
                }
*/
              }
            break;
            case C_VOL_DN:
              if ((sel_in>>4)==T_IN_LAN) {
                Serial.println("RPC_VOLDOWN");
              }
              else {
  //              Serial.println("Volume down");
                if (vol_l>MAXATT) {
                  vol_l=MAXATT;
                }
                if (vol_l>0) {
                  vol_l--;
                }
/*                
                if (vol_r>MAXATT) {
                  vol_r=MAXATT;
                }
                if (vol_r>0) {
                  vol_r--;
                }
*/                
              }
            break;
          }

          switch (cmd_rpt) {
            case C_VOL_UP:
            case C_VOL_DN:
              SetVolume();
            break;
          }

// non repeating commands        
        switch (cmd) {
          case C_SEL_IN_NEXT:  // Next input
            SelectNextInput();
          break;
          case C_SEL_IN_PREV:  // Next input
            SelectPrevInput();
          break;
// Input 0-4 analog input
          case C_SEL_IN_1:  // ANALOG 1
            sel_in=(T_IN_AN<<4) + 1;
            SelectInput();
          break;
          case C_SEL_IN_2:  // ANALOG 2
            sel_in=(T_IN_AN<<4) + 2;
            SelectInput();
          break;
          case C_SEL_IN_3:  // ANALOG 3
            sel_in=(T_IN_AN<<4) + 3;
            SelectInput();
          break;
          case C_SEL_IN_4:  // ANALOG 4
            sel_in=(T_IN_AN<<4) + 4;
            SelectInput();
          break;
// Input 5 RPI
          case C_SEL_IN_5:  // RPI
          sel_in=(T_IN_LAN<<4) + 0;
            SelectInput();
          break;
// Input 6 I2S 1
          case C_SEL_IN_6:  // I2S 1
            sel_in=(T_IN_I2S<<4) + 0;
            SelectInput();
          break;
// Input 7 I2S 2
          case C_SEL_IN_7:  // I2S 2
            sel_in=(T_IN_I2S<<4) + 1;
            SelectInput();
          break;
// Input 8 I2S 3
          case C_SEL_IN_8:  // I2S 3
            sel_in=(T_IN_I2S<<4) + 2;
            SelectInput();
          break;
// Input 8-15 SPDIF 1-8
          case C_SEL_IN_9:  // SPDIF 1
            sel_in=(T_IN_SPDIF<<4) + 0;
#ifdef DEBUGSERIAL
          Serial.println("SPDIF 1");
#endif
            SelectInput();
          break;
          case C_SEL_IN_10:  // SPDIF 2
            sel_in=(T_IN_SPDIF<<4) + 1;
#ifdef DEBUGSERIAL
          Serial.println("SPDIF 2");
#endif
            SelectInput();
          break;
          case C_SEL_IN_11:  // SPDIF 3
            sel_in=(T_IN_SPDIF<<4) + 2;
#ifdef DEBUGSERIAL
          Serial.println("SPDIF 3");
#endif
            SelectInput();
          break;
          case C_SEL_IN_12:  // SPDIF 4
#ifdef DEBUGSERIAL
          Serial.println("SPDIF 4");
#endif
            sel_in=(T_IN_SPDIF<<4) + 3;
            SelectInput();
          break;
          case C_SEL_IN_13:  // SPDIF 5
            sel_in=(T_IN_SPDIF<<4) + 4;
            SelectInput();
          break;
          case C_SEL_IN_14:  // SPDIF 6
            sel_in=(T_IN_SPDIF<<4) + 5;
            SelectInput();
          break;
          case C_SEL_IN_15:  // SPDIF 7
            sel_in=(T_IN_SPDIF<<4) + 6;
            SelectInput();
          break;
          case C_SEL_IN_16:  // SPDIF 8
            sel_in=(T_IN_SPDIF<<4) + 7;
            SelectInput();
          break;
          case C_VOL_MUTE:
            if (vol_l==0 && vol_l==0) {
              ReadStatus();
            }
            else {
              vol_l=0;
//              vol_r=0;
            }
            SetVolume();             
            EEPROM_Save=false;
          break;
        }
        if ((sel_in>>4)==T_IN_LAN) {
          switch (cmd) {
  // RPI Commands
            case C_P_PLAY:
              Serial.println("RPC_PLAY");
              RPI_WaitAnswer=true;
            break;
            case C_P_STOP:
              Serial.println("RPC_STOP");
              RPI_WaitAnswer=true;
            break;
            case C_P_PAUSE:
              Serial.println("RPC_PAUSE");
              RPI_WaitAnswer=true;
            break;
            case C_P_NEXT_TRACK:
              Serial.println("RPC_NEXT");
              RPI_WaitAnswer=true;
            break;
            case C_P_PREV_TRACK:
              Serial.println("RPC_PREV");
              RPI_WaitAnswer=true;
            break;
          }
        }
        SaveStatus();
      }
    }
    else {  // no power
//      Serial.println("No Power");
      if (oldcmd==C_PWR && cmd==C_K_KEYRELEASED) {
#ifdef DEBUGCMD
        Serial.println("Power_On cmd");
#endif
        Power_On();
      }
    }
    if (cmd!=C_NONE && cmd!=C_K_KEYRELEASED) {
      oldcmd=cmd;
#ifdef DEBUGCMD
          Serial.print("oldcmd=cmd=");
          Serial.println(oldcmd);
#endif
    }
#ifdef DEBUGCMD0
    Serial.println("No CMD");
#endif
    cmd=C_NONE;
  }
  else   // clear all commands on IR end
    if (IR_Recv) {
      oldcmd=cmd=C_NONE;
#ifdef DEBUGCMD
    Serial.println("oldcmd=cmd=C_NONE");
#endif
//  if (PWR_STAT && !RPI_Ready) {
//    Serial.println("RPC_READYSTATUS");
//  }   
    }
// read serial port  
//  else {
//    Serial.println("NO stringComplete");
//  }
  if (stringComplete) {
#ifdef DEBUGSERIAL
    Serial.println("stringComplete");
#endif
//    inputString.trim();
/*
    Serial.print("inputString.lastIndexOf('!')=");
    Serial.println(inputString.lastIndexOf('!'));    
    Serial.print("inputString.lastIndexOf('\\n')=");
    Serial.println(inputString.lastIndexOf('\n'));    
//    lcd.setCursor(0,0);
//    while (inputString.indexOf('\n')>0) {
    if (inputString.lastIndexOf('!')>0) {
        inpStr=inputString.substring(inputString.lastIndexOf('!')+1);
    }
    else {
      if  (inputString.lastIndexOf('\n')>0) 
        inpStr=inputString.substring(inputString.lastIndexOf('\n')+1);
      else
        inpStr=inputString;
    }
*/
//    inpStr=inputString;
//    Serial.println(inputString);
//    lcd.print(inpStr);
//    lcd.print("       ");

/*
#ifdef DEBUGSERIAL
    Serial.print("inputString=");
    Serial.println(inputString);
//    Serial.print("inpStr=");
//    Serial.println(inpStr);
    Serial.print("inputString[0]=");
    Serial.println(inputString.charAt(0));
    Serial.print("inputString[1]=");
    Serial.println(inputString.charAt(1));
    Serial.print("inputString[2]=");
    Serial.println(inputString.charAt(2));
    Serial.print("inputString[3]=");
    Serial.println(inputString.charAt(3));
#endif
*/    
    switch (inputString.charAt(0)){
      case 'O':
        if (inputString.charAt(1)=='K') {
          if (!RPI_Ready) {
            // delete ...
//            lcd.setCursor(0,1);
//            lcd.print("RPI Ready   ");
            lcd.setCursor(9,0);
            lcd.print("   ");
          }
#ifdef DEBUGSERIAL
          Serial.println("RPI Ready!");
#endif
          RPI_Ready=true;
        }
        break;
      case 'v':
        lcd.setCursor(0,1);
        inputString=inputString.substring(1);
//        lcd.print(inpStr);

        lcd.print("Vol ");
//        lcd.print(inpStr.substring(2,inpStr.indexOf('%')+1));
        lcd.print(inputString);
        lcd.print("%  ");
//        curvol_l=inpStr.toInt();
//        curvol_r=curvol_l;

//        lcd.print("   ");
        break;
      case 'S': // serial commands, begin with SC_
/*
#ifdef DEBUGSERIALCMD
          Serial.println("Serial command ");
          Serial.print(inputString.charAt(1));
          Serial.print(inputString.charAt(2));
          Serial.print(inputString.charAt(3));

#endif
*/
        if (inputString.charAt(1)=='C' && inputString.charAt(2)=='_') {
#ifdef DEBUGSERIALCMD
          Serial.print("decoded: ");
#endif          
          switch (inputString.charAt(3)){
            case '0':   // SC_0 =  C_K_KEYRELEASED
              cmd=C_K_KEYRELEASED;
#ifdef DEBUGSERIALCMD
              Serial.println("C_K_KEYRELEASED");
#endif
            break;
            case '1':   // SC_1 =  Power
              cmd=C_PWR;
#ifdef DEBUGSERIALCMD
              Serial.println("C_PWR");
#endif
            break;
            case '2':   // SC_2 =  Volume up
              cmd=C_VOL_UP;
#ifdef DEBUGSERIALCMD
              Serial.println("C_VOL_UP");
#endif
            break;
            case '3':   // SC_3 =  Volume down
              cmd=C_VOL_DN;
#ifdef DEBUGSERIALCMD
              Serial.println("C_VOL_DN");
#endif
            break;
            case '4':   // SC_4 =  Next input
              cmd=C_SEL_IN_NEXT;
#ifdef DEBUGSERIALCMD
              Serial.println("C_SEL_IN_NEXT");
#endif
            break;
            case '5':   // SC_5 =  Previous input
              cmd=C_SEL_IN_PREV;
#ifdef DEBUGSERIALCMD
              Serial.println("C_SEL_IN_PREV");
#endif
            break;
            case 'I':   // SC_I =  Select input, next symbol is from 0 to F
              switch (inputString.charAt(4)){
                case '0':
                  cmd=C_SEL_IN_1;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_1");
#endif
                  break;
                case '1':
                  cmd=C_SEL_IN_2;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_2");
#endif
                  break;
                case '2':
                  cmd=C_SEL_IN_3;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_3");
#endif
                  break;
                case '3':
                  cmd=C_SEL_IN_4;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_4");
#endif
                  break;
                case '4':
                  cmd=C_SEL_IN_5;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_5");
#endif
                  break;
                case '5':
                  cmd=C_SEL_IN_6;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_6");
#endif
                  break;
                case '6':
                  cmd=C_SEL_IN_7;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_7");
#endif
                  break;
                case '7':
                  cmd=C_SEL_IN_8;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_8");
#endif
                  break;
                case '8':
                  cmd=C_SEL_IN_9;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_9");
#endif
                  break;
                case '9':
                  cmd=C_SEL_IN_10;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_10");
#endif
                  break;
                case 'A':
                  cmd=C_SEL_IN_11;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_11");
#endif
                  break;
                case 'B':
                  cmd=C_SEL_IN_12;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_12");
#endif
                  break;
                case 'C':
                  cmd=C_SEL_IN_13;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_13");
#endif
                  break;
                case 'D':
                  cmd=C_SEL_IN_14;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_14");
#endif
                  break;
                case 'E':
                  cmd=C_SEL_IN_15;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_15");
#endif
                  break;
                case 'F':
                  cmd=C_SEL_IN_1;
#ifdef DEBUGSERIALCMD
                  Serial.println("C_SEL_IN_16");
#endif
                  break;

              }
            break;
          }
        }
        break;
      case 'F': // fader
        switch (inputString.charAt(1)){
            case 'U':
              fader++;
              Serial.print("fader=");
              Serial.println(fader);
    Timer1.pwm(LCD_BL_PIN, fader);

//              analogWrite(LCD_BL_PIN,fader);
              fade=false;
            break;
            case 'D':
              fader--;
              Serial.print("fader=");
              Serial.println(fader);
    Timer1.pwm(LCD_BL_PIN, fader);

//              analogWrite(LCD_BL_PIN,fader);
              fade=false;
            break;
        }
      break;
    }
    inputString = "";
    stringComplete = false;
// check RPI status
  }
#ifdef RPI_PRESENT
  if (PWR_STAT && !RPI_Ready) {
    if (millis()-time>2000) {
      time=millis();
      Serial.println("RPC_STATUS");
    }
  }
#endif
}

