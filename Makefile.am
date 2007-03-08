bin_PROGRAMS = xaric
xaric_SOURCES = alist.c  cmd_modes.c ctcp.c dcc.c     \
		debug.c exec.c expr.c flood.c fset.c functions.c funny.c  \
		hash.c history.c hook.c iflist.c ignore.c input.c irc.c ircaux.c   \
		signals.c keys.c lastlog.c list.c lister.c  log.c misc.c names.c network.c  \
		newio.c notice.c notify.c numbers.c output.c parse.c readlog.c   \
		reg.c screen.c server.c status.c ircterm.c timer.c vars.c whois.c   \
		whowas.c window.c words.c tcommand.c ternary.c ncommand.c cmd_help.c \
		cmd_hostname.c cmd_who.c cmd_scan.c cmd_save.c cmd_orignick.c \
		xscandir.c sa.c sa.h \
		alist.h commands.h ctcp.h dcc.h debug.h defaults.h \
		exec.h expr.h flood.h format.h fset.h funny.h \
		gai.h hash.h history.h hook.h iflist.h \
		ignore.h input.h irc.h irc_std.h ircaux.h \
		ircterm.h keys.h lastlog.h linklist.h list.h \
		lister.h log.h misc.h names.h newio.h notice.h notify.h \
		numbers.h output.h parse.h screen.h server.h status.h struct.h \
		tcommand.h threads.h timer.h vars.h whois.h whowas.h window.h

nodist_xaric_SOURCES = fset_gen.h vars_gen.h build.h
BUILT_SOURCES = fset_gen.h vars_gen.h build.h
CLEANFILES = fset_gen.h build.h stamp-h vars_gen.h config.log
EXTRA_xaric_SOURCES = freeaddrinfo.c ga_aistruct.c ga_echeck.c ga_nsearch.c \
		      ga_port.c ga_serv.c gai_strerror.c getaddrinfo.c getnameinfo.c \
		      gn_ipv46.c inet_ntop.c inet_pton.c setenv.c strtoul.c

HELP_FILES= help/copying help/help help/warranty
xaric_LDADD = @EXTRA_OBJS@ 
xaric_CFLAGS = -Wall -ggdb3
man_MANS = xaric.1
EXTRA_DIST = $(man_MANS) $(HELP_FILES) xaric.xpm

SED=sed

# generate array indexes automagicly
fset_gen.h: fset.c
	cat $(srcdir)/fset.c | $(SED) -n -e "/\*--start--\*/,/\*--end--\*/ { s/^[^\"]*\"\([^\"]*\).*$$/\1_FSET,/; s/\/\*[^*]*\*\///; p; }" > $@

vars_gen.h: vars.c
	cat $(srcdir)/vars.c | $(SED) -n -e "/\*--start--\*/,/\*--end--\*/ { s/^[^\"]*\"\([^\"]*\).*$$/\1_VAR,/; s/\/\*[^*]*\*\///; p; }" > $@

# Supply some more information to the build
build.h: Makefile
	@echo \#define XARIC_DATA_PATH \"$(pkgdatadir)\" >> .ver
	@mv .ver $@


helpdir = $(DESTDIR)$(pkgdatadir)/help
install-data-hook:
	$(mkdir_p) $(helpdir)
	for i in $(HELP_FILES) ; do \
		cp $(srcdir)/$$i $(helpdir) ; \
	done
