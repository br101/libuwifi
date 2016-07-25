#ifndef _UWIFI_APP_H_
#define _UWIFI_APP_H_

/* These are things the application needs to provide */

void __attribute__ ((format (printf, 2, 3)))
printlog(int level, const char *fmt, ...);

#endif
