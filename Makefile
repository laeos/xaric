#
# This is the Xaric Makefile, Copyright (C) 1998 Rex G. Feany <laeos@ptw.com>
#
# Please, don't edit.
#
# $Id$
#

#
# What version are we?
#

VERSION    := 0
PATCHLEVEL := 9
SUBLEVEL   := b

#
# other magical stuff
#

SHELL   := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
          else if [ -x /bin/bash ]; then echo /bin/bash; \
          else echo sh; fi ; fi)

TOPDIR  := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

RM		 := rm -f
mkinstalldirs    := scripts/mkinstalldirs
DEPEND		 := scripts/depend.sh

SUBDIRS 	:= source include scripts doc

SRC := 

EXTRADIST := Makefile configure AUTHORS COPYING ChangeLog INSTALL \
	     NEWS README TODO

OBJ := 

DEP :=


include $(patsubst %,%/module.mk,$(SUBDIRS))

OBJ := $(SRC:.c=.o)



all:    do-it-all

ifneq (scripts/Makeconfig,$(wildcard scripts/Makeconfig))
do-it-all: configure 
else


DEP := $(OBJ:.o=.d)
-include  $(DEP)

	
include scripts/Makeconfig
do-it-all:  xaric

endif

configure: dummy
	./configure


xaric: $(OBJ) Makefile scripts/Makeconfig 
	$(CC) -o xaric $(OBJ) $(LIBS) 

%.d: %.c
	$(DEPEND) $< $(CFLAGS) > $@

#
# Cleaning up
#
	

clean: 
	-$(RM) $(OBJ)

distclean: clean
	-$(RM) $(DEP)
	-$(RM) include/defs.h TAGS ID xaric 
	-$(RM) config.status stamp-h stamp-h[0-9]*
	-$(RM) scripts/Makeconfig scripts/config.log config.log config.cache



#
# Install!!
#

ifneq (scripts/Makeconfig,$(wildcard scripts/Makeconfig))
install:
	@echo 
	@echo "How do you expect me to do that? Its not even compiled!!"
	@echo
else
install:
	@echo ""
	@echo "Installing Xaric"
	@echo ""
	@echo "Installing binary(s) into $(bindir)..."
	@echo ""
	$(mkinstalldirs) $(bindir)
	$(INSTALL_PROGRAM) -m 755 xaric $(bindir)
	@echo ""	
	@echo "Xaric Installation is compleate!"
	@echo "Make sure you read the docs, and"
	@echo "configure the bot correctly!"
	@echo ""
endif



#
# For making a distrib
#

distname = xaric-$(VERSION).$(PATCHLEVEL)$(SUBLEVEL)
distdir  = $(TOPDIR)/$(distname)
distfile  = 


DISTDIR=include 

dist: 
	$(RM) -r $(distdir)
	mkdir $(distdir)

	for i in $(SUBDIRS); do mkdir $(distdir)/$$i; done
	cd $(distdir) && \
	for i in $(EXTRADIST) $(SRC); do \
		ln -sf $(TOPDIR)/$$i $$i; done 

	tar chvf - $(distname) | gzip -9 > $(distname).tar.gz
	@$(RM) -r $(distdir)


source/ver.c: dummy
	@echo "const char irc_version[] = \"$(VERSION).$(PATCHLEVEL)$(SUBLEVEL)\";" > source/ver.c
	@echo "const char irc_lib[] = \"$(datadir)\";" >> source/ver.c


dummy:
