/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2015 Bruno Randolf <br1@einfach.org>
 * Copyright (C) 2015 Jeromy Fu <fuji246@gmail.com>
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#include "ifctrl.h"
#include "main.h"
#include "wlan_util.h"

#import <CoreWLAN/CoreWLAN.h>

bool osx_set_freq(const char *interface, unsigned int freq)
{
    int channel = wlan_freq2chan(freq);

    CWWiFiClient * wifiClient = [CWWiFiClient sharedWiFiClient];
    NSString * interfaceName = [[NSString alloc] initWithUTF8String: interface];
    CWInterface * currentInterface = [wifiClient interfaceWithName: interfaceName];
    [interfaceName release];

    NSSet * channels = [currentInterface supportedWLANChannels];
    CWChannel * wlanChannel = nil;
    for (CWChannel * _wlanChannel in channels) {
        if ([_wlanChannel channelNumber] == channel)
            wlanChannel = _wlanChannel;
    }

    bool ret = true;
    if (wlanChannel != nil) {
        NSError *err = nil;
        BOOL result = [currentInterface setWLANChannel:wlanChannel error:&err];
        if( !result ) {
            LOG_INF("set channel %ld err: %s", (long)[wlanChannel channelNumber], [[err localizedDescription] UTF8String]);
            ret = false;
        }
    }

    return ret;
}

enum chan_width get_channel_width(CWChannelWidth width)
{
    enum chan_width current_width = CHAN_WIDTH_UNSPEC;
    switch (width) {
        case kCWChannelWidth20MHz:
            current_width = CHAN_WIDTH_20;
            break;

        case kCWChannelWidth40MHz:
            current_width = CHAN_WIDTH_40;
            break;

        case kCWChannelWidth80MHz:
            current_width = CHAN_WIDTH_80;
            break;

        case kCWChannelWidth160MHz:
            current_width = CHAN_WIDTH_160;
            break;

        default:
            break;
    }
    return current_width;
}

int osx_get_channels(const char* devname, struct channel_list* channels) {
    CWWiFiClient * wifiClient = [CWWiFiClient sharedWiFiClient];
    NSString * interfaceName = [[NSString alloc] initWithUTF8String: devname];
    CWInterface * currentInterface = [wifiClient interfaceWithName: interfaceName];
    [interfaceName release];

    NSSet * supportedChannelsSet = [currentInterface supportedWLANChannels];
    NSSortDescriptor * sort = [NSSortDescriptor sortDescriptorWithKey:@"channelNumber" ascending:YES];
    NSArray * sortedChannels = [supportedChannelsSet sortedArrayUsingDescriptors:[NSArray arrayWithObject:sort]];

    int i = 0;
    for (int i = 0; i < MAX_BANDS; ++i) {
        channels->band[i].num_channels = 0;
        channels->band[i].streams_rx = 0;
        channels->band[i].streams_tx = 0;
        channels->band[i].max_chan_width = CHAN_WIDTH_20;
    }
    channels->num_bands = MAX_BANDS;

    i = 0;
    NSInteger lastNum = -1;
    for( id eachChannel in sortedChannels )
    {
        NSInteger num = [eachChannel channelNumber];
        CWChannelBand band = [eachChannel channelBand];
        CWChannelWidth width = [eachChannel channelWidth];
        LOG_INF("num: %ld, band: %ld, width: %ld", num, (long)band, (long)width);

        if (lastNum != num ) {
            channel_list_add(ieee80211_channel2freq(num));
        }

        int bandIdx = -1;
        if( kCWChannelBand2GHz == band ) {
            bandIdx = 0;
        } else if( kCWChannelBand5GHz == band ) {
            bandIdx = 1;
        }
        if( bandIdx >= 0) {
            if (lastNum != num ) {
                ++(channels->band[bandIdx].num_channels);
            }
            enum chan_width w = get_channel_width(width);
            channels->band[bandIdx].max_chan_width = \
            channels->band[bandIdx].max_chan_width < w ? w : channels->band[bandIdx].max_chan_width;
        }

        lastNum = num;
        if( ++i > MAX_CHANNELS) {
            break;
        }
    }

    LOG_INF("band 0 channels: %d", channels->band[0].num_channels);
    LOG_INF("band 1 channels: %d", channels->band[1].num_channels);

    return i;
}

bool ifctrl_init() {
    CWWiFiClient * wifiClient = [CWWiFiClient sharedWiFiClient];
    NSString * interfaceName = [[NSString alloc] initWithUTF8String: conf.ifname];
    CWInterface * currentInterface = [wifiClient interfaceWithName: interfaceName];
    [interfaceName release];

    [currentInterface disassociate];
    return true;
};

void ifctrl_finish() {
    CWWiFiClient * wifiClient = [CWWiFiClient sharedWiFiClient];
    NSString * interfaceName = [[NSString alloc] initWithUTF8String: conf.ifname];
    CWInterface * currentInterface = [wifiClient interfaceWithName: interfaceName];
    [interfaceName release];

    CWNetwork * _network = [[CWNetwork alloc] init];
    [currentInterface associateToNetwork:_network password:nil error:nil];
};

bool ifctrl_iwadd_monitor(__attribute__((unused))const char *interface, __attribute__((unused))const char *monitor_interface) {
    LOG_ERR("add monitor: not implemented");
    return false;
};

bool ifctrl_iwdel(__attribute__((unused))const char *interface) {
    LOG_ERR("iwdel: not implemented");
    return false;
};

bool ifctrl_iwset_monitor(__attribute__((unused))const char *interface) {
    LOG_ERR("set monitor: not implemented");
    return false;
};

bool ifctrl_iwset_freq(__attribute__((unused))const char *interface, __attribute__((unused))unsigned int freq,
                       __attribute__((unused))enum chan_width width, __attribute__((unused))unsigned int center) {
    if (osx_set_freq(interface, freq))
        return true;
    return false;
};

bool ifctrl_iwget_interface_info(__attribute__((unused)) struct uwifi_interface* intf) {
    LOG_ERR("get interface info: not implemented");
    return false;
};

bool ifctrl_iwget_freqlist(struct uwifi_interface* intf) {
    int num_channels = osx_get_channels(intf->ifname, &intf->channels);
    if (num_channels)
        return true;
    return false;
};

bool ifctrl_is_monitor(__attribute__((unused)) struct uwifi_interface* intf) {
    return true;
};
