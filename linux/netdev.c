/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <err.h>

#include "netdev.h"
#include "platform.h"
#include "util.h"
#include "log.h"

int netdev_get_hwinfo(char* ifname)
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		LOG_ERR("Could not get arptype for '%s'", ifname);
		close(fd);
		return -1;
	}

	close(fd);
	LOG_DBG("ARPTYPE %d", ifr.ifr_hwaddr.sa_family);
	return ifr.ifr_hwaddr.sa_family;
}

bool netdev_get_mac_address(const char* ifname, unsigned char* mac)
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		LOG_ERR("MAC addr ioctl failed for '%s'", ifname);
		close(fd);
		return false;
	}

	close(fd);
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
	return true;
}

bool netdev_get_ip_address(const char* ifname, uint32_t* ip) {
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
		LOG_ERR("IP addr ioctl failed for '%s'", ifname);
		close(fd);
		return false;
	}

	struct sockaddr_in* sin = (struct sockaddr_in *)&ifr.ifr_addr;
	*ip = sin->sin_addr.s_addr;

	close(fd);
	return true;
}

bool netdev_set_ip_address(const char* ifname, uint32_t ip) {
	struct ifreq ifr;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	struct sockaddr_in* sai = (struct sockaddr_in *)&ifr.ifr_addr;
	sai->sin_family = AF_INET;
	sai->sin_addr.s_addr = ip;

	if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
		LOG_ERR("IP set addr ioctl failed for '%s'", ifname);
		close(fd);
		return false;
	}

	close(fd);
	return true;
}

bool netdev_set_up_promisc(const char *const ifname, bool up, bool promisc)
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		LOG_ERR("Could not get flags for %s", ifname);
		close(fd);
		return false;
	}

	if (up)
		ifr.ifr_flags |= IFF_UP;
	else
		ifr.ifr_flags &= ~IFF_UP;

	if (promisc)
		ifr.ifr_flags |= IFF_PROMISC;
	else
		ifr.ifr_flags &= ~IFF_PROMISC;

	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		LOG_ERR("Could not set flags for %s", ifname);
		close(fd);
		return false;
	}

	close(fd);
	return true;
}
