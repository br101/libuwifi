/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_ESP_PROMISC_H_
#define _UWIFI_ESP_PROMISC_H_

/* Promiscous callback structures, see ESP manual */

struct RxControl {
	signed rssi:8;
	unsigned rate:4;
	unsigned is_group:1;
	unsigned:1;
	unsigned sig_mode:2;		// 0: is 11n packet; 1: is not 11n
	unsigned legacy_length:12;	// if not 11n, shows length of packet
	unsigned damatch0:1;
	unsigned damatch1:1;
	unsigned bssidmatch0:1;
	unsigned bssidmatch1:1;
	unsigned MCS:7;			// if 11n, shows modulation and code used (range from 0-76)
	unsigned CWB:1;			// if 11n, shows if HT40 or not
	unsigned HT_length:16;		// if 11n, shows length of packet
	unsigned Smoothing:1;
	unsigned Not_Sounding:1;
	unsigned:1;
	unsigned Aggregation:1;
	unsigned STBC:2;
	unsigned FEC_CODING:1;		// if 11n, shows if LDPC or not
	unsigned SGI:1;
	unsigned rxend_state:8;
	unsigned ampdu_cnt:8;
	unsigned channel:4;
	unsigned:12;
};

struct LenSeq {
	uint16_t length;
	uint16_t seq;		// high 12bits are seqno, low 14 bits are fragment no
	uint8_t  address3[6];	// third address in packet
};

struct sniffer_buf {
	struct RxControl rx_ctrl;
	uint8_t buf[36];		// header of ieee80211 packet
	uint16_t cnt;			// number count?
	struct LenSeq lenseq[1];	// length and seq
};

struct sniffer_buf2 {
	struct RxControl rx_ctrl;
	uint8_t buf[112];
	uint16_t cnt;
	uint16_t len;			// length
};

struct uwifi_packet;

bool uwifi_esp_parse(uint8_t* buf, uint16_t len, struct uwifi_packet* pkt);

#endif
