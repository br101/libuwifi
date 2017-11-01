#ifndef UWIFI_INJECT_H_
#define UWIFI_INJECT_H_

#include <stdint.h>
#include <stdbool.h>

int uwifi_create_beacon_probe_response(unsigned char* buf, bool probe_response,
				       unsigned char* sa, unsigned char* da,
				       unsigned char* bssid, char* essid,
				       uint64_t tsf, int channel, int bintval,
				       uint16_t seqno);

int uwifi_create_nulldata(unsigned char* buf, unsigned char* sa, unsigned char* da,
			  unsigned char* bssid, uint16_t seq);

#ifdef __linux__
int uwifi_create_radiotap_header(unsigned char* buf, int freq, bool ack);
#endif

#endif
