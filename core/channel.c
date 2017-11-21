/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <stdio.h>

#include "platform.h"
#include "util.h"
#include "ifctrl.h"
#include "channel.h"
#include "wlan_util.h"
#include "conf.h"
#include "log.h"

uint32_t uwifi_channel_get_remaining_dwell_time(struct uwifi_interface* intf)
{
	if (!intf->channel_scan)
		return UINT32_MAX;

	int64_t ret = (int64_t)intf->channel_time - (plat_time_usec() - intf->last_channelchange);

	if (ret < 0)
		return 0;
	else if (ret > UINT32_MAX)
		return UINT32_MAX;
	else
		return ret;
}

static struct uwifi_band channel_get_band_from_idx(struct uwifi_channels* channels, int idx)
{
	int b = idx - channels->band[0].num_channels < 0 ? 0 : 1;
	return channels->band[b];
}

static int get_center_freq_vht(unsigned int freq, enum uwifi_chan_width width)
{
	unsigned int center1 = 0;
	switch(width) {
		case CHAN_WIDTH_80:
			/*
			 * VHT80 channels are non-overlapping and the primary
			 * channel can be on any HT20/40 channel in the range
			 */
			if (freq >= 5180 && freq <= 5240)
				center1 = 5210;
			else if (freq >= 5260 && freq <= 5320)
				center1 = 5290;
			else if (freq >= 5500 && freq <= 5560)
				center1 = 5530;
			else if (freq >= 5580 && freq <= 5640)
				center1 = 5610;
			else if (freq >= 5660 && freq <= 5720)
				center1 = 5690;
			else if (freq >= 5745 && freq <= 5805)
				center1 = 5775;
			break;
		case CHAN_WIDTH_160:
			/*
			 * There are only two possible VHT160 channels
			 */
			if (freq >= 5180 && freq <= 5320)
				center1 = 5250;
			else if (freq >= 5180 && freq <= 5320)
				center1 = 5570;
			break;
		case CHAN_WIDTH_8080:
			LOG_ERR("VHT80+80 not supported");
			break;
		default:
			LOG_ERR("%s is not VHT", uwifi_channel_width_string(width));
	}
	return center1;
}

/* upper only used if HT40 */
void uwifi_channel_fix_center_freq(struct uwifi_chan_spec* chan, bool ht40plus)
{
	/* set center freq from width or HT40+/- */
	switch (chan->width) {
		case CHAN_WIDTH_20_NOHT:
		case CHAN_WIDTH_20:
			break; /* no center freq necessary */
		case CHAN_WIDTH_40:
			/* this may select a channel out of range */
			chan->center_freq = chan->freq + (ht40plus ? 10 : -10);
			break;
		case CHAN_WIDTH_80:
		case CHAN_WIDTH_160:
			chan->center_freq = get_center_freq_vht(chan->freq, chan->width);
			break;
		default:
			LOG_ERR("%s not implemented",
				uwifi_channel_width_string(chan->width));
			break;
	}
}

const char* uwifi_channel_width_string(enum uwifi_chan_width w)
{
	switch (w) {
		case CHAN_WIDTH_UNSPEC: return "?";
		case CHAN_WIDTH_20_NOHT: return "20 (no HT)";
		case CHAN_WIDTH_20: return "HT20";
		case CHAN_WIDTH_40: return "HT40";
		case CHAN_WIDTH_80: return "VHT80";
		case CHAN_WIDTH_160: return "VHT160";
		case CHAN_WIDTH_8080: return "VHT80+80";
	}
	return "";
}

const char* uwifi_channel_width_string_short(enum uwifi_chan_width w, int ht40p)
{
	switch (w) {
		case CHAN_WIDTH_UNSPEC: return "?";
		case CHAN_WIDTH_20_NOHT: return "20g";
		case CHAN_WIDTH_20: return "20";
		case CHAN_WIDTH_40: return ht40p < 0 ? "40" : ht40p ? "40+" : "40-";
		case CHAN_WIDTH_80: return "80";
		case CHAN_WIDTH_160: return "160";
		case CHAN_WIDTH_8080: return "80+80";
	}
	return "";
}

enum uwifi_chan_width uwifi_channel_width_from_mhz(int width)
{
	switch (width) {
		case 20: return CHAN_WIDTH_20;
		case 40: return CHAN_WIDTH_40;
		case 80: return CHAN_WIDTH_80;
		case 160: return CHAN_WIDTH_160;
	}
	return CHAN_WIDTH_UNSPEC;
}

/* internal version does not check primary channel and width */
static bool uwifi_channel_verify_ch(struct uwifi_chan_spec* ch, struct uwifi_channels* channels)
{
	int idx;

	if (ch->width == CHAN_WIDTH_40) {
		/* center freq difference */
		int d = ch->center_freq - ch->freq;
		if (d != 10 && d != -10)
			return false;
		/* secondary channel exists */
		idx = uwifi_channel_idx_from_freq(channels, ch->center_freq + d);
		if (idx == -1)
			return false;
	} else if (ch->width == CHAN_WIDTH_80 || ch->width == CHAN_WIDTH_160) {
		/* all channels in band exist */
		int max = ch->width == CHAN_WIDTH_80 ? 30 : 70;
		for (int i = -max; i < max; i += 20) {
			idx = uwifi_channel_idx_from_freq(channels, ch->center_freq + i);
			if (idx == -1)
				return false;
		}
	}

	return true;
}

bool uwifi_channel_verify(struct uwifi_chan_spec* ch, struct uwifi_channels* channels)
{
	/* primary channel exists */
	int idx = uwifi_channel_idx_from_freq(channels, ch->freq);
	if (idx == -1)
		return false;

	/* width is ok */
	if (ch->width > channels->chan[idx].max_width)
		return false;

	return uwifi_channel_verify_ch(ch, channels);
}

char* uwifi_channel_list_string(struct uwifi_channels* channels, int idx)
{
	static char buf[32];
	struct uwifi_chan_freq* c = &channels->chan[idx];
	int pos = sprintf(buf, "%-3d: %d %s", c->chan, c->freq,
			uwifi_channel_width_string(c->max_width));

	if (c->max_width >= CHAN_WIDTH_40)
		pos += sprintf(buf+pos, "%s%s", c->ht40plus ? "+" : "",
						c->ht40minus ? "-" : "");
#if 0
	int pos=0;
	int cent = get_center_freq_ht40(channels, c->freq, false);
	if (cent)
		pos = sprintf(buf, "%d\t%d\t%d\n", c->freq, 40, cent);
	cent = get_center_freq_ht40(channels, c->freq, true);
	if (cent)
		sprintf(buf+pos, "%d\t%d\t%d", c->freq, 40, cent);
#endif
	return buf;
}

char* uwifi_channel_get_string(const struct uwifi_chan_spec* spec)
{
	static char buf[32];
	int pos = sprintf(buf, "CH %d (%d MHz) %s",
			  wlan_freq2chan(spec->freq), spec->freq,
			  uwifi_channel_width_string(spec->width));
	if (spec->width == CHAN_WIDTH_40)
		sprintf(buf+pos, "%c",
			spec->center_freq < spec->freq ? '-' : '+');
	else if (spec->width >= CHAN_WIDTH_80)
		sprintf(buf+pos, " (center %d)", spec->center_freq);
	return buf;
}

bool uwifi_channel_is_ht40plus(const struct uwifi_chan_spec* spec)
{
	return spec->width == CHAN_WIDTH_40 && spec->center_freq > spec->freq;
}

bool uwifi_channel_change(struct uwifi_interface* intf, struct uwifi_chan_spec* spec)
{
	/* only 20 MHz channels don't need additional center freq, otherwise warn
	 * if someone tries invalid HT40+/- channels */
	if (spec->center_freq == 0 && !(spec->width == CHAN_WIDTH_20_NOHT || spec->width == CHAN_WIDTH_20)) {
		LOG_ERR("%s not valid", uwifi_channel_get_string(spec));
		return false;
	}

	uint32_t the_time = plat_time_usec();

	if (!ifctrl_iwset_freq(intf->ifname, spec->freq, spec->width, spec->center_freq)) {
		LOG_ERR("Failed to set %s center %d after %dms",
			uwifi_channel_get_string(spec), spec->center_freq,
			(the_time - intf->last_channelchange) / 1000);
		return false;
	}

	LOG_DBG("Set %s center %d after %dms",
		uwifi_channel_get_string(spec), spec->center_freq,
		(the_time - intf->last_channelchange) / 1000);

	intf->channel_idx = uwifi_channel_idx_from_freq(&intf->channels, spec->freq);
	intf->channel = *spec;
	intf->max_phy_rate = wlan_max_phy_rate(spec->width, channel_get_band_from_idx(&intf->channels, intf->channel_idx).streams_rx);
	intf->last_channelchange = the_time;
	return true;
}

void uwifi_channel_get_next(struct uwifi_interface* intf,
			    struct uwifi_chan_spec* new_chan)
{
	int new_idx = intf->channel_idx;
	bool ht40plus = uwifi_channel_is_ht40plus(&intf->channel);

	struct uwifi_chan_freq* ch = &intf->channels.chan[new_idx];

	/* increment channel, but for HT40 visit the same channel twice,
	 * once with HT40+ and once HT40-, but only if supported */
	if (intf->channel.width == CHAN_WIDTH_40 && !ht40plus && ch->ht40plus) {
		ht40plus = true;
	} else {
		/* increment & wrap around */
		new_idx++;
		if (new_idx >= intf->channels.num_channels ||
		    new_idx >= MAX_CHANNELS ||
		    (intf->channel_max &&
		     uwifi_channel_get_chan(&intf->channels, new_idx) > intf->channel_max)) {
			new_idx = 0;
		}
		ch = &intf->channels.chan[new_idx];
		/* new channel might not be able to do HT- */
		ht40plus = ch->ht40minus ? false : true;
	}

	new_chan->freq = ch->freq;
	new_chan->width = ch->max_width;
	uwifi_channel_fix_center_freq(new_chan, ht40plus);

	/* should always be OK */
	bool ok = uwifi_channel_verify(new_chan, &intf->channels);
	if (!ok)
		LOG_ERR("next channel not ok");
}

/* Return -1 on error, 0 when no change necessary and 1 on success */
int uwifi_channel_auto_change(struct uwifi_interface* intf)
{
	int ret = 0;
	int tries;

	if (!intf->channel_scan)
		return 0;

	/* Return if the current channel is still unknown for some reason
	 * (mac80211 bug, busy physical interface, etc.), it will be fixed when
	 * the first packet arrives, see fixup_packet_channel(). */
	if (intf->channel_idx == -1)
		return 0;

	if (uwifi_channel_get_remaining_dwell_time(intf) > 0)
		return 0; /* too early */

	/* maximum number of tries until we give up. we use the number of allowed
	 * channels multiplied by two because we likely try HT40+ and HT40- on
	 * each channel, even though it may fail. Also the exact number of tries
	 * does not matter as long as we try every channel */
	if (intf->channel_max)
		tries = uwifi_channel_idx_from_chan(&intf->channels, intf->channel_max) * 2;
	else
		tries = intf->channels.num_channels * 2;

	struct uwifi_chan_spec new_chan = { 0 };

	do {
		tries--;
		uwifi_channel_get_next(intf, &new_chan);
		LOG_INF("Set %s", uwifi_channel_get_string(&new_chan));
		ret = uwifi_channel_change(intf, &new_chan);

		/* try setting different channels in case we get errors only on
		 * some channels (e.g. ipw2200 reports channel 14 but cannot be
		 * set to use it). stop if we tried all channels */
	} while (ret != 1 && tries > 0);

	/* even when all channels failed, set the last channel change time, so
	 * we don't get into a busy loop, unsuccessfully trying to change
	 * channels all the time. also we hope the application reacts to the -1
	 * error code */
	if (ret != 1) {
		intf->last_channelchange = plat_time_usec();
		return -1;
	}

	return 1;
}

static void chan_check_capab(int idx, struct uwifi_channels* channels)
{
	enum uwifi_chan_width max_width = channel_get_band_from_idx(channels, idx).max_chan_width;
	int ch = channels->chan[idx].chan;

	/* we can always do 20 MHz */
	channels->chan[idx].max_width = CHAN_WIDTH_20;

	/* special case: CH 14 is only allowed for 20 Mhz operation in Japan */
	if (uwifi_channel_get_freq(channels, idx) == 2484)
		return;

	/* HT40 is easier to check directly */
	if (max_width >= CHAN_WIDTH_40) {
		channels->chan[idx].ht40minus = uwifi_channel_idx_from_chan(channels, ch - 4) != -1;
		channels->chan[idx].ht40plus = uwifi_channel_idx_from_chan(channels, ch + 4) != -1;
		if (channels->chan[idx].ht40minus || channels->chan[idx].ht40plus)
			channels->chan[idx].max_width = CHAN_WIDTH_40;
		else
			return; // can't do any higher width either
	}

	/* check VHT80 and 160 */
	struct uwifi_chan_spec new_chan = { 0 };
	new_chan.freq = uwifi_channel_get_freq(channels, idx);
	new_chan.width = CHAN_WIDTH_80;

	while (new_chan.width <= max_width) {
		uwifi_channel_fix_center_freq(&new_chan, false);
		if (!uwifi_channel_verify_ch(&new_chan, channels))
			return;
		channels->chan[idx].max_width = new_chan.width;
		new_chan.width++;
	}
}

bool uwifi_channel_init(struct uwifi_interface* intf)
{
	/* get available channels */
	ifctrl_iwget_freqlist(intf);
	intf->channel_initialized = 1;
	intf->channel_idx = -1;
	intf->last_channelchange = plat_time_usec();

	LOG_INF("Got %d Bands, %d Channels:", intf->channels.num_bands, intf->channels.num_channels);
	for (int i = 0; i < intf->channels.num_channels && i < MAX_CHANNELS; i++) {
		chan_check_capab(i, &intf->channels);
		LOG_INF("%s", uwifi_channel_list_string(&intf->channels, i));
	}

	if (intf->channels.num_bands <= 0 || intf->channels.num_channels <= 0)
		return false;

	if (intf->channel_set.freq > 0) {
		/* configured values */
		LOG_INF("Setting configured channel %s",
			 uwifi_channel_get_string(&intf->channel_set));
		if (!uwifi_channel_change(intf, &intf->channel_set))
			return false;
	} else {
		if (intf->channel.freq <= 0) {
			/* this happens when we are on secondary monitor interface */
			LOG_ERR("Could not get current channel");
			intf->max_phy_rate = wlan_max_phy_rate(intf->channels.band[0].max_chan_width,
							       intf->channels.band[0].streams_rx);
			intf->channel_idx = -1;
			return true; // not failure

		}

		intf->channel_idx = uwifi_channel_idx_from_freq(&intf->channels, intf->channel.freq);
		intf->channel_set = intf->channel;
		LOG_INF("Current channel: %s", uwifi_channel_get_string(&intf->channel));

		/* get max rate from band */
		struct uwifi_band b = channel_get_band_from_idx(&intf->channels, intf->channel_idx);
		intf->max_phy_rate = wlan_max_phy_rate(b.max_chan_width, b.streams_rx);

		/* set max width for this channel */
		struct uwifi_chan_freq* ch = &intf->channels.chan[intf->channel_idx];
		if (intf->channel.width != ch->max_width) {
			intf->channel_set.width = ch->max_width;
			bool ht40plus = (ch->max_width == CHAN_WIDTH_40 && !ch->ht40minus);
			uwifi_channel_fix_center_freq(&intf->channel_set, ht40plus);
			LOG_INF("Set max channel width %s",
				uwifi_channel_get_string(&intf->channel_set));
			if (!uwifi_channel_change(intf, &intf->channel_set))
				return true; // not failure
		}
	}
	return true;
}

int uwifi_channel_idx_from_chan(struct uwifi_channels* channels, int c)
{
	int i = -1;
	for (i = 0; i < channels->num_channels && i < MAX_CHANNELS; i++)
		if (channels->chan[i].chan == c)
			return i;
	return -1;
}

int uwifi_channel_idx_from_freq(struct uwifi_channels* channels, unsigned int f)
{
	int i = -1;
	for (i = 0; i < channels->num_channels && i < MAX_CHANNELS; i++)
		if (channels->chan[i].freq == f)
			return i;
	return -1;
}

int uwifi_channel_get_chan(struct uwifi_channels* channels, int i)
{
	if (i >= 0 && i < channels->num_channels && i < MAX_CHANNELS)
		return channels->chan[i].chan;
	else
		return -1;
}

int uwifi_channel_get_freq(struct uwifi_channels* channels, int idx)
{
	if (idx >= 0 && idx < channels->num_channels && idx < MAX_CHANNELS)
		return channels->chan[idx].freq;
	else
		return -1;
}

bool uwifi_channel_list_add(struct uwifi_channels* channels, int freq)
{
	if (channels->num_channels >=  MAX_CHANNELS)
		return false;

	channels->chan[channels->num_channels].chan = wlan_freq2chan(freq);
	channels->chan[channels->num_channels].freq = freq;
	channels->num_channels++;
	return true;
}

int uwifi_channel_get_num_channels(struct uwifi_channels* channels)
{
	return channels->num_channels;
}

int uwifi_channel_get_num_bands(struct uwifi_channels* channels)
{
	return channels->num_bands;
}

int uwifi_channel_idx_from_band_idx(struct uwifi_channels* channels, int band, int idx)
{
	if (band < 0 || band >= channels->num_bands)
		return -1;

	if (idx < 0 || idx >= channels->band[band].num_channels)
		return -1;

	if (band > 0)
		idx = idx + channels->band[0].num_channels;

	return idx;
}

const struct uwifi_band* uwifi_channel_get_band(struct uwifi_channels* channels, int b)
{
	if (b < 0 || b > channels->num_bands)
		return NULL;
	return &channels->band[b];
}

bool uwifi_channel_band_add(struct uwifi_channels* channels, int num_channels,
		      enum uwifi_chan_width max_chan_width,
		      unsigned char streams_rx, unsigned char streams_tx)
{
	if (channels->num_bands >= MAX_BANDS)
		return false;

	channels->band[channels->num_bands].num_channels = num_channels;
	channels->band[channels->num_bands].max_chan_width = max_chan_width;
	channels->band[channels->num_bands].streams_rx = streams_rx;
	channels->band[channels->num_bands].streams_tx = streams_tx;
	channels->num_bands++;
	return true;
}
