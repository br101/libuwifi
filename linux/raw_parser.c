#include "prism_header.h"
#include "radiotap.h"
#include "radiotap_iter.h"
#include "wlan80211.h"
#include "wlan_util.h"
#include "util.h"
#include "wlan_parser.h"
#include "platform.h"
#include "phy_info.h"
#include "conf.h"
#include "raw_parser.h"

/* return consumed length or -1 on error */
int parse_prism_header(unsigned char* buf, int len, struct packet_info* p)
{
	wlan_ng_prism2_header* ph;

	DEBUG("PRISM2 HEADER\n");

	if (len > 0 && (size_t)len < sizeof(wlan_ng_prism2_header))
		return -1;

	ph = (wlan_ng_prism2_header*)buf;

	/*
	 * different drivers report S/N and rssi values differently
	*/
	if (((int)ph->noise.data) < 0) {
		/* new madwifi */
		p->phy_signal = ph->signal.data;
	}
	else if (((int)ph->rssi.data) < 0) {
		/* broadcom hack */
		p->phy_signal = ph->rssi.data;
	}
	else {
		/* assume hostap */
		p->phy_signal = ph->signal.data;
	}

	p->phy_rate = ph->rate.data * 10;

	/* just in case...*/
	if (p->phy_rate == 0 || p->phy_rate > 1080) {
		/* assume min rate, guess mode from channel */
		DEBUG("*** fixing wrong rate\n");
		if (ph->channel.data > 14)
			p->phy_rate = 120; /* 6 * 2 */
		else
			p->phy_rate = 20; /* 1 * 2 */
	}

	p->phy_rate_idx = rate_to_index(p->phy_rate);

	/* guess phy mode */
	if (ph->channel.data > 14)
		p->phy_flags |= PHY_FLAG_A;
	else
		p->phy_flags |= PHY_FLAG_G;
	/* always assume shortpre */
	p->phy_flags |= PHY_FLAG_SHORTPRE;

	DEBUG("devname: %s\n", ph->devname);
	DEBUG("signal: %d -> %d\n", ph->signal.data, p->phy_signal);
	DEBUG("rate: %d\n", ph->rate.data);
	DEBUG("rssi: %d\n", ph->rssi.data);

	return sizeof(wlan_ng_prism2_header);
}

static void get_radiotap_info(struct ieee80211_radiotap_iterator *iter, struct packet_info* p)
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
	case IEEE80211_RADIOTAP_TX_FLAGS:
	case IEEE80211_RADIOTAP_RX_FLAGS:
	case IEEE80211_RADIOTAP_RTS_RETRIES:
	case IEEE80211_RADIOTAP_DATA_RETRIES:
	case IEEE80211_RADIOTAP_AMPDU_STATUS:
		break;
	case IEEE80211_RADIOTAP_FLAGS:
		/* short preamble */
		DEBUG("[flags %0x", *iter->this_arg);
		if (*iter->this_arg & IEEE80211_RADIOTAP_F_SHORTPRE) {
			p->phy_flags |= PHY_FLAG_SHORTPRE;
			DEBUG(" shortpre");
		}
		if (*iter->this_arg & IEEE80211_RADIOTAP_F_BADFCS) {
			p->phy_flags |= PHY_FLAG_BADFCS;
			DEBUG(" badfcs");
		}
		DEBUG("]");
		break;
	case IEEE80211_RADIOTAP_RATE:
		//TODO check!
		//printf("\trate: %lf\n", (double)*iter->this_arg/2);
		DEBUG("[rate %0x]", *iter->this_arg);
		p->phy_rate = (*iter->this_arg)*5; /* rate is in 500kbps */
		p->phy_rate_idx = rate_to_index(p->phy_rate);
		break;
#define IEEE80211_CHAN_A \
	(IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM)
#define IEEE80211_CHAN_G \
	(IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM)
	case IEEE80211_RADIOTAP_CHANNEL:
		/* channel & channel type */
		p->phy_freq = le16toh(*(uint16_t*)iter->this_arg);
		DEBUG("[freq %d", p->phy_freq);
		iter->this_arg = iter->this_arg + 2;
		x = le16toh(*(uint16_t*)iter->this_arg);
		if ((x & IEEE80211_CHAN_A) == IEEE80211_CHAN_A) {
			p->phy_flags |= PHY_FLAG_A;
			DEBUG("A]");
		}
		else if ((x & IEEE80211_CHAN_G) == IEEE80211_CHAN_G) {
			p->phy_flags |= PHY_FLAG_G;
			DEBUG("G]");
		}
		else if ((x & IEEE80211_CHAN_2GHZ) == IEEE80211_CHAN_2GHZ) {
			p->phy_flags |= PHY_FLAG_B;
			DEBUG("B]");
		}
		break;
	case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
		c = *(signed char*)iter->this_arg;
		DEBUG("[sig %0d]", c);
		/* we get the signal per rx chain with newer drivers.
		 * save the highest value, but make sure we don't override
		 * with invalid values */
		if (c < 0 && (p->phy_signal == 0 || c > p->phy_signal))
			p->phy_signal = c;
		break;
	case IEEE80211_RADIOTAP_DBM_ANTNOISE:
		DEBUG("[noi %0x]", *(signed char*)iter->this_arg);
		// usually not present
		//p->phy_noise = *(signed char*)iter->this_arg;
		break;
	case IEEE80211_RADIOTAP_ANTENNA:
		DEBUG("[ant %0x]", *iter->this_arg);
		break;
	case IEEE80211_RADIOTAP_DB_ANTSIGNAL:
		DEBUG("[snr %0x]", *iter->this_arg);
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
		DEBUG("[MCS known %0x flags %0x index %0x]", known, flags, *iter->this_arg);
		if (known & IEEE80211_RADIOTAP_MCS_HAVE_BW)
			ht20 = (flags & IEEE80211_RADIOTAP_MCS_BW_MASK) == IEEE80211_RADIOTAP_MCS_BW_20;
		else
			ht20 = 1; /* assume HT20 if not present */

		if (known & IEEE80211_RADIOTAP_MCS_HAVE_GI)
			lgi = !(flags & IEEE80211_RADIOTAP_MCS_SGI);
		else
			lgi = 1; /* assume long GI if not present */

		DEBUG(" %s %s", ht20 ? "HT20" : "HT40", lgi ? "LGI" : "SGI");

		p->phy_rate_idx = 12 + *iter->this_arg;
		p->phy_rate_flags = flags;
		p->phy_rate = mcs_index_to_rate(*iter->this_arg, ht20, lgi);

		DEBUG(" RATE %d ", p->phy_rate);
		break;
	default:
		printlog("UNKNOWN RADIOTAP field %d", iter->this_arg_index);
		break;
	}
}

/* return consumed length, 0 for bad FCS, -1 on error */
int parse_radiotap_header(unsigned char* buf, size_t len, struct packet_info* p)
{
	struct ieee80211_radiotap_header* rh;
	struct ieee80211_radiotap_iterator iter;
	int err, rt_len;

	if (len < sizeof(struct ieee80211_radiotap_header))
		return -1;

	rh = (struct ieee80211_radiotap_header*)buf;
	rt_len = le16toh(rh->it_len);

	err = ieee80211_radiotap_iterator_init(&iter, rh, rt_len, NULL);
	if (err) {
		DEBUG("malformed radiotap header (init returns %d)\n", err);
		return -1;
	}

	DEBUG("Radiotap: ");
	while (!(err = ieee80211_radiotap_iterator_next(&iter))) {
		if (iter.is_radiotap_ns) {
			get_radiotap_info(&iter, p);
		}
	}

	DEBUG("\nSIG %d", p->phy_signal);

	/* sanitize */
	if (p->phy_rate == 0 || p->phy_rate > 6000) {
		/* assume min rate for mode */
		DEBUG("*** fixing wrong rate\n");
		if (p->phy_flags & PHY_FLAG_A)
			p->phy_rate = 120; /* 6 * 2 */
		else if (p->phy_flags & PHY_FLAG_B)
			p->phy_rate = 20; /* 1 * 2 */
		else if (p->phy_flags & PHY_FLAG_G)
			p->phy_rate = 120; /* 6 * 2 */
		else
			p->phy_rate = 20;
	}

	DEBUG("\nrate: %.2f = idx %d\n", (float)p->phy_rate/10, p->phy_rate_idx);
	DEBUG("signal: %d\n", p->phy_signal);

	if (p->phy_flags & PHY_FLAG_BADFCS) {
		/* we can't trust frames with a bad FCS - stop parsing */
		DEBUG("=== bad FCS, stop ===\n");
		return 0;
	} else {
		return rt_len;
	}
}

/* return rest of packet length (may be 0) or negative value on error */
int wlan_parse_packet(unsigned char* buf, size_t len, struct packet_info* p, int arphdr)
{
	int ret;
	if (arphdr == ARPHRD_IEEE80211_PRISM) {
		ret = parse_prism_header(buf, len, p);
		if (ret <= 0)
			return -1;
	} else if (arphdr == ARPHRD_IEEE80211_RADIOTAP) {
		ret = parse_radiotap_header(buf, len, p);
		if (ret <= 0) /* 0: Bad FCS, allow packet but stop parsing */
			return 0;
	} else {
		return -1;
	}

	if ((size_t)ret >= len) {
		DEBUG("impossible radiotap len");
		return -1;
	}

	DEBUG("before parse 80211 len: %zd\n", len - ret);
	return parse_80211_header(buf + ret, len - ret, p);
}

void fixup_packet_channel(struct packet_info* p, struct wlan_interface* intf)
{
	int i = -1;

	/* get channel index for packet */
	if (p->phy_freq) {
		i = channel_find_index_from_freq(&intf->channels, p->phy_freq);
	}

	/* if not found from pkt, best guess from config but it might be
	 * unknown (-1) too */
	if (i < 0)
		p->pkt_chan_idx = intf->channel_idx;
	else
		p->pkt_chan_idx = i;

	/* wlan_channel is only known for beacons and probe response,
	 * otherwise we set it from the physical channel */
	if (p->wlan_channel == 0 && p->pkt_chan_idx >= 0)
		p->wlan_channel = channel_get_chan(&intf->channels, p->pkt_chan_idx);

	/* if current channel is unknown (this is a mac80211 bug), guess it from
	 * the packet */
	if (intf->channel_idx < 0 && p->pkt_chan_idx >= 0)
		intf->channel_idx = p->pkt_chan_idx;
}
