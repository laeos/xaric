#ident "@(#)tst_insert.c 1.2"
/*
 * tst_insert.c - insert a node into a ternary search tree.
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



/* Recursivly insert a node into the tree. ugh. should get rid of recursion. */
static struct tst_node *
tst_recurse_insert (struct tst_node *p, const char *string, void *data, int *retval)
{
	unsigned char c = tolower(*string);

	if (p == NULL) {
		p = xmalloc (sizeof (struct tst_node));
		p->tn_split = c;
		p->tn_low = p->tn_eq = p->tn_hi = NULL;
		memset(p->tn_data, 0, sizeof(p->tn_data));
	}
	if ( c < p->tn_split )
		p->tn_low = tst_recurse_insert (p->tn_low, string, data, retval);
	else if ( c > p->tn_split )
		p->tn_hi = tst_recurse_insert (p->tn_hi, string, data, retval);
	else if (*++string == 0) {
		if ( p->tn_data )
			*retval = 1;
		else 
			p->tn_data = data;

	} else 
			p->tn_eq = tst_recurse_insert (p->tn_eq, string, data, retval);
	return p;
}


int
tst_insert (struct tst_head *th, const char *str, void *data)
{
	int retval = 0;

	assert(th);
	assert(str);
	assert(*str);
	assert(data);

	TST_BUCKET(th, str) = tst_recurse_insert(TST_BUCKET(th, str), str, data, &retval);

	return retval;
}

void *
tst_replace (struct tst_head *th, const char *str, void *data)
{
	int retval;
	void *old;

	assert(th);
	assert(str);
	assert(*str);
	assert(data);

	old = tst_remove(th, str);

	TST_BUCKET(th, str) = tst_recurse_insert(TST_BUCKET(th, str), str, data, &retval);

	return old;
}

