/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_CHANNEL_H_
#define _UWIFI_CHANNEL_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_BANDS		2
#define MAX_CHANNELS		64

enum uwifi_chan_width {
	CHAN_WIDTH_UNSPEC,
	CHAN_WIDTH_20_NOHT,
	CHAN_WIDTH_20,
	CHAN_WIDTH_40,
	CHAN_WIDTH_80,
	CHAN_WIDTH_160,
	CHAN_WIDTH_8080,
};

/* channel to frequency mapping */
struct uwifi_chan_freq {
	int chan;
	unsigned int freq;
	enum uwifi_chan_width max_width;
	bool ht40plus;
	bool ht40minus;
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

struct uwifi_chan_spec {
	unsigned int freq;
	enum uwifi_chan_width width;
	unsigned int center_freq;
};

struct uwifi_interface;

bool uwifi_channel_change(struct uwifi_interface* intf, struct uwifi_chan_spec* spec);
int uwifi_channel_auto_change(struct uwifi_interface* intf);
void uwifi_channel_get_next(struct uwifi_interface* intf, struct uwifi_chan_spec* new_chan);
int uwifi_channel_idx_from_chan(struct uwifi_channels* channels, int c);
int uwifi_channel_idx_from_freq(struct uwifi_channels* channels, unsigned int f);
int uwifi_channel_get_chan(struct uwifi_channels* channels, int idx);
int uwifi_channel_get_freq(struct uwifi_channels* channels, int idx);
int uwifi_channel_get_num_channels(struct uwifi_channels* channels);
bool uwifi_channel_init(struct uwifi_interface* intf);
bool uwifi_channel_list_add(struct uwifi_channels* channels, int freq);
uint32_t uwifi_channel_get_remaining_dwell_time(struct uwifi_interface* intf);
char* uwifi_channel_list_string(struct uwifi_channels* channels, int idx);
const char* uwifi_channel_width_string(enum uwifi_chan_width w);
/* Note: ht40p is used only for HT40 channels. If it should not be shown use -1 */
const char* uwifi_channel_width_string_short(enum uwifi_chan_width w, int ht40p);
enum uwifi_chan_width uwifi_channel_width_from_mhz(int width);
bool uwifi_channel_verify(struct uwifi_chan_spec* ch, struct uwifi_channels* channels);
char* uwifi_channel_get_string(const struct uwifi_chan_spec* spec);
int uwifi_channel_get_num_bands(struct uwifi_channels* channels);
int uwifi_channel_idx_from_band_idx(struct uwifi_channels* channels, int band, int idx);
const struct uwifi_band* uwifi_channel_get_band(struct uwifi_channels* channels, int b);
bool uwifi_channel_band_add(struct uwifi_channels* channels, int num_channels, enum uwifi_chan_width max_chan_width,
		      unsigned char streams_rx, unsigned char streams_tx);
bool uwifi_channel_is_ht40plus(const struct uwifi_chan_spec* spec);
void uwifi_channel_fix_center_freq(struct uwifi_chan_spec* chan, bool ht40plus);

#endif
