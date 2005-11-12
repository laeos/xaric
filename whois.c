#ident "@(#)whois.c 1.7"
/*
 * whois.c: Some tricky routines for querying the server for information
 * about a nickname using WHOIS.... all the time hiding this from the user.  
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#undef MONITOR_Q		/* this one is for monitoring of the 'whois queue' (debug) */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "whois.h"
#include "hook.h"
#include "lastlog.h"
#include "vars.h"
#include "server.h"
#include "ignore.h"
#include "ircaux.h"
#include "notify.h"
#include "numbers.h"
#include "window.h"
#include "commands.h"
#include "output.h"
#include "parse.h"
#include "ctcp.h"
#include "misc.h"
#include "status.h"
#include "whowas.h"
#include "struct.h"
#include "fset.h"

char whois_nick[] = "#WHOIS#";

/* current setting for BEEP_ON_MSG */
static int ignore_whois_crap = 0;
static int eat_away = 0;

/* WQ_head and WQ_tail point to the head and tail of the whois queue */
WhoisQueue *WQ_head = NULL;
WhoisQueue *WQ_tail = NULL;

static char show_away_flag = 0;

void typed_add_to_whois_queue (int, char *, void (*)(WhoisStuff *, char *, char *), char *,...);
static char *whois_queue_head (int);
static int whois_type_head (int);
static void (*whois_func_head (int)) (WhoisStuff *, char *, char *);
static WhoisQueue *remove_from_whois_queue (int);

/*
 * whois_queue_head: returns the nickname at the head of the whois queue, or
 * NULL if the queue is empty.  It does not modify the queue in any way. 
 */
static char *
whois_queue_head (int server_index)
{
	if ((WQ_head = (WhoisQueue *) get_server_qhead (server_index)) != NULL)
		return (WQ_head->nick);
	else
		return (NULL);
}

static int 
whois_type_head (int server_index)
{
	if ((WQ_head = (WhoisQueue *) get_server_qhead (server_index)) != NULL)
		return (WQ_head->type);
	else
		return -1;
}

static void (*whois_func_head (int server_index)) ()
{
	if ((WQ_head = (WhoisQueue *) get_server_qhead (server_index)) != NULL)
		return ((void *) WQ_head->func);
	else
		return NULL;
}

/*
 * remove_from_whois_queue: removes the top element of the whois queue and
 * returns the element as its function value.  This routine repairs handles
 * all queue stuff, but the returned element is mallocd and must be freed by
 * the calling routine 
 */
static WhoisQueue *
remove_from_whois_queue (int server_index)
{
	WhoisQueue *new;

	new = (WhoisQueue *) get_server_qhead (server_index);
	set_server_qhead (server_index, new->next);
	if (new->next == NULL)
		set_server_qtail (server_index, NULL);
	return (new);
}

/*
 * clean_whois_queue: this empties out the whois queue.  This is used after
 * server reconnection to assure that no bogus entries are left in the whois
 * queue 
 */
void 
clean_whois_queue (void)
{
	WhoisQueue *thing;

	while (whois_queue_head (from_server))
	{
		thing = remove_from_whois_queue (from_server);
		new_free (&thing->nick);
		new_free (&thing->text);
		new_free ((char **) &thing);
	}
	ignore_whois_crap = 0;
	eat_away = 0;
}

/* ison_returned: this is called when numeric 303 is received in
 * numbers.c. ISON must always be the property of the WHOIS queue.
 * Although we will check first that the top element expected is
 * actually an ISON.
 */
void 
ison_returned (char *from, char **ArgList)
{
	WhoisQueue *thing;

	if (whois_type_head (from_server) == WHOIS_ISON)
	{
		thing = remove_from_whois_queue (from_server);
		thing->func (thing->nick, ArgList[0] ? ArgList[0] : empty_str);
		new_free (&thing->nick);
		new_free (&thing->text);
		new_free ((char **) &thing);
	}
	else
		ison_now (NULL, ArgList[0] ? ArgList[0] : empty_str);
}

/* userhost_returned: this is called when numeric 302 is received in
 * numbers.c. USERHOST must also remain the property of the WHOIS
 * queue. Sending it without going via the WHOIS queue will cause
 * the queue to become corrupted.
 *
 * While USERHOST gives us similar information to WHOIS, this routine's
 * format is a little different to those that handle the WHOIS numerics.
 * A list of nicks can be supplied to USERHOST, and any of those
 * nicks are not currently signed on, it will just omit reporting it.
 * this means we must go through the list of nicks returned and check
 * them for missing entries. This means stepping through the requested
 * nicks one at a time.
 *
 * A side effect of this is that user initiated USERHOST requests get
 * reported as separate replies even though one request gets made and
 * only one reply is actually received. This should make it easier for
 * people wanting to use USERHOST for their own purposes.
 *
 * In this routine, cnick points to all the information in the next
 * entry from the returned data.
 */
void 
userhost_returned (char *from, char **ArgList)
{
	WhoisQueue *thing;
	WhoisStuff *whois_stuff = NULL;
	char *nick = NULL, *cnick = NULL, *tnick = NULL;
	char *user = NULL, *cuser = NULL;
	char *host = NULL, *chost = NULL;

	int ishere, cishere = 1;
	int isoper, cisoper = 0;
	int noton, isuser, parsed;

	char *queue_nicks = NULL;

	if (!ArgList[0] || *ArgList[0] == '\0')
		return;

	if (whois_type_head (from_server) == WHOIS_USERHOST)
	{
		isuser = ((void *) whois_func_head (from_server) == USERHOST_USERHOST);
		whois_stuff = get_server_whois_stuff (from_server);
		thing = remove_from_whois_queue (from_server);
		queue_nicks = thing->nick;
		if (*queue_nicks == ' ')
			++queue_nicks;
	}
	else
	{
		isuser = 1;
		thing = NULL;
		queue_nicks = NULL;
		whois_stuff = NULL;
	}
	parsed = 0;
	while ((parsed || (cnick = next_arg (ArgList[0], ArgList))) || queue_nicks)
	{
		if (queue_nicks)
		{
			tnick = next_arg (queue_nicks, &queue_nicks);
			if (!*queue_nicks)
				queue_nicks = NULL;
		}
		else
			tnick = NULL;
		if (cnick && !parsed)
		{
			if (!(cuser = strchr (cnick, '=')))
				break;
			if (*(cuser - 1) == '*')
			{
				*(cuser - 1) = '\0';
				cisoper = 1;
			}
			else
				cisoper = 0;
			*cuser++ = '\0';
			if (*cuser++ == '+')
				cishere = 1;
			else
				cishere = 0;
			if (!(chost = strchr (cuser, '@')))
				break;
			*chost++ = '\0';
			parsed = 1;
		}
		if (!cnick && thing)
		{
			if (!tnick && !queue_nicks)	/* ever happen? */
			{
				say ("tnick and queue_nick are both NULL, tell rex!");
				goto out_free;
			}
			if (tnick)
			{
				if (queue_nicks)
					*--queue_nicks = ' ';
				queue_nicks = tnick;
				memcpy (thing->nick, queue_nicks, strlen (queue_nicks) + 1);
			}

			thing->next = get_server_qhead (from_server);
			set_server_qhead (from_server, thing);
			return;
		}
		if (tnick && my_stricmp (cnick, tnick))
		{
			if (tnick)
				nick = tnick;
			else
				nick = cnick;
			user = host = "<UNKNOWN>";
			isoper = 0;
			ishere = 1;
			noton = 1;
		}
		else
		{
			nick = cnick;
			user = cuser;
			host = chost;
			isoper = cisoper;
			ishere = cishere;
			noton = parsed = 0;
		}
		if (!isuser)
		{
			malloc_strcpy (&whois_stuff->nick, nick);
			malloc_strcpy (&whois_stuff->user, user);
			malloc_strcpy (&whois_stuff->host, host);
			whois_stuff->oper = isoper;
			whois_stuff->not_on = noton;
			if (!ishere)
				malloc_strcpy (&whois_stuff->away, empty_str);
			else
				new_free (&whois_stuff->away);
			thing->func (whois_stuff, tnick, thing->text);
			new_free (&whois_stuff->away);
		}
		else
		{
			char *nsl = NULL;
			if (get_int_var (AUTO_NSLOOKUP_VAR) && host && isdigit (*(host + strlen (host) - 1)))
				nsl = do_nslookup (host);

			if (do_hook (current_numeric, "%s %s %s %s %s", nick,
				     isoper ? "+" : "-", ishere ? "+" : "-", user, nsl ? nsl : host))
			{
				put_it ("%s %s is %s@%s%s%s", line_thing,
				      nick, user, nsl ? nsl : host, isoper ?
				     " (Is an IRC operator)" : empty_str,
					ishere ? empty_str : " (away)");
			}
		}
	}
	if (thing)
	{
	      out_free:new_free (&thing->nick);
		new_free (&thing->text);
		new_free ((char **) &thing);
	}
}

/*
 * whois_name: routine that is called when numeric 311 is received in
 * numbers.c. This routine parses out the information in the numeric and
 * saves it until needed (see whois_server()).  If the whois queue is empty,
 * this routine simply displays the data in the normal fashion.  Why would
 * the queue ever be empty, you ask? If the user does a "WHOIS *" or any
 * other whois with a wildcard, you will get multiple returns from the
 * server.  So, instead of attempting to handle each of these, only the first
 * is handled, and the others fall through.  It is up to the programmer to
 * prevent wildcards from interfering with what they want done.  See
 * channel() in edit.c 
 */
void 
whois_name (char *from, char **ArgList)
{
	char *nick, *user, *host, *channel, *ptr, *name;

	WhoisStuff *whois_stuff;

	PasteArgs (ArgList, 4);
	nick = ArgList[0];
	user = ArgList[1];
	host = ArgList[2];
	channel = ArgList[3];
	name = ArgList[4];
	if (!nick || !user || !host || !channel || !name)
		return;

	whois_stuff = get_server_whois_stuff (from_server);
	if ((ptr = whois_queue_head (from_server)) &&
	    (whois_type_head (from_server) == WHOIS_WHOIS) &&
	    (my_stricmp (ptr, nick) == 0))
	{
		malloc_strcpy (&whois_stuff->nick, nick);
		malloc_strcpy (&whois_stuff->user, user);
		malloc_strcpy (&whois_stuff->host, host);
		malloc_strcpy (&whois_stuff->name, name);
		malloc_strcpy (&whois_stuff->channel, channel);
		new_free (&whois_stuff->away);
		whois_stuff->oper = 0;
		whois_stuff->chop = 0;
		whois_stuff->not_on = 0;
		ignore_whois_crap = 1;
		eat_away = 1;
	}
	else
	{
		char *nsl = NULL;
		ignore_whois_crap = 0;
		eat_away = 0;

		if (host && isdigit (*(host + strlen (host) - 1)) && get_int_var (AUTO_NSLOOKUP_VAR))
			nsl = do_nslookup (host);

		message_from (NULL, LOG_CRAP);
		if (do_hook (current_numeric, "%s %s %s %s %s %s", from, nick,
			     user, nsl ? nsl : host, channel, name))
		{
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_HEADER_FSET), NULL));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_NICK_FSET), "%s %s %s %s", nick, user, nsl ? nsl : host, country (host)));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_NAME_FSET), "%s", name));
		}
	}
}

/*
 * whowas_name: same as whois_name() above but it is called with a numeric of
 * 314 when the user does a WHOWAS or when a WHOIS'd user is no longer on IRC 
 * and has set the AUTO_WHOWAS variable.
 */
void 
whowas_name (char *from, char **ArgList)
{
	char *nick, *user, *host, *channel, *ptr, *name;
	WhoisStuff *whois_stuff;
	int lastlog_level;

	PasteArgs (ArgList, 4);
	nick = ArgList[0];
	user = ArgList[1];
	host = ArgList[2];
	channel = ArgList[3];
	name = ArgList[4];
	if (!nick || !user || !host || !channel || !name)
		return;

	lastlog_level = set_lastlog_msg_level (LOG_CRAP);
	whois_stuff = get_server_whois_stuff (from_server);
	if ((ptr = whois_queue_head (from_server)) &&
	    (whois_type_head (from_server) & WHOIS_WHOIS) &&
	    (my_stricmp (ptr, nick) == 0))
	{
		malloc_strcpy (&whois_stuff->nick, nick);
		malloc_strcpy (&whois_stuff->user, user);
		malloc_strcpy (&whois_stuff->host, host);
		malloc_strcpy (&whois_stuff->name, name);
		malloc_strcpy (&whois_stuff->channel, channel);
		new_free (&whois_stuff->away);
		whois_stuff->oper = 0;
		whois_stuff->chop = 0;
		whois_stuff->not_on = 1;
		ignore_whois_crap = 1;
	}
	else
	{
		ignore_whois_crap = 0;
		message_from (NULL, LOG_CRAP);
		if (do_hook (current_numeric, "%s %s %s %s %s %s", from, nick,
			     user, host, channel, name))
		{
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOWAS_HEADER_FSET), NULL));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOWAS_NICK_FSET), "%s %s %s", nick, user, host));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_NAME_FSET), "%s", name));
		}
	}
	set_lastlog_msg_level (lastlog_level);
}

void 
whois_channels (char *from, char **ArgList)
{
	char *ptr;
	char *line;
	WhoisStuff *whois_stuff;

	PasteArgs (ArgList, 1);
	line = ArgList[1];
	whois_stuff = get_server_whois_stuff (from_server);
	if ((ptr = whois_queue_head (from_server)) &&
	    (whois_type_head (from_server) & WHOIS_WHOIS ) &&
	    whois_stuff->nick &&
	    (my_stricmp (ptr, whois_stuff->nick) == 0))
	{
		if (whois_stuff->channels == NULL)
			malloc_strcpy (&whois_stuff->channels, line);
		else
			malloc_strcat (&whois_stuff->channels, line);
	}
	else
	{
		message_from (NULL, LOG_CRAP);
		if (do_hook (current_numeric, "%s %s", from, line))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_CHANNELS_FSET), "%s", line));
	}
}

/*
 * whois_server: Called in numbers.c when a numeric of 312 is received.  If
 * all went well, this routine collects the needed information, pops the top
 * element off the queue and calls the function as described above in
 * WhoisQueue.  It then releases all the mallocd data.  If the queue is empty
 * (same case as described in whois_name() above), the information is simply
 * displayed in the normal fashion. Added a check to see if whois_stuff->nick
 * is NULL. This can happen if something is added to an empty whois queue
 * between the whois name being received and the server.
 */
void 
whois_server (char *from, char **ArgList)
{
	char *server, *ptr;
	char *line;
	WhoisStuff *whois_stuff;

	show_away_flag = 1;
	if (!ArgList[0] || !ArgList[1])
		return;
	if (ArgList[2])
	{
		server = ArgList[1];
		line = ArgList[2];
	}
	else
	{
		server = ArgList[0];
		line = ArgList[1];
	}
	whois_stuff = get_server_whois_stuff (from_server);
	if ((ptr = whois_queue_head (from_server)) &&
	    (whois_type_head (from_server) & WHOIS_WHOIS ) &&
	    whois_stuff->nick &&	/* This is *weird* */
	    (!my_stricmp (ptr, whois_stuff->nick)))
	{
		malloc_strcpy (&whois_stuff->server, server);
		malloc_strcpy (&whois_stuff->server_stuff, line);
	}
	else
	{
		message_from (NULL, LOG_CRAP);
		if (do_hook (current_numeric, "%s %s %s", from, server, line))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_SERVER_FSET), "%s %s", server, line));
	}
}

/*
 * whois_oper: This displays the operator status of a user, as returned by
 * numeric 313 from the server.  If the ignore_whois_crap flag is set,
 * nothing is dispayed. 
 */
void 
whois_oper (char *from, char **ArgList)
{
	WhoisStuff *whois_stuff;

	whois_stuff = get_server_whois_stuff (from_server);
	PasteArgs (ArgList, 1);
	if (ignore_whois_crap)
		whois_stuff->oper = 1;
	else
	{
		char *nick;

		if ((nick = ArgList[0]) != NULL)
		{
			message_from (NULL, LOG_CRAP);
			if (do_hook (current_numeric, "%s %s %s", from, nick, ArgList[1]))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_OPER_FSET), "%s %s", nick, " (is \002NOT\002 an IRC warrior)"));
		}

	}
}

void 
whois_lastcom (char *from, char **ArgList)
{
	if (!ignore_whois_crap)
	{
		char *nick, *idle_str;

		PasteArgs (ArgList, 2);
		message_from (NULL, LOG_CRAP);
		if ((nick = ArgList[0]) && (idle_str = ArgList[1]) &&
		    do_hook (current_numeric, "%s %s %s %s", from,
			     nick, idle_str, ArgList[2]))
		{
			int seconds = 0;
			int hours = 0;
			int minutes = 0;
			int secs = my_atol (idle_str);
			hours = secs / 3600;
			minutes = (secs - (hours * 3600)) / 60;
			seconds = secs % 60;
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_IDLE_FSET), "%d %d %d %s", hours, minutes, seconds, ArgList[2] ? ArgList[2] : empty_str));
		}
	}
}

void 
end_of_whois (char *from, char **ArgList)
{
	char *nick;
	char *ptr;
	WhoisStuff *whois_stuff;

	whois_stuff = get_server_whois_stuff (from_server);

	show_away_flag = 0;
	if ((nick = ArgList[0]) != NULL)
	{
		ptr = whois_queue_head (from_server);
		if (ptr && (whois_type_head (from_server) & WHOIS_WHOIS ) &&
		    (!my_stricmp (ptr, nick)))
		{
			WhoisQueue *thing;

			thing = remove_from_whois_queue (from_server);
			whois_stuff->not_on = 0;
			thing->func (whois_stuff, thing->nick, thing->text);
			new_free (&whois_stuff->channels);
			new_free (&thing->nick);
			new_free (&thing->text);
			new_free ((char **) &thing);
			ignore_whois_crap = 0;
			return;
		}
		PasteArgs (ArgList, 0);
		message_from (NULL, LOG_CRAP);
		if (do_hook (current_numeric, "%s %s", from, ArgList[0]) && get_fset_var (FORMAT_WHOIS_FOOTER_FSET))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_FOOTER_FSET), NULL));
	}
}

/*
 * no_such_nickname: Handler for numeric 401, the no such nickname error. If
 * the nickname given is at the head of the queue, then this routine pops the
 * top element from the queue, sets the whois_stuff->flag to indicate that the
 * user is no longer on irc, then calls the func() as normal.  It is up to
 * that function to set the ignore_whois_crap variable which will determine
 * if any other information is displayed or not. 
 *
 * that is, it used to.  now it does bugger all, seeing all the functions that
 * used to use it, now use no such command.  -phone, april 1993.
 */
void 
no_such_nickname (char *from, char **ArgList)
{
	char *nick;
	char *ptr, *bar;
	WhoisStuff *whois_stuff;
	int foo;

	whois_stuff = get_server_whois_stuff (from_server);
	ptr = whois_queue_head (from_server);

	if (ptr && *ptr == ' ')
	{
		++ptr;
	}
	PasteArgs (ArgList, 1);
	nick = ArgList[0];
	if (*nick == '!')
	{
		char *name = nick + 1;

		if (ptr && (whois_type_head (from_server) & WHOIS_WHOIS ) && !strcmp (ptr, name))
		{
			WhoisQueue *thing;

			/* There's a query in the WhoisQueue : assume it's
			   completed and remove it from the queue -Sol */

			thing = remove_from_whois_queue (from_server);
			whois_stuff->not_on = 0;
			thing->func (whois_stuff, thing->nick, thing->text);
			new_free (&whois_stuff->channels);
			new_free (&thing->nick);
			new_free (&thing->text);
			new_free ((char **) &thing);
			ignore_whois_crap = 0;
			return;
		}
		return;
	}
	notify_mark (nick, NULL, NULL, 0);
	if (ptr && (whois_type_head (from_server) & WHOIS_USERHOST) &&
	    (bar = strstr (ptr, nick)))
	{
		WhoisQueue *thing;
		int i;

		/* Remove query from queue. Don't display anything. -Sol */


		i = strlen (nick);


		if (*(bar + i + 1) == '\0' && (bar != ptr))
		{
			*bar = '\0';
		}
		if ((*(bar + i) == ' ') && (*(bar + i + 1) != '\0'))
		{
			memcpy (bar, bar + i + 1, strlen (bar) - i);
		}
		else
		{
			/* take it off all together */
			thing = remove_from_whois_queue (from_server);
			new_free (&whois_stuff->channels);
			new_free (&thing->nick);
			new_free (&thing->text);
			new_free ((char **) &thing);
			ignore_whois_crap = 0;
		}
		return;
	}
	message_from (NULL, LOG_CRAP);
	if ((foo = get_int_var (AUTO_WHOWAS_VAR)))
	{
		if (foo > -1)
			send_to_server ("WHOWAS %s %d", nick, foo);
		else
			send_to_server ("WHOWAS %s", nick);
	}
	else if (do_hook (current_numeric, "%s %s %s", from, nick, ArgList[1]))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_NONICK_FSET), "%s %s %s %s", update_clock (GET_TIME), nick, from, ArgList[1]));
}

/*
 * user_is_away: called when a 301 numeric is received.  Nothing is displayed
 * by this routine if the ignore_whois_crap flag is set 
 */
void 
user_is_away (char *from, char **ArgList)
{
	char *message, *who;
	WhoisStuff *whois_stuff;

	if (!from)
		return;

	PasteArgs (ArgList, 1);
	whois_stuff = get_server_whois_stuff (from_server);

	if ((who = ArgList[0]) && (message = ArgList[1]))
	{
		if (whois_stuff->nick && (!strcmp (who, whois_stuff->nick)) && eat_away)
			malloc_strcpy (&whois_stuff->away, message);
		else
		{
			message_from (NULL, LOG_CRAP);
			if (do_hook (current_numeric, "%s %s", who, message))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_WHOIS_AWAY_FSET), "%s %s", who, message));
		}
	}
}

/*
 * The stuff below this point are all routines suitable for use in the
 * add_to_whois_queue() call as the func parameter 
 */

/*
 * whois_ignore_msgs: This is used when you are ignoring MSGs using the
 * user@hostname format 
 */
void 
whois_ignore_msgs (WhoisStuff * stuff, char *nick, char *text)
{
	char *ptr;
	int level;

	if (stuff)
	{
		ptr = (char *) new_malloc (strlen (stuff->user) +
					   strlen (stuff->host) + 2);
		strcpy (ptr, stuff->user);
		strcat (ptr, "@");
		strcat (ptr, stuff->host);
		if (is_ignored (ptr, IGNORE_MSGS) != IGNORED)
		{
			level = set_lastlog_msg_level (LOG_MSG);
			message_from (stuff->nick, LOG_MSG);
			if (sed == 1 && !do_hook (ENCRYPTED_PRIVMSG_LIST, "%s %s", stuff->nick, text))
			{
				sed = 0;
				return;
			}
			if (do_hook (MSG_LIST, "%s %s", stuff->nick, text))
			{
				if (away_set)
				{
					time_t t;
					char *msg = NULL;

					t = time (NULL);
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_IGNORE_MSG_AWAY_FSET), "%s %s %s", update_clock (GET_TIME), stuff->nick, msg));
					beep_em (get_int_var (BEEP_WHEN_AWAY_VAR));
				}
				else
				{
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_IGNORE_MSG_FSET), "%s %s %s", update_clock (GET_TIME), stuff->nick, text));
					beep_em (get_int_var (BEEP_ON_MSG_VAR));
				}
			}
			if (beep_on_level & LOG_MSG)
				beep_em (1);
			set_lastlog_msg_level (level);
			sed = 0;
			notify_mark (nick, stuff->user, stuff->host, 1);
		}
		else if (get_int_var (SEND_IGNORE_MSG_VAR))
			send_to_server ("NOTICE %s :%s is ignoring you.",
				   nick, get_server_nickname (from_server));
		new_free (&ptr);
	}
}

void 
whois_nickname (WhoisStuff * stuff, char *nick, char *text)
{
	if (stuff)
	{
		if (!(my_stricmp (stuff->user, username)) &&
		    (!my_stricmp (stuff->host, hostname)))
			set_server_nickname (from_server, nick);
	}
}

/*
 * whois_ignore_notices: This is used when you are ignoring NOTICEs using the
 * user@hostname format 
 */
void 
whois_ignore_notices (WhoisStuff * stuff, char *nick, char *text)
{
	char *ptr;
	int level;

	if (stuff)
	{
		ptr = (char *) new_malloc (strlen (stuff->user) +
					   strlen (stuff->host) + 2);
		strcpy (ptr, stuff->user);
		strcat (ptr, "@");
		strcat (ptr, stuff->host);
		if (is_ignored (ptr, IGNORE_NOTICES) != IGNORED)
		{
			level = set_lastlog_msg_level (LOG_NOTICE);
			message_from (stuff->nick, LOG_NOTICE);
			if (sed == 0 && !do_hook (ENCRYPTED_NOTICE_LIST, "%s %s", stuff->nick, text))
			{
				sed = 0;
				return;
			}
			if (do_hook (NOTICE_LIST, "%s %s", stuff->nick, text))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_IGNORE_NOTICE_FSET), "%s %s %s", update_clock (GET_TIME), stuff->nick, text));
			set_lastlog_msg_level (level);
			sed = 0;
		}
		new_free (&ptr);
	}
}

/*
 * whois_ignore_invites: This is used when you are ignoring INVITES using the
 * user@hostname format 
 */
void 
whois_ignore_invites (WhoisStuff * stuff, char *nick, char *text)
{
	char *ptr;

	if (stuff)
	{
		ptr = (char *) new_malloc (strlen (stuff->user) +
					   strlen (stuff->host) + 2);
		strcpy (ptr, stuff->user);
		strcat (ptr, "@");
		strcat (ptr, stuff->host);
		if (is_ignored (ptr, IGNORE_INVITES) != IGNORED)
		{
			struct whowas_chan_list *w_chan = NULL;
			struct channel *chan = NULL;
			if (do_hook (INVITE_LIST, "%s %s", stuff->nick, text))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_IGNORE_INVITE_FSET), "%s %s", stuff->nick, text));
			malloc_strcpy (&invite_channel, text);
			if (!(chan = lookup_channel (invite_channel, from_server, 0)))
			{
				if ((w_chan = check_whowas_chan_buffer (invite_channel, 0)))
					chan = w_chan->channellist;
			}
			if (chan && get_int_var (AUTO_REJOIN_VAR))
				send_to_server ("JOIN %s%s%s", invite_channel, chan->key ? " " : empty_str, chan->key ? chan->key : empty_str);
		}
		else if (get_int_var (SEND_IGNORE_MSG_VAR))
			send_to_server ("NOTICE %s :%s is ignoring you.",
				   nick, get_server_nickname (from_server));
		new_free (&ptr);
	}
}

/*
 * whois_ignore_walls: This is used when you are ignoring WALLS using the
 * user@hostname format 
 */
void 
whois_ignore_walls (WhoisStuff * stuff, char *nick, char *text)
{
	char *ptr;
	int level;

	level = set_lastlog_msg_level (LOG_WALL);
	message_from (stuff->nick, LOG_WALL);
	if (stuff)
	{
		ptr = (char *) new_malloc (strlen (stuff->user) +
					   strlen (stuff->host) + 2);
		strcpy (ptr, stuff->user);
		strcat (ptr, "@");
		strcat (ptr, stuff->host);
		if (is_ignored (ptr, IGNORE_WALLS) != IGNORED)
		{
			if (do_hook (WALL_LIST, "%s %s", stuff->nick, text))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_IGNORE_WALL_FSET), "%s %s %s", update_clock (GET_TIME), stuff->nick, text));
			if (beep_on_level & LOG_WALL)
				beep_em (1);
		}
		new_free (&ptr);
	}
	set_lastlog_msg_level (level);
}

/*
 * add_to_whois_queue: This routine is called whenever you want to do a WHOIS
 * or WHOWAS.  What happens is this... each time this function is called it
 * adds a new element to the whois queue using the nick and func as in
 * WhoisQueue, and creating the text element using the format and args.  It
 * then issues the WHOIS or WHOWAS.  
 */
void 
add_to_whois_queue (char *nick, void (*func) (WhoisStuff *, char *, char *), char *format,...)
{
	int Type;

	if (func == USERHOST_USERHOST || func == userhost_cmd_returned)
		Type = WHOIS_USERHOST;
	else
		Type = WHOIS_WHOIS;

	if (format)
	{
		char blah[BIG_BUFFER_SIZE + 1];
		va_list vlist;
		va_start (vlist, format);
		vsnprintf (blah, BIG_BUFFER_SIZE, format, vlist);
		va_end (vlist);
		typed_add_to_whois_queue (Type, nick, func, "%s", blah);
	}
}

void 
add_to_userhost_queue (char *nick, void (*func) (WhoisStuff *, char *, char *), char *format,...)
{
	if (format)
	{
		char blah[BIG_BUFFER_SIZE + 1];
		va_list vlist;
		va_start (vlist, format);
		vsnprintf (blah, BIG_BUFFER_SIZE, format, vlist);
		va_end (vlist);

		typed_add_to_whois_queue (WHOIS_USERHOST, nick, func, "%s", blah);
	}
}

void 
add_ison_to_whois (char *nick, void (*func) ())
{
	typed_add_to_whois_queue (WHOIS_ISON, nick, func, NULL);
}

void 
typed_add_to_whois_queue (int type, char *nick, void (*func) (WhoisStuff *, char *, char *), char *format,...)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	WhoisQueue *new = NULL;
	char *p = nick;

	if ((nick == NULL) || !is_server_connected (from_server))
		return;

	for (; *p == ' ' || *p == '\t'; p++);
	if (!*p)
		return;		/* nick should always be a non-blank string, but
				   I'd rather check because of a "ISON not enough
				   parameters" coming from the server -Sol */

	if (strchr (nick, '*') == NULL)
	{
		new = (WhoisQueue *) new_malloc (sizeof (WhoisQueue));
		memset (new, 0, sizeof (WhoisQueue));
		new->func = func;
		new->type = type;
		new->nick = m_strdup (nick);

		if (format)
		{
			va_list args;
			va_start (args, format);
			vsnprintf (buffer, BIG_BUFFER_SIZE, format, args);
			va_end (args);
			new->text = m_strdup (buffer);
		}
		if ((void *) get_server_qhead (from_server) == NULL)
			set_server_qhead (from_server, new);
		if (get_server_qtail (from_server))
			((WhoisQueue *) get_server_qtail (from_server))->next = new;
		set_server_qtail (from_server, new);
		switch (type)
		{
		case WHOIS_ISON:
#ifdef MONITOR_Q
			put_it ("+++ ISON %s", nick);
#endif
			send_to_server ("ISON %s", nick);
			break;
		case WHOIS_USERHOST:
#ifdef MONITOR_Q
			put_it ("+++ USERHOST %s", nick);
#endif
			send_to_server ("USERHOST %s", nick);
			break;
		case WHOIS_WHOIS:
#ifdef MONITOR_Q
			put_it ("+++ WHOIS %s", nick);
#endif
			send_to_server ("WHOIS %s", nick);
			break;
		case WHOIS_WHOWAS:
#ifdef MONITOR_Q
			put_it ("+++ WHOWAS %s", nick);
#endif
			send_to_server ("WHOWAS %s", nick);
			break;
		}
	}
}

extern void 
userhost_cmd_returned (WhoisStuff * stuff, char *nick, char *text)
{
	char args[BIG_BUFFER_SIZE + 1];

	strcpy (args, stuff->nick ? stuff->nick : empty_str);
	strcat (args, stuff->oper ? " + " : " - ");
	strcat (args, stuff->away ? "+ " : "- ");
	strcat (args, stuff->user ? stuff->user : empty_str);
	strcat (args, space_str);
	strcat (args, stuff->host ? stuff->host : empty_str);
	parse_line ("USERHOST", text, args, 0, 0);
}
