/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include "platform.h"
#include "util.h"
#include "ifctrl.h"
#include "channel.h"
#include "wlan_util.h"
#include "conf.h"

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


static int get_center_freq_ht40(struct uwifi_channels* channels, unsigned int freq, bool upper)
{
	unsigned int center = 0;
	/*
	 * For HT40 we have a channel offset of 20 MHz, and the
	 * center frequency is in the middle: +/- 10 MHz, depending
	 * on HT40+ or HT40- and whether the channel exists
	 */
	if (upper && uwifi_channel_idx_from_freq(channels, freq + 20) != -1)
		center = freq + 10;
	else if (!upper && uwifi_channel_idx_from_freq(channels, freq - 20) != -1)
		center = freq - 10;
	return center;
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
			printlog(LOG_ERR, "VHT80+80 not supported");
			break;
		default:
			printlog(LOG_ERR, "%s is not VHT", uwifi_channel_width_string(width, -1));
	}
	return center1;
}


const char* uwifi_channel_width_string(enum uwifi_chan_width w, int ht40p)
{
	switch (w) {
		case CHAN_WIDTH_UNSPEC: return "?";
		case CHAN_WIDTH_20_NOHT: return "20 (no HT)";
		case CHAN_WIDTH_20: return "HT20";
		case CHAN_WIDTH_40: return ht40p < 0 ? "HT40" : ht40p ? "HT40+" : "HT40-";
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

/* Note: ht40plus is only used for HT40 channel width, to distinguish between
 * HT40+ and HT40- */
bool uwifi_channel_change(struct uwifi_interface* intf, int idx, enum uwifi_chan_width width, bool ht40plus)
{
	unsigned int center1 = 0;

	if (width == CHAN_WIDTH_UNSPEC)
		width = channel_get_band_from_idx(&intf->channels, idx).max_chan_width;

	switch (width) {
		case CHAN_WIDTH_20_NOHT:
		case CHAN_WIDTH_20:
			break;
		case CHAN_WIDTH_40:
			center1 = get_center_freq_ht40(&intf->channels, intf->channels.chan[idx].freq, ht40plus);
			break;
		case CHAN_WIDTH_80:
		case CHAN_WIDTH_160:
			center1 = get_center_freq_vht(intf->channels.chan[idx].freq, width);
			break;
		default:
			printlog(LOG_ERR, "%s not implemented", uwifi_channel_width_string(width, -1));
			break;
	}

	/* only 20 MHz channels don't need additional center freq, otherwise warn
	 * if someone tries invalid HT40+/- channels */
	if (center1 == 0 && !(width == CHAN_WIDTH_20_NOHT || width == CHAN_WIDTH_20)) {
		printlog(LOG_ERR, "CH %d (%d MHz) %s not valid",
			 intf->channels.chan[idx].chan, intf->channels.chan[idx].freq,
			 uwifi_channel_width_string(width, ht40plus));
		return false;
	}

	uint32_t the_time = plat_time_usec();

	if (!ifctrl_iwset_freq(intf->ifname, intf->channels.chan[idx].freq, width, center1)) {
		printlog(LOG_ERR, "Failed to set CH %d (%d MHz) %s center %d after %dms",
			intf->channels.chan[idx].chan, intf->channels.chan[idx].freq,
			uwifi_channel_width_string(width, ht40plus),
			center1, (the_time - intf->last_channelchange) / 1000);
		return false;
	}

// 	printlog(LOG_INFO, "Set CH %d (%d MHz) %s center %d after %dms",
// 		intf->channels.chan[idx].chan, intf->channels.chan[idx].freq,
// 		uwifi_channel_width_string(width, ht40plus),
// 		 center1, (the_time - intf->last_channelchange) / 1000);

	intf->channel_idx = idx;
	intf->channel_width = width;
	intf->channel_ht40plus = ht40plus;
	intf->max_phy_rate = wlan_max_phy_rate(width, channel_get_band_from_idx(&intf->channels, idx).streams_rx);
	intf->last_channelchange = the_time;
	return true;
}

/* Return -1 on error, 0 when no change necessary and 1 on success */
int uwifi_channel_auto_change(struct uwifi_interface* intf)
{
	int ret;
	int new_idx;
	int start_idx;
	bool ht40plus;

	if (!intf->channel_scan)
		return 0;

	/* Return if the current channel is still unknown for some reason
	 * (mac80211 bug, busy physical interface, etc.), it will be fixed when
	 * the first packet arrives, see fixup_packet_channel().
	 *
	 * Without this return, we would busy-loop forever below because
	 * start_idx would be -1 as well. Short-circuit exiting here is quite
	 * logical though: it does not make any sense to scan channels as long
	 * as the channel module is not initialized properly. */
	if (intf->channel_idx == -1)
		return 0;

	if (uwifi_channel_get_remaining_dwell_time(intf) > 0)
		return 0; /* too early */

	ht40plus = intf->channel_ht40plus;
	start_idx = new_idx = intf->channel_idx;

	do {
		enum uwifi_chan_width max_width = channel_get_band_from_idx(&intf->channels, new_idx).max_chan_width;
		/*
		 * For HT40 we visit the same channel twice, once with HT40+
		 * and once with HT40-. This is necessary to see the HT40+/-
		 * data packets
		 */
		if (max_width == CHAN_WIDTH_40) {
			if (ht40plus)
				new_idx++;
			ht40plus = !ht40plus; // toggle
		} else {
			new_idx++;
		}

		if (new_idx >= intf->channels.num_channels ||
		    new_idx >= MAX_CHANNELS ||
		    (intf->channel_max &&
		     uwifi_channel_get_chan(&intf->channels, new_idx) > intf->channel_max)) {
			new_idx = 0;
			max_width = channel_get_band_from_idx(&intf->channels, new_idx).max_chan_width;
			ht40plus = true;
		}

		/* since above we simply toggle HT40+/- check here to avoid invalid HT40+/- channels */
		if (get_center_freq_ht40(&intf->channels, intf->channels.chan[new_idx].freq, ht40plus) == 0)
			continue;

		ret = uwifi_channel_change(intf, new_idx, max_width, ht40plus);

		/* try setting different channels in case we get errors only on
		 * some channels (e.g. ipw2200 reports channel 14 but cannot be
		 * set to use it). stop if we tried all channels */
	} while (ret != 1 && (new_idx != start_idx || ht40plus != intf->channel_ht40plus));

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

char* uwifi_channel_get_string(struct uwifi_channels* channels, int idx)
{
	static char buf[32];
	struct uwifi_chan_freq* c = &channels->chan[idx];
	sprintf(buf, "%-3d: %d HT40%s%s", c->chan, c->freq,
			get_center_freq_ht40(channels, c->freq, true) ? "+" : "",
			get_center_freq_ht40(channels, c->freq, false) ? "-" : "");
	return buf;
}

bool uwifi_channel_init(struct uwifi_interface* intf)
{
	/* get available channels */
	ifctrl_iwget_freqlist(intf);
	intf->channel_initialized = 1;

	printlog(LOG_INFO, "Got %d Bands, %d Channels:", intf->channels.num_bands, intf->channels.num_channels);
	for (int i = 0; i < intf->channels.num_channels && i < MAX_CHANNELS; i++)
		printlog(LOG_INFO, "%s", uwifi_channel_get_string(&intf->channels, i));

	if (intf->channels.num_bands <= 0 || intf->channels.num_channels <= 0)
		return false;

	if (intf->channel_set_num > 0) {
		/* configured values */
		printlog(LOG_INFO, "Setting configured channel %d", intf->channel_set_num);
		int ini_idx = uwifi_channel_idx_from_chan(&intf->channels, intf->channel_set_num);
		if (!uwifi_channel_change(intf, ini_idx, intf->channel_set_width, intf->channel_set_ht40plus))
			return false;
	} else {
		if (intf->if_freq <= 0) {
			/* this happens when we are on secondary monitor interface */
			printlog(LOG_ERR, "Could not get current channel");
			intf->max_phy_rate = wlan_max_phy_rate(intf->channels.band[0].max_chan_width,
							       intf->channels.band[0].streams_rx);
			intf->channel_idx = -1;
			return true; // not failure

		}
		intf->channel_idx = uwifi_channel_idx_from_freq(&intf->channels, intf->if_freq);
		intf->channel_set_num = uwifi_channel_get_chan(&intf->channels, intf->channel_idx);
		printlog(LOG_INFO, "Current channel: %d (%d MHz) %s",
			 intf->channel_set_num, intf->if_freq,
			 uwifi_channel_width_string(intf->channel_width, intf->channel_ht40plus));

		/* try to set max width */
		struct uwifi_band b = channel_get_band_from_idx(&intf->channels, intf->channel_idx);
		if (intf->channel_width != b.max_chan_width) {
			printlog(LOG_INFO, "Try to set max channel width %s",
				uwifi_channel_width_string(b.max_chan_width, -1));
			// try both HT40+ and HT40- if necessary
			if (!uwifi_channel_change(intf, intf->channel_idx, b.max_chan_width, true) &&
			    !uwifi_channel_change(intf, intf->channel_idx, b.max_chan_width, false))
				return false;
		} else {
			intf->channel_set_width = intf->channel_width;
			intf->channel_set_ht40plus = intf->channel_ht40plus;
			intf->max_phy_rate = wlan_max_phy_rate(intf->channel_width, b.streams_rx);
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
