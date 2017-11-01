/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_NODE_H_
#define _UWIFI_NODE_H_

#include "platform.h"
#include "wlan_parser.h"
#include "ccan/list/list.h"
#include "average.h"
#include "conf.h"
#include "essid.h"
#include "wlan_util.h"

struct uwifi_node {
	/* housekeeping */
	struct list_node	list;								// X
	struct list_node	essid_nodes;
	struct list_head	on_channels;	/* channels this node was seen on */
	struct list_head	ap_nodes;	/* stations associated to AP */
	struct list_node	ap_list;
	struct uwifi_node*	ap_node;
	unsigned int		num_on_channels;
	uint32_t		last_seen;	/* timestamp */					// X uint32

	/* general packet info */
	unsigned int		pkt_types;	/* bitmask of packet types we've seen */
	unsigned int		pkt_count;	/* nr of packets seen */
	unsigned int		rx_pkt_count;   /* nr of packets seen */
	int			rx_only;

	/* wlan phy (from radiotap) */
	unsigned int		phy_rate_last;
	int			phy_sig_last;
	int			phy_sig_max;
	struct ewma		phy_sig_avg;
	unsigned long		phy_sig_sum;							// X
	int			phy_sig_count;							// X

	/* wlan mac */
	unsigned char		wlan_src[WLAN_MAC_LEN];	/* Sender MAC address (ID) */		// X
	unsigned char		wlan_bssid[WLAN_MAC_LEN];
	unsigned int		wlan_channel;	/* channel from beacon, probe frames */		// X
	unsigned int		wlan_mode;	/* AP, STA or IBSS */				// X
	uint64_t		wlan_tsf;
	unsigned int		wlan_bintval;
	unsigned int		wlan_retries_all;
	unsigned int		wlan_retries_last;
	unsigned int		wlan_seqno;
	struct essid_info*	essid;
	enum uwifi_chan_width	wlan_chan_width;
	unsigned char		wlan_tx_streams;
	unsigned char		wlan_rx_streams;
	enum uwifi_80211_std	wlan_std;

	unsigned int		wlan_wep:1,	/* WEP active? */
				wlan_wpa:1,
				wlan_rsn:1,
				wlan_ht40plus:1;


	/* batman */
	unsigned char		bat_gw:1;

	/* IP */
	unsigned int		ip_src;		/* IP address (if known) */
	unsigned int		olsr_count;	/* number of OLSR packets */
	unsigned int		olsr_neigh;	/* number if OLSR neighbours */
	unsigned int		olsr_tc;	/* unused */
};

struct uwifi_node* uwifi_node_update(struct uwifi_packet* p,
				     struct list_head* nodes);
struct uwifi_node* uwifi_node_update_receiver(struct uwifi_packet* p,
					      struct list_head* nodes);
void uwifi_nodes_find_ap(struct uwifi_node* n, struct list_head* nodes);
void uwifi_nodes_timeout(struct list_head* nodes, unsigned int timeout_sec,
			 uint32_t* last_nodetimeout);
void uwifi_nodes_free(struct list_head* nodes);

#endif
