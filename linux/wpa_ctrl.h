/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (c) 2004-2007, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef UWIFI_WPA_CTRL_H_
#define UWIFI_WPA_CTRL_H_

/*
 * wpa_supplicant and hostapd control interface
 *
 * Taken from the hostapd source code and simplified
 */

struct wpa_ctrl* wpa_ctrl_open(const char* ctrl_path);

void wpa_ctrl_close(struct wpa_ctrl* ctrl);

int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd,
		     char *reply, size_t reply_len,
		     void (*msg_cb)(char *msg, size_t len));

#endif
