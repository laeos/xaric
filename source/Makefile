# Makefile fragment
# Copyright (c) 2000 Rex Feany (laeos@xaric.org)
#
# This file is a part of Xaric, a silly IRC client.
# You can find Xaric at http://www.xaric.org/.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# @(#)Makefile 1.3
#

ifndef REALLY_INCLUDE
all clean distclean:
	@echo "Doh, wrong directory. Lets try again"
	$(MAKE) -C .. $@
else

# source files
source-src-lcl =	\
	alist.c alias.c cmd_modes.c commands.c ctcp.c dcc.c     \
	debug.c exec.c  flood.c fset.c functions.c funny.c  \
	hash.c help.c history.c hook.c iflist.c ignore.c input.c irc.c ircaux.c   \
	ircsig.c keys.c lastlog.c list.c log.c misc.c names.c network.c  \
	newio.c notice.c notify.c numbers.c output.c parse.c readlog.c   \
	reg.c screen.c server.c status.c term.c timer.c vars.c whois.c   \
	whowas.c window.c words.c tcommand.c ternary.c ncommand.c  \
	cmd_hostname.c cmd_who.c cmd_scan.c cmd_save.c cmd_orignick.c \
	version.c

source-obj-lcl =

# this should't change.
O_SRC +=  $(addprefix source/, $(sort $(source-src-lcl)))
O_OBJ +=  $(addprefix source/, $(sort $(source-obj-lcl)))

endif



## Process this file with automake to create Makefile.in


bin_PROGRAMS = xaric

xaric_SOURCES = \

EXTRA_DIST = expr.c 


