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

# build options
DEBUG=1
PCAP=0
WEXT=0
LIBNL=3.0
OSX=0

NAME=libuwifi
OBJS=	util/average.o				\
	util/util.o				\
	core/wlan_parser.o			\
	core/wlan_util.o			\
	core/node.o				\
	core/channel.o				\
	core/ieee80211_util.o			\
	linux/raw_parser.o			\
	linux/capture.o				\
	linux/ifctrl-ioctl.o			\
	linux/platform.o			\
	

LIBS=-lm -lradiotap
CFLAGS+=-std=gnu99 -Wall -Wextra -g -I. -fPIC

ifeq ($(OSX),1)
    PCAP=1
    WEXT=0
    LIBNL=0
    LIBS+=-framework CoreWLAN -framework CoreData -framework Foundation
    OBJS += ifctrl-osx.o
endif

ifeq ($(PCAP),1)
  CFLAGS+=-DPCAP
  LIBS+=-lpcap
  OBJS+=osx/capture-pcap.o
endif

ifeq ($(WEXT),1)
  OBJS += linux/ifctrl-wext.o
else
  ifeq ($(LIBNL),0)
    ifeq ($(OSX),0)
        OBJS += core/ifctrl-dummy.o
    endif
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

.PHONY: all check clean force

all: $(NAME)

.objdeps.mk: $(OBJS:%.o=%.c)
	gcc -MM -I. $^ >$@
ifeq ($(OSX),1)
	gcc -MM -I. ifctrl-osx.m >>$@
endif

-include .objdeps.mk

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@.so.1 -o $@.so $(OBJS) $(LIBS)
	$(AR) rcs $@.a $(OBJS)

$(OBJS): .buildflags

check: $(OBJS:%.o=%.c)
	sparse $(CFLAGS) $^

clean:
	-rm -f core/*.o linux/*.o osx/*.o util/*.o *~
	-rm -f $(NAME).so*
	-rm -f $(NAME).a*
	-rm -f .buildflags
	-rm -f .objdeps.mk

.buildflags: force
	echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
