/*
 * irc_std.h: header to define things used in all the programs ircii
 * comes with
 *
 * hacked together from various other files by matthew green
 * copyright(c) 1993 
 *
 * See the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id$
 */

#ifndef __irc_std_h
#define __irc_std_h

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#undef _
#undef const
#undef volatile
#ifdef __STDC__
# define _(a) a
#else
# define _(a) ()
# define const
# define volatile
#endif

#ifndef __GNUC__
#define __inline                /* delete gcc keyword */
#define __A(x)
#define __N
#else
#define __A(x)
/*__attribute__ ((format (printf, x, x + 1))) */
#define __N    __attribute__ ((noreturn))
#endif

#if defined (__GNUC__) && !defined(HAVE_ALLOCA_H)
#  ifndef HAVE_ALLOCA_H
#    undef alloca
#    define alloca __builtin_alloca
#  endif
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca
char *alloca();
#   endif
#  endif
# endif
#endif


# include <errno.h>
extern	int	errno;

#ifndef NBBY
# define NBBY	8		/* number of bits in a byte */
#endif /* NBBY */

#ifndef NFDBITS
# define NFDBITS	(sizeof(long) * NBBY)	/* bits per mask */
#endif /* NFDBITS */

#ifndef FD_SET
#define FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#endif

#ifndef FD_CLR
#define FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#endif

#ifndef FD_ISSET
#define FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#endif

#ifndef FD_ZERO
#define FD_ZERO(p)	memset((void *)(p), 0, sizeof(*(p)))
#endif

#ifndef	FD_SETSIZE
#define FD_SETSIZE	32
#endif


#ifdef HAVE_SYS_SYSLIMITS_H
# include <sys/syslimits.h>
#endif
   
#include <limits.h>
   


/* signal handler stuff */
typedef RETSIGTYPE sigfunc(int);
sigfunc *my_signal(int, sigfunc *);

extern volatile int cntl_c_hit;   /* ctl-c was pressed */
extern volatile int got_sigchild; /* .. duh. got sigchild */


#include <string.h>
#include <stdlib.h>
#undef index
#define index strchr

#ifndef MAXPATHLEN
#define MAXPATHLEN  PATHSIZE
#endif

#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DECLARED
extern  char    *sys_errlist[];
#endif
#define strerror(x) sys_errlist[x]
#endif

#ifdef HAVE_BSDGETTIMEOFDAY
#define gettimeofday BSDgettimeofday
#endif

#ifdef GETTOD_NOT_DECLARED
extern	int	gettimeofday(struct timeval *tv, struct timezone *tz);
#endif


/* we need an unsigned 32 bit integer for dcc, how lame */

#ifdef UNSIGNED_LONG32

typedef		unsigned long		u_32int_t;

#else
# ifdef UNSIGNED_INT32

typedef		unsigned int		u_32int_t;

# else

typedef		unsigned long		u_32int_t;

# endif /* UNSIGNED_INT32 */
#endif /* UNSIGNED_LONG32 */

#ifdef __STDC__
#define BUILT_IN_COMMAND(x) \
	void x (char *command, char *args, char *subargs, char *helparg)
#else
#define BUILT_IN_COMMAND(x) \
	void x (command, args, subargs, helparg) char *command, *args, *subargs, *helparg;
#endif


#if defined(_AIX)
int getpeername(int s, struct sockaddr *, int *);
int getsockname(int s, struct sockaddr *, int *);
int socket(int, int, int);
int bind(int, struct sockaddr *, int);
int listen(int, int);
int accept(int, struct sockaddr *, int *);
int recv(int, void *, int, unsigned int);
int send(int, void *, int, unsigned int);
int gettimeofday(struct timeval *, struct timezone *);
int gethostname(char *, int);
int setsockopt(int, int, int, void *, int);
int setitimer(int, struct itimerval *, struct itimerval *);
int ioctl(int, int, ...);

#endif

#endif /* __irc_std_h */

