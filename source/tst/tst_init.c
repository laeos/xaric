#ident "@(#)tst_init.c 1.2"
/*
 * tst_init.c - handle creating ternary search trees
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

void
tst_init(struct tst_head *th)
{
	assert(th);

	memset(th, 0, sizeof(struct tst_head));
}
