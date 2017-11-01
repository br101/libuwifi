/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <stdlib.h>

#include "platform.h"
#include "util.h"
#include "wlan80211.h"
#include "node.h"
#include "essid.h"
#include "log.h"

static void copy_nodeinfo(struct uwifi_node* n, struct uwifi_packet* p)
{
	memcpy(n->wlan_src, p->wlan_src, WLAN_MAC_LEN);
	n->rx_only = false;

	if (MAC_NOT_EMPTY(p->wlan_bssid))
		memcpy(n->wlan_bssid, p->wlan_bssid, WLAN_MAC_LEN);

	n->last_seen = plat_time_usec();
	n->pkt_count++;
	n->pkt_types |= p->pkt_types;
	if (p->ip_src)
		n->ip_src = p->ip_src;
	if (p->wlan_mode)
		n->wlan_mode |= p->wlan_mode;
	if (p->olsr_tc)
		n->olsr_tc = p->olsr_tc;
	if (p->olsr_neigh)
		n->olsr_neigh = p->olsr_neigh;
//	if (p->pkt_types & PKT_TYPE_OLSR)
//		n->olsr_count++;
	if (p->bat_gw)
		n->bat_gw = 1;
	if (p->wlan_ht40plus)
		n->wlan_ht40plus = 1;
	if (p->wlan_tx_streams)
		n->wlan_tx_streams = p->wlan_tx_streams;
	if (p->wlan_rx_streams)
		n->wlan_rx_streams = p->wlan_rx_streams;

	if ((p->wlan_type == WLAN_FRAME_BEACON) ||
	    (p->wlan_type == WLAN_FRAME_PROBE_RESP)) {
		n->wlan_tsf = p->wlan_tsf;
		n->wlan_bintval = p->wlan_bintval;
		n->wlan_wpa = p->wlan_wpa;
		n->wlan_rsn = p->wlan_rsn;
		// Channel is only really known for Beacon and Probe response
		n->wlan_channel = p->wlan_channel;
	} else if ((n->wlan_mode & WLAN_MODE_STA) && n->ap_node) {
		// for STA we can use the channel from the AP
		n->wlan_channel = n->ap_node->wlan_channel;
	} else if (n->wlan_channel == 0 && p->wlan_channel != 0) {
		// otherwise only override if channel was unknown
		n->wlan_channel = p->wlan_channel;
	}

	n->phy_rate_last = p->phy_rate;
	n->phy_sig_last = p->phy_signal;
	ewma_add(&n->phy_sig_avg, -p->phy_signal);
	n->phy_sig_sum += -p->phy_signal;
	n->phy_sig_count += 1;

	if (p->phy_signal > n->phy_sig_max || n->phy_sig_max == 0)
		n->phy_sig_max = p->phy_signal;

	if ((p->wlan_type == WLAN_FRAME_DATA) ||
	    (p->wlan_type == WLAN_FRAME_QDATA) ||
	    (p->wlan_type == WLAN_FRAME_AUTH) ||
	    (p->wlan_type == WLAN_FRAME_BEACON) ||
	    (p->wlan_type == WLAN_FRAME_PROBE_RESP) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_ACK) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_POLL) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_ACKPOLL) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_ACK) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_POLL) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_ACKPOLL))
		n->wlan_wep = p->wlan_wep;

	if (p->wlan_seqno != 0) {
		if (p->wlan_retry && p->wlan_seqno == n->wlan_seqno) {
			n->wlan_retries_all++;
			n->wlan_retries_last++;
		} else
			n->wlan_retries_last = 0;
		n->wlan_seqno = p->wlan_seqno;
	}

	if (p->wlan_chan_width > n->wlan_chan_width)
		n->wlan_chan_width = p->wlan_chan_width;

	/* guess IEEE802.11 Standard from channel width, packet type and rate */
	enum uwifi_80211_std chstd = wlan_80211std_from_chan(p->wlan_chan_width, p->wlan_channel);
	enum uwifi_80211_std rstd = wlan_80211std_from_rate(p->phy_rate_idx, p->wlan_channel);
	enum uwifi_80211_std ptstd = wlan_80211std_from_type(p->wlan_type);
	enum uwifi_80211_std mstd = MAX(chstd, rstd);
	mstd = MAX(mstd, ptstd);
	n->wlan_std = MAX(n->wlan_std, mstd);

	/* set packet retries from node sum */
	p->wlan_retries = n->wlan_retries_last;
}

struct uwifi_node* uwifi_node_update(struct uwifi_packet* p, struct list_head* nodes)
{
	struct uwifi_node* n;

	if (p->phy_flags & PHY_FLAG_BADFCS)
		return NULL;

	if (p->wlan_src[0] == 0 && p->wlan_src[1] == 0 &&
	    p->wlan_src[2] == 0 && p->wlan_src[3] == 0 &&
	    p->wlan_src[4] == 0 && p->wlan_src[5] == 0)
		return NULL;

	/* find node by wlan source address */
	list_for_each(nodes, n, list) {
		if (memcmp(p->wlan_src, n->wlan_src, WLAN_MAC_LEN) == 0) {
			LOG_DBG("NODE found %p " MAC_FMT, n, MAC_PAR(p->wlan_src));
			break;
		}
	}

	/* not found */
	if (&n->list == &nodes->n) {
		n = (struct uwifi_node*)malloc(sizeof(struct uwifi_node));
		memset(n, 0, sizeof(struct uwifi_node));
		ewma_init(&n->phy_sig_avg, 1024, 8);
		list_head_init(&n->on_channels);
		list_head_init(&n->ap_nodes);
		list_add_tail(nodes, &n->list);
		LOG_DBG("NODE adding %p " MAC_FMT, n, MAC_PAR(p->wlan_src));
	}

	copy_nodeinfo(n, p);
	return n;
}

static void copy_rx_nodeinfo(struct uwifi_node* n, struct uwifi_packet* p)
{
	memcpy(n->wlan_src, p->wlan_dst, WLAN_MAC_LEN);

	if (MAC_NOT_EMPTY(p->wlan_bssid))
		memcpy(n->wlan_bssid, p->wlan_bssid, WLAN_MAC_LEN);

	n->last_seen = plat_time_usec();
	n->rx_pkt_count++;
	n->pkt_types |= p->pkt_types;

	/* if packet sender was AP we know recipient is STA and vice versa */
	if (p->wlan_mode == WLAN_MODE_AP)
		n->wlan_mode = WLAN_MODE_STA;
	else if (p->wlan_mode == WLAN_MODE_STA)
		n->wlan_mode = WLAN_MODE_AP;
	else if (p->wlan_mode == WLAN_MODE_IBSS)
		n->wlan_mode = WLAN_MODE_IBSS;

	if ((n->wlan_mode & WLAN_MODE_STA) && n->ap_node) {
		// for STA we can use the channel from the AP
		n->wlan_channel = n->ap_node->wlan_channel;
	} else if (n->wlan_channel == 0 && p->wlan_channel != 0) {
		// otherwise only override if channel was unknown
		n->wlan_channel = p->wlan_channel;
	}

	if ((p->wlan_type == WLAN_FRAME_DATA) ||
	    (p->wlan_type == WLAN_FRAME_QDATA) ||
	    (p->wlan_type == WLAN_FRAME_AUTH) ||
	    (p->wlan_type == WLAN_FRAME_BEACON) ||
	    (p->wlan_type == WLAN_FRAME_PROBE_RESP) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_ACK) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_POLL) ||
	    (p->wlan_type == WLAN_FRAME_DATA_CF_ACKPOLL) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_ACK) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_POLL) ||
	    (p->wlan_type == WLAN_FRAME_QDATA_CF_ACKPOLL))
		n->wlan_wep = p->wlan_wep;
}

struct uwifi_node* uwifi_node_update_receiver(struct uwifi_packet* p, struct list_head* nodes)
{
	struct uwifi_node* n;

	if (p->phy_flags & PHY_FLAG_BADFCS)
		return NULL;

	if (MAC_EMPTY(p->wlan_dst) || MAC_BCAST(p->wlan_dst))
		return NULL;

	/* find node by wlan source address */
	list_for_each(nodes, n, list) {
		if (memcmp(p->wlan_dst, n->wlan_src, WLAN_MAC_LEN) == 0) {
			LOG_DBG("RX NODE found %p " MAC_FMT, n, MAC_PAR(p->wlan_dst));
			break;
		}
	}

	/* not found */
	if (&n->list == &nodes->n) {
		n = (struct uwifi_node*)malloc(sizeof(struct uwifi_node));
		memset(n, 0, sizeof(struct uwifi_node));
		ewma_init(&n->phy_sig_avg, 1024, 8);
		list_head_init(&n->on_channels);
		list_head_init(&n->ap_nodes);
		list_add_tail(nodes, &n->list);
		LOG_DBG("RX NODE adding %p " MAC_FMT, n, MAC_PAR(p->wlan_dst));
		n->rx_only = true;
	}

	copy_rx_nodeinfo(n, p);
	return n;
}

void uwifi_nodes_find_ap(struct uwifi_node* n, struct list_head* nodes)
{
	struct uwifi_node* ap;

	/* in station mode, when BSSID is valid and different than current AP */
	if (n->wlan_mode & WLAN_MODE_STA &&
	    n->wlan_bssid[0] != 0xff &&
	    MAC_NOT_EMPTY(n->wlan_bssid) &&
	    (n->ap_node == NULL ||
	     memcmp(n->wlan_bssid, n->ap_node->wlan_src, WLAN_MAC_LEN) != 0)) {
		/* first remove from old AP if there was any */
		if (n->ap_node) {
			list_del_from(&n->ap_node->ap_nodes, &n->ap_list);
			n->ap_node = NULL;
		}
		/* find AP node and add to his list of stations */
		list_for_each(nodes, ap, list) {
			if (memcmp(n->wlan_bssid, ap->wlan_src, WLAN_MAC_LEN) == 0) {
				LOG_DBG("AP node found %p " MAC_FMT,
					ap, MAC_PAR(n->wlan_bssid));
				list_add_tail(&ap->ap_nodes, &n->ap_list);
				n->ap_node = ap;
				break;
			}
		}
		/* TODO: what if AP is unknown? */
	}
}

void uwifi_nodes_timeout(struct list_head* nodes, unsigned int timeout_sec,
			 uint32_t* last_nodetimeout)
{
	struct uwifi_node *n, *m, *n2, *m2;
//	struct chan_node *cn, *cn2;
	uint32_t the_time = plat_time_usec();

	if ((the_time - *last_nodetimeout) < timeout_sec * 1000000)
		return;
	LOG_DBG("NODE timeout %d", timeout_sec);

	list_for_each_safe(nodes, n, m, list) {
		if (the_time - n->last_seen > timeout_sec * 1000000) {
			LOG_DBG("NODE timeout %p " MAC_FMT, n,
				MAC_PAR(n->wlan_src));
			list_del_from(nodes, &n->list);
			if (n->ap_node) {
				list_del_from(&n->ap_node->ap_nodes, &n->ap_list);
				n->ap_node = NULL;
			}
			if (n->essid != NULL)
				uwifi_essids_remove_node(n);
//			list_for_each_safe(&n->on_channels, cn, cn2, node_list) {
//				list_del(&cn->node_list);
//				list_del(&cn->chan_list);
//				cn->chan->num_nodes--;
//				free(cn);
//			}
			/* clear AP list */
			list_for_each_safe(&n->ap_nodes, n2, m2, ap_list) {
				list_del_from(&n->ap_nodes, &n2->ap_list);
				n2->ap_node = NULL;
			}
			free(n);
		}
	}
	*last_nodetimeout = the_time;
}

void uwifi_nodes_free(struct list_head* nodes)
{
	struct uwifi_node *ni, *mi;

	/* protect against uninitialized lists */
	if (nodes->n.next == NULL)
		return;

	list_for_each_safe(nodes, ni, mi, list) {
		LOG_DBG("NODE free %p " MAC_FMT, ni, MAC_PAR(ni->wlan_src));
		list_del_from(nodes, &ni->list);
		free(ni);
	}
}
