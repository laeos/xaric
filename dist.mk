# dist.mk - targets for managing the distribution
# Copyright (c) 2000 Rex Feany (laeos@xaric.org)
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
# @(#)dist.mk 1.7
#


# Programs
SHTOOL	:= ./shtool




# things that we should ignore when building the distribution tar
TARIGNORE=config.log,/config.h$$,^xaric,^Makefile$$,config.cache,config.status,SCCS,.depend,BitKeeper,CVS,Changeet,.cvsignore,.[oa]$$

# Find out our version from the xaric version file.
VERSION:=`$(SHTOOL) version -d short source/xversion.c`


gen:
	aclocal
	autoheader
	autoconf

dist:
	@echo "Building distribution for Xaric v$(VERSION)"
	@$(SHTOOL) tarball -v -o xaric-$(VERSION).tar.gz -c 'gzip -9' -e '$(TARIGNORE)' .
	@/usr/bin/md5sum xaric-$(VERSION).tar.gz > xaric-$(VERSION).tar.gz.md5
	@echo "Done."


release:
	@(cd source && ../shtool version -lc -n xaric -p x  -iv xversion.c)
revision:
	@(cd source && ../shtool version -lc -n xaric -p x  -ir xversion.c)
patch:
	@(cd source && ../shtool version -lc -n xaric -p x  -il xversion.c)

