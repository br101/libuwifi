/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include "platform.h"
#include "wlan80211.h"
#include "node.h"
#include "util.h"
#include "essid.h"
#include "log.h"

struct essid_meta_info essids;

static void update_essid_split_status(struct essid_info* e)
{
	struct uwifi_node* n;
	unsigned char* last_bssid = NULL;

	e->split = 0;

	/* essid can't be split if it only contains 1 node */
	if (e->num_nodes <= 1 && essids.split_essid == e) {
		essids.split_active = 0;
		essids.split_essid = NULL;
		return;
	}

	/* check for split */
	list_for_each(&e->nodes, n, essid_nodes) {
		LOG_DBG("SPLIT      node %p src " MAC_FMT " bssid " MAC_FMT,
			n, MAC_PAR(n->wlan_src), MAC_PAR(n->wlan_bssid));

		if (n->wlan_mode & WLAN_MODE_AP || n->wlan_mode & WLAN_MODE_PROBE)
			continue;

		if (last_bssid && memcmp(last_bssid, n->wlan_bssid, WLAN_MAC_LEN) != 0) {
			e->split = 1;
			LOG_DBG("SPLIT *** DETECTED!!!");
		}
		last_bssid = n->wlan_bssid;
	}

	/* if a split occurred on this essid, record it */
	if (e->split > 0) {
		LOG_DBG("SPLIT *** active");
		essids.split_active = 1;
		essids.split_essid = e;
	}
	else if (e == essids.split_essid) {
		LOG_DBG("SPLIT *** ok now");
		essids.split_active = 0;
		essids.split_essid = NULL;
	}
}

static void remove_node_from_essid(struct uwifi_node* n)
{
	LOG_DBG("SPLIT   remove node from old essid");
	list_del(&n->essid_nodes);
	n->essid->num_nodes--;

	update_essid_split_status(n->essid);

	/* delete essid if it has no more nodes */
	if (n->essid->num_nodes == 0) {
		LOG_DBG("SPLIT   essid empty, delete");
		list_del(&n->essid->list);
		free(n->essid);
	}
	n->essid = NULL;
}

void uwifi_essids_update(struct uwifi_packet* p, struct uwifi_node* n)
{
	struct essid_info* e;

	if (n == NULL || (p->phy_flags & PHY_FLAG_BADFCS))
		return; /* ignore */

	/* only check beacons and probe response frames */
	if (p->wlan_type != WLAN_FRAME_BEACON &&
	    p->wlan_type != WLAN_FRAME_PROBE_RESP &&
	    p->wlan_type != WLAN_FRAME_PROBE_REQ)
		return;

	LOG_DBG("SPLIT check ibss '%s' node " MAC_FMT "bssid " MAC_FMT, p->wlan_essid,
		MAC_PAR(n->wlan_src), MAC_PAR(p->wlan_bssid));

	/* find essid if already recorded */
	list_for_each(&essids.list, e, list) {
		if (strncmp(e->essid, p->wlan_essid, WLAN_MAX_SSID_LEN) == 0) {
			LOG_DBG("SPLIT   essid found");
			break;
		}
	}

	/* if not add new essid */
	if (&e->list == &essids.list.n) {
		LOG_DBG("SPLIT   essid not found, adding new");
		e = malloc(sizeof(struct essid_info));
		strncpy(e->essid, p->wlan_essid, WLAN_MAX_SSID_LEN);
		e->essid[WLAN_MAX_SSID_LEN-1] = '\0';
		e->num_nodes = 0;
		e->split = 0;
		list_head_init(&e->nodes);
		list_add_tail(&essids.list, &e->list);
	}

	/* if node had another essid before, remove it there */
	if (n->essid != NULL && n->essid != e)
		remove_node_from_essid(n);

	/* new node */
	if (n->essid == NULL) {
		LOG_DBG("SPLIT   node not found, adding new " MAC_FMT,
			MAC_PAR(n->wlan_src));
		list_add_tail(&e->nodes, &n->essid_nodes);
		e->num_nodes++;
		n->essid = e;
	}

	update_essid_split_status(e);
}

void uwifi_essids_init(void) {
	list_head_init(&essids.list);
}

void uwifi_essids_reset(void) {
	essids.split_active = 0;
	essids.split_essid = NULL;
}

void uwifi_essids_free(void) {
	struct essid_info *e, *f;

	list_for_each_safe(&essids.list, e, f, list) {
		LOG_DBG("free essid '%s'", e->essid);
		list_del(&e->list);
		free(e);
	}
}
