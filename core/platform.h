
#include <stdint.h>

#ifdef __linux__

// directly included:
//#include <stdbool.h>	// bool
//#include <stdint.h>	// int types
//#include <stddef.h>	// size_t

#include <string.h>	// memcpy
#include <stdlib.h>	// malloc, free
#include <math.h>	// pow
#include <stdio.h>	// printf...

void __attribute__ ((format (printf, 1, 2)))
printlog(const char *fmt, ...);

#if DO_DEBUG
#define DEBUG(...) do { if (conf.debug) printf(__VA_ARGS__); } while (0)
#else
#define DEBUG(...)
#endif

///// TODO::::
#ifdef _ALLBSD_SOURCE
#include <machine/endian.h>
#elif __linux__
#include <endian.h>
#endif

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#else
#include <byteswap.h>
#endif


#if BYTE_ORDER == LITTLE_ENDIAN

#if !defined(le64toh)
	#define le64toh(x) (x)
#endif
#if !defined(le32toh)
	#define le32toh(x) (x)
#endif
#if !defined(le16toh)
	#define le16toh(x) (x)
#endif
#if !defined(htole64)
	#define htole64(x) (x)
#endif
#if !defined(htole32)
	#define htole32(x) (x)
#endif
#if !defined(htole16)
	#define htole16(x) (x)
#endif

#else
#if !defined(le64toh)
	#define le64toh(x) bswap_64(x)
#endif
#if !defined(le32toh)
	#define le32toh(x) bswap_32(x)
#endif
#if !defined(le16toh)
	#define le16toh(x) bswap_16(x)
#endif
#if !defined(htole64)
	#define htole64(x) bswap_64(x)
#endif
#if !defined(htole32)
	#define htole32(x) bswap_32(x)
#endif
#if !defined(htole16)
	#define htole16(x) bswap_16(x)
#endif
#endif
///// END OF TODO

#else

// for now ESP8266

#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>

#define malloc(x)		os_malloc(x)
#define free(x)			os_free(x)
#define memcpy(x, y, z)		os_memcpy(x, y, z)
#define memset(x, y, z)		os_memset(x, y, z)
#define memcmp(x, y, z)		os_memcmp(x, y, z)
#define le16toh(x)		(x)
#define le64toh(x)		(x)	/* TODO! */
#define sprintf			os_sprintf

#define plat_time_usec		system_get_time

#define printlog		os_printf

#if DO_DEBUG
#define DEBUG(...) do { if (conf.debug) os_printf(__VA_ARGS__); } while (0)
#else
#define DEBUG(...)
#endif

#endif

uint32_t plat_time_usec(void);	// return monotonic time in usec
