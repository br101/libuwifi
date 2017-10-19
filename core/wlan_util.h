/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_WLAN_UTIL_H_
#define _UWIFI_WLAN_UTIL_H_

#include <stdbool.h>
#include <stdint.h>

#include "wlan80211.h"

enum uwifi_chan_width;

enum uwifi_80211_std {
	IEEE80211_, // UNKNOWN
	IEEE80211_B,
	IEEE80211_G,
	IEEE80211_A,
	IEEE80211_N,
	IEEE80211_AC,
};

struct pkt_name {
	const char c;
	const char* name;
	const uint16_t fc;
	const char* desc;
};

/*
 * Names and abbreviations for all WLAN frame types (2 bit, but only MGMT, CTRL
 * and DATA defined) and subtypes (4 bit)
 */
extern const struct pkt_name stype_names[WLAN_NUM_TYPES][WLAN_NUM_STYPES];

struct pkt_name wlan_get_packet_struct(uint16_t type);
char wlan_get_packet_type_char(uint16_t type);
const char* wlan_get_packet_type_name(uint16_t type);
int wlan_rate_to_index(int rate);
int wlan_rate_to_rate(int idx);
int wlan_ht_mcs_to_rate(int mcs, bool ht20, bool lgi);
int wlan_vht_mcs_to_rate(enum uwifi_chan_width width, int streams, int mcs, bool sgi);
enum uwifi_chan_width wlan_chan_width_from_vht_capab(uint32_t vht);
void wlan_ht_streams_from_mcs(unsigned char* mcs, unsigned char* rx, unsigned char* tx);
void wlan_vht_streams_from_mcs(unsigned char* mcs, unsigned char* rx, unsigned char* tx);
enum uwifi_80211_std wlan_80211std_from_chan(enum uwifi_chan_width width, int chan);
enum uwifi_80211_std wlan_80211std_from_rate(int rate_idx, int chan);
enum uwifi_80211_std wlan_80211std_from_type(uint16_t fc);
const char* wlan_80211std_str(enum uwifi_80211_std std);
const char* wlan_mode_string(int mode);
int wlan_max_phy_rate(enum uwifi_chan_width width, unsigned char streams_rx);
int wlan_freq2chan(int freq);
/* limited version as ambiguous without band */
int wlan_chan2freq(int channel);

#endif
