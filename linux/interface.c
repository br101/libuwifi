/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <unistd.h>

#include "conf.h"
#include "ifctrl.h"
#include "netdev.h"
#include "packet_sock.h"
#include "util.h"
#include "node.h"
#include "log.h"

bool uwifi_init(struct uwifi_interface* intf)
{
	list_head_init(&intf->wlan_nodes);
	intf->channel_idx = -1;
	intf->last_channelchange = plat_time_usec();
	intf->sock = packet_socket_open(intf->ifname);

	if (intf->sock < 0) {
		LOG_ERR("Could not open packet socket on '%s'", intf->ifname);
		return false;
	}

	if (!netdev_set_up_promisc(intf->ifname, true, true)) {
		LOG_ERR("Failed to bring '%s' up", intf->ifname);
		return false;
	}

	intf->arphdr = netdev_get_hwinfo(intf->ifname);

	if (intf->arphdr != ARPHRD_IEEE80211_RADIOTAP &&
	    intf->arphdr != ARPHRD_IEEE80211_PRISM) {
		LOG_ERR("Interface '%s' not in monitor mode", intf->ifname);
		return false;
	}

	if (!ifctrl_iwget_interface_info(intf))
		return false;

	if (!uwifi_channel_init(intf)) {
		LOG_ERR("Failed to initialize channels");
		return false;
	}

	return true;
}

void uwifi_fini(struct uwifi_interface* intf)
{
	if (intf->sock > 0) {
		close(intf->sock);
		intf->sock = -1;
	}

	netdev_set_up_promisc(intf->ifname, true, false);

	uwifi_nodes_free(&intf->wlan_nodes);
}
