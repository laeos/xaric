#ifndef util_h__
#define util_h__

/*
 * util.h : various utility stuff for xaric
 * Copyright (c) 2000 Rex Feany (laeos@xaric.org)
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
 * @(#)$Id$
 *
 */

#include "tst.h"
#include "fset.h"

/* Some random IRC limits */
#define CHANNEL_MAX	200

#define OFF	0
#define ON	1
#define TOGGLE	2

#define on_string	(var_settings[ON])
#define off_string	(var_settings[OFF])
#define toggle_string	(var_settings[TOGGLE])

/* the strings ON, OFF, TOGGLE indexed by defines above */
extern const char *var_settings[];

/* just the number zero! */
extern char zero_string[];

/* just.. an empty string. */
extern char empty_string[];

/* and this contains a single space */
extern char space_string[];

/* Check if this is a valid nick */
int is_nick(const char *nick);

/* Check if this is a valid DCC nick (i.e. =nick) */
int is_dcc_nick(const char *nick);

/* Check if this is a valid channel name */
int is_channel(const char *chan);

/* Returns true if the given string is a number, false otherwise */
int is_number (const char *str);

/* Expand ~ in path names. taken from ircII. return is malloc'ed */
char * expand_twiddle (const char *str);

/* just a handy thing.  Returns 1 if the str is not ON, OFF, or TOGGLE. */
int do_boolean (const char *str, int *value);

/* convert a "long" to a string. XXX uses static space, so copy it if you want it. */
char *ltoa (long foo);

/* Convert string to all uppercase, in place. */
char *upper (char *str);

/* Convert string to all lowercase, in place. */
char *lower (char *str);

/* terminate string at first occurence of any of chars */
char * terminate (char *string, char *chars);

/* in bsearch.c: find elements in an array. will do partial matches 
 * cnt will be set to:
 *	0, element was not found
 *	n, number of matching elements
 *	-n, exact match, with n-1 inexact matches.
 *	loc: index into list, returns pointer to best element 
 */

void *bsearch_array (void *list, size_t size, int howmany, const char *name, int *cnt, int *loc);

/* in xscandir.c */
int xscandir(char *dir, char *prefix, char **ret);

/* in xdlist.c */
int display_list(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt);
int display_list_l(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt, int longest);
int display_list_c(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt, int count);
int display_list_cl(TST_ARRAY *list, const char * (*fcn)(void *), enum FSET_TYPES fmt, int count, int longest);



#endif /* util_h__ */
