#ifndef _RAW_PARSE_H_
#define _RAW_PARSE_H_

#include <stddef.h>
#include "wlan_parser.h"

#ifndef ARPHRD_IEEE80211_RADIOTAP
#define ARPHRD_IEEE80211_RADIOTAP 803    /* IEEE 802.11 + radiotap header */
#endif

#ifndef ARPHRD_IEEE80211_PRISM
#define ARPHRD_IEEE80211_PRISM 802      /* IEEE 802.11 + Prism2 header  */
#endif

int wlan_parse_packet(unsigned char* buf, size_t len, struct packet_info* p, int arphdr);

#endif
