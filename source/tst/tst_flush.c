#ident "@(#)tst_flush.c 1.2"
/*
 * tst_flush.c - remove all nodes in this tst.
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

#include "tst.h"
#include "xmalloc.h"


/* depth-first, free off the tree. */
static int
do_flush(struct tst_node *p, void (*freefunc)(void *data))
{
	int count = 1;

	if ( p->tn_hi )
		count += do_flush(p->tn_hi, freefunc);
	if ( p->tn_eq )
		count += do_flush(p->tn_eq, freefunc);
	if ( p->tn_low )
		count += do_flush(p->tn_low, freefunc);
	if ( p->tn_data )
		freefunc(p->tn_data);

	xfree(&p);

	return count;
}


/* Flush all of the nodes out of a tree. returns number of nodes removed. */
int 
tst_flush(TST_HEAD *tst, void (*freefunc)(void *data))
{
	int i = 0;
	int cnt = 0;

	assert(tst);
	assert(freefunc);


	for ( ; i < TST_NBUCKETS; i++ ) {
		if ( tst->th_head[i] )
			cnt += do_flush(tst->th_head[i], freefunc);
		tst->th_head[i] = NULL;
	}
	return cnt;
}

