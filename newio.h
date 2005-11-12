/*
 * newio.h - header for newio.c
 *
 * written by matthew green
 *
 * copyright (c) 1995;
 *
 * @(#)$Id$
 */

#ifndef __newio_h_
# define __newio_h_

#ifdef ESIX
	void	mark_socket(int);
	void	unmark_socket(int);
#endif
	time_t	dgets_timeout(int);
	int	dgets(char *, int, int, char *);
	int	new_select(fd_set *, fd_set *, struct timeval *);
	void	new_close(int);
	void	set_socket_options(int);

#endif /* __newio_h_ */
