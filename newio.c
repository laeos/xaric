/*
 * newio.c: This is some handy stuff to deal with file descriptors in a way
 * much like stdio's FILE pointers 
 *
 * IMPORTANT NOTE:  If you use the routines here-in, you shouldn't switch to
 * using normal reads() on the descriptors cause that will cause bad things
 * to happen.  If using any of these routines, use them all 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "ircaux.h"
#include "newio.h"

#ifdef ISC22
#include <sys/bsdtypes.h>
#endif				/* ISC22 */

#include "irc_std.h"

#define IO_BUFFER_SIZE 512

#ifdef HAVE_SYSCONF
#define IO_ARRAYLEN sysconf(_SC_OPEN_MAX)
#else
#ifdef FD_SETSIZE
#define IO_ARRAYLEN FDSETSIZE
#else
#define IO_ARRAYLEN NFDBITS
#endif
#endif

typedef struct myio_struct {
    char buffer[IO_BUFFER_SIZE + 1];
    unsigned int read_pos, write_pos;
    unsigned misc_flags;
} MyIO;

#define IO_SOCKET 1

static struct timeval right_away = { 0L, 0L };
static MyIO **io_rec = NULL;

static struct timeval dgets_timer = { 0, 0 };
static struct timeval *timer = NULL;
int dgets_errno = 0;

static void init_io(void);

/*
 * dgets_timeout: does what you'd expect.  Sets a timeout in seconds for
 * dgets to read a line.  if second is -1, then make it a poll.
 */
extern time_t dgets_timeout(int sec)
{
    time_t old_timeout = dgets_timer.tv_sec;

    if (sec) {
	dgets_timer.tv_sec = (sec == -1) ? 0 : sec;
	dgets_timer.tv_usec = 0;
	timer = &dgets_timer;
    } else
	timer = NULL;
    return old_timeout;
}

static void init_io(void)
{
    static int first = 1;

    if (first) {
	int c, max_fd = IO_ARRAYLEN;

	io_rec = (MyIO **) new_malloc(sizeof(MyIO *) * max_fd);
	for (c = 0; c < max_fd; c++)
	    io_rec[c] = NULL;
	(void) dgets_timeout(-1);
	first = 0;
    }
}

/*
 * dgets: works much like fgets except on descriptor rather than file
 * pointers.  Returns the number of character read in.  Returns 0 on EOF and
 * -1 on a timeout (see dgets_timeout()) 
 */
int dgets(char *str, int len, int des, char *specials)
{
#if 1
    int cnt = 0, c;
    fd_set rd;
    int BufferEmpty;

    init_io();
    if (io_rec[des] == NULL) {
	io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
	io_rec[des]->read_pos = 0;
	io_rec[des]->write_pos = 0;
	io_rec[des]->misc_flags = 0;
    }

    while (1) {
	if ((BufferEmpty = (io_rec[des]->read_pos == io_rec[des]->write_pos))) {
	    if (BufferEmpty) {
		io_rec[des]->read_pos = 0;
		io_rec[des]->write_pos = 0;
	    }
	    FD_ZERO(&rd);
	    FD_SET(des, &rd);
	    switch (select(des + 1, &rd, NULL, NULL, timer)) {
	    case 0:
		{
		    str[cnt] = (char) 0;
		    dgets_errno = 0;
		    return (-1);
		}
	    default:
		{
		    c = read(des, io_rec[des]->buffer + io_rec[des]->write_pos, IO_BUFFER_SIZE - io_rec[des]->write_pos);

		    if (c <= 0) {
			if (c == 0)
			    dgets_errno = -1;
			else
			    dgets_errno = errno;
			return 0;
		    }
		    io_rec[des]->write_pos += c;
		    break;
		}
	    }
	}
	while (io_rec[des]->read_pos < io_rec[des]->write_pos) {
	    if (((str[cnt++] = io_rec[des]->buffer[(io_rec[des]->read_pos)++])
		 == '\n') || (cnt == len)) {
		dgets_errno = 0;
		str[cnt] = (char) 0;
		return (cnt);
	    }
	}
    }
#else
    int cnt = 0, c, s;
    fd_set rd;
    int BufferEmpty;

    init_io();
    if (io_rec[des] == NULL) {
	io_rec[des] = (MyIO *) new_malloc(sizeof(MyIO));
	io_rec[des]->read_pos = 0;
	io_rec[des]->write_pos = 0;
	io_rec[des]->misc_flags = 0;
    }

    if (io_rec[des]->read_pos == io_rec[des]->write_pos) {
	io_rec[des]->read_pos = 0;
	io_rec[des]->write_pos = 0;

	FD_ZERO(&rd);
	FD_SET(des, &rd);
	s = select(des + 1, &rd, 0, 0, timer);
	if (s <= 0) {
	    strcpy(str, empty_str);
	    dgets_errno = (s == 0) ? -1 : errno;
	    return 0;
	} else {
	    c = read(des, io_rec[des]->buffer + io_rec[des]->write_pos, IO_BUFFER_SIZE - io_rec[des]->write_pos);

	    if (c <= 0) {
		dgets_errno = (s == 0) ? -1 : errno;
		return 0;
	    }
	    io_rec[des]->write_pos += c;
	}
    }

    while (io_rec[des]->read_pos < io_rec[des]->write_pos) {
	str[cnt] = io_rec[des]->buffer[io_rec[des]->read_pos];
	io_rec[des]->read_pos++;
	if (str[cnt] == '\n')
	    break;
	cnt++;
    }

    /* Add on newline if its not there. */
    if (str[cnt] != '\n')
	str[++cnt] = '\n';
    dgets_errno = 0;
    str[++cnt] = 0;
    return cnt - 1;
#endif
}

/*
 * new_select: works just like select(), execpt I trimmed out the excess
 * parameters I didn't need.  
 */
int new_select(fd_set * rd, fd_set * wd, struct timeval *timeout)
{
    int i, set = 0;
    fd_set new;
    struct timeval thetimeout, *newtimeout = &thetimeout;
    int max_fd = -1;
    static int num_fd = 0;

    if (!num_fd) {
	num_fd = IO_ARRAYLEN;	/* why do it a zillion times? */
	if (num_fd > FD_SETSIZE)
	    num_fd = FD_SETSIZE;
    }

    if (timeout)
	memcpy(newtimeout, timeout, sizeof(struct timeval));
    else
	newtimeout = NULL;

    init_io();
    FD_ZERO(&new);
    for (i = 0; i < num_fd; i++) {
	if (i > max_fd && ((rd && FD_ISSET(i, rd)) || (wd && FD_ISSET(i, wd))))
	    max_fd = i;
	if (io_rec[i]) {
	    if (io_rec[i]->read_pos < io_rec[i]->write_pos) {
		FD_SET(i, &new);
		set = 1;
	    }
	}
    }
    if (set) {
	set = 0;
	if (select(max_fd + 1, rd, wd, NULL, &right_away) <= 0)
	    FD_ZERO(rd);
	for (i = 0; i < num_fd; i++) {
	    if ((FD_ISSET(i, rd)) || (FD_ISSET(i, &new))) {
		set++;
		FD_SET(i, rd);
	    } else
		FD_CLR(i, rd);
	}
	return (set);
    }
    return (select(max_fd + 1, rd, wd, NULL, newtimeout));
}

/* new_close: works just like close */
void new_close(int des)
{
    if (des < 0 || !io_rec)
	return;
    new_free((char **) &(io_rec[des]));
    close(des);
}

/* set's socket options */
extern void set_socket_options(int s)
{
    int opt = 1;
    int optlen = sizeof(opt);

#ifndef NO_STRUCT_LINGER
    struct linger lin = { 0 };
    lin.l_onoff = lin.l_linger = 0;
    (void) setsockopt(s, SOL_SOCKET, SO_LINGER, (void *) &lin, optlen);
    opt = 1;
#endif				/* NO_STRUCT_LINGER */
    (void) setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *) &opt, optlen);
    opt = 1;
    (void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (void *) &opt, optlen);
}
