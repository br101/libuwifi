/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include "esp8266/platform.h"
#include "esp8266/esp_promisc.h"
#include "core/wlan_parser.h"

bool uwifi_esp_parse(uint8_t* buf, uint16_t len, struct uwifi_packet* pkt)
{
	struct sniffer_buf* sb;
	struct sniffer_buf2* sb2;
	struct RxControl* rxc = NULL;
	uint8_t* frame = NULL;
	int frame_len = 0;

	if (len == 12) {
		// unreliable, no MAC, no header length, no AMPDU
		// has: rssi, FEC_CODING
		rxc = (struct RxControl*)buf;
		DBG_PRINT("--- RX ???? RSSI %d\n", rxc->rssi);
		os_printf("~");
	}
	else if (len == 128) {
		// MGMT: 112 bytes of header & data
		sb2 = (struct sniffer_buf2*)buf;
		DBG_PRINT("--- RX MGMT cnt %d len %d RSSI %d\n",
			  sb2->cnt, sb2->len, sb2->rx_ctrl.rssi);
		os_printf(",");
		rxc = &sb2->rx_ctrl;
		frame = sb2->buf;
		frame_len = 112;
	}
	else {
		// DATA: 36 bytes of header
		sb = (struct sniffer_buf*)buf;
		DBG_PRINT("--- RX DATA cnt %d len %d RSSI %d\n",
			  sb->cnt, sb->lenseq[0].length, sb->rx_ctrl.rssi);
		os_printf(".");
		rxc = &sb->rx_ctrl;
		frame = sb->buf;
		frame_len = 36;
	}

	if (rxc != NULL && frame != NULL && frame_len != 0) {
		os_memset(pkt, 0, sizeof(struct uwifi_packet));
		pkt->phy_signal = rxc->rssi;

		return uwifi_parse_80211_header(frame, frame_len, pkt) >= 0;
	}
	return false;
}
