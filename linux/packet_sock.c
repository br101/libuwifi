/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <err.h>

#include "packet_sock.h"
#include "util.h"
#include "platform.h"
#include "log.h"

void socket_set_receive_buffer(int fd, int sockbufsize)
{
	int ret;

	/* the maximum allowed value is set by the rmem_max sysctl */
	FILE* PF = fopen("/proc/sys/net/core/rmem_max", "w");
	fprintf(PF, "%d", sockbufsize);
	fclose(PF);

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, sizeof(sockbufsize));
	if (ret != 0)
		err(1, "setsockopt failed");

#if DEBUG
	socklen_t size = sizeof(sockbufsize);
	sockbufsize = 0;
	ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, &size);
	if (ret != 0)
		err(1, "getsockopt failed");
	LOG_DBG("socket receive buffer size %d", sockbufsize);
#endif
}

int packet_socket_open(char* devname)
{
	struct sockaddr_ll sall;

	int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		LOG_ERR("Could not create packet socket! Please run as root!");
		exit(1);
	}

	/* bind only to one interface */
	unsigned int ifindex = if_nametoindex(devname);
	if (ifindex == 0)
		return -1;

	memset(&sall, 0, sizeof(struct sockaddr_ll));
	sall.sll_ifindex = ifindex;
	sall.sll_family = AF_PACKET;
	sall.sll_protocol = htons(ETH_P_ALL);

	int ret = bind(fd, (struct sockaddr*)&sall, sizeof(sall));
	if (ret != 0)
		err(1, "bind failed");

	return fd;
}

ssize_t packet_socket_recv(int fd, unsigned char* buffer, size_t bufsize)
{
	return recv(fd, buffer, bufsize, MSG_DONTWAIT);
}
