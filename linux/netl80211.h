/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015 Tuomas Räsänen <tuomasjjrasanen@tjjr.fi>
 * Copyright (C) 2015-2016 Bruno Randolf <br1@einfach.org>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef UWIFI_NETLINK_H_
#define UWIFI_NETLINK_H_

#include <stdbool.h>
#include <netlink/handlers.h>
#include <linux/nl80211.h>

extern struct nl_sock *nl_sock;
extern struct nl_sock *nl_event;

bool nl80211_init(void);

void nl80211_finish(void);

bool nl80211_msg_prepare(struct nl_msg **const msgp,
			const enum nl80211_commands cmd,
			const char *const interface);

bool nl80211_send_recv(struct nl_sock *const sock, struct nl_msg *const msg,
		       nl_recvmsg_msg_cb_t cb_func, void* cb_arg);

bool nl80211_send(struct nl_sock *const sock, struct nl_msg *const msg);

struct nlattr** nl80211_parse(struct nl_msg *msg);

int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group);

#endif
