#ifndef _UWIFI_RAW_PARSE_H_
#define _UWIFI_RAW_PARSE_H_

#include <stddef.h>
#include "wlan_parser.h"

/* return rest of packet length (may be 0) or negative value on error */
int uwifi_parse_raw(unsigned char* buf, size_t len, struct uwifi_packet* p, int arphdr);

/* return consumed length, 0 for bad FCS, -1 on error */
int uwifi_parse_radiotap(unsigned char* buf, size_t len, struct uwifi_packet* p);

/* return consumed length or -1 on error */
int uwifi_parse_prism_header(unsigned char* buf, int len, struct uwifi_packet* p);

void uwifi_fixup_packet_channel(struct uwifi_packet* p, struct uwifi_interface* intf);

#endif
