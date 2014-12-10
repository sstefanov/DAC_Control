#ifndef I_O_h
#define I_O_h
/*
Inputs:

1	KBD			AD0		Analogue
1	IR_IN			2		INT connected
3	ENC_A,B,Button			Encoder Internal Pull-up

Total:
1 analogue
4 digital

Outputs:

Arduino
1	Power					1 when power on
1	LCD_Backlight		10	must be PWM!
2	I2C

Total:
1	PWM
3	Digital


TCA6424A:
	P0:
0-5	VOLUME0-5
6	MUTE
	
	P1:
0-1	I2SMUX0-1
2-4 SPDIFMUX0-2
5-7	EXTPOEWR1-3

	P2:
0-3	ANALOG_IN_0-3
4	AN/DIGI
7	SPDIF_STATUS - Input!

Total:
32

*/

// I/O and other lines
#define VOLUMEBANK	0
#define MUXBANK 	1
#define ANALOGINBANK	2
// PCA9539
//#define PCA9539_ADDR 0x74
// Volume control relays
// P0.0-P0.5 - VOL0-VOL5 - binary
#define MASK_VOL 0x3f
// P1.0-P1.4 - ANALOG MUX RELAY0-4 - single relay
#define MASK_ANALOGMUX B00011111
#define MASK_ANALOGIN B00001111
// relays
#define MUTE_PIN 6

#define ANALOG_IN_0	16
#define ANALOG_IN_1	17
#define ANALOG_IN_2	18
#define ANALOG_IN_3	19
#define ANALOG_DIGI	20

// P1.5-P1.7 - SPDIF MUX S0-S2 - binary
#define SPDIFMUX_MASK B00011100
#define SPDIFMUX_SHIFT 2

//I2S inputs
#define I2S_IN_MASK B00000011
#define I2S_IN_SHIFT 0
#define I2S_IN_SPDIF 0
#define I2S_IN_RPI 1 
#define I2S_IN_TRANSPORT1 2
#define I2S_IN_TRANSPORT2 3

// number of inputs
#define MAX_IN_ANALOG 4
#define MAX_IN_SPDIF 8
#define MAX_IN_I2S 2

#define SPDIF_PIN	23

// DAC Analog Input
#define DAC_IN ANALOG_IN_4

// global input numeration
// 0x00-0x04 	- analog input
// 0x10-0x17 	- spdif input
// 0x20-0x21 	- I2S external transport
// 0x30			- RPI
#define T_IN_AN 0x0
#define T_IN_SPDIF 0x1
#define T_IN_I2S 0x2
#define T_IN_LAN 0x3

// if hardware is set
//#define ACTIVE_IN_CHECK
// arduino
#define IR_LED_PIN 13
#define PWR_PIN 12
#define LCD_BL_PIN 10
#define IR_RECV_PIN 2
#define RST_PIN A1
//#define SPDIF_STAT_PIN 3
//#define SPDIF_RD_PIN A3
#define SDA_PIN A4
#define SCL_PIN A5

//#define SPDIF_BAD 200	// threshold for bad SPDIF data
#define PLL_LOCK_UP 120	// DIR9001 PLL Lock up time, 100ms from datasheet

#define RPI_POWEROFF_DELAY 35000
#define INVERTED_RELAY	// relay is on when output is 0
#ifdef INVERTED_RELAY
#define RELAY_ON LOW
#define RELAY_OFF HIGH
#else
#define RELAY_ON HIGH
#define RELAY_OFF LOW
#endif
// rotary encoder

#define ENC_STEPS	3	// steps per pulse
#define ENC_PIN1	A1
#define ENC_PIN2	A2
#define ENC_BTN		11

#define SELINDELAY	3000
//#define LED_INP		13

#endif