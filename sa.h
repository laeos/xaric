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
**  sa.h: socket abstraction API
*/

#ifndef __SA_H__
#define __SA_H__

/* system definitions of "size_t", "socklen_t", "struct sockaddr *" */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

/* include optional Autoconf header */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* fallback for POSIX socklen_t */
#if defined(HAVE_CONFIG_H) && !defined(HAVE_SOCKLEN_T)
typedef int socklen_t;
#endif

/* embedding support */
#ifdef SA_PREFIX
#if defined(__STDC__) || defined(__cplusplus)
#define __SA_CONCAT(x,y) x ## y
#define SA_CONCAT(x,y) __SA_CONCAT(x,y)
#else
#define __SA_CONCAT(x) x
#define SA_CONCAT(x,y) __SA_CONCAT(x)y
#endif
#define sa_addr_st      SA_CONCAT(SA_PREFIX,sa_addr_st)
#define sa_addr_t       SA_CONCAT(SA_PREFIX,sa_addr_t)
#define sa_rc_t         SA_CONCAT(SA_PREFIX,sa_rc_t)
#define sa_st           SA_CONCAT(SA_PREFIX,sa_st)
#define sa_t            SA_CONCAT(SA_PREFIX,sa_t)
#define sa_addr_create  SA_CONCAT(SA_PREFIX,sa_addr_create)
#define sa_addr_destroy SA_CONCAT(SA_PREFIX,sa_addr_destroy)
#define sa_addr_u2a     SA_CONCAT(SA_PREFIX,sa_addr_u2a)
#define sa_addr_s2a     SA_CONCAT(SA_PREFIX,sa_addr_s2a)
#define sa_addr_a2u     SA_CONCAT(SA_PREFIX,sa_addr_a2u)
#define sa_addr_a2s     SA_CONCAT(SA_PREFIX,sa_addr_a2s)
#define sa_addr_match   SA_CONCAT(SA_PREFIX,sa_addr_match)
#define sa_create       SA_CONCAT(SA_PREFIX,sa_create)
#define sa_destroy      SA_CONCAT(SA_PREFIX,sa_destroy)
#define sa_type         SA_CONCAT(SA_PREFIX,sa_type)
#define sa_timeout      SA_CONCAT(SA_PREFIX,sa_timeout)
#define sa_buffer       SA_CONCAT(SA_PREFIX,sa_buffer)
#define sa_option       SA_CONCAT(SA_PREFIX,sa_option)
#define sa_syscall      SA_CONCAT(SA_PREFIX,sa_syscall)
#define sa_bind         SA_CONCAT(SA_PREFIX,sa_bind)
#define sa_connect      SA_CONCAT(SA_PREFIX,sa_connect)
#define sa_listen       SA_CONCAT(SA_PREFIX,sa_listen)
#define sa_accept       SA_CONCAT(SA_PREFIX,sa_accept)
#define sa_getremote    SA_CONCAT(SA_PREFIX,sa_getremote)
#define sa_getlocal     SA_CONCAT(SA_PREFIX,sa_getlocal)
#define sa_getfd        SA_CONCAT(SA_PREFIX,sa_getfd)
#define sa_shutdown     SA_CONCAT(SA_PREFIX,sa_shutdown)
#define sa_read         SA_CONCAT(SA_PREFIX,sa_read)
#define sa_readln       SA_CONCAT(SA_PREFIX,sa_readln)
#define sa_write        SA_CONCAT(SA_PREFIX,sa_write)
#define sa_writef       SA_CONCAT(SA_PREFIX,sa_writef)
#define sa_flush        SA_CONCAT(SA_PREFIX,sa_flush)
#define sa_recv         SA_CONCAT(SA_PREFIX,sa_recv)
#define sa_send         SA_CONCAT(SA_PREFIX,sa_send)
#define sa_sendf        SA_CONCAT(SA_PREFIX,sa_sendf)
#define sa_id           SA_CONCAT(SA_PREFIX,sa_id)
#define sa_error        SA_CONCAT(SA_PREFIX,sa_error)
#endif

/* socket address abstraction object type */
struct sa_addr_st;
typedef struct sa_addr_st sa_addr_t;

/* socket abstraction object type */
struct sa_st;
typedef struct sa_st sa_t;

/* socket connection types */
typedef enum {
    SA_TYPE_STREAM,
    SA_TYPE_DATAGRAM
} sa_type_t;

/* return codes */
typedef enum {
    SA_OK,			/* Everything Ok */
    SA_ERR_ARG,			/* Invalid Argument */
    SA_ERR_USE,			/* Invalid Use Or Context */
    SA_ERR_MEM,			/* Not Enough Memory */
    SA_ERR_MTC,			/* Matching Failed */
    SA_ERR_EOF,			/* End Of Communication */
    SA_ERR_TMT,			/* Communication Timeout */
    SA_ERR_SYS,			/* Operating System Error */
    SA_ERR_NET,			/* Networking Error */
    SA_ERR_FMT,			/* Formatting Error */
    SA_ERR_IMP,			/* Implementation Not Available */
    SA_ERR_INT			/* Internal Error */
} sa_rc_t;

/* list of timeouts */
typedef enum {
    SA_TIMEOUT_ALL = -1,
    SA_TIMEOUT_ACCEPT = 0,
    SA_TIMEOUT_CONNECT = 1,
    SA_TIMEOUT_READ = 2,
    SA_TIMEOUT_WRITE = 3
} sa_timeout_t;

/* list of buffers */
typedef enum {
    SA_BUFFER_READ,
    SA_BUFFER_WRITE
} sa_buffer_t;

/* list of options */
typedef enum {
    SA_OPTION_NAGLE = 0,
    SA_OPTION_LINGER = 1,
    SA_OPTION_REUSEADDR = 2,
    SA_OPTION_REUSEPORT = 3,
    SA_OPTION_NONBLOCK = 4
} sa_option_t;

/* list of system calls */
typedef enum {
    SA_SYSCALL_CONNECT,
    SA_SYSCALL_ACCEPT,
    SA_SYSCALL_SELECT,
    SA_SYSCALL_READ,
    SA_SYSCALL_WRITE,
    SA_SYSCALL_RECVFROM,
    SA_SYSCALL_SENDTO
} sa_syscall_t;

/* unique library identifier */
extern const char sa_id[];

/* address object operations */
extern sa_rc_t sa_addr_create(sa_addr_t ** __saa);
extern sa_rc_t sa_addr_destroy(sa_addr_t * __saa);

/* address operations */
extern sa_rc_t sa_addr_u2a(sa_addr_t * __saa, const char *__uri, ...);
extern sa_rc_t sa_addr_s2a(sa_addr_t * __saa, const struct sockaddr *__sabuf, socklen_t __salen);
extern sa_rc_t sa_addr_a2u(const sa_addr_t * __saa, char **__uri);
extern sa_rc_t sa_addr_a2s(const sa_addr_t * __saa, struct sockaddr **__sabuf, socklen_t * __salen);
extern sa_rc_t sa_addr_match(const sa_addr_t * __saa1, const sa_addr_t * __saa2, int __prefixlen);

/* socket object operations */
extern sa_rc_t sa_create(sa_t ** __sa);
extern sa_rc_t sa_destroy(sa_t * __sa);

/* socket parameter operations */
extern sa_rc_t sa_type(sa_t * __sa, sa_type_t __id);
extern sa_rc_t sa_timeout(sa_t * __sa, sa_timeout_t __id, long __sec, long __usec);
extern sa_rc_t sa_buffer(sa_t * __sa, sa_buffer_t __id, size_t __size);
extern sa_rc_t sa_option(sa_t * __sa, sa_option_t __id, ...);
extern sa_rc_t sa_syscall(sa_t * __sa, sa_syscall_t __id, void (*__fptr) (void), void *__fctx);

/* socket connection operations */
extern sa_rc_t sa_bind(sa_t * __sa, const sa_addr_t * __laddr);
extern sa_rc_t sa_connect(sa_t * __sa, const sa_addr_t * __raddr);
extern sa_rc_t sa_listen(sa_t * __sa, int __backlog);
extern sa_rc_t sa_accept(sa_t * __sa, sa_addr_t ** __caddr, sa_t ** __csa);
extern sa_rc_t sa_getremote(sa_t * __sa, sa_addr_t ** __raddr);
extern sa_rc_t sa_getlocal(sa_t * __sa, sa_addr_t ** __laddr);
extern sa_rc_t sa_getfd(sa_t * __sa, int *__fd);
extern sa_rc_t sa_shutdown(sa_t * __sa, const char *__flags);

/* socket input/output operations (stream communication) */
extern sa_rc_t sa_read(sa_t * __sa, char *__buf, size_t __buflen, size_t * __bufdone);
extern sa_rc_t sa_readln(sa_t * __sa, char *__buf, size_t __buflen, size_t * __bufdone);
extern sa_rc_t sa_write(sa_t * __sa, const char *__buf, size_t __buflen, size_t * __bufdone);
extern sa_rc_t sa_writef(sa_t * __sa, const char *__fmt, ...);
extern sa_rc_t sa_flush(sa_t * __sa);

/* socket input/output operations (datagram communication) */
extern sa_rc_t sa_recv(sa_t * __sa, sa_addr_t ** __raddr, char *__buf, size_t __buflen, size_t * __bufdone);
extern sa_rc_t sa_send(sa_t * __sa, sa_addr_t * __raddr, const char *__buf, size_t __buflen, size_t * __bufdone);
extern sa_rc_t sa_sendf(sa_t * __sa, sa_addr_t * __raddr, const char *__fmt, ...);

/* error handling operations */
extern const char *sa_error(sa_rc_t __rv);

#endif				/* __SA_H__ */
