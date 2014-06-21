#ifndef I_O_h
#define I_O_h

// I/O and other lines

// PCA9539
#define PCA9539_ADDR 0x74
// Volume control relays
// P0.0-P0.5 - VOL0-VOL5 - binary
#define MASK_VOL 0x3f
// P0.6-P0.7 - I2S MUX S0-S1 - binary
#define MASK_I2SMUX 0xc0
#define SHIFT_I2SMUX 6
// P1.0-P1.4 - ANALOG MUX RELAY0-4 - single relay
#define MASK_ANALOGMUX 0x1f
#define MASK_ANALOGIN 0x0f
// relays
#define ANALOG_IN_0	0
#define ANALOG_IN_1	1
#define ANALOG_IN_2	2
#define ANALOG_IN_3	3
#define ANALOG_IN_DIGI	4
// DAC Analog Input
#define DAC_IN ANALOG_IN_4
// P1.5-P1.7 - SPDIF MUX S0-S2 - binary
#define MASK_SPDIFMUX 0xe0
#define SHIFT_SPDIFMUX 5

//I2S inputs
#define I2S_IN_MASK 0x03
#define I2S_IN_SPDIF 0
#define I2S_IN_RPI 1 
#define I2S_IN_TRANSPORT1 2
#define I2S_IN_TRANSPORT2 3


// global input numeration
// 0x00-0x04 	- analog input
// 0x10-0x17 	- spdif input
// 0x20-0x21 	- I2S external transport
// 0x30			- RPI
#define T_IN_AN 0x0
#define T_IN_SPDIF 0x1
#define T_IN_I2S 0x2
#define T_IN_LAN 0x3

#endif