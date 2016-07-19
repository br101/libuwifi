#ifdef _ALLBSD_SOURCE
#include <machine/endian.h>

#if defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#endif

#endif
