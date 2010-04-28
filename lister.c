/*
 * lister.c - display a list of things
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

#include "irc.h"
#include "ircaux.h"
#include "ircterm.h"
#include "output.h"
#include "fset.h"
#include "lister.h"

#define EXTRA_END_SPACE	2	/* space to leave empty at the end of the line */

/* find the longest element */
static int find_longest(ARRAY * list, const char *(*fcn) (void *), int cnt)
{
    const char *data;
    int max;
    int len;

    /* never call me with a NULL list please */
    assert(list);
    assert(cnt);
    assert(fcn);

    max = 0;
    do {
	data = fcn(list->data);

	len = strlen(data) - count_ansi(data, 0);

	if (len > max)
	    max = len;

	list++;
    } while (--cnt);

    return max;
}

/* count the number of elements, and find the longest one. we expect a NULL
 * somewhere at the end of the list */
static int find_count(ARRAY * list, const char *(*fcn) (void *))
{
    int cnt;

    /* never call me with a NULL list please */
    assert(list);
    assert(list->data);
    assert(fcn);

    cnt = 0;
    do {
	cnt++;
    } while (++list && list->data);

    return cnt;
}

/**
 * display_list_cl - format and display an array of things
 * @list: array of things to display
 * @fcn: function to retrive data to display
 * @fmt: line format (not per entry)
 * @count: number of items in array.
 * @longest: longest element in array.
 *
 **/
int display_list_cl(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int count, int longest)
{
    int cols;
    int epl;			/* entrys per line */
    int cnt;
    char *buf, *save_buf;

#ifndef NDEBUG
    char *buf_end;
#endif

    assert(list);
    assert(fcn);
    assert(count);
    assert(longest);

    /* the length for each entry should count the space between each entry */
    longest += 1;

    /* the avaliable length in the column */
    cols = term_cols - get_format_len(fmt) - EXTRA_END_SPACE;

    /* We can't (and shouldn't be) printing entries that are longer then the screen length.  this is for printing small lists! */
    assert(longest <= cols);

    /* entries per line. */
    epl = cols / longest;

    /* allocate the buffer we use to construct each line */
    save_buf = buf = malloc(epl * longest + 1);

#ifndef NDEBUG
    buf_end = buf + (epl * longest + 1) + 1;
#endif

    cnt = count;
    do {
	int i;
	int real_len;
	int print_len;

	save_buf[0] = '\0';
	buf = save_buf;

	for (i = epl; i && cnt; i--, cnt--) {
	    strcpy(buf, fcn(list->data));

	    real_len = strlen(buf);
	    print_len = real_len - count_ansi(buf, 0);

	    buf += real_len;

	    /* fill in between entries */
	    while (print_len++ < longest)
		*buf++ = ' ';

	    assert(buf < buf_end);
	    ++list;
	}
	*buf = '\0';
	put_fmt(fmt, "%s", save_buf);

    } while (cnt);

    free(save_buf);

    return count;
}

/**
 * display_list - format and display a null-terminated list 
 * @list: array of things to display
 * @fcn: function to retrive data to display
 * @fmt: line format (not per entry)
 *
 **/
int display_list(ARRAY * list, const char *(*fcn) (void *), xformat fmt)
{
    int count = find_count(list, fcn);
    int longest = find_longest(list, fcn, count);

    return display_list_cl(list, fcn, fmt, count, longest);
}

/**
 * display_list_c - format and display list of count items
 * @list: array of things to display.
 * @fcn: function to retrive data to display.
 * @fmt: line format.
 * @count: number of things in array.
 *
 **/
int display_list_c(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int count)
{
    int longest = find_longest(list, fcn, count);

    return display_list_cl(list, fcn, fmt, count, longest);
}

/**
 * display_list_l - format and display list of things
 * @list: array of things to display.
 * @fcn: function to retrive data to display.
 * @fmt: line format.
 * @longest: longest thing in list.
 *
 **/
int display_list_l(ARRAY * list, const char *(*fcn) (void *), xformat fmt, int longest)
{
    int count = find_count(list, fcn);

    return display_list_cl(list, fcn, fmt, count, longest);
}
