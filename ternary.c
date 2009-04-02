#ident "@(#)ternary.c 1.5"
/*
 * ternary.c : ternary search tree, for commands.
 * (c) 1998 Rex Feany <laeos@ptw.com> 
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
 * TODO:
 *
 *      * make insert more robust. (i.e. no insert-over-already-there)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "tcommand.h"

struct tnode {
    char splitchar;
    struct tnode *low, *eq, *hi;
    struct command *data;
};

/* t_insert: insert a node into the tree */
struct tnode *t_insert(struct tnode *p, const char *string, struct command *data)
{
    if (p == NULL) {
	p = malloc(sizeof(struct tnode));
	p->splitchar = *string;
	p->low = p->eq = p->hi = NULL;
	p->data = NULL;
    }
    if (*string < p->splitchar)
	p->low = t_insert(p->low, string, data);
    else if (*string == p->splitchar) {
	if (*++string == 0)
	    p->data = data;
	else if (*string != 0)
	    p->eq = t_insert(p->eq, string, data);
    } else
	p->hi = t_insert(p->hi, string, data);
    return p;
}

/* t_remove: remove a node from the tree */
struct tnode *t_remove(struct tnode *p, char *s, struct command **data)
{
    if (!p) {
	(*data) = NULL;
	return NULL;
    }
    if (*s == p->splitchar) {
	if (*++s)
	    p->eq = t_remove(p->eq, s, data);
	else {
	    (*data) = p->data;
	    p->data = NULL;
	}
    } else if (*s > p->splitchar)
	p->hi = t_remove(p->hi, s, data);
    else
	p->low = t_remove(p->low, s, data);

    if ((p->low || p->hi || p->eq || p->data) == 0) {
	free(p);
	return NULL;
    } else
	return p;
}

static inline struct command *t_finish_search(struct tnode *p)
{
    int mask;

    /* well, end of string, and there is data. must be a full command. */
    if (p->data)
	return p->data;

    /* if not, we need to start our search on the NEXT child. remeber, *s == splitchar in t_search :) */

    p = p->eq;

    while (p) {
	/* kind of ugly, but we need to make sure only ONE is non-null. */

	mask = p->hi != NULL;
	mask |= (p->eq != NULL) << 1;
	mask |= (p->low != NULL) << 2;

	if (mask && p->data)
	    return C_AMBIG;

	switch (mask) {
	case 0x0:
	    return p->data;
	case 0x1:
	    p = p->hi;
	    break;
	case 0x2:
	    p = p->eq;
	    break;
	case 0x4:
	    p = p->low;
	    break;
	default:
	    return C_AMBIG;
	}
    }
    return C_AMBIG;		/* hmm.. never get here? */
}

/* t_search: given a tree, and a string, return the command struct, NULL, or C_AMBIG */
struct command *t_search(struct tnode *root, const char *s)
{
    struct tnode *p;

    p = root;
    while (p) {
	if (*s == p->splitchar) {
	    if (*++s == 0)
		return t_finish_search(p);
	    p = p->eq;
	} else if (*s < p->splitchar)
	    p = p->low;
	else
	    p = p->hi;
    }
    return NULL;
}

/* t_build: given an array, insert all the commands. Will produce better
   results if the array is sorted.      */
struct tnode *t_build(struct tnode *root, struct command c[], int n)
{
    int m;

    if (n < 1)
	return root;
    m = n / 2;
    root = t_insert(root, c[m].name, &c[m]);

    root = t_build(root, c, m);
    root = t_build(root, c + m + 1, n - m - 1);
    return root;
}

static void local_traverse(struct tnode *p, void (*fcn) (struct command *, char *), char *data)
{
    if (!p)
	return;
    if (p->data)
	fcn(p->data, data);
    local_traverse(p->low, fcn, data);
    local_traverse(p->eq, fcn, data);
    local_traverse(p->hi, fcn, data);
}

void t_traverse(struct tnode *p, char *s, void (*fcn) (struct command *, char *), char *data)
{
    while (p) {
	if (*s == p->splitchar) {
	    p = p->eq;
	    if (*++s == 0)
		break;
	} else if (*s < p->splitchar)
	    p = p->low;
	else
	    p = p->hi;
    }
    if (p)
	local_traverse(p, fcn, data);
}
