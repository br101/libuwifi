// LINUX

// directly included:
//#include <stdbool.h>	// bool
//#include <stdint.h>	// int types
//#include <stddef.h>	// size_t

#include <string.h>	// memcpy
#include <time.h>	// time_t
//#include <sys/time.h>
#include <stdlib.h>	// malloc, free
#include <math.h>	// pow

#if 0
#define memcpy
#endif

void __attribute__ ((format (printf, 1, 2)))
printlog(const char *fmt, ...);

#if DO_DEBUG
#define DEBUG(...) do { if (conf.debug) printf(__VA_ARGS__); } while (0)
#else
#define DEBUG(...)
#endif

extern struct timespec the_time;
