#ifndef _LINUX_LLIST_H
#define _LINUX_LLIST_H

/*
 * Simple doubly linked llist implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole llists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct llist_head {
	struct llist_head *next, *prev;
};

#define LLIST_HEAD_INIT(name) { &(name), &(name) }

#define LLIST_HEAD(name) \
	struct llist_head name = LLIST_HEAD_INIT(name)

#define INIT_LLIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal llist manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __llist_add(struct llist_head * new,
	struct llist_head * prev,
	struct llist_head * next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * llist_add - add a new entry
 * @new: new entry to be added
 * @head: llist head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static __inline__ void llist_add(struct llist_head *new, struct llist_head *head)
{
	__llist_add(new, head, head->next);
}

/**
 * llist_add_tail - add a new entry
 * @new: new entry to be added
 * @head: llist head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static __inline__ void llist_add_tail(struct llist_head *new, struct llist_head *head)
{
	__llist_add(new, head->prev, head);
}

/*
 * Delete a llist entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal llist manipulation where we know
 * the prev/next entries already!
 */
static __inline__ void __llist_del(struct llist_head * prev,
				  struct llist_head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * llist_del - deletes entry from llist.
 * @entry: the element to delete from the llist.
 * Note: llist_empty on entry does not return true after this, the entry is in an undefined state.
 */
static __inline__ void llist_del(struct llist_head *entry)
{
	__llist_del(entry->prev, entry->next);
}

/**
 * llist_del_init - deletes entry from llist and reinitialize it.
 * @entry: the element to delete from the llist.
 */
static __inline__ void llist_del_init(struct llist_head *entry)
{
	__llist_del(entry->prev, entry->next);
	INIT_LLIST_HEAD(entry); 
}

/**
 * llist_empty - tests whether a llist is empty
 * @head: the llist to test.
 */
static __inline__ int llist_empty(struct llist_head *head)
{
	return head->next == head;
}

/**
 * llist_splice - join two llists
 * @llist: the new llist to add.
 * @head: the place to add it in the first llist.
 */
static __inline__ void llist_splice(struct llist_head *llist, struct llist_head *head)
{
	struct llist_head *first = llist->next;

	if (first != llist) {
		struct llist_head *last = llist->prev;
		struct llist_head *at = head->next;

		first->prev = head;
		head->next = first;

		last->next = at;
		at->prev = last;
	}
}

/**
 * llist_entry - get the struct for this entry
 * @ptr:	the &struct llist_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the llist_struct within the struct.
 */
#define llist_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * llist_for_each	-	iterate over a llist
 * @pos:	the &struct llist_head to use as a loop counter.
 * @head:	the head for your llist.
 */
#define llist_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#endif /* _LINUX_LLIST_H */
