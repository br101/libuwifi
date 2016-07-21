#ifndef _UWIFI_CONF_H_
#define _UWIFI_CONF_H_

#include "ccan/list/list.h"
#include "wlan80211.h"
#include "channel.h"
#include "platform.h"

#define IF_NAMESIZE		16
#define MAX_CONF_VALUE_STRLEN	200
#define MAX_CONF_NAME_STRLEN	32
#define MAX_FILTERMAC		9

struct wlan_interface {
	int			sock;
	char			ifname[IF_NAMESIZE + 1];
	int			channel_time;	/* dwell time in usec */
	int			channel_max;
	bool			channel_scan;
	int			channel_scan_rounds;
	int			channel_set_num;	/* value we want to set */
	enum chan_width		channel_set_width;	/* value we want to set */
	bool			channel_set_ht40plus;	/* value we want to set */

	/* not config but state */
	struct list_head	wlan_nodes;
	struct channel_list	channels;
	int			num_channels;
	bool			channel_initialized;

	int			channel_idx;	/* index into channels array */
	enum chan_width		channel_width;
	bool			channel_ht40plus;	/* channel is HT40+ */
	uint32_t		last_channelchange;

	int			if_phy;
	unsigned int		if_freq;
	unsigned int		max_phy_rate;
	int			if_type;
	int			arphdr; // the device ARP type
};

#endif
