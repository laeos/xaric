#ident "@(#)tst_build.c 1.3"
/*
 * tst_build.c - given a table, init the tree
 * (c) 2000  Rex Feany <laeos@laeos.net> 
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

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "xdebug.h"
#include "tst.h"

/* for XDEBUG */
#define MODULE_ID	XD_TST

void 
tst_build(struct tst_head *th, void *data, int elen, int noffset, int nelt)
{
	unsigned char *dp = (unsigned char *) data;

	XDEBUG(15, "begining");

	for ( ; nelt > 0 ; nelt-- ) {
		XDEBUG(15, "[offset:%p] [nptr:%p] name: '%s' dp: '%p'", noffset, *(char **)(dp + noffset), *(char **)(dp + noffset), dp);
		tst_insert(th, *(char **)(dp + noffset), dp);
		dp += elen;
	}
	XDEBUG(15, "all done!");
}
