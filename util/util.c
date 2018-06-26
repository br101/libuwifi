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
#include <string.h>

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

const char* mac_sprint(const unsigned char *mac)
{
	static char buf[18];
	sprintf(buf, MAC_FMT, MAC_PAR(mac));
	return buf;
}

bool string_to_mac(const char* str, unsigned char* mac)
{
	if (str == NULL || mac == NULL)
		return false;

	int n = sscanf(str, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
			mac, mac+1, mac+2, mac+3, mac+4, mac+5);
	return (n == 6);
}

/* simple ilog2 implementation */
int ilog2(int x)
{
	int n;
	for (n = 0; !(x & 1); n++)
		x = x >> 1;
	return n;
}

/* VERSION is defined by Makefile from git describe */
#ifndef VERSION
#define VERSION "unknown"
#endif

const char* UWIFI_VERSION = VERSION;
