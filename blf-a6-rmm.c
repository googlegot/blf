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

#include "driver.h"
#include "default_modes.h"

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

// counter for entering config mode
// (needs to be remembered while off, but only for up to half a second)
uint8_t fast_presses __attribute__ ((section (".noinit")));

#ifdef LOCK_MODE
// LOCK_MODE variable
uint8_t locked_in  __attribute__ ((section (".noinit")));
#endif

const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
    255,      // Ceiling, don't remove
};

void save_mode_idx(uint8_t idx) {  // central method for writing (with wear leveling)
    // a single 16-bit write uses less ROM space than two 8-bit writes
#if ( ATTINY == 13 || ATTINY == 25 )
    uint8_t oldpos=eepos;
#elif ( ATTINY == 85 ) 
    uint16_t oldpos=eepos;
#endif
    eepos = (eepos+1) & (EEPMODE - 1);  // wear leveling, use next cell
    eeprom_write_byte((uint8_t *)(eepos), ~idx);      // save current index, flipped
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);    // erase old state
}

inline uint8_t restore_mode_idx() {
    uint8_t eep;
    // find the config data
    for(eepos=0; eepos<EEPMODE; eepos++) {
        eep = ~(eeprom_read_byte((const uint8_t *)eepos));
        if (eep != 0x00) {
            return eep;
        }
    } 
    return 0;
}


void save_config(uint8_t config) {
    eeprom_write_byte((uint8_t *)(EEPLEN - 2), ~config); // save the config
}

inline uint8_t restore_config() {
    return ~eeprom_read_byte((uint8_t *)(EEPLEN - 2));
}

#ifdef TEMP_CAL_MODE
void save_maxtemp(uint8_t maxtemp){
    eeprom_update_byte((uint8_t *)(EEPLEN - 3), maxtemp);
}

inline uint8_t restore_maxtemp(){
    uint8_t maxtemp = eeprom_read_byte((uint8_t *)(EEPLEN - 3));
    if ( maxtemp == 0 ) { maxtemp = 79; }
    return maxtemp;
}
#endif

inline uint8_t next(uint8_t i, int8_t dir, uint8_t start){
    i += dir;
    if (i > solid_high || i < solid_low){
        i = start;
    }
    return i;
}

inline uint8_t med(uint8_t i, int8_t dir, uint8_t start){
    if (i == (HIDDEN_HIGH)) {
        // If we hit the end of the hidden modes, go back to start 
        i = start;
    } else if (i <= solid_high && i > solid_low){
        // regular mode: under solid_high, above solid_low
        i -= dir;
        if (i > solid_high) { i = solid_low; };
    } else if (i < (HIDDEN_HIGH)) {
        i ++;
    } else {
        // Otherwise, jump to hidden modes 
        i = HIDDEN_LOW;
    }
    return i;
}
#ifdef TEMP_CAL_MODE
inline void ADC_on_temperature() {
    // select ADC4 by writing 0b00001111 to ADMUX
    // 1.1v reference, left-adjust, ADC4
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | TEMP_CHANNEL;
    // disable digital input on ADC pin to reduce power consumption
    //DIDR0 |= (1 << TEMP_DIDR);
    // enable, start, prescale
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;
}
#endif
void ADC_on() {
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
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
    save_mode_idx(idx);
}

void blink(uint8_t val, uint8_t speed, uint8_t brightness) {
    for (; val>0; val--)
    {
        set_output(brightness,0);
        _delay_ms(speed);
        set_output(0,0);
        _delay_ms(speed); _delay_ms(speed);
    }
}

uint8_t get_voltage(){
    // Get the voltage for later 
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    return ADCH;
}

void emergency_shutdown(){
    // Shut down, voltage is too low.
    blink(3, 50, 30);
    set_output(0,0);
    // Power down as many components as possible
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_mode();
}
#ifdef TEMP_CAL_MODE
uint8_t get_temperature() {
    ADC_on_temperature();
    // average a few values; temperature is noisy
    uint16_t temp = 0;
    uint8_t i;
    get_voltage();
    for(i=0; i<16; i++) {
        temp += get_voltage();
    }
    temp >>= 4;
    return temp;
}
#endif

#ifdef LOCK_MODE
void set_lock() {
    if (locked_in != 1 && ((config & LOCK_MODE) != 0)) {
        _delay_s(); _delay_s(); _delay_s();
        locked_in = 1;
    }
}
#endif
int main(void) {
    uint8_t cap_val;
    uint8_t i=2;
    // Read the off-time cap *first* to get the most accurate reading
    // Start up ADC for capacitor pin
    DIDR0 |= (1 << CAP_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale

    // Read cap value, twice per datasheet
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    ADCSRA |= (1 << ADSC);
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
    
    config = restore_config();
    // Wipe the config if option 5 (with LOCK MODE undefined)
    // or option 6 (with LOCK MODE defined) is 1
    // or config is empty (fresh flash)
    if ((config & CONFIGMAX) !=0 || config == 0) {
        config = CONFIG_DEFAULT;
        save_config(config);
    }
    
    uint8_t mode_idx = restore_mode_idx();
#ifdef TEMP_CAL_MODE
    uint8_t maxtemp = restore_maxtemp();
#endif
    
    /*
     * Determine how many solid and hidden modes we have, and where the 
     * boundries for each mode group are.
     *
     * This is needed for mode group selection.
     *
     */

    if ((config & MODE_GROUP) == 0) {
        solid_low  = SOLID_LOW1; 
        solid_high = SOLID_HIGH1;
    } else {
        solid_low  = SOLID_LOW2;
        solid_high = SOLID_HIGH2;
    }
   
    //set the direction and first mode
    int8_t dir;
    uint8_t start;
    if ((config & MODE_DIR) == 0){
        dir = 1;
        start = solid_low;
    } else {
        dir = -1;
        start = solid_high;
    }

    if (cap_val < CAP_MED || (cap_val < CAP_SHORT && ((config & MED_PRESS) == 0))) {
        // Long press, keep the same mode
        // ... or reset to the first mode
        fast_presses = 0;
        if ((config & MEMORY) == 0) {
            // Reset to the first mode
            mode_idx = start;
        }
#ifdef LOCK_MODE
        locked_in = 0;
    } else if (locked_in != 0 && ((config & LOCK_MODE) != 0)) {
        // Do nothing
#endif    
    } else if (cap_val < CAP_SHORT) {
        fast_presses = 0;
        // User did a medium press, go back one mode
        mode_idx = med(mode_idx, dir, start);
    } else {
        // We don't care what the value is as long as it's over 15
        fast_presses = (fast_presses+1) & 0x1f;
        // Indicates they did a short press, go to the next mode
        mode_idx = next(mode_idx, dir, start); // Will handle wrap arounds

    }
    save_mode_idx(mode_idx);

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High
    ADC_on();

    uint8_t ticks = 0;
    uint8_t output;
    uint8_t lowbatt_overheat_cnt = 0;
    uint8_t voltage = get_voltage();
    
    if (voltage < ADC_LOW) {
        // Protect the battery if we're just starting and the voltage is too low.
        emergency_shutdown();
    } else if (voltage < ADC_0){
        // If the battery is getting low, flash twice when turning on or changing brightness
        blink(2, 50, 30);
    }
    
    while(1) {
#ifdef TEMP_CAL_MODE
		uint8_t temp=get_temperature();
		ADC_on();
#endif
        voltage = get_voltage();

        output = modesNx[mode_idx];

        if (fast_presses > 0x0f) {  // Config mode
            _delay_s();       // wait for user to stop fast-pressing button
            fast_presses = 0; // exit this mode after one use
            mode_idx = solid_low;
#ifdef TEMP_CAL_MODE
            output = TEMP_CAL_MODE;
#endif
            // Loop through each config option, toggle, blink the mode number,
            // buzz a second for user to confirm, toggle back.
            //
            // Config items:
            //
            // 1 = Mode Group
            // 2 = Mode Memory
            // 4 = Reverse Mode Order
            // 8 = Medium Press Disable
            // 16 = Mode Locking 
            //
            // Each toggle's blink count will be
            // linear, so 1 blink for Mode Group,
            // 3 blinks for Reverse Mode Order,
            // 4 blinks for Medium Press.

            uint8_t blinks=1;
            for (i=1; i<=CONFIGMAX; i<<=1, blinks++) {
                blink(blinks, 124, 30);
                _delay_ms(50);
                config ^= i;
                save_config(config);
                blink(48, 15, 20);
                config ^= i;
                save_config(config);
                _delay_s();
            }

        }
#ifdef SOS
        if (output == SOS) {
            blink(3,100,255);
            _delay_ms(200);
            blink(3,200,255);
            blink(3,100,255);
            _delay_s();
        } else 
#endif
#ifdef TEMP_CAL_MODE
        if (output == TEMP_CAL_MODE) {
            blink(5, 124, 30);
            save_maxtemp(255);
            _delay_s(); _delay_s();
            set_output(255,0);
            while(1) {
                maxtemp = get_temperature();
                save_maxtemp(maxtemp);
                _delay_s();
            }
        } else 
#endif
        if (output == STROBE || output == BIKING_STROBE) {
            // 10Hz tactical strobe
            blink(4, 25, 255);
            if (output == BIKING_STROBE) {
                // 2-level stutter beacon for biking and such
                // normal version
                set_output(0,255);
#ifdef LOCK_MODE
                set_lock();
#endif
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
            if ((ticks > TURBO_TIMEOUT) && (output == TURBO)) {
                mode_idx = solid_high - 2; // step down to second-highest mode
            }
            set_mode(mode_idx);
#ifdef LOCK_MODE
            set_lock();
#endif
            _delay_s();
        }
#ifdef TEMP_CAL_MODE        
        if (voltage < ADC_LOW || temp >= maxtemp) {
#else
        if (voltage < ADC_LOW) {
#endif
            lowbatt_overheat_cnt ++;
        } else {
            lowbatt_overheat_cnt = 0;
        }

        // See if it's been low for a while, and maybe step down
        if (lowbatt_overheat_cnt >= 15) {
            // Skip FET and blinky modes if they're in the normal mode rotation
            for (i=mode_idx; i>solid_low && modesNx[i] > 0; i--){}
            
            if (i < solid_low) { 
                i=solid_low; // If in hidden modes, drop to low       
            } else if (i == solid_low) {
                // If we're already at solid_low, save state at low and turn off
                mode_idx = i;
                set_mode(i);
                emergency_shutdown();
            } else {
                // Fet is inactive, we're not in a hidden mode, drop down a level
                i--;
            }
            
            mode_idx = i; 
            set_mode(i);
            lowbatt_overheat_cnt = 0;
        }
        // If we got this far, the user has stopped fast-pressing.
        // So, don't enter config mode.
        fast_presses = 0;
    }
}
