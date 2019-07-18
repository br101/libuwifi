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
#include <endian.h>

// directly included:
//#include <stdbool.h>	// bool
//#include <stdint.h>	// int types
//#include <stddef.h>	// size_t

#include <string.h>	// memcpy
#include <stdlib.h>	// malloc, free
#include <math.h>	// pow
#include <stdio.h>	// printf...

#ifdef __cplusplus
extern "C" {
#endif

uint32_t plat_time_usec(void);	// return monotonic time in usec

#ifdef __cplusplus
}
#endif
