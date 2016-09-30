/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf <br1@einfach.org>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
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
		//DBG_PRINT("%02x ", data[i]);
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

const char* mac_sprint(const unsigned char *mac)
{
	static char buf[18];
	sprintf(buf, MAC_FMT, MAC_PAR(mac));
	return buf;
}

/* simple ilog2 implementation */
int ilog2(int x)
{
	int n;
	for (n = 0; !(x & 1); n++)
		x = x >> 1;
	return n;
}

/* UWIFI_VER is defined by Makefile from git describe */
#ifndef UWIFI_VER
#define UWIFI_VER "unknown"
#endif

const char* UWIFI_VERSION = UWIFI_VER;
