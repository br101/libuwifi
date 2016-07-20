#ifndef _UWIFI_CONF_H_
#define _UWIFI_CONF_H_

#include "wlan80211.h"
#include "channel.h"
#include "platform.h"

#define IF_NAMESIZE		16
#define MAX_CONF_VALUE_STRLEN	200
#define MAX_CONF_NAME_STRLEN	32
#define MAX_FILTERMAC		9

struct config {
	char			ifname[IF_NAMESIZE + 1];
	int			port;
	int			quiet;
	uint32_t		node_timeout;		// in seconds
	int			channel_time;		// in usec
	int			channel_max;
	int			channel_set_num;	/* value we want to set */
	enum chan_width		channel_set_width;	/* value we want to set */
	enum chan_width		channel_width;
	int			channel_idx;	/* index into channels array */
	int			channel_scan_rounds;
	int			display_interval;
	char			display_view;
	char			dumpfile[MAX_CONF_VALUE_STRLEN + 1];
	int			recv_buffer_size;
	char			serveraddr[MAX_CONF_VALUE_STRLEN + 1];
	char			control_pipe[MAX_CONF_VALUE_STRLEN + 1];
	char			mac_name_file[MAX_CONF_VALUE_STRLEN + 1];

	unsigned char		filtermac[MAX_FILTERMAC][MAC_LEN];
	char			filtermac_enabled[MAX_FILTERMAC];
	unsigned char		filterbssid[MAC_LEN];
	unsigned int		filter_pkt;
	uint16_t		filter_stype[WLAN_NUM_TYPES];  /* one for MGMT, CTRL, DATA */
	unsigned int		filter_mode;
	unsigned int		filter_off:1,
				filter_badfcs:1,
				do_change_channel:1,
				channel_ht40plus:1,	/* channel is HT40+ */
				channel_set_ht40plus:1,	/* value we want to set */
				allow_client:1,
				allow_control:1,
				debug:1,
				mac_name_lookup:1,
				add_monitor:1,
	/* this isn't exactly config, but wtf... */
				do_macfilter:1,
				display_initialized:1,
				channel_initialized:1,
				monitor_added:1;
	int			arphrd; // the device ARP type
	unsigned char		my_mac_addr[MAC_LEN];
	int			paused;
	int			if_type;
	int			if_phy;
	unsigned int		if_freq;
	unsigned int		max_phy_rate;
};

extern struct config conf;

#endif
