/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015 Tuomas Räsänen <tuomasjjrasanen@tjjr.fi>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#define _GNU_SOURCE	/* necessary for libnl-tiny */

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

#include "ifctrl.h"
#include "wlan_util.h"
#include "conf.h"

#ifndef NL80211_GENL_NAME
#define NL80211_GENL_NAME "nl80211"
#endif

static struct nl_sock *sock = NULL;
static struct nl_cache *cache = NULL;
static struct genl_family *family = NULL;

static bool nl80211_init(void)
{
	int err;

	sock = nl_socket_alloc();
	if (!sock) {
		fprintf(stderr, "failed to allocate netlink socket\n");
		goto out;
	}

	err = genl_connect(sock);
	if (err) {
		nl_perror(err, "failed to make generic netlink connection");
		goto out;
	}

	err = genl_ctrl_alloc_cache(sock, &cache);
	if (err) {
		nl_perror(err, "failed to allocate netlink controller cache");
		goto out;
	}

	family = genl_ctrl_search_by_name(cache, NL80211_GENL_NAME);
	if (!family) {
		fprintf(stderr, "failed to find nl80211\n");
		goto out;
	}

	return true;
out:
	genl_family_put(family);
	nl_cache_free(cache);
	nl_socket_free(sock);
	return false;
}

static void nl80211_finish(void)
{
	nl_socket_free(sock);
	genl_family_put(family);
	nl_cache_free(cache);
}

static bool nl80211_msg_prepare(struct nl_msg **const msgp,
				const enum nl80211_commands cmd,
				const char *const interface)
{
	struct nl_msg *msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return false;
	}

	if (!genlmsg_put(msg, 0, 0, genl_family_get_id(family), 0, 0 /*flags*/, cmd, 0)) {
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
static bool nl80211_send_recv(struct nl_sock *const sock, struct nl_msg *const msg,
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
static bool nl80211_send(struct nl_sock *const sock, struct nl_msg *const msg)
{
	return nl80211_send_recv(sock, msg, NULL, NULL); /* frees msg */
}

static struct nlattr** nl80211_parse(struct nl_msg *msg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	static struct nlattr *attr[NL80211_ATTR_MAX + 1];

	nla_parse(attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
	          genlmsg_attrlen(gnlh, 0), NULL);

	return attr;
}

/*
 * ifctrl interface
 */

bool ifctrl_init(void)
{
	return nl80211_init();
}

void ifctrl_finish(void)
{
	nl80211_finish();
}

bool ifctrl_iwadd_monitor(const char *const interface,
			  const char *const monitor_interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_NEW_INTERFACE, interface))
		return false;

	NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, monitor_interface);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);

	return nl80211_send(sock, msg); /* frees msg */

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

bool ifctrl_iwdel(const char *const interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_DEL_INTERFACE, interface))
		return false;

	return nl80211_send(sock, msg); /* frees msg */
}

bool ifctrl_iwset_monitor(const char *const interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_SET_INTERFACE, interface))
		return false;

	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);
	return nl80211_send(sock, msg); /* frees msg */

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

bool ifctrl_iw_disconnect(const char *const interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_DISCONNECT, interface))
		return false;

	return nl80211_send(sock, msg); /* frees msg */
}

bool ifctrl_iw_connect(const char *const interface, const char* essid, int freq,
		       const unsigned char* bssid)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_CONNECT, interface))
		return false;

	NLA_PUT(msg, NL80211_ATTR_SSID, strlen(essid), essid);

	if (freq)
		NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);

	if (bssid)
		NLA_PUT(msg, NL80211_ATTR_MAC, 6, bssid);

	return nl80211_send(sock, msg); /* frees msg */

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

bool ifctrl_iwset_freq(const char *const interface, unsigned int freq,
		       enum uwifi_chan_width width,
		       unsigned int center1)
{
	struct nl_msg *msg;
	int nl_width = NL80211_CHAN_WIDTH_20_NOHT;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_SET_CHANNEL, interface))
		return false;

	switch (width) {
		case CHAN_WIDTH_UNSPEC:
		case CHAN_WIDTH_20_NOHT:
			nl_width = NL80211_CHAN_WIDTH_20_NOHT; break;
		case CHAN_WIDTH_20:
			nl_width = NL80211_CHAN_WIDTH_20; break;
		case CHAN_WIDTH_40:
			nl_width = NL80211_CHAN_WIDTH_40; break;
		case CHAN_WIDTH_80:
			nl_width = NL80211_CHAN_WIDTH_80; break;
		case CHAN_WIDTH_160:
			nl_width = NL80211_CHAN_WIDTH_160; break;
		case CHAN_WIDTH_8080:
			nl_width = NL80211_CHAN_WIDTH_80P80; break;
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY_FREQ, freq);
	NLA_PUT_U32(msg, NL80211_ATTR_CHANNEL_WIDTH, nl_width);

	if (center1)
		NLA_PUT_U32(msg, NL80211_ATTR_CENTER_FREQ1, center1);

	return nl80211_send(sock, msg); /* frees msg */

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

static int nl80211_get_interface_info_cb(struct nl_msg *msg, void *arg)
{
	struct uwifi_interface* intf = arg;
	struct nlattr **tb = nl80211_parse(msg);

	if (tb[NL80211_ATTR_WIPHY_FREQ])
		intf->if_freq = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);

	if (tb[NL80211_ATTR_CHANNEL_WIDTH]) {
		int nlw = nla_get_u32(tb[NL80211_ATTR_CHANNEL_WIDTH]);
		switch (nlw) {
			case NL80211_CHAN_WIDTH_20_NOHT:
				intf->channel_width = CHAN_WIDTH_20_NOHT; break;
			case NL80211_CHAN_WIDTH_20:
				intf->channel_width = CHAN_WIDTH_20; break;
			case NL80211_CHAN_WIDTH_40:
				intf->channel_width = CHAN_WIDTH_40; break;
			case NL80211_CHAN_WIDTH_80:
				intf->channel_width = CHAN_WIDTH_80; break;
			case NL80211_CHAN_WIDTH_160:
				intf->channel_width = CHAN_WIDTH_160; break;
			case NL80211_CHAN_WIDTH_80P80:
				intf->channel_width = CHAN_WIDTH_8080; break;
			default:
				intf->channel_width = CHAN_WIDTH_UNSPEC; break;
		}
	}

	if (intf->channel_width == CHAN_WIDTH_40 && tb[NL80211_ATTR_CENTER_FREQ1]) {
		unsigned int center1 = nla_get_u32(tb[NL80211_ATTR_CENTER_FREQ1]);
		intf->channel_ht40plus = center1 > intf->if_freq;
	}

	if (tb[NL80211_ATTR_IFTYPE])
		intf->if_type = nla_get_u32(tb[NL80211_ATTR_IFTYPE]);

	if (tb[NL80211_ATTR_WIPHY])
		intf->if_phy = nla_get_u32(tb[NL80211_ATTR_WIPHY]);

	return NL_SKIP;
}

bool ifctrl_iwget_interface_info(struct uwifi_interface* intf)
{
	struct nl_msg *msg;
	bool ret;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_GET_INTERFACE, intf->ifname))
		return false;

	ret = nl80211_send_recv(sock, msg, nl80211_get_interface_info_cb, intf); /* frees msg */
	if (!ret)
		fprintf(stderr, "failed to get interface info\n");
	return ret;
}

static size_t sta_maxlen;
static size_t sta_idx;

static int nl80211_get_station_cb(struct nl_msg *msg, void *arg)
{
	struct sta_info* stas = arg;
	struct nlattr **tb = nl80211_parse(msg);
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	static struct nla_policy sta_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
		[NL80211_STA_INFO_T_OFFSET] = { .type = NLA_U64 },
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
		[NL80211_STA_INFO_TX_RETRIES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_FAILED] = { .type = NLA_U32 },
		[NL80211_STA_INFO_STA_FLAGS] = { .minlen = sizeof(struct nl80211_sta_flag_update) },
		[NL80211_STA_INFO_LOCAL_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_PEER_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_NONPEER_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_CHAIN_SIGNAL] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_CHAIN_SIGNAL_AVG] = { .type = NLA_NESTED },
	};

	if (!tb[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "STA info missing!\n");
		return NL_SKIP;
	}

	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     sta_policy)) {
		fprintf(stderr, "failed to parse STA nested attributes!\n");
		return NL_SKIP;
	}

	if (sta_idx >= sta_maxlen)
		return NL_SKIP;

	unsigned char* mac = nla_data(tb[NL80211_ATTR_MAC]);
	memcpy(stas[sta_idx].mac, mac, WLAN_MAC_LEN);

	if (sinfo[NL80211_STA_INFO_INACTIVE_TIME])
		stas[sta_idx].last = nla_get_u32(sinfo[NL80211_STA_INFO_INACTIVE_TIME]);

	if (sinfo[NL80211_STA_INFO_SIGNAL])
		stas[sta_idx].rssi = (int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);

	sta_idx++;
	return NL_SKIP;
}

int ifctrl_iwget_stations(const char *const ifname, struct sta_info* stas, size_t maxlen)
{
	struct nl_msg *msg;
	bool ret;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_GET_STATION, ifname))
		return false;

	nlmsg_hdr(msg)->nlmsg_flags |= NLM_F_DUMP;

	sta_maxlen = maxlen;
	ret = nl80211_send_recv(sock, msg, nl80211_get_station_cb, stas); /* frees msg */
	if (!ret) {
		fprintf(stderr, "failed to get stations\n");
		return ret;
	} else {
		return sta_idx;
	}
}

static int nl80211_get_freqlist_cb(struct nl_msg *msg, void *arg)
{
	int bands_remain, freqs_remain, i = 0, b = 0;

	struct nlattr **attr = nl80211_parse(msg);
	struct nlattr *bands[NL80211_BAND_ATTR_MAX + 1];
	struct nlattr *freqs[NL80211_FREQUENCY_ATTR_MAX + 1];
	struct nlattr *band, *freq;

	struct uwifi_channels* list = arg;

	nla_for_each_nested(band, attr[NL80211_ATTR_WIPHY_BANDS], bands_remain)
	{
		nla_parse(bands, NL80211_BAND_ATTR_MAX,
		          nla_data(band), nla_len(band), NULL);

		list->band[b].max_chan_width = CHAN_WIDTH_20_NOHT; /* default */

		if (bands[NL80211_BAND_ATTR_HT_CAPA]) {
			uint16_t cap = nla_get_u16(bands[NL80211_BAND_ATTR_HT_CAPA]);
			if (cap & WLAN_IE_HT_CAPAB_INFO_CHAN_WIDTH_40)
				list->band[b].max_chan_width = CHAN_WIDTH_40;
			else
				list->band[b].max_chan_width = CHAN_WIDTH_20;
		}

		if (bands[NL80211_BAND_ATTR_HT_MCS_SET] &&
		    nla_len(bands[NL80211_BAND_ATTR_HT_MCS_SET]) == 16) {
			wlan_ht_streams_from_mcs(nla_data(bands[NL80211_BAND_ATTR_HT_MCS_SET]),
						&list->band[b].streams_rx, &list->band[b].streams_tx);
		}

		if (bands[NL80211_BAND_ATTR_VHT_CAPA]) {
			uint32_t vht = nla_get_u32(bands[NL80211_BAND_ATTR_VHT_CAPA]);
			list->band[b].max_chan_width = wlan_chan_width_from_vht_capab(vht);
		}

		if (bands[NL80211_BAND_ATTR_VHT_MCS_SET] &&
		    nla_len(bands[NL80211_BAND_ATTR_VHT_MCS_SET]) == 8) {
			wlan_vht_streams_from_mcs(nla_data(bands[NL80211_BAND_ATTR_VHT_MCS_SET]),
						&list->band[b].streams_rx, &list->band[b].streams_tx);
		}

		nla_for_each_nested(freq, bands[NL80211_BAND_ATTR_FREQS], freqs_remain)
		{
			nla_parse(freqs, NL80211_FREQUENCY_ATTR_MAX,
			          nla_data(freq), nla_len(freq), NULL);

			if (!freqs[NL80211_FREQUENCY_ATTR_FREQ] ||
			    freqs[NL80211_FREQUENCY_ATTR_DISABLED])
				continue;

			uwifi_channel_list_add(list, nla_get_u32(freqs[NL80211_FREQUENCY_ATTR_FREQ]));

			if (++i >= MAX_CHANNELS)
				goto end;
		}

		list->band[b].num_channels = b == 0 ? i : i - list->band[0].num_channels;

		if (++b >= MAX_BANDS)
			goto end;
	}

end:
	list->num_channels = i;
	list->num_bands = b;
	return NL_SKIP;
}

bool ifctrl_iwget_freqlist(struct uwifi_interface* intf)
{
	struct nl_msg *msg;
	bool ret;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_GET_WIPHY, NULL))
		return false;

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, intf->if_phy);

	ret = nl80211_send_recv(sock, msg, nl80211_get_freqlist_cb, &intf->channels); /* frees msg */
	if (!ret)
		fprintf(stderr, "failed to get freqlist\n");
	return ret;

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

bool ifctrl_is_monitor(struct uwifi_interface* intf)
{
	return intf->if_type == NL80211_IFTYPE_MONITOR;
}
