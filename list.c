#ident "@(#)list.c 1.5"
/*
 * list.c: some generic linked list managing stuff 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "list.h"
#include "ircaux.h"

static int add_list_stricmp(struct list *, struct list *);
static int list_stricmp(struct list *, char *);
static int list_match(struct list *, char *);

/*
 * These have now been made more general. You used to only be able to
 * order these lists by alphabetical order. You can now order them
 * arbitrarily. The functions are still called the same way if you
 * wish to use alphabetical order on the key string, and the old
 * function name now represents a stub function which calls the
 * new with the appropriate parameters.
 *
 * The new function name is the same in each case as the old function
 * name, with the addition of a new parameter, cmp_func, which is
 * used to perform comparisons.
 *
 */

static int add_list_stricmp(struct list *item1, struct list *item2)
{
    return strcasecmp(item1->name, item2->name);
}

static int list_stricmp(struct list *item1, char *str)
{
    return my_stricmp(item1->name, str);
}

int list_strnicmp(struct list *item1, char *str)
{
    return my_strnicmp(item1->name, str, strlen(str));
}

static int list_match(struct list *item1, char *str)
{
    return wild_match(item1->name, str);
}

/*
 * add_to_list: This will add an element to a list.  The requirements for the
 * list are that the first element in each list structure be a pointer to the
 * next element in the list, and the second element in the list structure be
 * a pointer to a character (char *) which represents the sort key.  For
 * example 
 *
 * struct my_list{ struct my_list *next; char *name; <whatever else you want>}; 
 *
 * The parameters are:  "list" which is a pointer to the head of the list. "add"
 * which is a pre-allocated element to be added to the list.  
 */
void add_to_list_ext(struct list **list, struct list *add, cmp_fn * cmp_func)
{
    struct list *tmp;
    struct list *last;

    if (!cmp_func)
	cmp_func = (cmp_fn *) add_list_stricmp;
    last = NULL;
    for (tmp = *list; tmp; tmp = tmp->next) {
	if (cmp_func(tmp, (void *) add) > 0)
	    break;
	last = tmp;
    }
    if (last)
	last->next = add;
    else
	*list = add;
    add->next = tmp;
}

void add_to_list(struct list **list, struct list *add)
{
    add_to_list_ext(list, add, NULL);
}

/*
 * find_in_list: This looks up the given name in the given list.  struct list and
 * name are as described above.  If wild is true, each name in the list is
 * used as a wild card expression to match name... otherwise, normal matching
 * is done 
 */
struct list *find_in_list_ext(struct list **list, char *name, int wild, cmp_fn * cmp_func)
{
    struct list *tmp;
    int best_match, current_match;

    if (!cmp_func)
	cmp_func = wild ? list_match : list_stricmp;
    best_match = 0;

    if (wild) {
	struct list *match = NULL;

	for (tmp = *list; tmp; tmp = tmp->next) {
	    if ((current_match = cmp_func(tmp, name)) > best_match) {
		match = tmp;
		best_match = current_match;
	    }
	}
	return (match);
    } else {
	for (tmp = *list; tmp; tmp = tmp->next)
	    if (cmp_func(tmp, name) == 0)
		return (tmp);
    }
    return NULL;
}

struct list *find_in_list(struct list **list, char *name, int wild)
{
    return find_in_list_ext(list, name, wild, NULL);
}

/*
 * remove_from_list: this remove the given name from the given list (again as
 * described above).  If found, it is removed from the list and returned
 * (memory is not deallocated).  If not found, null is returned. 
 */
struct list *remove_from_list_ext(struct list **list, char *name, cmp_fn * cmp_func)
{
    struct list *tmp;
    struct list *last;

    if (!cmp_func)
	cmp_func = list_stricmp;
    last = NULL;
    for (tmp = *list; tmp; tmp = tmp->next) {
	if (cmp_func(tmp, name) == 0) {
	    if (last)
		last->next = tmp->next;
	    else
		*list = tmp->next;
	    return (tmp);
	}
	last = tmp;
    }
    return NULL;
}

struct list *remove_from_list(struct list **list, char *name)
{
    return remove_from_list_ext(list, name, NULL);
}

/*
 * list_lookup: this routine just consolidates remove_from_list and
 * find_in_list.  I did this cause it fit better with some already existing
 * code 
 */
struct list *list_lookup_ext(struct list **list, char *name, int wild, int delete, cmp_fn * cmp_func)
{
    struct list *tmp;

    if (delete)
	tmp = remove_from_list_ext(list, name, cmp_func);
    else
	tmp = find_in_list_ext(list, name, wild, cmp_func);
    return (tmp);
}

struct list *list_lookup(struct list **list, char *name, int wild, int delete)
{
    return list_lookup_ext(list, name, wild, delete, NULL);
}
