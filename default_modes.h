// WARNING: You can only have a maximum of 16 modes TOTAL
// That means NUM_MODES1 + NUM_MODES2 + NUM_HIDDEN MUST be <= 16

// Normal modes (mode group 2 is defunct, it now increments mode_idx by 2)
#define NUM_MODES   8
#define MODESNx1    0,0,0,7,56,90,137,255      //(FET or Nx7135)
#define MODES1x1    3,20,110,255,255,255,255,0 //(1x7135)

// Hidden modes are *after* the normal modes
#define NUM_HIDDEN       6
#define HIDDENMODES      BATTCHECK,TURBO,STROBE,BIKING_STROBE,SOS,BEACON
#define HIDDENMODES_ALT  0,0,0,0,0,0   // Zeroes, same length as NUM_HIDDEN

#define TURBO         255 // Convenience code for turbo mode
#define BATTCHECK     254 // Convenience code for battery check mode
#define STROBE        253 // Convenience code for strobe mode
#define BIKING_STROBE 252 // Convenience code for biking strobe mode
#define SOS           251 // Convenience code for SOS mode
#define BEACON        250 // Convenience code for beacon mode

// Temp cal mode allows temperature monitoring on the attiny25/45/85.
// Since they are not the target of this firmware, I've put less time
// into making the temperature calibration process easy to use. Once
// I get the attiny13 hacked into temperature measurement, I'll make
// calibration more intuitive. 
//
// TODO: Measure temperature on attiny13a using datasheet spec for 
// WDT Frequency compared to CPU clock.  WDT clock decreases exponentially 
// with temperature, MCU frequency increases linearly.
//
#if (ATTINY == 85 || ATTINY == 25)
#define TEMP_CAL_MODE 1   // Convenience code for temperature calibration mode 
#endif

// How many timer ticks before before dropping down.
// Each timer tick is 1s, so "30" would be a 30-second stepdown.
// Max value of 255 unless you change "ticks"
#define TURBO_TIMEOUT 20 
// Turbo step down mode index
#define TURBO_STEP_DOWN (NUM_MODES - 2)

#define MODE_CNT (NUM_MODES + NUM_HIDDEN - 1) // Subtract 1 since mode_idx starts at 0

// Modes
const uint8_t modesNx[] = { MODESNx1, HIDDENMODES };
const uint8_t modes1x[] = { MODES1x1, HIDDENMODES_ALT };

// config / state variables
// config bitfield
#define MUGGLE       1   // Muggle mode (max two steps below turbo, no medium press)
#define MEMORY       2   // Memory on or off
#define MOON_MODE    4   // When set, disable moon mode
#define MODE_DIR     8   // Mode order reversal
#define MODE_GROUP   16  // Mode group
#define MED_PRESS    32  // Disable medium press
#define LOCK_MODE    64  // "Lock in" to mode after 3 seconds
#define CONFIG_SET   128 // if 0, set CONFIG_DEFAULT MUST always be the last user-configurable mode

// Set the bit value of the config mode you'd like when starting fresh,
// or when the config is wiped
#define CONFIG_DEFAULT (CONFIG_SET + MODE_GROUP + MOON_MODE) // 4 modes default
