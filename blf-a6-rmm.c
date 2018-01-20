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
 *		   ---
 *		 -|   |- VCC
 *	 OTC -|   |- Voltage ADC
 *  Star 3 -|   |- PWM (FET)
 *	 GND -|   |- PWM (1x7135)
 *		   ---
 *
 * FUSES
 *	  I use these fuse settings
 *	  Low:  0x75  (4.8MHz CPU without 8x divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM)
 *	  High: 0xfd  (to enable brownout detection)
 *
 *	  For more details on these settings, visit http://github.com/JCapSolutions/blf-firmware/wiki/PWM-Frequency
 *
 * VOLTAGE
 *	  Resistor values for voltage divider (reference BLF-VLD README for more info)
 *	  Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *		   VCC
 *			|
 *		   Vd (~.25 v drop from protection diode)
 *			|
 *		  1912 (R1 19,100 ohms)
 *			|
 *			|---- PB2 from MCU
 *			|
 *		  4701 (R2 4,700 ohms)
 *			|
 *		   GND
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

// counter for entering config mode
// (needs to be remembered while off, but only for up to half a second)
uint8_t fast_presses __attribute__ ((section (".noinit")));

#ifdef LOCK_MODE
// LOCK_MODE variable
uint8_t locked_in  __attribute__ ((section (".noinit")));
#endif

// Initialize the config variable
uint8_t config = CONFIG_DEFAULT;
#ifdef TEMP_CAL_MODE
uint8_t maxtemp;
#endif

const uint8_t voltage_blinks[] = {
	ADC_0,	// 1 blink  for 0%-25%
	ADC_25,   // 2 blinks for 25%-50%
	ADC_50,   // 3 blinks for 50%-75%
	ADC_75,   // 4 blinks for 75%-100%
	ADC_100,  // 5 blinks for >100%
	255
};

// EEPROM_read/write taken from the datasheet
void EEPROM_write(uint8_t Address, uint8_t Data) {
	// Set Programming mode
	EECR = (0<<EEPM1)|(0>>EEPM0);

	// Set up address and data registers
	EEARL = Address;
	EEDR = Data;

	// Write logical one to EEMPE
	EECR |= (1<<EEMPE);

	// Start eeprom write by setting EEPE
	EECR |= (1<<EEPE);
	
	// Wait for completion of write
	while(EECR & (1<<EEPE));
}

uint8_t EEPROM_read(uint8_t Address) {
	// Set up address register
	EEARL = Address;
	// Start eeprom read by writing EERE
	EECR |= (1<<EERE);
	// Return data from data register
	return EEDR;
}

void save_mode_idx(uint8_t idx) {  // Write mode index to EEPROM (with wear leveling)
#if ( ATTINY == 13 || ATTINY == 25 )
	uint8_t oldpos=eepos;
#elif ( ATTINY == 85 )
	uint16_t oldpos=eepos;
#endif
	// Reverse the index again if we're reversed
	if (((config & MODE_DIR) == 4) && (idx < NUM_MODES)) {
		idx = (NUM_MODES - 1 - idx);
	}
	
	eepos = (eepos+1) & (EEPMODE - 1);            // wear leveling, use next cell
	EEPROM_write(eepos, ~idx);  // save current index, flipped
	EEPROM_write(oldpos, 0xff); // erase old state
}

inline uint8_t restore_mode_idx() {
	uint8_t eep;
	// find the config data
	for(eepos=0; eepos<EEPMODE; eepos++) {
		eep = ~(EEPROM_read(eepos));
		if (eep != 0x00) {
			return eep;
		}
	}
	return 0;
}

#ifdef TEMP_CAL_MODE
void save_config(){
	// Save both the max temperature and config
	EEPROM_write((EEPLEN - 2), maxtemp);
	EEPROM_write((EEPLEN - 1), ~config);
}

inline void restore_config(){
	maxtemp = EEPROM_read((EEPLEN - 2));
	config = ~EEPROM_read((EEPLEN - 1));
	if ( maxtemp == 0 ) { maxtemp = 79; }
}
#else
void save_config() {
	EEPROM_write((EEPLEN - 2), ~config);
}

inline void restore_config() {
	config = ~EEPROM_read((EEPLEN - 2));
}
#endif

inline void ADC_on(uint8_t dpin, uint8_t channel) {
	DIDR0 |= (1 << dpin);                           // disable digital input on ADC pin to reduce power consumption
	//DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
	ADMUX  = (1 << V_REF) | (1 << ADLAR) | channel; // 1.1v reference, left-adjust, ADC1/PB2
	//ADMUX  = (1 << V_REF) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
	// Toss out the garbage first result
	while (ADCSRA & (1 << ADSC));
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
		_delay_10_ms(50);
		if ((byte & (0x01 << x)) == 0 ) {
			set_output(0,15);
		} else {
			set_output(0,120);
		}
		_delay_10_ms(10);
	}
	set_output(0,0);
}
#endif

void blink(uint8_t val, uint8_t speed, uint8_t brightness) {
	for (; val>0; val--)
	{
		set_output(brightness,0);
		_delay_10_ms(speed);
		set_output(0,0);
		_delay_10_ms(speed); _delay_10_ms(speed);
	}
}

inline uint8_t get_voltage() {
	// Get the voltage for later
	ADCSRA |= (1 << ADSC);
	// Wait for completion
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void emergency_shutdown(){
	// Shut down, voltage is too low.
	set_output(0,0);
	// Power down as many components as possible
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
}
#ifdef TEMP_CAL_MODE
uint8_t get_temperature() {
	// Configure the ADC for temperature readings
	ADC_on(ADC_DIDR, TEMP_CHANNEL);
	// average a few values; temperature is noisy
	uint16_t temp = 0;
	uint8_t i;
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
	uint8_t i=2; // This is reused a lot, it's set to 2 for mode group 2 step augmentation
	
	// Read the off-time cap *first* to get the most accurate reading
	// Start up ADC for capacitor pin
	ADC_on(CAP_DIDR, CAP_CHANNEL);	
	uint8_t cap_val = get_voltage(); // save this for later

	// Set PWM pin to output
	DDRB |= (1 << PWM_PIN);	 // enable main channel
	DDRB |= (1 << ALT_PWM_PIN); // enable second channel

	// Set timer to do PWM for correct output pin and set prescaler timing
	TCCR0A = PHASE;
	TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

	// Charge up the capacitor by setting CAP_PIN to output
	DDRB  |= (1 << CAP_PIN);	// Output
	PORTB |= (1 << CAP_PIN);	// High
	ADC_on(ADC_DIDR, ADC_CHANNEL);

	uint8_t voltage = get_voltage();
	if (voltage < ADC_0) {
		// If the battery is getting low, flash twice when turning on or changing brightness
		blink(3, 5, 30);
		if (voltage < ADC_0){
		// Protect the battery if we're just starting and the voltage is too low.
			emergency_shutdown();
		}
	}

	// Read config values and saved state
	restore_config();
	
	// Wipe the config if option 5 (with LOCK MODE undefined)
	// or option 6 (with LOCK MODE defined) is 1
	// or config is empty (fresh flash)
	if ((config & CONFIGMAX) !=0 || config == 0) {
		config = CONFIG_DEFAULT;
		save_config();
	}

	uint8_t mode_idx = restore_mode_idx();
	// First, get the "mode group" (increment value)
	if ((config & MODE_GROUP) == 0) {
		i=1;
	}

	if (cap_val < CAP_MED || (cap_val < CAP_SHORT && ((config & MED_PRESS) == 0))) {
		// Long press, keep the same mode
		// ... or reset to the first mode
		fast_presses = 0;
		if ((config & MEMORY) == 0) {
			// Reset to the first mode
			mode_idx = 0;
		}
#ifdef LOCK_MODE
		locked_in = 0;
	} else if (locked_in != 0 && ((config & LOCK_MODE) != 0)) {
		// Do nothing
#endif
	} else if (cap_val < CAP_SHORT) {
		fast_presses = 0;
		// User did a medium press, go back one mode
		if (mode_idx == MODE_CNT) {
			mode_idx = 0;
		} else if ((mode_idx < NUM_MODES) && (mode_idx > 0)) {
			mode_idx -= i;
			if (mode_idx > MODE_CNT) { mode_idx = 0; };
		} else if (mode_idx >= NUM_MODES) {
			mode_idx += i;
		} else {
			mode_idx = NUM_MODES;
		}
	} else {
		// We don't care what the value is as long as it's over 15
		fast_presses = (fast_presses+1) & 0x1f;

		// Indicates they did a short press, go to the next mode
		mode_idx += i;
		if (mode_idx >= NUM_MODES) {
			mode_idx = 0;
		}
	}

	// Handle mode order reversal
	// If we're reverse biased and not in hidden modes
	if ( ((config & MODE_DIR) == 4) && (mode_idx < NUM_MODES) ) {
		mode_idx = (NUM_MODES - 1 - mode_idx); // subtract 1 since mode_idx starts at 0
	} 
	uint8_t output = modesNx[mode_idx];
	save_mode_idx(mode_idx);

	// Main running loop
	uint8_t ticks = 0;
	uint8_t lowbatt_overheat_cnt = 0;

	while(1) {
#ifdef TEMP_CAL_MODE
		uint8_t temp=get_temperature();
		ADC_on(ADC_DIDR, ADC_CHANNEL);
		voltage = get_voltage();
		if (voltage < ADC_LOW || temp >= maxtemp) {
#else
		voltage = get_voltage();
		if (voltage < ADC_LOW) {
#endif
			lowbatt_overheat_cnt ++;
		} else {
			lowbatt_overheat_cnt = 0;
		}

		// See if it's been low for a while, and maybe step down
		if (lowbatt_overheat_cnt >= 8) {
			// Skip FET and blinky modes if they're in the normal mode rotation
			for (i = mode_idx; i >= NUM_MODES || modesNx[i] > 0; i--){}

			if (i == 0) {
				// If we're already at 0, save state at low and turn off
				set_output(0,0);
				emergency_shutdown();
			}

			// Fet is inactive, we're not in a hidden mode, drop down a level
			i--;
			mode_idx = i;
			save_mode_idx(i);
			set_output(modesNx[i], modes1x[i]);
			lowbatt_overheat_cnt = 0;
		}
		
		if (fast_presses > 0x0f) {  // Config mode
			_delay_s();	   // wait for user to stop fast-pressing button
			fast_presses = 0; // exit this mode after one use
			mode_idx = 0;
#ifdef TEMP_CAL_MODE
			// Wait through all config options to enter temperature calibration mode.
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
				blink(blinks, 12, 30);
				_delay_10_ms(50);
				config ^= i;
				save_config();
				blink(48, 1, 20);
				config ^= i;
				save_config();
				_delay_s();
			}
		}
		switch (output) {
#ifdef SOS
			case SOS:
				blink(3,10,255);
				_delay_10_ms(20);
				blink(3,20,255);
				blink(3,10,255);
				_delay_s();
				break;
#endif
#ifdef TEMP_CAL_MODE
			case TEMP_CAL_MODE:
				blink(5, 12, 30);
				maxtemp = 255;
				save_config();
				_delay_s(); _delay_s();
				set_output(255,0);
				while(1) {
					maxtemp = get_temperature();
					save_config();
					_delay_s();
				}
				break;
#endif
			case STROBE:
			case BIKING_STROBE:
				// 10Hz tactical strobe
				blink(4, 2, 255);
				if (output == BIKING_STROBE) {
					// 2-level stutter beacon for biking and such
					// normal version
					set_output(0,255);
#ifdef LOCK_MODE
					set_lock();
#endif
					_delay_s();
				}
				break;

			case BATTCHECK:
				// figure out how many times to blink
				for (i=0; voltage > voltage_blinks[i]; i ++) {}
				// blink zero to five times to show voltage
				// (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
				blink(i, 12, 30);
				// wait between readouts
				_delay_s();
				break;

			default:
				// Do some magic here to handle turbo step-down
				ticks ++;
				if ((output == TURBO) && (ticks > TURBO_TIMEOUT)) {
					mode_idx = NUM_MODES - 2; // step down to second-highest mode
					save_mode_idx(mode_idx);
				}
				// Regular non-hidden solid mode
				set_output(modesNx[mode_idx], modes1x[mode_idx]);
#ifdef LOCK_MODE
				set_lock();
#endif
				_delay_s();
				break;
		}
		// If we got this far, the user has stopped fast-pressing.
		// So, don't enter config mode.
		fast_presses = 0;
	}
}

