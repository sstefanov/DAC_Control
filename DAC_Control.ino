// DAC_Control.ino
#include <TimerOne.h>
#include <LiquidCrystal.h>
#include <IRremote_INT.h>
#include <EEPROM.h>
#include <DTIOI2CtoParallelConverter.h>
#include <Wire.h>

#include "I_O.h"
#include "EEPROM_circular.h"

//#include "IR_Packard_Bell.h"
#include "IR_Uni_CODE_159.h"
#include "commands.h"

// I2C port 1
#ifdef PCA9539_ADDR
DTIOI2CtoParallelConverter i2c_p1(PCA9539_ADDR);
#endif

#define DEBUGSERIAL

// delay 62.5ns on a 16MHz AtMega
#define NOP __asm__ __volatile__ ("nop\n\t")
#define pulse_pin(x)    delayMicroseconds(1); digitalWrite(x, HIGH); delayMicroseconds(1); digitalWrite(x, LOW);

// volume control




// ******* Pins *******
int IR_LED=13;
int LCD_BL=10;  // lcd backlight and power on
int RECV_PIN = 2;

	// serial register
//int SER_D=15; // HC595 - DS
//int SER_C=16; // HC595 - SH_CP
//int ATT_L=17; // HC595 - ST_CP

// I2C are defined in Arduino.h

int PWR_ON=0;
//int SEL_L=0;


// constants

#define DELAY1  200
#define DELAY2  2

#define vol_RPT	50
#define MAXATT  63
#define MAXINPUT	6

// EEPROM addresses
#define EEADDR_Input  0
#define EEADDR_Volume_L	1
#define EEADDR_Volume_R 2

// variables

// ******* IR *******
IRrecv irrecv(RECV_PIN);

#define ELEMENTSIZE 3
#define PTRSIZE 16
#define BBUFSIZE 80
EEPROM_C eec(ELEMENTSIZE,PTRSIZE,BBUFSIZE);
static unsigned int oldIRKey;
static unsigned int IRKey=IRNone;
static boolean IR_Recv;
static boolean IR_Pressed;
decode_results results;
int IR_TimerCount=0;  // counter
unsigned long time=0;
_COMMAND cmd=C_NONE;
_COMMAND oldcmd=C_NONE;
bool PWR_STAT=false;
bool EEPROM_Save=false; // flag to save to EEPROM

// Raspberry PI
bool RPI_WaitAnswer;
bool RPI_Ready;



// serial input
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
String inpStr;

byte vol_l; // L volume
byte vol_r; // R volume
byte sel_in;  // selected input
byte curvol_l; // current L volume
byte curvol_r; // current R volume
byte cur_in; // current selected input


// bytes for EEPPROM
#define PARAMS 0
#define P_VOL_L 1
#define P_VOL_R 2

// ******* LCD *******
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// define some values used by the panel and buttons
unsigned int lcd_key = 0;
int adc_key_in = 0;
#define btnRIGHT 1
#define btnUP 2
#define btnDOWN 3
#define btnLEFT 4
#define btnSELECT 5
#define btnNONE 0
#define btnReleased 128

/*
static int S_goOn = 1;
static int S_count = -1;
static int S_pos1;
static int S_pos2 = request.length();
*/

// read serial
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

// read the buttons
int read_LCD_buttons() {
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

void IRLED_Blink() {
  IR_TimerCount=0;
  IR_Recv=true;
  IR_Pressed=true;
  if (PWR_STAT) {
    digitalWrite(IR_LED, HIGH); 
  }
  else {
    digitalWrite(IR_LED, LOW); 
  }
}

void IRLED_Off() {
  if (IR_TimerCount==5) {
    if (PWR_STAT) {
      digitalWrite(IR_LED, LOW); 
    }
    else {
      digitalWrite(IR_LED, HIGH); 
    }
  }
  if (IR_Recv) {
    IR_TimerCount++;
  }
  if (IR_TimerCount>12) {
//    digitalWrite(IR_LED, LOW); 
//    Timer1.stop();
    IR_Recv=false;
//    Serial.print("IR Stop - "); 
//    Serial.println(IR_TimerCount); 
    IR_TimerCount=0;  
  }
}

unsigned int read_IR() {
  unsigned long v;  // place for the value
  int t;
  if (results.valid) {
    t=results.decode_type;
    if (t==-1) {
      return IRNone;
    }
    v=results.value;
    results.valid=0;
    IR_TimerCount=0;
    if (IR_check_RPT(v)) {  // repeat command
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

_COMMAND LCD_decode(int value){
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

/*
// shift 8 bytes
// MSB first
void shout(byte a) {
	byte i;
	digitalWrite(SER_C, LOW);
	for (i=0; i<8; i++) {
		((a & 0x80)==0x80 ? digitalWrite(SER_D, HIGH) : digitalWrite(SER_D, LOW));
		delayMicroseconds(1);
		digitalWrite(SER_C, HIGH);
		a<<=1;
    delayMicroseconds(1);
		digitalWrite(SER_C, LOW);
	}
}
*/

void SetVolume (void) {
	byte tl;
	byte tr;
  byte t;
/*	lcd.setCursor(0,1);
  lcd.print("Vol ");
	lcd.print(curvol_l);
	lcd.print("  ");
  lcd.print(curvol_r);
  lcd.print("  ");
  */

  if ((sel_in>>4)==T_IN_LAN) { // selected RPI
    if (vol_l > 100) {
      vol_l=100;
      vol_r=vol_l;
    }
//    if ((curvol_l != vol_l) || (curvol_r != vol_r)) {
      if (curvol_l < vol_l) {
//        curvol_l++;
//        Serial.println("RPC_VOLUP");
//        EEPROM_Save=true;
      }
      else if (curvol_l > vol_l) {
//        curvol_l--;
//        Serial.println("RPC_VOLDOWN");
//        EEPROM_Save=true;
      }
      if (curvol_r < vol_r) {
//        curvol_r++;
//        Serial.println("RPC_VOLUP_R");
//        EEPROM_Save=true;
      }
      else if (curvol_r > vol_r) {
//        curvol_r--;
//        Serial.println("RPC_VOLDOWN_R");
//        EEPROM_Save=true;
      }
//    }
//    lcd.print(curvol_l);
// set volume registers
// TODO Add R channel   
//    shout(curvol_l);
//    pulse_pin(ATT_L);
//    }
      if (cur_in=sel_in) {
        delay(vol_RPT);
      }
      else {
        delay(vol_RPT/2);      // faster when input is switched
      }
  }
  else {
  	if (vol_l > MAXATT) {
  		vol_l=MAXATT;
  		vol_r=vol_l;
  	}

  	while ((curvol_l != vol_l) || (curvol_r != vol_r)) {
  		tl=curvol_l;
  		tr=curvol_r;
  		if (curvol_l < vol_l) {
  			curvol_l++;
        EEPROM_Save=true;
  		}
  		else if (curvol_l > vol_l) {
  			curvol_l--;
        EEPROM_Save=true;
  		}
  		if (curvol_r < vol_r) {
  			curvol_r++;
        EEPROM_Save=true;
  		}
  		else if (curvol_r > vol_r) {
  			curvol_r--;
        EEPROM_Save=true;
  		}
    

//make-before-break
// set all ones
//      shout(curvol_l|tl); 
      i2c_p1.digitalReadPort0(t); // read port status in temp byte
/*
      Serial.print("curvol_l=");
      Serial.println(curvol_l,HEX);
      Serial.print("tl=");
      Serial.println(tl,HEX);
      Serial.print("Read t=");
      Serial.println(t,HEX);
*/
      t&=~MASK_VOL; // clear relay bytes
/*
      Serial.print("mask t=");
      Serial.println(t,HEX);
*/
      t|=(curvol_l & tl) & MASK_VOL;
/*
      Serial.print("write t=");
      Serial.println(t,HEX);
*/
      i2c_p1.digitalWritePort0(t);  // power off all relays

//       pulse_pin(ATT_L);
      delay(10);
// set all
//      shout(curvol_l);  
      t&=~MASK_VOL;
      t|=curvol_l & MASK_VOL;
//      pulse_pin(ATT_L);
//      Serial.print("write t=");
//      Serial.println(t,HEX);
      i2c_p1.digitalWritePort0(t);  // power off all relays
      delay(10);
// end  set volume registers
//    if ((curvol_l != vol_l) || (curvol_r != vol_r)) {
//      Serial.print("Volume delay ");
//      Serial.println(vol_RPT);
      lcd.setCursor(0,1);
      if (curvol_l==0 && curvol_r==0) {
        lcd.print("Mute        ");
      }
      else {
        lcd.print("Vol ");
        lcd.print(curvol_l);
        lcd.print(" ");
        lcd.print(curvol_r);
        lcd.print("  ");
      }
      if (cur_in=sel_in) {
        delay(vol_RPT);
      }
      else {
        delay(vol_RPT/2);      // faster when input is switched
      }
    }
    EEPROM_Save=true;
  }
}

void SelectInput (void) {
// global input numeration
// 0x00-0x04  - analog input
// 0x10-0x17  - spdif input
// 0x20-0x21  - I2S external transport
// 0x30     - RPI

  byte vl,vr,t, inptype;
  if (sel_in != cur_in ) {
    inptype=sel_in>>4; // type of input is in high nibble
    lcd.setCursor(0,0);
    lcd.print("In: ");
    switch (inptype) {
      case T_IN_AN:
        lcd.print("Analog ");
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
    lcd.print(sel_in + 1 & 0x0f);
    lcd.print("  ");

    vl=vol_l;  // mute when switching
    vr=vol_r;
    vol_l=0;
    vol_r=0;
    SetVolume();

// switch input
    // analog inputs
    i2c_p1.digitalReadPort1(t); // read port status in temp byte
    t&=~MASK_ANALOGMUX; // clear relay bytes
    i2c_p1.digitalWritePort1(t);  // power off all relays
    delay(20);
    t=(inptype==T_IN_AN ? sel_in & MASK_ANALOGIN : ANALOG_IN_DIGI);
    i2c_p1.digitalWrite1(t,HIGH);
    delay(20);

    switch (inptype) {
      case T_IN_SPDIF:
      // set SPDIF MUX
        i2c_p1.digitalReadPort1(t); // read port status in temp byte
        t&=~MASK_SPDIFMUX;  // clear SPDIFMUX bytes
        t|=(sel_in&0x07)<<SHIFT_SPDIFMUX;
        i2c_p1.digitalWritePort1(t);
      // set I2S MUX to SPDIF
        i2c_p1.digitalReadPort0(t); // read port status in temp byte
        t&=~MASK_I2SMUX;  // clear SPDIFMUX bytes
        t|=I2S_IN_SPDIF<<SHIFT_I2SMUX;
        i2c_p1.digitalWritePort0(t);
      break;
      case T_IN_I2S:
      // set I2S MUX to SPDIF
        i2c_p1.digitalReadPort0(t); // read port status in temp byte
        t&=~MASK_I2SMUX;  // clear SPDIFMUX bytes
        t|=((sel_in + I2S_IN_TRANSPORT1) & I2S_IN_MASK)<<SHIFT_I2SMUX;
        i2c_p1.digitalWritePort0(t);
      break;
      case T_IN_LAN:
      // set I2S MUX to SPDIF
        i2c_p1.digitalReadPort0(t); // read port status in temp byte
        t&=~MASK_I2SMUX;  // clear SPDIFMUX bytes
        t|=I2S_IN_RPI<<SHIFT_I2SMUX;
        i2c_p1.digitalWritePort0(t);
      break;
    }
    cur_in=sel_in;

    vol_l=vl;  // unmute
    vol_r=vr;
    SetVolume();
    cur_in=sel_in;
    EEPROM_Save=true;
    SaveStatus();
  } 
}

void Power_Off() {
  Serial.println("Power off");
  oldcmd=C_NONE;
  cmd=C_NONE;
  vol_l=0;
  vol_r=0;
  SetVolume();
//  sel_in=0;
//  SelectInput();
  digitalWrite(LCD_BL, LOW);
  digitalWrite(IR_LED, HIGH);
  lcd.noDisplay();
  i2c_p1.digitalWritePort0(0);  // max attenuation
  i2c_p1.digitalWritePort1(0);  // analog in 0
  PWR_STAT=false;
  RPI_Ready=false;
  Serial.println("RPC_POWEROFF");
}

void Power_On() {
  Serial.println("Power on");
  oldcmd=C_NONE;
  cmd=C_NONE;
  digitalWrite(LCD_BL, HIGH);
  digitalWrite(IR_LED, LOW);
  lcd.display();
//  lcd.setCursor(0,0);
//  lcd.print("Ready"); // print a simplemessage
  PWR_STAT=true;
  RPI_Ready=false;
  Serial.println("RPC_POWERON");
  RPI_WaitAnswer=true;  cur_in=0;
  eec.FindPtr();
  ReadStatus();
  SelectInput();
//  SetVolume();

/*
  lcd.setCursor(0,1);
  lcd.print(vol_l);
  lcd.setCursor(3,1);
  lcd.print(vol_r);
  lcd.setCursor(7,1);
  lcd.print(sel_in);
*/
}

void SaveStatus(){
  if (EEPROM_Save) {
#ifdef DEBUGSERIAL
    Serial.println("EEPROM Save");
#endif
    eec.bufelement[EEADDR_Volume_L]=vol_l;
    eec.bufelement[EEADDR_Volume_R]=vol_r;
    eec.bufelement[EEADDR_Input]=sel_in;
    eec.WriteBufElement();
    EEPROM_Save=false;
  }
}

void ReadStatus () {
#ifdef DEBUGSERIAL
  Serial.println("EEPROM Read");
#endif
  eec.ReadBufElement();
  vol_l=eec.bufelement[EEADDR_Volume_L];
  vol_r=eec.bufelement[EEADDR_Volume_R];
  sel_in=eec.bufelement[EEADDR_Input];
  if (sel_in==0) {
    sel_in=1;
  }
}

unsigned int read_RPI() {
  if (!RPI_WaitAnswer) return 0;  // nothing waiting
// read serial
// update status
  return 0;
}

void setup() {
  pinMode(IR_LED, OUTPUT);
  pinMode(LCD_BL, OUTPUT);
/*  pinMode(SER_C, OUTPUT);
  pinMode(SER_D, OUTPUT);
  pinMode(ATT_L, OUTPUT);
  digitalWrite(SER_C, LOW);
  digitalWrite(SER_D, LOW);
  digitalWrite(ATT_L, LOW);
*/
  Serial.begin(115200);
  Serial.println("Ready");
  Wire.begin();
  i2c_p1.portMode0(ALLOUTPUT);
  i2c_p1.portMode1(ALLOUTPUT);
  i2c_p1.digitalWritePort0(MAXATT);  // max attenuation
  i2c_p1.digitalWritePort1(0);  // analog in 0
  inputString.reserve(40);
  lcd.begin(16, 2);
  eec.FindPtr();
  Power_Off();
  Timer1.initialize(10000);
  Timer1.attachInterrupt(IRLED_Off,10000); // 10 ms
  IR_Recv=false;
  IR_TimerCount=0;
  irrecv.setPin(RECV_PIN);
  irrecv.blink13(0);
  irrecv.enableIRIn(); // Start the receiver
}

/*
// check for loop speed to provide proper IR work
void loop2() {
//  Serial.println("Ready");
  lcd_key=read_IR();
  cmd=IR_decode(lcd_key);
  if (cmd!=C_NONE) {  // process command
//    Serial.println("cmd");
    Serial.println(cmd);
  }

}
*/

void loop() {
  lcd_key=read_IR();
/*  if (lcd_key!=IRNone) {
    Serial.println("IR key");
  	Serial.println(lcd_key, HEX);
  }*/
  cmd=IR_decode(lcd_key);
  if (IR_Pressed && !IR_Recv) {
    cmd=C_K_KEYRELEASED;
    IR_Pressed=false;
  }
  if (cmd==C_NONE) {  // read keyboard
    lcd_key = read_KBD(); // read the buttons
/*    if (lcd_key!=btnNONE){
      Serial.println("KBD key");
      Serial.println(lcd_key, HEX);
    } */
    cmd=LCD_decode(lcd_key);
  }
  if (cmd!=C_NONE) {  // process command
#ifdef DEBUGSERIAL
    Serial.print("oldcmd = ");
    Serial.println(oldcmd);
    Serial.print("cmd = ");
    Serial.println(cmd);
#endif
  // process command
    if (PWR_STAT) {  // powered
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
#ifdef DEBUGSERIAL
        Serial.print("cmd to process=");
        Serial.println(cmd);
        Serial.print("oldcmd=");
        Serial.println(oldcmd);
#endif
// repeated commands
        do {
          switch (cmd) {
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
                if (vol_r>=MAXATT) {
                  vol_r=MAXATT;
                }
                else {
                  vol_r++;
                }
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
                if (vol_r>MAXATT) {
                  vol_r=MAXATT;
                }
                if (vol_l>0) {
                  vol_l--;
                }
                if (vol_r>0) {
                  vol_r--;
                }
              }
            break;
          }

          switch (cmd) {
            case C_VOL_UP:
            case C_VOL_DN:
              SetVolume();
            break;
          }

        } while (IR_Recv);

        switch (cmd) {
          case C_VOL_UP:
          case C_VOL_DN:
            oldcmd=cmd;
            cmd=C_NONE;
          break;
        }

// non repeating commands        
        switch (cmd) {
// Input 0-4 analog input
          case C_SEL_IN_1:  // ANALOG 1
            sel_in=(T_IN_AN<<4) + 0;
            SelectInput();
          break;
          case C_SEL_IN_2:  // ANALOG 2
            sel_in=(T_IN_AN<<4) + 1;
            SelectInput();
          break;
          case C_SEL_IN_3:  // ANALOG 3
            sel_in=(T_IN_AN<<4) + 2;
            SelectInput();
          break;
          case C_SEL_IN_4:  // ANALOG 4
            sel_in=(T_IN_AN<<4) + 3;
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
            SelectInput();
          break;
          case C_SEL_IN_10:  // SPDIF 2
            sel_in=(T_IN_SPDIF<<4) + 1;
            SelectInput();
          break;
          case C_SEL_IN_11:  // SPDIF 3
            sel_in=(T_IN_SPDIF<<4) + 2;
            SelectInput();
          break;
          case C_SEL_IN_12:  // SPDIF 4
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
//              Serial.println("UnMute");
              // read volume from EEPROM buffer
              ReadStatus();
            }
            else {
//              Serial.println("Mute");
              vol_l=0;
              vol_r=0;
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
      Serial.println("No Power");
      if (oldcmd==C_PWR && cmd==C_K_KEYRELEASED) {
//        Serial.println("Power_On cmd");
        Power_On();
      }
    }
    if (cmd!=C_NONE && cmd!=C_K_KEYRELEASED) {
      oldcmd=cmd;
    }
    Serial.println("No CMD");
    cmd=C_NONE;
  }   
//  if (PWR_STAT && !RPI_Ready) {
//    Serial.println("RPC_READYSTATUS");
//  }   

// read serial port  
  if (stringComplete) {
    inputString.trim();
//    lcd.setCursor(0,0);
//    while (inputString.indexOf('\n')>0) {
      inpStr=inputString.substring(inputString.lastIndexOf('\n')+1);
//    }
//    Serial.println(inputString);
//    lcd.print(inpStr);
//    lcd.print("       ");
//    Serial.println(inpStr);
    switch (inpStr.charAt(0)){
      case 'O':
        if (inpStr.charAt(1)=='K') {
          if (!RPI_Ready) {
            // delete ...
//            lcd.setCursor(0,1);
//            lcd.print("RPI Ready   ");
            lcd.setCursor(9,0);
            lcd.print("   ");
          }
          RPI_Ready=true;
        }
        break;
      case 'v':
        lcd.setCursor(0,1);
        inpStr=inpStr.substring(1);
//        lcd.print(inpStr);

        lcd.print("Vol ");
//        lcd.print(inpStr.substring(2,inpStr.indexOf('%')+1));
        lcd.print(inpStr);
        lcd.print("%  ");
//        curvol_l=inpStr.toInt();
//        curvol_r=curvol_l;

//        lcd.print("   ");
        break;
    }
    inputString = "";
    stringComplete = false;
// check RPI status
  }
  if (PWR_STAT && !RPI_Ready) {
    if (millis()-time>5000) {
//      Serial.println(millis()-time);
      time=millis();
      Serial.println("RPC_STATUS");
    }
  }
/*
// try to ping RPI
  if (!PWR_STAT) {
    if (millis()-time>5000) {
//      Serial.println(millis()-time);
      time=millis();
      Serial.println("RPC_STATUS");
  }
  */
}

