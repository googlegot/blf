// WARNING: You can only have a maximum of 16 modes TOTAL
// That means NUM_MODES1 + NUM_MODES2 + NUM_HIDDEN MUST be <= 16

// Mode group 1
#define NUM_MODES1          7
#define MODESNx1            0,0,0,7,56,137,255 //(FET or Nx7135)
#define MODES1x1            3,20,110,255,255,255,0 //(1x7135)

// Mode group 2
#define NUM_MODES2          4
#define MODESNx2            0,0,90,255
#define MODES1x2            3,110,255,0

// Hidden modes are *before* the lowest (moon) mode
#define NUM_HIDDEN          4
#define HIDDENMODES         TURBO,STROBE,BATTCHECK,BIKING_STROBE
#define HIDDENMODES_ALT     0,0,0,0   // Zeroes, same length as NUM_HIDDEN

#define TURBO     255       // Convenience code for turbo mode
#define BATTCHECK 254       // Convenience code for battery check mode
#define STROBE    253       // Convenience code for strobe mode
#define BIKING_STROBE 252   // Convenience code for biking strobe mode

// Temp cal mode allows temperature monitoring.  It is HUGE and really only works 
// on the attiny13 if you disable a few other options.  It is also broken at the moment,
// feel free to fix it! 
//#define TEMP_CAL_MODE 251   // Convenience code for temperature calibration mode 

//#define SOS 250           // Convenience code for SOS mode

// How many timer ticks before before dropping down.
// Each timer tick is 1s, so "30" would be a 30-second stepdown.
// Max value of 255 unless you change "ticks"
#define TURBO_TIMEOUT       60

#define MODE_CNT (NUM_HIDDEN + NUM_MODES1 + NUM_MODES2)
#define HIDDEN_HIGH (NUM_HIDDEN - 1)
#define HIDDEN_LOW 0
#define SOLID_LOW1  (MODE_CNT - NUM_MODES1 - NUM_MODES2);
#define SOLID_HIGH1 (MODE_CNT - NUM_MODES2 - 1);
#define SOLID_LOW2  (MODE_CNT - NUM_MODES2);
#define SOLID_HIGH2 (MODE_CNT - 1);

/*
 * global variables
 */

uint8_t solid_low;
uint8_t solid_high;

// modes (gets set when the light starts up based on saved config values)
const uint8_t modesNx[] = { HIDDENMODES, MODESNx1, MODESNx2 };
const uint8_t modes1x[] = { HIDDENMODES_ALT, MODES1x1, MODES1x2 };

// config / state variables
// config bitfield
#define MODE_GROUP   1
#define MEMORY       2
#define MODE_DIR     4
#define MED_PRESS    8
#define CONFIG_SET 128
// Lock mode disables moving to another mode after 3 seconds of being in that mode.
// To exit the mode, turn the flashlight off for 3 seconds, and it will either start
// from the beginning or continue where you left off depending on whether mode memory
// is enabled.
//
// Comment out the next line to disable this function.
#define LOCK_MODE   16

// Set the bit value of the config mode you'd like when starting fresh,
// or when the config is wiped
#define CONFIG_DEFAULT (MODE_DIR + CONFIG_SET) // Medium press enabled by default

#ifdef LOCK_MODE
#define CONFIGMAX 32
#else
#define CONFIGMAX 16
#endif

uint8_t config = CONFIG_DEFAULT;
