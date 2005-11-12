/*
 * hash2.h: function header file for hash.c
 *
 * Written by Scott H Kilau
 *
 * CopyRight(c) 1997
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 *
 * @(#)$Id$
 */

#ifndef _HASH2_H_
#define _HASH2_H_

#include "struct.h"
#include "whowas.h"
#include "hash.h"

/* Generic List type hash list */
void	add_name_to_genericlist (char *, struct hash_entry *, unsigned int);
struct list	*find_name_in_genericlist (char *, struct hash_entry *, unsigned int, int);
struct list	*next_namelist(struct hash_entry *, struct list *, unsigned int);

void	add_nicklist_to_channellist(struct nick_list *, struct channel *);

void	add_whowas_userhost_channel(struct whowas_list *, struct whowas_list_head *);

struct whowas_list *find_userhost_channel(char *, char *, int, struct whowas_list_head *);

void	clear_whowas_hash_table(struct whowas_list_head *);
int	remove_oldest_whowas_hashlist(struct whowas_list_head *, time_t, int);

struct whowas_list *next_userhost_channel(struct whowas_list_head *, struct whowas_list *);

struct nick_list *find_nicklist_in_channellist(char *, struct channel *, int);
struct nick_list *next_nicklist(struct channel *, struct nick_list *);

void	clear_nicklist_hashtable(struct channel *);
void	show_nicklist_hashtable(struct channel *);

void show_whowas_hashtable(struct whowas_list_head *cptr, char *);
struct whowas_list *next_userhost(struct whowas_list_head *cptr, struct whowas_list *nptr);
int show_wholeft_hashtable(struct whowas_list_head *cptr, time_t ltime, int *total, int *hook, char *);

/* Added to sort a hash'd nicklist and them remove the sorted list */
extern struct nick_list *sorted_nicklist(struct channel *);
extern void clear_sorted_nicklist(struct nick_list **);

struct flood *find_name_in_floodlist(char *, struct hash_entry *, unsigned int, int);
struct flood *add_name_to_floodlist(char *, char *, struct hash_entry *, unsigned int);

#endif
