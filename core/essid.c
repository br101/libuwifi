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
#include "wlan80211.h"
#include "node.h"
#include "util.h"
#include "essid.h"
#include "log.h"

static void update_essid_split_status(struct essid_info* e)
{
	struct uwifi_node* n;
	unsigned char* last_bssid = NULL;

	e->split = 0;

	/* essid can't be split if it only contains 1 node */
	if (e->num_nodes <= 1)
		return;

	list_for_each(&e->nodes, n, essid_nodes) {
		LOG_DBG("ESSID SPLIT check node %p src " MAC_FMT " bssid " MAC_FMT,
			n, MAC_PAR(n->wlan_src), MAC_PAR(n->wlan_bssid));

		if (n->wlan_mode & WLAN_MODE_AP || n->wlan_mode & WLAN_MODE_PROBE)
			continue;

		if (last_bssid && memcmp(last_bssid, n->wlan_bssid, WLAN_MAC_LEN) != 0)
			e->split = 1;

		last_bssid = n->wlan_bssid;
	}

	if (e->split > 0)
		LOG_INF("ESSID SPLIT detected");
}

void uwifi_essids_remove_node(struct uwifi_node* n)
{
	struct essid_info* e = n->essid;
	if (e == NULL)
		return;

	/* first remove ESSID from node */
	LOG_DBG("ESSID remove node " MAC_FMT, MAC_PAR(n->wlan_src));
	list_del_from(&e->nodes, &n->essid_nodes);
	n->essid = NULL;

	/* then deal with ESSID itself */
	LOG_DBG("ESSID remove mark 2");
	e->num_nodes--;

	/* delete essid if it has no more nodes */
	if (e->num_nodes == 0) {
		LOG_DBG("ESSID empty, delete");
		list_del(&e->list);
		free(e);
	} else {
		LOG_DBG("ESSID remove mark 1");
		update_essid_split_status(e);
	}
}

void uwifi_essids_update(struct list_head* essids, struct uwifi_packet* p,
			 struct uwifi_node* n)
{
	struct essid_info* e;

	if (n == NULL || p == NULL || p->phy_flags & PHY_FLAG_BADFCS ||
	    p->wlan_essid[0] == '\0')
		return; /* ignore */

	/* only check beacons and probe response frames */
	if (p->wlan_type != WLAN_FRAME_BEACON &&
	    p->wlan_type != WLAN_FRAME_PROBE_RESP)
		return;

	LOG_DBG("ESSID check '%s' node " MAC_FMT " bssid " MAC_FMT,
		p->wlan_essid, MAC_PAR(n->wlan_src), MAC_PAR(p->wlan_bssid));

	/* find essid if already recorded */
	list_for_each(essids, e, list) {
		if (strncmp(e->essid, p->wlan_essid, WLAN_MAX_SSID_LEN) == 0) {
			LOG_DBG("ESSID found");
			break;
		}
	}

	/* if not add new essid */
	if (&e->list == &essids->n) {
		LOG_DBG("ESSID not found, adding new");
		e = malloc(sizeof(struct essid_info));
		memset(e, 0, sizeof(struct essid_info));
		strncpy(e->essid, p->wlan_essid, WLAN_MAX_SSID_LEN);
		e->essid[WLAN_MAX_SSID_LEN-1] = '\0';
		list_head_init(&e->nodes);
		list_add_tail(essids, &e->list);
	}

	/* if node had another essid before, remove it there */
	if (n->essid != NULL && n->essid != e) {
		LOG_DBG("ESSID remove old '%s'", n->essid->essid);
		uwifi_essids_remove_node(n);
	}

	/* new node */
	if (n->essid == NULL) {
		LOG_DBG("ESSID adding " MAC_FMT " to '%s'",
			MAC_PAR(n->wlan_src), e->essid);
		list_add_tail(&e->nodes, &n->essid_nodes);
		e->num_nodes++;
		n->essid = e;
	}

	update_essid_split_status(e);
}

void uwifi_essids_free(struct list_head* essids) {
	struct essid_info *e, *f;

	list_for_each_safe(essids, e, f, list) {
		LOG_DBG("ESSID free '%s'", e->essid);
		list_del_from(essids, &e->list);
		free(e);
	}
}
