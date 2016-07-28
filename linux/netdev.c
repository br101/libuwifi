#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <err.h>

#include "netdev.h"
#include "platform.h"
#include "util.h"

int netdev_get_hwinfo(char* ifname)
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		printlog(LOG_ERR, "Could not get arptype for '%s'", ifname);
		close(fd);
		return -1;
	}

	close(fd);
	DBG_PRINT("ARPTYPE %d\n", ifr.ifr_hwaddr.sa_family);
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
		printlog(LOG_ERR, "MAC addr ioctl failed for '%s'", ifname);
		close(fd);
		return false;
	}

	close(fd);
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
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
		printlog(LOG_ERR, "Could not get flags for %s", ifname);
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
		printlog(LOG_ERR, "Could not set flags for %s", ifname);
		close(fd);
		return false;
	}

	close(fd);
	return true;
}
