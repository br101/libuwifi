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

bool ifctrl_iwadd_monitor(__attribute__((unused)) const char *interface,
			  __attribute__((unused))const char *monitor_interface)
{
	LOG_ERR("add monitor: not implemented");
	return false;
};

bool ifctrl_iwdel(__attribute__((unused)) const char *interface)
{
	LOG_ERR("iwdel: not implemented");
	return false;
};

bool ifctrl_iwset_monitor(__attribute__((unused)) const char *interface)
{
	LOG_ERR("set monitor: not implemented");
	return false;
};

bool ifctrl_iwset_freq(__attribute__((unused)) const char *const interface,
		       __attribute__((unused)) unsigned int freq,
		       __attribute__((unused)) enum uwifi_chan_width width,
		       __attribute__((unused)) unsigned int center1)
{
	LOG_ERR("set freq: not implemented");
	return false;
};

bool ifctrl_iwget_interface_info(__attribute__((unused)) const char *interface)
{
	LOG_ERR("get interface info: not implemented");
	return false;
};

bool ifctrl_iwget_freqlist(__attribute__((unused)) int phy,
			   __attribute__((unused)) struct uwifi_channels* channels)
{
	LOG_ERR("get freqlist: not implemented");
	return false;
};

bool ifctrl_is_monitor(void)
{
	return true;
};
