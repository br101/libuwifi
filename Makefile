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

LIBS=-lm -lradiotap
CFLAGS+=-std=gnu99 -Wall -Wextra -g -I. -fPIC

.PHONY: all check clean force

all: $(NAME)

.objdeps.mk: $(OBJS:%.o=%.c)
	gcc -MM -I. $^ >$@

-include .objdeps.mk

$(NAME): $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@.so.1 -o $@.so $(OBJS) $(LIBS)
	$(AR) rcs $@.a $(OBJS)

$(OBJS): .buildflags

check: $(OBJS:%.o=%.c)
	sparse $(CFLAGS) $^

clean:
	-rm -f $(NAME).so*
	-rm -f $(NAME).a*
	-rm -f .buildflags
	-rm -f .objdeps.mk

.buildflags: force
	echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
