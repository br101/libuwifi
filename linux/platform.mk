# horst - Highly Optimized Radio Scanning Tool
#
# Copyright (C) 2005-2015 Bruno Randolf (br1@einfach.org)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

INST_PATH=/usr/local

# build options
WEXT=0
LIBNL=3.0
#PCAP=0 #TODO revive

OBJS+=	linux/capture.o			\
	linux/ifctrl-ioctl.o		\
	linux/platform.o		\
	linux/raw_parser.o		\

LIBS=-lradiotap
CFLAGS+=-fPIC

ifeq ($(WEXT),1)
  OBJS += linux/ifctrl-wext.o
else
  ifeq ($(LIBNL),0)
    OBJS += core/ifctrl-dummy.o
  else
    OBJS += linux/ifctrl-nl80211.o
    CFLAGS += $(shell pkg-config --cflags libnl-$(LIBNL))
    ifeq ($(LIBNL),tiny)
      LIBS+=-lnl-tiny
    else
      LIBS+=-lnl-3 -lnl-genl-3
    endif
  endif
endif

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
