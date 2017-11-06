# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
#
# This program is licensed under the GNU Lesser General Public License,
# Version 3. See the file COPYING for more details.

NAME		= libuwifi

# build options
DEBUG		= 0
PLATFORM	= linux

SRC		+= core/channel.c
SRC		+= core/inject.c
SRC		+= core/node.c
SRC		+= core/wlan_parser.c
SRC		+= core/wlan_util.c
SRC		+= core/essid.c
SRC		+= util/average.c
SRC		+= util/util.c

ifeq ($(DEBUG),1)
  SRC		+= ccan/list/list.c
endif

INCLUDES	+= -I. -I./include/uwifi -I./$(PLATFORM)
CFLAGS		+= -std=gnu99 -Wall -Wextra -g
DEFS		+= -DDEBUG=$(DEBUG)
CHECK_FLAGS	+= -D__linux__

all: lib-static lib-dynamic
check:
clean:

include $(PLATFORM)/platform.mk
include Makefile.default
