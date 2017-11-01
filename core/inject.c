#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <endian.h>
//#include <err.h>

#include "platform.h"
#include "inject.h"
#include "wlan80211.h"

static unsigned char supprates[] = {
	0x8c /* 6Mbps (basic)*/, 0x12 /* 9Mbps */, 0x98 /* 12Mbps (basic) */,
	0x24 /* 18Mbps */, 0xb0 /* 24Mbps (basic) */, 0x48 /* 36Mbps */,
	0x60 /* 48Mbps */, 0x6c /* 64Mbps */ };

int uwifi_create_beacon_probe_response(unsigned char* buf, bool probe_response,
				       unsigned char* sa, unsigned char* da,
				       unsigned char* bssid, char* essid,
				       uint64_t tsf, int channel, int bintval,
				       uint16_t seqno)
{
	int i = 0;
	int essidlen = strlen(essid);
	struct wlan_frame* header = (struct wlan_frame*)buf;
	struct wlan_frame_beacon* bcn  = (struct wlan_frame_beacon*)(buf + 24);

	header->fc = htole16(probe_response ? WLAN_FRAME_PROBE_RESP : WLAN_FRAME_BEACON);
	header->duration = htole16(0);

	if (probe_response)
		memcpy(header->addr1, da, 6);
	else
		memset(header->addr1, 0xff, 6);

	memcpy(header->addr2, sa, 6);
	memcpy(header->addr3, bssid, 6);
	header->seq = htole16(seqno) << 4;
	bcn->tsf = htole64(tsf);
	bcn->bintval = htole16(bintval);
	bcn->capab = htole16(WLAN_CAPAB_ESS);
	bcn->ie[i++] = WLAN_IE_ID_SSID;
	bcn->ie[i++] = essidlen;
	memcpy(&bcn->ie[i], essid, essidlen);
	i += essidlen;
	bcn->ie[i++] = WLAN_IE_ID_SUPP_RATES;
	bcn->ie[i++] = 8;
	memcpy(&bcn->ie[i], supprates, 8);
	i += 8;
	bcn->ie[i++] = WLAN_IE_ID_DSSS_PARAM;
	bcn->ie[i++] = 1;
	bcn->ie[i++] = channel;
	#if 0
	bcn->u.beacon.variable[i++] = WLAN_EID_ERP_INFO;
	bcn->u.beacon.variable[i++] = 1;
	bcn->u.beacon.variable[i++] = 0;
	bcn->u.beacon.variable[i++] = WLAN_EID_EXT_SUPP_RATES;
	bcn->u.beacon.variable[i++] = 1;
	bcn->u.beacon.variable[i++] = 0;
	#endif

	return 36 + i;
}

int uwifi_create_nulldata(unsigned char* buf, unsigned char* sa, unsigned char* da,
			  unsigned char* bssid, uint16_t seq)
{
	struct wlan_frame* header = (struct wlan_frame*)buf;

	header->fc = htole16(WLAN_FRAME_NULL | WLAN_FRAME_FC_TO_DS);
	header->duration = htole16(0);
	//TODO: order is different depending on direction!
	memcpy(header->addr1, bssid, 6);
	memcpy(header->addr2, sa, 6);
	memcpy(header->addr3, da, 6);
	header->seq = htole16(seq) << 4;
	return sizeof(struct wlan_frame);
}
