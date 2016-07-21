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

void init_midi(void)
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

void send_byte(uint8_t value)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = value;
}

int main(void)
{
	init_midi();
	
	while (1)
	{
		send_byte(0x90);
		send_byte(0x3A);
		send_byte(0x45);
		_delay_ms(250);
	}
}


