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

#include "wlan80211.h"

struct essid_info {
	struct list_node	list;
	char			essid[WLAN_MAX_SSID_LEN];
	struct list_head	nodes;
	unsigned int		num_nodes;
	int			split;
};

struct essid_meta_info {
	struct list_head	list;
	struct essid_info*	split_essid;
	int			split_active;
};

extern struct essid_meta_info essids;

void uwifi_essids_init();
void uwifi_essids_remove_node(struct uwifi_node* n);
void uwifi_essids_update(struct uwifi_packet* p, struct uwifi_node* n);
void uwifi_essids_reset();
void uwifi_essids_free();

#endif
