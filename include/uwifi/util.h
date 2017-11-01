/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_UTIL_H_
#define _UWIFI_UTIL_H_

/* for use in printf-like functions */
#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_PAR(x) x[0], x[1], x[2], x[3], x[4], x[5]

#define MAC_NOT_EMPTY(_mac) (_mac[0] || _mac[1] || _mac[2] || _mac[3] || _mac[4] || _mac[5])

#define MAC_EMPTY(_mac) (!_mac[0] && !_mac[1] && !_mac[2] && !_mac[3] && !_mac[4] && !_mac[5])

#define MAC_BCAST(_mac) (_mac[0] == 0xff && _mac[1] == 0xff && _mac[2] == 0xff \
			 && _mac[3] == 0xff && _mac[4] == 0xff && _mac[5] == 0xff)

#ifndef BIT
#define BIT(nr) (1 << (nr))
#endif

#define TOGGLE_BIT(_x, _m) (_x) ^= (_m)

/**
 * TOGGLE_BITSET() - toggle set of bits as a whole
 * @_x: an integer variable
 * @_s: an integer variable interpreted as a bitset
 * @_t: type (e.g. uint16_t)
 * If any of the bits are set, all bits will be unset. Otherwise, if
 * none of the bits are set, all bits will be set.
 */
#define TOGGLE_BITSET(_x, _s, _type) do {	\
		if ((_x) & (_s))		\
		(_x) &= (_type)~(_s);		\
		else				\
			(_x) |= (_s);		\
		} while(0)

#ifndef MAX
#define MAX(_x, _y) ((_x) > (_y) ? (_x) : (_y))
#endif

#ifndef MIN
#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

void dump_hex(const unsigned char* data, int len, const char* txt);

const char* mac_sprint(const unsigned char *mac);

int ilog2(int x);

static inline __attribute__((const))
int is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

extern const char* UWIFI_VERSION;

#endif
