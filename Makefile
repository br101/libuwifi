# libuwifi - Userspace Wifi Library
#
# Copyright (C) 2005-2016 Bruno Randolf (br1@einfach.org)
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

NAME=libuwifi

# build options
DEBUG=0
PLATFORM=linux

OBJS=	core/channel.o			\
	core/node.o			\
	core/wlan_parser.o		\
	core/wlan_util.o		\
	util/average.o			\
	util/util.o			\

INCLUDES=-I. -I./core -I./util -I./$(PLATFORM)
CFLAGS+=-std=gnu99 -Wall -Wextra $(INCLUDES) -DDEBUG=$(DEBUG)

include $(PLATFORM)/platform.mk

.PHONY: all check clean force

.objdeps.mk: $(OBJS:%.o=%.c)
	gcc -MM $(INCLUDES) $^ >$@

-include .objdeps.mk

$(NAME).a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

$(OBJS): .buildflags

check: $(OBJS:%.o=%.c)
	sparse $(CFLAGS) $^

clean:
	-rm -f core/*.o util/*.o linux/*.o osx/*.o esp8266/*.o *~
	-rm -f $(NAME).so*
	-rm -f $(NAME).a*
	-rm -f .buildflags
	-rm -f .objdeps.mk

.buildflags: force
	echo '$(CFLAGS)' | cmp -s - $@ || echo '$(CFLAGS)' > $@
