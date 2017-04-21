/*
 * Library: libcrc
 * File:    src/crcsick.c
 * Author:  Lammert Bies
 *
 * This file is licensed under the MIT License as stated below
 *
 * Copyright (c) 2007-2016 Lammert Bies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description
 * -----------
 * The source file src/crcsick.c contains routines that help in calculating the
 * CRC value that is used as dataprotection measure in communications with Sick
 * electronic devices.
 */
#include "sickd.h"

#define		CRC_POLY_SICK		0x8005
#define		CRC_START_SICK		0x0000


/*
 * Calculates the SICK CRC value of an input string in one pass.
 */
uint16_t crc_sick(const uint8_t *input_str, size_t num_bytes)
{

	uint16_t crc;
	uint16_t short_c;
	uint16_t short_p;
	const uint8_t *ptr;
	size_t a;

	crc = CRC_START_SICK;
	ptr = input_str;
	short_p = 0;

	if (!ptr)
		return crc;

	for (a = 0; a < num_bytes; a++) {

		short_c = 0x00ff & (uint16_t) *ptr;

		if ( crc & 0x8000 )
			crc = ( crc << 1 ) ^ CRC_POLY_SICK;
		else
			crc =   crc << 1;

		crc    ^= ( short_c | short_p );
		short_p = short_c << 8;

		ptr++;
	}

	return crc;
}
