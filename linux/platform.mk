# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
#
# This program is licensed under the GNU Lesser General Public License,
# Version 3. See the file COPYING for more details.

INST_PATH	= /usr/local

# build options
WEXT		= 0
LIBNL		= 3.0
BUILD_RADIOTAP	= 1
#PCAP		= 0 #TODO revive

SRC		+= linux/inject_rtap.c
SRC		+= linux/interface.c
SRC		+= linux/netdev.c
SRC		+= linux/netl80211.c
SRC		+= linux/packet_sock.c
SRC		+= linux/platform.c
SRC		+= linux/raw_parser.c
SRC		+= linux/wpa_ctrl.c

CFLAGS		+= -fPIC
CHECK_FLAGS	+= -D__linux__

ifeq ($(BUILD_RADIOTAP),1)
  INCLUDES	+= -I./radiotap
  OBJS		+= radiotap/radiotap.o
else
  LIBS		= -lradiotap
endif

ifeq ($(WEXT),1)
  OBJS		+= linux/ifctrl-wext.o
else
  ifeq ($(LIBNL),0)
    OBJS	+= core/ifctrl-dummy.o
  else
    OBJS	+= linux/ifctrl-nl80211.o
    CFLAGS += $(shell pkg-config --cflags libnl-$(LIBNL))
    ifeq ($(LIBNL),tiny)
      LIBS	+= -lnl-tiny
    else
      LIBS	+= -lnl-3 -lnl-genl-3
    endif
  endif
endif

install: lib-static lib-dynamic
	-mkdir -p $(INST_PATH)/include/uwifi
	-mkdir -p $(INST_PATH)/lib
	cp ./core/*.h $(INST_PATH)/include/uwifi
	cp ./util/*.h $(INST_PATH)/include/uwifi
	cp ./linux/*.h $(INST_PATH)/include/uwifi
	cp -r ./ccan $(INST_PATH)/include/
	cp $(BUILD_DIR)/libuwifi.a $(INST_PATH)/lib/
	cp $(BUILD_DIR)/libuwifi.so* $(INST_PATH)/lib/
