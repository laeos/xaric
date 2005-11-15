#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct ll_head {
    struct ll_head *next, *prev;
};

#define LL_HEAD_INIT(name) { &(name), &(name) }

#define LL_HEAD(name) \
	struct ll_head name = LL_HEAD_INIT(name)

#define INIT_LL_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __ll_add(struct ll_head *new, struct ll_head *prev, struct ll_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/**
 * ll_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void ll_add(struct ll_head *new, struct ll_head *head)
{
    __ll_add(new, head, head->next);
}

/**
 * ll_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void ll_add_tail(struct ll_head *new, struct ll_head *head)
{
    __ll_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __ll_del(struct ll_head *prev, struct ll_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * ll_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: ll_empty on entry does not return true after this, the entry is in an undefined state.
 */
static __inline__ void ll_del(struct ll_head *entry)
{
    __ll_del(entry->prev, entry->next);
}

/**
 * ll_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static __inline__ void ll_del_init(struct ll_head *entry)
{
    __ll_del(entry->prev, entry->next);
    INIT_LL_HEAD(entry);
}

/**
 * ll_empty - tests whether a list is empty
 * @head: the list to test.
 */
static __inline__ int ll_empty(struct ll_head *head)
{
    return head->next == head;
}

/**
 * ll_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static __inline__ void ll_splice(struct ll_head *list, struct ll_head *head)
{
    struct ll_head *first = list->next;

    if (first != list) {
	struct ll_head *last = list->prev;
	struct ll_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
    }
}

/**
 * ll_tail - is this the tail entry?
 * @head: head of list
 * @ent: entry to test
 **/
#define ll_tail(head,ent) ((ent)->next == (head))

/**
 * ll_head - is this the head entry?
 * @head: head of list
 * @ent: entry to test
 **/
#define ll_head(head,ent) ((ent)->prev == (head))

/**
 * ll_entry - get the struct for this entry
 * @ptr:	the &struct ll_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the ll_struct within the struct.
 */
#define ll_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * ll_for_each	-	iterate over a list
 * @pos:	the &struct ll_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define ll_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * ll_for_each_backwards - iterate over a list, backwards.
 * @pos:	the &struct ll_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define ll_for_each_backwards(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#endif				/* _LINUX_LIST_H */
