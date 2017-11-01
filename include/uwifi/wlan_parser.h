/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_PARSE_WLAN_H_
#define _UWIFI_PARSE_WLAN_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "wlan80211.h"
#include "channel.h"
#include "util.h"

#define PHY_FLAG_SHORTPRE	BIT(0)
#define PHY_FLAG_BADFCS		BIT(1)
#define PHY_FLAG_A		BIT(2)
#define PHY_FLAG_B		BIT(3)
#define PHY_FLAG_G		BIT(4)
#define PHY_FLAG_MODE_MASK	0x1C

#define WLAN_MODE_AP		BIT(0)
#define WLAN_MODE_IBSS		BIT(1)
#define WLAN_MODE_STA		BIT(2)
#define WLAN_MODE_PROBE		BIT(3)
#define WLAN_MODE_4ADDR		BIT(4)
#define WLAN_MODE_UNKNOWN	BIT(5)

#define WLAN_MODE_ALL		(WLAN_MODE_AP | WLAN_MODE_IBSS | WLAN_MODE_STA | WLAN_MODE_PROBE | WLAN_MODE_4ADDR | WLAN_MODE_UNKNOWN)

struct uwifi_packet {
	/* general */
	unsigned int		pkt_types;	/* bitmask of packet types */

	/* wlan phy (from radiotap) */
	int			phy_signal;	/* signal strength (usually dBm) */		// X
	unsigned int		phy_rate;	/* physical rate * 10 (=in 100kbps) */
	unsigned char		phy_rate_idx;	/* MCS index */
	unsigned char		phy_rate_flags;	/* MCS flags */
	unsigned int		phy_freq;	/* frequency from driver */
	unsigned int		phy_flags;	/* A, B, G, shortpre */
	bool			phy_injected;	/* frame was injected by ourselves */

	/* wlan mac */
	unsigned int		wlan_len;	/* packet length */
	uint16_t		wlan_type;	/* frame control field */			// X
	unsigned char		wlan_src[WLAN_MAC_LEN]; /* transmitter (TA) */			// X
	unsigned char		wlan_dst[WLAN_MAC_LEN]; /* receiver (RA) */
	unsigned char		wlan_bssid[WLAN_MAC_LEN];						// X?
	char			wlan_essid[WLAN_MAX_SSID_LEN];
	uint64_t		wlan_tsf;	/* timestamp from beacon */
	unsigned int		wlan_bintval;	/* beacon interval */
	unsigned int		wlan_mode;	/* AP, STA or IBSS */				// X
	unsigned char		wlan_channel;	/* channel from beacon, probe */		// X
	enum uwifi_chan_width	wlan_chan_width;
	unsigned char		wlan_tx_streams;
	unsigned char		wlan_rx_streams;
	unsigned char		wlan_qos_class;	/* for QDATA frames */
	unsigned int		wlan_nav;	/* frame NAV duration */
	unsigned int		wlan_seqno;	/* sequence number */

	/* flags */
	unsigned int		wlan_wep:1,	/* WEP on/off */
				wlan_retry:1,
				wlan_wpa:1,
				wlan_rsn:1,
				wlan_ht40plus:1;

	/* batman-adv */
	unsigned char		bat_version;
	unsigned char		bat_packet_type;
	unsigned char		bat_gw:1;

	/* IP */
	unsigned int		ip_src;
	unsigned int		ip_dst;
	unsigned int		tcpudp_port;
	unsigned int		olsr_type;
	unsigned int		olsr_neigh;
	unsigned int		olsr_tc;

	/* calculated from other values */
	unsigned int		pkt_duration;	/* packet "airtime" */
	int			pkt_chan_idx;	/* received while on channel */
	int			wlan_retries;	/* retry count for this frame */
};

int uwifi_parse_80211_header(unsigned char* buf, size_t len, struct uwifi_packet* p);
void uwifi_parse_information_elements(unsigned char* buf, size_t bufLen, struct uwifi_packet *p);

#endif
