// LINUX

#include <string.h>
#include <sys/time.h>

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
