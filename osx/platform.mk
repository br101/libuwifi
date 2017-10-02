# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
#
# This program is licensed under the GNU Lesser General Public License,
# Version 3. See the file COPYING for more details.

INST_PATH	= /usr/local

SRC		+= linux/capture.c
SRC		+= linux/ifctrl-ioctl.c
SRC		+= linux/platform.c
SRC		+= linux/raw_parser.c
SRC		+= osx/capture-pcap.c
SRC		+= osx/ifctrl-osx.c

LIBS		= -lpcap -framework CoreWLAN -framework CoreData -framework Foundation

CFLAGS		+= -fPIC

install: lib-static lib-dynamic
	-mkdir $(INST_PATH)/include/uwifi
	-mkdir $(INST_PATH)/lib
	cp ./core/*.h $(INST_PATH)/include/uwifi
	cp ./util/*.h $(INST_PATH)/include/uwifi
	cp ./linux/*.h $(INST_PATH)/include/uwifi
	cp -r ./ccan $(INST_PATH)/include/
	cp $(BUILD_DIR)/libuwifi.a $(INST_PATH)/lib/
	cp $(BUILD_DIR)/libuwifi.so* $(INST_PATH)/lib/
