dnl ##
dnl ##  GNU Pth - The GNU Portable Threads
dnl ##  Copyright (c) 1999-2000 Ralf S. Engelschall <rse@engelschall.com>
dnl ##
dnl ##  This file is part of GNU Pth, a non-preemptive thread scheduling
dnl ##  library which can be found at http://www.gnu.org/software/pth/.
dnl ##
dnl ##  This library is free software; you can redistribute it and/or
dnl ##  modify it under the terms of the GNU Lesser General Public
dnl ##  License as published by the Free Software Foundation; either
dnl ##  version 2.1 of the License, or (at your option) any later version.
dnl ##
dnl ##  This library is distributed in the hope that it will be useful,
dnl ##  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl ##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
dnl ##  Lesser General Public License for more details.
dnl ##
dnl ##  You should have received a copy of the GNU Lesser General Public
dnl ##  License along with this library; if not, write to the Free Software
dnl ##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl ##  USA, or contact Ralf S. Engelschall <rse@engelschall.com>.
dnl ##
dnl ##
dnl ##
dnl ## shamelessly stolen for use in Xaric.. 
dnl ##
dnl ## @(#)acinclude.m4 1.3
dnl ##

divert(-1)

dnl ##
dnl ##  Display Configuration Headers
dnl ##
dnl ##  configure.in:
dnl ##    AC_MSG_PART(<text>)
dnl ##

define(AC_MSG_PART,[dnl
    AC_MSG_RESULT()
    AC_MSG_RESULT(${TB}$1:${TN})
])dnl

dnl ##
dnl ##  Display a message under --verbose
dnl ##
dnl ##  configure.in:
dnl ##    AC_MSG_VERBOSE(<text>)
dnl ##

define(AC_MSG_VERBOSE,[dnl
if test ".$verbose" = .yes; then
    AC_MSG_RESULT([  $1])
fi
])dnl

dnl ## Check to make sure that the build environment is sane.

AC_DEFUN(AC_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)

# bleah. sane!
AM_MISSING_PROG(ACLOCAL, aclocal, $srcdir)
AM_MISSING_PROG(AUTOCONF, autoconf, $srcdir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $srcdir)])

dnl ## AC_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl ## The program must properly implement --version.

AC_DEFUN(AC_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])


dnl ##
dnl ##  Support for Configuration Headers
dnl ##
dnl ##  configure.in:
dnl ##    AC_HEADLINE(<short-name>, <long-name>,
dnl ##                <vers-var>, <vers-file>,
dnl ##                <copyright>)
dnl ##

AC_DEFUN(AC_HEADLINE,[dnl
AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)dnl
#   configuration header
if test ".`echo dummy [$]@ | grep enable-subdir`" != .; then
    enable_subdir=yes
fi
if test ".`echo dummy [$]@ | grep help`" = .; then
    #   bootstrapping shtool
    ac_prog=[$]0
changequote(, )dnl
    ac_srcdir=`echo $ac_prog | sed -e 's%/[^/][^/]*$%%' -e 's%\([^/]\)/*$%\1%'`
changequote([, ])dnl
    test ".$ac_srcdir" = ".$ac_prog" && ac_srcdir=.
    ac_shtool="${CONFIG_SHELL-/bin/sh} $ac_srcdir/shtool"

    #   find out terminal sequences
    if test ".$enable_subdir" != .yes; then
        TB=`$ac_shtool echo -n -e %B 2>/dev/null`
        TN=`$ac_shtool echo -n -e %b 2>/dev/null`
    else
        TB=''
        TN=''
    fi

    #   find out package version
    $3_STR="`$ac_shtool version -l c -d long $ac_srcdir/$4`"
    AC_SUBST($3_STR)

    #   friendly header ;)
    if test ".$enable_subdir" != .yes; then
        echo "Configuring ${TB}$1${TN} ($2), Version ${TB}${$3_STR}${TN}"
        echo "$5"
    fi

    #   additionally find out hex version
    $3_HEX="`$ac_shtool version -l c -d hex $ac_srcdir/$4`"
    AC_SUBST($3_HEX)
fi
AC_DIVERT_POP()
])dnl


dnl ##
dnl ##  Support for Platform IDs
dnl ##
dnl ##  configure.in:
dnl ##    AC_PLATFORM(<variable>)
dnl ##

AC_DEFUN(AC_PLATFORM,[
if test ".$host" != .NONE; then
    $1="$host"
elif test ".$nonopt" != .NONE; then
    $1="$nonopt"
else
    $1=`${CONFIG_SHELL-/bin/sh} $srcdir/config.guess`
fi
$1=`${CONFIG_SHELL-/bin/sh} $srcdir/config.sub $$1` || exit 1
AC_SUBST($1)
if test ".$enable_subdir" != .yes; then
    echo "Platform: ${TB}${$1}${TN}"
fi
])dnl

dnl ##
dnl ##  Support for config.param files
dnl ##
dnl ##  configure.in:
dnl ##    AC_CONFIG_PARAM(<file>)
dnl ##

AC_DEFUN(AC_CONFIG_PARAM,[
AC_DIVERT_PUSH(-1)
AC_ARG_WITH(param,[  --with-param=ID[,ID,..] load parameters from $1])
AC_DIVERT_POP()
AC_DIVERT_PUSH(AC_DIVERSION_NOTICE)
ac_prev=""
ac_param=""
if test -f $1; then
    ac_param="$1:common"
fi
for ac_option
do
    if test ".$ac_prev" != .; then
        eval "$ac_prev=\$ac_option"
        ac_prev=""
        continue
    fi
    case "$ac_option" in
        -*=*) ac_optarg=`echo "$ac_option" | sed 's/[[-_a-zA-Z0-9]]*=//'` ;;
           *) ac_optarg="" ;;
    esac
    case "$ac_option" in
        --with-param=* )
            case $ac_optarg in
                *:* )
                    ac_from=`echo $ac_optarg | sed -e 's/:.*//'`
                    ac_what=`echo $ac_optarg | sed -e 's/.*://'`
                    ;;
                * )
                    ac_from="$1"
                    ac_what="$ac_optarg"
                    ;;
            esac
            if test ".$ac_param" = .; then
                ac_param="$ac_from:$ac_what"
            else
                ac_param="$ac_param,$ac_from:$ac_what"
            fi
            ;;
    esac
done
if test ".$ac_param" != .; then
    # echo "loading parameters"
    OIFS="$IFS"
    IFS=","
    pconf="/tmp/autoconf.$$"
    echo "ac_options=''" >$pconf
    ac_from="$1"
    for ac_section in $ac_param; do
        changequote(, )
        case $ac_section in
            *:* )
                ac_from=`echo "$ac_section" | sed -e 's/:.*//'`
                ac_section=`echo "$ac_section" | sed -e 's/.*://'`
                ;;
        esac
        (echo ''; cat $ac_from; echo '') |\
        sed -e "1,/[ 	]*[ 	]*${ac_section}[ 	]*{[ 	]*/d" \
            -e '/[ 	]*}[ 	]*/,$d' \
            -e ':join' -e '/\\[ 	]*$/N' -e 's/\\[ 	]*\n[ 	]*//' -e 'tjoin' \
            -e 's/^[ 	]*//g' \
            -e 's/^\(--.*=.*\)$/ac_options="$ac_options \1"/' \
            -e 's/^\(--.*\)$/ac_options="$ac_options \1"/' \
            >>$pconf
        changequote([, ])
    done
    IFS="$OIFS"
    . $pconf
    rm -f $pconf >/dev/null 2>&1
    if test ".[$]*" = .; then
        set -- $ac_options
    else
        set -- "[$]@" $ac_options
    fi
fi
AC_DIVERT_POP()
])dnl

dnl ##
dnl ##  Check whether compiler option works
dnl ##
dnl ##  configure.in:
dnl ##    AC_COMPILER_OPTION(<name>, <display>, <option>,
dnl ##                       <action-success>, <action-failure>)
dnl ##

AC_DEFUN(AC_COMPILER_OPTION,[dnl
AC_MSG_CHECKING(whether compiler option(s) $2 work)
AC_CACHE_VAL(ac_cv_compiler_option_$1,[
SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $3"
AC_TRY_COMPILE([],[], ac_cv_compiler_option_$1=yes, ac_cv_compiler_option_$1=no)
CFLAGS="$SAVE_CFLAGS"
])dnl
if test ".$ac_cv_compiler_option_$1" = .yes; then
    ifelse([$4], , :, [$4])
else
    ifelse([$5], , :, [$5])
fi
AC_MSG_RESULT([$ac_cv_compiler_option_$1])
])dnl

dnl ##
dnl ##  Minimalistic Libtool Glue Code
dnl ##
dnl ##  configure.in:
dnl ##    AC_PROG_LIBTOOL(<platform-variable>)
dnl ##

AC_DEFUN(AC_PROG_LIBTOOL,[dnl
AC_ARG_ENABLE(static,dnl
[  --enable-static         build static libraries (default=yes)],
enable_static="$enableval",
if test ".$enable_static" = .; then
    enable_static=yes
fi
)dnl
AC_ARG_ENABLE(shared,dnl
[  --enable-shared         build shared libraries (default=yes)],
enable_shared="$enableval",
if test ".$enable_shared" = .; then
    enable_shared=yes
fi
)dnl
libtool_flags=''
dnl libtool_flags="$libtool_flags --cache-file=$cache_file"
test ".$silent"            = .yes && libtool_flags="$libtool_flags --silent"
test ".$enable_static"     = .no  && libtool_flags="$libtool_flags --disable-static"
test ".$enable_shared"     = .no  && libtool_flags="$libtool_flags --disable-shared"
test ".$ac_cv_prog_gcc"    = .yes && libtool_flags="$libtool_flags --with-gcc"
test ".$ac_cv_prog_gnu_ld" = .yes && libtool_flags="$libtool_flags --with-gnu-ld"
CC="$CC" CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LD="$LD" \
${CONFIG_SHELL-/bin/sh} $srcdir/ltconfig --no-reexec \
$libtool_flags --srcdir=$srcdir --no-verify $srcdir/ltmain.sh $1 ||\
AC_MSG_ERROR([libtool configuration failed])
dnl (AC_CACHE_LOAD) >/dev/null 2>&1
])dnl

dnl ##
dnl ##  Debugging Support
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_DEBUGGING
dnl ##

AC_DEFUN(AC_CHECK_DEBUGGING,[dnl
AC_ARG_ENABLE(debug,dnl
[  --enable-debug          build for debugging (default=no)],
[dnl
if test ".$ac_cv_prog_gcc" = ".yes"; then
    case "$CFLAGS" in
        *-O* ) ;;
           * ) CFLAGS="$CFLAGS -O2" ;;
    esac
    case "$CFLAGS" in
        *-g* ) ;;
           * ) CFLAGS="$CFLAGS -g" ;;
    esac
    case "$CFLAGS" in
        *-pipe* ) ;;
              * ) AC_COMPILER_OPTION(pipe, -pipe, -pipe, CFLAGS="$CFLAGS -pipe") ;;
    esac
    AC_COMPILER_OPTION(ggdb3, -ggdb3, -ggdb3, CFLAGS="$CFLAGS -ggdb3")
    case $PLATFORM in
        *-*-freebsd*|*-*-solaris* ) CFLAGS="$CFLAGS -pedantic" ;;
    esac
    CFLAGS="$CFLAGS -Wall"
    WMORE="-Wshadow -Wpointer-arith -Wcast-align -Winline"
    WMORE="$WMORE -Wmissing-prototypes -Wmissing-declarations -Wnested-externs"
    AC_COMPILER_OPTION(wmore, -W<xxx>, $WMORE, CFLAGS="$CFLAGS $WMORE")
else
    case "$CFLAGS" in
        *-g* ) ;;
           * ) CFLAGS="$CFLAGS -g" ;;
    esac
fi
msg="enabled"
AC_DEFINE(XARIC_DEBUG)
],[
if test ".$ac_cv_prog_gcc" = ".yes"; then
case "$CFLAGS" in
    *-pipe* ) ;;
          * ) AC_COMPILER_OPTION(pipe, -pipe, -pipe, CFLAGS="$CFLAGS -pipe") ;;
esac
fi
case "$CFLAGS" in
    *-g* ) CFLAGS=`echo "$CFLAGS" |\
                   sed -e 's/ -g / /g' -e 's/ -g$//' -e 's/^-g //g' -e 's/^-g$//'` ;;
esac
case "$CXXFLAGS" in
    *-g* ) CXXFLAGS=`echo "$CXXFLAGS" |\
                     sed -e 's/ -g / /g' -e 's/ -g$//' -e 's/^-g //g' -e 's/^-g$//'` ;;
esac
msg="disabled"
])dnl
AC_MSG_CHECKING(for compilation debug mode)
AC_MSG_RESULT([$msg])
if test ".$msg" = .enabled; then
    enable_shared=no
fi
])

dnl ##
dnl ##  Profiling Support
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_PROFILING
dnl ##

AC_DEFUN(AC_CHECK_PROFILING,[dnl
AC_MSG_CHECKING(for compilation profile mode)
AC_ARG_ENABLE(profile,dnl
[  --enable-profile        build for profiling (default=no)],
[dnl
if test ".$ac_cv_prog_gcc" = ".no"; then
    AC_MSG_ERROR([profiling requires gcc and gprof])
fi
CFLAGS=`echo "$CFLAGS" | sed -e 's/-O2//g'`
CFLAGS="$CFLAGS -O0 -pg"
LDFLAGS="$LDFLAGS -pg"
msg="enabled"
],[
msg="disabled"
])dnl
AC_MSG_RESULT([$msg])
if test ".$msg" = .enabled; then
    enable_shared=no
fi
])

dnl ##
dnl ##  Build Parameters
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_BUILDPARAM
dnl ##

AC_DEFUN(AC_CHECK_BUILDPARAM,[dnl
dnl #   determine build mode
AC_MSG_CHECKING(whether to activate batch build mode)
AC_ARG_ENABLE(batch,dnl
[  --enable-batch          enable batch build mode (default=no)],[dnl
],[dnl
enable_batch=no
])dnl
if test ".$silent" = .yes; then
    enable_batch=yes
fi
AC_MSG_RESULT([$enable_batch])
BATCH="$enable_batch"
AC_SUBST(BATCH)
dnl #   determine build targets
TARGET_ALL='$(TARGET_PREQ) $(TARGET_LIBS)'
AC_MSG_CHECKING(whether to activate maintainer build targets)
AC_ARG_ENABLE(maintainer,dnl
[  --enable-maintainer     enable maintainer build targets (default=no)],[dnl
],[dnl
enable_maintainer=no
])dnl
AC_MSG_RESULT([$enable_maintainer])
if test ".$enable_maintainer" = .yes; then
    TARGET_ALL="$TARGET_ALL \$(TARGET_MANS)"
fi
AC_MSG_CHECKING(whether to activate test build targets)
AC_ARG_ENABLE(tests,dnl
[  --enable-tests          enable test build targets (default=yes)],[dnl
],[dnl
enable_tests=yes
])dnl
AC_MSG_RESULT([$enable_tests])
if test ".$enable_tests" = .yes; then
    TARGET_ALL="$TARGET_ALL \$(TARGET_TEST)"
fi
AC_SUBST(TARGET_ALL)
])

dnl ##
dnl ##  Optimization Support
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_OPTIMIZE
dnl ##

AC_DEFUN(AC_CHECK_OPTIMIZE,[dnl
AC_ARG_ENABLE(optimize,dnl
[  --enable-optimize       build with optimization (default=no)],
[dnl
if test ".$ac_cv_prog_gcc" = ".yes"; then
    case "$CFLAGS" in
        *-O* ) ;;
        * ) CFLAGS="$CFLAGS -O2" ;;
    esac
    case "$CFLAGS" in
        *-pipe* ) ;;
        * ) AC_COMPILER_OPTION(pipe, -pipe, -pipe, CFLAGS="$CFLAGS -pipe") ;;
    esac
    OPT_CFLAGS='-funroll-loops -fstrength-reduce -fomit-frame-pointer -ffast-math'
    AC_COMPILER_OPTION(optimize_std, [-f<xxx> for optimizations], $OPT_CFLAGS, CFLAGS="$CFLAGS $OPT_CFLAGS")
    case $PLATFORM in
        i?86*-*-*|?86*-*-* )
            OPT_CFLAGS='-malign-functions=4 -malign-jumps=4 -malign-loops=4' 
            AC_COMPILER_OPTION(optimize_x86, [-f<xxx> for Intel x86 CPU], $OPT_CFLAGS, CFLAGS="$CFLAGS $OPT_CFLAGS")
            ;;
    esac
else
    case "$CFLAGS" in
        *-O* ) ;;
           * ) CFLAGS="$CFLAGS -O" ;;
    esac
fi
msg="enabled"
],[
msg="disabled"
])dnl
AC_MSG_CHECKING(for compilation optimization mode)
AC_MSG_RESULT([$msg])
])

dnl ##
dnl ##  Check for a pre-processor define in a header
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_DEFINE(<define>, <header>)
dnl ##  acconfig.h:
dnl ##    #undef HAVE_<define>
dnl ##

AC_DEFUN(AC_CHECK_DEFINE,[dnl
AC_MSG_CHECKING(for define $1 in $2)
AC_CACHE_VAL(ac_cv_define_$1,
[AC_EGREP_CPP([YES_IS_DEFINED], [
#include <$2>
#ifdef $1
YES_IS_DEFINED
#endif
], ac_cv_define_$1=yes, ac_cv_define_$1=no)])dnl
AC_MSG_RESULT($ac_cv_define_$1)
if test $ac_cv_define_$1 = yes; then
    AC_DEFINE(HAVE_[]translit($1, [a-z], [A-Z]))
fi
])

dnl ##
dnl ##  Check for an ANSI C typedef in a header
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_TYPEDEF(<typedef>, <header>)
dnl ##  acconfig.h:
dnl ##    #undef HAVE_<typedef>
dnl ##

AC_DEFUN(AC_CHECK_TYPEDEF,[dnl
AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for typedef $1)
AC_CACHE_VAL(ac_cv_typedef_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [
#include <$2>
], ac_cv_typedef_$1=yes, ac_cv_typedef_$1=no)])dnl
AC_MSG_RESULT($ac_cv_typedef_$1)
if test $ac_cv_typedef_$1 = yes; then
    AC_DEFINE(HAVE_[]translit($1, [a-z], [A-Z]))
fi
])

dnl ##
dnl ##  Check for an ANSI C struct attribute in a structure defined in a header
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_STRUCTATTR(<attr>, <struct>, <header>)
dnl ##  acconfig.h:
dnl ##    #undef HAVE_<attr>
dnl ##

AC_DEFUN(AC_CHECK_STRUCTATTR,[dnl
AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for attribute $1 in struct $2 from $3)
AC_CACHE_VAL(ac_cv_structattr_$1,[dnl

AC_TRY_LINK([
#include <sys/types.h>
#include <$3>
],[
struct $2 *sp1;
struct $2 *sp2;
sp1->$1 = sp2->$1;
], ac_cv_structattr_$1=yes, ac_cv_structattr_$1=no)])dnl
AC_MSG_RESULT($ac_cv_structattr_$1)
if test $ac_cv_structattr_$1 = yes; then
    AC_DEFINE(HAVE_[]translit($1, [a-z], [A-Z]))
fi
])

dnl ##
dnl ##  Check for argument type of a function
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_ARGTYPE(<header> [...], <func>, <arg-number>,
dnl ##                     <max-arg-number>, <action-with-${ac_type}>)
dnl ##

AC_DEFUN(AC_CHECK_ARGTYPE,[dnl
AC_REQUIRE_CPP()dnl
AC_MSG_CHECKING([for type of argument $3 for $2()])
AC_CACHE_VAL([ac_cv_argtype_$2$3],[
cat >conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
EOF
for ifile in $1; do
    echo "#include <$ifile>" >>conftest.$ac_ext
done
gpat=''
spat=''
i=1
changequote(, )dnl
while test $i -le $4; do
    gpat="$gpat[^,]*"
    if test $i -eq $3; then
        spat="$spat\\([^,]*\\)"
    else
        spat="$spat[^,]*"
    fi
    if test $i -lt $4; then
        gpat="$gpat,"
        spat="$spat,"
    fi
    i=`expr $i + 1`
done
(eval "$ac_cpp conftest.$ac_ext") 2>&AC_FD_CC |\
sed -e ':join' \
    -e '/,[ 	]*$/N' \
    -e 's/,[ 	]*\n[ 	]*/, /' \
    -e 'tjoin' |\
egrep "[^a-zA-Z0-9_]$2[ 	]*\\($gpat\\)" | head -1 |\
sed -e "s/.*[^a-zA-Z0-9_]$2[ 	]*($spat).*/\\1/" \
    -e 's/(\*[a-zA-Z_][a-zA-Z_0-9]*)/(*)/' \
    -e 's/^[ 	]*//' -e 's/[ 	]*$//' \
    -e 's/^/arg:/' \
    -e 's/^arg:\([^ 	]*\)$/type:\1/' \
    -e 's/^arg:\(.*_t\)*$/type:\1/' \
    -e 's/^arg:\(.*\*\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]\*\)[_a-zA-Z][_a-zA-Z0-9]*$/type:\1/' \
    -e 's/^arg:\(.*[ 	]char\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]short\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]int\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]long\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]float\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]double\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]unsigned\)$/type:\1/' \
    -e 's/^arg:\(.*[ 	]signed\)$/type:\1/' \
    -e 's/^arg:\(.*struct[ 	][_a-zA-Z][_a-zA-Z0-9]*\)$/type:\1/' \
    -e 's/^arg:\(.*\)[ 	]_[_a-zA-Z0-9]*$/type:\1/' \
    -e 's/^arg:\(.*\)[ 	]\([^ 	]*\)$/type:\1/' \
    -e 's/^type://' >conftest.output
ac_cv_argtype_$2$3=`cat conftest.output`
changequote([, ])dnl
rm -f conftest*
])
AC_MSG_RESULT([$ac_cv_argtype_$2$3])
ac_type="$ac_cv_argtype_$2$3"
[$5]
])

dnl ##
dnl ##  Check for existance of a function
dnl ##
dnl ##  configure.in:
dnl ##     AC_CHECK_FUNCTION(<function> [, <action-if-found> [, <action-if-not-found>]])
dnl ##     AC_CHECK_FUNCTIONS(<function> [...] [, <action-if-found> [, <action-if-not-found>]])
dnl ##

AC_DEFUN(AC_CHECK_FUNCTIONS,[dnl
for ac_func in $1; do
    AC_CHECK_FUNCTION($ac_func,
        [changequote(, )dnl
        ac_tr_func=HAVE_`echo $ac_func | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
        changequote([, ])dnl
        AC_DEFINE_UNQUOTED($ac_tr_func) $2], $3)dnl
done
])

AC_DEFUN(AC_CHECK_FUNCTION, [dnl
AC_MSG_CHECKING([for function $1])
AC_CACHE_VAL(ac_cv_func_$1, [dnl
AC_TRY_LINK([
/* System header to define __stub macros and hopefully few prototypes,
   which can conflict with char $1(); below. */
#include <assert.h>
#ifdef __cplusplus
extern "C"
#endif
/* We use char because int might match the return type of a gcc2
   builtin and then its argument prototype would still apply. */
char $1();
char (*f)();
], [
/* The GNU C library defines this for functions which it implements
   to always fail with ENOSYS.  Some functions are actually named
   something starting with __ and the normal name is an alias. */
#if defined (__stub_$1) || defined (__stub___$1)
choke me
#else
f = $1;
#endif
], eval "ac_cv_func_$1=yes", eval "ac_cv_func_$1=no")])
if eval "test \"`echo '$ac_cv_func_'$1`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3
])dnl
fi
])

dnl ##
dnl ##  Decision Hierachy
dnl ##

define(AC_IFALLYES,[dnl
ac_rc=yes
for ac_spec in $1; do
    ac_type=`echo "$ac_spec" | sed -e 's/:.*$//'`
    ac_item=`echo "$ac_spec" | sed -e 's/^.*://'`
    case $ac_type in
        header [)]
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_header_$ac_item"
            ;;
        file [)]
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_file_$ac_item"
            ;;
        func    [)] ac_var="ac_cv_func_$ac_item"   ;;
        lib     [)] ac_var="ac_cv_lib_$ac_item"    ;;
        define  [)] ac_var="ac_cv_define_$ac_item" ;;
        typedef [)] ac_var="ac_cv_typedef_$ac_item" ;;
        custom  [)] ac_var="$ac_item" ;;
    esac
    eval "ac_val=\$$ac_var"
    if test ".$ac_val" != .yes; then
        ac_rc=no
        break
    fi
done
if test ".$ac_rc" = .yes; then
    :
    $2
else
    :
    $3
fi
])

define(AC_BEGIN_DECISION,[dnl
ac_decision_item='$1'
ac_decision_msg='FAILED'
ac_decision=''
])

define(AC_DECIDE,[dnl
ac_decision='$1'
ac_decision_msg='$2'
ac_decision_$1=yes
ac_decision_$1_msg='$2'
])

define(AC_DECISION_OVERRIDE,[dnl
    ac_decision=''
    for ac_item in $1; do
         eval "ac_decision_this=\$ac_decision_${ac_item}"
         if test ".$ac_decision_this" = .yes; then
             ac_decision=$ac_item
             eval "ac_decision_msg=\$ac_decision_${ac_item}_msg"
         fi
    done
])

define(AC_DECISION_FORCE,[dnl
ac_decision="$1"
eval "ac_decision_msg=\"\$ac_decision_${ac_decision}_msg\""
])

define(AC_END_DECISION,[dnl
if test ".$ac_decision" = .; then
    echo "[$]0:Error: decision on $ac_decision_item failed." 1>&2
    echo "[$]0:Hint: see config.log for more details!" 1>&2
    exit 1
else
    if test ".$ac_decision_msg" = .; then
        ac_decision_msg="$ac_decision"
    fi
    AC_MSG_RESULT([decision on $ac_decision_item... ${TB}$ac_decision_msg${TN}])
fi
])

dnl ##
dnl ##  Check for existance of a file
dnl ##
dnl ##  configure.in:
dnl ##    AC_TEST_FILE(<file>, <success-action>, <failure-action>)
dnl ##

AC_DEFUN(AC_TEST_FILE, [
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_file_$ac_safe, [
if test -r $1; then
    eval "ac_cv_file_$ac_safe=yes"
else
    eval "ac_cv_file_$ac_safe=no"
fi
])dnl
if eval "test \"`echo '$ac_cv_file_'$ac_safe`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
else
    AC_MSG_RESULT(no)
    ifelse([$3], , :, [$3])
fi
])

dnl ##
dnl ##  Check for number of signals (NSIG)
dnl ##
dnl ##  configure.in:
dnl ##    AC_CHECK_NSIG(<define>)
dnl ##  acconfig.h:
dnl ##    #undef <define>
dnl ##  source.c:
dnl ##    #include "config.h"
dnl ##    ...<define>...

AC_DEFUN(AC_CHECK_NSIG,[dnl
AC_MSG_CHECKING(for number of signals)
cross_compile=no
AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    int nsig;

#if defined(NSIG)
    nsig = NSIG;
#elif defined(_NSIG)
    nsig = _NSIG;
#else
    nsig = (sizeof(sigset_t)*8);
    if (nsig < 32)
        nsig = 32;
#endif
    if ((fp = fopen("conftestval", "w")) == NULL)
        exit(1);
    fprintf(fp, "%d\n", nsig);
    fclose(fp);
    exit(0);
}
>>
changequote([, ])dnl
,
nsig=`cat conftestval`,
nsig=32,
nsig=32
)dnl
AC_MSG_RESULT([$nsig])
AC_DEFINE_UNQUOTED($1, $nsig)
])

dnl ##
dnl ##  Check for an external/extension library.
dnl ##  - is aware of <libname>-config style scripts
dnl ##  - searches under standard paths include, lib, etc.
dnl ##  - searches under subareas like .libs, etc.
dnl ##
dnl ##  configure.in:
dnl ##      AC_CHECK_EXTLIB(<realname>, <libname>, <func>, <header>,
dnl ##                      [<success-action> [, <fail-action>]])
dnl ##  Makefile.in:
dnl ##      CFLAGS  = @CFLAGS@
dnl ##      LDFLAGS = @LDFLAGS@
dnl ##      LIBS    = @LIBS@
dnl ##  shell:
dnl ##      $ ./configure --with-<libname>[=DIR]
dnl ##

AC_DEFUN(AC_CHECK_EXTLIB,[dnl
AC_ARG_WITH($2,dnl
[  --with-]substr([$2[[=DIR]]                 ], 0, 19)[build against $1 library (default=no)],
    if test ".$with_$2" = .yes; then
        #   via config script
        $2_version=`($2-config --version) 2>/dev/null`
        if test ".$$2_version" != .; then
            CPPFLAGS="$CPPFLAGS `$2-config --cflags`"
            CFLAGS="$CFLAGS `$2-config --cflags`"
            LDFLAGS="$LDFLAGS `$2-config --ldflags`"
        fi
    else
        if test -d "$with_$2"; then
            found=0
            #   via config script
            for dir in $with_$2/bin $with_$2; do
                if test -f "$dir/$2-config"; then
                    $2_version=`($dir/$2-config --version) 2>/dev/null`
                    if test ".$$2_version" != .; then
                        CPPFLAGS="$CPPFLAGS `$dir/$2-config --cflags`"
                        CFLAGS="$CFLAGS `$dir/$2-config --cflags`"
                        LDFLAGS="$LDFLAGS `$dir/$2-config --ldflags`"
                        found=1
                        break
                    fi
                fi
            done
            #   via standard paths
            if test ".$found" = .0; then
                for dir in $with_$2/include/$2 $with_$2/include $with_$2; do
                    if test -f "$dir/$4"; then
                        CPPFLAGS="$CPPFLAGS -I$dir"
                        CFLAGS="$CFLAGS -I$dir"
                        found=1
                        break
                    fi
                done
                for dir in $with_$2/lib/$2 $with_$2/lib $with_$2; do
                    if test -f "$dir/lib$2.a" -o -f "$dir/lib$2.so"; then
                        LDFLAGS="$LDFLAGS -L$dir"
                        found=1
                        break
                    fi
                done
            fi
            #   in any subarea
            if test ".$found" = .0; then
changequote(, )dnl
                for file in x `find $with_$2 -name "$4" -type f -print`; do
                    test .$file = .x && continue
                    dir=`echo $file | sed -e 's;[[^/]]*$;;' -e 's;\(.\)/$;\1;'`
                    CPPFLAGS="$CPPFLAGS -I$dir"
                    CFLAGS="$CFLAGS -I$dir"
                done
                for file in x `find $with_$2 -name "lib$2.[[aso]]" -type f -print`; do
                    test .$file = .x && continue
                    dir=`echo $file | sed -e 's;[[^/]]*$;;' -e 's;\(.\)/$;\1;'`
                    LDFLAGS="$LDFLAGS -L$dir"
                done
changequote([, ])dnl
            fi
        fi
    fi
    AC_HAVE_HEADERS($4)
    AC_CHECK_LIB($2, $3)
    AC_IFALLYES(header:$4 lib:$2_$3, with_$2=yes, with_$2=no)
    if test ".$with_$2" = .no; then
        AC_ERROR([Unable to find $1 library])
    fi
,
if test ".$with_$2" = .; then
    with_$2=no
fi
)dnl
AC_MSG_CHECKING(whether to build against $1 library)
if test ".$with_$2" = .yes; then
    ifelse([$5], , :, [$5])
else
    ifelse([$6], , :, [$6])
fi
AC_MSG_RESULT([$with_$2])
])dnl


divert


