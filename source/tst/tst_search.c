#ident "@(#)tst_search.c 1.2"
/*
 * tst_search.c - search routines for tst
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



/* get to a perticular node */
static struct tst_node *
tst_find_node (struct tst_head *th, const char *str)
{
	unsigned char c = tolower(*str);
	struct tst_node *p;

	assert(th);
	assert(str);
	assert(*str);

	p = TST_BUCKET(th, str);

	while (p) {
		if ( c < p->tn_split )
			p = p->tn_low;
		else if ( c > p->tn_split )
			p = p->tn_hi;
		else if (*++str == 0) 
			return p;
		else
			p = p->tn_eq;
	}
	return NULL;
}



/* Count the number of nodes below this one */
static int
tst_count (struct tst_node *p)
{
	int count = 0;

	if ( p->tn_hi )
		count += tst_count(p->tn_hi);
	if ( p->tn_eq )
		count += tst_count(p->tn_eq);
	if ( p->tn_low )
		count += tst_count(p->tn_low);
	
	count += (p->tn_data != NULL);

	return count;
}


static void
do_partial (struct tst_node *p, int *pos, int *len, struct tst_array **data)
{
	/* short circut */
	if ( *len <= 0 )
		return;
	
	if ( p->tn_hi )
		do_partial(p->tn_hi, pos, len, data);
	if ( p->tn_eq )
		do_partial(p->tn_eq, pos, len, data);
	if ( p->tn_low )
		do_partial(p->tn_low, pos, len, data);

	if ( p->tn_data ) {

		if ( *pos ) {
			(*pos)--;
			return;
		}


		(*data)->data = p->tn_data;
		(*data)++;

		(*len)--;
	}

}

/* Finish a partial search */
static struct tst_array *
finish_partial (struct tst_node *p, int pos, int len)
{
	size_t l = len ? len : tst_count(p);
	struct tst_array *data = xmalloc( (l+1) * sizeof(struct tst_array)  );
	struct tst_array *retval = data;

	len = l;
	do_partial(p, &pos, &len, &data);

	retval[l].data = NULL;
	return retval;
}


/* Return a list of partial matches */
struct tst_array *
tst_partial(struct tst_head *th, const char *str, int pos, int len)
{
	struct tst_node *p;

	if ( (p = tst_find_node(th, str)) ) {
		return finish_partial(p, pos, len);
	}
	return NULL;
}

/* Search for an entry in the tree */
void *
tst_find (struct tst_head *th, const char *str)
{
	struct tst_node *p;

	if ( (p = tst_find_node(th, str)) ) {
		return p->tn_data;
	}
	return NULL;
}
