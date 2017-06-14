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

#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <linux/nl80211.h>

#include "ifctrl.h"
#include "wlan_util.h"
#include "conf.h"
#include "util.h"
#include "netl80211.h"

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

bool ifctrl_iwadd_sta(int phyidx, const char *const new_interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_NEW_INTERFACE, NULL))
		return false;

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, phyidx);
	NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, new_interface);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_STATION);

	return nl80211_send(nl_sock, msg); /* frees msg */

nla_put_failure:
	fprintf(stderr, "failed to add attribute to netlink message\n");
	nlmsg_free(msg);
	return false;
}

bool ifctrl_iwadd_monitor(const char *const interface,
			  const char *const monitor_interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_NEW_INTERFACE, interface))
		return false;

	NLA_PUT_STRING(msg, NL80211_ATTR_IFNAME, monitor_interface);
	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);

	return nl80211_send(nl_sock, msg); /* frees msg */

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

	return nl80211_send(nl_sock, msg); /* frees msg */
}

bool ifctrl_iwset_monitor(const char *const interface)
{
	struct nl_msg *msg;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_SET_INTERFACE, interface))
		return false;

	NLA_PUT_U32(msg, NL80211_ATTR_IFTYPE, NL80211_IFTYPE_MONITOR);
	return nl80211_send(nl_sock, msg); /* frees msg */

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

	return nl80211_send(nl_sock, msg); /* frees msg */
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

	return nl80211_send(nl_sock, msg); /* frees msg */

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

	return nl80211_send(nl_sock, msg); /* frees msg */

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
		intf->channel.freq = nla_get_u32(tb[NL80211_ATTR_WIPHY_FREQ]);

	if (tb[NL80211_ATTR_CHANNEL_WIDTH]) {
		int nlw = nla_get_u32(tb[NL80211_ATTR_CHANNEL_WIDTH]);
		switch (nlw) {
			case NL80211_CHAN_WIDTH_20_NOHT:
				intf->channel.width = CHAN_WIDTH_20_NOHT; break;
			case NL80211_CHAN_WIDTH_20:
				intf->channel.width = CHAN_WIDTH_20; break;
			case NL80211_CHAN_WIDTH_40:
				intf->channel.width = CHAN_WIDTH_40; break;
			case NL80211_CHAN_WIDTH_80:
				intf->channel.width = CHAN_WIDTH_80; break;
			case NL80211_CHAN_WIDTH_160:
				intf->channel.width = CHAN_WIDTH_160; break;
			case NL80211_CHAN_WIDTH_80P80:
				intf->channel.width = CHAN_WIDTH_8080; break;
			default:
				intf->channel.width = CHAN_WIDTH_UNSPEC; break;
		}
	}

	if (tb[NL80211_ATTR_CENTER_FREQ1]) {
		intf->channel.center_freq = nla_get_u32(tb[NL80211_ATTR_CENTER_FREQ1]);
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

	ret = nl80211_send_recv(nl_sock, msg, nl80211_get_interface_info_cb, intf); /* frees msg */
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
		[NL80211_STA_INFO_SIGNAL_AVG] = { .type = NLA_U8 },
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

	if (sinfo[NL80211_STA_INFO_SIGNAL_AVG])
		stas[sta_idx].rssi_avg = (int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL_AVG]);

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

	sta_idx = 0;
	sta_maxlen = maxlen;

	ret = nl80211_send_recv(nl_sock, msg, nl80211_get_station_cb, stas); /* frees msg */
	if (!ret) {
		fprintf(stderr, "failed to get stations\n");
		return ret;
	} else {
		return sta_idx;
	}
}

static size_t surv_maxlen;
static size_t surv_idx;

static int nl80211_get_survey_cb(struct nl_msg *msg, void *arg)
{
	struct survey_info* inf = arg;
	struct nlattr **tb = nl80211_parse(msg);
	struct nlattr *sinfo[NL80211_SURVEY_INFO_MAX + 1];
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
	};

	if (!tb[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "Survey info missing!\n");
		return NL_SKIP;
	}

	if (nla_parse_nested(sinfo, NL80211_SURVEY_INFO_MAX,
			     tb[NL80211_ATTR_SURVEY_INFO],
			     survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (surv_idx >= surv_maxlen)
		return NL_SKIP;

	if (sinfo[NL80211_SURVEY_INFO_FREQUENCY])
		inf[surv_idx].freq = nla_get_u32(sinfo[NL80211_SURVEY_INFO_FREQUENCY]);

	inf[surv_idx].in_use = sinfo[NL80211_SURVEY_INFO_IN_USE];

	if (sinfo[NL80211_SURVEY_INFO_NOISE])
		inf[surv_idx].noise = nla_get_u8(sinfo[NL80211_SURVEY_INFO_NOISE]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME])
		inf[surv_idx].time_active = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY])
		inf[surv_idx].time_busy = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY])
		inf[surv_idx].time_busy_ext = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX])
		inf[surv_idx].time_rx = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX])
		inf[surv_idx].time_tx = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]);

	surv_idx++;
	return NL_SKIP;
}

int ifctrl_iwget_survey(const char *const ifname, struct survey_info* inf, size_t maxlen)
{
	struct nl_msg *msg;
	bool ret;

	if (!nl80211_msg_prepare(&msg, NL80211_CMD_GET_SURVEY, ifname))
		return false;

	nlmsg_hdr(msg)->nlmsg_flags |= NLM_F_DUMP;

	surv_idx = 0;
	surv_maxlen = maxlen;

	ret = nl80211_send_recv(nl_sock, msg, nl80211_get_survey_cb, inf); /* frees msg */
	if (!ret) {
		fprintf(stderr, "failed to get survey\n");
		return ret;
	} else {
		return surv_idx;
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

	ret = nl80211_send_recv(nl_sock, msg, nl80211_get_freqlist_cb, &intf->channels); /* frees msg */
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

static int nl80211_event_cb(struct nl_msg *msg, void *arg)
{
	iw_event_cb_t user_cb = arg;
	int ifindex = -1;
	int phy = -1;
	int x = -1;
	unsigned char* mac = NULL;

	/* don't use nl80211_parse here as we need the generic message header */
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	static struct nlattr *tb[NL80211_ATTR_MAX + 1];
	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
	          genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_IFINDEX])
		ifindex = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);

	if (tb[NL80211_ATTR_WIPHY])
		phy = nla_get_u32(tb[NL80211_ATTR_WIPHY]);

	switch (gnlh->cmd) {
		case NL80211_CMD_NEW_WIPHY:
		case NL80211_CMD_TRIGGER_SCAN:
		case NL80211_CMD_NEW_SCAN_RESULTS:
		case NL80211_CMD_SCAN_ABORTED:
		case NL80211_CMD_START_SCHED_SCAN:
		case NL80211_CMD_SCHED_SCAN_STOPPED:
		case NL80211_CMD_SCHED_SCAN_RESULTS:
		case NL80211_CMD_REG_CHANGE:
		case NL80211_CMD_REG_BEACON_HINT:
			break; /* ignored */

		case NL80211_CMD_NEW_STATION:
		case NL80211_CMD_DEL_STATION:
		case NL80211_CMD_JOIN_IBSS:
			mac = nla_data(tb[NL80211_ATTR_MAC]);
			break;

		case NL80211_CMD_AUTHENTICATE:
		case NL80211_CMD_ASSOCIATE:
		case NL80211_CMD_DEAUTHENTICATE:
		case NL80211_CMD_DISASSOCIATE:
		case NL80211_CMD_UNPROT_DEAUTHENTICATE:
		case NL80211_CMD_UNPROT_DISASSOCIATE:
			//tb[NL80211_ATTR_FRAME]
			//tb[NL80211_ATTR_TIMED_OUT]
			break;

		case NL80211_CMD_CONNECT:
			if (tb[NL80211_ATTR_STATUS_CODE])
				x = nla_get_u16(tb[NL80211_ATTR_STATUS_CODE]);
			if (tb[NL80211_ATTR_MAC])
				mac = nla_data(tb[NL80211_ATTR_MAC]);
			break;

		case NL80211_CMD_ROAM:
			if (tb[NL80211_ATTR_MAC])
				mac = nla_data(tb[NL80211_ATTR_MAC]);
			break;

		case NL80211_CMD_DISCONNECT:
			//tb[NL80211_ATTR_DISCONNECTED_BY_AP])
			if (tb[NL80211_ATTR_REASON_CODE])
				x = nla_get_u16(tb[NL80211_ATTR_REASON_CODE]);
			break;

		case NL80211_CMD_REMAIN_ON_CHANNEL:
		case NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL:
		case NL80211_CMD_NOTIFY_CQM:
			//connection quality monitor
		case NL80211_CMD_MICHAEL_MIC_FAILURE:
		case NL80211_CMD_FRAME_TX_STATUS:
			// injected frame TX status?
		case NL80211_CMD_PMKSA_CANDIDATE:
		case NL80211_CMD_SET_WOWLAN:
		case NL80211_CMD_PROBE_CLIENT:
		case NL80211_CMD_VENDOR:
		case NL80211_CMD_RADAR_DETECT:
		case NL80211_CMD_DEL_WIPHY:
			break; /* ignored */
	}

	user_cb(gnlh->cmd, phy, ifindex, mac, x);

	return NL_SKIP;
}

int ifctrl_iw_event_init_socket(iw_event_cb_t user_cb)
{
	int mcid, ret;

	/* open and connect genl socket */
	nl_event = nl_socket_alloc();
	if (!nl_event) {
		fprintf(stderr, "failed to allocate event netlink socket\n");
		return -1;
	}

	ret = genl_connect(nl_event);
	if (ret) {
		nl_perror(ret, "failed to make generic netlink connection");
		return -1;
	}

	/* resolve and subscribe to multicast groups */
	const char* grp_names[] = { "config", "scan", "regulatory", "mlme" /*, "vendor" */};
	for (size_t i = 0; i < ARRAY_SIZE(grp_names); i++) {
		mcid = nl_get_multicast_id(nl_event, NL80211_GENL_NAME, grp_names[i]);
		if (mcid >= 0) {
			ret = nl_socket_add_membership(nl_event, mcid);
			if (ret)
				return -1;
		}
	}

	nl_socket_disable_seq_check(nl_event);
	nl_socket_set_nonblocking(nl_event);

	nl_socket_modify_cb(nl_event, NL_CB_VALID, NL_CB_CUSTOM, nl80211_event_cb, user_cb);

	return nl_socket_get_fd(nl_event);
}

void ifctrl_iw_event_receive(void)
{
	if (nl_event)
		nl_recvmsgs_default(nl_event);
}
