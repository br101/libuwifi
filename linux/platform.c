/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <time.h>

#include "platform.h"

// return monotonic time in microseconds
uint32_t plat_time_usec(void) {
	struct timespec time_mono;
	clock_gettime(CLOCK_MONOTONIC, &time_mono);
	return time_mono.tv_sec * 1000000 + time_mono.tv_nsec / 1000;
}
