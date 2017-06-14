/*
 * BLF EE A6 firmware (special-edition group buy light)
 * This light uses a FET+1 style driver, with a FET on the main PWM channel
 * for the brightest high modes and a single 7135 chip on the secondary PWM
 * channel so we can get stable, efficient low / medium modes.  It also
 * includes a capacitor for measuring off time.
 *
 * Copyright (C) 2015 Selene Scriven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *     OTC -|   |- Voltage ADC
 *  Star 3 -|   |- PWM (FET)
 *     GND -|   |- PWM (1x7135)
 *           ---
 *
 * FUSES
 *      I use these fuse settings
 *      Low:  0x75  (4.8MHz CPU without 8x divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM)
 *      High: 0xfd  (to enable brownout detection)
 *
 *      For more details on these settings, visit http://github.com/JCapSolutions/blf-firmware/wiki/PWM-Frequency
 *
 * VOLTAGE
 *      Resistor values for voltage divider (reference BLF-VLD README for more info)
 *      Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *           VCC
 *            |
 *           Vd (~.25 v drop from protection diode)
 *            |
 *          1912 (R1 19,100 ohms)
 *            |
 *            |---- PB2 from MCU
 *            |
 *          4701 (R2 4,700 ohms)
 *            |
 *           GND
 *
 *   To find out what values to use, flash the driver with battcheck.hex
 *   and hook the light up to each voltage you need a value for.  This is
 *   much more reliable than attempting to calculate the values from a
 *   theoretical formula.
 *
 *   Same for off-time capacitor values.  Measure, don't guess.
 */
// Choose your MCU here, or in the build script
#define ATTINY 13
//#define ATTINY 25


// set some hardware-specific values...
// (while configuring this firmware, skip this section)
//#if (ATTINY == 13)
#define F_CPU 4800000UL
#define EEPLEN 64
//#elif (ATTINY == 25)
//#define F_CPU 8000000UL
//#define EEPLEN 128
//#else
//Hey, you need to define ATTINY.
//#endif

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
//#if (ATTINY == 13)
#define DELAY_TWEAK         950
//#elif (ATTINY == 25)
//#define DELAY_TWEAK         2000
//#endif

#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long

// WARNING: You can only have a maximum of 16 modes TOTAL
// That means NUM_MODES1 + NUM_MODES2 + NUM_HIDDEN MUST be <= 16
// Mode group 1
#define NUM_MODES1          7
// PWM levels for the big circuit (FET or Nx7135)
#define MODESNx1            0,0,0,7,56,137,255
// PWM levels for the small circuit (1x7135)
#define MODES1x1            3,20,110,255,255,255,0
// My sample:     6=0..6,  7=2..11,  8=8..21(15..32)
// Krono sample:  6=5..21, 7=17..32, 8=33..96(50..78)
// Manker2:       2=21, 3=39, 4=47, ... 6?=68
// PWM speed for each mode
//#define MODES_PWM1          PHASE,FAST,FAST,FAST,FAST,FAST,PHASE
// Mode group 2
#define NUM_MODES2          4
#define MODESNx2            0,0,90,255
#define MODES1x2            3,110,255,0
//#define MODES_PWM2          PHASE,FAST,FAST,PHASE

// Hidden modes are *before* the lowest (moon) mode
#define NUM_HIDDEN          4
#define HIDDENMODES         TURBO,STROBE,BATTCHECK,BIKING_STROBE
//#define HIDDENMODES_PWM     PHASE,PHASE,PHASE,PHASE
#define HIDDENMODES_ALT     0,0,0,0   // Zeroes, same length as NUM_HIDDEN

#define TURBO     255       // Convenience code for turbo mode
#define BATTCHECK 254       // Convenience code for battery check mode
// Uncomment to enable tactical strobe mode
#define STROBE    253       // Convenience code for strobe mode
// Uncomment to unable a 2-level stutter beacon instead of a tactical strobe
#define BIKING_STROBE 252   // Convenience code for biking strobe mode

// How many timer ticks before before dropping down.
// Each timer tick is 1s, so "30" would be a 30-second stepdown.
// Max value of 255 unless you change "ticks"
#define TURBO_TIMEOUT       60

// These values were measured using wight's "A17HYBRID-S" driver built by DBCstm.
// Your mileage may vary.
#define ADC_100         170 // the ADC value for 100% full (4.2V resting)
#define ADC_75          162 // the ADC value for 75% full (4.0V resting)
#define ADC_50          154 // the ADC value for 50% full (3.8V resting)
#define ADC_25          141 // the ADC value for 25% full (3.5V resting)
#define ADC_0           121 // the ADC value for 0% full (3.0V resting)
#define ADC_LOW         113 // When do we start ramping down (2.8V)
#define ADC_CRIT        109 // When do we shut the light off (2.7V)

// the BLF EE A6 driver may have different offtime cap values than most other drivers
// Values are between 1 and 255, and can be measured with offtime-cap.c
// These #defines are the edge boundaries, not the center of the target.
#ifdef OFFTIM3
#define CAP_SHORT           230  // Anything higher than this is a short press
#define CAP_MED             160  // Between CAP_MED and CAP_SHORT is a medium press
                                 // Below CAP_MED is a long press
#else
#define CAP_SHORT           115
#endif

/*
 * =========================================================================
 */

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#ifdef OWN_DELAY
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
void _delay_ms(uint16_t n)
{
    // TODO: make this take tenths of a ms instead of ms,
    // for more precise timing?
    while(n-- > 0) _delay_loop_2(DELAY_TWEAK);
}
void _delay_s()  // because it saves a bit of ROM space to do it this way
{
    _delay_ms(1000);
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

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

// Config / state variables
// Config bitfield
// mode group  = 1
// memory      = 2
// mode_dir    = 4
// med_press   = 8
uint8_t config = 8;

uint8_t eepos = 0;
// counter for entering config mode
// (needs to be remembered while off, but only for up to half a second)
uint8_t fast_presses __attribute__ ((section (".noinit")));


// Modes (gets set when the light starts up based on saved config values)
const uint8_t modesNx[] = { HIDDENMODES, MODESNx1, MODESNx2 };
const uint8_t modes1x[] = { HIDDENMODES_ALT, MODES1x1, MODES1x2 };

const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
    255,      // Ceiling, don't remove
};

void save_state(uint8_t idx) {  // central method for writing (with wear leveling)
    // a single 16-bit write uses less ROM space than two 8-bit writes
    uint8_t eep;
    uint8_t oldpos=eepos;

    eepos = (eepos+1) & (EEPLEN-1);  // wear leveling, use next cell
    // Bitfield all the things!
    // This limits the max number of brightness settings to 15.  More will clobber config settings.
    // Layout is: IIIINMRP (mode (I)ndex, mode (N)umber, mode (M)emory, mode (R)everse, medium (P)ress) 
    eep = ~(config | (idx << 4));
    // Flip it so if all config options are enabled and index is 15, we still find the config
    eeprom_write_byte((uint8_t *)(eepos), eep);      // save current state
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);    // erase old state
}

inline uint8_t restore_state() {
    uint8_t eep;
    // find the config data
    for(eepos=0; eepos<EEPLEN; eepos++) {
        eep = ~(eeprom_read_byte((const uint8_t *)eepos));
        if (eep != 0x00) {
            return eep;
        }
    } 
    return 0;
}

inline uint8_t next(uint8_t i){
    i ++;
    if (i > solid_high || i < solid_low){
        i = solid_low;
    }
    return i;
}

inline uint8_t prev(uint8_t i){
    i --;
    if (i > solid_high || i < solid_low) {
        i = solid_high;
    }
    return i;
}

inline uint8_t med(uint8_t i){
    if ((config & 4) == 0) {
        if (i == (HIDDEN_HIGH)) {
            // If we hit the end of the hidden modes, go back to moon
            i = solid_low;
	} else if (i <= solid_high && i > solid_low){
            // regular mode: under solid_high, above solid_low
            i --;
        } else if (i < (HIDDEN_HIGH)) {
            i ++;
        } else {
            // Otherwise, jump to hidden modes 
            i = HIDDEN_LOW;
        }
    } else {
	// reverse biased version of the same thing
	// This skips hidden turbo, since going turbo->turbo
    // just don't make no sense.
        if (i == (HIDDEN_HIGH)) {
            i = solid_high;
        } else if (i < solid_high && i > solid_low) {
            i ++;
        } else if (i < (HIDDEN_HIGH)) {
           i ++;
        } else {
            i = HIDDEN_LOW + 1;
        }
    }
    return i;
}

inline void ADC_on() {
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
//#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
//#elif (ATTINY == 25)
//    ADMUX  = (1 << REFS1) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
//#endif
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void set_output(uint8_t pwm1, uint8_t pwm2) {
    PWM_LVL = pwm1;
    ALT_PWM_LVL = pwm2;
}

#ifdef DEBUG            
// Blink out the contents of a byte
void debug_byte(uint8_t byte) {
    uint8_t x;
    for ( x=0; x <= 7; x++ ) {
        set_output(0,0);
        _delay_ms(500);
        if ((byte & (0x01 << x)) == 0 ) {
            set_output(0,15);
        } else {
            set_output(0,120);
        }
        _delay_ms(100);
    }
    set_output(0,0);
}
#endif

void set_mode(uint8_t idx) {
    set_output(modesNx[idx], modes1x[idx]);
    save_state(idx);
}

void blink(uint8_t val, uint8_t speed, uint8_t brightness) {
    for (; val>0; val--)
    {
        set_output(brightness,0);
        _delay_ms(speed);
        set_output(0,0);
        _delay_ms(speed);
        _delay_ms(speed);
    }
}

int main(void) {
    uint8_t mode_idx=NUM_HIDDEN;
    uint8_t cap_val;
    // Read the off-time cap *first* to get the most accurate reading
    // Start up ADC for capacitor pin
    DIDR0 |= (1 << CAP_DIDR);                           // disable digital input on ADC pin to reduce power consumption
//#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
//#elif (ATTINY == 25)
    //ADMUX  = (1 << REFS1) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
//#endif
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale

    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    cap_val = ADCH; // save this for later

    // Set PWM pin to output
    DDRB |= (1 << PWM_PIN);     // enable main channel
    DDRB |= (1 << ALT_PWM_PIN); // enable second channel

    // Set timer to do PWM for correct output pin and set prescaler timing
    //TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    //TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
    TCCR0A = PHASE;
    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Read config values and saved state
    uint8_t eep=restore_state();
    // unpack the config data
    if (eepos < EEPLEN) {
        config = (eep & ~0xF0);
        mode_idx = (eep >> 4 );
    }

    // Enable the current mode group

    /*
     * Determine how many solid and hidden modes we have, and where the 
     * boundries for each mode group are.
     *
     * This is needed for mode group selection.
     *
     */

    if ((config & 1) == 0) {
        solid_low  = SOLID_LOW1; 
        solid_high = SOLID_HIGH1;
    } else {
        solid_low  = SOLID_LOW2;
        solid_high = SOLID_HIGH2;
    }
    
    if (cap_val > CAP_SHORT) {
        // We don't care what the value is as long as it's over 15
        fast_presses = (fast_presses+1) & 0x1f;
        // Indicates they did a short press, go to the next mode
        if ((config & 4) == 0){ 
            mode_idx = next(mode_idx); // Will handle wrap arounds
        } else {
            mode_idx = prev(mode_idx);
        }

    } else if (cap_val > CAP_MED && ((config & 8) != 0)) {
        fast_presses = 0;
        // User did a medium press, go back one mode
        mode_idx = med(mode_idx);
    
    } else {
        // Long press, keep the same mode
        // ... or reset to the first mode
        fast_presses = 0;
        if ((config & 2) == 0) {
            // Reset to the first mode
            if ((config & 4) == 0 ){
                mode_idx = solid_low;
            } else {
                mode_idx = solid_high;
            }
        }
    }
    save_state(mode_idx);

    // Turn off ADC
    //ADC_off();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High

    // Turn features on or off as needed
    ADC_on();
    //ACSR   |=  (1<<7); //AC off

    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts
   // set_sleep_mode(SLEEP_MODE_IDLE);  // not used due to blinky modes
    uint8_t ticks = 0;
    uint8_t output;
    uint8_t lowbatt_cnt = 0;
    uint8_t i = 0;
    uint8_t voltage;
    while(1) {
        // Get the voltage for later 
        ADCSRA |= (1 << ADSC);
        // Wait for completion
        while (ADCSRA & (1 << ADSC));
        voltage = ADCH;
        output = modesNx[mode_idx];
        if (fast_presses > 0x0f) {  // Config mode
            _delay_s();       // wait for user to stop fast-pressing button
            fast_presses = 0; // exit this mode after one use
            mode_idx = solid_low;

            // Loop through each config option,
            // toggle, blink the mode number,
            // wait a second for user to confirm,
            // toggle back.
            //
            // Config items:
            //
            // 1 = Mode Group
            // 2 = Mode Memory
            // 4 = Reverse Mode Order
            // 8 = Medium Press Disable
            //
            // Each toggle's blink count will be
            // linear, so 1 blink for Mode Group,
            // 3 blinks for Reverse Mode Order,
            // 4 blinks for Medium Press. 
            uint8_t item=1;
            uint8_t blinks=1;
            for (; item<=8; item<<=1, blinks++) {
                blink(blinks, 124, 30);
                _delay_ms(50);
                config ^= item;
                save_state(mode_idx);
                blink(48, 15, 20);
                config ^= item;
                save_state(mode_idx);
                _delay_s();
            }
        } else if (output == STROBE || output == BIKING_STROBE) {
            // 10Hz tactical strobe
            blink(4, 25, 255);
            if (output == BIKING_STROBE) {
                // 2-level stutter beacon for biking and such
                // normal version
                set_output(0,255);
                _delay_s();
            }
        } else if (output == BATTCHECK) {
            // figure out how many times to blink
            for (i=0; voltage > voltage_blinks[i]; i ++) {}
            // blink zero to five times to show voltage
            // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
            blink(i, 124, 30);
            // wait between readouts
            _delay_s();
        } else {  // Regular non-hidden solid mode
            // Do some magic here to handle turbo step-down
            ticks ++;
            if ((ticks > TURBO_TIMEOUT) && (mode_idx == solid_high)) {
                mode_idx = solid_high - 1; // step down to second-highest mode
            }
            set_mode(mode_idx);
            // Otherwise, just sleep.
            _delay_s();
        }
        
        if (voltage < ADC_LOW) {
            lowbatt_cnt ++;
        } else {
            lowbatt_cnt = 0;
        }
        // See if it's been low for a while, and maybe step down
        if (lowbatt_cnt >= 8) {
            i = mode_idx; // save space by not accessing mode_idx more than necessary
            // properly track hidden vs normal modes
            if (i < solid_low) {
                // step down from blinky modes to medium
                i = solid_high - 2;
            } else if (mode_idx > solid_low) {
                // step down from solid modes one at a time
                i --;
            } else { // Already at the lowest mode
                i = solid_low;
                // Turn off the light
                set_output(0,0);
                // Power down as many components as possible
                set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                sleep_mode();
            }
            mode_idx = i;
            set_mode(mode_idx);
            lowbatt_cnt = 0;
            // Wait at least 2 seconds before lowering the level again
            _delay_ms(250);  // this will interrupt blinky modes
        }
        // If we got this far, the user has stopped fast-pressing.
        // So, don't enter config mode.
        fast_presses = 0;
    }
}
