#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>
#include <math.h>

#define malloc(x)		os_malloc(x)
#define free(x)			os_free(x)
#define memcpy(x, y, z)		os_memcpy(x, y, z)
#define memset(x, y, z)		os_memset(x, y, z)
#define memcmp(x, y, z)		os_memcmp(x, y, z)
#define le16toh(x)		(x)
#define le64toh(x)		(x)	/* TODO! */
#define sprintf			os_sprintf

#define plat_time_usec		system_get_time

#define printlog(_l, ...)	os_printf(__VA_ARGS__)
