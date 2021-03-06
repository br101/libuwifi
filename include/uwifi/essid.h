/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef UWIFI_ESSID_H_
#define UWIFI_ESSID_H_

#include "cc_list.h"
#include "wlan80211.h"

#ifdef __cplusplus
extern "C" {
#endif

struct essid_info {
	struct cc_list_node	list;
	char			essid[WLAN_MAX_SSID_LEN];
	struct cc_list_head	nodes;
	unsigned int		num_nodes;
	int			split;
};

struct uwifi_node;
struct uwifi_packet;

void uwifi_essids_update(struct cc_list_head* essids, struct uwifi_packet* p,
			 struct uwifi_node* n);
void uwifi_essids_remove_node(struct uwifi_node* n);
void uwifi_essids_free(struct cc_list_head* essids);

#ifdef __cplusplus
}
#endif

#endif
