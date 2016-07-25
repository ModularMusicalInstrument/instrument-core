/*
 * midi.c
 *
 * Created: 2016-07-21 10:05:59 AM
 *  Author: vwfchung
 */ 

#define F_CPU 8000000UL
#define BAUD 31250

#include <avr/io.h>
#include <util/setbaud.h>

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

void midi_send_noteon(uint8_t note, uint8_t velocity)
{
	_midi_send_byte((0x9 << 4) | default_channel);
	_midi_send_byte(note);
	_midi_send_byte(velocity);
}

void midi_send_noteoff(uint8_t note, uint8_t velocity)
{
	_midi_send_byte((0x8 << 4) | default_channel);
	_midi_send_byte(note);
	_midi_send_byte(velocity);
}

void midi_send_programchange(uint8_t program)
{
	uint8_t new_program = (program >= 1 && program <= 128) ? program - 1 : 0;
	_midi_send_byte((0xC << 4) | default_channel);
	_midi_send_byte(new_program);
}


void _midi_send_byte(uint8_t value)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = value;
}
