## Process this file with automake to create Makefile.in


bin_PROGRAMS = xaric

xaric_LDADD = @PERL_OBJ@ @perllibs@ 

PERL_CFLAGS = @PERL_CFLAGS@

xaric_SOURCES = \
	alist.c alias.c alloca.c cmd_modes.c commands.c ctcp.c dcc.c     \
	debug.c env.c exec.c files.c flood.c fset.c functions.c funny.c  \
	hash.c help.c history.c hook.c ignore.c input.c irc.c ircaux.c   \
	ircsig.c keys.c lastlog.c list.c log.c misc.c names.c network.c  \
	newio.c notice.c notify.c numbers.c output.c parse.c readlog.c   \
	reg.c screen.c server.c status.c term.c timer.c vars.c whois.c   \
	whowas.c window.c words.c tcommand.c ternary.c ncommand.c  \
	cmd_hostname.c cmd_who.c cmd_scan.c cmd_save.c cmd_orignick.c

xaric_DEPENDENCIES = @PERL_OBJ@

EXTRA_DIST = expr.c x_perl.xs x_perlxsi.c
DISTCLEAN_FILES = x_perl.c 


x_perl.o: dummy
	$(path_perl) @theperllib@/ExtUtils/xsubpp -prototypes -typemap @theperllib@/ExtUtils/typemap x_perl.xs > x_perl.c
	$(COMPILE) $(PERL_CFLAGS) -c -o x_perl.o x_perl.c

x_perlxsi.o: dummy
	$(COMPILE) $(PERL_CFLAGS) -c -o x_perlxsi.o x_perlxsi.c

.PHONY: dummy
		
