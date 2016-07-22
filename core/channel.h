/* horst - Highly Optimized Radio Scanning Tool
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _UWIFI_CHANNEL_H_
#define _UWIFI_CHANNEL_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_BANDS		2
#define MAX_CHANNELS		64

/* channel to frequency mapping */
struct chan_freq {
	int chan;
	unsigned int freq;
};

enum chan_width {
	CHAN_WIDTH_UNSPEC,
	CHAN_WIDTH_20_NOHT,
	CHAN_WIDTH_20,
	CHAN_WIDTH_40,
	CHAN_WIDTH_80,
	CHAN_WIDTH_160,
	CHAN_WIDTH_8080,
};

struct band_info {
	int num_channels;
	enum chan_width max_chan_width;
	unsigned char streams_rx;
	unsigned char streams_tx;
};

struct channel_list {
	struct chan_freq chan[MAX_CHANNELS];
	int num_channels;
	struct band_info band[MAX_BANDS];
	int num_bands;
};

struct uwifi_interface;

bool channel_change(struct uwifi_interface* intf, int idx, enum chan_width width, bool ht40plus);
bool channel_auto_change(struct uwifi_interface* intf);
int channel_find_index_from_chan(struct channel_list* channels, int c);
int channel_find_index_from_freq(struct channel_list* channels, unsigned int f);
int channel_get_chan(struct channel_list* channels, int idx);
int channel_get_freq(struct channel_list* channels, int idx);
int channel_get_num_channels(struct channel_list* channels);
bool channel_init(struct uwifi_interface* intf);
bool channel_list_add(struct channel_list* channels, int freq);
uint32_t channel_get_remaining_dwell_time(struct uwifi_interface* intf);
char* channel_get_string(struct channel_list* channels, int idx);
/* Note: ht40p is used only for HT40 channels. If it should not be shown use -1 */
const char* channel_width_string(enum chan_width w, int ht40p);
/* Note: ht40p is used only for HT40 channels. If it should not be shown use -1 */
const char* channel_width_string_short(enum chan_width w, int ht40p);
int channel_get_num_bands(struct channel_list* channels);
int channel_get_idx_from_band_idx(struct channel_list* channels, int band, int idx);
const struct band_info* channel_get_band(struct channel_list* channels, int b);
bool channel_band_add(struct channel_list* channels, int num_channels, enum chan_width max_chan_width,
		      unsigned char streams_rx, unsigned char streams_tx);

#endif
