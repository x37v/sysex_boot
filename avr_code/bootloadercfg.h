/*
 * A sysex bootloader for avr chips.
 * Copyright 2010 Alex Norman
 *
 * This file is part of SysexBoot.
 *
 * SysexBoot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SysexBoot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SysexBoot.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BOOTLOADERCFG_H
#define BOOTLOADERCFG_H

/*

//turn on port b bit zero as input
//turn on pullups
#define BOOTLOADER_INIT \
	DDRB = 0xFE; \
	PORTB = 0x01;

//a zero tells us that the button is down
#define BOOTLOADER_CONDITION (!(PINB & _BV(PINB0)))
*/

//turn on port b bit zero as input
//turn on pullups
#ifndef BOOTLOADER_INIT
#define BOOTLOADER_INIT \
	DDRB = 0xFF; \
	PORTB = 0xFF; \
	DDRD &= 0xF0; \
	PORTD |= 0xF0;
#endif

//a zero tells us that the button is down
#ifndef BOOTLOADER_CONDITION
#define BOOTLOADER_CONDITION (!(PIND & _BV(PIND4)))
#endif

#ifndef INDICATE_BOOTLOADER_MODE
#define INDICATE_BOOTLOADER_MODE \
	PORTB = 170;
#endif

//needs to be at least 
//2 [sysex start/end] 
//+ 9 [sysex headers] 
//+ 2 [write address]
//+ max size of data that you will be writing at a time
#ifndef MIDIIN_BUF_SIZE
#define MIDIIN_BUF_SIZE 192
#endif

#endif
