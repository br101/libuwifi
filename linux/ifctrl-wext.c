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
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

#include "util.h"
#include "platform.h"
#include "channel.h"
#include "conf.h"
#include "log.h"

static int wext_fd;

static bool wext_set_freq(int fd, const char* devname, int freq)
{
	struct iwreq iwr;

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, devname, IFNAMSIZ - 1);
	iwr.ifr_name[IFNAMSIZ - 1] = '\0';
	freq *= 100000;
	iwr.u.freq.m = freq;
	iwr.u.freq.e = 1;

	if (ioctl(fd, SIOCSIWFREQ, &iwr) < 0) {
		LOG_ERR("WEXT could not set channel");
		return false;
	}
	return true;
}

static int wext_get_freq(int fd, const char* devname)
{
	struct iwreq iwr;

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, devname, IFNAMSIZ - 1);
	iwr.ifr_name[IFNAMSIZ - 1] = '\0';

	if (ioctl(fd, SIOCGIWFREQ, &iwr) < 0)
		return 0;

	LOG_DBG("FREQ %d %d", iwr.u.freq.m, iwr.u.freq.e);

	return iwr.u.freq.m;
}

static int wext_get_channels(int fd, const char* devname,
			     struct uwifi_channels* channels)
{
	struct iwreq iwr;
	struct iw_range range;
	int i;
	int band0cnt = 0;
	int band1cnt = 0;

	memset(&iwr, 0, sizeof(iwr));
	memset(&range, 0, sizeof(range));

	strncpy(iwr.ifr_name, devname, IFNAMSIZ - 1);
	iwr.ifr_name[IFNAMSIZ - 1] = '\0';
	iwr.u.data.pointer = (caddr_t) &range;
	iwr.u.data.length = sizeof(range);
	iwr.u.data.flags = 0;

	if (ioctl(fd, SIOCGIWRANGE, &iwr) < 0) {
		LOG_ERR("WEXT get channel list");
		return 0;
	}

	if (range.we_version_compiled < 16) {
		LOG_ERR("WEXT version %d too old to get channels",
			 range.we_version_compiled);
		return 0;
	}

	for (i = 0; i < range.num_frequency && i < MAX_CHANNELS; i++) {
		LOG_DBG("  Channel %.2d: %dMHz", range.freq[i].i, range.freq[i].m);
		channels->chan[i].chan = range.freq[i].i;
		/* different drivers return different frequencies
		 * (e.g. ipw2200 vs mac80211) try to fix them up here */
		if (range.freq[i].m > 100000000)
			channels->chan[i].freq = range.freq[i].m / 100000;
		else
			channels->chan[i].freq = range.freq[i].m;
		if (channels->chan[i].freq <= 2500)
			band0cnt++;
		else
			band1cnt++;
	}
	channels->num_channels = i;
	channels->num_bands = band1cnt > 0 ? 2 : 1;
	channels->band[0].num_channels = band0cnt;
	channels->band[1].num_channels = band1cnt;
	return i;
}

/*
 * ifctrl.h implementation
 */

bool ifctrl_init(void)
{
	wext_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (wext_fd < 0)
		return false;
	return true;
};

void ifctrl_finish(void)
{
	if (wext_fd >= 0)
		close(wext_fd);
};

bool ifctrl_iwadd_monitor(__attribute__((unused)) const char *interface,
			  __attribute__((unused)) const char *monitor_interface)
{
	LOG_ERR("add monitor: not supported with WEXT");
	return false;
}

bool ifctrl_iwdel(__attribute__((unused)) const char *interface)
{
	LOG_ERR("del: not supported with WEXT");
	return false;
}

bool ifctrl_iwset_monitor(__attribute__((unused)) const char *interface)
{
	LOG_ERR("set monitor: not supported with WEXT");
	return false;
}


bool ifctrl_iwset_freq(const char *const interface,
		       unsigned int freq,
		       __attribute__((unused)) enum uwifi_chan_width width,
		       __attribute__((unused)) unsigned int center1)
{
	if (wext_set_freq(wext_fd, interface, freq))
		return true;
	return false;
}

bool ifctrl_iwget_interface_info(struct uwifi_interface* intf)
{
	intf->if_freq = wext_get_freq(wext_fd, intf->ifname);
	if (intf->if_freq == 0)
		return false;
	return true;
}

bool ifctrl_iwget_freqlist(struct uwifi_interface* intf)
{
	if (wext_get_channels(wext_fd, intf->ifname, &intf->channels))
		return true;
	return false;
}

bool ifctrl_is_monitor(__attribute__((unused)) struct uwifi_interface* intf)
{
	return true; /* assume yes */
}
