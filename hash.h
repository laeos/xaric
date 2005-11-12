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

/* TODO: work on this dependency mess */
struct whowas_list_head;
struct whowas_list;
struct nick_list;
struct command;
struct channel;
struct flood;

/* hashentry: structure for all hash lists we make. 
 * quite generic, but powerful. */
struct hash_entry {
    void *list;			/* our linked list, generic void * */
    int hits;			/* how many hits this spot has gotten */
    int links;			/* how many links we have at this spot */
};

/* hash.c */
void add_name_to_genericlist(char *name, struct hash_entry *list, unsigned int size);
struct list *find_name_in_genericlist(char *name, struct hash_entry *list, unsigned int size, int remove);
void add_nicklist_to_channellist(struct nick_list *nptr, struct channel *cptr);
struct nick_list *find_nicklist_in_channellist(char *nick, struct channel *cptr, int remove);
struct nick_list *next_nicklist(struct channel *cptr, struct nick_list *nptr);
struct list *next_namelist(struct hash_entry *cptr, struct list *nptr, unsigned int size);
void clear_nicklist_hashtable(struct channel *cptr);
void show_nicklist_hashtable(struct channel *cptr);
void show_whowas_debug_hashtable(struct whowas_list_head *cptr);
void cmd_show_hash(struct command *cmd, char *args);
void add_whowas_userhost_channel(struct whowas_list *wptr, struct whowas_list_head *list);
struct whowas_list *find_userhost_channel(char *host, char *channel, int remove, struct whowas_list_head *wptr);
struct whowas_list *next_userhost(struct whowas_list_head *cptr, struct whowas_list *nptr);
void show_whowas_hashtable(struct whowas_list_head *cptr, char *list);
int show_wholeft_hashtable(struct whowas_list_head *cptr, time_t ltime, int *total, int *hook, char *list);
int remove_oldest_whowas_hashlist(struct whowas_list_head *list, time_t timet, int count);
struct nick_list *sorted_nicklist(struct channel *chan);
void clear_sorted_nicklist(struct nick_list **list);
struct flood *add_name_to_floodlist(char *name, char *channel, struct hash_entry *list, unsigned int size);
struct flood *find_name_in_floodlist(char *name, struct hash_entry *list, unsigned int size, int remove);

#endif	/* HASH_H */
