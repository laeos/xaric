/*
 * whowas.c   a linked list buffer of people who have left your channel 
 * mainly used for ban prot and stats stuff.
 * Should even speed stuff up a bit too.
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 1995
 * Modified Colten Edwards 1996
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */

#include "irc.h"

#include "struct.h"

#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "names.h"
#include "alias.h"
#include "output.h"
#include "numbers.h"
#include "status.h"
#include "screen.h"
#include "commands.h"
#include "config.h"
#include "list.h"
#include "misc.h"
#include "hash.h"
#include "hash2.h"
#include "whowas.h"
#include "fset.h"

WhowasWrapList whowas_userlist_list =
{0};
WhowasWrapList whowas_reg_list =
{0};
WhowasWrapList whowas_splitin_list =
{0};

WhowasChanList *whowas_chan_list = NULL;

static int whowas_userlist_count = 0;
static int whowas_reg_count = 0;
static int whowas_chan_count = 0;

extern WhowasList *
check_whowas_buffer (char *nick, char *userhost, char *channel, int unlink)
{
	WhowasList *tmp = NULL;
	if (!(tmp = find_userhost_channel (userhost, channel, unlink, &whowas_userlist_list)))
		tmp = find_userhost_channel (userhost, channel, unlink, &whowas_reg_list);
	return tmp;
}


extern WhowasList *
check_whowas_nick_buffer (char *nick, char *channel, int unlink)
{
	WhowasList *tmp = NULL, *last = NULL;
	for (tmp = next_userhost (&whowas_userlist_list, NULL); tmp; tmp = next_userhost (&whowas_userlist_list, tmp))
	{
		if (!my_stricmp (tmp->nicklist->nick, nick) && !my_stricmp (tmp->channel, channel))
		{
			if (unlink)
			{
				last = find_userhost_channel (tmp->nicklist->host, tmp->channel, 1, &whowas_userlist_list);
				tmp = NULL;
			}
			return last ? last : tmp;
		}
	}
	for (tmp = next_userhost (&whowas_reg_list, NULL); tmp; tmp = next_userhost (&whowas_reg_list, tmp))
	{
		if (!my_stricmp (tmp->nicklist->nick, nick) && !my_stricmp (tmp->channel, channel))
		{
			if (unlink)
			{
				last = find_userhost_channel (tmp->nicklist->host, tmp->channel, 1, &whowas_reg_list);
				tmp = NULL;
			}
			return last ? last : tmp;
		}
	}
	return (NULL);
}

extern WhowasList *
check_whosplitin_buffer (char *nick, char *userhost, char *channel, int unlink)
{
	WhowasList *tmp = NULL;
	tmp = find_userhost_channel (userhost, channel, unlink, &whowas_splitin_list);
	return tmp;
}

void 
add_to_whowas_buffer (NickList * nicklist, char *channel, char *server1, char *server2)
{
	WhowasList *new;
	if (!nicklist || !nicklist->nick)
		return;

	if (whowas_reg_count >= whowas_reg_max)
	{
		whowas_reg_count -=
			remove_oldest_whowas (&whowas_reg_list, 0,
				   (whowas_reg_max + 1) - whowas_reg_count);
	}
	new = (WhowasList *) new_malloc (sizeof (WhowasList));
	new->has_ops = nicklist->chanop;
	new->nicklist = (NickList *) nicklist;
	malloc_strcpy (&(new->channel), channel);
	malloc_strcpy (&(new->server1), server1);
	malloc_strcpy (&(new->server2), server2);
	new->time = time (NULL);
	add_whowas_userhost_channel (new, &whowas_reg_list);
	whowas_reg_count++;
}

void 
add_to_whosplitin_buffer (NickList * nicklist, char *channel, char *server1, char *server2)
{
	WhowasList *new;

	new = (WhowasList *) new_malloc (sizeof (WhowasList));
	new->has_ops = nicklist->chanop;

	new->nicklist = (NickList *) new_malloc (sizeof (NickList));	/*nicklist; */
	new->nicklist->nick = m_strdup (nicklist->nick);
	new->nicklist->host = m_strdup (nicklist->host);

	malloc_strcpy (&(new->channel), channel);
	malloc_strcpy (&(new->server1), server1);
	malloc_strcpy (&(new->server2), server2);
	new->time = time (NULL);
	add_whowas_userhost_channel (new, &whowas_splitin_list);

}

int 
remove_oldest_whowas (WhowasWrapList * list, time_t timet, int count)
{
	int total = 0;
	/* if no ..count.. then remove ..time.. links */
	total = remove_oldest_whowas_hashlist (list, timet, count);
	return total;
}

void 
clean_whowas_list (void)
{
	if (whowas_userlist_count)
		whowas_userlist_count -= remove_oldest_whowas_hashlist (&whowas_userlist_list, 20 * 60, 0);
	if (whowas_reg_count)
		whowas_reg_count -= remove_oldest_whowas_hashlist (&whowas_reg_list, 10 * 60, 0);
	remove_oldest_whowas_hashlist (&whowas_splitin_list, 15 * 60, 0);
}



/* BELOW THIS MARK IS THE CHANNEL WHOWAS STUFF */

extern WhowasChanList *
check_whowas_chan_buffer (char *channel, int unlink)
{
	WhowasChanList *tmp, *last = NULL;

	for (tmp = whowas_chan_list; tmp; tmp = tmp->next)
	{
		if (!my_stricmp (tmp->channellist->channel, channel))
		{
			if (unlink)
			{
				if (last)
					last->next = tmp->next;
				else
					whowas_chan_list = tmp->next;
				whowas_chan_count--;
			}
			return (tmp);
		}
		last = tmp;
	}
	return (NULL);
}

void 
add_to_whowas_chan_buffer (ChannelList * channel)
{
	WhowasChanList *new;
	WhowasChanList **slot;

	if (whowas_chan_count >= whowas_chan_max)
	{
		whowas_chan_count -=
			remove_oldest_chan_whowas (&whowas_chan_list, 0,
				 (whowas_chan_max + 1) - whowas_chan_count);
	}
	new = (WhowasChanList *) new_malloc (sizeof (WhowasChanList));

	new->channellist = channel;
	new->time = time (NULL);
	clear_nicklist_hashtable (channel);
	/* we've created it, now put it in order */
	for (slot = &whowas_chan_list; *slot; slot = &(*slot)->next)
	{
		if ((*slot)->time > new->time)
			break;
	}
	new->next = *slot;
	*slot = new;
	whowas_chan_count++;
}

int 
remove_oldest_chan_whowas (WhowasChanList ** list, time_t timet, int count)
{
	WhowasChanList *tmp = NULL;
	time_t t;
	int total = 0;

	/* if no ..count.. then remove ..time.. links */
	if (!count)
	{
		t = time (NULL);
		while (*list && ((*list)->time + timet) <= t)
		{
			tmp = *list;
			new_free (&(tmp->channellist->channel));
			new_free (&(tmp->channellist->topic));
			tmp->channellist->bans = NULL;
			new_free ((char **) &(tmp->channellist));
			*list = tmp->next;
			new_free ((char **) &tmp);
			total++;
		}
	}
	else
	{
		while (*list && count)
		{
			tmp = *list;
			new_free (&(tmp->channellist->channel));
			new_free (&(tmp->channellist->topic));
			tmp->channellist->bans = NULL;
			new_free ((char **) &(tmp->channellist));
			*list = tmp->next;
			new_free ((char **) &tmp);
			total++;
			count--;
		}
	}
	return total;
}

void 
clean_whowas_chan_list (void)
{
	whowas_chan_count -= remove_oldest_chan_whowas (&whowas_chan_list, 24 * 60 * 60, 0);
}

void 
show_whowas (void)
{
	show_whowas_hashtable (&whowas_userlist_list, "Userlist");
	show_whowas_hashtable (&whowas_reg_list, "Reglist");
}

void 
show_wholeft (char *channel)
{
	int count = 0;
	int hook = 0;
	time_t ltime = time (NULL);
#if 0
	hook = show_wholeft_hashtable (&whowas_splitin_list, ltime, &count, &hook, "SplitList");
#endif
	hook = show_wholeft_hashtable (&whowas_userlist_list, ltime, &count, &hook, "Splitin");
	hook = show_wholeft_hashtable (&whowas_reg_list, ltime, &count, &hook, "Splitin");
	if (count && hook && get_fset_var (FORMAT_WHOLEFT_FOOTER_FSET))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOLEFT_FOOTER_FSET), NULL));
}
