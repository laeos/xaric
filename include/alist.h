/*
 * alist.h -- resizeable arrays
 * Copyright 1997 EPIC Software Labs
 */

#ifndef __alist_h__
#define __alist_h__

#include "irc.h"

/*
 * Everything that is to be filed with this system should have an
 * identifying name as the first item in the struct.
 */
typedef struct
{
	char *name;
} array_item;

/*
 * This is the actual list, that contains structs that are of the
 * form described above.  It contains the current size and the maximum
 * size of the array.
 */
typedef struct
{
	array_item **list;
	int max;
	int total_max;
} array;

array_item *add_to_array (array *, array_item *);
array_item *remove_from_array (array *, char *);
array_item *array_lookup (array *, char *, int wild, int delete);
array_item *find_array_item (array *, char *, int *cnt, int *loc);

void *find_fixed_array_item (void *array, size_t size, int siz, char *, int *cnt, int *loc);

#endif
