#ident "@(#)xmalloc.c 1.1"
/*
 * xmalloc.c - malloc/free wrappers 
 * Look at all we do -- just to remeber the size of our alloc's. 
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "xmalloc.h"


/* malloc something, does not return if we encounter an error */
void *
xmalloc(size_t size) 
{
	void *ptr = calloc(1, (size) + xalloc_extra);

	if (ptr == NULL)
		abort();

	ptr = ((char *)ptr) + xalloc_extra;
	xalloc_size(ptr) = (size);

	return ptr;
}


/* free something. some magic to get around alloc size caching */
/* Different from free(), it takes a pointer-to-a-pointer */
void
xfree_real (void **ptr)
{
	char *tmp = *((char **)(ptr));
	
	if (tmp) {
		free( (tmp - xalloc_extra) );
		*((char **)(ptr)) = NULL;
	}
}

/* mmm realloc */
void *
xrealloc (void *ptr, size_t size)
{
	void *nw = NULL;

	if (ptr)  {
		if (size) {
			size_t msize = xalloc_size(ptr);

			msize = msize > size ? size : msize;
			nw = xmalloc(size);
			memmove(nw, ptr, msize);
		}
		xfree(&ptr);
	} else if (size)
		nw = xmalloc(size);
	return nw;
}
