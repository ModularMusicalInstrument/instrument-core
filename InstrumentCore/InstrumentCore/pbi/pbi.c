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

#include <avr/io.h>
#include <stdbool.h>

#include "pbi.h"

void pbi_init()
{
	TWSR = 0x00;
	TWBR = 0x48; // SCL @ 100 kHz (16 MHz clock)
	
	TWAR = ((0x08<<1) | 0x0); // 0x08 address
	//TWDR = 0x00;

	//TWCR = _BV(TWEN);
	
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
}

void pbi_start()
{
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	//TWCR = (_BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE));
	while ((TWCR & _BV(TWINT)) == 0);
}

void pbi_stop()
{
	TWCR = (_BV(TWINT) | _BV(TWSTO) | _BV(TWEN));
	//TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE);
}

void pbi_write(uint8_t data)
{
	TWDR = data;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while ((TWCR & _BV(TWINT)) == 0);
}

void pbi_write_addr(uint8_t addr, bool rnw)
{
	pbi_write((addr<<1) | (rnw ? 1 : 0));
}

uint8_t pbi_read_ack()
{
	TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
	while ((TWCR & _BV(TWINT)) == 0);
	return TWDR;
}

uint8_t pbi_read_nack()
{
	TWCR = _BV(TWINT) | _BV(TWEN);
	while ((TWCR & _BV(TWINT)) == 0);
	return TWDR;
}

uint8_t pbi_get_status()
{
	return TWSR & 0xf8;
}
