#ifndef tst_h__
#define tst_h__
/*
 * tst.h - ternary search trees
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
 * @(#)tst.h 1.1
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

/* number of buckets to use. Please, be a power of two! */
#define TST_NBUCKETS	32

/* Given a string, choose the right bucket */
#define TST_INDEX(x)   ((unsigned int)(tolower(x)&(TST_NBUCKETS-1)))

#define TST_BUCKET(th, str)	((th)->th_head[TST_INDEX(*(str))])

/* each node of a tst */
struct tst_node {
	unsigned char	   tn_split;
	struct tst_node	   *tn_low;
	struct tst_node	   *tn_eq;
	struct tst_node	   *tn_hi;
	void		   *tn_data;
};

/* The head of a ternary search tree */
struct tst_head {
	struct tst_node	   *th_head[TST_NBUCKETS-1];
};

/* Used for the return array of tst_partial */
struct tst_array {
	void *data;
};

/* What the outside uses */
typedef struct tst_head TST_HEAD;
typedef struct tst_array TST_ARRAY;

/* Most of these routines are recursive. */

/* initilize a new tree (tst_init.c) */
void tst_init (TST_HEAD *tst);

/* Insert a node, fail if one with the same key exists */
int tst_insert(TST_HEAD *tst, const char *key, void *tst_node);

/* Insert a node, replace if one exists */
void * tst_replace(TST_HEAD *tst, const char *key, void *data);

/* Remove a node (tst_remove.c) */
void * tst_remove (TST_HEAD *tst, const char *key);

/* Find a specific node */
void * tst_find(TST_HEAD *tst, const char *key);

/* Partial search. Return a list of things found.
 * pos == where to start the list, len == how many to return. */
TST_ARRAY * tst_partial(TST_HEAD *tst, const char *key, int pos, int len);

/* Build a tre from a table. noffset is the offset to the key. */
void tst_build(TST_HEAD *tst, void *data, int elen, int noffset, int nelt);

/* Flush all of the nodes out of a tree. returns number of nodes removed. */
int tst_flush(TST_HEAD *tst, void (*freefunc)(void *data));

#endif /* tst_h__ */

