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

typedef int (*alist_func) (const char *, const char *, size_t);

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
	alist_func func;
} array;

array_item *	add_to_array 		(array *, array_item *);
array_item *	remove_from_array 	(array *, const char *);
array_item *	array_lookup 		(array *, const char *, int, int);
array_item *	find_array_item 	(array *, const char *, int *, int *);
array_item *	array_pop		(array *, int);
void *		find_fixed_array_item 	(void *, size_t, int, const char *, 
					 int *, int *);

#endif
