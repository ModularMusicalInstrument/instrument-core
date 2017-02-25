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
volatile int instr_sel_nxt = 0;
volatile uint8_t note_offset = 0x3c; // C4

ISR(INT0_vect)
{
	if (!bit_is_clear(PINC, PC3))
	{
		// Instrument Select
		if (!bit_is_clear(PIND, PD3))
		{
			instr_sel_nxt = (instr_sel_nxt > 0) ? instr_sel_nxt-1 : 127;
		}
		else
		{
			instr_sel_nxt = (instr_sel_nxt+1) % 128;
		}
		char lcd_line1[17];
		sprintf(lcd_line1, "InstrSel: %d\n", instr_sel_nxt);
		lcd_clrscr();
		lcd_puts(lcd_line1);
		lcd_puts(midi_gm1_instr[instr_sel_nxt]);
	}
	else
	{
		// Note Offset Selector
		if (!bit_is_clear(PIND, PD3))
		{
			note_offset = (note_offset > 0) ? note_offset-1 : 127;
		}
		else
		{
			note_offset = (note_offset+1) % 128;
		}
		char lcd_line1[17];
		sprintf(lcd_line1, "Note Offset: %d\n", note_offset);
		lcd_clrscr();
		lcd_puts(lcd_line1);
	}
	_delay_ms(200);
}

ISR(INT1_vect) {}

// Peripheral Bus Interface (PBI) ISR
void pbi_instr_exec();
void pbi_instr_clear();

volatile int dev_addr_term = 0x08;
volatile uint8_t instr_data[3];
volatile uint8_t instr_data_idx = 0;

void pbi_probe_dev_term()
{
	while (dev_addr_term > 0x08)
	{
		pbi_start();
		pbi_write_addr(dev_addr_term, true);
		uint8_t data = pbi_read_ack();
		pbi_stop();
		
		if (data == dev_addr_term)
		{
			break;
		}
		else
		{
			dev_addr_term -= 1;
		}
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
	else if (instr_data_idx == 3)
	{
		if (instr_data[1] == 0x01)
		{
			// Standard Key Actuation
			uint8_t note = ((instr_data[0]-0x09) + note_offset) % 128;
			if (instr_data[2] != 0x00)
			{
				PORTC |= _BV(2);
				midi_send_noteon(note, instr_data[2]);
			}
			else
			{
				PORTC &= ~(_BV(2));
				midi_send_noteoff(note, instr_data[2]);
			}
		}
	}
}

void pbi_instr_clear()
{
	for (uint8_t i = 0; i < sizeof(instr_data); i++)
	{
		instr_data[i] = 0;
	}
	instr_data_idx = 0;
}

ISR(TWI_vect)
{
	cli();
	
	switch(pbi_get_status())
	{
	case TW_SR_SLA_ACK:
		// START Condition, device addressed
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
		for (int note=0x1E; note < 0x5A; note++) {
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
}
