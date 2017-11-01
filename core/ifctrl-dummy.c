/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015-2016 Bruno Randolf <br1@einfach.org>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include "ifctrl.h"
#include "platform.h"
#include "log.h"

bool ifctrl_init(void)
{
	return true;
};

void ifctrl_finish(void)
{
};

bool ifctrl_iwadd_monitor(const char *interface, const char *monitor_interface)
{
	LOG_ERR("add monitor: not implemented");
	return false;
};

bool ifctrl_iwdel(const char *interface)
{
	LOG_ERR("iwdel: not implemented");
	return false;
};

bool ifctrl_iwset_monitor(const char *interface)
{
	LOG_ERR("set monitor: not implemented");
	return false;
};

bool ifctrl_iwset_freq(const char *const interface,
		       unsigned int freq,
		       enum uwifi_chan_width width,
		       unsigned int center1)
{
	LOG_ERR("set freq: not implemented");
	return false;
};

bool ifctrl_iwget_interface_info(struct uwifi_interface* intf)
{
	LOG_ERR("get interface info: not implemented");
	return false;
};

bool ifctrl_iwget_freqlist(struct uwifi_interface* intf)
{
	LOG_ERR("get freqlist: not implemented");
	return false;
};

bool ifctrl_is_monitor(struct uwifi_interface* intf)
{
	return true;
};
