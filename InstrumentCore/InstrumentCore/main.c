/*
 * InstrumentCore.c
 *
 * Created: 2016-07-21 9:54:35 AM
 * Author : vwfchung
 */ 

#define F_CPU 8000000UL
#define BAUD 31250

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <stdbool.h>

#include "midi.h"

const uint8_t instruments[6] = {1, 12, 14, 57, 80, 115};
volatile bool instrument_change = false;

ISR(PCINT1_vect) {
	if (PINC & _BV(PC1)) instrument_change = true;
	_delay_ms(200); // Debouncing
}

int main(void)
{
	// Initialize Ports and ISRs
	PCICR |= _BV(PCIE1);
	PCMSK1 |= _BV(PCINT9);
	sei();
	
	// Initialize MIDI
	midi_init();
	midi_set_channel(1);
	
	// Main Program
	int curr_instrument = 0;
	midi_send_programchange(instruments[curr_instrument]);
	bool instrument_change_local = false;
	while (1) {
		for (int note=0x1E; note < 0x5A; note++) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				instrument_change_local = instrument_change;
				instrument_change = false;
			}
			if (instrument_change_local) {
				curr_instrument = (curr_instrument + 1) % 6;
				midi_send_programchange(instruments[curr_instrument]);
				instrument_change_local = false;
			}
			midi_send_noteon(note, 0x45);
			_delay_ms(100);
			midi_send_noteoff(note, 0x00);
			_delay_ms(100);
		}
	}
}


