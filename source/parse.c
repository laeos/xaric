/*
 * parse.c: handles messages from the server.   Believe it or not.  I
 * certainly wouldn't if I were you. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 * Modified Colten Edwards 1997
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "server.h"
#include "names.h"
#include "vars.h"
#include "ctcp.h"
#include "hook.h"
#include "commands.h"
#include "ignore.h"
#include "whois.h"
#include "lastlog.h"
#include "input.h"
#include "notice.h"
#include "ircaux.h"
#include "funny.h"
#include "input.h"
#include "ircterm.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "notify.h"
#include "status.h"
#include "list.h"
#include "misc.h"
#include "whowas.h"
#include "timer.h"
#include "hash2.h"
#include "fset.h"
#include "tcommand.h"

#define space ' '
static void strip_modes (char *, char *, char *);

extern char *do_nslookup (char *);
extern char *random_str (int, int);

extern char *auto_str;

char *last_split_server = NULL;
char *last_split_from = NULL;
int in_server_ping = 0;

extern char *mircansi (char *);
#ifdef COMPRESS_MODES
extern char *compress_modes (int, char *, char *);
#endif

/*
 * joined_nick: the nickname of the last person who joined the current
 * channel 
 */
char *joined_nick = NULL;

/* public_nick: nick of the last person to send a message to your channel */
char *public_nick = NULL;

/* User and host information from server 2.7 */
char *FromUserHost = empty_string;

/* doing a PRIVMSG */
int doing_privmsg = 0;
int chan_who = 0;

extern int sed;


/* returns 1 if the ban is on the channel already, 0 if not */
BanList *
ban_is_on_channel (char *ban, ChannelList * chan)
{
	register BanList *bans;
	for (bans = chan->bans; bans; bans = bans->next)
	{
		if (match (bans->ban, ban) || match (ban, bans->ban))
			return bans;
		else if (!my_stricmp (bans->ban, ban))
			return bans;
	}
	return NULL;
}


void 
fake (void)
{
	bitchsay ("--- Fake Message recieved!!! ---");
	return;
}

int 
check_auto_reply (char *str)
{
	char *p = NULL;
	char *q = NULL;
	char *pat;
	if (!str || !*str || !get_int_var (NICK_COMPLETION_VAR))
		return 0;
	q = p = m_strdup (auto_str);
	if (q && *q)
	{
		while ((pat = next_arg (p, &p)))
		{
			switch (get_int_var (NICK_COMPLETION_TYPE_VAR))
			{
			case 2:
				if (wild_match (pat, str))
					goto found_auto;
				continue;
			case 1:
				if (stristr (str, pat))
					goto found_auto;
				continue;
			default:
			case 0:
				if (!my_strnicmp (str, pat, strlen (pat)))
					goto found_auto;
				continue;
			}
		}
	}
	new_free (&q);
	return 0;
      found_auto:
	new_free (&q);
	return 1;
}

char *
PasteArgs (char **Args, int StartPoint)
{
	int i;

	for (; StartPoint; Args++, StartPoint--)
		if (!*Args)
			return NULL;
	for (i = 0; Args[i] && Args[i + 1]; i++)
		Args[i][strlen (Args[i])] = ' ';
	Args[1] = NULL;
	return Args[0];
}

/*
 * BreakArgs: breaks up the line from the server, in to where its from,
 * setting FromUserHost if it should be, and returns all the arguements
 * that are there.   Re-written by phone, dec 1992.
 */
int 
BreakArgs (char *Input, char **Sender, char **OutPut, int ig_sender)
{
	int ArgCount = 0;

	/*
	 * The RFC describes it fully, but in a short form, a line looks like:
	 * [:sender[!user@host]] COMMAND ARGUMENT [[:]ARGUMENT]{0..14}
	 */

	/*
	 * Look to see if the optional :sender is present.
	 */
	if (!ig_sender)
	{
		if (*Input == ':')
		{
			*Sender = ++Input;
			while (*Input && *Input != space)
				Input++;
			if (*Input == space)
				*Input++ = 0;

			/*
			 * Look to see if the optional !user@host is present.
			 */
			FromUserHost = *Sender;
			while (*FromUserHost && *FromUserHost != '!')
				FromUserHost++;
			if (*FromUserHost == '!')
				*FromUserHost++ = 0;
		}
		/*
		 * No sender present.
		 */
		else
			*Sender = FromUserHost = empty_string;
	}
	/*
	 * Now we go through the argument list...
	 */
	for (;;)
	{
		while (*Input && *Input == space)
			Input++;

		if (!*Input)
			break;

		if (*Input == ':')
		{
			OutPut[ArgCount++] = ++Input;
			break;
		}

		OutPut[ArgCount++] = Input;
		if (ArgCount >= MAXPARA)
			break;

		while (*Input && *Input != space)
			Input++;
		if (*Input == space)
			*Input++ = 0;
	}
	OutPut[ArgCount] = NULL;
	return ArgCount;
}

/* in response to a TOPIC message from the server */
static void 
p_topic (char *from, char **ArgList)
{
	ChannelList *tmp;

	if (!ArgList[1])
	{
		fake ();
		return;
	}
	tmp = lookup_channel (ArgList[0], from_server, CHAN_NOUNLINK);
	malloc_strcpy (&tmp->topic, ArgList[1]);
	if (check_ignore (from, FromUserHost, tmp->channel, IGNORE_TOPICS | IGNORE_CRAP, NULL) != IGNORED)
	{
		message_from (ArgList[0], LOG_CRAP);
		if (do_hook (TOPIC_LIST, "%s %s %s", from, ArgList[0], ArgList[1]))
		{
			if (ArgList[1] && *ArgList[1])
			{
				if (get_fset_var (FORMAT_TOPIC_CHANGE_HEADER_FSET))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_TOPIC_CHANGE_HEADER_FSET), "%s %s %s %s", update_clock (GET_TIME), from, ArgList[0], ArgList[1]));
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_TOPIC_CHANGE_FSET), "%s %s %s %s", update_clock (GET_TIME), from, ArgList[0], ArgList[1]));
			}
			else
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_TOPIC_UNSET_FSET), "%s %s %s", update_clock (GET_TIME), from, ArgList[0]));
		}

		message_from (NULL, LOG_CRAP);
	}
	update_all_status (curr_scr_win, NULL, 0);
}

static void 
p_wallops (char *from, char **ArgList)
{
	char *line;
	int autorep = 0;
	int from_server = strchr (from, '.') ? 1 : 0;

	if (!(line = PasteArgs (ArgList, 0)))
	{
		fake ();
		return;
	}

	if (from_server || check_flooding (from, WALLOP_FLOOD, line, NULL))
	{
		/* The old server check, don't use the whois stuff for servers */
		int level;
		char *high;
		switch (check_ignore (from, FromUserHost, NULL, IGNORE_WALLOPS, NULL))
		{
		case (IGNORED):
			return;
		case (HIGHLIGHTED):
			high = highlight_char;
			break;
		default:
			high = empty_string;
			break;
		}
		message_from (from, LOG_WALLOP);
		level = set_lastlog_msg_level (LOG_WALLOP);
		autorep = check_auto_reply (line);
		if (do_hook (WALLOP_LIST, "%s %c %s", from, from_server ? 'S' : '*', line))
			put_it ("%s", convert_output_format (get_fset_var (from_server ? FORMAT_WALLOP_FSET : (autorep ? FORMAT_WALL_AR_FSET : FORMAT_WALL_FSET)),
				     "%s %s %s %s", update_clock (GET_TIME),
				      from, from_server ? "!" : "*", line));
		if (beep_on_level & LOG_WALLOP)
			beep_em (1);
		set_lastlog_msg_level (level);
		message_from (NULL, LOG_CRAP);
	}
}

void 
whoreply (char *from, char **ArgList)
{
	static char format[40];
	static int last_width = -1;
	int ok = 1, voice = 0, opped = 0;
	char *channel, *user, *host, *server, *nick, *stat, *name;
	ChannelList *chan = NULL;
	char buf_data[BIG_BUFFER_SIZE + 1];


	if (!ArgList[5])
		return;

	if (last_width != get_int_var (CHANNEL_NAME_WIDTH_VAR))
	{
		if ((last_width = get_int_var (CHANNEL_NAME_WIDTH_VAR)) != 0)
			snprintf (format, 39, "%%-%u.%us \002%%-9s\002 %%-3s %%s@%%s (%%s)",
				  (unsigned char) last_width,
				  (unsigned char) last_width);
		else
			strcpy (format, "%s\t\002%-9s\002 %-3s %s@%s (%s)");
	}
	channel = ArgList[0];
	user = ArgList[1];
	host = ArgList[2];
	server = ArgList[3];
	nick = ArgList[4];
	stat = ArgList[5];
	name = ArgList[6];
	PasteArgs (ArgList, 6);

	message_from (channel, LOG_CRAP);
	*buf_data = 0;
	strmopencat (buf_data, BIG_BUFFER_SIZE, user, "@", host, NULL);
	voice = (strchr (stat, '+') != NULL);
	opped = (strchr (stat, '@') != NULL);

	if (*stat == 'S')	/* this only true for the header WHOREPLY */
	{
		char buffer[BIG_BUFFER_SIZE + 1];
		channel = "Channel";
		snprintf (buffer, BIG_BUFFER_SIZE, "%s %s %s %s %s %s %s", channel,
			  nick, stat, user, host, server, name);
		if (do_hook (WHO_LIST, "%s", buffer))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHO_FSET), "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name));
		message_from (NULL, LOG_CRAP);
		return;
	}

	if (who_mask)
	{
		if (who_mask & WHO_HERE)
			ok = ok && (*stat == 'H');
		if (who_mask & WHO_AWAY)
			ok = ok && (*stat == 'G');
		if (who_mask & WHO_OPS)
			ok = ok && (*(stat + 1) == '*');
		if (who_mask & WHO_LUSERS)
			ok = ok && (*(stat + 1) != '*');
		if (who_mask & WHO_CHOPS)
			ok = ok && ((*(stat + 1) == '@') || (*(stat + 2) == '@'));
		if (who_mask & WHO_NAME)
			ok = ok && wild_match (who_name, user);
		if (who_mask & WHO_NICK)
			ok = ok && wild_match (who_nick, nick);
		if (who_mask & WHO_HOST)
			ok = ok && wild_match (who_host, host);
		if (who_mask & WHO_REAL)
			ok = ok && wild_match (who_real, name);
		if (who_mask & WHO_SERVER)
			ok = ok && wild_match (who_server, server);
	}

	if (ok)
	{
		char buffer[BIG_BUFFER_SIZE + 1];

		snprintf (buffer, BIG_BUFFER_SIZE, "%s %s %s %s %s %s %s", channel,
			  nick, stat, user, host, server, name);
		chan = add_to_channel (channel, nick, from_server, opped, voice, buf_data, server, stat);
		if (do_hook (WHO_LIST, "%s", buffer))
		{
			if (!get_int_var (SHOW_WHO_HOPCOUNT_VAR))
				next_arg (name, &name);
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHO_FSET), "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name));
		}
	}
/*      
   else if ((who_mask && WHO_KILL) && !ok)
   {
   put_it("%s", convert_output_format("$G No such match for /whokill", NULL, NULL));
   }
 */
	message_from (NULL, LOG_CRAP);
}

static void 
p_privmsg (char *from, char **Args)
{
	int level, list_type, flood_type, log_type, ar_true = 0, no_flood = 1,
	  do_beep = 1;

	unsigned char ignore_type;
	char *ptr = NULL, *to;
	char *high;
	static int com_do_log, com_lines = 0;
	ChannelList *channel = NULL;
	NickList *tmpnick = NULL;


	if (!from)
		return;
	PasteArgs (Args, 1);
	to = Args[0];
	ptr = Args[1];
	if (!to || !ptr)
	{
		fake ();
		return;
	}
	doing_privmsg = 1;

	if (is_channel (to) && im_on_channel (to))
	{
		message_from (to, LOG_MSG);
		malloc_strcpy (&public_nick, from);
		log_type = LOG_PUBLIC;
		ignore_type = IGNORE_PUBLIC;
		flood_type = PUBLIC_FLOOD;

		if (!is_on_channel (to, from_server, from))
			list_type = PUBLIC_MSG_LIST;
		else
		{
			if (is_current_channel (to, from_server, 0))
				list_type = PUBLIC_LIST;
			else
				list_type = PUBLIC_OTHER_LIST;
			channel = lookup_channel (to, from_server, CHAN_NOUNLINK);
			if (channel)
				tmpnick = find_nicklist_in_channellist (from, channel, 0);
		}
	}
	else
	{
		message_from (from, LOG_MSG);
		flood_type = MSG_FLOOD;
		if (my_stricmp (to, get_server_nickname (from_server)))
		{
			log_type = LOG_WALL;
			ignore_type = IGNORE_WALLS;
			list_type = MSG_GROUP_LIST;
		}
		else
		{
			log_type = LOG_MSG;
			ignore_type = IGNORE_MSGS;
			list_type = MSG_LIST;
		}
	}
	switch (check_ignore (from, FromUserHost, to, ignore_type, ptr))
	{
	case IGNORED:
		if ((list_type == MSG_LIST) && get_int_var (SEND_IGNORE_MSG_VAR))
			send_to_server ("NOTICE %s :%s is ignoring you", from, get_server_nickname (from_server));
		doing_privmsg = 0;
		return;
	case HIGHLIGHTED:
		high = highlight_char;
		break;
	case CHANNEL_GREP:
		high = highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}

	ptr = do_ctcp (from, to, ptr);
	if (!ptr || !*ptr)
	{
		doing_privmsg = 0;
		return;
	}

	level = set_lastlog_msg_level (log_type);
	com_do_log = 0;
	if (flood_type == PUBLIC_FLOOD)
	{
		int blah = 0;
		if (is_other_flood (channel, tmpnick, PUBLIC_FLOOD, &blah))
		{
			no_flood = 0;
			flood_prot (tmpnick->nick, FromUserHost, "PUBLIC", flood_type, get_int_var (PUBFLOOD_TIME_VAR), channel->channel);
		}
	}
	else
		no_flood = check_flooding (from, flood_type, ptr, NULL);

	if (sed == 1)
	{
		if (do_hook (ENCRYPTED_PRIVMSG_LIST, "%s %s %s", from, to, ptr))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_ENCRYPTED_PRIVMSG_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ptr));
		sed = 0;
	}
	else
	{
		if (list_type == PUBLIC_LIST || list_type == PUBLIC_OTHER_LIST || list_type == PUBLIC_MSG_LIST)
		{
			if (check_auto_reply (ptr))
			{
				com_do_log = 1;
				com_lines = 0;
				ar_true = 1;
			}
		}
		switch (list_type)
		{
		case PUBLIC_MSG_LIST:
			logmsg (LOG_PUBLIC, from, ptr, 0);
			if (no_flood && do_hook (list_type, "%s %s %s", from, to, ptr))
				put_it ("%s", convert_output_format (get_fset_var (ar_true ? FORMAT_PUBLIC_MSG_AR_FSET : FORMAT_PUBLIC_MSG_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, get_int_var (MIRCS_VAR) ? mircansi (ptr) : ptr));
			break;
		case MSG_GROUP_LIST:
			logmsg (LOG_PUBLIC, from, ptr, 0);
			if (no_flood && do_hook (list_type, "%s %s %s", from, to, ptr))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_MSG_GROUP_FSET), "%s %s %s %s", update_clock (GET_TIME), from, to, get_int_var (MIRCS_VAR) ? mircansi (ptr) : ptr));
			break;
		case MSG_LIST:
			{
				if (!no_flood)
					break;
				malloc_strcpy (&recv_nick, from);
				if (away_set)
				{
					do_beep = 0;
					beep_em (get_int_var (BEEP_WHEN_AWAY_VAR));
					set_int_var (MSGCOUNT_VAR, get_int_var (MSGCOUNT_VAR) + 1);
				}
				logmsg (LOG_MSG, from, ptr, 0);
				addtabkey (from, "msg");

				if (do_hook (list_type, "%s %s", from, ptr))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_MSG_FSET), "%s %s %s %s", update_clock (GET_TIME), from, FromUserHost, get_int_var (MIRCS_VAR) ? mircansi (ptr) : ptr));

				if (from_server > -1 && get_server_away (from_server) && get_int_var (SEND_AWAY_MSG_VAR))
				{
					my_send_to_server (from_server, "NOTICE %s :%s", from, stripansicodes (convert_output_format (get_fset_var (FORMAT_SEND_AWAY_FSET), "%l %l %s", time (NULL), server_list[from_server].awaytime, get_int_var (MSGLOG_VAR) ? "On" : "Off")));
				}
				break;
			}
		case PUBLIC_LIST:
			{
				if (ar_true)
					list_type = AR_PUBLIC_LIST;
				logmsg (LOG_PUBLIC, from, ptr, 0);
				if (no_flood && do_hook (list_type, "%s %s %s", from, to, ptr))
					put_it ("%s", convert_output_format (get_fset_var ((list_type == AR_PUBLIC_LIST) ? FORMAT_PUBLIC_AR_FSET : FORMAT_PUBLIC_FSET), "%s %s %s %s", update_clock (GET_TIME), from, to, get_int_var (MIRCS_VAR) ? mircansi (ptr) : ptr));
				break;
			}
		case PUBLIC_OTHER_LIST:
			{
				logmsg (LOG_PUBLIC, from, ptr, 0);
				if (ar_true)
					list_type = AR_PUBLIC_OTHER_LIST;

				if (no_flood && do_hook (list_type, "%s %s %s", from, to, ptr))
					put_it ("%s", convert_output_format (get_fset_var (list_type == AR_PUBLIC_OTHER_LIST ? FORMAT_PUBLIC_OTHER_AR_FSET : FORMAT_PUBLIC_OTHER_FSET), "%s %s %s %s", update_clock (GET_TIME), from, to, get_int_var (MIRCS_VAR) ? mircansi (ptr) : ptr));
				break;
			}	/* case */
		}		/* switch */
	}
	if (beep_on_level & log_type && do_beep)
		beep_em (1);
	set_lastlog_msg_level (level);
	message_from (NULL, LOG_CRAP);
	doing_privmsg = 0;
}

static void 
p_quit (char *from, char **ArgList)
{
	int one_prints = 0;
	char *chan = NULL;
	char *Reason;
	int netsplit = 0;
	int ignore;


	PasteArgs (ArgList, 0);
	if (ArgList[0])
	{
		Reason = ArgList[0];
		netsplit = check_split (from, Reason, chan);
	}
	else
		Reason = "?";

	ignore = check_ignore (from, FromUserHost, NULL, (netsplit ? IGNORE_SPLITS : IGNORE_QUITS) | IGNORE_ALL, NULL);
	for (chan = walk_channels (from, 1, from_server); chan; chan = walk_channels (from, 0, -1))
	{
		if (ignore != IGNORED)
		{
			message_from (chan, LOG_CRAP);
			if (do_hook (CHANNEL_SIGNOFF_LIST, "%s %s %s", chan, from, Reason))
				one_prints = 1;
			message_from(NULL, LOG_CURRENT);
		}
	}
	if (one_prints)
	{
		chan = what_channel (from, from_server);

		ignore = check_ignore (from, FromUserHost, chan, (netsplit ? IGNORE_SPLITS : IGNORE_QUITS) | IGNORE_ALL, NULL);

		message_from(chan, LOG_CRAP); 
		if ((ignore != IGNORED) && do_hook (SIGNOFF_LIST, "%s %s", from, Reason) && !netsplit)
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_CHANNEL_SIGNOFF_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, chan, Reason));
		message_from(NULL, LOG_CURRENT);
	}
	if ( !netsplit )
		check_orig_nick(from);
	notify_mark (from, NULL, NULL, 0);
	remove_from_channel (NULL, from, from_server, netsplit, Reason);
	message_from (NULL, LOG_CRAP);
	update_all_status (curr_scr_win, NULL, 0);
}

static void 
p_pong (char *from, char **ArgList)
{
	int is_server = 0;
	int i;

	if (!ArgList[0])
		return;
	is_server = match ("*.*", ArgList[0]);
	if (in_server_ping && is_server);
	{
		int old_from_server = from_server;
		for (i = 0; i < number_of_servers; i++)
		{
			if ((!my_stricmp (ArgList[0], get_server_name (i)) || !my_stricmp (ArgList[0], get_server_itsname (i))) && is_server_connected (i))
			{
				int old_lag = server_list[i].lag;
				from_server = i;
				server_list[i].lag = time (NULL) - server_list[i].lag_time;
				in_server_ping--;
				if (old_lag != server_list[i].lag)
					status_update (1);
				from_server = old_from_server;
				return;
			}
		}
		from_server = old_from_server;
	}
	if (check_ignore (from, FromUserHost, NULL, IGNORE_PONGS | IGNORE_CRAP, NULL) != IGNORED)
	{
		if (!is_server)
			return;
		if (!ArgList[1])
			say ("%s: PONG received from %s", ArgList[0], from);
		else
			say ("%s: PING received from %s %s", ArgList[0], from, ArgList[1]);
	}
}


static void 
p_error (char *from, char **ArgList)
{

	PasteArgs (ArgList, 0);
	if (!ArgList[0])
	{
		fake ();
		return;
	}
	say ("%s", ArgList[0]);
}

static void 
p_channel (char *from, char **ArgList)
{
	char *channel;
	char *user, *host;
	ChannelList *chan = NULL;
	WhowasList *whowas = NULL;
	int its_me = 0;

	if (!strcmp (ArgList[0], "0"))
	{
		fake ();
		return;
	}

	channel = ArgList[0];
	message_from (channel, LOG_CRAP);
	malloc_strcpy (&joined_nick, from);

	if (!my_stricmp (from, get_server_nickname (from_server)))
	{
		int refnum;
		Window *old_window = curr_scr_win;
		int switched = 0;
		if (!in_join_list (channel, from_server))
			add_to_join_list (channel, from_server, curr_scr_win->refnum);
		else
		{

			if (curr_scr_win->refnum != (refnum = get_win_from_join_list (channel, from_server)))
			{
				switched = 1;
				set_current_window (get_window_by_refnum (refnum));
			}
		}

		if (*channel != '+')
			send_to_server ("MODE %s\r\nMODE %s b", channel, channel, channel);

		(void) do_hook (JOIN_ME_LIST, "%s", channel);
		its_me = 1;
		chan = add_channel (channel, from_server);
		if (*channel == '+')
		{
			got_info (channel, from_server, GOTBANS);
			got_info (channel, from_server, GOTMODE);
		}
		if (switched)
			set_current_window (old_window);
	}
	else
	{
		int op = 0, vo = 0;
		char *c;

		/*
		 * Workaround for gratuitous protocol change in ef2.9
		 */
		if ((c = strchr (channel, '\007')))
		{
			for (*c++ = 0; *c; c++)
			{
				if (*c == 'o')
					op = 1;
				else if (*c == 'v')
					vo = 1;
			}
		}

		chan = add_to_channel (channel, from, from_server, op, vo, FromUserHost, NULL, NULL);
	}

	flush_mode_all (chan);

	user = m_strdup (FromUserHost);
	host = strchr (user, '@');
	*host++ = '\0';

	if (check_ignore (from, FromUserHost, channel, IGNORE_JOINS | IGNORE_CRAP, NULL) != IGNORED && chan)
	{
		irc_server *irc_serv = NULL;
		char *tmp2 = NULL;



		if (get_int_var (AUTO_NSLOOKUP_VAR) && isdigit (*(host + strlen (host) - 1)))
			tmp2 = do_nslookup (host);
		message_from (channel, LOG_CRAP);
		if ((whowas = check_whosplitin_buffer (from, FromUserHost, channel, 0)) && (irc_serv = check_split_server (whowas->server1)))
		{
			if (do_hook (LLOOK_JOIN_LIST, "%s %s", irc_serv->name, irc_serv->link))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETJOIN_FSET), "%s %s %s %d", update_clock (GET_TIME), irc_serv->name, irc_serv->link, 0));
			remove_split_server (whowas->server1);
		}
		if (do_hook (JOIN_LIST, "%s %s %s", from, channel, tmp2 ? tmp2 : FromUserHost ? FromUserHost : "UnKnown"))
		{
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_JOIN_FSET), "%s %s %s %s", update_clock (GET_TIME), from, tmp2 ? tmp2 : FromUserHost ? FromUserHost : "UnKnown", channel));
		}
		message_from (NULL, LOG_CRAP);
	}
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
	update_all_status (curr_scr_win, NULL, 0);
	notify_mark (from, user, host, 1);
	new_free (&user);
}

static void 
p_invite (char *from, char **ArgList)
{
	char *high;

	switch (check_ignore (from, FromUserHost, ArgList[1] ? ArgList[1] : NULL, IGNORE_INVITES, NULL))
	{
	case IGNORED:
		if (get_int_var (SEND_IGNORE_MSG_VAR))
			send_to_server ("NOTICE %s :%s is ignoring you",
				   from, get_server_nickname (from_server));
		return;
	case HIGHLIGHTED:
		high = highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}
	if (ArgList[0] && ArgList[1])
	{
		ChannelList *chan = NULL;
		WhowasChanList *w_chan = NULL;

		message_from (from, LOG_CRAP);
		malloc_strcpy (&invite_channel, ArgList[1]);
		if (check_flooding (from, INVITE_FLOOD, ArgList[1], NULL) &&
		    do_hook (INVITE_LIST, "%s %s", from, ArgList[1]))
		{
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_INVITE_FSET), "%s %s %s", update_clock (GET_TIME), from, ArgList[1]));
		}
		if (!(chan = lookup_channel (invite_channel, from_server, 0)))
			if ((w_chan = check_whowas_chan_buffer (invite_channel, 0)))
				chan = w_chan->channellist;
		if (chan && get_int_var (AUTO_REJOIN_VAR) && invite_channel)
		{
			if (!in_join_list (invite_channel, from_server))
				send_to_server ("JOIN %s %s", invite_channel, ArgList[2] ? ArgList[2] : "");
		}

		malloc_strcpy (&recv_nick, from);
	}
}

static void 
p_silence (char *from, char **ArgList)
{
	char *target = ArgList[0];
	char *mag = target++;

	if (do_hook (SILENCE_LIST, "%c %s", *mag, target))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_SILENCE_FSET), "%s %c %s", update_clock (GET_TIME), *mag, target));
}


static void 
p_kill (char *from, char **ArgList)
{
	char sc[20];
	int port;
	int local = 0;


	port = get_server_port (from_server);
	if (ArgList[1] && strstr (ArgList[1], get_server_name (from_server)))
		if (!strchr (from, '.'))
			local = 1;
	snprintf (sc, 19, "+%i %d", from_server, port);

	close_server (from_server, empty_string);
	clean_whois_queue ();
	window_check_servers ();
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);

	if (strchr (from, '.'))
	{
		say ("Server [%s] has rejected you (probably due to a nick collision)", from);
		t_parse_command ("SERVER", NULL);
	}
	else
	{
		if (local && get_int_var (NEXT_SERVER_ON_LOCAL_KILL_VAR))
		{
			int i = from_server + 1;
			if (i >= number_of_servers)
				i = 0;
			snprintf (sc, 19, "+%i", i);
			from_server = -1;
		}
		if (do_hook (DISCONNECT_LIST, "Killed by %s (%s)", from,
			     ArgList[1] ? ArgList[1] : "(No Reason Given)"))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_KILL_FSET), "%s %s", update_clock (GET_TIME), ArgList[1] ? ArgList[1] : "You have been Killed"));
		if (get_int_var (AUTO_RECONNECT_VAR))
			t_parse_command ("SERVER", NULL);
	}
	update_all_status (curr_scr_win, NULL, 0);
}

static void 
p_ping (char **ArgList)
{

	PasteArgs (ArgList, 0);
	send_to_server ("PONG %s", ArgList[0]);
}

static void 
p_nick (char *from, char **ArgList)
{
	int one_prints = 0, ign = 0, its_me = 0;
	ChannelList *chan;
	char *line;


	line = ArgList[0];
	if (!my_stricmp (from, get_server_nickname (from_server)))
	{
		accept_server_nickname (from_server, line);
		its_me = 1;
		user_changing_nickname = 0;
	}
	ign = check_ignore (from, FromUserHost, NULL, IGNORE_NICKS | IGNORE_CRAP, NULL);
	for (chan = server_list[from_server].chan_list; chan; chan = chan->next)
	{
		message_from (chan->channel, LOG_CRAP);
		if (ign != IGNORED && do_hook (CHANNEL_NICK_LIST, "%s %s %s", chan->channel, from, line))
			one_prints = 1;
	}
	if (one_prints)
	{
		if (its_me)
		{
			message_from (NULL, LOG_CRAP);
		}
		else
			message_from (what_channel (from, from_server), LOG_CRAP);
		if (ign != IGNORED && do_hook (NICKNAME_LIST, "%s %s", from, line))
			put_it ("%s", convert_output_format (get_fset_var (its_me ? FORMAT_NICKNAME_USER_FSET : im_on_channel (what_channel (from, from_server)) ? FORMAT_NICKNAME_FSET : FORMAT_NICKNAME_OTHER_FSET), "%s %s %s %s", update_clock (GET_TIME), from, "-", line));
	}
	rename_nick (from, line, from_server);
	if (!its_me)
	{
		char *user, *host;

		user = m_strdup (FromUserHost);
		host = strchr (user, '@');
		*host++ = '\0';

		notify_mark (from, user, host, 0);
		notify_mark (line, user, host, 1);
		new_free (&user);
	}
}

static void 
p_mode (char *from, char **ArgList)
{
	char *channel;
	char *line;
	int flag;

	ChannelList *chan = NULL;
	char buffer[BIG_BUFFER_SIZE + 1];
	char *smode;
#ifdef COMPRESS_MODES
	char *tmpbuf = NULL;
#endif
	PasteArgs (ArgList, 1);
	channel = ArgList[0];
	line = ArgList[1];
	smode = strchr (from, '.');

	flag = check_ignore (from, FromUserHost, channel, (smode ? IGNORE_SMODES : IGNORE_MODES) | IGNORE_CRAP, NULL);

	message_from (channel, LOG_CRAP);
	if (channel && line)
	{
		strcpy (buffer, line);
		if (get_int_var (MODE_STRIPPER_VAR))
			strip_modes (from, channel, line);
		if (is_channel (channel))
		{
#ifdef COMPRESS_MODES
			chan = (ChannelList *) find_in_list ((List **) & server_list[from_server].chan_list, channel, 0);
			if (get_int_var (COMPRESS_MODES_VAR))
			{
				tmpbuf = compress_modes (from_server, channel, line);
				if (tmpbuf)
					strcpy (line, tmpbuf);
				else
					goto end_p_mode;
			}
#endif
			/* CDE handle mode protection here instead of later */
			update_channel_mode (from, channel, from_server, buffer, chan);

			if (flag != IGNORED && do_hook (MODE_LIST, "%s %s %s", from, channel, line))
				put_it ("%s", convert_output_format (get_fset_var (smode ? FORMAT_SMODE_FSET : FORMAT_MODE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, smode ? "*" : FromUserHost, channel, line));
		}
		else
		{
			if (flag != IGNORED && do_hook (MODE_LIST, "%s %s %s", from, channel, line))
			{
				if (!my_stricmp (from, channel))
				{
					if (!my_stricmp (from, get_server_nickname (from_server)))
						put_it ("%s", convert_output_format (get_fset_var (FORMAT_USERMODE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, "*", channel, line));
					else
						put_it ("%s", convert_output_format (get_fset_var (FORMAT_USERMODE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, smode ? "*" : FromUserHost, channel, line));
				}
				else
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_MODE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, smode ? "*" : FromUserHost, channel, line));
			}
			update_user_mode (line);
		}
#ifdef COMPRESS_MODES
	      end_p_mode:
#endif
		update_all_status (curr_scr_win, NULL, 0);
	}
	message_from (NULL, LOG_CRAP);
}

static void 
strip_modes (char *from, char *channel, char *line)
{
	char *mode;
	char *pointer;
	char mag = '+';		/* XXXX Bogus */
	char *copy = NULL;
#ifdef __GNUC__
	char free_copy[strlen (line) + 1];
	strcpy (free_copy, line);
#else
	char *free_copy = NULL;
	malloc_strcpy (&free_copy, line);
#endif

	copy = free_copy;
	mode = next_arg (copy, &copy);
	if (is_channel (channel))
	{
		for (pointer = mode; *pointer; pointer++)
		{
			char c = *pointer;
			switch (c)
			{
			case '+':
			case '-':
				mag = c;
				break;
			case 'l':
				if (mag == '+')
					do_hook (MODE_STRIPPED_LIST, "%s %s %c%c %s",
						 from, channel, mag, c, next_arg (copy, &copy));
				else
					do_hook (MODE_STRIPPED_LIST, "%s %s %c%c",
						 from, channel, mag, c);
				break;
			case 'a':
			case 'i':
			case 'm':
			case 'n':
			case 'p':
			case 's':
			case 't':
				do_hook (MODE_STRIPPED_LIST, "%s %s %c%c", from,
					 channel, mag, c);
				break;
			case 'b':
			case 'k':
			case 'o':
			case 'v':
				do_hook (MODE_STRIPPED_LIST, "%s %s %c%c %s", from,
				   channel, mag, c, next_arg (copy, &copy));
				break;
			}
		}
	}
	else
		/* User mode */
	{
		for (pointer = mode; *pointer; pointer++)
		{
			char c = *pointer;
			switch (c)
			{
			case '+':
			case '-':
				mag = c;
				break;
			default:
				do_hook (MODE_STRIPPED_LIST, "%s %s %c%c", from, channel, mag, c);
				break;
			}
		}
	}
#ifndef __GNUC__
	new_free (&free_copy);
#endif
}

static void 
p_kick (char *from, char **ArgList)
{
	char *channel, *who, *comment;
	char *chankey = NULL;
	ChannelList *chan = NULL;
	NickList *tmpnick = NULL;
	int t = 0;


	channel = ArgList[0];
	who = ArgList[1];
	comment = ArgList[2] ? ArgList[2] : "(no comment)";

	if ((chan = lookup_channel (channel, from_server, CHAN_NOUNLINK)))
		tmpnick = find_nicklist_in_channellist (from, chan, 0);
	message_from (channel, LOG_CRAP);
	if (channel && who && chan)
	{
		if (!my_stricmp (who, get_server_nickname (from_server)))
		{
			Window *window = chan->window;

			if (chan->key)
				malloc_strcpy (&chankey, chan->key);
			if (get_int_var (AUTO_REJOIN_VAR))
			{
				send_to_server ("JOIN %s %s", channel, chankey ? chankey : empty_string);
				add_to_join_list (channel, from_server, window ? window->refnum : 0);
			}
			new_free (&chankey);
			remove_channel (channel, from_server);
			update_all_status (curr_scr_win, NULL, 0);
			update_input (UPDATE_ALL);
			if (do_hook (KICK_LIST, "%s %s %s %s", who, from, channel, comment ? comment : empty_string))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_KICK_USER_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, channel, who, comment));
		}
		else
		{
			int itsme = 0;
			itsme = !my_stricmp (get_server_nickname (from_server), from) ? 1 : 0;

			if ((check_ignore (from, FromUserHost, channel, IGNORE_KICKS | IGNORE_CRAP, NULL) != IGNORED) &&
			    do_hook (KICK_LIST, "%s %s %s %s", who, from, channel, comment))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_KICK_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, channel, who, comment));
			if (!itsme)
			{
				NickList *f_nick = NULL;
				f_nick = find_nicklist_in_channellist (who, chan, 0);
				if (chan->chop && tmpnick && is_other_flood (chan, tmpnick, KICK_FLOOD, &t))
				{
					if (get_int_var (KICK_ON_KICKFLOOD_VAR) > get_int_var (DEOP_ON_KICKFLOOD_VAR))
						send_to_server ("MODE %s -o %s", chan->channel, from);
					else if (!f_nick->kickcount++)
						send_to_server ("KICK %s %s :\002Mass kick detected - (%d kicks in %dsec%s)\002", chan->channel, from, get_int_var (KICK_ON_KICKFLOOD_VAR), t, plural (t));
				}
			}
			remove_from_channel (channel, who, from_server, 0, NULL);
		}

	}
	update_all_status (curr_scr_win, NULL, 0);
	message_from (NULL, LOG_CRAP);
}

static void 
p_part (char *from, char **ArgList)
{
	char *channel;

	if (!from || !*from)
		return;
	channel = ArgList[0];

	PasteArgs (ArgList, 1);
	message_from (channel, LOG_CRAP);
	in_on_who = 1;

	if ((check_ignore (from, FromUserHost, channel, IGNORE_PARTS | IGNORE_CRAP, NULL) != IGNORED) &&
	    do_hook (LEAVE_LIST, "%s %s %s %s", from, channel, FromUserHost, ArgList[1] ? ArgList[1] : empty_string))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_LEAVE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, channel, ArgList[1] ? ArgList[1] : empty_string));
	if (!my_stricmp (from, get_server_nickname (from_server)))
	{
		remove_channel (channel, from_server);
		remove_from_mode_list (channel, from_server);
		remove_from_join_list (channel, from_server);
		set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
	}
	else
	{
		remove_from_channel (channel, from, from_server, 0, NULL);
	}
	update_all_status (curr_scr_win, NULL, 0);
	update_input (UPDATE_ALL);
	message_from (NULL, LOG_CRAP);
	in_on_who = 0;
}

static void 
p_odd (char *from, char *comm, char **ArgList)
{
	PasteArgs (ArgList, 0);
	if (do_hook (ODD_SERVER_STUFF_LIST, "%s %s %s", from ? from : "*", comm, ArgList[0]))
	{
		if (from)
			say ("Odd server stuff: \"%s %s\" (%s)", comm, ArgList[0], from);
		else
			say ("Odd server stuff: \"%s %s\"", comm, ArgList[0]);
	}
}

void 
parse_server (char *line)
{
	char *from, *comm, *end;
	int numeric;
	char **ArgList;
	char copy[BIG_BUFFER_SIZE + 1];
	char *TrueArgs[MAXPARA + 1] =
	{NULL};


	if (!line || !*line)
		return;

	end = strlen (line) + line;
	if (*--end == '\n')
		*end-- = '\0';
	if (*end == '\r')
		*end-- = '\0';

	if (!line || !*line)
		return;
	if (*line == ':')
	{
		if (!do_hook (RAW_IRC_LIST, "%s", line + 1))
			return;
	}
	else if (!do_hook (RAW_IRC_LIST, "* %s", line))
		return;

	ArgList = TrueArgs;
	strncpy (copy, line, BIG_BUFFER_SIZE);
	FixColorAnsi (line);
	BreakArgs (line, &from, ArgList, 0);

	if (!(comm = (*ArgList++)) || !from || !*ArgList)
		return;		/* Empty line from server - ByeBye */

	/* 
	 * I reformatted these in may '96 by using the output of /stats m
	 * from a few busy servers.  They are arranged so that the most 
	 * common types are high on the list (to save the average number
	 * of compares.)  I will be doing more testing in the future on
	 * a live client to see if this is a reasonable order.
	 */
	if ((numeric = atoi (comm)))
		numbered_command (from, numeric, ArgList);

	/* There are the core msgs for most non-numeric traffic. */
	else if (!strcmp (comm, "PRIVMSG"))
		p_privmsg (from, ArgList);
	else if (!strcmp (comm, "JOIN"))
		p_channel (from, ArgList);
	else if (!strcmp (comm, "PART"))
		p_part (from, ArgList);
	else if (!strcmp (comm, "MODE"))
		p_mode (from, ArgList);
	else if (!strcmp (comm, "QUIT"))
		p_quit (from, ArgList);
	else if (!strcmp (comm, "NOTICE"))
		parse_notice (from, ArgList);
	else if (!strcmp (comm, "NICK"))
		p_nick (from, ArgList);
	else if (!strcmp (comm, "TOPIC"))
		p_topic (from, ArgList);
	else if (!strcmp (comm, "KICK"))
		p_kick (from, ArgList);
	else if (!strcmp (comm, "INVITE"))
		p_invite (from, ArgList);

	/* These are used, but not nearly as much as ones above */
	else if (!strcmp (comm, "WALLOPS"))
		p_wallops (from, ArgList);
	else if (!strcmp (comm, "ERROR"))
		p_error (from, ArgList);
	else if (!strcmp (comm, "ERROR:"))
		p_error (from, ArgList);
	else if (!strcmp (comm, "SILENCE"))
		p_silence (from, ArgList);
	else if (!strcmp (comm, "KILL"))
		p_kill (from, ArgList);
	else if (!strcmp (comm, "PONG"))
		p_pong (from, ArgList);
	else if (!strcmp (comm, "PING"))
		p_ping (ArgList);

	/* Some kind of unrecognized/unsupported command */
	else
		p_odd (from, comm, ArgList);
	from_server = -1;
}
