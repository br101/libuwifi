/* libuwifi - Userspace Wifi Library
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
struct uwifi_chan_freq {
	int chan;
	unsigned int freq;
};

enum uwifi_chan_width {
	CHAN_WIDTH_UNSPEC,
	CHAN_WIDTH_20_NOHT,
	CHAN_WIDTH_20,
	CHAN_WIDTH_40,
	CHAN_WIDTH_80,
	CHAN_WIDTH_160,
	CHAN_WIDTH_8080,
};

struct uwifi_band {
	int num_channels;
	enum uwifi_chan_width max_chan_width;
	unsigned char streams_rx;
	unsigned char streams_tx;
};

struct uwifi_channels {
	struct uwifi_chan_freq chan[MAX_CHANNELS];
	int num_channels;
	struct uwifi_band band[MAX_BANDS];
	int num_bands;
};

struct uwifi_interface;

bool uwifi_channel_change(struct uwifi_interface* intf, int idx, enum uwifi_chan_width width, bool ht40plus);
bool uwifi_channel_auto_change(struct uwifi_interface* intf);
int uwifi_channel_idx_from_chan(struct uwifi_channels* channels, int c);
int uwifi_channel_idx_from_freq(struct uwifi_channels* channels, unsigned int f);
int uwifi_channel_get_chan(struct uwifi_channels* channels, int idx);
int uwifi_channel_get_freq(struct uwifi_channels* channels, int idx);
int uwifi_channel_get_num_channels(struct uwifi_channels* channels);
bool uwifi_channel_init(struct uwifi_interface* intf);
bool uwifi_channel_list_add(struct uwifi_channels* channels, int freq);
uint32_t uwifi_channel_get_remaining_dwell_time(struct uwifi_interface* intf);
char* uwifi_channel_get_string(struct uwifi_channels* channels, int idx);
/* Note: ht40p is used only for HT40 channels. If it should not be shown use -1 */
const char* uwifi_channel_width_string(enum uwifi_chan_width w, int ht40p);
/* Note: ht40p is used only for HT40 channels. If it should not be shown use -1 */
const char* uwifi_channel_width_string_short(enum uwifi_chan_width w, int ht40p);
int uwifi_channel_get_num_bands(struct uwifi_channels* channels);
int uwifi_channel_idx_from_band_idx(struct uwifi_channels* channels, int band, int idx);
const struct uwifi_band* uwifi_channel_get_band(struct uwifi_channels* channels, int b);
bool uwifi_channel_band_add(struct uwifi_channels* channels, int num_channels, enum uwifi_chan_width max_chan_width,
		      unsigned char streams_rx, unsigned char streams_tx);

#endif
