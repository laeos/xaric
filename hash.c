/************
 *  hash.c  *
 ************
 *
 * My hash routines for hashing NickLists, and eventually struct channel's
 * and struct whowas_list's
 *
 * These are not very robust, as the add/remove functions will have
 * to be written differently for each type of struct
 * (To DO: use C++, and create a hash "class" so I don't need to
 * have the functions different.)
 *
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 1997
 *
 * Modified by Colten Edwards for use in BitchX.
 * Added Whowas buffer hashing.
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "hook.h"
#include "vars.h"
#include "output.h"
#include "misc.h"
#include "server.h"
#include "list.h"
#include "window.h"
#include "fset.h"
#include "hash.h"
#include "tcommand.h"
#include "struct.h"
#include "whowas.h"

/*
 * hash_nickname: for now, does a simple hash of the 
 * nick by counting up the ascii values of the lower case, and 
 * then %'ing it by NICKLIST_HASHSIZE (always a prime!)
 */
static unsigned long hash_nickname(char *nick, unsigned int size)
{
    register u_char *p = (u_char *) nick;
    unsigned long hash = 0, g;

    while (*p) {
	hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p);
	if ((g = hash & 0xF0000000))
	    hash ^= g >> 24;
	hash &= ~g;
	p++;
    }
    return (hash %= size);
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_link_to_top(struct nick_list *tmp, struct nick_list *prev, struct hash_entry *location)
{
    if (prev) {
	struct nick_list *old_list;

	old_list = (struct nick_list *) location->list;
	location->list = (void *) tmp;
	prev->next = tmp->next;
	tmp->next = old_list;
    }
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_link_from_list(struct nick_list *tmp, struct nick_list *prev, struct hash_entry *location)
{
    if (prev) {
	/* remove the link from the middle of the list */
	prev->next = tmp->next;
    } else {
	/* unlink the first link, and connect next one up */
	location->list = (void *) tmp->next;
    }
    /* set tmp's next to NULL, as its unlinked now */
    tmp->next = NULL;
}

void add_name_to_genericlist(char *name, struct hash_entry *list, unsigned int size)
{
    struct list *nptr;
    unsigned long hvalue = hash_nickname(name, size);

    nptr = new_malloc(sizeof(struct list));
    nptr->next = (struct list *) list[hvalue].list;
    nptr->name = m_strdup(name);

    /* assign our new linked list into array spot */
    list[hvalue].list = (void *) nptr;
    /* quick tally of nicks in chain in this array spot */
    list[hvalue].links++;
    /* keep stats on hits to this array spot */
    list[hvalue].hits++;
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_gen_link_to_top(struct list *tmp, struct list *prev, struct hash_entry *location)
{
    if (prev) {
	struct list *old_list;

	old_list = (struct list *) location->list;
	location->list = (void *) tmp;
	prev->next = tmp->next;
	tmp->next = old_list;
    }
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_gen_link_from_list(struct list *tmp, struct list *prev, struct hash_entry *location)
{
    if (prev) {
	/* remove the link from the middle of the list */
	prev->next = tmp->next;
    } else {
	/* unlink the first link, and connect next one up */
	location->list = (void *) tmp->next;
    }
    /* set tmp's next to NULL, as its unlinked now */
    tmp->next = NULL;
}

struct list *find_name_in_genericlist(char *name, struct hash_entry *list, unsigned int size, int remove)
{
    struct hash_entry *location;
    register struct list *tmp, *prev = NULL;
    unsigned long hvalue = hash_nickname(name, size);

    location = &(list[hvalue]);

    /* at this point, we found the array spot, now search as regular linked list, or as ircd likes to say... "We found the bucket, now 
     * search the chain" */
    for (tmp = (struct list *) location->list; tmp; prev = tmp, tmp = tmp->next) {
	if (!my_stricmp(name, tmp->name)) {
	    if (remove != REMOVE_FROM_LIST)
		move_gen_link_to_top(tmp, prev, location);
	    else {
		location->links--;
		remove_gen_link_from_list(tmp, prev, location);
	    }
	    return tmp;
	}
    }
    return NULL;
}

/*
 * add_nicklist_to_channellist: This function will add the nicklist
 * into the channellist, ensuring that we hash the nicklist, and
 * insert the struct correctly into the channelist's Nicklist hash
 * array
 */
void add_nicklist_to_channellist(struct nick_list *nptr, struct channel *cptr)
{
    unsigned long hvalue = hash_nickname(nptr->nick, NICKLIST_HASHSIZE);

    /* take this nicklist, and attach it as the HEAD pointer in our chain at the hashed location in our array... Note, by doing this,
     * this ensures that the "most active" users always remain at the top of the chain... ie, faster lookups for active users, (and as 
     * a side note, makes doing the add quite simple!) */
    nptr->next = (struct nick_list *) cptr->NickListTable[hvalue].list;

    /* assign our new linked list into array spot */
    cptr->NickListTable[hvalue].list = (void *) nptr;
    /* quick tally of nicks in chain in this array spot */
    cptr->NickListTable[hvalue].links++;
    /* keep stats on hits to this array spot */
    cptr->NickListTable[hvalue].hits++;
}

struct nick_list *find_nicklist_in_channellist(char *nick, struct channel *cptr, int remove)
{
    struct hash_entry *location;
    register struct nick_list *tmp, *prev = NULL;
    unsigned long hvalue = hash_nickname(nick, NICKLIST_HASHSIZE);

    location = &(cptr->NickListTable[hvalue]);

    /* at this point, we found the array spot, now search as regular linked list, or as ircd likes to say... "We found the bucket, now 
     * search the chain" */
    for (tmp = (struct nick_list *) location->list; tmp; prev = tmp, tmp = tmp->next) {
	if (!my_stricmp(nick, tmp->nick)) {
	    if (remove != REMOVE_FROM_LIST)
		move_link_to_top(tmp, prev, location);
	    else {
		location->links--;
		remove_link_from_list(tmp, prev, location);
	    }
	    return tmp;
	}
    }
    return NULL;
}

/*
 * Basically this makes the hash table "look" like a straight linked list
 * This should be used for things that require you to cycle through the
 * full list, ex. for finding ALL matching stuff.
 * : usage should be like :
 *
 *      for (nptr = next_nicklist(cptr, NULL); nptr; nptr = 
 *           next_nicklist(cptr, nptr))
 *              YourCodeOnTheNickListStruct
 */
struct nick_list *next_nicklist(struct channel *cptr, struct nick_list *nptr)
{
    unsigned long hvalue = 0;

    if (!cptr)
	/* No channel! */
	return NULL;
    else if (!nptr) {
	/* wants to start the walk! */
	while ((struct nick_list *) cptr->NickListTable[hvalue].list == NULL) {
	    hvalue++;
	    if (hvalue >= NICKLIST_HASHSIZE)
		return NULL;
	}
	return (struct nick_list *) cptr->NickListTable[hvalue].list;
    } else if (nptr->next) {
	/* still returning a chain! */
	return nptr->next;
    } else if (!nptr->next) {
	int hvalue;

	/* hit end of chain, go to next bucket */
	hvalue = hash_nickname(nptr->nick, NICKLIST_HASHSIZE) + 1;
	if (hvalue >= NICKLIST_HASHSIZE) {
	    /* end of list */
	    return NULL;
	} else {
	    while ((struct nick_list *) cptr->NickListTable[hvalue].list == NULL) {
		hvalue++;
		if (hvalue >= NICKLIST_HASHSIZE)
		    return NULL;
	    }
	    /* return head of next filled bucket */
	    return (struct nick_list *) cptr->NickListTable[hvalue].list;
	}
    } else
	/* shouldn't ever be here */
	say("HASH_ERROR: next_nicklist");
    return NULL;
}

struct list *next_namelist(struct hash_entry *cptr, struct list *nptr, unsigned int size)
{
    unsigned long hvalue = 0;

    if (!cptr)
	/* No channel! */
	return NULL;
    else if (!nptr) {
	/* wants to start the walk! */
	while ((struct list *) cptr[hvalue].list == NULL) {
	    hvalue++;
	    if (hvalue >= size)
		return NULL;
	}
	return (struct list *) cptr[hvalue].list;
    } else if (nptr->next) {
	/* still returning a chain! */
	return nptr->next;
    } else if (!nptr->next) {
	int hvalue;

	/* hit end of chain, go to next bucket */
	hvalue = hash_nickname(nptr->name, size) + 1;
	if (hvalue >= size) {
	    /* end of list */
	    return NULL;
	} else {
	    while ((struct list *) cptr[hvalue].list == NULL) {
		hvalue++;
		if (hvalue >= size)
		    return NULL;
	    }
	    /* return head of next filled bucket */
	    return (struct list *) cptr[hvalue].list;
	}
    } else
	/* shouldn't ever be here */
	say("HASH_ERROR: next_namelist");
    return NULL;
}

void clear_nicklist_hashtable(struct channel *cptr)
{
    if (cptr) {
	memset((char *) cptr->NickListTable, 0, sizeof(struct hash_entry) * NICKLIST_HASHSIZE);
    }
}

void show_nicklist_hashtable(struct channel *cptr)
{
    int count, count2;
    struct nick_list *ptr;

    for (count = 0; count < NICKLIST_HASHSIZE; count++) {
	if (cptr->NickListTable[count].links == 0)
	    continue;
	say("HASH DEBUG: %d   links %d   hits %d", count, cptr->NickListTable[count].links, cptr->NickListTable[count].hits);

	for (ptr = (struct nick_list *) cptr->NickListTable[count].list, count2 = 0; ptr; count2++, ptr = ptr->next) {
	    say("HASH_DEBUG: %d:%d  %s!%s", count, count2, ptr->nick, ptr->host);
	}
    }
}

void show_whowas_debug_hashtable(struct whowas_list_head *cptr)
{
    int count, count2;
    struct whowas_list *ptr;

    for (count = 0; count < WHOWASLIST_HASHSIZE; count++) {
	if (cptr->NickListTable[count].links == 0)
	    continue;
	say("HASH DEBUG: %d   links %d   hits %d", count, cptr->NickListTable[count].links, cptr->NickListTable[count].hits);

	for (ptr = (struct whowas_list *) cptr->NickListTable[count].list, count2 = 0; ptr; count2++, ptr = ptr->next) {
	    say("HASH_DEBUG: %d:%d  %10s %s!%s", count, count2, ptr->channel, ptr->nicklist->nick, ptr->nicklist->host);
	}
    }
}

void cmd_show_hash(struct command *cmd, char *args)
{
    char *c;
    struct channel *chan = NULL;

    extern int from_server;
    extern struct whowas_list_head whowas_userlist_list;
    extern struct whowas_list_head whowas_reg_list;
    extern struct whowas_list_head whowas_splitin_list;

    if (args && *args)
	c = next_arg(args, &args);
    else
	c = get_current_channel_by_refnum(0);
    if (c && from_server > -1)
	chan = (struct channel *) find_in_list((struct list **) &server_list[from_server].chan_list, c, 0);
    if (chan)
	show_nicklist_hashtable(chan);
    show_whowas_debug_hashtable(&whowas_userlist_list);
    show_whowas_debug_hashtable(&whowas_reg_list);
    show_whowas_debug_hashtable(&whowas_splitin_list);
}

/*
 * the following routines are written by Colten Edwards (panasync)
 * to hash the whowas lists that the client keeps.
 */

static unsigned long hash_userhost_channel(char *userhost, char *channel, unsigned int size)
{
    const unsigned char *p = (const unsigned char *) userhost;
    unsigned long g, hash = 0;

    while (*p) {
	hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p);
	if ((g = hash & 0xF0000000))
	    hash ^= g >> 24;
	hash &= ~g;
	p++;
    }
    p = (const unsigned char *) channel;
    if (p) {
	while (*p) {
	    hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p);
	    if ((g = hash & 0xF0000000))
		hash ^= g >> 24;
	    hash &= ~g;
	    p++;
	}
    }
    return (hash % size);
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_link_to_top_whowas(struct whowas_list *tmp, struct whowas_list *prev, struct hash_entry *location)
{
    if (prev) {
	struct whowas_list *old_list;

	old_list = (struct whowas_list *) location->list;
	location->list = (void *) tmp;
	prev->next = tmp->next;
	tmp->next = old_list;
    }
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_link_from_whowaslist(struct whowas_list *tmp, struct whowas_list *prev, struct hash_entry *location)
{
    if (prev) {
	/* remove the link from the middle of the list */
	prev->next = tmp->next;
    } else {
	/* unlink the first link, and connect next one up */
	location->list = (void *) tmp->next;
    }
    /* set tmp's next to NULL, as its unlinked now */
    tmp->next = NULL;
}

/*
 * add_nicklist_to_channellist: This function will add the nicklist
 * into the channellist, ensuring that we hash the nicklist, and
 * insert the struct correctly into the channelist's Nicklist hash
 * array
 */
void add_whowas_userhost_channel(struct whowas_list *wptr, struct whowas_list_head *list)
{
    unsigned long hvalue = hash_userhost_channel(wptr->nicklist->host, wptr->channel, WHOWASLIST_HASHSIZE);

    /* take this nicklist, and attach it as the HEAD pointer in our chain at the hashed location in our array... Note, by doing this,
     * this ensures that the "most active" users always remain at the top of the chain... ie, faster lookups for active users, (and as 
     * a side note, makes doing the add quite simple!) */
    wptr->next = (struct whowas_list *) list->NickListTable[hvalue].list;

    /* assign our new linked list into array spot */
    list->NickListTable[hvalue].list = (void *) wptr;
    /* quick tally of nicks in chain in this array spot */
    list->NickListTable[hvalue].links++;
    /* keep stats on hits to this array spot */
    list->NickListTable[hvalue].hits++;
    list->total_links++;
}

struct whowas_list *find_userhost_channel(char *host, char *channel, int remove, struct whowas_list_head *wptr)
{
    struct hash_entry *location;
    register struct whowas_list *tmp, *prev = NULL;
    unsigned long hvalue;

    hvalue = hash_userhost_channel(host, channel, WHOWASLIST_HASHSIZE);
    location = &(wptr->NickListTable[hvalue]);

    /* at this point, we found the array spot, now search as regular linked list, or as ircd likes to say... "We found the bucket, now 
     * search the chain" */
    for (tmp = (struct whowas_list *) (&(wptr->NickListTable[hvalue]))->list; tmp; prev = tmp, tmp = tmp->next) {
	if (!my_stricmp(host, tmp->nicklist->host) && !my_stricmp(channel, tmp->channel)) {
	    if (remove != REMOVE_FROM_LIST)
		move_link_to_top_whowas(tmp, prev, location);
	    else {
		location->links--;
		remove_link_from_whowaslist(tmp, prev, location);
		wptr->total_unlinks++;
	    }
	    wptr->total_hits++;
	    return tmp;
	}
    }
    return NULL;
}

/*
 * Basically this makes the hash table "look" like a straight linked list
 * This should be used for things that require you to cycle through the
 * full list, ex. for finding ALL matching stuff.
 * : usage should be like :
 *
 *      for (nptr = next_userhost(cptr, NULL); nptr; nptr = 
 *           next_userhost(cptr, nptr))
 *              YourCodeOnTheWhowasListStruct
 */
struct whowas_list *next_userhost(struct whowas_list_head *cptr, struct whowas_list *nptr)
{
    unsigned long hvalue = 0;

    if (!cptr)
	/* No channel! */
	return NULL;
    else if (!nptr) {
	/* wants to start the walk! */
	while ((struct whowas_list *) cptr->NickListTable[hvalue].list == NULL) {
	    hvalue++;
	    if (hvalue >= WHOWASLIST_HASHSIZE)
		return NULL;
	}
	return (struct whowas_list *) cptr->NickListTable[hvalue].list;
    } else if (nptr->next) {
	/* still returning a chain! */
	return nptr->next;
    } else if (!nptr->next) {
	int hvalue;

	/* hit end of chain, go to next bucket */
	hvalue = hash_userhost_channel(nptr->nicklist->host, nptr->channel, WHOWASLIST_HASHSIZE) + 1;
	if (hvalue >= WHOWASLIST_HASHSIZE) {
	    /* end of list */
	    return NULL;
	} else {
	    while ((struct whowas_list *) cptr->NickListTable[hvalue].list == NULL) {
		hvalue++;
		if (hvalue >= WHOWASLIST_HASHSIZE)
		    return NULL;
	    }
	    /* return head of next filled bucket */
	    return (struct whowas_list *) cptr->NickListTable[hvalue].list;
	}
    } else
	/* shouldn't ever be here */
	say("WHOWAS_HASH_ERROR: next_userhost");
    return NULL;
}

void show_whowas_hashtable(struct whowas_list_head *cptr, char *list)
{
    int count, count2 = 1;
    struct whowas_list *ptr;

    say("WhoWas %s Cache Stats: %lu hits  %lu  links  %lu unlinks", list, cptr->total_hits, cptr->total_links, cptr->total_unlinks);
    for (count = 0; count < WHOWASLIST_HASHSIZE; count++) {

	if (cptr->NickListTable[count].links == 0)
	    continue;
	for (ptr = (struct whowas_list *) cptr->NickListTable[count].list; ptr; count2++, ptr = ptr->next)
	    put_it("%s",
		   convert_output_format("%K[%W$[3]0%K] %Y$[10]1 %W$2%G!%c$3", "%d %s %s %s", count2, ptr->channel,
					 ptr->nicklist->nick, ptr->nicklist->host));
    }
}

int show_wholeft_hashtable(struct whowas_list_head *cptr, time_t ltime, int *total, int *hook, char *list)
{
    int count, count2;
    struct whowas_list *ptr;

    for (count = 0; count < WHOWASLIST_HASHSIZE; count++) {

	if (cptr->NickListTable[count].links == 0)
	    continue;
	for (ptr = (struct whowas_list *) cptr->NickListTable[count].list, count2 = 1; ptr; count2++, ptr = ptr->next) {
	    if (ptr->server1 && ptr->server2) {
		if (!(*total)++
		    && (*hook =
			do_hook(WHOLEFT_HEADER_LIST, "%s %s %s %s %s %s", "Nick", "Host", "Channel", "Time", "Server", "Server")))
		    put_it("%s", convert_output_format(get_fset_var(FORMAT_WHOLEFT_HEADER_FSET), NULL));
		if (do_hook
		    (WHOLEFT_LIST, "%s %s %s %ld %s %s", ptr->nicklist->nick, ptr->nicklist->host, ptr->channel, ltime - ptr->time,
		     ptr->server1 ? ptr->server1 : "Unknown", ptr->server2 ? ptr->server2 : "Unknown"))
		    put_it("%s",
			   convert_output_format(get_fset_var(FORMAT_WHOLEFT_USER_FSET), "%s %s %s %l %s", ptr->nicklist->nick,
						 ptr->nicklist->host, ptr->channel, (long) ltime - ptr->time,
						 ptr->server1 ? ptr->server1 : ""));
	    }
	}
    }
    return *hook;
}

int remove_oldest_whowas_hashlist(struct whowas_list_head *list, time_t timet, int count)
{
    struct whowas_list *ptr;
    register time_t t;
    int total = 0;
    register unsigned long x;

    if (!count) {
	t = time(NULL);
	for (x = 0; x < WHOWASLIST_HASHSIZE; x++) {
	    ptr = (struct whowas_list *) (&(list->NickListTable[x]))->list;
	    if (!ptr || !ptr->nicklist)
		continue;
	    while (ptr) {
		if ((ptr->time + timet) <= t) {
		    ptr = find_userhost_channel(ptr->nicklist->host, ptr->channel, 1, list);
		    new_free(&(ptr->nicklist->nick));
		    new_free(&(ptr->nicklist->host));
		    new_free(&(ptr->nicklist));
		    new_free(&(ptr->channel));
		    new_free(&(ptr->server1));
		    new_free(&(ptr->server2));
		    new_free(&ptr);
		    total++;
		    ptr = (struct whowas_list *) (&(list->NickListTable[x]))->list;
		} else
		    ptr = ptr->next;
	    }
	}
    } else {
	while ((ptr = next_userhost(list, NULL)) && count) {
	    x = hash_userhost_channel(ptr->nicklist->host, ptr->channel, WHOWASLIST_HASHSIZE);
	    ptr = find_userhost_channel(ptr->nicklist->host, ptr->channel, 1, list);
	    if (ptr->nicklist) {
		new_free(&(ptr->nicklist->nick));
		new_free(&(ptr->nicklist->host));
		new_free(&(ptr->nicklist));
	    }
	    new_free(&(ptr->channel));
	    new_free(&(ptr->server1));
	    new_free(&(ptr->server2));
	    new_free(&ptr);
	    total++;
	    count--;
	}
    }
    return total;
}

struct nick_list *sorted_nicklist(struct channel *chan)
{
    struct nick_list *tmp, *l = NULL, *list = NULL;

    for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp)) {
	l = new_malloc(sizeof(struct nick_list));
	memcpy(l, tmp, sizeof(struct nick_list));
	l->next = NULL;
	add_to_list((struct list **) &list, (struct list *) l);
    }
    return list;
}

void clear_sorted_nicklist(struct nick_list **list)
{
    struct nick_list *t;

    while (*list) {
	t = (*list)->next;
	new_free((char **) &(*list));
	*list = t;
    }
}

struct flood *add_name_to_floodlist(char *name, char *channel, struct hash_entry *list, unsigned int size)
{
    struct flood *nptr;
    unsigned long hvalue = hash_nickname(name, size);
    nptr = new_malloc(sizeof(struct flood));
    nptr->next = (struct flood *) list[hvalue].list;
    strmcpy(nptr->name, name, sizeof(nptr->name) - 1);
    list[hvalue].list = (void *) nptr;
    /* quick tally of nicks in chain in this array spot */
    list[hvalue].links++;
    /* keep stats on hits to this array spot */
    list[hvalue].hits++;
    return nptr;
}

struct flood *find_name_in_floodlist(char *name, struct hash_entry *list, unsigned int size, int remove)
{
    struct hash_entry *location;
    register struct flood *tmp, *prev = NULL;
    unsigned long hvalue = hash_nickname(name, size);

    location = &(list[hvalue]);

    /* at this point, we found the array spot, now search as regular linked list, or as ircd likes to say... "We found the bucket, now 
     * search the chain" */
    for (tmp = (struct flood *) location->list; tmp; prev = tmp, tmp = tmp->next) {
	if (!my_stricmp(name, tmp->name)) {
	    if (remove != REMOVE_FROM_LIST)
		move_gen_link_to_top((struct list *) tmp, (struct list *) prev, location);
	    else {
		location->links--;
		remove_gen_link_from_list((struct list *) tmp, (struct list *) prev, location);
	    }
	    return tmp;
	}
    }
    return NULL;
}
