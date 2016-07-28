#ifndef _UWIFI_NETDEV_H_
#define _UWIFI_NETDEV_H_

#include <stdbool.h>

#ifndef ARPHRD_IEEE80211_RADIOTAP
#define ARPHRD_IEEE80211_RADIOTAP 803    /* IEEE 802.11 + radiotap header */
#endif

#ifndef ARPHRD_IEEE80211_PRISM
#define ARPHRD_IEEE80211_PRISM 802      /* IEEE 802.11 + Prism2 header  */
#endif

/** Get the hardware type of the given interface as ARPHRD_xxx constant */
int netdev_get_hwinfo(char* ifname);

bool netdev_get_mac_address(const char* ifname, unsigned char* mac);

bool netdev_set_up_promisc(const char *const ifname, bool up, bool promisc);

#endif
