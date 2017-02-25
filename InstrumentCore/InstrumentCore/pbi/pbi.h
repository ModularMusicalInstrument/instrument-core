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


#ifndef PBI_H
#define PBI_H

void pbi_init();

void pbi_start();

void pbi_stop();

void pbi_write(uint8_t data);

void pbi_write_addr(uint8_t addr, bool rnw);

uint8_t pbi_read_ack();

uint8_t pbi_read_nack();

uint8_t pbi_get_status();

#endif // PBI_H
