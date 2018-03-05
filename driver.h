// Required libraries 
//#include <avr/pgmspace.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>
#include <util/delay_basic.h>

// Choose your MCU here, or in the build script
//#define ATTINY 13
//#define ATTINY 25
//#define ATTINY 85

// set some hardware-specific values...
// (while configuring this firmware, skip this section)
#if (ATTINY == 13)
#define F_CPU 4800000UL
#define EEPMODE 61
#define EEPLEN 63
#elif (ATTINY == 25)
#define F_CPU 8000000UL
#define EEPMODE 125
#define EEPLEN 127
#elif (ATTINY == 85)
#define F_CPU 8000000UL
// Saving space by limiting eeprom to 8 bit addressable space
#define EEPMODE 253
#define EEPLEN 255
#else
Hey, you need to define ATTINY.
#endif

#if (ATTINY == 13)
#define V_REF REFS0
#elif (ATTINY == 25 || ATTINY == 85)
#define V_REF REFS1
#endif

/*
 * =========================================================================
 * Settings to modify per driver
 */
//#define DEBUG

//#define FAST 0x23           // fast PWM channel 1 only
//#define PHASE 0x21          // phase-correct PWM channel 1 only
#define FAST 0xA3           // fast PWM both channels
#define PHASE 0xA1          // phase-correct PWM both channels

#define OWN_DELAY           // Should we use the built-in delay or our own?
// Adjust the timing per-driver, since the hardware has high variance
// Higher values will run slower, lower values run faster.
#if (ATTINY == 13)
#define DELAY_TWEAK         950
#define DELAY_TWEAK_10ms    9500
#elif (ATTINY == 25 || ATTINY == 85)
#define DELAY_TWEAK         2000
#define DELAY_TWEAK_10ms    19000
#endif

// These values were measured using wight's "A17HYBRID-S" driver built by DBCstm.
// Your mileage may vary.
#define ADC_100         170 // the ADC value for 100% full (4.2V resting)
#define ADC_75          162 // the ADC value for 75% full (4.0V resting)
#define ADC_50          154 // the ADC value for 50% full (3.8V resting)
#define ADC_25          141 // the ADC value for 25% full (3.5V resting)
#define ADC_0           121 // the ADC value for 0% full (3.0V resting)
#define ADC_LOW         113 // When do we start ramping down (2.8V)
#define ADC_CRIT        109 // When do we shut the light off (2.7V)

#define TEMP_CHANNEL 0x0f

// the BLF EE A6 driver may have different offtime cap values than most other drivers
// Values are between 1 and 255, and can be measured with offtime-cap.c
// These #defines are the edge boundaries, not the center of the target.
#define CAP_SHORT           230  // Anything higher than this is a short press
#define CAP_MED             160  // Between CAP_MED and CAP_SHORT is a medium press
                                 // Below CAP_MED is a long press

#define CAP_PIN     PB3
#define CAP_CHANNEL 0x03    // MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR    ADC3D   // Digital input disable bit corresponding with PB3
#define PWM_PIN     PB1
#define ALT_PWM_PIN PB0
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64
#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1
#define ALT_PWM_LVL OCR0A   // OCR0A is the output compare register for PB0

/*
 * =========================================================================
 */


void _delay_ms(uint8_t n)
{
    // TODO: make this take tenths of a ms instead of ms,
    // for more precise timing?
    while(n-- > 0) _delay_loop_2(DELAY_TWEAK);
}
//void _delay_s()  // because it saves a bit of ROM space to do it this way
//{
//	_delay_ms(1000);
//}

// Max delay time 2550ms
void _delay_10_ms(uint8_t n)
{
   	while(n-- > 0) _delay_loop_2(DELAY_TWEAK_10ms);
}
void _delay_s()  // because it saves a bit of ROM space to do it this way
{
	_delay_10_ms(100);
}


