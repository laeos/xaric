/*
**  OSSP sa - Socket Abstraction
**  Copyright (c) 2001-2005 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2001-2005 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2001-2005 Cable & Wireless <http://www.cw.com/>
**
**  This file is part of OSSP sa, a socket abstraction library which
**  can be found at http://www.ossp.org/pkg/lib/sa/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  sa.c: socket abstraction library
*/

/* include optional Autoconf header */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* include system API headers */
#include <stdio.h>       /* for "s[n]printf()" */
#include <stdlib.h>      /* for "malloc()" & friends */
#include <stdarg.h>      /* for "va_XXX()" and "va_list" */
#include <string.h>      /* for "strXXX()" and "size_t" */
#include <sys/types.h>   /* for general prerequisites */
#include <ctype.h>       /* for "isXXX()" */
#include <errno.h>       /* for "EXXX" */
#include <fcntl.h>       /* for "F_XXX" and "O_XXX" */
#include <unistd.h>      /* for standard Unix stuff */
#include <netdb.h>       /* for "struct prototent" */
#include <sys/time.h>    /* for "struct timeval" */
#include <sys/un.h>      /* for "struct sockaddr_un" */
#include <netinet/in.h>  /* for "struct sockaddr_in[6]" */
#include <sys/socket.h>  /* for "PF_XXX", "AF_XXX", "SOCK_XXX" and "SHUT_XX" */
#include <arpa/inet.h>   /* for "inet_XtoX" */

/* include own API header */
#include "sa.h"

/* unique library identifier */
const char sa_id[] = "OSSP sa";

/* support for OSSP ex based exception throwing */
#ifdef WITH_EX
#include "ex.h"
#define SA_RC(rv) \
    (  (rv) != SA_OK && (ex_catching && !ex_shielding) \
     ? (ex_throw(sa_id, NULL, (rv)), (rv)) : (rv) )
#else
#define SA_RC(rv) (rv)
#endif /* WITH_EX */

/* boolean values */
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE  (!FALSE)
#endif

/* backward compatibility for AF_LOCAL */
#if !defined(AF_LOCAL) && defined(AF_UNIX)
#define AF_LOCAL AF_UNIX
#endif

/* backward compatibility for PF_XXX (still unused) */
#if !defined(PF_LOCAL) && defined(AF_LOCAL)
#define PF_LOCAL AF_LOCAL
#endif
#if !defined(PF_INET) && defined(AF_INET)
#define PF_INET AF_INET
#endif
#if !defined(PF_INET6) && defined(AF_INET6)
#define PF_INET6 AF_INET6
#endif

/* backward compatibility for SHUT_XX. Some platforms (like brain-dead
   OpenUNIX) define those only if _XOPEN_SOURCE is defined, but unfortunately
   then fail in their other vendor includes due to internal inconsistencies. */
#if !defined(SHUT_RD)
#define SHUT_RD 0
#endif
#if !defined(SHUT_WR)
#define SHUT_WR 1
#endif
#if !defined(SHUT_RDWR)
#define SHUT_RDWR 2
#endif

/* backward compatibility for ssize_t */
#if defined(HAVE_CONFIG_H) && !defined(HAVE_SSIZE_T)
#define ssize_t long
#endif

/* backward compatibility for O_NONBLOCK */
#if !defined(O_NONBLOCK) && defined(O_NDELAY)
#define O_NONBLOCK O_NDELAY
#endif

/* system call structure declaration macros */
#define SA_SC_DECLARE_0(rc_t, fn) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(void); \
                rc_t (*ctx)(void *); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_1(rc_t, fn, a1_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t); \
                rc_t (*ctx)(void *, a1_t); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_2(rc_t, fn, a1_t, a2_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t, a2_t); \
                rc_t (*ctx)(void *, a1_t, a2_t); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_3(rc_t, fn, a1_t, a2_t, a3_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t, a2_t, a3_t); \
                rc_t (*ctx)(void *, a1_t, a2_t, a3_t); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_4(rc_t, fn, a1_t, a2_t, a3_t, a4_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t, a2_t, a3_t, a4_t); \
                rc_t (*ctx)(void *, a1_t, a2_t, a3_t, a4_t); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_5(rc_t, fn, a1_t, a2_t, a3_t, a4_t, a5_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t, a2_t, a3_t, a4_t, a5_t); \
                rc_t (*ctx)(void *, a1_t, a2_t, a3_t, a4_t, a5_t); } fptr; \
        void *fctx; \
    } sc_##fn;
#define SA_SC_DECLARE_6(rc_t, fn, a1_t, a2_t, a3_t, a4_t, a5_t, a6_t) \
    struct { \
        union { void (*any)(void); \
                rc_t (*std)(a1_t, a2_t, a3_t, a4_t, a5_t, a6_t); \
                rc_t (*ctx)(void *, a1_t, a2_t, a3_t, a4_t, a5_t, a6_t); } fptr; \
        void *fctx; \
    } sc_##fn;

/* system call structure assignment macro */
#define SA_SC_ASSIGN(sa, fn, ptr, ctx) \
    do { \
        (sa)->scSysCall.sc_##fn.fptr.any = (void (*)(void))(ptr); \
        (sa)->scSysCall.sc_##fn.fctx = (ctx); \
    } while (0)

/* system call structure assignment macro */
#define SA_SC_COPY(sa1, sa2, fn) \
    do { \
        (sa1)->scSysCall.sc_##fn.fptr.any = (sa2)->scSysCall.sc_##fn.fptr.any; \
        (sa1)->scSysCall.sc_##fn.fctx     = (sa2)->scSysCall.sc_##fn.fctx; \
    } while (0)

/* system call function call macros */
#define SA_SC_CALL_0(sa, fn) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(void) )
#define SA_SC_CALL_1(sa, fn, a1) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1) )
#define SA_SC_CALL_2(sa, fn, a1, a2) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1, a2) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1, a2) )
#define SA_SC_CALL_3(sa, fn, a1, a2, a3) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1, a2, a3) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1, a2, a3) )
#define SA_SC_CALL_4(sa, fn, a1, a2, a3, a4) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1, a2, a3, a4) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1, a2, a3, a4) )
#define SA_SC_CALL_5(sa, fn, a1, a2, a3, a4, a5) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1, a2, a3, a4, a5) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1, a2, a3, a4, a5) )
#define SA_SC_CALL_6(sa, fn, a1, a2, a3, a4, a5, a6) \
    (   (sa)->scSysCall.sc_##fn.fctx != NULL \
     ? ((sa)->scSysCall.sc_##fn.fptr.ctx)((sa)->scSysCall.sc_##fn.fctx, a1, a2, a3, a4, a5, a6) \
     : ((sa)->scSysCall.sc_##fn.fptr.std)(a1, a2, a3, a4, a5, a6) )

/* system call table */
typedef struct {
    SA_SC_DECLARE_3(int,              connect,       int, const struct sockaddr *, socklen_t)
    SA_SC_DECLARE_3(int,              accept,        int, struct sockaddr *, socklen_t *)
    SA_SC_DECLARE_5(int,              select,        int, fd_set *, fd_set *, fd_set *, struct timeval *)
    SA_SC_DECLARE_3(ssize_t,          read,          int, void *, size_t)
    SA_SC_DECLARE_3(ssize_t,          write,         int, const void *, size_t)
    SA_SC_DECLARE_6(ssize_t,          recvfrom,      int, void *, size_t, int, struct sockaddr *, socklen_t *)
    SA_SC_DECLARE_6(ssize_t,          sendto,        int, const void *, size_t, int, const struct sockaddr *, socklen_t)
} sa_syscall_tab_t;

/* socket option information */
typedef struct {
    int todo;
    int value;
} sa_optinfo_t;

/* socket abstraction object */
struct sa_st {
    sa_type_t        eType;            /* socket type (stream or datagram) */
    int              fdSocket;         /* socket file descriptor */
    struct timeval   tvTimeout[4];     /* timeout values (sec, usec) */
    int              nReadLen;         /* read  buffer current length */
    int              nReadSize;        /* read  buffer current size */
    char            *cpReadBuf;        /* read  buffer memory chunk */
    int              nWriteLen;        /* write buffer current length */
    int              nWriteSize;       /* write buffer current size */
    char            *cpWriteBuf;       /* write buffer memory chunk */
    sa_syscall_tab_t scSysCall;        /* table of system calls */
    sa_optinfo_t     optInfo[5];       /* option storage */
};

/* socket address abstraction object */
struct sa_addr_st {
    int              nFamily;          /* the socket family (AF_XXX) */
    struct sockaddr *saBuf;            /* the "struct sockaddr_xx" actually */
    socklen_t        slBuf;            /* the length of "struct sockaddr_xx" */
};

/* handy struct timeval check */
#define SA_TVISZERO(tv) \
    ((tv).tv_sec == 0 && (tv).tv_usec == 0)

/* convert Internet address from presentation to network format */
#ifndef HAVE_GETADDRINFO
static int sa_inet_pton(int family, const char *strptr, void *addrptr)
{
#ifdef HAVE_INET_PTON
    return inet_pton(family, strptr, addrptr);
#else
    struct in_addr in_val;

    if (family == AF_INET) {
#if defined(HAVE_INET_ATON)
        /* at least for IPv4 we can rely on the old inet_aton(3)
           and for IPv6 inet_pton(3) would exist anyway */
        if (inet_aton(strptr, &in_val) == 0)
            return 0;
        memcpy(addrptr, &in_val, sizeof(struct in_addr));
        return 1;
#elif defined(HAVE_INET_ADDR)
        /* at least for IPv4 try to rely on the even older inet_addr(3) */
        memset(&in_val, '\0', sizeof(in_val));
        if ((in_val.s_addr = inet_addr(strptr)) == ((in_addr_t)-1))
            return 0;
        memcpy(addrptr, &in_val, sizeof(struct in_addr));
        return 1;
#endif
    }
    errno = EAFNOSUPPORT;
    return 0;
#endif
}
#endif /* !HAVE_GETADDRINFO */

/* convert Internet address from network to presentation format */
static const char *sa_inet_ntop(int family, const void *src, char *dst, size_t size)
{
#ifdef HAVE_INET_NTOP
    return inet_ntop(family, src, dst, size);
#else
#ifdef HAVE_INET_NTOA
    char *cp;
    int n;
#endif

    if (family == AF_INET) {
#ifdef HAVE_INET_NTOA
        /* at least for IPv4 we can rely on the old inet_ntoa(3)
           and for IPv6 inet_ntop(3) would exist anyway */
        if ((cp = inet_ntoa(*((struct in_addr *)src))) == NULL)
            return NULL;
        n = strlen(cp);
        if (n > size-1)
            n = size-1;
        memcpy(dst, cp, n);
        dst[n] = '\0';
        return dst;
#endif
    }
    errno = EAFNOSUPPORT;
    return NULL;
#endif
}

/* minimal output-independent vprintf(3) variant which supports %{c,s,d,%} only */
static int sa_mvxprintf(int (*output)(void *ctx, const char *buffer, size_t bufsize), void *ctx, const char *format, va_list ap)
{
    /* sufficient integer buffer: <available-bits> x log_10(2) + safety */
    char ibuf[((sizeof(int)*8)/3)+10];
    char *cp;
    char c;
    int d;
    int n;
    int bytes;

    if (format == NULL)
        return -1;
    bytes = 0;
    while (*format != '\0') {
        if (*format == '%') {
            c = *(format+1);
            if (c == '%') {
                /* expand "%%" */
                cp = &c;
                n = (int)sizeof(char);
            }
            else if (c == 'c') {
                /* expand "%c" */
                c = (char)va_arg(ap, int);
                cp = &c;
                n = (int)sizeof(char);
            }
            else if (c == 's') {
                /* expand "%s" */
                if ((cp = (char *)va_arg(ap, char *)) == NULL)
                    cp = "(null)";
                n = (int)strlen(cp);
            }
            else if (c == 'd') {
                /* expand "%d" */
                d = (int)va_arg(ap, int);
#ifdef HAVE_SNPRINTF
                snprintf(ibuf, sizeof(ibuf), "%d", d); /* explicitly secure */
#else
                sprintf(ibuf, "%d", d);                /* implicitly secure */
#endif
                cp = ibuf;
                n = (int)strlen(cp);
            }
            else {
                /* any other "%X" */
                cp = (char *)format;
                n  = 2;
            }
            format += 2;
        }
        else {
            /* plain text */
            cp = (char *)format;
            if ((format = strchr(cp, '%')) == NULL)
                format = strchr(cp, '\0');
            n = (int)(format - cp);
        }
        /* perform output operation */
        if (output != NULL)
            if ((n = output(ctx, cp, (size_t)n)) == -1)
                break;
        bytes += n;
    }
    return bytes;
}

/* output callback function context for sa_mvsnprintf() */
typedef struct {
    char *bufptr;
    size_t buflen;
} sa_mvsnprintf_cb_t;

/* output callback function for sa_mvsnprintf() */
static int sa_mvsnprintf_cb(void *_ctx, const char *buffer, size_t bufsize)
{
    sa_mvsnprintf_cb_t *ctx = (sa_mvsnprintf_cb_t *)_ctx;

    if (bufsize > ctx->buflen)
        return -1;
    memcpy(ctx->bufptr, buffer, bufsize);
    ctx->bufptr += bufsize;
    ctx->buflen -= bufsize;
    return (int)bufsize;
}

/* minimal vsnprintf(3) variant which supports %{c,s,d} only */
static int sa_mvsnprintf(char *buffer, size_t bufsize, const char *format, va_list ap)
{
    int n;
    sa_mvsnprintf_cb_t ctx;

    if (format == NULL)
        return -1;
    if (buffer != NULL && bufsize == 0)
        return -1;
    if (buffer == NULL)
        /* just determine output length */
        n = sa_mvxprintf(NULL, NULL, format, ap);
    else {
        /* perform real output */
        ctx.bufptr = buffer;
        ctx.buflen = bufsize;
        n = sa_mvxprintf(sa_mvsnprintf_cb, &ctx, format, ap);
        if (n != -1 && ctx.buflen == 0)
            n = -1;
        if (n != -1)
            *(ctx.bufptr) = '\0';
    }
    return n;
}

/* minimal snprintf(3) variant which supports %{c,s,d} only */
static int sa_msnprintf(char *buffer, size_t bufsize, const char *format, ...)
{
    int chars;
    va_list ap;

    /* pass through to va_list based variant */
    va_start(ap, format);
    chars = sa_mvsnprintf(buffer, bufsize, format, ap);
    va_end(ap);

    return chars;
}

/* create address object */
sa_rc_t sa_addr_create(sa_addr_t **saap)
{
    sa_addr_t *saa;

    /* argument sanity check(s) */
    if (saap == NULL)
        return SA_RC(SA_ERR_ARG);

    /* allocate and initialize new address object */
    if ((saa = (sa_addr_t *)malloc(sizeof(sa_addr_t))) == NULL)
        return SA_RC(SA_ERR_MEM);
    saa->nFamily = 0;
    saa->saBuf   = NULL;
    saa->slBuf   = 0;

    /* pass object to caller */
    *saap = saa;

    return SA_OK;
}

/* destroy address object */
sa_rc_t sa_addr_destroy(sa_addr_t *saa)
{
    /* argument sanity check(s) */
    if (saa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* free address objects and sub-object(s) */
    if (saa->saBuf != NULL)
        free(saa->saBuf);
    free(saa);

    return SA_OK;
}

/* import URI into address object */
sa_rc_t sa_addr_u2a(sa_addr_t *saa, const char *uri, ...)
{
    va_list ap;
    int sf;
    socklen_t sl;
    struct sockaddr *sa;
    struct sockaddr_un un;
#ifdef HAVE_GETADDRINFO
    struct addrinfo ai_hints;
    struct addrinfo *ai = NULL;
    int err;
#else
    struct sockaddr_in sa4;
#ifdef AF_INET6
    struct sockaddr_in6 sa6;
#endif
    struct hostent *he;
#endif
    struct servent *se;
    int bIPv6;
    int bNumeric;
    const char *cpHost;
    const char *cpPort;
    char *cpProto;
    unsigned int nPort;
    const char *cpPath;
    char uribuf[1024];
    char *cp;
    int i;
    size_t n;
    int k;

    /* argument sanity check(s) */
    if (saa == NULL || uri == NULL)
        return SA_RC(SA_ERR_ARG);

    /* on-the-fly create or just take over URI */
    va_start(ap, uri);
    k = sa_mvsnprintf(uribuf, sizeof(uribuf), uri, ap);
    va_end(ap);
    if (k == -1)
        return SA_RC(SA_ERR_MEM);

    /* initialize result variables */
    sa = NULL;
    sl = 0;
    sf = 0;

    /* parse URI and resolve contents.
       The following syntax is recognized:
       - unix:<path>
       - inet://<host>:<port>[#(tcp|udp)] */
    uri = uribuf;
    if (strncmp(uri, "unix:", 5) == 0) {
        /* parse URI */
        cpPath = uri+5;

        /* mandatory(!) socket address structure initialization */
        memset(&un, 0, sizeof(un));

        /* fill-in socket address structure */
        n = strlen(cpPath);
        if (n == 0)
            return SA_RC(SA_ERR_ARG);
        if ((n+1) > sizeof(un.sun_path))
            return SA_RC(SA_ERR_MEM);
        if (cpPath[0] != '/') {
            if (getcwd(un.sun_path, sizeof(un.sun_path)-(n+1)) == NULL)
                return SA_RC(SA_ERR_MEM);
            cp = un.sun_path + strlen(un.sun_path);
        }
        else
            cp = un.sun_path;
        memcpy(cp, cpPath, n+1);
        un.sun_family = AF_LOCAL;

        /* provide results */
        sa = (struct sockaddr *)&un;
        sl = (socklen_t)sizeof(un);
        sf = AF_LOCAL;
    }
    else if (strncmp(uri, "inet://", 7) == 0) {
        /* parse URI into host, port and protocol parts */
        cpHost = uri+7;
        bIPv6 = FALSE;
        if (cpHost[0] == '[') {
            /* IPv6 address (see RFC2732) */
#ifndef AF_INET6
            return SA_RC(SA_ERR_IMP);
#else
            bIPv6 = TRUE;
            cpHost++;
            if ((cp = strchr(cpHost, ']')) == NULL)
                return SA_RC(SA_ERR_ARG);
            *cp++ = '\0';
            if (*cp != ':')
                return SA_RC(SA_ERR_ARG);
            cp++;
#endif
        }
        else {
            /* IPv4 address or hostname */
            if ((cp = strrchr(cpHost, ':')) == NULL)
                return SA_RC(SA_ERR_ARG);
            *cp++ = '\0';
        }
        cpPort = cp;
        cpProto = "tcp";
        if ((cp = strchr(cpPort, '#')) != NULL) {
            *cp++ = '\0';
            cpProto = cp;
        }

        /* resolve port */
        nPort = 0;
        bNumeric = 1;
        for (i = 0; cpPort[i] != '\0'; i++) {
            if (!isdigit((int)cpPort[i])) {
                bNumeric = 0;
                break;
            }
        }
        if (bNumeric)
            nPort = (unsigned int)atoi(cpPort);
        else {
            if ((se = getservbyname(cpPort, cpProto)) == NULL)
                return SA_RC(SA_ERR_SYS);
            nPort = ntohs(se->s_port);
        }

#ifdef HAVE_GETADDRINFO
        memset(&ai_hints, 0, sizeof(ai_hints));
        ai_hints.ai_family = PF_UNSPEC;
        if ((err = getaddrinfo(cpHost, NULL, &ai_hints, &ai)) != 0) {
            if (err == EAI_MEMORY)
                return SA_RC(SA_ERR_MEM);
            else if (err == EAI_SYSTEM)
                return SA_RC(SA_ERR_SYS);
            else
                return SA_RC(SA_ERR_ARG);
        }
        sa = ai->ai_addr;
        sl = ai->ai_addrlen;
        sf = ai->ai_family;
        if (sf == AF_INET)
            ((struct sockaddr_in *)sa)->sin_port = htons(nPort);
        else if (sf == AF_INET6)
            ((struct sockaddr_in6 *)sa)->sin6_port = htons(nPort);
        else
            return SA_RC(SA_ERR_ARG);
#else /* !HAVE_GETADDRINFO */

        /* mandatory(!) socket address structure initialization */
        memset(&sa4, 0, sizeof(sa4));
#ifdef AF_INET6
        memset(&sa6, 0, sizeof(sa6));
#endif

        /* resolve host by trying to parse it as either directly an IPv4 or
           IPv6 address or by resolving it to either an IPv4 or IPv6 address */
        if (!bIPv6 && sa_inet_pton(AF_INET, cpHost, &sa4.sin_addr.s_addr) == 1) {
            sa4.sin_family = AF_INET;
            sa4.sin_port = htons(nPort);
            sa = (struct sockaddr *)&sa4;
            sl = (socklen_t)sizeof(sa4);
            sf = AF_INET;
        }
#ifdef AF_INET6
        else if (bIPv6 && sa_inet_pton(AF_INET6, cpHost, &sa6.sin6_addr.s6_addr) == 1) {
            sa6.sin6_family = AF_INET6;
            sa6.sin6_port = htons(nPort);
            sa = (struct sockaddr *)&sa6;
            sl = (socklen_t)sizeof(sa6);
            sf = AF_INET6;
        }
#endif
        else if ((he = gethostbyname(cpHost)) != NULL) {
            if (he->h_addrtype == AF_INET) {
                sa4.sin_family = AF_INET;
                sa4.sin_port = htons(nPort);
                memcpy(&sa4.sin_addr.s_addr, he->h_addr_list[0],
                       sizeof(sa4.sin_addr.s_addr));
                sa = (struct sockaddr *)&sa4;
                sl = (socklen_t)sizeof(sa4);
                sf = AF_INET;
            }
#ifdef AF_INET6
            else if (he->h_addrtype == AF_INET6) {
                sa6.sin6_family = AF_INET6;
                sa6.sin6_port = htons(nPort);
                memcpy(&sa6.sin6_addr.s6_addr, he->h_addr_list[0],
                       sizeof(sa6.sin6_addr.s6_addr));
                sa = (struct sockaddr *)&sa6;
                sl = (socklen_t)sizeof(sa6);
                sf = AF_INET6;
            }
#endif
            else
                return SA_RC(SA_ERR_ARG);
        }
        else
            return SA_RC(SA_ERR_ARG);
#endif /* !HAVE_GETADDRINFO */
    }
    else
        return SA_RC(SA_ERR_ARG);

    /* fill-in result address structure */
    if (saa->saBuf != NULL)
        free(saa->saBuf);
    if ((saa->saBuf = (struct sockaddr *)malloc((size_t)sl)) == NULL)
        return SA_RC(SA_ERR_MEM);
    memcpy(saa->saBuf, sa, (size_t)sl);
    saa->slBuf = sl;
    saa->nFamily = (int)sf;

#ifdef HAVE_GETADDRINFO
    if (ai != NULL)
        freeaddrinfo(ai);
#endif

    return SA_OK;
}

/* import "struct sockaddr" into address object */
sa_rc_t sa_addr_s2a(sa_addr_t *saa, const struct sockaddr *sabuf, socklen_t salen)
{
    /* argument sanity check(s) */
    if (saa == NULL || sabuf == NULL || salen == 0)
        return SA_RC(SA_ERR_ARG);

    /* make sure we import only supported addresses */
    if (!(   sabuf->sa_family == AF_LOCAL
          || sabuf->sa_family == AF_INET
#ifdef AF_INET6
          || sabuf->sa_family == AF_INET6
#endif
         ))
        return SA_RC(SA_ERR_USE);

    /* create result address structure */
    if (saa->saBuf != NULL)
        free(saa->saBuf);
    if ((saa->saBuf = (struct sockaddr *)malloc((size_t)salen)) == NULL)
        return SA_RC(SA_ERR_MEM);
    memcpy(saa->saBuf, sabuf, (size_t)salen);
    saa->slBuf = salen;

    /* remember family */
    saa->nFamily = (int)(sabuf->sa_family);

    return SA_OK;
}

/* export address object into URI */
sa_rc_t sa_addr_a2u(const sa_addr_t *saa, char **uri)
{
    char uribuf[1024];
    struct sockaddr_un *un;
    struct sockaddr_in *sa4;
#ifdef AF_INET6
    struct sockaddr_in6 *sa6;
#endif
    char caHost[512];
    unsigned int nPort;

    /* argument sanity check(s) */
    if (saa == NULL || uri == NULL)
        return SA_RC(SA_ERR_ARG);

    /* export object contents */
    if (saa->nFamily == AF_LOCAL) {
        un = (struct sockaddr_un *)((void *)saa->saBuf);
        if (   (   saa->slBuf >= (socklen_t)(&(((struct sockaddr_un *)0)->sun_path[0]))
                && un->sun_path[0] == '\0')
            || (size_t)(saa->slBuf) < sizeof(struct sockaddr_un)) {
            /* in case the remote side of a Unix Domain socket was not
               bound, a "struct sockaddr_un" can occur with a length less
               than the expected one. Then there is actually no path at all.
               This has been verified under FreeBSD, Linux and Solaris. */
            if (sa_msnprintf(uribuf, sizeof(uribuf), "unix:/NOT-BOUND") == -1)
                return SA_RC(SA_ERR_FMT);
        }
        else {
            if (sa_msnprintf(uribuf, sizeof(uribuf), "unix:%s", un->sun_path) == -1)
                return SA_RC(SA_ERR_FMT);
        }
    }
    else if (saa->nFamily == AF_INET) {
        sa4 = (struct sockaddr_in *)((void *)saa->saBuf);
        if (sa_inet_ntop(AF_INET, &sa4->sin_addr.s_addr, caHost, sizeof(caHost)) == NULL)
            return SA_RC(SA_ERR_NET);
        nPort = ntohs(sa4->sin_port);
        if (sa_msnprintf(uribuf, sizeof(uribuf), "inet://%s:%d", caHost, nPort) == -1)
            return SA_RC(SA_ERR_FMT);
    }
#ifdef AF_INET6
    else if (saa->nFamily == AF_INET6) {
        sa6 = (struct sockaddr_in6 *)((void *)saa->saBuf);
        if (sa_inet_ntop(AF_INET6, &sa6->sin6_addr.s6_addr, caHost, sizeof(caHost)) == NULL)
            return SA_RC(SA_ERR_NET);
        nPort = ntohs(sa6->sin6_port);
        if (sa_msnprintf(uribuf, sizeof(uribuf), "inet://[%s]:%d", caHost, nPort) == -1)
            return SA_RC(SA_ERR_FMT);
    }
#endif
    else
        return SA_RC(SA_ERR_INT);

    /* pass result to caller */
    *uri = strdup(uribuf);

    return SA_OK;
}

/* export address object into "struct sockaddr" */
sa_rc_t sa_addr_a2s(const sa_addr_t *saa, struct sockaddr **sabuf, socklen_t *salen)
{
    /* argument sanity check(s) */
    if (saa == NULL || sabuf == NULL || salen == 0)
        return SA_RC(SA_ERR_ARG);

    /* export underlying address structure */
    if ((*sabuf = (struct sockaddr *)malloc((size_t)saa->slBuf)) == NULL)
        return SA_RC(SA_ERR_MEM);
    memmove(*sabuf, saa->saBuf, (size_t)saa->slBuf);
    *salen = saa->slBuf;

    return SA_OK;
}

sa_rc_t sa_addr_match(const sa_addr_t *saa1, const sa_addr_t *saa2, int prefixlen)
{
    const unsigned char *ucp1, *ucp2;
    unsigned int uc1, uc2, mask;
    size_t l1, l2;
    int nBytes;
    int nBits;
#ifdef AF_INET6
    int i;
    const unsigned char *ucp0;
#endif
    unsigned int np1, np2;
    int bMatchPort;

    /* argument sanity check(s) */
    if (saa1 == NULL || saa2 == NULL)
        return SA_RC(SA_ERR_ARG);

    /* short circuiting for wildcard matching */
    if (prefixlen == 0)
        return SA_OK;

    /* determine address representation pointer and size */
    if (saa1->nFamily == AF_LOCAL) {
        np1  = 0;
        np2  = 0;
        ucp1 = (const unsigned char *)(((struct sockaddr_un *)saa1->saBuf)->sun_path);
        ucp2 = (const unsigned char *)(((struct sockaddr_un *)saa2->saBuf)->sun_path);
        l1 = strlen(((struct sockaddr_un *)saa1->saBuf)->sun_path) * 8;
        l2 = strlen(((struct sockaddr_un *)saa2->saBuf)->sun_path) * 8;
        if (prefixlen < 0) {
            if (l1 != l2)
                return SA_RC(SA_ERR_MTC);
            nBits = (int)l1;
        }
        else {
            if ((int)l1 < prefixlen || (int)l2 < prefixlen)
                return SA_RC(SA_ERR_MTC);
            nBits = prefixlen;
        }
    }
#ifdef AF_INET6
    else if (   (saa1->nFamily == AF_INET  && saa2->nFamily == AF_INET6)
             || (saa1->nFamily == AF_INET6 && saa2->nFamily == AF_INET )) {
        /* special case of comparing a regular IPv4 address (1.2.3.4) with an
           "IPv4-mapped IPv6 address" (::ffff:1.2.3.4). For details see RFC 2373. */
        if (saa1->nFamily == AF_INET6) {
            np1  = (unsigned int)(((struct sockaddr_in6 *)saa1->saBuf)->sin6_port);
            np2  = (unsigned int)(((struct sockaddr_in  *)saa2->saBuf)->sin_port);
            ucp1 = (const unsigned char *)&(((struct sockaddr_in6 *)saa1->saBuf)->sin6_addr);
            ucp2 = (const unsigned char *)&(((struct sockaddr_in  *)saa2->saBuf)->sin_addr);
            ucp0 = ucp1;
            ucp1 += 12;
        }
        else {
            np1  = (unsigned int)(((struct sockaddr_in  *)saa1->saBuf)->sin_port);
            np2  = (unsigned int)(((struct sockaddr_in6 *)saa2->saBuf)->sin6_port);
            ucp1 = (const unsigned char *)&(((struct sockaddr_in  *)saa1->saBuf)->sin_addr);
            ucp2 = (const unsigned char *)&(((struct sockaddr_in6 *)saa2->saBuf)->sin6_addr);
            ucp0 = ucp2;
            ucp2 += 12;
        }
        for (i = 0; i < 10; i++)
            if ((int)ucp0[i] != 0x00)
                return SA_RC(SA_ERR_MTC);
        if (!((int)ucp0[10] == 0xFF && (int)ucp0[11] == 0xFF))
            return SA_RC(SA_ERR_MTC);
        nBits = 32;
    }
#endif
    else if (saa1->nFamily == AF_INET) {
        np1  = (unsigned int)(((struct sockaddr_in *)saa1->saBuf)->sin_port);
        np2  = (unsigned int)(((struct sockaddr_in *)saa2->saBuf)->sin_port);
        ucp1 = (const unsigned char *)&(((struct sockaddr_in *)saa1->saBuf)->sin_addr);
        ucp2 = (const unsigned char *)&(((struct sockaddr_in *)saa2->saBuf)->sin_addr);
        nBits = 32;
    }
#ifdef AF_INET6
    else if (saa1->nFamily == AF_INET6) {
        np1  = (unsigned int)(((struct sockaddr_in6 *)saa1->saBuf)->sin6_port);
        np2  = (unsigned int)(((struct sockaddr_in6 *)saa2->saBuf)->sin6_port);
        ucp1 = (const unsigned char *)&(((struct sockaddr_in6 *)saa1->saBuf)->sin6_addr);
        ucp2 = (const unsigned char *)&(((struct sockaddr_in6 *)saa2->saBuf)->sin6_addr);
        nBits = 128;
    }
#endif
    else
        return SA_RC(SA_ERR_INT);

    /* make sure we do not compare than possible */
    if (prefixlen > (nBits+1))
        return SA_RC(SA_ERR_ARG);

    /* support equal matching (= all bits plus optionally port) */
    bMatchPort = FALSE;
    if (prefixlen < 0) {
        if (prefixlen < -1)
            bMatchPort = TRUE;
        prefixlen = nBits;
    }

    /* perform address representation comparison
       (assumption guaranteed by API: network byte order is used) */
    nBytes = (prefixlen / 8);
    nBits  = (prefixlen % 8);
    if (nBytes > 0) {
        if (memcmp(ucp1, ucp2, (size_t)nBytes) != 0)
            return SA_RC(SA_ERR_MTC);
    }
    if (nBits > 0) {
        uc1 = (unsigned int)ucp1[nBytes];
        uc2 = (unsigned int)ucp2[nBytes];
        mask = ((unsigned int)0xFF << (8-nBits)) & (unsigned int)0xFF;
        if ((uc1 & mask) != (uc2 & mask))
            return SA_RC(SA_ERR_MTC);
    }

    /* optionally perform additional port matching */
    if (bMatchPort)
        if (np1 != np2)
            return SA_RC(SA_ERR_MTC);

    return SA_OK;
}

/* set timeouts if timeouts or done in kernel */
static sa_rc_t sa_socket_settimeouts(const sa_t *sa)
{
    /* stop processing if socket is still not allocated */
    if (sa->fdSocket == -1)
        return SA_OK;

#if defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO)
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_READ])) {
        if (setsockopt(sa->fdSocket, SOL_SOCKET, SO_RCVTIMEO,
                       (const void *)(&sa->tvTimeout[SA_TIMEOUT_READ]),
                       (socklen_t)(sizeof(sa->tvTimeout[SA_TIMEOUT_READ]))) < 0)
            return SA_RC(SA_ERR_SYS);
    }
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_WRITE])) {
        if (setsockopt(sa->fdSocket, SOL_SOCKET, SO_SNDTIMEO,
                       (const void *)(&sa->tvTimeout[SA_TIMEOUT_WRITE]),
                       (socklen_t)(sizeof(sa->tvTimeout[SA_TIMEOUT_WRITE]))) < 0)
            return SA_RC(SA_ERR_SYS);
    }
#endif
    return SA_OK;
}

/* set socket options */
static sa_rc_t sa_socket_setoptions(sa_t *sa)
{
    int i;
    sa_rc_t rv;

    /* stop processing if socket is still not allocated */
    if (sa->fdSocket == -1)
        return SA_OK;

    /* check for pending options */
    rv = SA_OK;
    for (i = 0; i < (int)(sizeof(sa->optInfo)/sizeof(sa->optInfo[0])); i++) {
        if (sa->optInfo[i].todo) {
            switch (i) {
                /* enable/disable Nagle's Algorithm (see RFC898) */
                case SA_OPTION_NAGLE: {
#if defined(IPPROTO_TCP) && defined(TCP_NODELAY)
                    int mode = sa->optInfo[i].value;
                    if (setsockopt(sa->fdSocket, IPPROTO_TCP, TCP_NODELAY,
                                   (const void *)&mode,
                                   (socklen_t)sizeof(mode)) < 0)
                        rv = SA_ERR_SYS;
                    else
                        sa->optInfo[i].todo = FALSE;
#endif
                    break;
                }
                /* enable/disable linger close semantics */
                case SA_OPTION_LINGER: {
#if defined(SO_LINGER)
                    struct linger linger;
                    linger.l_onoff  = (sa->optInfo[i].value == 0 ? 0 : 1);
                    linger.l_linger = (sa->optInfo[i].value <= 0 ? 0 : sa->optInfo[i].value);
                    if (setsockopt(sa->fdSocket, SOL_SOCKET, SO_LINGER,
                                   (const void *)&linger,
                                   (socklen_t)sizeof(struct linger)) < 0)
                        rv = SA_ERR_SYS;
                    else
                        sa->optInfo[i].todo = FALSE;
#endif
                    break;
                }
                /* enable/disable reusability of binding to address */
                case SA_OPTION_REUSEADDR: {
#if defined(SO_REUSEADDR)
                    int mode = sa->optInfo[i].value;
                    if (setsockopt(sa->fdSocket, SOL_SOCKET, SO_REUSEADDR,
                                   (const void *)&mode, (socklen_t)sizeof(mode)) < 0)
                        rv = SA_ERR_SYS;
                    else
                        sa->optInfo[i].todo = FALSE;
#endif
                    break;
                }
                /* enable/disable reusability of binding to port */
                case SA_OPTION_REUSEPORT: {
#if defined(SO_REUSEPORT)
                    int mode = sa->optInfo[i].value;
                    if (setsockopt(sa->fdSocket, SOL_SOCKET, SO_REUSEPORT,
                                   (const void *)&mode, (socklen_t)sizeof(mode)) < 0)
                        rv = SA_ERR_SYS;
                    else
                        sa->optInfo[i].todo = FALSE;
#endif
                    break;
                }
                /* enable/disable non-blocking I/O mode */
                case SA_OPTION_NONBLOCK: {
                    int mode = sa->optInfo[i].value;
                    int flags;
                    if ((flags = fcntl(sa->fdSocket, F_GETFL, 0)) < 0) {
                        rv = SA_ERR_SYS;
                        break;
                    }
                    if (mode == 0)
                        flags &= ~(O_NONBLOCK);
                    else
                        flags |= O_NONBLOCK;
                    if (fcntl(sa->fdSocket, F_SETFL, flags) < 0)
                        rv = SA_ERR_SYS;
                    else
                        sa->optInfo[i].todo = FALSE;
                    break;
                }
                default: /* NOTREACHED */ break;
            }
        }
    }

    return SA_RC(rv);
}

/* internal lazy/delayed initialization of underlying socket */
static sa_rc_t sa_socket_init(sa_t *sa, int nFamily)
{
    int nType;
    int nProto;
#if !(defined(IPPROTO_TCP) && defined(IPPROTO_UDP))
    struct protoent *pe;
#endif
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* only perform operation if socket still does not exist */
    if (sa->fdSocket != -1)
        return SA_RC(SA_ERR_USE);

    /* determine socket type */
    if (sa->eType == SA_TYPE_STREAM)
        nType = SOCK_STREAM;
    else if (sa->eType == SA_TYPE_DATAGRAM)
        nType = SOCK_DGRAM;
    else
        return SA_RC(SA_ERR_INT);

    /* determine socket protocol */
    if (nFamily == AF_LOCAL)
        nProto = 0;
#ifdef AF_INET6
    else if (nFamily == AF_INET || nFamily == AF_INET6) {
#else
    else if (nFamily == AF_INET) {
#endif
#if defined(IPPROTO_TCP) && defined(IPPROTO_UDP)
        if (nType == SOCK_STREAM)
            nProto = IPPROTO_TCP;
        else if (nType == SOCK_DGRAM)
            nProto = IPPROTO_UDP;
        else
            return SA_RC(SA_ERR_INT);
#else
        if (nType == SOCK_STREAM)
            pe = getprotobyname("tcp");
        else if (nType == SOCK_DGRAM)
            pe = getprotobyname("udp");
        else
            return SA_RC(SA_ERR_INT);
        if (pe == NULL)
            return SA_RC(SA_ERR_SYS);
        nProto = pe->p_proto;
#endif
    }
    else
        return SA_RC(SA_ERR_INT);

    /* create the underlying socket */
    if ((sa->fdSocket = socket(nFamily, nType, nProto)) == -1)
        return SA_RC(SA_ERR_SYS);

    /* optionally set kernel timeouts */
    if ((rv = sa_socket_settimeouts(sa)) != SA_OK)
        return SA_RC(rv);

    /* optionally configure changed options */
    if ((rv = sa_socket_setoptions(sa)) != SA_OK)
        return SA_RC(rv);

    return SA_OK;
}

/* internal destruction of underlying socket */
static sa_rc_t sa_socket_kill(sa_t *sa)
{
    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* check context */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* close socket */
    (void)close(sa->fdSocket);
    sa->fdSocket = -1;

    return SA_OK;
}

/* create abstract socket object */
sa_rc_t sa_create(sa_t **sap)
{
    sa_t *sa;
    int i;

    /* argument sanity check(s) */
    if (sap == NULL)
        return SA_RC(SA_ERR_ARG);

    /* allocate and initialize socket object */
    if ((sa = (sa_t *)malloc(sizeof(sa_t))) == NULL)
        return SA_RC(SA_ERR_MEM);

    /* init object attributes */
    sa->eType          = SA_TYPE_STREAM;
    sa->fdSocket       = -1;
    sa->nReadLen       = 0;
    sa->nReadSize      = 0;
    sa->cpReadBuf      = NULL;
    sa->nWriteLen      = 0;
    sa->nWriteSize     = 0;
    sa->cpWriteBuf     = NULL;

    /* init timeval object attributes */
    for (i = 0; i < (int)(sizeof(sa->tvTimeout)/sizeof(sa->tvTimeout[0])); i++) {
        sa->tvTimeout[i].tv_sec  = 0;
        sa->tvTimeout[i].tv_usec = 0;
    }

    /* init options object attributes */
    for (i = 0; i < (int)(sizeof(sa->optInfo)/sizeof(sa->optInfo[0])); i++) {
        sa->optInfo[i].todo  = FALSE;
        sa->optInfo[i].value = 0;
    }

    /* init syscall object attributes */
    SA_SC_ASSIGN(sa, connect,       connect,       NULL);
    SA_SC_ASSIGN(sa, accept,        accept,        NULL);
    SA_SC_ASSIGN(sa, select,        select,        NULL);
    SA_SC_ASSIGN(sa, read,          read,          NULL);
    SA_SC_ASSIGN(sa, write,         write,         NULL);
    SA_SC_ASSIGN(sa, recvfrom,      recvfrom,      NULL);
    SA_SC_ASSIGN(sa, sendto,        sendto,        NULL);

    /* pass object to caller */
    *sap = sa;

    return SA_OK;
}

/* destroy abstract socket object */
sa_rc_t sa_destroy(sa_t *sa)
{
    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* kill underlying socket */
    (void)sa_socket_kill(sa);

    /* free object and sub-objects */
    if (sa->cpReadBuf != NULL)
        free(sa->cpReadBuf);
    if (sa->cpWriteBuf != NULL)
        free(sa->cpWriteBuf);
    free(sa);

    return SA_OK;
}

/* switch communication type of socket */
sa_rc_t sa_type(sa_t *sa, sa_type_t type)
{
    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);
    if (!(type == SA_TYPE_STREAM || type == SA_TYPE_DATAGRAM))
        return SA_RC(SA_ERR_ARG);

    /* kill underlying socket if type changes */
    if (sa->eType != type)
        (void)sa_socket_kill(sa);

    /* set new type */
    sa->eType = type;

    return SA_OK;
}

/* configure I/O timeout */
sa_rc_t sa_timeout(sa_t *sa, sa_timeout_t id, long sec, long usec)
{
    int i;
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    if (id == SA_TIMEOUT_ALL) {
        for (i = 0; i < (int)(sizeof(sa->tvTimeout)/sizeof(sa->tvTimeout[0])); i++) {
            sa->tvTimeout[i].tv_sec  = sec;
            sa->tvTimeout[i].tv_usec = usec;
        }
    }
    else {
        sa->tvTimeout[id].tv_sec  = sec;
        sa->tvTimeout[id].tv_usec = usec;
    }

    /* try to already set timeouts */
    if ((rv = sa_socket_settimeouts(sa)) != SA_OK)
        return SA_RC(rv);

    return SA_OK;
}

/* configure I/O buffers */
sa_rc_t sa_buffer(sa_t *sa, sa_buffer_t id, size_t size)
{
    char *cp;

    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    if (id == SA_BUFFER_READ) {
        /* configure read/incoming buffer */
        if (sa->nReadLen > (int)size)
            return SA_RC(SA_ERR_USE);
        if (size > 0) {
            if (sa->cpReadBuf == NULL)
                cp = (char *)malloc(size);
            else
                cp = (char *)realloc(sa->cpReadBuf, size);
            if (cp == NULL)
                return SA_RC(SA_ERR_MEM);
            sa->cpReadBuf = cp;
            sa->nReadSize = (int)size;
        }
        else {
            if (sa->cpReadBuf != NULL)
                free(sa->cpReadBuf);
            sa->cpReadBuf = NULL;
            sa->nReadSize = 0;
        }
    }
    else if (id == SA_BUFFER_WRITE) {
        /* configure write/outgoing buffer */
        if (sa->nWriteLen > (int)size)
            return SA_RC(SA_ERR_USE);
        if (size > 0) {
            if (sa->cpWriteBuf == NULL)
                cp = (char *)malloc(size);
            else
                cp = (char *)realloc(sa->cpWriteBuf, size);
            if (cp == NULL)
                return SA_RC(SA_ERR_MEM);
            sa->cpWriteBuf = cp;
            sa->nWriteSize = (int)size;
        }
        else {
            if (sa->cpWriteBuf != NULL)
                free(sa->cpWriteBuf);
            sa->cpWriteBuf = NULL;
            sa->nWriteSize = 0;
        }
    }
    else
        return SA_RC(SA_ERR_ARG);

    return SA_OK;
}

/* configure socket option */
sa_rc_t sa_option(sa_t *sa, sa_option_t id, ...)
{
    sa_rc_t rv;
    va_list ap;

    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* process option */
    rv = SA_OK;
    va_start(ap, id);
    switch (id) {
        case SA_OPTION_NAGLE: {
#if defined(IPPROTO_TCP) && defined(TCP_NODELAY)
            int mode = ((int)va_arg(ap, int) ? 1 : 0);
            sa->optInfo[SA_OPTION_NAGLE].value = mode;
            sa->optInfo[SA_OPTION_NAGLE].todo = TRUE;
#else
            rv = SA_ERR_IMP;
#endif
            break;
        }
        case SA_OPTION_LINGER: {
#if defined(SO_LINGER)
            int amount = (int)va_arg(ap, int);
            sa->optInfo[SA_OPTION_LINGER].value = amount;
            sa->optInfo[SA_OPTION_LINGER].todo = TRUE;
#else
            rv = SA_ERR_IMP;
#endif
            break;
        }
        case SA_OPTION_REUSEADDR:
        {
#if defined(SO_REUSEADDR)
            int mode = ((int)va_arg(ap, int) ? 1 : 0);
            sa->optInfo[SA_OPTION_REUSEADDR].value = mode;
            sa->optInfo[SA_OPTION_REUSEADDR].todo = TRUE;
#else
            rv = SA_ERR_IMP;
#endif
            break;
        }
        case SA_OPTION_REUSEPORT:
        {
#if defined(SO_REUSEPORT)
            int mode = ((int)va_arg(ap, int) ? 1 : 0);
            sa->optInfo[SA_OPTION_REUSEPORT].value = mode;
            sa->optInfo[SA_OPTION_REUSEPORT].todo = TRUE;
#else
            rv = SA_ERR_IMP;
#endif
            break;
        }
        case SA_OPTION_NONBLOCK: {
            int mode = (int)va_arg(ap, int);
            sa->optInfo[SA_OPTION_NONBLOCK].value = mode;
            sa->optInfo[SA_OPTION_NONBLOCK].todo = TRUE;
            break;
        }
        default: {
            rv = SA_ERR_ARG;
        }
    }
    va_end(ap);

    /* if an error already occured, stop here */
    if (rv != SA_OK)
        return SA_RC(rv);

    /* else already (re)set) options (if possible) */
    if ((rv = sa_socket_setoptions(sa)) != SA_OK)
        return SA_RC(rv);

    return SA_OK;
}

/* override system call */
sa_rc_t sa_syscall(sa_t *sa, sa_syscall_t id, void (*fptr)(void), void *fctx)
{
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL || fptr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* assign system call */
    rv = SA_OK;
    switch (id) {
        case SA_SYSCALL_CONNECT:       SA_SC_ASSIGN(sa, connect,       fptr, fctx); break;
        case SA_SYSCALL_ACCEPT:        SA_SC_ASSIGN(sa, accept,        fptr, fctx); break;
        case SA_SYSCALL_SELECT:        SA_SC_ASSIGN(sa, select,        fptr, fctx); break;
        case SA_SYSCALL_READ:          SA_SC_ASSIGN(sa, read,          fptr, fctx); break;
        case SA_SYSCALL_WRITE:         SA_SC_ASSIGN(sa, write,         fptr, fctx); break;
        case SA_SYSCALL_RECVFROM:      SA_SC_ASSIGN(sa, recvfrom,      fptr, fctx); break;
        case SA_SYSCALL_SENDTO:        SA_SC_ASSIGN(sa, sendto,        fptr, fctx); break;
        default: rv = SA_ERR_ARG;
    }

    return SA_RC(rv);
}

/* bind socket to a local address */
sa_rc_t sa_bind(sa_t *sa, const sa_addr_t *laddr)
{
    sa_rc_t rv;
    struct sockaddr_un *un;

    /* argument sanity check(s) */
    if (sa == NULL || laddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* lazy creation of underlying socket */
    if (sa->fdSocket == -1)
        if ((rv = sa_socket_init(sa, laddr->nFamily)) != SA_OK)
            return SA_RC(rv);

    /* remove a possibly existing old Unix Domain socket on filesystem */
    if (laddr->nFamily == AF_LOCAL) {
        un = (struct sockaddr_un *)((void *)laddr->saBuf);
        (void)unlink(un->sun_path);
    }

    /* perform bind operation on underlying socket */
    if (bind(sa->fdSocket, laddr->saBuf, laddr->slBuf) == -1)
        return SA_RC(SA_ERR_SYS);

    return SA_OK;
}

/* connect socket to a remote address */
sa_rc_t sa_connect(sa_t *sa, const sa_addr_t *raddr)
{
    int flags, n, error;
    fd_set rset, wset;
    socklen_t len;
    sa_rc_t rv;
    struct timeval *tv;
    struct timeval tv_buf;

    /* argument sanity check(s) */
    if (sa == NULL || raddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* connecting is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* lazy creation of underlying socket */
    if (sa->fdSocket == -1)
        if ((rv = sa_socket_init(sa, raddr->nFamily)) != SA_OK)
            return SA_RC(rv);

    /* prepare return code decision */
    rv = SA_OK;
    error = 0;

    /* temporarily switch underlying socket to non-blocking mode
       (necessary under timeout-aware operation only) */
    flags = 0;
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_CONNECT])) {
        flags = fcntl(sa->fdSocket, F_GETFL, 0);
        (void)fcntl(sa->fdSocket, F_SETFL, flags|O_NONBLOCK);
    }

    /* perform the connect operation */
    if ((n = SA_SC_CALL_3(sa, connect, sa->fdSocket, raddr->saBuf, raddr->slBuf)) < 0) {
        if (errno != EINTR && errno != EINPROGRESS) {
            /* we have to perform the following post-processing under
               EINPROGRESS anway, but actually also for EINTR according
               to Unix Network Programming, volume 1, section 5.9, W.
               Richard Stevens: "What we are doing [] is restarting
               the interrupted system call ourself. This is fine for
               accept, along with the functions such as read, write,
               select and open. But there is one function that we cannot
               restart ourself: connect. If this function returns EINTR,
               we cannot call it again, as doing so will return an
               immediate error. When connect is interrupted by a caught
               signal and is not automatically restarted, we must call
               select to wait for the connection to complete, as we
               describe in section 15.3." */
            error = errno;
            goto done;
        }
    }

    /* ok if connect completed immediately */
    if (n == 0)
        goto done;

    /* wait for read or write possibility */
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(sa->fdSocket, &rset);
    FD_SET(sa->fdSocket, &wset);
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_CONNECT])) {
        memcpy(&tv_buf, &sa->tvTimeout[SA_TIMEOUT_CONNECT], sizeof(struct timeval));
        tv = &tv_buf;
    }
    else
        tv = NULL;
    do {
        n = SA_SC_CALL_5(sa, select, sa->fdSocket+1, &rset, &wset, (fd_set *)NULL, tv);
    } while (n == -1 && errno == EINTR);

    /* decide on return semantic */
    if (n < 0) {
        error = errno;
        goto done;
    }
    else if (n == 0) {
        (void)close(sa->fdSocket); /* stop TCP three-way handshake */
        sa->fdSocket = -1;
        rv = SA_ERR_TMT;
        goto done;
    }

    /* fetch pending error */
    len = (socklen_t)sizeof(error);
    if (getsockopt(sa->fdSocket, SOL_SOCKET, SO_ERROR, (void *)&error, &len) < 0)
        error = errno;

    done:

    /* reset socket flags
       (necessary under timeout-aware operation only) */
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_CONNECT]))
        (void)fcntl(sa->fdSocket, F_SETFL, flags);

    /* optionally set errno */
    if (error != 0) {
        (void)close(sa->fdSocket); /* just in case */
        sa->fdSocket = -1;
        errno = error;
        rv = SA_ERR_SYS;
    }

    return SA_RC(rv);
}

/* listen on socket for connections */
sa_rc_t sa_listen(sa_t *sa, int backlog)
{
    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* listening is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least sa_bind() has to be already performed */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* perform listen operation on underlying socket */
    if (listen(sa->fdSocket, backlog) == -1)
        return SA_RC(SA_ERR_SYS);

    return SA_OK;
}

/* accept a connection on socket */
sa_rc_t sa_accept(sa_t *sa, sa_addr_t **caddr, sa_t **csa)
{
    sa_rc_t rv;
    int n;
    fd_set fds;
    union {
        struct sockaddr_un un;
        struct sockaddr_in sa4;
#ifdef AF_INET6
        struct sockaddr_in6 sa6;
#endif
    } sa_buf;
    socklen_t sa_size;
    struct timeval tv;
    int s;
    int i;

    /* argument sanity check(s) */
    if (sa == NULL || caddr == NULL || csa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* accepting connections is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least sa_listen() has to be already performed */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* if timeout is enabled, perform a smart-blocking wait */
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_ACCEPT])) {
        FD_ZERO(&fds);
        FD_SET(sa->fdSocket, &fds);
        memcpy(&tv, &sa->tvTimeout[SA_TIMEOUT_ACCEPT], sizeof(struct timeval));
        do {
            n = SA_SC_CALL_5(sa, select, sa->fdSocket+1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
        } while (n == -1 && errno == EINTR);
        if (n == 0)
            return SA_RC(SA_ERR_TMT);
        if (n <= 0)
            return SA_RC(SA_ERR_SYS);
    }

    /* perform accept operation on underlying socket */
    sa_size = (socklen_t)sizeof(sa_buf);
    do {
        s = SA_SC_CALL_3(sa, accept, sa->fdSocket, (struct sockaddr *)&sa_buf, &sa_size);
    } while (s == -1 && errno == EINTR);
    if (s == -1)
        return SA_RC(SA_ERR_SYS);

    /* create result address object */
    if ((rv = sa_addr_create(caddr)) != SA_OK)
        return SA_RC(rv);
    if ((rv = sa_addr_s2a(*caddr, (struct sockaddr *)&sa_buf, sa_size)) != SA_OK) {
        (void)sa_addr_destroy(*caddr);
        return SA_RC(rv);
    }

    /* create result socket object */
    if ((rv = sa_create(csa)) != SA_OK) {
        (void)sa_addr_destroy(*caddr);
        return SA_RC(rv);
    }

    /* fill-in child socket */
    (*csa)->fdSocket = s;

    /* copy-over original system calls */
    SA_SC_COPY((*csa), sa, connect);
    SA_SC_COPY((*csa), sa, accept);
    SA_SC_COPY((*csa), sa, select);
    SA_SC_COPY((*csa), sa, read);
    SA_SC_COPY((*csa), sa, write);
    SA_SC_COPY((*csa), sa, recvfrom);
    SA_SC_COPY((*csa), sa, sendto);

    /* copy-over original timeout values */
    for (i = 0; i < (int)(sizeof(sa->tvTimeout)/sizeof(sa->tvTimeout[0])); i++) {
        (*csa)->tvTimeout[i].tv_sec  = sa->tvTimeout[i].tv_sec;
        (*csa)->tvTimeout[i].tv_usec = sa->tvTimeout[i].tv_usec;
    }

    return SA_OK;
}

/* determine remote address */
sa_rc_t sa_getremote(sa_t *sa, sa_addr_t **raddr)
{
    sa_rc_t rv;
    union {
        struct sockaddr_in sa4;
#ifdef AF_INET6
        struct sockaddr_in6 sa6;
#endif
    } sa_buf;
    socklen_t sa_size;

    /* argument sanity check(s) */
    if (sa == NULL || raddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* peers exist only for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least sa_connect() or sa_accept() has to be already performed */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* determine remote address of underlying socket */
    sa_size = (socklen_t)sizeof(sa_buf);
    if (getpeername(sa->fdSocket, (struct sockaddr *)&sa_buf, &sa_size) < 0)
        return SA_RC(SA_ERR_SYS);

    /* create result address object */
    if ((rv = sa_addr_create(raddr)) != SA_OK)
        return SA_RC(rv);
    if ((rv = sa_addr_s2a(*raddr, (struct sockaddr *)&sa_buf, sa_size)) != SA_OK) {
        (void)sa_addr_destroy(*raddr);
        return SA_RC(rv);
    }

    return SA_OK;
}

/* determine local address */
sa_rc_t sa_getlocal(sa_t *sa, sa_addr_t **laddr)
{
    sa_rc_t rv;
    union {
        struct sockaddr_in sa4;
#ifdef AF_INET6
        struct sockaddr_in6 sa6;
#endif
    } sa_buf;
    socklen_t sa_size;

    /* argument sanity check(s) */
    if (sa == NULL || laddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* at least sa_bind() has to be already performed */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* determine local address of underlying socket */
    sa_size = (socklen_t)sizeof(sa_buf);
    if (getsockname(sa->fdSocket, (struct sockaddr *)&sa_buf, &sa_size) < 0)
        return SA_RC(SA_ERR_SYS);

    /* create result address object */
    if ((rv = sa_addr_create(laddr)) != SA_OK)
        return SA_RC(rv);
    if ((rv = sa_addr_s2a(*laddr, (struct sockaddr *)&sa_buf, sa_size)) != SA_OK) {
        (void)sa_addr_destroy(*laddr);
        return SA_RC(rv);
    }

    return SA_OK;
}

/* peek at underlying socket */
sa_rc_t sa_getfd(sa_t *sa, int *fd)
{
    /* argument sanity check(s) */
    if (sa == NULL || fd == NULL)
        return SA_RC(SA_ERR_ARG);

    /* if still no socket exists, say this explicitly */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* pass socket to caller */
    *fd = sa->fdSocket;

    return SA_OK;
}

/* internal raw read operation */
static int sa_read_raw(sa_t *sa, char *cpBuf, int nBufLen)
{
    int rv;
#if !(defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO))
    fd_set fds;
    struct timeval tv;
#endif

    /* if timeout is enabled, perform explicit/smart blocking instead
       of implicitly/hard blocking in the read(2) system call */
#if !(defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO))
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_READ])) {
        FD_ZERO(&fds);
        FD_SET(sa->fdSocket, &fds);
        memcpy(&tv, &sa->tvTimeout[SA_TIMEOUT_READ], sizeof(struct timeval));
        do {
            rv = SA_SC_CALL_5(sa, select, sa->fdSocket+1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
        } while (rv == -1 && errno == EINTR);
        if (rv == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
#endif

    /* perform read operation on underlying socket */
    do {
        rv = (int)SA_SC_CALL_3(sa, read, sa->fdSocket, cpBuf, (size_t)nBufLen);
    } while (rv == -1 && errno == EINTR);

#if defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO)
    if (rv == -1 && errno == EWOULDBLOCK)
        errno = ETIMEDOUT;
#endif

    return rv;
}

/* read data from socket */
sa_rc_t sa_read(sa_t *sa, char *cpBuf, size_t nBufReq, size_t *nBufRes)
{
    int n;
    sa_rc_t rv;
    int res;

    /* argument sanity check(s) */
    if (sa == NULL || cpBuf == NULL || nBufReq == 0)
        return SA_RC(SA_ERR_ARG);

    /* reading is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* perform read operation */
    rv = SA_OK;
    if (sa->nReadSize == 0) {
        /* user-space unbuffered I/O */
        if (sa->nWriteLen > 0)
            (void)sa_flush(sa);
        res = sa_read_raw(sa, cpBuf, (int)nBufReq);
        if (res == 0)
            rv = SA_ERR_EOF;
        else if (res < 0 && errno == ETIMEDOUT)
            rv = SA_ERR_TMT;
        else if (res < 0)
            rv = SA_ERR_SYS;
    }
    else {
        /* user-space buffered I/O */
        res = 0;
        for (;;) {
            if ((int)nBufReq <= sa->nReadLen) {
                /* buffer holds enough data, so just use this */
                memmove(cpBuf, sa->cpReadBuf, nBufReq);
                memmove(sa->cpReadBuf, sa->cpReadBuf+nBufReq, sa->nReadLen-nBufReq);
                sa->nReadLen -= nBufReq;
                res += nBufReq;
            }
            else {
                if (sa->nReadLen > 0) {
                    /* fetch already existing buffer contents as a start */
                    memmove(cpBuf, sa->cpReadBuf, (size_t)sa->nReadLen);
                    nBufReq -= sa->nReadLen;
                    cpBuf   += sa->nReadLen;
                    res     += sa->nReadLen;
                    sa->nReadLen = 0;
                }
                if (sa->nWriteLen > 0)
                    (void)sa_flush(sa);
                if ((int)nBufReq >= sa->nReadSize) {
                    /* buffer is too small at all, so read directly */
                    n = sa_read_raw(sa, cpBuf, (int)nBufReq);
                    if (n > 0)
                        res += n;
                    else if (n == 0)
                        rv = (res == 0 ? SA_ERR_EOF : SA_OK);
                    else if (n < 0 && errno == ETIMEDOUT)
                        rv = (res == 0 ? SA_ERR_TMT : SA_OK);
                    else if (n < 0)
                        rv = (res == 0 ? SA_ERR_SYS : SA_OK);
                }
                else {
                    /* fill buffer with new data */
                    n = sa_read_raw(sa, sa->cpReadBuf, sa->nReadSize);
                    if (n < 0 && errno == ETIMEDOUT)
                        /* timeout on this read, but perhaps ok as a whole */
                        rv = (res == 0 ? SA_ERR_TMT : SA_OK);
                    else if (n < 0)
                        /* error on this read, but perhaps ok as a whole */
                        rv = (res == 0 ? SA_ERR_SYS : SA_OK);
                    else if (n == 0)
                        /* EOF on this read, but perhaps ok as a whole */
                        rv = (res == 0 ? SA_ERR_EOF : SA_OK);
                    else {
                        sa->nReadLen = n;
                        continue; /* REPEAT OPERATION! */
                    }
                }
            }
            break;
        }
    }

    /* pass number of actually read bytes to caller */
    if (nBufRes != NULL)
        *nBufRes = (size_t)res;

    return SA_RC(rv);
}

/* read data from socket until [CR]LF (convenience function) */
sa_rc_t sa_readln(sa_t *sa, char *cpBuf, size_t nBufReq, size_t *nBufRes)
{
    char c;
    size_t n;
    size_t res;
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL || cpBuf == NULL || nBufReq == 0)
        return SA_RC(SA_ERR_ARG);

    /* reading is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* we just perform a plain sa_read() per character, because if
       buffers are enabled, this is not as stupid as it looks at the first
       hand and if buffers are disabled, there is no better solution
       anyway. */
    rv = SA_OK;
    res = 0;
    while (res < (nBufReq-1)) {
        rv = sa_read(sa, &c, 1, &n);
        if (rv != SA_OK)
            break;
        if (n == 0)
            break;
        cpBuf[res++] = c;
        if (c == '\n')
            break;
    }
    cpBuf[res] = '\0';

    /* pass number of actually read characters to caller */
    if (nBufRes != NULL)
        *nBufRes = res;

    return SA_RC(rv);
}

/* internal raw write operation */
static int sa_write_raw(sa_t *sa, const char *cpBuf, int nBufLen)
{
    int rv;
#if !(defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO))
    fd_set fds;
    struct timeval tv;
#endif

    /* if timeout is enabled, perform explicit/smart blocking instead
       of implicitly/hard blocking in the write(2) system call */
#if !(defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO))
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_WRITE])) {
        FD_ZERO(&fds);
        FD_SET(sa->fdSocket, &fds);
        memcpy(&tv, &sa->tvTimeout[SA_TIMEOUT_WRITE], sizeof(struct timeval));
        do {
            rv = SA_SC_CALL_5(sa, select, sa->fdSocket+1, (fd_set *)NULL, &fds, (fd_set *)NULL, &tv);
        } while (rv == -1 && errno == EINTR);
        if (rv == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
#endif

    /* perform write operation on underlying socket */
    do {
        rv = (int)SA_SC_CALL_3(sa, write, sa->fdSocket, cpBuf, (size_t)nBufLen);
    } while (rv == -1 && errno == EINTR);

#if defined(SO_RCVTIMEO) && defined(USE_SO_RCVTIMEO) && defined(SO_SNDTIMEO) && defined(USE_SO_SNDTIMEO)
    if (rv == -1 && errno == EWOULDBLOCK)
        errno = ETIMEDOUT;
#endif

    return rv;
}

/* write data to socket */
sa_rc_t sa_write(sa_t *sa, const char *cpBuf, size_t nBufReq, size_t *nBufRes)
{
    int n;
    int res;
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL || cpBuf == NULL || nBufReq == 0)
        return SA_RC(SA_ERR_ARG);

    /* writing is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    rv = SA_OK;
    if (sa->nWriteSize == 0) {
        /* user-space unbuffered I/O */
        res = sa_write_raw(sa, cpBuf, (int)nBufReq);
        if (res < 0 && errno == ETIMEDOUT)
            rv = SA_ERR_TMT;
        else if (res < 0)
            rv = SA_ERR_SYS;
    }
    else {
        /* user-space buffered I/O */
        if ((int)nBufReq > (sa->nWriteSize - sa->nWriteLen)) {
            /* not enough space in buffer, so flush buffer first */
            (void)sa_flush(sa);
        }
        res = 0;
        if ((int)nBufReq >= sa->nWriteSize) {
            /* buffer too small at all, so write immediately */
            while (nBufReq > 0) {
                n = sa_write_raw(sa, cpBuf, (int)nBufReq);
                if (n < 0 && errno == ETIMEDOUT)
                    rv = (res == 0 ? SA_ERR_TMT : SA_OK);
                else if (n < 0)
                    rv = (res == 0 ? SA_ERR_SYS : SA_OK);
                if (n <= 0)
                    break;
                nBufReq  -= n;
                cpBuf    += n;
                res      += n;
            }
        }
        else {
            /* (again) enough sprace in buffer, so store data */
            memmove(sa->cpWriteBuf+sa->nWriteLen, cpBuf, nBufReq);
            sa->nWriteLen += nBufReq;
            res = (int)nBufReq;
        }
    }

    /* pass number of actually written bytes to caller */
    if (nBufRes != NULL)
        *nBufRes = (size_t)res;

    return SA_RC(rv);
}

/* output callback function context for sa_writef() */
typedef struct {
    sa_t *sa;
    sa_rc_t rv;
} sa_writef_cb_t;

/* output callback function for sa_writef() */
static int sa_writef_cb(void *_ctx, const char *buffer, size_t bufsize)
{
    size_t n;
    sa_writef_cb_t *ctx = (sa_writef_cb_t *)_ctx;

    if ((ctx->rv = sa_write(ctx->sa, buffer, bufsize, &n)) != SA_OK)
        return -1;
    return (int)n;
}

/* write formatted string to socket (convenience function) */
sa_rc_t sa_writef(sa_t *sa, const char *cpFmt, ...)
{
    va_list ap;
    int n;
    sa_writef_cb_t ctx;

    /* argument sanity check(s) */
    if (sa == NULL || cpFmt == NULL)
        return SA_RC(SA_ERR_ARG);

    /* writing is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* format string into temporary buffer */
    va_start(ap, cpFmt);
    ctx.sa = sa;
    ctx.rv = SA_OK;
    n = sa_mvxprintf(sa_writef_cb, &ctx, cpFmt, ap);
    if (n == -1 && ctx.rv == SA_OK)
        ctx.rv = SA_ERR_FMT;
    va_end(ap);

    return ctx.rv;
}

/* flush write/outgoing I/O buffer */
sa_rc_t sa_flush(sa_t *sa)
{
    int n;
    sa_rc_t rv;

    /* argument sanity check(s) */
    if (sa == NULL)
        return SA_RC(SA_ERR_ARG);

    /* flushing is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* try to flush buffer */
    rv = SA_OK;
    if (sa->nWriteSize > 0) {
        while (sa->nWriteLen > 0) {
            n = sa_write_raw(sa, sa->cpWriteBuf, sa->nWriteLen);
            if (n < 0 && errno == ETIMEDOUT)
                rv = SA_ERR_TMT;
            else if (n < 0)
                rv = SA_ERR_SYS;
            if (n <= 0)
                break;
            memmove(sa->cpWriteBuf, sa->cpWriteBuf+n, (size_t)(sa->nWriteLen-n));
            sa->nWriteLen -= n;
        }
        sa->nWriteLen = 0;
    }
    return SA_RC(rv);
}

/* shutdown a socket connection */
sa_rc_t sa_shutdown(sa_t *sa, const char *flags)
{
    int how;

    /* argument sanity check(s) */
    if (sa == NULL || flags == NULL)
        return SA_RC(SA_ERR_ARG);

    /* shutdown is only possible for stream communication */
    if (sa->eType != SA_TYPE_STREAM)
        return SA_RC(SA_ERR_USE);

    /* at least a connection has to exist */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* determine flags for shutdown(2) */
    how = 0;
    if (strcmp(flags, "r") == 0)
        how = SHUT_RD;
    else if (strcmp(flags, "w") == 0)
        how = SHUT_WR;
    else if (strcmp(flags, "rw") == 0 || strcmp(flags, "wr") == 0)
        how = SHUT_RDWR;
    else
        return SA_RC(SA_ERR_ARG);

    /* flush write buffers */
    if ((how & SHUT_WR) || (how & SHUT_RDWR))
        (void)sa_flush(sa);

    /* perform shutdown operation on underlying socket */
    if (shutdown(sa->fdSocket, how) == -1)
        return SA_RC(SA_ERR_SYS);

    return SA_OK;
}

/* receive data via socket */
sa_rc_t sa_recv(sa_t *sa, sa_addr_t **raddr, char *buf, size_t buflen, size_t *bufdone)
{
    sa_rc_t rv;
    union {
        struct sockaddr_in sa4;
#ifdef AF_INET6
        struct sockaddr_in6 sa6;
#endif
    } sa_buf;
    socklen_t sa_size;
    ssize_t n;
    int k;
    fd_set fds;
    struct timeval tv;

    /* argument sanity check(s) */
    if (sa == NULL || buf == NULL || buflen == 0 || raddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* receiving is only possible for datagram communication */
    if (sa->eType != SA_TYPE_DATAGRAM)
        return SA_RC(SA_ERR_USE);

    /* at least a sa_bind() has to be performed */
    if (sa->fdSocket == -1)
        return SA_RC(SA_ERR_USE);

    /* if timeout is enabled, perform explicit/smart blocking instead
       of implicitly/hard blocking in the recvfrom(2) system call */
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_READ])) {
        FD_ZERO(&fds);
        FD_SET(sa->fdSocket, &fds);
        memcpy(&tv, &sa->tvTimeout[SA_TIMEOUT_READ], sizeof(struct timeval));
        do {
            k = SA_SC_CALL_5(sa, select, sa->fdSocket+1, &fds, (fd_set *)NULL, (fd_set *)NULL, &tv);
        } while (k == -1 && errno == EINTR);
        if (k == 0)
            errno = ETIMEDOUT;
        if (k <= 0)
            return SA_RC(SA_ERR_SYS);
    }

    /* perform receive operation on underlying socket */
    sa_size = (socklen_t)sizeof(sa_buf);
    if ((n = SA_SC_CALL_6(sa, recvfrom, sa->fdSocket, buf, buflen, 0,
                          (struct sockaddr *)&sa_buf, &sa_size)) == -1)
        return SA_RC(SA_ERR_SYS);

    /* create result address object */
    if ((rv = sa_addr_create(raddr)) != SA_OK)
        return rv;
    if ((rv = sa_addr_s2a(*raddr, (struct sockaddr *)&sa_buf, sa_size)) != SA_OK) {
        (void)sa_addr_destroy(*raddr);
        return rv;
    }

    /* pass actual number of received bytes to caller */
    if (bufdone != NULL)
        *bufdone = (size_t)n;

    return SA_OK;
}

/* send data via socket */
sa_rc_t sa_send(sa_t *sa, sa_addr_t *raddr, const char *buf, size_t buflen, size_t *bufdone)
{
    ssize_t n;
    int k;
    fd_set fds;
    sa_rc_t rv;
    struct timeval tv;

    /* argument sanity check(s) */
    if (sa == NULL || buf == NULL || buflen == 0 || raddr == NULL)
        return SA_RC(SA_ERR_ARG);

    /* sending is only possible for datagram communication */
    if (sa->eType != SA_TYPE_DATAGRAM)
        return SA_RC(SA_ERR_USE);

    /* lazy creation of underlying socket */
    if (sa->fdSocket == -1)
        if ((rv = sa_socket_init(sa, raddr->nFamily)) != SA_OK)
            return rv;

    /* if timeout is enabled, perform explicit/smart blocking instead
       of implicitly/hard blocking in the sendto(2) system call */
    if (!SA_TVISZERO(sa->tvTimeout[SA_TIMEOUT_WRITE])) {
        FD_ZERO(&fds);
        FD_SET(sa->fdSocket, &fds);
        memcpy(&tv, &sa->tvTimeout[SA_TIMEOUT_WRITE], sizeof(struct timeval));
        do {
            k = SA_SC_CALL_5(sa, select, sa->fdSocket+1, (fd_set *)NULL, &fds, (fd_set *)NULL, &tv);
        } while (k == -1 && errno == EINTR);
        if (k == 0)
            errno = ETIMEDOUT;
        if (k <= 0)
            return SA_RC(SA_ERR_SYS);
    }

    /* perform send operation on underlying socket */
    if ((n = SA_SC_CALL_6(sa, sendto, sa->fdSocket, buf, buflen, 0, raddr->saBuf, raddr->slBuf)) == -1)
        return SA_RC(SA_ERR_SYS);

    /* pass actual number of sent bytes to caller */
    if (bufdone != NULL)
        *bufdone = (size_t)n;

    return SA_OK;
}

/* send formatted string to socket (convenience function) */
sa_rc_t sa_sendf(sa_t *sa, sa_addr_t *raddr, const char *cpFmt, ...)
{
    va_list ap;
    va_list apbak;
    int nBuf;
    char *cpBuf;
    sa_rc_t rv;
    char caBuf[1024];

    /* argument sanity check(s) */
    if (sa == NULL || raddr == NULL || cpFmt == NULL)
        return SA_RC(SA_ERR_ARG);

    /* format string into temporary buffer */
    va_start(ap, cpFmt);
    va_copy(apbak, ap);
    if ((nBuf = sa_mvsnprintf(NULL, 0, cpFmt, ap)) == -1)
        return SA_RC(SA_ERR_FMT);
    va_copy(ap, apbak);
    if ((nBuf+1) > (int)sizeof(caBuf)) {
        /* requires a larger buffer, so allocate dynamically */
        if ((cpBuf = (char *)malloc((size_t)(nBuf+1))) == NULL)
            return SA_RC(SA_ERR_MEM);
    }
    else {
        /* fits into small buffer, so allocate statically */
        cpBuf = caBuf;
    }
    rv = SA_OK;
    if (sa_mvsnprintf(cpBuf, (size_t)(nBuf+1), cpFmt, ap) == -1)
        rv = SA_ERR_FMT;
    va_end(ap);

    /* pass-through to sa_send() */
    if (rv == SA_OK)
        rv = sa_send(sa, raddr, cpBuf, (size_t)nBuf, NULL);

    /* cleanup dynamically allocated buffer */
    if ((nBuf+1) > (int)sizeof(caBuf))
        free(cpBuf);

    return rv;
}

/* return error string */
char *sa_error(sa_rc_t rv)
{
    char *sz;

    /* translate result value into corresponding string */
    if      (rv == SA_OK)      sz = "Everything Ok";
    else if (rv == SA_ERR_ARG) sz = "Invalid Argument";
    else if (rv == SA_ERR_USE) sz = "Invalid Use Or Context";
    else if (rv == SA_ERR_MEM) sz = "Not Enough Memory";
    else if (rv == SA_ERR_MTC) sz = "Matching Failed";
    else if (rv == SA_ERR_EOF) sz = "Remote end closed connection";
    else if (rv == SA_ERR_TMT) sz = "Communication Timeout";
    else if (rv == SA_ERR_SYS) sz = "Operating System Error";
    else if (rv == SA_ERR_NET) sz = "Networking Error";
    else if (rv == SA_ERR_FMT) sz = "Formatting Error";
    else if (rv == SA_ERR_IMP) sz = "Implementation Not Available";
    else if (rv == SA_ERR_INT) sz = "Internal Error";
    else                       sz = "Invalid Result Code";
    return sz;
}

