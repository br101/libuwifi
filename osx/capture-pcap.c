/* libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 * Copyright (C) 2007 Sven-Ola Tuecke
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <stdio.h>
#include <string.h>
#include <pcap.h>
#include <err.h>

#include "capture.h"
#include "util.h"
#include "log.h"

#define PCAP_TIMEOUT 200

static unsigned char* pcap_buffer;
static size_t pcap_bufsize;
static pcap_t *pcap_fp = NULL;

static void handler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes)
{
	*((int *)user) = h->len;
	if (pcap_bufsize < h->len) {
		LOG_ERR("ERROR: Buffer(%d) too small for %d bytes",
			 (int)pcap_bufsize, h->len);
		*((int *)user) = pcap_bufsize;
	}
	memmove(pcap_buffer, bytes, *((int *)user));
}

int packet_socket_open(char* devname)
{
	char error[PCAP_ERRBUF_SIZE];
	int ret;

	pcap_fp = pcap_create(devname, error);
	if (pcap_fp == NULL) {
		fprintf(stderr, "Couldn't create pcap: %s\n", error);
		return -1;
	}

	pcap_set_promisc(pcap_fp, 1);

#if defined(__APPLE__)
	if (pcap_can_set_rfmon(pcap_fp))
		pcap_set_rfmon(pcap_fp, 1);
	else
		err(1, "Couldn't activate monitor mode");
#endif

	ret = pcap_activate(pcap_fp);
	if (ret < 0) {
		fprintf(stderr, "Can't activate pcap: %d\n", ret);
		return -1;
	}

	return pcap_fileno(pcap_fp);
}

int device_get_hwinfo(__attribute__((unused)) int fd,
		      __attribute__((unused)) char* ifname)
{
	if (pcap_fp != NULL) {
		switch (pcap_datalink(pcap_fp)) {
		case DLT_IEEE802_11_RADIO:
			return 803;
		case DLT_PRISM_HEADER:
			return 802;
		default:
			return 801;
		}
	}
	return -1;
}

ssize_t capture_recv(__attribute__((unused)) int fd,
		unsigned char* buffer, size_t bufsize)
{
	int ret = 0;
	pcap_buffer = buffer;
	pcap_bufsize = bufsize;
	if (0 == pcap_dispatch(pcap_fp, 1, handler, (u_char *)&ret))
		return -1;
	return ret;
}


void capture_close(__attribute__((unused)) int fd)
{
	if (pcap_fp != NULL)
		pcap_close(pcap_fp);
}
