#ident "@(#)tst_remove.c 1.2"
/*
 * tst_remove.c - remove node from tst
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "tst.h"
#include "xmalloc.h"


/* remove a node recursivly */
static struct tst_node *
tst_recurse_remove (struct tst_node *p, const char *string, void **old)
{
	unsigned char c = tolower(*string);

	if (p == NULL) 
		return NULL;

	if ( c < p->tn_split )
		p->tn_low = tst_recurse_remove (p->tn_low, string, old);
	else if ( c > p->tn_split )
		p->tn_hi = tst_recurse_remove (p->tn_hi, string, old);
	else if (*++string)
		p->tn_eq = tst_recurse_remove (p->tn_eq, string, old);
	else {
		*old = p->tn_data;
		p->tn_data = NULL;
	}

	/* now remove this node if its empty */
	if ((p->tn_low || p->tn_hi || p->tn_eq || p->tn_data) == 0) 
		xfree (&p);

	return p;
}

/* remove a node from the tree */
void *
tst_remove (struct tst_head *th, const char *str)
{
	void *old = NULL;

	assert(th);
	assert(str);

	TST_BUCKET(th, str) = tst_recurse_remove (TST_BUCKET(th, str), str, &old);

	return old;
}
