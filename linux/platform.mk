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
PCAP=0
WEXT=0
LIBNL=3.0
OSX=0

OBJS+=	linux/raw_parser.o			\
	linux/capture.o				\
	linux/ifctrl-ioctl.o			\
	linux/platform.o			\

LIBS=-lm -lradiotap
CFLAGS+=-fPIC

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

all: $(NAME).so $(NAME).a

$(NAME).so: $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(NAME).so.1 -o $(NAME).so $(OBJS) $(LIBS)
	ldconfig -n .

