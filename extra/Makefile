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
# @(#)Makefile 1.1
#

ifndef REALLY_INCLUDE
all clean distclean:
	@echo "Doh, wrong directory. Lets try again"
	make -C .. $@
endif

# extra files
extra-src-lcl =	

ifdef ALLOCA
extra-src-lcl += alloca.c
endif

ifdef STRTOUL
extra-src-lcl += strtoul.c
endif

ifdef SETENV
extra-src-lcl += setenv.c
endif

# prebuilt object files
extra-obj-lcl =

# this should't change.
extra-src =  $(addprefix extra/, $(sort $(extra-src-lcl)))
extra-obj =  $(addprefix extra/, $(sort $(extra-obj-lcl)))

