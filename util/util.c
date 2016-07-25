/* horst - Highly Optimized Radio Scanning Tool
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "platform.h"
#include "util.h"
#include <stdio.h>

#if DO_DEBUG
void dump_hex(const unsigned char* data, int len, const char* txt)
{
	int i;
	if (txt != NULL)
		printf("%s: ", txt);
	for (i=0; i < len; i++) {
		//debug("%02x ", data[i]);
		if ((i % 2) == 0)
			printf(" ");
		if ((i % 16) == 0)
			printf("\n");
		printf("%02x", data[i]);
	}
	printf("\n");
}
#else
void
dump_hex(__attribute__((unused)) const unsigned char* data,
	 __attribute__((unused)) int len,
	 __attribute__((unused)) const char* txt)
{
}
#endif

const char* ether_sprintf(const unsigned char *mac)
{
	static char etherbuf[18];
	sprintf(etherbuf, MAC_FMT, MAC_PAR(mac));
	return etherbuf;
}

/* simple ilog2 implementation */
int ilog2(int x)
{
	int n;
	for (n = 0; !(x & 1); n++)
		x = x >> 1;
	return n;
}
