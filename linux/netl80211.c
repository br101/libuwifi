/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015 Tuomas Räsänen <tuomasjjrasanen@tjjr.fi>
 * Copyright (C) 2015-2016 Bruno Randolf <br1@einfach.org>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#define _GNU_SOURCE	/* necessary for libnl-tiny */

#include <stdbool.h>
#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>

#include <linux/nl80211.h>

#include "netl80211.h"

#ifndef NL80211_GENL_NAME
#define NL80211_GENL_NAME "nl80211"
#endif

struct nl_sock *nl_sock;
struct nl_sock *nl_event;
static int family_id;

bool nl80211_init(void)
{
	int err;

	nl_sock = nl_socket_alloc();
	if (!nl_sock) {
		fprintf(stderr, "failed to allocate netlink socket\n");
		goto out;
	}

	err = genl_connect(nl_sock);
	if (err) {
		nl_perror(err, "failed to make generic netlink connection");
		goto out;
	}

	family_id = genl_ctrl_resolve(nl_sock, NL80211_GENL_NAME);
	if (family_id < 0) {
		fprintf(stderr, "failed to find nl80211\n");
		goto out;
	}

	return true;
out:
	nl_socket_free(nl_sock);
	return false;
}

void nl80211_finish(void)
{
	nl_socket_free(nl_sock);
	nl_socket_free(nl_event);
}

bool nl80211_msg_prepare(struct nl_msg **const msgp,
			const enum nl80211_commands cmd,
			const char *const interface)
{
	struct nl_msg *msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return false;
	}

	if (!genlmsg_put(msg, 0, 0, family_id, 0, 0 /*flags*/, cmd, 0)) {
		fprintf(stderr, "failed to add generic netlink headers\n");
		goto nla_put_failure;
	}

	if (interface) { //TODO: PHY commands don't need interface name but wiphy index
		unsigned int if_index = if_nametoindex(interface);
		if (!if_index) {
			fprintf(stderr, "interface %s does not exist\n", interface);
			goto nla_put_failure;
		}
		NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, if_index);
	}

	*msgp = msg;
	return true;

nla_put_failure:
	nlmsg_free(msg);
	return false;
}

static int nl80211_ack_cb(__attribute__((unused)) struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0; /* set "ACK" */
	return NL_STOP;
}

static int nl80211_finish_cb(__attribute__((unused)) struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0; /* set "ACK" */
	return NL_SKIP;
}

static int nl80211_err_cb(__attribute__((unused)) struct sockaddr_nl *nla,
			  struct nlmsgerr *nlerr, __attribute__((unused)) void *arg)
{
	int *ret = arg;
	/* as we want to treat the error like other errors from recvmsg, and
	 * print it with nl_perror, we need to convert the error code to libnl
	 * error codes like it is done in the verbose error handler of libnl */
	*ret = -nl_syserr2nlerr(nlerr->error);
	return NL_STOP;
}

static int nl80211_default_cb(__attribute__((unused)) struct nl_msg *msg,
			      __attribute__((unused)) void *arg)
{
	return NL_SKIP;
}

/**
 * send message, free msg, receive reply and wait for ACK
 */
bool nl80211_send_recv(struct nl_sock *const sock, struct nl_msg *const msg,
		       nl_recvmsg_msg_cb_t cb_func, void* cb_arg)
{
	int err;
	struct nl_cb *cb;

	/* set up callback */
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callback\n");
		return false;
	}

	if (cb_func != NULL)
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, cb_func, cb_arg);
	else
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_default_cb, NULL);

	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, nl80211_ack_cb, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, nl80211_finish_cb, &err);
	nl_cb_err(cb, NL_CB_CUSTOM, nl80211_err_cb, &err);

	err = nl_send_auto_complete(sock, msg);
	nlmsg_free(msg);

	if (err <= 0) {
		nl_perror(err, "failed to send netlink message");
		return false;
	}

	/*
	 * wait for reply message *and* ACK, or error
	 *
	 * Note that err is set by the handlers above. This is done because we
	 * receive two netlink messages, one with the result (and handled by
	 * cb_func) and another one with ACK. We are only done when we received
	 * the ACK or an error!
	 */
	err = 1;
	while (err > 0)
		nl_recvmsgs(sock, cb);

	nl_cb_put(cb);

	if (err < 0) {
		nl_perror(err, "nl80211 message failed");
		return false;
	}

	return true;
}

/**
 * send message, free msg and wait for ACK
 */
bool nl80211_send(struct nl_sock *const sock, struct nl_msg *const msg)
{
	return nl80211_send_recv(sock, msg, NULL, NULL); /* frees msg */
}

struct nlattr** nl80211_parse(struct nl_msg *msg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	static struct nlattr *attr[NL80211_ATTR_MAX + 1];

	nla_parse(attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
	          genlmsg_attrlen(gnlh, 0), NULL);

	return attr;
}

/*** somewhat inefficient way of resolving multicast group ids ***/

struct nl_group_id {
	const char *group;
	int id;
};

static int nl_family_group_id_cb(struct nl_msg *msg, void *arg)
{
	struct nl_group_id *grp = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
			  nla_data(mcgrp), nla_len(mcgrp), NULL);

		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
			    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}

int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
	struct nl_group_id grp = {
		.group = group,
		.id = -ENOENT,
	};

	struct nl_msg* msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return -ENOMEM;
	}

	int ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	if (nl80211_send_recv(sock, msg, nl_family_group_id_cb, &grp))
		return grp.id;

nla_put_failure:
	nlmsg_free(msg);
	return -1;
}
