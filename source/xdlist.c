#ident "@(#)xdlist.c 1.2"
/*
 * xdlist.c - display a list of things
 * Copyright (C) 1999, 2000 Rex Feany <laeos@laeos.net>
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
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "xmalloc.h"	/* for alloca */

#include "ircterm.h"
#include "tst.h"
#include "fset.h"
#include "output.h"
#include "util.h"

/* --- all for misc.h */
#include "irc.h"
#include "ircaux.h"
#include "struct.h"
#include "misc.h" /* for convert_output_format only.. FIX THIS POS */


/* find the longest element */
static int
find_longest (TST_ARRAY *list, const char * (*fcn)(void *), int cnt)
{
	int lng = 0;

	/* never call me with a NULL list please */
	assert(list);
	assert(cnt);
	assert(fcn);

	do {
		int t = strlen(fcn(list->data));
		if ( t > lng ) 
			lng = t;

	} while (--cnt);

	return lng;
}

/* count the number of elements, and find the longest one. we expect a NULL
 * somewhere at the end of the list */
static int
find_count (TST_ARRAY *list, const char * (*fcn)(void *))
{
	int cnt = 0;

	/* never call me with a NULL list please */
	assert(list);
	assert(list->data);
	assert(fcn);

	do {
		cnt++;
	} while(++list && list->data);

	return cnt;
}

/* Pretty-print a "list" or array of things */
int 
display_list_cl(TST_ARRAY *list, const char * (*fcn)(void *), 
			enum FSET_TYPES fmt, int count, int longest)
{
	int cols = CO - 10;	/* extra space for the begin/end of the line */
	int epr;
	char *buf, *sv;
	int cnt = count;
	char *fmat = get_fset_var(fmt);

#ifndef NDEBUG	/* some extra stuff if asserts are defined */
	char *en;
#endif

	assert(list);
	assert(fcn);
	assert(count);
	assert(longest);
	assert(fmat);

	/* the length for each entry should count the space between each entry */
	longest += 1;

	/* We can't (and shouldn't be) printing entries that are longer then
	 * the screen length.  this is for printing small lists! */
	assert(longest <= cols);

	epr = cols/longest;

	/* allocate the buffer we use to construct each line */
	sv = buf = alloca(epr*longest+1);

#ifndef NDEBUG
	en = buf + epr*longest+1;
#endif

	do {
		int i;
		int len = 0;

		for(i = epr; i && cnt; i--, cnt--) {
			len = strlen(strcpy(buf, fcn(list->data)));

			buf += len;
			++list;
			while(len++<longest) 
				*buf++ = ' ';
			assert(buf < en);
		}
		*buf = '\0';	
		put_it("%s", convert_output_format(fmat, "%s", sv));
		sv[0] = '\0';
		buf = sv;

	} while (cnt);

	return count;
}



/* Various combos depending on what data we have */
int
display_list(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt)
{
	int count = find_count(list, fcn);
	int longest = find_longest(list, fcn, count);

	return display_list_cl(list, fcn, fmt, count, longest);
}

int
display_list_c(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt, int count)
{
	int longest = find_longest(list, fcn, count);

	return display_list_cl(list, fcn, fmt, count, longest);
}

int
display_list_l(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt, int longest)
{
	int count = find_count(list, fcn);

	return display_list_cl(list, fcn, fmt, count, longest);
}

