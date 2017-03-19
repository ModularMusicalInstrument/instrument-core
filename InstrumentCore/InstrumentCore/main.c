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
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <util/twi.h>
#include <stdio.h>
#include <stdbool.h>

#include "midi/midi.h"
#include "midi/midi_instrs.h"
#include "lcd/lcd.h"
#include "pbi/pbi.h"


// Rotary Encoder ISR
volatile uint8_t instr_sel_nxt = 0;
volatile uint8_t note_transpose = 0x3c; // C4

ISR(INT0_vect)
{
	if (!bit_is_clear(PINC, PC3))
	{
		// Instrument Select
		if (!bit_is_clear(PIND, PD3))
		{
			instr_sel_nxt = (instr_sel_nxt+1) % 128;
		}
		else
		{
			instr_sel_nxt = (instr_sel_nxt > 0) ? instr_sel_nxt-1 : 127;
		}
		char lcd_line1[17];
		sprintf(lcd_line1, "InstrmntSel: %d\n", instr_sel_nxt);
		lcd_clrscr();
		lcd_puts(lcd_line1);
		lcd_puts(midi_gm1_instr[instr_sel_nxt]);
	}
	else
	{
		// Note Offset Selector
		if (!bit_is_clear(PIND, PD3))
		{
			note_transpose = (note_transpose+1) % 128;
		}
		else
		{
			note_transpose = (note_transpose > 0) ? note_transpose-1 : 127;			
		}
		char lcd_line1[17];
		sprintf(lcd_line1, "Transpose:   %d\n", note_transpose);
		lcd_clrscr();
		lcd_puts(lcd_line1);
		lcd_puts("(Note C4=60)");
	}
	_delay_ms(200);
}

ISR(INT1_vect) {}

// Peripheral Bus Interface (PBI) ISR
void pbi_instr_exec();
void pbi_instr_clear();

volatile uint8_t dev_addr_term = 0x08;
volatile uint8_t instr_data[2];
volatile uint8_t instr_data_idx = 0;

void pbi_probe_dev_term()
{
	while (dev_addr_term > 0x08)
	{
		pbi_start();
		pbi_write_addr(dev_addr_term, false);
		bool ping_ack = (pbi_get_status() == 0x18);
		pbi_stop();
		
		if (ping_ack) return;
		
		dev_addr_term -= 1;
	}
}

void pbi_instr_exec()
{
	if (instr_data_idx == 1)
	{
		if (instr_data[0] == 0x01)
		{
			// Request to be assigned an address
			// (i) Probe last connected device
			pbi_probe_dev_term();
			// (ii) Assign next address
			if (dev_addr_term < 0x77)
			{
				pbi_start();
				pbi_write_addr(0x00, false);
				pbi_write(dev_addr_term+1);
				pbi_stop();
				dev_addr_term += 1;
			}
		}
	}
	else if (instr_data_idx == 2)
	{
		// Standard Key Actuation
		uint8_t note = ((instr_data[0]-0x09) + note_transpose) % 128;
		if (instr_data[1] != 0x00)
		{
			PORTC |= _BV(2);
			midi_send_noteon(note, instr_data[1]);
		}
		else
		{
			PORTC &= ~(_BV(2));
			midi_send_noteoff(note, 0x00);
		}
	}
}

void pbi_instr_clear()
{
	instr_data_idx = 0;
}

ISR(TWI_vect)
{
	cli();
	
	switch(pbi_get_status())
	{
	case TW_SR_SLA_ACK:
		// START Condition, device addressed
		pbi_instr_clear();
		TWCR |= (_BV(TWIE) | _BV(TWINT) | _BV(TWEA) | _BV(TWEN));
		break;
	case TW_SR_DATA_ACK:
		// Device in receiver mode, data available
		if (instr_data_idx < sizeof(instr_data))
		{
			instr_data[instr_data_idx] = TWDR;
			instr_data_idx += 1;
		}
		TWCR |= (_BV(TWIE) | _BV(TWINT) | _BV(TWEA) | _BV(TWEN));		
		break;
	case TW_ST_DATA_ACK:
		// Device in transmitter mode, send data
		// ...		
		TWCR |= (_BV(TWIE) | _BV(TWINT) | _BV(TWEA) | _BV(TWEN));		
		break;
	default:
		// STOP Condition
		pbi_instr_exec();
		pbi_instr_clear();		
		TWCR |= (_BV(TWIE) | _BV(TWEA) | _BV(TWEN));
	}

	sei();
}

int main(void)
{
	// Enable port for RGB LED
	DDRC |= _BV(2);
	
	// Enable ports for Rotary Encoder
	DDRD &= ~(_BV(3) | _BV(2));
	PORTD |= (_BV(3) | _BV(2));
	EIMSK |= (_BV(INT0) | _BV(INT1));
	MCUCR |= (_BV(ISC01) | _BV(ISC11) | _BV(ISC10));	
	DDRC &= ~(_BV(3));
	PORTC &= ~(_BV(3));
	
	// Initialize MIDI
	midi_init();
	midi_set_channel(1);
	
	// Set the MIDI instrument
	int instr_sel = 0; // todo: Read saved value from EEPROM
	midi_send_programchange(instr_sel);
	
	// Initialize Peripheral Bus
	pbi_init();
	
	// Initialize Character LCD
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	lcd_puts("ModulrMusicInst\n");
	lcd_puts(midi_gm1_instr[instr_sel_nxt]);
	
	// Enable Interrupts
	sei();	
	
	// Main Program
	while (1) {
		// Process instrument change (if any)
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			if (instr_sel_nxt != instr_sel)
			{
				instr_sel = instr_sel_nxt;
				midi_send_programchange(instr_sel);
			}
		}
	}
}
