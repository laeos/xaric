#ident "@(#)freeaddrinfo.c 1.3"
/*
 * freeaddrinfo.c - freeaddrinfo()
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

#include "gai.h"

void
freeaddrinfo (struct addrinfo *aihead)
{
	struct addrinfo	*ai, *ainext;

	for (ai = aihead; ai != NULL; ai = ainext) {
		if (ai->ai_addr != NULL)
			free(ai->ai_addr);	/* socket address structure */

		if (ai->ai_canonname != NULL)
			free(ai->ai_canonname);

		ainext = ai->ai_next;		/* can't fetch ai_next after free() */
		free(ai);			/* the addrinfo{} itself */
	}
}
