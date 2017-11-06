/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <stdint.h>
#include <endian.h>

#include "prism_header.h"
#include "radiotap.h"
#include "radiotap_iter.h"
#include "wlan80211.h"
#include "wlan_util.h"
#include "util.h"
#include "wlan_parser.h"
#include "platform.h"
#include "conf.h"
#include "raw_parser.h"
#include "netdev.h"
#include "log.h"

int uwifi_parse_prism_header(unsigned char* buf, int len, struct uwifi_packet* p)
{
	wlan_ng_prism2_header* ph = (wlan_ng_prism2_header*)buf;

	if (len > 0 && (size_t)len < sizeof(wlan_ng_prism2_header))
		return -1;

	/*
	 * different drivers report S/N and rssi values differently
	*/
	if (((int)ph->noise.data) < 0) /* new madwifi */
		p->phy_signal = ph->signal.data;
	else if (((int)ph->rssi.data) < 0) /* broadcom hack */
		p->phy_signal = ph->rssi.data;
	else /* assume hostap */
		p->phy_signal = ph->signal.data;

	p->phy_rate = ph->rate.data * 10;

	/* just in case...*/
	if (p->phy_rate == 0 || p->phy_rate > 1080) {
		/* assume min rate, guess mode from channel */
		LOG_DBG("Prism2: *** fixing wrong rate");
		if (ph->channel.data > 14)
			p->phy_rate = 120; /* 6 * 2 */
		else
			p->phy_rate = 20; /* 1 * 2 */
	}

	p->phy_rate_idx = wlan_rate_to_index(p->phy_rate);

	/* guess phy mode */
	if (ph->channel.data > 14)
		p->phy_flags |= PHY_FLAG_A;
	else
		p->phy_flags |= PHY_FLAG_G;
	/* always assume shortpre */
	p->phy_flags |= PHY_FLAG_SHORTPRE;

	LOG_DBG("Prism2: devname %s", ph->devname);
	LOG_DBG("Prism2: signal %d -> %d", ph->signal.data, p->phy_signal);
	LOG_DBG("Prism2: rate %d", ph->rate.data);
	LOG_DBG("Prism2: rssi %d", ph->rssi.data);

	return sizeof(wlan_ng_prism2_header);
}

static void get_radiotap_info(struct ieee80211_radiotap_iterator *iter, struct uwifi_packet* p)
{
	uint16_t x;
	signed char c;
	unsigned char known, flags, ht20, lgi;

	switch (iter->this_arg_index) {
	/* ignoring these */
	case IEEE80211_RADIOTAP_TSFT:
	case IEEE80211_RADIOTAP_FHSS:
	case IEEE80211_RADIOTAP_LOCK_QUALITY:
	case IEEE80211_RADIOTAP_TX_ATTENUATION:
	case IEEE80211_RADIOTAP_DB_TX_ATTENUATION:
	case IEEE80211_RADIOTAP_DBM_TX_POWER:
	case IEEE80211_RADIOTAP_RX_FLAGS:
	case IEEE80211_RADIOTAP_RTS_RETRIES:
	case IEEE80211_RADIOTAP_DATA_RETRIES:
	case IEEE80211_RADIOTAP_AMPDU_STATUS:
		break;
	case IEEE80211_RADIOTAP_TX_FLAGS:
		/* when TX flags are present we can conclude that a userspace
		 * program has injected this packet */
		p->phy_injected = true;
		break;
	case IEEE80211_RADIOTAP_FLAGS:
		/* short preamble */
		if (*iter->this_arg & IEEE80211_RADIOTAP_F_SHORTPRE) {
			p->phy_flags |= PHY_FLAG_SHORTPRE;
		}
		if (*iter->this_arg & IEEE80211_RADIOTAP_F_BADFCS) {
			p->phy_flags |= PHY_FLAG_BADFCS;
		}
		break;
	case IEEE80211_RADIOTAP_RATE:
		//TODO check!
		//printf("\trate: %lf\n", (double)*iter->this_arg/2);
		LOG_DBG("Radiotap: rate %0x", *iter->this_arg);
		p->phy_rate = (*iter->this_arg)*5; /* rate is in 500kbps */
		p->phy_rate_idx = wlan_rate_to_index(p->phy_rate);
		break;
#define IEEE80211_CHAN_A \
	(IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_G \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM)
	case IEEE80211_RADIOTAP_CHANNEL:
		/* channel & channel type */
		p->phy_freq = le16toh(*(uint16_t*)iter->this_arg);
		iter->this_arg = iter->this_arg + 2;
		x = le16toh(*(uint16_t*)iter->this_arg);
		if ((x & IEEE80211_CHAN_A) == IEEE80211_CHAN_A) {
			p->phy_flags |= PHY_FLAG_A;
		}
		else if ((x & IEEE80211_CHAN_G) == IEEE80211_CHAN_G) {
			p->phy_flags |= PHY_FLAG_G;
		}
		else if ((x & IEEE80211_CHAN_2GHZ) == IEEE80211_CHAN_2GHZ) {
			p->phy_flags |= PHY_FLAG_B;
		}
		break;
	case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
		c = *(signed char*)iter->this_arg;
		LOG_DBG("Radiotap: signal %ddBm", c);
		/* we get the signal per rx chain with newer drivers.
		 * save the highest value, but make sure we don't override
		 * with invalid values */
		if (c < 0 && (p->phy_signal == 0 || c > p->phy_signal))
			p->phy_signal = c;
		break;
	case IEEE80211_RADIOTAP_DBM_ANTNOISE:
		LOG_DBG("Radiotap: noise %ddBm", *(signed char*)iter->this_arg);
		// usually not present
		//p->phy_noise = *(signed char*)iter->this_arg;
		break;
	case IEEE80211_RADIOTAP_ANTENNA:
		LOG_DBG("Radiotap: antenna %d", *iter->this_arg);
		break;
	case IEEE80211_RADIOTAP_DB_ANTSIGNAL:
		LOG_DBG("Radiotap: signal %ddB (ref?)", *iter->this_arg);
		// usually not present
		//p->phy_snr = *iter->this_arg;
		break;
	case IEEE80211_RADIOTAP_DB_ANTNOISE:
		//printf("\tantnoise: %02d\n", *iter->this_arg);
		break;
	case IEEE80211_RADIOTAP_MCS:
		/* Ref http://www.radiotap.org/defined-fields/MCS */
		known = *iter->this_arg++;
		flags = *iter->this_arg++;
		if (known & IEEE80211_RADIOTAP_MCS_HAVE_BW)
			ht20 = (flags & IEEE80211_RADIOTAP_MCS_BW_MASK) == IEEE80211_RADIOTAP_MCS_BW_20;
		else
			ht20 = 1; /* assume HT20 if not present */

		if (known & IEEE80211_RADIOTAP_MCS_HAVE_GI)
			lgi = !(flags & IEEE80211_RADIOTAP_MCS_SGI);
		else
			lgi = 1; /* assume long GI if not present */

		//LOG_DBG(" %s %s", ht20 ? "HT20" : "HT40", lgi ? "LGI" : "SGI");

		p->phy_rate_idx = 12 + *iter->this_arg;
		p->phy_rate_flags = flags;
		p->phy_rate = wlan_ht_mcs_to_rate(*iter->this_arg, ht20, lgi);

		LOG_DBG("Radiotap: MCS rate %d ", p->phy_rate);
		break;
	default:
		LOG_DBG("Radiotap: UNKNOWN FIELD %d", iter->this_arg_index);
		break;
	}
}

int uwifi_parse_radiotap(unsigned char* buf, size_t len, struct uwifi_packet* p)
{
	struct ieee80211_radiotap_header* rh = (struct ieee80211_radiotap_header*)buf;
	struct ieee80211_radiotap_iterator iter;
	int rt_len = le16toh(rh->it_len);

	if (len < sizeof(struct ieee80211_radiotap_header))
		return -1;

	int err = ieee80211_radiotap_iterator_init(&iter, rh, rt_len, NULL);
	if (err) {
		LOG_DBG("Radiotap: MALFORMED HEADER (err %d)", err);
		return -1;
	}

	while (!(err = ieee80211_radiotap_iterator_next(&iter))) {
		if (iter.is_radiotap_ns) {
			get_radiotap_info(&iter, p);
		}
	}

	/* sanitize */
	if (p->phy_rate == 0 || p->phy_rate > 6000) {
		/* assume min rate for mode */
		LOG_DBG("Radiotap: *** fixing wrong rate");
		if (p->phy_flags & PHY_FLAG_A)
			p->phy_rate = 120; /* 6 * 2 */
		else if (p->phy_flags & PHY_FLAG_B)
			p->phy_rate = 20; /* 1 * 2 */
		else if (p->phy_flags & PHY_FLAG_G)
			p->phy_rate = 120; /* 6 * 2 */
		else
			p->phy_rate = 20;
	}

	LOG_DBG("Radiotap: RATE %.2f = idx %d", (float)p->phy_rate/10, p->phy_rate_idx);
	LOG_DBG("Radiotap: SIGNAL %d", p->phy_signal);

	if (p->phy_flags & PHY_FLAG_BADFCS) {
		/* we can't trust frames with a bad FCS - stop parsing */
		LOG_DBG("=== bad FCS, stop ===");
		return 0;
	} else {
		return rt_len;
	}
}

int uwifi_parse_raw(unsigned char* buf, size_t len, struct uwifi_packet* p, int arphdr)
{
	int ret;
	if (arphdr == ARPHRD_IEEE80211_PRISM) {
		ret = uwifi_parse_prism_header(buf, len, p);
		if (ret <= 0)
			return -1;
	} else if (arphdr == ARPHRD_IEEE80211_RADIOTAP) {
		ret = uwifi_parse_radiotap(buf, len, p);
		if (ret <= 0) /* 0: Bad FCS, allow packet but stop parsing */
			return 0;
	} else {
		return -1;
	}

	if ((size_t)ret >= len) {
		LOG_DBG("impossible len");
		return -1;
	}

	return uwifi_parse_80211_header(buf + ret, len - ret, p);
}

void uwifi_fixup_packet_channel(struct uwifi_packet* p, struct uwifi_interface* intf)
{
	int i = -1;

	/* get channel index for packet */
	if (p->phy_freq)
		i = uwifi_channel_idx_from_freq(&intf->channels, p->phy_freq);

	/* if not found from pkt, best guess from config but it might be
	 * unknown (-1) too */
	if (i < 0)
		p->pkt_chan_idx = intf->channel_idx;
	else
		p->pkt_chan_idx = i;

	/* wlan_channel is only known for beacons and probe response,
	 * otherwise we set it from the physical channel */
	if (p->wlan_channel == 0 && p->pkt_chan_idx >= 0)
		p->wlan_channel = uwifi_channel_get_chan(&intf->channels, p->pkt_chan_idx);

	/* if current channel is unknown (this is a mac80211 bug), guess it from
	 * the packet */
	if (intf->channel_idx < 0 && p->pkt_chan_idx >= 0)
		intf->channel_idx = p->pkt_chan_idx;
}
