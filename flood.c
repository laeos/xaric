#ident "@(#)flood.c 1.6"
/*
 * flood.c: handle channel flooding. 
 *
 * This attempts to give you some protection from flooding.  Basically, it keeps
 * track of how far apart (timewise) messages come in from different people.
 * If a single nickname sends more than 3 messages in a row in under a
 * second, this is considered flooding.  It then activates the ON FLOOD with
 * the nickname and type (appropriate for use with IGNORE). 
 *
 * Thanks to Tomi Ollila <f36664r@puukko.hut.fi> for this one. 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "hook.h"
#include "ircaux.h"
#include "ignore.h"
#include "flood.h"
#include "vars.h"
#include "output.h"
#include "list.h"
#include "misc.h"
#include "server.h"
#include "timer.h"
#include "ignore.h"
#include "status.h"
#include "hash.h"
#include "fset.h"
#include "tcommand.h"


static char *ignore_types[] =
{
	"",
	"MSG",
	"PUBLIC",
	"NOTICE",
	"WALL",
	"WALLOP",
	"CTCP",
	"INVITE",
	"ACTION"
};

#if 0
typedef struct flood_stru
{
	char nick[NICKNAME_LEN + 1];
	char channel[BIG_BUFFER_SIZE + 1];
	int type;
	char flood;
	long cnt;
	time_t start;
}
struct flood;
#endif

#define FLOOD_HASHSIZE 31


struct hash_entry no_flood_list[FLOOD_HASHSIZE];
struct hash_entry flood_list[FLOOD_HASHSIZE];

static int remove_oldest_flood_hashlist (struct hash_entry *, time_t, int);




extern char *FromUserHost;
extern unsigned int window_display;
extern int from_server;

static double allow_flood = 0.0;
static double this_flood = 0.0;

#define NO_RESET 0
#define RESET 1



void
cmd_no_flood (struct command *cmd, char *args)
{
	char *nick;
	struct list *nptr;


	if (!args || !*args)
	{
		int count = 0;

		for (nptr = next_namelist (no_flood_list, NULL, FLOOD_HASHSIZE); nptr; nptr = next_namelist (no_flood_list, nptr, FLOOD_HASHSIZE))
		{
			if (count++ == 0)
				put_it ("%s", convert_output_format ("%G+-[ %CNo Flood List %G]-------------------------------------", NULL));
			put_it ("%s", convert_output_format ("%G| %c$0", "%s", nptr->name));
		}
		if (count == 0)
			put_it ("%s", convert_output_format ("%RNo flood list%C is empty.", NULL));
		return;
	}
	nick = next_arg (args, &args);
	while (nick && *nick)
	{
		if (*nick == '-')
		{
			if ((nptr = find_name_in_genericlist (nick + 1, no_flood_list, FLOOD_HASHSIZE, 1)))
			{
				new_free (&nptr->name);
				new_free ((char **) &nptr);
				bitchsay ("%s removed from no-flood list", nick);
			}
			else
				bitchsay ("%s is not on your no-flood list", nick);
		}
		else
		{
			if (*nick == '+')
				++nick;
			if (!(nptr = find_name_in_genericlist (nick, no_flood_list, FLOOD_HASHSIZE, 0)))
			{
				add_name_to_genericlist (nick, no_flood_list, FLOOD_HASHSIZE);
				bitchsay ("%s will no longer be checked for flooding");
			}
		}
		nick = next_arg (args, &args);
	}
}

int 
get_flood_rate (int type, struct channel * channel)
{
	int flood_rate = get_int_var (FLOOD_RATE_VAR);
	if (channel)
	{
		switch (type)
		{
		case JOIN_FLOOD:
			flood_rate = get_int_var (JOINFLOOD_TIME_VAR);
			break;
		case PUBLIC_FLOOD:
			flood_rate = get_int_var (PUBFLOOD_TIME_VAR);
			break;
		case NICK_FLOOD:
			flood_rate = get_int_var (NICKFLOOD_TIME_VAR);
			break;
		case KICK_FLOOD:
			flood_rate = get_int_var (KICKFLOOD_TIME_VAR);
			break;
		case DEOP_FLOOD:
			flood_rate = get_int_var (DEOPFLOOD_TIME_VAR);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (type)
		{
		case CTCP_FLOOD:
			flood_rate = get_int_var (DCC_FLOOD_RATE_VAR);
		case CTCP_ACTION_FLOOD:
		default:
			break;
		}
	}
	return flood_rate;
}

int 
get_flood_count (int type, struct channel * channel)
{
	int flood_count = get_int_var (FLOOD_AFTER_VAR);
	if (channel)
	{
		switch (type)
		{
		case JOIN_FLOOD:
			flood_count = get_int_var (KICK_ON_JOINFLOOD_VAR);
			break;
		case PUBLIC_FLOOD:
			flood_count = get_int_var (KICK_ON_PUBFLOOD_VAR);
			break;
		case NICK_FLOOD:
			flood_count = get_int_var (KICK_ON_NICKFLOOD_VAR);
			break;
		case KICK_FLOOD:
			flood_count = get_int_var (KICK_ON_KICKFLOOD_VAR);
			break;
		case DEOP_FLOOD:
			flood_count = get_int_var (KICK_ON_DEOPFLOOD_VAR);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (type)
		{
		case CTCP_FLOOD:
			flood_count = get_int_var (DCC_FLOOD_AFTER_VAR);
		case CTCP_ACTION_FLOOD:
		default:
			break;
		}
	}
	return flood_count;
}

int 
set_flood (int type, time_t flood_time, int reset, struct nick_list * tmpnick)
{
	if (!tmpnick)
		return 0;
	switch (type)
	{
	case JOIN_FLOOD:
		if (reset == RESET)
		{
			tmpnick->joincount = 1;
			tmpnick->jointime = flood_time;
		}
		else
			tmpnick->joincount++;
		break;
	case PUBLIC_FLOOD:
		if (reset == RESET)
		{
			tmpnick->floodcount = 1;
			tmpnick->floodtime = tmpnick->idle_time = flood_time;
		}
		else
			tmpnick->floodcount++;
		break;
	case NICK_FLOOD:
		if (reset == RESET)
		{
			tmpnick->nickcount = 1;
			tmpnick->nicktime = flood_time;
		}
		else
			tmpnick->nickcount++;
		break;
	case KICK_FLOOD:
		if (reset == RESET)
		{
			tmpnick->kickcount = 1;
			tmpnick->kicktime = flood_time;
		}
		else
			tmpnick->kickcount++;
		break;
	case DEOP_FLOOD:
		if (reset == RESET)
		{
			tmpnick->dopcount = 1;
			tmpnick->doptime = flood_time;
		}
		else
			tmpnick->dopcount++;
		break;
	default:
		break;
	}
	return 1;
}

int 
is_other_flood (struct channel * channel, struct nick_list * tmpnick, int type, int *t_flood)
{
	time_t diff = 0, flood_time = 0;
	int doit = 0;
	int count = 0;
	int flood_rate = 0, flood_count = 0;

	flood_time = time (NULL);

	if (!channel || !tmpnick)
		return 0;
	if (!strcmp (get_server_nickname (from_server), tmpnick->nick))
		return 0;
	if (find_name_in_genericlist (tmpnick->nick, no_flood_list, FLOOD_HASHSIZE, 0))
		return 0;

	set_flood (type, flood_time, NO_RESET, tmpnick);
	switch (type)
	{
	case JOIN_FLOOD:
		if (!get_int_var (JOINFLOOD_VAR))
			break;
		diff = flood_time - tmpnick->jointime;
		count = tmpnick->joincount;
		doit = 1;
		break;
	case PUBLIC_FLOOD:
		if (!get_int_var (PUBFLOOD_VAR))
			break;
		diff = flood_time - tmpnick->floodtime;
		count = tmpnick->floodcount;
		doit = 1;
		break;
	case NICK_FLOOD:
		if (!get_int_var (NICKFLOOD_VAR))
			break;
		diff = flood_time - tmpnick->nicktime;
		count = tmpnick->nickcount;
		doit = 1;
		break;
	case DEOP_FLOOD:
		if (!get_int_var (DEOPFLOOD_VAR))
			break;
		diff = flood_time - tmpnick->doptime;
		count = tmpnick->dopcount;
		doit = 1;
		break;
	case KICK_FLOOD:
		if (!get_int_var (KICKFLOOD_VAR))
			break;
		diff = flood_time - tmpnick->kicktime;
		count = tmpnick->kickcount;
		doit = 1;
		break;
	default:
		break;
	}
	if (doit)
	{
		flood_count = get_flood_count (type, channel);
		flood_rate = get_flood_rate (type, channel);
		if (count >= flood_count)
		{
			int flooded = 0;
			if (!diff || (flood_rate && (diff < flood_rate)))
			{
				*t_flood = diff;
				flooded = 1;
			}
			set_flood (type, flooded ? 0L : flood_time, RESET, tmpnick);
			return flooded;
		}
	}
	return 0;
}

char *
get_ignore_types (unsigned int type)
{
	int x = 0;
	while (type)
	{
		type = type >> 1;
		x++;
	}
	return ignore_types[x];
}
/*
 * check_flooding: This checks for message flooding of the type specified for
 * the given nickname.  This is described above.  This will return 0 if no
 * flooding took place, or flooding is not being monitored from a certain
 * person.  It will return 1 if flooding is being check for someone and an ON
 * FLOOD is activated. 
 */
int 
check_flooding (char *nick, int type, char *line, char *channel)
{
	static int users = 0, pos = 0;
	time_t flood_time = time (NULL), diff = 0;

	struct flood *tmp;
	int flood_rate, flood_count;


	if (!(users = get_int_var (FLOOD_USERS_VAR)))
		return 1;
	if (find_name_in_genericlist (nick, no_flood_list, FLOOD_HASHSIZE, 0))
		return 1;
	if (!(tmp = find_name_in_floodlist (nick, flood_list, FLOOD_HASHSIZE, 0)))
	{
		if (pos >= users)
		{
			pos -= remove_oldest_flood_hashlist (&flood_list[0], 0, (users + 1 - pos));
		}
		tmp = add_name_to_floodlist (nick, channel, flood_list, FLOOD_HASHSIZE);
		tmp->type = type;
		tmp->cnt = 1;
		tmp->start = flood_time;
		tmp->flood = 0;
		pos++;
		return 1;
	}
	if (!(tmp->type & type))
	{
		tmp->type |= type;
		return 1;
	}
	flood_count = get_flood_count (type, NULL);	/* FLOOD_AFTER_VAR */
	flood_rate = get_flood_rate (type, NULL);	/* FLOOD_RATE_VAR */
	if (!flood_count || !flood_rate)
		return 1;
	tmp->cnt++;
	if (tmp->cnt > flood_count)
	{
		diff = flood_time - tmp->start;
		if (diff != 0)
			this_flood = (double) tmp->cnt / (double) diff;
		else
			this_flood = 0;
		allow_flood = (double) flood_count / (double) flood_rate;
		if (!diff || !this_flood || (this_flood > allow_flood))
		{
			if (tmp->flood == 0)
			{
				tmp->flood = 1;
				switch (type)
				{
				case MSG_FLOOD:
				case NOTICE_FLOOD:
				case CTCP_FLOOD:
					if (flood_prot (nick, FromUserHost, get_ignore_types (type), type, get_int_var (IGNORE_TIME_VAR), channel))
						return 0;
					break;
				case CTCP_ACTION_FLOOD:
					if (flood_prot (nick, FromUserHost, get_ignore_types (CTCP_FLOOD), type, get_int_var (IGNORE_TIME_VAR), channel))
						return 0;
				default:
					break;
				}
				if (get_int_var (FLOOD_WARNING_VAR))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_FLOOD_FSET), "%s %s %s %s %s", update_clock (GET_TIME), get_ignore_types (type), nick, FromUserHost, channel ? channel : "unknown"));
			}
			return 1;
		}
		else
		{
			tmp->flood = 0;
			tmp->cnt = 1;
			tmp->start = flood_time;
		}
	}
	return 1;
}

int 
flood_prot (char *nick, char *userhost, char *type, int ctcp_type, int ignoretime, char *channel)
{
	struct channel *chan;
	struct nick_list *Nick;
	char tmp[BIG_BUFFER_SIZE + 1];
	char *uh;
	int old_window_display;
	int kick_on_flood = 1;

	if ((ctcp_type == CTCP_FLOOD || ctcp_type == CTCP_ACTION_FLOOD) && !get_int_var (CTCP_FLOOD_PROTECTION_VAR))
		return 0;
	else if (!get_int_var (FLOOD_PROTECTION_VAR))
		return 0;
	else if (!my_stricmp (nick, get_server_nickname (from_server)))
		return 0;
	switch (ctcp_type)
	{
	case MSG_FLOOD:
		break;
	case NOTICE_FLOOD:
		break;
	case PUBLIC_FLOOD:
		if (channel)
		{
			if ((chan = lookup_channel (channel, from_server, 0)))
			{
				kick_on_flood = get_int_var (PUBFLOOD_VAR);
				if ((Nick = find_nicklist_in_channellist (nick, chan, 0)) && kick_on_flood)
				{
					if (chan->chop)
						send_to_server (SERVER(from_server), "KICK %s %s :\002%s\002 flooder", chan->channel, nick, type);
				}
			}
		}
		break;
	case CTCP_ACTION_FLOOD:
	case CTCP_FLOOD:
	default:
		if (get_int_var (FLOOD_KICK_VAR) && kick_on_flood && channel)
		{
			for (chan = server_list[from_server].chan_list; chan; chan = chan->next)
			{
				if ((Nick = find_nicklist_in_channellist (nick, chan, 0)))
				{
					if (chan->chop)
						send_to_server (SERVER(from_server), "KICK %s %s :\002%s\002 flooder", chan->channel, nick, type);
				}
			}
		}
	}
	if (!ignoretime)
		return 0;
	uh = clear_server_flags (userhost);
	sprintf (tmp, "*!*%s", uh);
	old_window_display = window_display;
	window_display = 0;
	ignore_nickname (tmp, ignore_type (type, strlen (type)), 0);
	window_display = old_window_display;
	sprintf (tmp, "%d ^IGNORE *!*%s NONE", ignoretime, uh);
	t_parse_command ("TIMER", tmp);
	bitchsay ("Auto-ignoring %s for %d minutes [\002%s\002 flood]", nick, ignoretime / 60, type);
	return 1;
}

static int 
remove_oldest_flood_hashlist (struct hash_entry * list, time_t timet, int count)
{
	struct flood *ptr;
	register time_t t;
	int total = 0;
	register unsigned long x;
	t = time (NULL);
	if (!count)
	{
		for (x = 0; x < FLOOD_HASHSIZE; x++)
		{
			ptr = (struct flood *) (list + x)->list;
			if (!ptr || !*ptr->name)
				continue;
			while (ptr)
			{
				if ((ptr->start + timet) <= t)
				{
					ptr = find_name_in_floodlist (ptr->name, flood_list, FLOOD_HASHSIZE, 1);
					new_free (&(ptr->channel));
					new_free ((char **) &ptr);
					total++;
					ptr = (struct flood *) (list + x)->list;
				}
				else
					ptr = ptr->next;
			}
		}
	}
	else
	{
		for (x = 0; x < FLOOD_HASHSIZE; x++)
		{
			struct flood *next = NULL;
			ptr = (struct flood *) (list + x)->list;
			if (!ptr || !*ptr->name)
				continue;
			while (ptr && count)
			{
				ptr = find_name_in_floodlist (ptr->name, flood_list, FLOOD_HASHSIZE, 1);
				next = ptr->next;
				new_free (&(ptr->channel));
				new_free ((char **) &ptr);
				total++;
				count--;
				ptr = (struct flood *) (list + x)->list;
				ptr = next;
			}
		}
	}
	return total;
}

void 
clean_flood_list ()
{
	remove_oldest_flood_hashlist (&flood_list[0], get_int_var (FLOOD_RATE_VAR) + 1, 0);
}
