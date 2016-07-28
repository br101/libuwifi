#ifndef _UWIFI_RAW_PARSE_H_
#define _UWIFI_RAW_PARSE_H_

#include <stddef.h>
#include "wlan_parser.h"

int uwifi_parse_raw(unsigned char* buf, size_t len, struct uwifi_packet* p, int arphdr);
void uwifi_fixup_packet_channel(struct uwifi_packet* p, struct uwifi_interface* intf);

#endif
