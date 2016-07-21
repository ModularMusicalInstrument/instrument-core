/*
 * InstrumentCore.c
 *
 * Created: 2016-07-21 9:54:35 AM
 * Author : vwfchung
 */ 

#define F_CPU 8000000UL
#define BAUD 31250

#include <avr/io.h>
#include <util/setbaud.h>
#include <util/delay.h>

#include "midi.h"

int main(void)
{
	midi_init();
	midi_set_channel(1);
	
	midi_send_programchange(80);
	
	while (1) {
		for (int note=0x1E; note < 0x5A; note++) {
			midi_send_noteon(note, 0x45);
			_delay_ms(250);
			midi_send_noteoff(note, 0x00);
			_delay_ms(250);
		}
	}
}


