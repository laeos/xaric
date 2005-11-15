#ident "$Id$"
/*
 * alist.c -- resizeable arrays.
 * Written by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs
 *
 * This file presumes a good deal of chicanery.  Specifically, it assumes
 * that your compiler will allocate disparate structures congruently as 
 * long as the members match as to their type and location.  This is
 * critically important for how this code works, and all hell will break
 * loose if your compiler doesnt do this.  Every compiler i know of does
 * it, which is why im assuming it, even though im not allowed to assume it.
 *
 * This file is hideous.  Ill kill each and every one of you who made
 * me do this. ;-)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "alist.h"
#include "ircaux.h"
#include "output.h"

#define ARRAY_ITEM(array, loc) ((array_item *) ((array) -> list [ (loc) ]))
#define LARRAY_ITEM(array, loc) (((array) -> list [ (loc) ]))

/*
 * This is just a generalization of the old function  ``find_command''
 *
 * You give it an alist_item and a name, and it gives you the number of
 * items that are completed by that name, and where you could find/put that
 * item in the list.  It returns the alist_item most appropriate.
 *
 * If ``cnt'' is less than -1, then there was one exact match and one or
 *      more ambiguous matches in addition.  The exact match's location 
 *      is put into ``loc'' and its entry is returned.  The ambigous matches
 *      are (of course) immediately subsequent to it.
 *
 * If ``cnt'' is -1, then there was one exact match.  Its location is
 *      put into ``loc'' and its entry is returned.
 *
 * If ``cnt'' is zero, then there were no matches for ``name'', but ``loc''
 *      is set to the location in the array in which you could place the
 *      specified name in the sorted list.
 *
 * If ``cnt'' is one, then there was one command that non-ambiguously 
 *      completes ``name''
 *
 * If ``cnt'' is greater than one, then there was exactly ``cnt'' number
 *      of entries that completed ``name'', but they are all ambiguous.
 *      The entry that is lowest alphabetically is returned, and its
 *      location is put into ``loc''.
 */

#define FIXED_ITEM(list, pos, size) (*(array_item *) ((char *)list + ( pos * size )))
 /* 
  * This is useful for finding items in a fixed array (eg, those lists that
  * are a simple fixed length arrays of 1st level structs.)
  *
  * This code is identical to find_array_item except ``list'' is a 1st
  * level array instead of a 2nd level array.
  */
void *find_fixed_array_item(void *list, size_t size, int howmany, const char *name, int *cnt, int *loc)
{
    int len = strlen(name), min = 0, max = howmany, old_pos = -1, pos, c;

    *cnt = 0;

    while (1) {
	pos = (max + min) / 2;
	if (pos == old_pos) {
	    *loc = pos;
	    return NULL;
	}
	old_pos = pos;

	c = strncasecmp(name, FIXED_ITEM(list, pos, size).name, len);
	if (c == 0)
	    break;
	else if (c > 0)
	    min = pos;
	else
	    max = pos;
    }

    /* 
     * If we've gotten this far, then we've found at least
     * one appropriate entry.  So we set *cnt to one
     */
    *cnt = 1;

    /* 
     * So we know that 'pos' is a match.  So we start 'min'
     * at one less than 'pos' and walk downard until we find
     * an entry that is NOT matched.
     */
    min = pos - 1;
    while (min >= 0 && !strncasecmp(name, FIXED_ITEM(list, min, size).name, len))
	(*cnt)++, min--;
    min++;

    /* 
     * Repeat the same ordeal, except this time we walk upwards
     * from 'pos' until we dont find a match.
     */
    max = pos + 1;
    while ((max < howmany) && !strncasecmp(name, FIXED_ITEM(list, max, size).name, len))
	(*cnt)++, max++;

    /* 
     * Alphabetically, the string that would be identical to
     * 'name' would be the first item in a string of items that
     * all began with 'name'.  That is to say, if there is an
     * exact match, its sitting under item 'min'.  So we check
     * for that and whack the count appropriately.
     */
    if (strlen(FIXED_ITEM(list, min, size).name) == len)
	*cnt *= -1;

    /* 
     * Then we tell the caller where the lowest match is,
     * in case they want to insert here.
     */
    if (loc)
	*loc = min;

    /* 
     * Then we return the first item that matches.
     */
    return (void *) &FIXED_ITEM(list, min, size);
}
