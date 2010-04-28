#
# configure.in - autoconf source for xaric. 
# Copyright (c) 2000-2005 Rex Feany (laeos@xaric.org)
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
# @(#)configure.in 1.10
#

dnl ---------------------------------------------------------------
dnl autoconf prolog
dnl ---------------------------------------------------------------

AC_INIT([Xaric],[0.13.7],[support@xaric.org],[xaric])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([irc.c])
AC_CONFIG_HEADERS([config.h])
AC_PREFIX_DEFAULT(/usr/local)
AM_MAINTAINER_MODE

CFLAGS="$CFLAGS -D_GNU_SOURCE"

dnl ----------------------------------------------------------------
dnl build tools
dnl ----------------------------------------------------------------

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_ISC_POSIX
AC_AIX

dnl ----------------------------------------------------------
dnl Stuff we need
dnl ----------------------------------------------------------

AC_HEADER_STDC
AC_HEADER_TIME

AC_CHECK_HEADERS(
 	stdio.h stdlib.h stdarg.h string.h unistd.h fcntl.h dnl
 	ctype.h sys/types.h sys/time.h sys/wait.h sys/socket.h dnl
 	dirent.h sys/stat.h assert.h  dnl
 	netdb.h)

AC_CHECK_FUNC(select, ,
  AC_CHECK_LIB(socket, select, LIBS="$LIBS -lsocket",
    AC_CHECK_LIB(nsl, select, LIBS="$LIBS -lnsl",
      AC_CHECK_LIB(inet, select, LIBS="$LIBS -linet",
        AC_CHECK_LIB(cposix, select, LIBS="$LIBS -lcposix",
          AC_CHECK_LIB(net, select, LIBS="$LIBS -lnet",
            AC_MSG_WARN(i can not find select.  you might need to help me)))))))

dnl ----------------------------------------------------------
dnl System services
dnl ----------------------------------------------------------

sinclude(sa.ac)
SA_CHECK_ALL

dnl ----------------------------------------------------------
dnl System services
dnl ----------------------------------------------------------

AC_MSG_CHECKING(for IPv4 support)
AC_CACHE_VAL(ac_cv_ipv4,
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
	/* Make sure the definitions for AF_INET and struct sockaddr_in
	 * are defined, and that we can actually create an IPv4 TCP socket.
	 */
	main()
	{
		int fd;
		struct sockaddr_in foo;
		fd = socket(AF_INET, SOCK_STREAM, 0);
		exit(fd >= 0 ? 0 : 1);
	}]])],[ac_cv_ipv4=yes],[ac_cv_ipv4=no],[ac_cv_ipv4=no]))
AC_MSG_RESULT($ac_cv_ipv4)
if test $ac_cv_ipv4 = yes ; then
	AC_DEFINE(IPV4, 1, Defined if your system has IPv4 support.)
fi

dnl Let's see if the system really supports IPv6.
dnl
AC_MSG_CHECKING(for IPv6 support)
AC_CACHE_VAL(ac_cv_ipv6,
	AC_RUN_IFELSE([AC_LANG_SOURCE([[
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
	/* Make sure the definitions for AF_INET6 and struct sockaddr_in6
	 * are defined, and that we can actually create an IPv6 TCP socket.
	 */
	main()
	{
		int fd;
		struct sockaddr_in6 foo;
		fd = socket(AF_INET6, SOCK_STREAM, 0);
		exit(fd >= 0 ? 0 : 1);
	}]])],[ac_cv_ipv6=yes],[ac_cv_ipv6=no],[ac_cv_ipv6=no]))
AC_MSG_RESULT($ac_cv_ipv6)
if test $ac_cv_ipv6 = yes ; then
	AC_DEFINE(IPV6, 1, Defined if your system has IPv6 support.)
fi

AC_CHECK_LIB(pthread, pthread_create)
if test "$ac_cv_lib_pthread_pthread_create" = yes ; then
   CFLAGS="$CFLAGS -D_REENTRANT"
else
   AC_CHECK_LIB(pthreads, pthread_create)
   if test "$ac_cv_lib_pthreads_pthread_create" = yes ; then
      CFLAGS="$CFLAGS -D_REENTRANT"
   fi
fi

dnl ----------------------------------------------------------
dnl Extra libraries
dnl ----------------------------------------------------------

AC_MSG_CHECKING(for svr4's /usr/ccs/lib)
if test -d /usr/ccs/lib; then
	LIBS="-L/usr/ccs/lib $LIBS"
	AC_MSG_RESULT(found it!)
else
	AC_MSG_RESULT(no)
fi

AC_CHECK_LIB(sun, getpwnam, LIBS="$LIBS -lsun",)
AC_CHECK_LIB(dgc, inet_addr, LIBS="$LIBS -ldgc",)
AC_CHECK_LIB(gnumalloc, malloc, LIBS="$LIBS -lgnumalloc",
AC_CHECK_LIB(bsdmalloc, malloc, LIBS="$LIBS -lbsdmalloc",))

dnl ----------------------------------------------------------
dnl Data Types
dnl ----------------------------------------------------------

AC_TYPE_SIGNAL
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T

AC_C_CONST
AC_C_INLINE

AC_CACHE_CHECK(for addrinfo{}, ac_cv_have_addrinfo_struct,
	AC_EGREP_HEADER(addrinfo, netdb.h,
		ac_cv_have_addrinfo_struct=yes,
		ac_cv_have_addrinfo_struct=no))
if test $ac_cv_have_addrinfo_struct = yes ; then
	AC_DEFINE(HAVE_ADDRINFO_STRUCT, 1, Defined if you have struct addrinfo.)
fi

AC_CACHE_CHECK(if sockaddr{} has sa_len member, ac_cv_sockaddr_has_sa_len,
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>]], [[unsigned int i = sizeof(((struct sockaddr *)0)->sa_len)]])],[ac_cv_sockaddr_has_sa_len=yes],[ac_cv_sockaddr_has_sa_len=no]))
if test $ac_cv_sockaddr_has_sa_len = yes ; then
	AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, Defined if struct sockaddr has sa_len field.)
fi

AC_CACHE_CHECK(for socklen_t, ac_cv_socklen_t,
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#		include <sys/types.h>
#		include <sys/socket.h>]], [[socklen_t foo = 0]])],[ac_cv_socklen_t=yes],[ac_cv_socklen_t=no]))
if test $ac_cv_socklen_t = no ; then
	AC_DEFINE_UNQUOTED(socklen_t, unsigned int, Defined unless socklen_t is defined.)
fi


dnl ugh can we do this better?
AC_CHECK_SIZEOF(unsigned int, 4)
if test "$ac_cv_sizeof_unsigned_int" = 4; then
	AC_CHECK_TYPE(u_int32_t, unsigned int)
else
	AC_CHECK_SIZEOF(unsigned long, 4)
	if test "$ac_cv_sizeof_unsigned_long" = 4; then
		AC_CHECK_TYPE(u_int32_t, unsigned long)
	else
		AC_MSG_ERROR([i need a 32 bit type!])
	fi
fi

dnl ----------------------------------------------------------
dnl Random header files and functions
dnl ----------------------------------------------------------

AC_CHECK_HEADERS(sys/ptem.h sys/select.h)
AC_CHECK_HEADERS(sys/syslimits.h sys/twg_config.h sys/un.h)
AC_CHECK_HEADERS(sys/termios.h sys/ttycom.h termcap.h)

AC_CHECK_FUNC(fpathconf, AC_DEFINE(HAVE_FPATHCONF, 1, Defined if you have fpathconf().),)
AC_CHECK_FUNC(gettimeofday, AC_DEFINE(HAVE_GETTIMEOFDAY, 1, Defined if you have gettimeofday.),)
AC_CHECK_FUNC(sysconf, AC_DEFINE(HAVE_SYSCONF, 1, Defined if system has sysconf.),)
AC_CHECK_FUNC(uname, AC_DEFINE(HAVE_UNAME, 1, Defined if system has uname().),)
AC_CHECK_FUNC(killpg, AC_DEFINE(HAVE_KILLPG, 1, Defined if system has killpg().),)
AC_CHECK_FUNC(putenv,, AC_DEFINE(NEED_PUTENV, 1, Defined if system has putenv().))
AC_CHECK_FUNC(getenv,, AC_DEFINE(NEED_GETENV, 1, Defined if system has getenv().))

AC_MSG_CHECKING(for struct linger)
AC_EGREP_HEADER([struct( |	)*linger], sys/socket.h, AC_MSG_RESULT(yes), AC_DEFINE(NO_STRUCT_LINGER, 1, Defined if struct linger is missing.) AC_MSG_RESULT(no, ugh.))


dnl ----------------------------------------------------------
dnl things we need that we have replacements for
dnl ----------------------------------------------------------

needaddr=0

AC_CHECK_FUNC(inet_ntop,, EXTRA_OBJS="$EXTRA_OBJS inet_ntop.o")
AC_CHECK_FUNC(inet_pton,, EXTRA_OBJS="$EXTRA_OBJS inet_pton.o")
AC_CHECK_FUNC(strtoul,,   EXTRA_OBJS="$EXTRA_OBJS strtoul.o")
AC_CHECK_FUNC(setenv,,    EXTRA_OBJS="$EXTRA_OBJS setenv.o")

AC_CHECK_FUNC(freeaddrinfo, AC_DEFINE(HAVE_FREEADDRINFO, 1, Defined if system has freeaddrinfo().), EXTRA_OBJS="$EXTRA_OBJS freeaddrinfo.o")
AC_CHECK_FUNC(gai_strerror, AC_DEFINE(HAVE_GAI_STRERROR, 1, Defined if system has gai_strerror().), EXTRA_OBJS="$EXTRA_OBJS gai_strerror.o")
AC_CHECK_FUNC(getaddrinfo, AC_DEFINE(HAVE_GETADDRINFO, 1, Defined if system has getaddrinfo().),  needaddr=1)
AC_CHECK_FUNC(getnameinfo, AC_DEFINE(HAVE_GETNAMEINFO, 1, Defined if system has getnameinfo().),  EXTRA_OBJS="$EXTRA_OBJS getnameinfo.c gn_ipv46.o")

if test "$needaddr" -eq "1" ; then
	EXTRA_OBJS="$EXTRA_OBJS ga_aistruct.o ga_echeck.o"
	EXTRA_OBJS="$EXTRA_OBJS ga_nsearch.o ga_serv.o" 
	EXTRA_OBJS="$EXTRA_OBJS getaddrinfo.o ga_port.o"
fi

AC_SUBST(EXTRA_OBJS)

dnl ----------------------------------------------------------
dnl check for sys_errlist declaration
dnl ----------------------------------------------------------

AC_CHECK_FUNC(strerror, AC_DEFINE(HAVE_STRERROR, 1, Defined if strerror() exists.))
AC_CHECK_DECL(sys_errlist, AC_DEFINE(SYS_ERRLIST_DECLARED, 1, Defined if sys_errlist is declared.))

dnl ----------------------------------------------------------
dnl Check for sys_siglist declaration
dnl ----------------------------------------------------------

AC_MSG_CHECKING(for sys_siglist declaration)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <signal.h>
main()
{
	char *s = sys_siglist[0];
	exit(0);
}
]])],[AC_MSG_RESULT(yes) 
  sys_siglist="1"
  AC_DEFINE(SYS_SIGLIST_DECLARED, 1, Defined if sys_siglist is declared.)],[AC_MSG_RESULT(no)],[:])


dnl ----------------------------------------------------------
dnl termcap or curses
dnl ----------------------------------------------------------

if test x$hpux = x1 -o x$aix = x1
then
	LIBS="-lcurses $LIBS"
	AC_DEFINE(USING_CURSES, 1, Defined if we are using curses library.)
else
  olibs="$LIBS"
  AC_CHECK_LIB(termcap, tgetent, LIBS="-ltermcap $olibs",
    AC_CHECK_LIB(termlib, tgetent, LIBS="-ltermlib $olibs",
      AC_CHECK_LIB(curses, tgetent, LIBS="-lcurses $olibs",
        AC_CHECK_LIB(xtermcap, tgetent, LIBS="-lxtermcap $olibs",
          AC_MSG_ERROR([i can not find tgetent.  you need to help me])))))
fi

dnl ----------------------------------------------------------
dnl sun_len in <sys/un.h>?
dnl ----------------------------------------------------------

AC_MSG_CHECKING(for sun_len in <sys/un.h>)
AC_EGREP_CPP([sun_len], 
[
#include <sys/un.h>
], AC_DEFINE(HAVE_SUN_LEN, 1, Defined if sockaddr_un has sun_len field.) AC_MSG_RESULT(yes), AC_MSG_RESULT(no))

dnl ----------------------------------------------------------
dnl
dnl svr3's getpgrp() didnt take arguments.  BSD's did.
dnl POSIX said that there wasnt any usefulness to giving arguments
dnl to getpgrp(), so that was forbidden.  So svr4 made a new syscall
dnl getpgid() to fill in this functionality gap (grrrr).  BSD still
dnl doesnt have any way to do it, even though they were the ones that
dnl originally wanted to have it around, but svr3 insisted not so. (grrr)
dnl

AC_CHECK_FUNC(getpgid, AC_DEFINE(HAVE_GETPGID, 1, Defined if you have getpgid.),)
AC_FUNC_GETPGRP

dnl ----------------------------------------------------------
dnl Generate signal list
dnl This is all stolen from perl-4.036's Configure.  larry is god.
dnl ----------------------------------------------------------

if test x$sys_siglist != x1
then
echo "generating a list of signal names..."
set X `cat /usr/include/signal.h /usr/include/sys/signal.h /usr/include/linux/signal.h 2>&1 | sed 's/^#[	]*/#/' | awk '
$1 ~ /^#define$/ && $2 ~ /^SIG[A-Z0-9]*$/ && $3 ~ /^[1-9][0-9]*$/ {
  [sig[$3] = substr($2,4,20)]
  if (max < $3 && $3 < 60) {
	max = $3
  }
}

END {
  for (i=1; i<=max; i++) {
    [if (sig[i] == "")]
      printf "%d", i
    else
      [printf "%s", sig[i]]
    if (i < max)
      printf " "
  }
  printf "\n"
}
'`
shift
case $# in
0)
  if test -f /bin/csh; then
     set X `/bin/csh -cf 'kill -l'`
     shift
     case $* in
     0) 
       set HUP INT QUIT ILL TRAP ABRT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM
       ;;
    esac
  fi
  ;;
esac

sig_name="ZERO $*"
if test -f sig.inc; then
  /bin/rm -f sig.inc
fi
[echo $sig_name | sed -e 's/$/"};/' -e 's/ /", "/g' -e 's/^/char *sys_siglist[] = { "/' >> sig.inc]
fi

dnl ----------------------------------------------------------
dnl OpenSSL!
dnl ----------------------------------------------------------

AC_MSG_CHECKING(for OpenSSL)
for dir in $withval /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr; do
    ssldir="$dir"
    if test -f "$dir/include/openssl/ssl.h"; then
	found_ssl="yes";
	CFLAGS="$CFLAGS -I$ssldir/include -DHAVE_SSL";
	CXXFLAGS="$CXXFLAGS -I$ssldir/include -DHAVE_SSL";
	break;
    fi
    if test -f "$dir/include/ssl.h"; then
	found_ssl="yes";
	CFLAGS="$CFLAGS -I$ssldir/include/ -DHAVE_SSL";
	CXXFLAGS="$CXXFLAGS -I$ssldir/include/ -DHAVE_SSL";
	break
    fi
done
if test x_$found_ssl != x_yes; then
    AC_MSG_ERROR(Cannot find ssl libraries)
else
    printf "OpenSSL found in $ssldir\n";
    LIBS="$LIBS -lssl -lcrypto";
    LDFLAGS="$LDFLAGS -L$ssldir/lib";
    HAVE_SSL=yes
fi
AC_SUBST(HAVE_SSL)

dnl ----------------------------------------------------------
dnl All done!
dnl ----------------------------------------------------------

AC_CACHE_SAVE
AC_CONFIG_FILES([Makefile])
AC_CONFIG_COMMANDS(stamp-h, [date > stamp-h])
AC_OUTPUT

echo ""
echo "There we go! That didn't hurt, did it? You might want to"
echo "look in config.h and see if there is anything"
echo "you want to change, else you can just run make here..."
echo "Good luck..."
echo ""
