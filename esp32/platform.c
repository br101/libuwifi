#include <sys/time.h>

#include "platform.h"

uint32_t plat_time_usec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}
