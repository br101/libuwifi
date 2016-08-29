/*
 * libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This source code is licensed under the GNU Lesser General Public License,
 * Version 3. See the file COPYING for more details.
 */

#ifndef _UWIFI_PKT_SOCKET_H_
#define _UWIFI_PKT_SOCKET_H_

#include <stddef.h>

int packet_socket_open(char* devname);

ssize_t packet_socket_recv(int fd, unsigned char* buffer, size_t bufsize);

void socket_set_receive_buffer(int fd, int sockbufsize);

#endif // _UWIFI_PKT_SOCKET_H_
