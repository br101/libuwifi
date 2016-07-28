/* libuwifi - Userspace Wifi Library
 *
 * Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _UWIFI_PKT_SOCKET_H_
#define _UWIFI_PKT_SOCKET_H_

#include <stddef.h>

int packet_socket_open(char* devname);

ssize_t packet_socket_recv(int fd, unsigned char* buffer, size_t bufsize);

void socket_set_receive_buffer(int fd, int sockbufsize);

#endif // _UWIFI_PKT_SOCKET_H_
