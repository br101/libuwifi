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

uint32_t plat_time_usec(void);	// return monotonic time in usec
