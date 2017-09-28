# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
#
# This program is licensed under the GNU Lesser General Public License,
# Version 3. See the file COPYING for more details.

INST_PATH	= /usr/local

OBJS		+= linux/capture.o
OBJS		+= linux/ifctrl-ioctl.o
OBJS		+= linux/platform.o
OBJS		+= linux/raw_parser.o
OBJS		+= osx/capture-pcap.o
OBJS		+= osx/ifctrl-osx.o

LIBS		= -lpcap -framework CoreWLAN -framework CoreData -framework Foundation

CFLAGS		+= -fPIC

all: $(NAME).so $(NAME).a

$(NAME).so: $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(NAME).so.1 -o $(NAME).so $(OBJS) $(LIBS)
	ln -s $(NAME).so $(NAME).so.1

install:
	-mkdir $(INST_PATH)/include/uwifi
	-mkdir $(INST_PATH)/lib
	cp ./core/*.h $(INST_PATH)/include/uwifi
	cp ./util/*.h $(INST_PATH)/include/uwifi
	cp ./linux/*.h $(INST_PATH)/include/uwifi
	cp -r ./ccan $(INST_PATH)/include/
	cp ./libuwifi.a $(INST_PATH)/lib/
	cp ./libuwifi.so* $(INST_PATH)/lib/
