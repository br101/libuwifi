# libuwifi - Userspace Wifi Library

This is a C library for parsing, generating and analyzing Wifi (WLAN 802.11)
frames in userspace and related functions. It is written in plain C and the core
part is platform independent and can be used anywhere from Microcontrollers
to Linux and OSX.

 * IEEE 802.11 frame formats and frame parsing
 * RSSI and statistics per source MAC address ("node")
 * Channel management (channel, frequency, channel width) for 802.11n and 802.11ac
 * Raw sockets under Linux
 * Creating monitor interfaces and configuring wifi interfaces (nl80211 and OSX)
 * ESP8266 promiscuous mode
 * Frame injection

"uwifi" is licensed under the "GNU Lesser General Public License" (LGPL), so it
can be used in any application, as long as the changes to the library itself are
redistributed under the LGPL.

It is used by the following projects:

 * https://github.com/br101/esp-monitor - Wifi Monitor on the ESP8266
 * https://github.com/br101/horst  - horst (new version in development)
