
#include <stdint.h>

#ifdef __linux__

// directly included:
//#include <stdbool.h>	// bool
//#include <stdint.h>	// int types
//#include <stddef.h>	// size_t

#include <string.h>	// memcpy
#include <stdlib.h>	// malloc, free
#include <math.h>	// pow

void __attribute__ ((format (printf, 1, 2)))
printlog(const char *fmt, ...);

#if DO_DEBUG
#define DEBUG(...) do { if (conf.debug) printf(__VA_ARGS__); } while (0)
#else
#define DEBUG(...)
#endif

#else

#endif

uint32_t plat_time_usec(void);	// return monotonic time in usec
