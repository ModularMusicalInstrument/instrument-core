/**
 * Instrument Core software for Modular Musical Instrument
 *
 * Copyright (C) 2016-2017 ECE 2017.011, University of Waterloo
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
 */

#define F_CPU 16000000UL
#define BAUD 31250

#include <avr/io.h>
#include <util/setbaud.h>

#include "midi.h"

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static uint8_t default_channel = 0;

void midi_init()
{
	UCSR0B = _BV(TXEN0); // Enable Tx
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 8-bit Messages
	
	// USART Baud Rate Register
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= _BV(U2X0);
	#else
	UCSR0A &= ~(_BV(U2X0));
	#endif
}

void midi_set_channel(uint8_t channel)
{
	default_channel = (channel >= 1 && channel <= 16) ? channel - 1 : 0;
}

void midi_send_programchange(uint8_t program)
{
	_midi_send_byte((0xc << 4) | default_channel);
	_midi_send_byte(CLAMP(program, 0, 127));
}

void midi_send_noteon(uint8_t note, uint8_t velocity)
{
	_midi_send_byte((0x9 << 4) | default_channel);
	_midi_send_byte(CLAMP(note, 0, 127));
	_midi_send_byte(CLAMP(velocity, 0, 127));
}

void midi_send_noteoff(uint8_t note, uint8_t velocity)
{
	_midi_send_byte((0x8 << 4) | default_channel);
	_midi_send_byte(CLAMP(note, 0, 127));
	_midi_send_byte(CLAMP(velocity, 0, 127));
}

void _midi_send_byte(uint8_t value)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = value;
}
