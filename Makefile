# Makefile - top level makefile for Xaric
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
# @(#)Makefile 1.4
#

.DELETE_ON_ERROR:


# Programs
SHTOOL := ./shtool
MD5    := md5sum

# Random Variables
VFILE_DIR := source
VFILE     := version.c

# Figure out our version
VERSION:=`$(SHTOOL) version -d short $(VFILE_DIR)/$(VFILE)`


# This contains all the build rules.
-include Makerules

########################################
# Generate autoconf files              #
########################################
gen:
	aclocal
	autoheader
	autoconf

########################################
# Build the distribution files         #
########################################

# things that we should ignore when building the distribution tar
TARIGNORE = config.log,/config.h$$,^xaric,^Makerules$$,\
	    config.cache,config.status,SCCS,.depend,\
	    BitKeeper,CVS,ChangeSet,.cvsignore,.[oa]$$

dist:
	@echo "Building distribution for Xaric v$(VERSION)"
	@$(SHTOOL) tarball -v -o xaric-$(VERSION).tar.gz \
					-c 'gzip -9' -e '$(TARIGNORE)' .
	@$(MD5) xaric-$(VERSION).tar.gz > xaric-$(VERSION).tar.gz.md5
	@echo "Done."


########################################
# Increment version numbers            #
########################################

doversion = cd $(VFILE_DIR) && ../shtool version -lc -n xaric -p x 

release:
	@$(doversion) -iv $(VFILE)
revision:
	@$(doversion) -ir $(VFILE)
patch:
	@$(doversion) -il $(VFILE)


