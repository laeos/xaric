#ifndef xmalloc_h__
#define xmalloc_h__
/*
 * xmalloc.h - malloc/free wrappers 
 * Copyright (C) 2000 Rex Feany <laeos@laeos.net>
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
 * @(#)xmalloc.h 1.2
 *
 */

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif


/* Figure out alloca stuff */
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


/* How much more do we have to allocate? */
#define xalloc_extra		(sizeof(void *))

/* The location where we store this allocation's size */
#define xalloc_size(PTR)	(*(int *)( ((char *)PTR) - sizeof(void *)))

/* malloc something, does not return if we encounter an error */
void *xmalloc(size_t size);

/* free something. some magic to get around alloc size caching */
/* Different from free(), it takes a pointer-to-a-pointer */

#define xfree(x)	xfree_real( (void **) (x) )

void xfree_real(void **ptr);

/* mmm realloc */
void *xrealloc (void *ptr, size_t size);


#endif /* xmalloc_h__ */
