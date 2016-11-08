#include <stdint.h>
#include <endian.h>
#include <radiotap.h>

#include "inject.h"

/*
 * mac80211 currently interprets only the following fields for injected frames:
 *
 * IEEE80211_RADIOTAP_FLAGS
 *	IEEE80211_RADIOTAP_F_FCS: FCS will be r*emoved and recalculated
 *	IEEE80211_RADIOTAP_F_WEP: frame will be encrypted if key available
 *	IEEE80211_RADIOTAP_F_FRAG: frame will be fragmented if longer than the
 *				   current fragmentation threshold.
 *
 * IEEE80211_RADIOTAP_TX_FLAGS
 *	IEEE80211_RADIOTAP_F_TX_NOACK: frame should be sent without waiting for
 *				       an ACK even if it is a unicast frame
 *
 * Note: following fileds are set, but currently NOT interepreted by mac80211:
 * 	IEEE80211_RADIOTAP_RATE
 *	IEEE80211_RADIOTAP_CHANNEL
 *	IEEE80211_RADIOTAP_DATA_RETRIES
 */
struct inject_radiotap_header {
	uint8_t		it_version;
	uint8_t		it_pad;
	uint16_t	it_len;
	uint32_t	it_present;
	uint8_t		rt_flags;
	uint8_t 	rt_rate;
	uint16_t	rt_chan_freq;
	uint16_t	rt_chan_flags;
	uint16_t	rt_txflags;
	uint8_t		rt_retry;
} __attribute__((__packed__));

#define INJECT_RTAP_PRESENT (1 << IEEE80211_RADIOTAP_FLAGS | \
			     1 << IEEE80211_RADIOTAP_RATE  | \
			     1 << IEEE80211_RADIOTAP_CHANNEL | \
			     1 << IEEE80211_RADIOTAP_TX_FLAGS | \
			     1 << IEEE80211_RADIOTAP_DATA_RETRIES)

#define IEEE80211_RADIOTAP_F_TX_NOACK  0x0008  /* don't expect an ack */

int uwifi_create_radiotap_header(unsigned char* buf, int freq, bool ack)
{
	struct inject_radiotap_header* rtaphdr;

	rtaphdr = (struct inject_radiotap_header *)buf;
	rtaphdr->it_version = 0;
	rtaphdr->it_pad = 0;
	rtaphdr->it_len = htole16(sizeof(struct inject_radiotap_header));
	rtaphdr->it_present = htole32(INJECT_RTAP_PRESENT);
	rtaphdr->rt_flags = 0;
	/*
	 * For 802.11b/g channels, send at 1 Mbits
	 * For 802.11a channels, send at 6 Mbits
	 */
	if (freq <= 2484)
		rtaphdr->rt_rate = 1*2; /* 1 Mbits */
	else
		rtaphdr->rt_rate = 6*2; /* 6 Mbits */
	rtaphdr->rt_chan_freq = htole16(freq);
	rtaphdr->rt_chan_flags = 0;
	rtaphdr->rt_txflags = htole16(ack ? 0 : IEEE80211_RADIOTAP_F_TX_NOACK);
	rtaphdr->rt_retry = 0;

	return sizeof(struct inject_radiotap_header);
}
