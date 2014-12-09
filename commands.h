#ifndef commands_h_
#define commands_h_

// IR constants 

#define IRKeyUp 0x10000000  // RPT flag - key is down until RPT is finished

// IR variables

// ******* Commands *******
enum _COMMAND { 
// empty command
    C_NONE=0,
// power
    C_PWR_ON,
    C_PWR_OFF,
    C_PWR,
// select input
    C_SEL_IN_NEXT,
    C_SEL_IN_PREV,
// analog inputs
    C_SEL_IN_1,
    C_SEL_IN_2,
    C_SEL_IN_3,
    C_SEL_IN_4,
// RPI
    C_SEL_IN_5,
// I2S transport
    C_SEL_IN_6,
// USB
    C_SEL_IN_7,
// ??
    C_SEL_IN_8,
// SPDIF 1-8
    C_SEL_IN_9,
    C_SEL_IN_10,
    C_SEL_IN_11,
    C_SEL_IN_12,
    C_SEL_IN_13,
    C_SEL_IN_14,
    C_SEL_IN_15,
    C_SEL_IN_16,
// volume
    C_VOL_UP,
    C_VOL_DN,
    C_VOL_MUTE,
    C_VOL_UNMUTE,
// play
    C_P_PLAY,
    C_P_STOP,
    C_P_PAUSE,
    C_P_NEXT_TRACK,
    C_P_PREV_TRACK,
    C_P_FF,
    C_P_RW,
// keys
    C_K_LEFT,
    C_K_RIGHT,
    C_K_UP,
    C_K_DOWN,
    C_K_JD_UP,
    C_K_JD_DOWN,
    C_K_JD_LEFT,
    C_K_JD_RIGHT,
    C_K_0,
    C_K_1,
    C_K_2,
    C_K_3,
    C_K_4,
    C_K_5,
    C_K_6,
    C_K_7,
    C_K_8,
    C_K_9,
    C_K_ASTERISK,
    C_K_SHARP,
    C_K_MENU,
    C_K_DISPLAY,
    
// read
    C_R_STATUS=128,
    C_R_VOL,
    C_R_TRK_NUM,
    C_R_TRK_TITLE,
    C_R_TRK_TIME,
    C_R_TRK_REST,
    C_R_PWR_STATUS,

    C_K_KEYRELEASED=255
};

#endif
