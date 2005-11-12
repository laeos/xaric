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
void	add_name_to_genericlist (char *, HashEntry *, unsigned int);
List	*find_name_in_genericlist (char *, HashEntry *, unsigned int, int);
List	*next_namelist(HashEntry *, List *, unsigned int);

void	add_nicklist_to_channellist(NickList *, ChannelList *);

void	add_whowas_userhost_channel(WhowasList *, WhowasWrapList *);

WhowasList *find_userhost_channel(char *, char *, int, WhowasWrapList *);

void	clear_whowas_hash_table(WhowasWrapList *);
int	remove_oldest_whowas_hashlist(WhowasWrapList *, time_t, int);

WhowasList *next_userhost_channel(WhowasWrapList *, WhowasList *);

NickList *find_nicklist_in_channellist(char *, ChannelList *, int);
NickList *next_nicklist(ChannelList *, NickList *);

void	clear_nicklist_hashtable(ChannelList *);
void	show_nicklist_hashtable(ChannelList *);

void show_whowas_hashtable(WhowasWrapList *cptr, char *);
WhowasList *next_userhost(WhowasWrapList *cptr, WhowasList *nptr);
int show_wholeft_hashtable(WhowasWrapList *cptr, time_t ltime, int *total, int *hook, char *);

/* Added to sort a hash'd nicklist and them remove the sorted list */
extern NickList *sorted_nicklist(ChannelList *);
extern void clear_sorted_nicklist(NickList **);

Flooding *find_name_in_floodlist(char *, HashEntry *, unsigned int, int);
Flooding *add_name_to_floodlist(char *, char *, HashEntry *, unsigned int);

#endif
