/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef __linux__
#error "platform is not Linux"
#endif

#include <stdint.h>

uint32_t plat_time_usec(void);	// return monotonic time in usec
