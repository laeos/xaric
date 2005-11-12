/*
 * list.h: header for list.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef LIST_H
#define LIST_H

#define REMOVE_FROM_LIST 1
#define USE_WILDCARDS 1

struct list {
    struct list *next;
    char *name;
};

/* TODO: last arg should be void, not char */
typedef int (cmp_fn) (struct list *, char *);

/* list.c */
int list_strnicmp(struct list *item1, char *str);
void add_to_list_ext(struct list **list, struct list *add, cmp_fn *cmp_func);
void add_to_list(struct list **list, struct list *add);
struct list *find_in_list_ext(struct list **list, char *name, int wild, cmp_fn *cmp_func);
struct list *find_in_list(struct list **list, char *name, int wild);
struct list *remove_from_list_ext(struct list **list, char *name, cmp_fn *cmp_func);
struct list *remove_from_list(struct list **list, char *name);
struct list *list_lookup_ext(struct list **list, char *name, int wild, int delete, cmp_fn *cmp_func);
struct list *list_lookup(struct list **list, char *name, int wild, int delete);

#endif /* LIST_H */
