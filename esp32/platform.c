#include <sys/time.h>

#include "platform.h"
#include "uwifi/log.h"

uint32_t plat_time_usec(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

void __attribute__ ((format (printf, 2, 3)))
log_out(enum loglevel ll, const char *fmt, ...)
{
	;//ESP_LOGI("IYD", fmt, NULL);
}
