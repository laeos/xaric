#ifndef threads_h__
#define threads_h__
/*
 * threads.h - wrappers around pthread stuff.
 * Copyright (C) 1998, 1999, 2000 Rex Feany <laeos@laeos.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @(#)threads.h 1.1
 *
 */

#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
# define HAVE_THREADS
#else
# ifdef HAVE_LIBPTHREADS
#  include <pthreads.h>		/* this is a total guess */
#  define HAVE_THREADS
# endif
#endif

#ifdef HAVE_THREADS
# define THR_EXIT()		pthread_exit(NULL); return NULL

static inline int THR_CREATE(void *(*func) (void *), void *arg)
{
    pthread_t x;

    int retval = pthread_create(&x, NULL, (void *(*)(void *)) func, arg);

    if (retval == 0)
	pthread_detach(x);
    return retval;
}

#else				/* HAVE_THREADS */
#  define THR_EXIT()		return NULL
#  define THR_CREATE(x, y)	x((y))
#endif				/* HAVE_THREADS */

#endif				/* threads_h__ */
