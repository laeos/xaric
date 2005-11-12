/*
 * hash.h: header file for hash.c
 *
 * Written by Scott H Kilau
 *
 * CopyRight(c) 1997
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * @(#)$Id$
 */

#ifndef HASH_H
#define HASH_H

#define NICKLIST_HASHSIZE 79
#define WHOWASLIST_HASHSIZE 271

#ifndef REMOVE_FROM_LIST
# define REMOVE_FROM_LIST 1
#endif

/* hashentry: structure for all hash lists we make. 
 * quite generic, but powerful. */
struct hash_entry {
    void *list;			/* our linked list, generic void * */
    int hits;			/* how many hits this spot has gotten */
    int links;			/* how many links we have at this spot */
};

#endif				/* HASH_H */
