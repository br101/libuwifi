/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015 Tuomas Räsänen <tuomasjjrasanen@tjjr.fi>
 * Copyright (C) 2015-2016 Bruno Randolf <br1@einfach.org>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_IFCTRL_H
#define _UWIFI_IFCTRL_H

#include <stdbool.h>
#include <stddef.h>

#include "channel.h"

bool ifctrl_init(void);
void ifctrl_finish(void);

/**
 * ifctrl_iwadd_monitor() - add virtual 802.11 monitor interface
 *
 * @interface: the name of the interface the monitor interface is attached to
 * @monitor_interface: the name of the new monitor interface
 *
 * Return true on success, false on error.
 */
bool ifctrl_iwadd_monitor(const char *interface, const char *monitor_interface);

/**
 * ifctrl_iwdel() - delete 802.11 interface
 *
 * @interface: the name of the interface
 *
 * Return true on success, false on error.
 */
bool ifctrl_iwdel(const char *interface);

/**
 * ifctrl_iwset_monitor() - set 802.11 interface to monitor mode
 *
 * @interface: the name of the interface
 *
 * Return true on success, false on error.
 */
bool ifctrl_iwset_monitor(const char *interface);

bool ifctrl_iwset_freq(const char *const interface, unsigned int freq,
		       enum uwifi_chan_width width, unsigned int center1);

bool ifctrl_iwget_interface_info(struct uwifi_interface* intf);

bool ifctrl_iwget_freqlist(struct uwifi_interface* intf);

bool ifctrl_is_monitor(struct uwifi_interface* intf);

struct sta_info {
	unsigned char mac[6];
	int8_t rssi;
	int8_t rssi_avg;
	uint32_t last;
};

int ifctrl_iwget_stations(const char *const ifname, struct sta_info* inf, size_t maxlen);

struct survey_info {
	uint32_t freq;
	bool in_use;
	int8_t noise;
	uint64_t time_active;
	uint64_t time_busy;
	uint64_t time_busy_ext;
	uint64_t time_rx;
	uint64_t time_tx;
};

int ifctrl_iwget_survey(const char *const ifname, struct survey_info* inf, size_t maxlen);

bool ifctrl_iw_disconnect(const char *const interface);

bool ifctrl_iw_connect(const char *const interface, const char* essid, int freq,
		       const unsigned char* bssid);

typedef void (*iw_event_cb_t)(int evt, int phy, int ifindex, const unsigned char* mac, int x);

int ifctrl_iw_event_init_socket(iw_event_cb_t);
void ifctrl_iw_event_receive();
bool ifctrl_iwadd_sta(int phyidx, const char *const new_interface);

#endif
