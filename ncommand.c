#ident "$Id: ncommand.c,v 1.4 1999/12/02 03:30:51 laeos Exp $"
/*
 * ncommand.c : new commands for Xaric
 * (c) 1998 Rex Feany <laeos@ptw.com> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * TODO:
 *
 *  change the userage function to just be a macro? it is kind of silly now.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "irc.h"

#include <sys/stat.h>

#ifdef ESIX
#include <lan/net_types.h>
#endif


#include "parse.h"
#include "ircterm.h"
#include "server.h"
#include "commands.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "alist.h"
#include "names.h"
#include "expr.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "timer.h"
#include "list.h"
#include "misc.h"
#include "hash.h"
#include "fset.h"
#include "notice.h"
#include "whowas.h"


#include "tcommand.h"

/* recv_nick: the nickname of the last person to send you a privmsg */
extern char *recv_nick;

	/* sent_nick: the nickname of the last person to whom you sent a privmsg */
extern char *sent_nick;
extern char *sent_body;

/* 
 * Global vars used all over the place
 */

int user_changing_nickname = 0;

#ifdef HAVE_GETTIMEOFDAY
struct timeval in_sping =
{0};
#else
time_t in_sping = 0;
#endif


extern NickTab *tabkey_array;



/* -- These are all the commands ------------------------ */


void
cmd_abort (struct command *cmd, char *args)
{
	char *filename = next_arg (args, &args);

	filename = filename ? filename : "irc.aborted";

	save_all (filename);
	abort ();
}

void
cmd_about (struct command *cmd, char *args)
{
	int i = strip_ansi_in_echo;

	strip_ansi_in_echo = 0;

	charset_ibmpc ();
	put_it (empty_str);
	put_it ("\t[36mX a r i c");
	put_it ("\t[35mv%s brought to you by Laeos, Korndawg, and Hawky", PACKAGE_VERSION);
	put_it ("\t[35m<http://www.laeos.net/xaric>");
	put_it (empty_str);
	charset_lat1 ();
	strip_ansi_in_echo = i;
}

void
cmd_alias (struct command *cmd, char *args)
{
	char *str;
	char *command;

	str = next_arg (args, &args);

	if (str == NULL)
	{
		userage (cmd->name, cmd->qhelp);
		return;
	}
	command = next_arg (args, &args);

	if (command == NULL)
	{
		userage (cmd->name, cmd->qhelp);
		return;
	}

	upper (str);
	upper (command);

	if (t_bind (str, command, args) < 0)
		bitchsay ("Error creating alias!");

}

#ifdef XARIC_DEBUG
void 
cmd_debug (struct command *cmd, char *args)
{
	if (args && *args) {
		if (xd_parse(args)) {
			yell("Invalid DEBUG values!");
		}
	} else {
		xd_list(0);
	}
}
#endif /* XARIC_DEBUG */

void
cmd_away (struct command *cmd, char *args)
{
	int len;
	char *arg = NULL;
	int flag = AWAY_ONE;
	int i;

	if (*args)
	{
		if ((*args == '-') || (*args == '/'))
		{
			arg = strchr (args, ' ');
			if (arg)
				*arg++ = '\0';
			else
				arg = empty_str;
			len = strlen (args);
			if (0 == my_strnicmp (args + 1, "A", 1))	/* all */
			{
				flag = AWAY_ALL;
				args = arg;
			}
			else if (0 == my_strnicmp (args + 1, "O", 1))	/* one */
			{
				flag = AWAY_ONE;
				args = arg;
			}
			else
			{
				userage (cmd->name, cmd->qhelp);
				return;
			}
		}
	}
	if (flag == AWAY_ALL)
	{
		for (i = 0; i < number_of_servers; i++)
			set_server_away (i, args);
	}
	else
	{
		if (args && *args)
			set_server_away (curr_scr_win->server, args);
	}
	update_all_status (curr_scr_win, NULL, 0);
}

static void
read_away_log (char *stuff, char *line)
{
	if (!line || !*line || (line && toupper (*line) == 'Y'))
		t_parse_command ("READLOG", NULL);
	update_input (UPDATE_ALL);
}

void
cmd_back (struct command *cmd, char *args)
{
	char *tmp = NULL;
	int minutes = 0;
	int hours = 0;
	int seconds = 0;

	if (!curr_scr_win || curr_scr_win->server == -1)
		return;

	if (server_list[curr_scr_win->server].awaytime)
	{
		struct channel *chan = NULL;
		time_t current_t = time (NULL) - server_list[curr_scr_win->server].awaytime;
		int old_server = from_server;
		from_server = curr_scr_win->server;

		hours = current_t / 3600;
		minutes = (current_t - (hours * 3600)) / 60;
		seconds = current_t % 60;


		if (get_fset_var (FORMAT_BACK_FSET) && get_int_var (SEND_AWAY_MSG_VAR))
		{
			bitchsay ("You were /away for %i hours %i minutes and %i seconds.",
				  hours, minutes, seconds);
			if (get_int_var (MSGLOG_VAR))
				log_toggle (0, chan);
			for (chan = server_list[curr_scr_win->server].chan_list; chan; chan = chan->next)
			{
				send_to_server ("PRIVMSG %s :ACTION %s", chan->channel,
						stripansicodes (convert_output_format (get_fset_var (FORMAT_BACK_FSET), "%s %d %d %d %d %s",
						    update_clock (GET_TIME),
						    hours, minutes, seconds,
										       get_int_var (MSGCOUNT_VAR), args ? args : get_server_away (curr_scr_win->server))));
			}
		}
		set_server_away (curr_scr_win->server, NULL);
		from_server = old_server;
	}
	else
		set_server_away (curr_scr_win->server, NULL);

	server_list[curr_scr_win->server].awaytime = (time_t) 0;
	if (get_fset_var (FORMAT_BACK_FSET) && get_int_var (MSGLOG_VAR))
	{
		malloc_sprintf (&tmp, " read /away msgs (%d msg%s) log [Y/n]? ", get_int_var (MSGCOUNT_VAR), plural (get_int_var (MSGCOUNT_VAR)));
		add_wait_prompt (tmp, read_away_log, empty_str, WAIT_PROMPT_LINE);
		new_free (&tmp);
	}
	set_int_var (MSGCOUNT_VAR, 0);
	update_all_status (curr_scr_win, NULL, 0);
}


void
cmd_chwall (struct command *cmd, char *args)
{
	char *channel = NULL;
	char *chops = NULL;
	char *include = NULL;
	char *exclude = NULL;
	struct channel *chan;
	struct nick_list *tmp;
	char buffer[BIG_BUFFER_SIZE + 1];

	if (!args || (args && !*args))
	{
		userage (cmd->name, cmd->qhelp);
		return;
	}
	if (get_current_channel_by_refnum (0))
	{
		int count = 0;
		int i = 0;
		char *nick = NULL;
		malloc_strcpy (&channel, get_current_channel_by_refnum (0));
		chan = lookup_channel (channel, curr_scr_win->server, 0);
		while (args && (*args == '-' || *args == '+'))
		{
			nick = next_arg (args, &args);
			if (*nick == '-')
			{
				malloc_strcat (&exclude, nick + 1);
				malloc_strcat (&exclude, " ");
			}
			else
			{
				malloc_strcat (&include, nick + 1);
				malloc_strcat (&include, " ");
			}
		}
		if (!args || !*args)
		{
			bitchsay ("NO Wallmsg included");
			new_free (&exclude);
			new_free (&include);
			new_free (&channel);
		}
		message_from (channel, LOG_NOTICE);
		sprintf (buffer, "[\002Xaric-Wall\002/\002%s\002] %s", channel, args);
		for (tmp = next_nicklist (chan, NULL); tmp; tmp = next_nicklist (chan, tmp))
		{
			if (!my_stricmp (tmp->nick, get_server_nickname (from_server)))
				continue;
			if (exclude && stristr (exclude, tmp->nick))
				continue;
			if (tmp->chanop == 1 || (include && stristr (include, tmp->nick)))
			{
				if (chops)
					malloc_strcat (&chops, ",");
				malloc_strcat (&chops, tmp->nick);
				count++;
			}
			if (count >= 8 && chops)
			{
				send_to_server ("%s %s :%s", "NOTICE", chops, buffer);
				i += count;
				count = 0;
				new_free (&chops);
			}
		}
		i += count;
		if (chops)
			send_to_server ("%s %s :%s", "NOTICE", chops, buffer);
		if (i)
		{
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_BWALL_FSET), "%s %s %s %s %s", update_clock (GET_TIME), channel, "*", "*", args));
			if (exclude)
			{
				exclude[strlen (exclude) - 1] = '\0';
				bitchsay ("Excluded <%s> from wallmsg", exclude);
			}
			if (include)
			{
				include[strlen (include) - 1] = '\0';
				bitchsay ("Included <%s> in wallmsg", include);
			}
		}
		else
			put_it ("%s", convert_output_format ("$G No ops on $0", "%s", channel));
		message_from (NULL, LOG_CRAP);
	}
	else
		say ("No Current Channel for this Window.");
	new_free (&include);
	new_free (&channel);
	new_free (&chops);
	new_free (&exclude);
}

void
cmd_clear_tab (struct command *cmd, char *args)
{
	clear_array (&tabkey_array);
}

void
cmd_ctcp_version (struct command *cmd, char *args)
{
	char *person;
	int type = 0;

	if ((person = next_arg (args, &args)) == NULL || !strcmp (person, "*"))
	{
		if ((person = get_current_channel_by_refnum (0)) == NULL)
			person = zero_str;
	}
	if ((type = in_ctcp ()) == -1)
		say ("You may not use the CTCP command in an ON CTCP_REPLY!");
	else
	{
		send_ctcp (type, person, CTCP_VERSION, NULL);
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_CTCP_FSET),
		   "%s %s %s", update_clock (GET_TIME), person, "VERSION"));
	}
}

void
cmd_ctcp (struct command *cmd, char *args)
{
	char *to;
	char *stag = NULL;
	int tag;
	int type;

	if ((to = next_arg (args, &args)) != NULL)
	{
		if (!strcmp (to, "*"))
			if ((to = get_current_channel_by_refnum (0)) == NULL)
				to = "0";

		if ((stag = next_arg (args, &args)) != NULL)
			tag = get_ctcp_val (upper (stag));
		else
			tag = CTCP_VERSION;

		if ((type = in_ctcp ()) == -1)
			say ("You may not use the CTCP command from an ON CTCP_REPLY!");
		else
		{
			if (args && *args)
				send_ctcp (type, to, tag, "%s", args);
			else
				send_ctcp (type, to, tag, NULL);
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_CTCP_FSET),
			     "%s %s %s %s", update_clock (GET_TIME), to, stag ? stag : "VERSION", args ? args : empty_str));
		}
	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_clear (struct command *cmd, char *args)
{
	char *arg;
	int all = 0, unhold = 0;

	while ((arg = next_arg (args, &args)) != NULL)
	{
		/* -ALL and ALL here becuase the help files used to be wrong */
		if (!my_strnicmp (arg, "A", 1) || !my_strnicmp (arg + 1, "A", 1))
			all = 1;
		/* UNHOLD */
		else if (!my_strnicmp (arg + 1, "U", 1))
			unhold = 1;
		else {
			userage ("clear", cmd->qhelp);
			return;
		}
	}
	if (all)
		clear_all_windows (unhold);
	else
	{
		if (unhold)
			hold_mode (NULL, OFF, 1);
		clear_window_by_refnum (0);
	}
	update_input (UPDATE_JUST_CURSOR);
}

extern char *channel_key (char *);

void
cmd_cycle (struct command *cmd, char *args)
{
	char *to = NULL;
	int server = from_server;
	struct channel *chan;

	if (args && args)
		to = next_arg (args, &args);

	if (!(chan = prepare_command (&server, to, NO_OP)))
		return;
	my_send_to_server (server, "PART %s", chan->channel);
	my_send_to_server (server, "JOIN %s%s%s", chan->channel, chan->key ? " " : "", chan->key ? chan->key : "");
}

static void
handle_dcc_chat (WhoisStuff * stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp (stuff->user, "<UNKNOWN>") || !strcmp (stuff->host, "<UNKNOWN>"))
	{
		bitchsay ("No such nick %s", nick);
		return;
	}
	dcc_chat (NULL, nick);
}

void
cmd_dcc_chat (struct command *cmd, char *args)
{
	char *nick = next_arg (args, &args);

	if (nick)
	{
		do
		{
			add_to_userhost_queue (nick, handle_dcc_chat, nick, NULL);
		}
		while ((nick = next_arg (args, &args)));
	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_dcc (struct command *cmd, char *args)
{
	if (*args)
		process_dcc (args);
	else
		dcc_glist (NULL, NULL);
}

void
cmd_describe (struct command *cmd, char *args)
{
	char *target;

	target = next_arg (args, &args);
	if (target && args && *args)
	{
		int old;
		int from_level;
		char *message;

		message = args;
		send_ctcp (CTCP_PRIVMSG, target, CTCP_ACTION, "%s", message);

		old = set_lastlog_msg_level (LOG_ACTION);
		from_level = message_from_level (LOG_ACTION);
		if (do_hook (SEND_ACTION_LIST, "%s %s", target, message))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_ACTION_OTHER_FSET),
							     "%s %s %s %s", update_clock (GET_TIME), get_server_nickname (from_server), target, message));
		set_lastlog_msg_level (old);
		message_from_level (from_level);

	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_disconnect (struct command *cmd, char *args)
{
	char *server;
	char *message;
	int i;

	if ((server = next_arg (args, &args)) != NULL)
	{
		i = parse_server_index (server);
		if (-1 == i)
		{
			say ("No such server!");
			return;
		}
	}
	else
		i = get_window_server (0);

	/*
	 * XXX - this is a major kludge.  i should never equal -1 at
	 * this point.  we only do this because something has gotten
	 * *really* confused at this point.  .mrg.
	 *
	 * Like something so obvious as already being disconnected?
	 */
	if (i == -1)
	{
		if (connected_to_server)
		{
			yell ("Ouch.. this is a -shouldnt-happen-");
			return;
		}
		bitchsay ("You arn't connected to any server!!");
		return;
	}

	if (!args || !*args)
		message = "Disconnecting";
	else
		message = args;

	if (server_list[i].sock == NULL)
	{
		say ("That server isn't connected!");
		return;
	}
	if (do_hook (DISCONNECT_LIST, "Connection Closed."))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_DISCONNECT_FSET), "%s %s %s", update_clock (GET_TIME), "Disconnecting from server", server_list[i].itsname));
	clear_channel_list (i);
	close_server (i, message);
	server_list[i].eof = 1;

	clean_whois_queue ();
	window_check_servers ();
	if (!connected_to_server)
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_DISCONNECT_FSET), "%s %s", update_clock (GET_TIME), "You are not connected to a server. Use /SERVER to connect."));
}

void
cmd_echo (struct command *cmd, char *args)
{
	unsigned int display;
	int lastlog_level = 0;
	int from_level = 0;
	char *flag_arg;
	int temp;
	int all_windows = 0;
	int x = -1, y = -1, raw = 0;
	int banner = 0;
	char *stuff = NULL;
	Window *old_to_window;

	old_to_window = to_window;

	if (*cmd->name == 'X')
	{
		while (args && (*args == '-' || *args == '/'))
		{
			flag_arg = next_arg (args, &args);
			switch (toupper (flag_arg[1]))
			{
			case 'C':
				{
					to_window = curr_scr_win;
					break;
				}
			case 'L':
				{
					if (!(flag_arg = next_arg (args, &args)))
						break;
					if ((temp = parse_lastlog_level (flag_arg)) != 0)
					{
						lastlog_level = set_lastlog_msg_level (temp);
						from_level = message_from_level (temp);
					}
					break;
				}
			case 'W':
				{
					if (!(flag_arg = next_arg (args, &args)))
						break;
					if (isdigit (*flag_arg))
						to_window = get_window_by_refnum (atoi (flag_arg));
					else
						to_window = get_window_by_name (flag_arg);
					break;
				}
			case 'A':
			case '*':
				all_windows = 1;
				break;
			case 'X':
				if (!(flag_arg = next_arg (args, &args)))
					break;
				x = my_atol (flag_arg);
				break;
			case 'Y':
				if (!(flag_arg = next_arg (args, &args)))
					break;
				y = my_atol (flag_arg);
				break;
			case 'R':
				raw = 1;
				break;
			case 'B':
				banner = 1;
				break;
			}
			if (!args)
				args = empty_str;
		}
	}
	display = window_display;
	window_display = 1;
	strip_ansi_in_echo = 0;
	if (banner)
	{
		malloc_strcpy (&stuff, line_thing);
		if (*stuff)
		{
			m_3cat (&stuff, space_str, args);
			args = stuff;
		}
	}
	if (all_windows)
	{
		Window *win = NULL;

		while (traverse_all_windows (&win))
		{
			to_window = win;
			put_echo (args);
		}
	}
	else if (x > -1 || y > -1 || raw)
	{
		if (x <= -1)
			x = 0;
		if (y <= -1)
		{
			if (to_window)
				y = to_window->cursor;
			else
				y = curr_scr_win->cursor;
		}
		if (!raw)
			term_move_cursor (x, y);
		term_puts (args, strlen (args));
	}
	else
		put_echo (args);
	strip_ansi_in_echo = 1;
	window_display = display;
	if (lastlog_level)
	{
		set_lastlog_msg_level (lastlog_level);
		message_from_level (from_level);
	}
	if (stuff)
		new_free (&stuff);
	to_window = old_to_window;
}

void
cmd_flush (struct command *cmd, char *args)
{
	if (get_int_var (HOLD_MODE_VAR))
		flush_everything_being_held (NULL);
	bitchsay ("Standby, Flushing server output...");
	flush_server ();
	bitchsay ("Done");
}


void
cmd_join (struct command *cmd, char *args)
{
	char *chan;
	int len;
	char *buffer = NULL;
	message_from (NULL, LOG_CURRENT);

	if ((chan = next_arg (args, &args)) != NULL)
	{
		len = strlen (chan);
		if (my_strnicmp (chan, "-i", 2) == 0)
		{
			if (invite_channel)
				send_to_server ("JOIN %s %s", invite_channel, args);
			else
				bitchsay ("You have not been invited to a channel!");
		}
		else
		{
			buffer = make_channel (chan);
			if (is_on_channel (buffer, from_server, get_server_nickname (from_server)))
			{
				/* XXX -
				   right here we want to check to see if the
				   channel is bound to this window.  if it is,
				   we set it as the default channel.  If it
				   is not, we warn the user that we cant do it
				 */
				if (is_bound_anywhere (buffer) &&
				!(is_bound_to_window (curr_scr_win, buffer)))
					bitchsay ("Channel %s is bound to another window", buffer);

				else
				{
					is_current_channel (buffer, from_server, 1);
					bitchsay ("You are now talking to channel %s",
						  set_current_channel_by_refnum (0, buffer));
					update_all_windows ();
					update_input (UPDATE_ALL);
				}
			}
			else
			{
				send_to_server ("JOIN %s%s%s", buffer, args ? " " : empty_str, args ? args : empty_str);
				if (!is_bound (buffer, curr_scr_win->server))
					malloc_strcpy (&curr_scr_win->waiting_channel, buffer);
			}
		}
	}
	else
		list_channels ();
	message_from (NULL, LOG_CRAP);
}

void
cmd_linklook (struct command *cmd, char *args)
{
	struct server_split *serv = server_last;
	int count;

	if (!serv)
	{
		bitchsay ("No active splits");
		return;
	}

	count = 0;
	while (serv)
	{
		if (serv->status & SPLIT)
		{
			if (!count)
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETSPLIT_HEADER_FSET), "%s %s %s %s", "time", "server", "uplink", "hops"));
			if (do_hook (LLOOK_SPLIT_LIST, "%s %s %d", serv->name, serv->link, serv->hopcount))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETSPLIT_FSET), "%s %s %s %d", serv->time, serv->name, serv->link, serv->hopcount));
			count++;
		}
		serv = serv->next;
	}
	if (count)
		bitchsay ("There %s %d split servers", (count == 1) ? "is" : "are", count);
	else
		bitchsay ("No split servers found");
}

void
cmd_map (struct command *cmd, char *args)
{
	if (server_list[from_server].link_look == 0)
	{
		bitchsay ("Generating irc server map");
		send_to_server ("LINKS");
		server_list[from_server].link_look = 2;
	}
	else
		bitchsay ("Wait until previous %s is done", server_list[from_server].link_look == 2 ? "MAP" : "LLOOK");
}

void
cmd_me (struct command *cmd, char *args)
{
	if (args && *args)
	{
		char *target = NULL;

		if ((target = query_nick ()) == NULL)
			target = get_current_channel_by_refnum (0);

		if (target)
		{
			int old;
			char *message;
			message = args;
			send_ctcp (CTCP_PRIVMSG, target, CTCP_ACTION, "%s", message);

			message_from (target, LOG_ACTION);
			old = set_lastlog_msg_level (LOG_ACTION);
			if (do_hook (SEND_ACTION_LIST, "%s %s", target, message))
			{
				if (strchr ("&#", *target))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_ACTION_FSET),
									     "%s %s %s %s", update_clock (GET_TIME), get_server_nickname (from_server), target, message));
				else
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_ACTION_OTHER_FSET),
									     "%s %s %s %s", update_clock (GET_TIME), get_server_nickname (from_server), target, message));
			}
			set_lastlog_msg_level (old);
			message_from (NULL, LOG_CRAP);
		}
		else
			userage (cmd->name, cmd->qhelp);
	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_names (struct command *cmd, char *args)
{
	char *channel = NULL;
	int server = from_server;
	struct channel *chan;

	if (args)
		channel = next_arg (args, &args);

	if (!(chan = prepare_command (&server, channel, NO_OP)))
		return;

	my_send_to_server (server, "NAMES %s", chan->channel);
}

void
cmd_nick (struct command *cmd, char *args)
{
	char *nick;

	if (!(nick = next_arg (args, &args)))
	{
		bitchsay ("Your nickname is %s", get_server_nickname (get_window_server (0)));
		if (get_pending_nickname (get_window_server (0)))
			say ("A nickname change to %s is pending.", get_pending_nickname (get_window_server (0)));

		return;
	}
	if (!(nick = check_nickname (nick)))
	{
		bitchsay ("Nickname specified is illegal.");
		return;
	}
	user_changing_nickname = 1;
	change_server_nickname (from_server, nick);
}

static void
userhost_nsl (WhoisStuff * stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp (stuff->user, "<UNKNOWN>") || my_stricmp (stuff->nick, nick))
	{
		say ("No information for %s", nick);
		return;
	}
#if HAVE_PTHREADS
	do_nslookup_thread (stuff->host);
#else
	do_nslookup (stuff->host);
#endif
}

void
cmd_nslookup (struct command *cmd, char *args)
{
	char *host;

	if ((host = next_arg (args, &args)))
	{
		bitchsay ("Checking tables...");
		if (!strchr (host, '.'))
			add_to_userhost_queue (host, userhost_nsl, "%s", host);
		else
#ifdef HAVE_PTHREADS
			do_nslookup_thread (host);
#else
			do_nslookup (host);
#endif

	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_nwhowas (struct command *cmd, char *args)
{
	show_whowas ();
}

void
cmd_part (struct command *cmd, char *args)
{
	char *channel = NULL;
	struct channel *chan;
	int server = from_server;
	int all = 0;

	if (!my_stricmp (cmd->name, "PARTALL"))
		all = 1;
	if (!all)
	{
		if (args)
			channel = next_arg (args, &args);
		if (!(chan = prepare_command (&server, channel ? make_channel (channel) : channel, NO_OP)))
			return;
		my_send_to_server (server, "PART %s", chan->channel);
	}
	else
	{
		for (chan = server_list[server].chan_list; chan; chan = chan->next)
			my_send_to_server (server, "PART %s", chan->channel);
	}
}

void
cmd_ping (struct command *cmd, char *args)
{
	struct timeval tp;
	char *to;

	get_time (&tp);

	if ((to = next_arg (args, &args)) == NULL || !strcmp (to, "*"))
	{
		if ((to = get_current_channel_by_refnum (0)) == NULL)
			to = zero_str;
	}

	if (in_ctcp () == -1)
		say ("You may not use the CTCP command in an ON CTCP_REPLY!");
	else
	{
		send_ctcp (0, to, CTCP_PING, "%ld %ld", (long) tp.tv_sec, (long) tp.tv_usec);
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_SEND_CTCP_FSET),
		   "%s %s %s", update_clock (GET_TIME), to, "PING"));
	}

}

void
cmd_privmsg (struct command *cmd, char *args)
{
	char *nick;

	if ((nick = next_arg (args, &args)) != NULL)
	{
		if (strcmp (nick, ".") == 0)
		{
			if (!(nick = sent_nick))
			{
				bitchsay ("You have not sent a message to anyone yet");
				return;
			}
		}
		else if (strcmp (nick, ",") == 0)
		{
			if (!(nick = recv_nick))
			{
				bitchsay ("You have not received a message from anyone yet");
				return;
			}
		}
		else if (!strcmp (nick, "*") && (!(nick = get_current_channel_by_refnum (0))))
			nick = "0";
		send_text (nick, args, cmd->rname, window_display, 1);
	}
	else
		userage (cmd->name, cmd->qhelp);
}

static void 
real_quit (char *dummy, char *ptr)
{
	if (ptr && *ptr && (*ptr == 'Y' || *ptr == 'y'))
		irc_exit (dummy, convert_output_format (get_fset_var (FORMAT_SIGNOFF_FSET), "%s %s %s %s", update_clock (GET_TIME), get_server_nickname (get_window_server (0)), m_sprintf ("%s@%s", username, hostname), dummy));
	bitchsay ("Excelllaaant!!");
}

void
cmd_query (struct command *xcmd, char *args)
{
	char *nick, *rest;
	char *cmd = NULL;

	message_from (NULL, LOG_CURRENT);

	if (args && !my_strnicmp (args, "-cmd", 4))
	{
		cmd = next_arg (args, &args);
		cmd = next_arg (args, &args);
	}
	if ((nick = next_arg (args, &rest)) != NULL)
	{
		if (*nick == '.')
		{
			if (!(nick = sent_nick))
			{
				bitchsay ("You have not messaged anyone yet");
				return;
			}
		}
		else if (*nick == ',')
		{
			if (!(nick = recv_nick))
			{
				bitchsay ("You have not recieved a message from \
						anyone yet");
				return;
			}
		}
		else if ((*nick == '*') && !(nick = get_current_channel_by_refnum (0)))
		{
			bitchsay ("You are not on a channel");
			return;
		}

		if (*nick == '%')
		{
			if (is_process (nick) == 0)
			{
				bitchsay ("Invalid processes specification");
				message_from (NULL, LOG_CRAP);
				return;
			}
		}
		bitchsay ("Starting conversation with %s", nick);
		set_query_nick (nick, args, cmd);
	}
	else if (cmd)
		set_query_nick (NULL, NULL, cmd);
	else
	{
		if (query_nick ())
		{
			bitchsay ("Ending conversation with %s", query_nick ());
			set_query_nick (NULL, NULL, NULL);
		}
		else
			userage (xcmd->name, xcmd->qhelp);
	}
	update_input (UPDATE_ALL);
	message_from (NULL, LOG_CRAP);
}

void
cmd_quit (struct command *cmd, char *args)
{
	int old_server = from_server;
	char *Reason;
	DCC_list *Client;
	int active_dcc = 0;

	if (args && *args)
		Reason = args;
	else
		Reason = get_signoffreason (get_server_nickname (from_server));
	if (!Reason || !*Reason)
		Reason = (char *) PACKAGE_VERSION;

	for (Client = ClientList; Client; Client = Client->next)
	{
		if (((Client->flags & DCC_TYPES) == DCC_REGETFILE) || ((Client->flags & DCC_TYPES) == DCC_FILEREAD) || ((Client->flags & DCC_TYPES) == DCC_FILEOFFER) || ((Client->flags & DCC_TYPES) == DCC_RESENDOFFER))
			active_dcc++;
		if ((Client->flags & DCC_TYPES) == DCC_FTPGET)
			active_dcc++;
	}

	if (active_dcc)
		add_wait_prompt ("Active DCC's. Really Quit [y/N] ? ", real_quit, Reason, WAIT_PROMPT_LINE);
	else
	{
		from_server = old_server;
		irc_exit (Reason, convert_output_format (get_fset_var (FORMAT_SIGNOFF_FSET), "%s %s %s %s", update_clock (GET_TIME), get_server_nickname (get_window_server (0)), m_sprintf ("%s@%s", username, hostname), Reason));
	}
}

void
cmd_quote (struct command *cmd, char *args)
{
	if (!in_on_who && !doing_privmsg && args && *args)
		send_to_server ("%s", args);
}

void
cmd_reconnect (struct command *cmd, char *args)
{
	char scommnd[6];

	if (from_server == -1)
	{
		bitchsay ("Try connecting to a server first.");
		return;
	}

	if (do_hook (DISCONNECT_LIST, "Reconnecting to server"))
		put_it ("%s", convert_output_format ("$G Reconnecting to server %K[%W$1%K]", "%s %d", update_clock (GET_TIME), from_server));

	snprintf (scommnd, 5, "+%i", from_server);
	close_server (from_server, (args && *args) ? args : "Reconnecting");
	clean_whois_queue ();
	window_check_servers ();
	t_parse_command ("SERVER", scommnd);
}

void
cmd_reset (struct command *cmd, char *args)
{
	refresh_screen (0, NULL);
}

void
cmd_generic_ch (struct command *cmd, char *args)
{
	char *name = cmd->rname ? cmd->rname : cmd->name;
	char *ptr, *s;

	ptr = next_arg (args, &args);

	if (ptr && !strcmp (ptr, "*"))
	{
		if ((s = get_current_channel_by_refnum (0)) != NULL)
			send_to_server ("%s %s %s", name, s, args ? args : empty_str);
		else
			say ("%s * is not valid since you are not on a channel", name);
	}
	else if (ptr)
		send_to_server ("%s %s %s", name, ptr, args ? args : empty_str);
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_generic (struct command *cmd, char *args)
{
	char *name = cmd->rname ? cmd->rname : cmd->name;
	send_to_server ("%s %s", name, args ? args : "");
}

void
cmd_hook (struct command *cmd, char *args)
{
	if (args && *args)
		do_hook (HOOK_LIST, "%s", args);
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_send_text (struct command *cmd, char *args)
{
	char *tmp;
	char *c = NULL;

	if (cmd->rname)
	{
		tmp = get_current_channel_by_refnum (0);
	}
	else
	{
		tmp = get_target_by_refnum (0);
		c = get_target_cmd_by_refnum (0);
	}
	if (c)
	{
		if (!my_stricmp (c, "SAY") || !my_stricmp (c, "PRIVMSG") || !my_stricmp (c, "MSG"))
			send_text (tmp, args, NULL, 1, 1);
		else if (!my_stricmp (c, "NOTICE"))
			send_text (tmp, args, "NOTICE", 1, 1);

	}
	else
		send_text (tmp, args, NULL, 1, 1);
}

void
cmd_server (struct command *cmd, char *args)
{
	char *server = NULL;
	int i;

	if (!(server = next_arg (args, &args)))
	{
		display_server_list ();
		return;
	}

	/*
	 * Delete an existing server
	 */
	if (!my_strnicmp (server, "-DELETE", strlen (server)))
	{
		if ((server = next_arg (args, &args)) != NULL)
		{
			if ((i = parse_server_index (server)) == -1)
			{
				if ((i = find_in_server_list (server, 0)) == -1)
				{
					say ("No such server in list");
					return;
				}
			}
			if (server_list[i].connected)
			{
				say ("Can not delete server that is already open");
				return;
			}
			remove_from_server_list (i);
		}
		else
			say ("Need server number for -DELETE");
	}
	/*
	 * Add a server, but dont connect
	 */
	else if (!my_strnicmp (server, "-ADD", strlen (server)))
	{
		if ((server = new_next_arg (args, &args)))
			(void) find_server_refnum (server, &args);
		else
			say ("Need server info for -ADD");
	}
	/*
	   * The difference between /server +foo.bar.com  and
	   * /window server foo.bar.com is that this can support
	   * doing a server number.  That makes it a tad bit more
	   * difficult to parse, too. :P  They do the same thing,
	   * though.
	 */
	else if (*server == '+')
	{
		if (*++server)
		{
			i = find_server_refnum (server, &args);
			if (!connect_to_server_by_refnum (i, -1))
				set_window_server (0, i, 0);
		}
		else
			get_connected (primary_server + 1);
	}
	/*
	 * You can only detach a server using its refnum here.
	 */
	else if (*server == '-')
	{
		if (*++server)
		{
			i = find_server_refnum (server, &args);
			if (i == primary_server)
			{
				say ("You can't close your primary server!");
				return;
			}
			close_server (i, "closing server");
			window_check_servers ();
		}
		else
			get_connected (primary_server - 1);
	}
	/*
	 * Just a naked /server with no flags
	 */
	else
	{
		i = find_server_refnum (server, &args);
		if (!connect_to_server_by_refnum (i, primary_server))
		{
			change_server_channels (primary_server, from_server);
			if (primary_server > -1 && from_server != primary_server &&
			    !get_server_away (from_server) && get_server_away (primary_server))
				set_server_away (from_server, get_server_away (primary_server));
			set_window_server (-1, from_server, 1);
		}
	}
}

void
cmd_setenv (struct command *cmd, char *args)
{
	char *env_var;

	if ((env_var = next_arg (args, &args)) != NULL)
		setenv (env_var, args, 1);
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_sping (struct command *cmd, char *args)
{
	char *servern = next_arg (args, &args);

	if (!servern || !*servern)
		servern = get_server_name (from_server);
	if (servern && *servern && match ("*.*", servern))
	{
#ifdef HAVE_GETTIMEOFDAY
		if (!in_sping.tv_sec)
#else
		if (!in_sping)
#endif
		{
			bitchsay ("Sent server ping to [\002%s\002]", servern);
			send_to_server ("VERSION %s", servern);
#ifdef HAVE_GETTIMEOFDAY
			gettimeofday (&in_sping, NULL);
#else
			in_sping = time (NULL);
#endif
		}
		else
			bitchsay ("You may only ping one server at a time");
	}
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_squit (struct command *cmd, char *args)
{
	char *srv1 = next_arg (args, &args);
	char *srv2 = empty_str;

	if (args && srv1)
	{
		srv2 = next_arg (args, &args);
		if (args)
		{
			userage (cmd->name, cmd->qhelp);
			return;
		}
	}

	if (srv1)
		send_to_server ("%s %s %s", cmd->name, srv1, srv2);
	else
		userage (cmd->name, cmd->qhelp);
}

void
cmd_stats (struct command *cmd, char *args)
{
	char *flags = NULL, *serv = NULL;
	char *new_flag = empty_str;
	char *str = NULL;

	server_list[from_server].stats_flags = 0;
	flags = next_arg (args, &args);
	if (flags)
	{
		switch (*flags)
		{
		case 'o':
			server_list[from_server].stats_flags |= STATS_OLINE;
			str = "%GType   Host                      Nick";
			break;
		case 'y':
			server_list[from_server].stats_flags |= STATS_YLINE;
			str = "%GType   Class  PingF  ConnF  MaxLi      SendQ";
			break;
		case 'i':
			server_list[from_server].stats_flags |= STATS_ILINE;
			str = "%GType   Host                      Host                       Port Class";
			break;
		case 'h':
			server_list[from_server].stats_flags |= STATS_HLINE;
			str = "%GType    Server Mask     Server Name";
			break;
		case 'l':
			server_list[from_server].stats_flags |= STATS_LINK;
			str = "%Glinknick|server               sendq smsgs sbytes rmsgs rbytes timeopen";
			break;
		case 'c':
			server_list[from_server].stats_flags |= STATS_CLASS;
			str = "%GType   Host                      Host                       Port Class";
			break;
		case 'u':
			server_list[from_server].stats_flags |= STATS_UPTIME;
			break;
		case 'k':
			server_list[from_server].stats_flags |= STATS_KLINE;
			str = "%GType   Host                      UserName              Reason";
			break;
		case 'm':
			server_list[from_server].stats_flags |= STATS_MLINE;
		default:
			break;
		}
		new_flag = flags;
	}
	if (args && *args)
		serv = args;
	else
		serv = get_server_itsname (from_server);
	if (str)
		put_it ("%s", convert_output_format (str, NULL, NULL));
	send_to_server ("%s %s %s", cmd->name, new_flag, serv);
}

void
cmd_showidle (struct command *cmd, char *args)
{
	struct channel *tmp;
	char *channel = NULL;
	int count = 0;
	struct nick_list *nick;
	time_t ltime;
	int server;

	if (args && *args)
		channel = next_arg (args, &args);
	if (!(tmp = prepare_command (&server, channel, NO_OP)))
		return;

	put_it ("%s", convert_output_format ("%G+-[ %CIdle check for %W$0%n %G]-----------------------------------", "%s", tmp->channel));
	for (nick = next_nicklist (tmp, NULL); nick; nick = next_nicklist (tmp, nick))
	{
		ltime = time (NULL) - nick->idle_time;
		put_it ("%s", convert_output_format ("%G| %n$[20]0 Idle%W: %K[%n$1- %K]", "%s %s", nick->nick, convert_time (ltime)));
		count++;
	}
}

void
cmd_topic (struct command *cmd, char *args)
{
	char *arg = NULL;
	char *arg2;
	struct channel *chan;
	int server;

	arg = next_arg (args, &args);
	if (!(chan = prepare_command (&server, arg ? (is_channel (arg) ? arg : NULL) : NULL, NO_OP)))
		return;
	if (*cmd->name == 'U')
	{
		my_send_to_server (server, "TOPIC %s :", chan->channel);
		return;
	}
	if (arg && (!(chan->mode & MODE_TOPIC) || chan->chop))
	{
		if (is_channel (arg))
		{
			if ((arg2 = next_arg (args, &args)))
				my_send_to_server (server, "TOPIC %s :%s %s", chan->channel, arg2, args);
			else
				my_send_to_server (server, "TOPIC %s", chan->channel);
		}
		else
		{
			char *p = NULL;
			p = m_sprintf ("%s%s%s", arg, arg ? space_str : empty_str, args ? args : empty_str);
			my_send_to_server (server, "TOPIC %s :%s%s%s", chan->channel, arg, args ? space_str : empty_str, args ? args : empty_str);
			new_free (&p);
		}
	}
	else
		my_send_to_server (server, "TOPIC %s", chan->channel);
}

void
cmd_trace (struct command *cmd, char *args)
{
	char *flags = NULL, *serv = NULL;

	server_list[from_server].trace_flags = 0;
	while (args && *args)
	{
		flags = next_arg (args, &args);
		if (flags && *flags == '-')
		{
			switch (toupper (*(flags + 1)))
			{
			case 'O':
				server_list[from_server].trace_flags |= TRACE_OPER;
				break;
			case 'U':
				server_list[from_server].trace_flags |= TRACE_USER;
				break;
			case 'S':
				server_list[from_server].trace_flags |= TRACE_SERVER;
			default:
				break;
			}
			continue;
		}
		if (flags && *flags != '-')
			serv = flags;
		else
			serv = next_arg (args, &args);
	}
	if (server_list[from_server].trace_flags & TRACE_OPER)
		bitchsay ("Tracing server %s%sfor Operators", serv ? serv : empty_str, serv ? " " : empty_str);
	if (server_list[from_server].trace_flags & TRACE_USER)
		bitchsay ("Tracing server %s%sfor Users", serv ? serv : empty_str, serv ? " " : empty_str);
	if (server_list[from_server].trace_flags & TRACE_SERVER)
		bitchsay ("Tracing server %s%sfor servers", serv ? serv : empty_str, serv ? " " : empty_str);
	send_to_server ("%s%s%s", cmd->name, serv ? " " : empty_str, serv ? serv : empty_str);
}

void
cmd_unalias (struct command *cmd, char *args)
{
	char *c;

	if (args && (c = next_arg (args, &args)))
	{
		upper (c);

		if (t_unbind (c) < 0)
			bitchsay ("Error removing command");
	}
}

static void
userhost_ignore (WhoisStuff * stuff, char *nick1, char *args)
{
	char *p, *arg;
	char *nick = NULL, *user = NULL, *host = NULL;
	int old_window_display;
	char ignorebuf[BIG_BUFFER_SIZE + 1];
	Ignore *igptr, *igtmp;
	struct whowas_list *whowas;

	arg = next_arg (args, &args);
	if (!stuff || !stuff->nick || !nick1 || !strcmp (stuff->user, "<UNKNOWN>") || my_stricmp (stuff->nick, nick1))
	{
		if ((whowas = check_whowas_nick_buffer (nick1, arg, 0)))
		{
			bitchsay ("Using WhoWas info for %s of %s ", arg, nick1);
			user = host;
			host = strchr (host, '@');
			*host++ = 0;
			nick = whowas->nicklist->nick;
		}
		else
		{
			say ("No match for user %s", nick1);
			return;
		}
	}
	else
	{
		user = clear_server_flags (stuff->user);
		host = stuff->host;
		nick = stuff->nick;
	}
	if (!my_stricmp (arg, "+HOST"))
		sprintf (ignorebuf, "*!*@%s ALL -CRAP -PUBLIC", cluster (host));
	else if (!my_stricmp (arg, "+USER"))
		sprintf (ignorebuf, "*%s@%s ALL -CRAP -PUBLIC", user, cluster (host));
	else if (!my_stricmp (arg, "-USER") || !my_stricmp (arg, "-HOST"))
	{
		int found = 0;
		if (!my_stricmp (arg, "-HOST"))
			sprintf (ignorebuf, "*!*@%s", cluster (host));
		else
			sprintf (ignorebuf, "%s!%s@%s", nick, user, host);
		igptr = ignored_nicks;
		while (igptr != NULL)
		{
			igtmp = igptr->next;
			if (match (igptr->nick, ignorebuf) ||
			    match (nick, igptr->nick))
			{
				sprintf (ignorebuf, "%s NONE", igptr->nick);
				old_window_display = window_display;
				window_display = 0;
				t_parse_command ("IGNORE", ignorebuf);
				window_display = old_window_display;
				bitchsay ("Unignored %s!%s@%s", nick, user, host);
				found++;
			}
			igptr = igtmp;
		}
		if (!found)
			bitchsay ("No Match for ignorance of %s", nick);
		return;
	}
	old_window_display = window_display;
	window_display = 0;
	t_parse_command ("IGNORE", ignorebuf);
	if ((arg = next_arg (args, &args)))
	{
		char tmp[BIG_BUFFER_SIZE + 1];
		sprintf (tmp, "%s ^IGNORE %s NONE", arg, ignorebuf);
		t_parse_command ("TIMER", tmp);
	}
	window_display = old_window_display;
	if ((p = strchr (ignorebuf, ' ')))
		*p = 0;
	say ("Now ignoring ALL except CRAP and PUBLIC from %s", ignorebuf);
	return;
}

void
cmd_doig (struct command *cmd, char *args)
{
	char *nick;
	static char ignore_type[6];
	int got_ignore_type = 0;
	int need_time = 0;

	if (!args || !*args)
		goto bad_ignore;


	while ((nick = next_arg (args, &args)))
	{
		if (!nick || !*nick)
			goto bad_ignore;


		if (*nick == '-' || *nick == '+')
		{
			if (!my_stricmp (nick, "-USER") || !my_stricmp (nick, "+HOST") || !my_stricmp (nick, "+USER") || !my_stricmp (nick, "-HOST"))
				strcpy (ignore_type, nick);
			if (!args || !*args)
				goto bad_ignore;
			++got_ignore_type;
			continue;
		}
		else if (!got_ignore_type)
		{
			if (!my_strnicmp (cmd->name, "IGH", 3))
				strcpy (ignore_type, "+HOST");
			else if (!my_strnicmp (cmd->name, "IG", 2))
				strcpy (ignore_type, "+USER");
			if (!my_strnicmp (cmd->name, "UNIGH", 5))
				strcpy (ignore_type, "-HOST");
			else if (!my_strnicmp (cmd->name, "UNIG", 4))
				strcpy (ignore_type, "-USER");
			if (toupper (cmd->name[strlen (cmd->name) - 1]) == 'T')
				need_time++;
		}
		if (need_time)
			add_to_userhost_queue (nick, userhost_ignore, "%s %d", ignore_type, get_int_var (IGNORE_TIME_VAR) * 60);
		else
			add_to_userhost_queue (nick, userhost_ignore, "%s", ignore_type);
	}
	return;
      bad_ignore:
	userage (cmd->name, cmd->qhelp);
}


void
ison_now (char *notused, char *nicklist)
{
	if (do_hook (current_numeric, "%s", nicklist))
		put_it ("%s Currently online: %s", line_thing, nicklist);
}

void
cmd_ison (struct command *cmd, char *args)
{
	if (!args[strspn (args, space_str)])
		args = get_server_nickname (from_server);
	add_ison_to_whois (args, ison_now);
}

void
cmd_info (struct command *cmd, char *args)
{
	say ("We should say something really nice here. But i dont know what.");
	send_to_server ("%s %s", cmd->name, args);
}

void
cmd_invite (struct command *cmd, char *args)
{
	char *inick;
	struct channel *chan = NULL;
	int server = from_server;

	if (args && *args)
	{
		while (1)
		{
			inick = next_arg (args, &args);
			if (!inick)
				return;
			if (args && *args)
			{
				if (!is_channel (args) || !(chan = prepare_command (&server, args, NO_OP)))
					return;
			}
			else if (!(chan = prepare_command (&server, NULL, NO_OP)))
				return;

			if (!chan)
				return;
			my_send_to_server (server, "INVITE %s %s%s%s", inick, chan->channel, chan->key ? " " : "", chan->key ? chan->key : "");
		}
	}
	else
		userage (cmd->name, cmd->qhelp);
	return;
}

void
cmd_ircii_version (struct command *cmd, char *args)
{
	char *host;

	if ((host = next_arg (args, &args)) != NULL)
		send_to_server ("%s %s", cmd->name, host);
	else
	{
		bitchsay ("Client: %s", PACKAGE_VERSION);
		send_to_server ("%s", cmd->name);
	}
}

void
cmd_userhost (struct command *cmd, char *args)
{
	int n = 0, total = 0, userhost_cmd = 0;
	char *nick;
	char buffer[BIG_BUFFER_SIZE + 1];

	while ((nick = next_arg (args, &args)) != NULL)
	{
		int len;

		++total;
		len = strlen (nick);
		if ((*nick == '-' || *nick == '/') && !my_strnicmp (nick + 1, "C", 1))
		{
			if (total < 2)
			{
				userage (cmd->name, cmd->qhelp);
				return;
			}
			userhost_cmd = 1;
			while (my_isspace (*args))
				args++;
			break;
		}
		else
		{
			if (n++)
				strmcat (buffer, space_str, BIG_BUFFER_SIZE);
			else
				*buffer = '\0';
			strmcat (buffer, nick, BIG_BUFFER_SIZE);
		}
	}
	if (n)
	{
		char *the_list = NULL;
		char *s, *t;
		int i;

		malloc_strcpy (&the_list, buffer);
		s = t = the_list;
		while (n)
		{
			for (i = 5; i && *s; s++)
				if (isspace (*s))
					i--, n--;
			if (isspace (s[-1]))
				s[-1] = '\0';
			else
				n--;
			strcpy (buffer, t);
			t = s;

			if (userhost_cmd)
				add_to_whois_queue (buffer, userhost_cmd_returned, "%s", args);
			else
				add_to_whois_queue (buffer, USERHOST_USERHOST, "%s", empty_str);
		}
		new_free (&the_list);
	}
	else if (!total)
		/* Default to yourself.  */
		add_to_whois_queue (get_server_nickname (from_server), USERHOST_USERHOST, "%s", get_server_nickname (from_server));
}

void
cmd_users (struct command *cmd, char *args)
{
	struct channel *chan;
	struct nick_list *nicks;
	char *to, *spec, *rest, *temp1;
	char modebuf[BIG_BUFFER_SIZE + 1];
	char msgbuf[BIG_BUFFER_SIZE + 1];
	int count, ops = 0, msg = 0;
	int hook = 0;
	int server = from_server;

	rest = NULL;
	spec = NULL;
	temp1 = NULL;
	*msgbuf = 0;
	*modebuf = 0;

	if (!(to = next_arg (args, &args)))
		to = NULL;
	if (to && !is_channel (to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command (&server, to, NO_OP)))
		return;

	message_from (chan->channel, LOG_CRAP);
	if (!spec && !(spec = next_arg (args, &args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		temp1 = spec;
		spec = "*!*@*";
	}
	else
		temp1 = next_arg (args, &args);
	ops = 0;
	msg = 0;

	if (((temp1 && (!my_strnicmp (temp1, "-ops", strlen (temp1)))) || (!my_stricmp (cmd->name, "CHOPS"))))
		ops = 1;
	if (((temp1 && (!my_strnicmp (temp1, "-nonops", strlen (temp1)))) || (!my_stricmp (cmd->name, "NOPS"))))
		ops = 2;
	if (ops)
		temp1 = next_arg (args, &args);

	if (temp1)
	{
		if (!my_strnicmp (temp1, "-msg", strlen (temp1)))
			msg = 1;
		else if (!my_strnicmp (temp1, "-notice", strlen (temp1)))
			msg = 2;
		else if (!my_strnicmp (temp1, "-nkill", strlen (temp1)))
			msg = 3;
		else if (!my_strnicmp (temp1, "-kill", strlen (temp1)))
			msg = 4;
		else if (!my_strnicmp (temp1, "-kick", strlen (temp1)))
			msg = 5;
		else if (!my_strnicmp (temp1, "-stats", strlen (temp1)))
			msg = 6;
	}
	if ((msg == 1 || msg == 2) && (!args || !*args))
	{
		say ("No message given");
		message_from (NULL, LOG_CRAP);
		return;
	}

	count = 0;
	switch (msg)
	{
	case 6:
		if (do_hook (STAT_HEADER_LIST, "%s %s %s %s %s", "Nick", "dops", "kicks", "nicks", "publics"))
			put_it ("Nick        dops  kicks  nicks  publics");
		break;
	default:
		break;
	}
	for (nicks = next_nicklist (chan, NULL); nicks; nicks = next_nicklist (chan, nicks))
	{
		sprintf (modebuf, "%s!%s", nicks->nick,
			 nicks->host ? nicks->host : "<UNKNOWN@UNKOWN>");
		if (match (spec, modebuf) && (!ops ||
					    ((ops == 1) && nicks->chanop) ||
					    ((ops == 2) && !nicks->chanop)))
		{
			if (msg == 3)
				count--;
			else if (msg == 4)
			{
				if (!isme (nicks->nick))
					my_send_to_server (server, "KILL %s :%s (%i", nicks->nick,
							   args && *args ? args : get_reason (nicks->nick),
							   count + 1);
				else
					count--;
			}
			else if (msg == 5)
			{
				if (!isme (nicks->nick))
					my_send_to_server (server, "KICK %s %s :%s", chan->channel,
					nicks->nick, (args && *args) ? args :
						  get_reason (nicks->nick));
				else
					count--;
			}
			else if (msg == 6)
			{
				if (my_stricmp (nicks->nick, get_server_nickname (from_server)))
				{
					if (do_hook (STAT_LIST, "%s %d %d %d %d", nicks->nick, nicks->dopcount, nicks->kickcount,
					nicks->nickcount, nicks->floodcount))
						put_it ("%-10s  %4d   %4d   %4d     %4d",
							nicks->nick, nicks->dopcount, nicks->kickcount,
							nicks->nickcount, nicks->floodcount);
				}
			}
			else if (msg == 1 || msg == 2)
			{
				if (count)
					strcat (msgbuf, ",");
				strcat (msgbuf, nicks->nick);
			}
			else
			{
				if (!count && do_hook (USERS_HEADER_LIST, "%s %s %s %s %s %s %s", "Level", "aop", "prot", "Channel", "Nick", "+o", "UserHost"))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_USERS_HEADER_FSET), "%s", chan->channel));

				if ((hook = do_hook (USERS_LIST, "%d %d %d %s %s %s %c",
						     0,
						     0,
						     0,
						 chan->channel, nicks->nick,
						     nicks->host,
						nicks->chanop ? '@' : ' ')))
				{
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_USERS_FSET), "%d %d %d %s %s %s %s",
									  0,
									  0,
									  0,
						 chan->channel, nicks->nick,
								nicks->host,
									     nicks->chanop ? "@" : (nicks->voice ? "v" : "ÿ")));
				}
			}
			count++;
		}
	}
	if (!count)
	{
		if (*cmd->name == 'U')
			say ("No match of %s on %s", spec, chan->channel);
		else
			say ("There are no [\002%s\002] on %s", cmd->name, chan->channel);
	}
	else if (!msg && !hook)
		bitchsay ("End of UserList on %s %d counted", chan->channel, count);

	if (msg && (msg != 3) && (msg != 4) && (msg != 5) && (msg != 6) /*&& (msg != 7) */  && count)
	{
		put_it ("%s", convert_output_format (get_fset_var ((msg == 1) ? FORMAT_SEND_MSG_FSET : FORMAT_SEND_NOTICE_FSET), "%s %s %s", update_clock (GET_TIME), msgbuf, args));
		my_send_to_server (server, "%s %s :%s", (msg == 1) ? "PRIVMSG" : "NOTICE", msgbuf, args);
	}
	message_from (NULL, LOG_CRAP);

}


void cmd_oper_stuff (struct command *cmd, char *args)
{
	char *use = cmd->rname ? cmd->rname : cmd->name;

	if ( cmd->data && (!args || !*args))
	{
		userage (cmd->name, cmd->qhelp);
		return;
	}
	if (!get_server_operator (current_screen->current_window->server))
	{
		yell ("You arn't worthy enough to use /%s!", cmd->name);
		return;
	}
	
	message_from (NULL, LOG_WALLOP);
	if (!in_on_who)
		send_to_server ("%s :%s", use, args);

	message_from (NULL, LOG_CRAP);
}

void
cmd_whois_lm (struct command *cmd, char *args)
{
	if (recv_nick)
		send_to_server ("WHOIS %s", recv_nick);
	else
		bitchsay ("Nobody has messaged you yet");
	return;
}

void
cmd_whois_i (struct command *cmd, char *args)
{
	char *channel = NULL;

	if (args && *args)
	{
		channel = next_arg (args, &args);
		send_to_server ("WHOIS %s %s", channel, channel);
	}
	else
		send_to_server ("WHOIS %s %s", get_server_nickname (from_server),
				get_server_nickname (from_server));
}

void
cmd_whois (struct command *cmd, char *args)
{
	if (args && *args)
		send_to_server ("WHOIS %s", args);
	else
		send_to_server ("WHOIS %s", get_server_nickname (from_server));
}

void
cmd_wholeft (struct command *cmd, char *args)
{
	show_wholeft (NULL);
}

void
cmd_whowas (struct command *cmd, char *args)
{
	char *stuff = NULL;
	char *word_one = next_arg (args, &args);

	if (args && *args)
		malloc_sprintf (&stuff, "%s %s", word_one, args);
	else if (word_one && *word_one)
		malloc_sprintf (&stuff, "%s %d", word_one, /*get_int_var(NUM_OF_WHOWAS_VAR) */ 4);
	else
		malloc_sprintf (&stuff, "%s %d", get_server_nickname (from_server), /*get_int_var(NUM_OF_WHOWAS_VAR) */ 4);

	send_to_server ("WHOWAS %s", stuff);
	new_free (&stuff);
}


/* 
 * Commands in other files
 *
 */

void cmd_help (struct command *, char *);	/* in help.c */
void cmd_window (struct command *, char *);	/* in window.c */
void cmd_awaylog (struct command *, char *);	/* in lastlog.c */
void cmd_lastlog (struct command *, char *);	/* in lastlog.c */
void cmd_parsekey (struct command *cmd, char *args);	/* in keys.c */
void cmd_bind (struct command *cmd, char *args);	/* in keys.c */
void cmd_rbind (struct command *cmd, char *args);	/* in keys.c */
void cmd_type (struct command *cmd, char *args);	/* in keys.c */
void cmd_set (struct command *cmd, char *args);		/* in vars.c */
void cmd_remove_log (struct command *cmd, char *args);	/* in readlog.c */
void cmd_readlog (struct command *cmd, char *args);	/* in readlog.c */
void cmd_exec (struct command *cmd, char *args);	/* in exec.c */
void cmd_history (struct command *cmd, char *args);	/* in history.c */
void cmd_notify (struct command *cmd, char *args);	/* in notify.c */
void cmd_timer (struct command *cmd, char *args);	/* in timer.c */
void cmd_ignore (struct command *cmd, char *args);	/* in ignore.c */
void cmd_tignore (struct command *cmd, char *args);	/* in ignore.c */
void cmd_no_flood (struct command *cmd, char *args);	/* in flood.c */
void cmd_who (struct command *cmd, char *args);		/* in cmd_who.c */
void cmd_hostname (struct command *cmd, char *args);	/* in cmd_hostname.c */
void cmd_scan (struct command *cmd, char *args);	/* in cmd_scan.c */
void cmd_show_hash (struct command *cmd, char *args);	/* in hash.c */
void cmd_chat (struct command *cmd, char *args);	/* in dcc.c */
void cmd_fset (struct command *cmd, char *args);	/* in fset.c */
void cmd_save (struct command *cmd, char *args);	/* in save.c */
void cmd_deop (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_deoper (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_op (struct command *cmd, char *args);		/* in cmd_modes.c */
void cmd_oper (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_umode (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_unkey (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_kick (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_kill (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_unban (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_kickban (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_ban (struct command *cmd, char *args);		/* in cmd_modes.c */
void cmd_tban (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_banstat (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_bantype (struct command *cmd, char *args);	/* in cmd_modes.c */
void cmd_orig_nick (struct command *cmd, char *args);	/* in cmd_orignick.c */


struct command xaric_cmds[] =
{
	{"ABORT", NULL, NULL, cmd_abort, "- Saves Xaric settings then exits IRC"},
	{"ABOUT", NULL, NULL, cmd_about, "- Info about Xaric"},
	{"ADMIN", NULL, NULL, cmd_generic, "%R[%nserver%R]%n\n- Shows the iRC Administration information on current server or %R[%nserver%R]%n"},
	{"ALIAS", NULL, NULL, cmd_alias, "- Make one command operate like another"},
	{"AWAY", NULL, NULL, cmd_away, "%R[%nreason%R]%n\n- Sets you away on server if %R[%nreason%R]%n else sets you back"},
	{"AWAYLOG", NULL, NULL, cmd_awaylog, "%R[%nALL|MSGS|NOTICES|...%R]%n\n- D isplays or changes what you want logged in your lastlog. This is a comma separated list"},
	{"B", NULL, NULL, cmd_ban, "See %YBAN%n"},
	{"BACK", NULL, NULL, cmd_back, "- Sets you back from being away"},
	{"BAN", NULL, NULL, cmd_ban, "%Y<%Cnick%G|%Cnick%G!%nuser%y@%nhostname%Y>%n\n- Ban %Y<%Cnick%G|%Cnick%G!%nuser%y@%nhostname%Y>%n from current channel"},
	{"BANSTAT", NULL, NULL, cmd_banstat, "%R[%Bchannel%R]%n\n- Show bans on current channel or %R[%Bchannel%R]%n"},
	{"BANTYPE", NULL, NULL, cmd_bantype, "%W/%nbantype %Y<%nNormal%G|%nBetter%G|%nHost%G|%nDomain%G |%nScrew%Y>%n\n- When a ban is done on a nick, it uses %Y<%nbantype%Y>%n"},
	{"BIND", NULL, NULL, cmd_bind, "- Command to bind a key to a function"},
	{"BK", NULL, NULL, cmd_kickban, "%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, bans and kicks %Y<%Cnick%Y>%n for %R[%nreason%R]%n"},
	{"BYE", NULL, NULL, cmd_quit, "- Quit Irc"},
	{"CHAT", NULL, NULL, cmd_chat, "%Y<%nNick%Y>%n\n- Attempts to dcc chat nick"},
	{"CHOPS", NULL, NULL, cmd_users, "%R[%Bchannel%R]%n\n- Shows, in a full format, all the nicks with op status"},
	{"CL", NULL, NULL, cmd_clear, "- Clears the screen"},
	{"CLEAR", NULL, NULL, cmd_clear, "- Clears the screen"},
	{"CLEARTAB", NULL, NULL, cmd_clear_tab, "- Clears the nicks in the tabkey list"},
	{"CLOSE", NULL, NULL, cmd_oper_stuff, "Requires irc operator status. Close any connections from clients who have not fully registered yet."},   
	{"CONNECT", NULL, NULL, cmd_oper_stuff, "%Y<%nserver1%Y>%n %Y<%nport%Y>%n %R[%nserver2%R]%n\n%Y*%n Requires irc operator status"},
	{"CTCP", NULL, NULL, cmd_ctcp, "%Y<%Cnick%Y>%n %Y<%nrequest%Y>%n\n- CTCP sends %Y<%Cnick%Y>%n with %Y<%nrequest%Y>%n"},
	{"CYCLE", NULL, NULL, cmd_cycle, "%R[%Bchannel%R]%n\n- Leaves current channel or %R[%Bchannel%R]%n and immediately rejoins"},
	{"D", NULL, NULL, cmd_describe, "See %YDESCRIBE%n"},
	{"DATE", "TIME", NULL, cmd_generic, "- Shows current time and date from current server"},
	{"DBAN", NULL, NULL, cmd_unban, "- Clears all bans on current channel"},
	{"DC", NULL, NULL, cmd_dcc_chat, "%Y<%Cnick%Y>%n\n- Starts a DCC CHAT to %Y<%Cnick%Y>%n"},
	{"DCC", NULL, NULL, cmd_dcc, "try /dcc help"},
	{"DEBUGHASH", NULL, NULL, cmd_show_hash, NULL},
	{"DEOP", NULL, NULL, cmd_deop, "%Y<%C%nnick(s)%Y>%n\n- Deops %Y<%Cnick%y(%Cs%y)%Y>%n"},
	{"DEOPER", NULL, NULL, cmd_deoper, "%Y*%n Requires irc operator status\n- Removes irc operator status"},
	{"DESCRIBE", NULL, NULL, cmd_describe, "%Y<%Cnick%G|%Bchannel%Y>%n %Y<%naction%Y>%n\n- Describes to %Y<%Cnick%G|%Bchannel%Y>%n with %Y<%naction%Y>%n"},
	{"DEVOICE", "DeVoice", NULL, cmd_deop, "%Y<%C%nnick(s)%Y>%n\n- de-voices %Y<%Cnick%y(%Cs%y)%Y>%n"},
#ifdef XARIC_DEBUG
	{"DEBUG", NULL, NULL, cmd_debug, NULL},
#endif
	{"DIE", NULL, NULL, cmd_generic, "%Y*%n Requires irc operator status\n- Kills the IRC server you are on"},
	{"DISCONNECT", NULL, NULL, cmd_disconnect, "- Disconnects you from the current server"},
	{"DNS", NULL, NULL, cmd_nslookup, "%Y<%nnick|hostname%y>%n\n- Attempts to nslookup on nick or hostname"},
	{"DOP", NULL, NULL, cmd_deop, "- See deop"},
	{"ECHO", NULL, NULL, cmd_echo, NULL},
	{"EXEC", NULL, NULL, cmd_exec, "%Y<%ncommand%Y>%n\n- Executes %Y<%ncommand%Y>%n with the shell set from %PSHELL%n"},
	{"EXIT", NULL, NULL, cmd_quit, "- Quits IRC"},
	{"FLUSH", NULL, NULL, cmd_flush, "- Flush ALL server output"},
	{"FRESET", NULL, NULL, cmd_freset, NULL},
	{"FSET", NULL, NULL, cmd_fset, NULL},
	{"HASH", NULL, NULL, cmd_generic, "- Shows some stats about ircd's internal hashes."},
	{"HELP", NULL, NULL, cmd_help, "%Y<%nindex%Y|%ncommand%Y>%n\n- Show an index of commands or get help on a specific command"},
	{"HISTORY", NULL, NULL, cmd_history, "- Shows recently typed commands"},
	{"HOSTNAME", NULL, NULL, cmd_hostname, "%Y<%nhostname%Y>%n\n- Shows list of possible hostnames with option to change it on virtual hosts"},
	{"HOOK", NULL, NULL, cmd_hook, "- View / delete / add a hook"},
	{"HTM", NULL, NULL, cmd_oper_stuff, "- manipulate ircd's High Traffic Mode. Requires irc operator status"},
	{"I", NULL, NULL, cmd_invite, "- See %YINVITE%n"},
	{"IG", NULL, NULL, cmd_doig, "+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of nick!host matching %Y<%Cnick%Y>%n"},
	{"IGH", NULL, NULL, cmd_doig, "+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of hostname matching %Y<%Cnick%Y>%n"},
	{"IGHT", NULL, NULL, cmd_doig, "+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of hostname matching %Y<%Cnick%Y>%n and expires this on a timer"},
	{"IGT", NULL, NULL, cmd_doig, "+%G|%n-%Y<%Cnick%Y>%n\n- Ignores ALL except crap and public of nick!host matching %Y<%Cnick%Y>%n and expires this on a timer"},
	{"IGNORE", NULL, NULL, cmd_ignore, NULL},
	{"ISON", NULL, NULL, cmd_ison, NULL},
	{"INFO", NULL, NULL, cmd_info, "- Shows current client info"},
	{"INVITE", NULL, NULL, cmd_invite, "%Y<%Cnick%Y>%n %R[%Bchannel%R]%n\n- Invites %Y<%Cnick%Y>%n to current channel or %R[%Bchannel%R]%n"},
	{"J", NULL, NULL, cmd_join, "%Y<%nchannel%Y> %R[%nkey%R]%n\n- Joins a %Y<%nchannel%Y>"},
	{"JOIN", NULL, NULL, cmd_join, "%Y<%nchannel%Y> %R[%nkey%R]%n\n- Joins a %Y<%nchannel%Y>"},
	{"K", NULL, NULL, cmd_kick, "%Y[<%Bchannel%G|%Y*>]%n %Y<%Cnick%Y>%n %R[%nreason%R]%n"},
	{"KB", NULL, NULL, cmd_kickban, "%Y<%Cnick%Y>%n %R[%nreason%R]%n\n- Deops, kicks and bans %Y<%Cnick%Y>%n for %R[%nreason%R]%n"},
	{"KICK", NULL, NULL, cmd_kick, "%Y[<%Bchannel%G|%Y*>]%n %Y<%Cnick%Y>%n %R[%nreason%R]%n"},
	{"KILL", NULL, NULL, cmd_kill, "%Y<%Cnick%Y>%n %R[%nreason%R]%n\n%Y*%n Requires irc operator status\n- Kills %Y<%Cnick%Y>%n for %R[%nreason%R]%n"},
	{"L", NULL, NULL, cmd_part, "- See %YLEAVE"},
	{"LASTLOG", NULL, NULL, cmd_lastlog, "-file filename%G|%n-clear%G|%n-max #%G|%n-liternal pat%G|%n-beep%G|%nlevel"},
	{"LEAVE", NULL, NULL, cmd_part, "%Y<%Bchannel%Y>%n\n- Leaves current channel or %Y<%Bchannel%Y>%n"},
	{"LINKS", NULL, NULL, cmd_generic, "- Shows servers and links to other servers"},
	{"LIST", NULL, NULL, cmd_generic, "- Lists all channels"},
	{"LLOOK", NULL, NULL, cmd_linklook, "%Y*%n Requires set %YLLOOK%n %RON%n\n- Lists all the servers which are current split from the IRC network"},
	{"LOCOPS", NULL, empty_str, cmd_oper_stuff, "\n%Y<%Cmessage%Y>%n Requires irc operator status. Sends a message to all local operators.%n"},
	{"LUSERS", NULL, NULL, cmd_generic, "- Shows stats on current server"},
	{"M", "PRIVMSG", NULL, cmd_privmsg, "See %YMSG%n"},
	{"MAP", NULL, NULL, cmd_map, "- Displays a map of the current servers"},
	{"ME", NULL, NULL, cmd_me, "<action>\n- Sends an action to current window"},
	{"MSG", "PRIVMSG", NULL, cmd_privmsg, "%Y<%Cnick%Y>%n %Y<%ntext%Y>%n\n- Send %Y<%Cnick%Y>%n a message with %Y<%ntext%Y>%n"},
	{"MODE", NULL, NULL, cmd_generic_ch, "- Set modes on IRC"},
	{"MOTD", NULL, NULL, cmd_generic, "%R[%nserver%R]%n\n- Shows MOTD on current server %R[%nserver%R]%n"},
	{"MORE", NULL, NULL, cmd_readlog, "%R[%nfilename%R]%n\n- displays file in pages"},
	{"MUB", NULL, NULL, cmd_unban, "- Mass unbans current channel"},
	{"NAMES", NULL, NULL, cmd_generic, "%R[%Bchannel%R]%n\n- Shows names on current channel or %R[%Bchannel%R]%n"},
	{"NICK", "NICK", NULL, cmd_nick, "-Changes nick to specified nickname"},
	{"NOCHAT", NULL, NULL, cmd_chat, "%Y<%nnick%Y>%n\n- Kills chat reqest from %Y<%nnick%Y>"},
	{"NOTE", NULL, NULL, cmd_generic, "- send a note to someone, if the irc server supports it."},
	{"NOTICE", "NOTICE", NULL, cmd_privmsg, "%Y<%Cnick%G|%Bchannel%Y> %Y<%ntext%Y>%n\n- Sends a notice to %Y<%Cnick%G|%Bchannel%Y> with %Y<%ntext%Y>%n"},
	{"NOTIFY", NULL, NULL, cmd_notify, "%Y<%Cnick|+|-nick%Y>%n\n- Adds/displays/removes %Y<%Cnick%Y>%n to notify list"},
	{"NOPS", NULL, NULL, cmd_users, "%R[%Bchannel%R]%n\n- Shows, in a full format, all the nicks without ops in %R[%Bchannel%R]%n or channel"},
	{"NOFLOOD", NULL, NULL, cmd_no_flood, "%R[%n-%G|%n+%R]%Y<%Cnick%Y>%n\n - Adds or removes %Cnick%n to your no flood list"},
	{"NSLOOKUP", NULL, NULL, cmd_nslookup, "%Y<%nhostname%Y>%n\n- Returns the IP adress and IP number for %Y<%nhostname%Y>%n"},
	{"NWHOWAS", NULL, NULL, cmd_nwhowas, "- Displays internal whowas info for all channels. This information expires after 20 minutes for users on internal list, 10 minutes for others"},
	{"OP", NULL, NULL, cmd_op, "%Y<%Cnick%Y>%n\n- Gives %Y<%Cnick%Y>%n +o"},
	{"OPER", NULL, NULL, cmd_oper, "%Y*%n Requires irc operator status\n%Y<%Cnick%Y>%n %R[%npassword%R]%n"},
	{"OPERWALL", NULL, empty_str, cmd_oper_stuff, "\n%Y<%Cmessage%Y>%n Requires irc operator status. Sends a message to operators.%n"},
	{"ORIGNICK", NULL, NULL, cmd_orig_nick, "- Trys to regain old nick"},
	{"PART", NULL, NULL, cmd_part, "- Leaves %Y<%nchannel%Y>%n"},
	{"PARTALL", NULL, NULL, cmd_part, "- Leaves all channels"},
	{"PING", "PING", NULL, cmd_ping, "- send a ping to some dumbass"},
	{"P", "PING", NULL, cmd_ping, "- send a ping to some dumbass"},
	{"Q", NULL, NULL, cmd_query, "%Y<%n-cmd cmdname%Y> <%Cnick%Y>%n\n- Starts a query to %Y<%Cnick%Y>%n"},
	{"QUERY", NULL, NULL, cmd_query, "%Y<%n-cmd cmdname%Y> <%Cnick%Y>%n\n- Starts a query to %Y<%Cnick%Y>%n"},
	{"RBIND", NULL, NULL, cmd_rbind, "- Removes a key binding"},
	{"READLOG", NULL, NULL, cmd_readlog, "- Displays current away log"},
	{"REMLOG", NULL, NULL, cmd_remove_log, "- Removes logfile"},
	{"RECONNECT", NULL, NULL, cmd_reconnect, "- Reconnects you to current server"},
	{"REHASH", NULL, NULL, cmd_oper_stuff, "%Y*%n Requires irc operator status\n- Rehashs ircd.conf for new configuration"},
	{"RESET", NULL, NULL, cmd_reset, "- Fixes flashed terminals"},
	{"RESTART", NULL, NULL, cmd_oper_stuff, "%Y*%n Requires irc operator status\n- Restarts server"},
	{"QUIT", NULL, NULL, cmd_quit, "- Quit IRC"},
	{"QUOTE", NULL, NULL, cmd_quote, "%Y<%ntext%Y>%n\n- Sends text directly to the server"},
	{"SAVEIRC", NULL, NULL, cmd_save, "- Saves ~/.ircrc"},
	{"SAY", empty_str, NULL, cmd_send_text, "-%Y<%ntype%Y>%n\n- Says its argument"},
	{"SC", NULL, NULL, cmd_names, "- See %YNAMES%n"},
	{"SCAN", NULL, NULL, cmd_scan, "%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for all nicks"},
	{"SCANI", NULL, NULL, cmd_scan, "%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for ircops"},
	{"SCANN", NULL, NULL, cmd_scan, "%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for non-ops"},
	{"SCANO", NULL, NULL, cmd_scan, "%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for ops"},
	{"SCANV", NULL, NULL, cmd_scan, "%R[%Bchannel%R]%n\n- Scans %R[%Bchannel%R]%n or current channel for voice"},
	{"SERVER", NULL, NULL, cmd_server, "%Y<%nserver%Y>%n\n- Changes to %Y<%nserver%Y>%n"},
	{"SET", NULL, NULL, cmd_set, "- Set Variables"},
	{"SETENV", NULL, NULL, cmd_setenv, NULL},
	{"SEND", NULL, NULL, cmd_send_text, NULL},
	{"SHOWIDLE", NULL, NULL, cmd_showidle, "%R[%Bchannel%R]%n\n- Shows idle people on current channel or %R[%Bchannel%R]%n"},
	{"SHOWSPLIT", NULL, NULL, cmd_linklook, "- Shows split servers"},
	{"SILENCE", NULL, NULL, cmd_generic, "- Undernet silence command"},
	{"SPING", NULL, NULL, cmd_sping, "%Y<%nserver%Y>%n\n- Checks how lagged it is to %Y<%nserver%Y>%n"},
	{"SQUIT", NULL, NULL, cmd_squit, "%Y<%nserver1%Y>%n %R[%nserver2%R]%n\n%Y*%n Requires irc operator status\n- Disconnects %Y<%nserver1%Y>%n from current server or %R[%nserver2%R]%n"},
	{"STATS", NULL, NULL, cmd_stats, "- Shows some server statistics"},
	{"T", NULL, NULL, cmd_topic, "%Y<%ntext%Y>%n\n- Sets %Y<%ntext%Y>%n as topic on current channel"},
	{"TBAN", NULL, NULL, cmd_tban, "- Interactive channel ban delete"},
	{"TIGNORE", NULL, NULL, cmd_tignore, NULL},
	{"TIME", NULL, NULL, cmd_generic, "- Shows time and date of current server"},
	{"TIMER", NULL, NULL, cmd_timer, NULL},
	{"TOPIC", NULL, NULL, cmd_topic, "%Y<%ntext%Y>%n\n- Sets %Y<%ntext%Y>%n as topic on current channel"},
	{"TRACE", NULL, NULL, cmd_trace, "<argument> <server>\n- Without a specified server it shows the current connections on local server\n- Arguments: -s -u -o   trace for servers, users, ircops"},
	{"TYPE", NULL, NULL, cmd_type, "<string>\n- Processes string as if the user typed it on the keyboard"},
	{"U", NULL, NULL, cmd_users, "- Shows users on current channel"},
	{"UB", NULL, NULL, cmd_unban, "%R[%Cnick%R]%n\n- Removes ban on %R[%Cnick%R]%n"},
	{"UNBAN", NULL, NULL, cmd_unban,"%R[%Cnick%R]%n\n- Removes ban on %R[%Cnick%R]%n"},
	{"UMODE", "MODE", NULL, cmd_umode, "%Y<%nmode%Y>%n\n- Sets %Y<%nmode%Y>%n on yourself"},
	{"UNALIAS", NULL, NULL, cmd_unalias, "- Remove a command"},
	{"UNIG", NULL, NULL, cmd_doig, "%Y<%Cnick%Y>%n\n- UnIgnores %Y<%Cnick%Y>%n"},
	{"UNIGH", NULL, NULL, cmd_doig, "%Y<%Cnick%Y>%n\n- Removes %Y<%Cnick%Y>%n's host from the ignore list"},
	{"UNKEY", NULL, NULL, cmd_unkey, "- Removes channel key from current channel"},
	{"UNTOPIC", NULL, NULL, cmd_topic, "- Unset the channel topic"},
	{"UNVOICE", "Unvoice", NULL, cmd_deop, "- see devoice"},
	{"USER", NULL, NULL, cmd_users, "- Shows users on current channel"},
	{"USERHOST", NULL, NULL, cmd_userhost, NULL},
	{"VER", NULL, NULL, cmd_ctcp_version, "- Request a CTCP VERSION"},
	{"VERSION", NULL, NULL, cmd_ircii_version, "- Gives server and client version (a la ircII)"},
	{"VOICE", "Voice", NULL, cmd_op, "- Gives someone voice (+v) on the channel"},
	{"WALL", NULL, NULL, cmd_chwall, "- Send a message to all channel ops"},
	{"WALLOPS", NULL, empty_str, cmd_oper_stuff, NULL},
	{"WALLCHOPS", NULL, NULL, cmd_chwall, "- Send a message to all channel ops"},
	{"WHOIS", NULL, NULL, cmd_whois, "- Get info on a person"},
	{"WHOLEFT", NULL, NULL, cmd_wholeft, "- Shows who left?"},
	{"WHOWAS", NULL, NULL, cmd_whowas, "- Get info about a person who has left"},
	{"W", NULL, NULL, cmd_who, NULL},
	{"WHO", NULL, NULL, cmd_who, NULL},
	{"WI", NULL, NULL, cmd_whois, "- Get info about a person"},
	{"WII", NULL, NULL, cmd_whois_i, "- Get info about a person"},
	{"WILM", NULL, NULL, cmd_whois_lm, "- Do a whois on the person that last messaged you"},
	{"WW", NULL, NULL, cmd_whowas, "- Get info about a person who has left"},
	{"WINDOW", NULL, NULL, cmd_window, NULL},
	{"XECHO", NULL, NULL, cmd_echo, NULL}
};

extern struct tnode *t_commands;

void
init_commands (void)
{
	t_commands = t_build (t_commands, xaric_cmds, sizeof (xaric_cmds) / sizeof (struct command));
}



/* ---- foo ---- */


/*
 * commands.c: This is really a mishmash of function and such that deal with IRCII
 * commands (both normal and keybinding commands) 
 *
 * Written By Michael Sandrof
 * Portions are based on EPIC.
 * Modified by panasync (Colten Edwards) 1995-97
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Moved here while the new commands is written. 
 */

extern int doing_notice;


/* The maximum number of recursive LOAD levels allowed */
#define MAX_LOAD_DEPTH 10

/* recv_nick: the nickname of the last person to send you a privmsg */
char *recv_nick = NULL;

/* sent_nick: the nickname of the last person to whom you sent a privmsg */
char *sent_nick = NULL;
char *sent_body = NULL;

/* Used to keep down the nesting of /LOADs and to determine if we
 * should activate the warning for /ONs if the NOVICE variable is set.
 */
int load_depth = 0;



/* Used to prevent global messaging */
extern int doing_who;


/*
 * irc_command: all the availble irc commands:  Note that the first entry has
 * a zero length string name and a null server command... this little trick
 * makes "/ blah blah blah" to always be sent to a channel, bypassing queries,
 * etc.  Neato.  This list MUST be sorted.
 */


#if 0
static IrcCommand irc_command[] =
{
	{"LOAD", "LOAD", load, 0, "filename\n- Loads filename as a irc script"},
	{"ON", NULL, oncmd, 0, scripting_command},
	{"SHOOK", "Shook", shook, 0, NULL},
};
#endif


struct target_type
{
	char *nick_list;
	char *message;
	int hook_type;
	char *command;
	char *format;
	int level;
	const char *output;
	const char *other_output;
};


int current_target = 0;

/*
 * The whole shebang.
 *
 * The message targets are parsed and collected into one of 4 buckets.
 * This is not too dissimilar to what was done before, except now i 
 * feel more comfortable about how the code works.
 *
 * Bucket 0 -- Unencrypted PRIVMSGs to nicknames
 * Bucket 1 -- Unencrypted PRIVMSGs to channels
 * Bucket 2 -- Unencrypted NOTICEs to nicknames
 * Bucket 3 -- Unencrypted NOTICEs to channels
 *
 * All other messages (encrypted, and DCC CHATs) are dispatched 
 * immediately, and seperately from all others.  All messages that
 * end up in one of the above mentioned buckets get sent out all
 * at once.
 */
void 
send_text (char *nick_list, char *text, char *command, int hook, int log)
{
	int i, old_server;
	int lastlog_level;
	int is_current = 0;
	static int recursion = 0;
	char *current_nick, *next_nick, *free_nick;
	char *line;
	int old_window_display = window_display;

	struct target_type target[4] =
	{
		{NULL, NULL, SEND_MSG_LIST, "PRIVMSG", "*%s*> %s", LOG_MSG, NULL, NULL},
		{NULL, NULL, SEND_PUBLIC_LIST, "PRIVMSG", "%s> %s", LOG_PUBLIC, NULL, NULL},
		{NULL, NULL, SEND_NOTICE_LIST, "NOTICE", "-%s-> %s", LOG_NOTICE, NULL, NULL},
		{NULL, NULL, SEND_NOTICE_LIST, "NOTICE", "-%s-> %s", LOG_NOTICE, NULL, NULL}
	};


	target[0].output = get_format (FORMAT_SEND_MSG_FSET);
	target[1].output = get_format (FORMAT_SEND_PUBLIC_FSET);
	target[1].other_output = get_format (FORMAT_SEND_PUBLIC_OTHER_FSET);
	target[2].output = get_format (FORMAT_SEND_NOTICE_FSET);
	target[3].output = get_format (FORMAT_SEND_NOTICE_FSET);
	target[3].other_output = get_format (FORMAT_SEND_NOTICE_FSET);

	if (recursion)
		hook = 0;
	window_display = hook;
	recursion++;
	free_nick = next_nick = m_strdup (nick_list);

	while ((current_nick = next_nick))
	{
		if ((next_nick = strchr (current_nick, ',')))
			*next_nick++ = 0;

		if (!*current_nick)
			continue;

		if (*current_nick == '%')
		{
			if ((i = get_process_index (&current_nick)) == -1)
				say ("Invalid process specification");
			else
				text_to_process (i, text, 1);
		}
		/*
		 * This test has to be here because it is legal to
		 * send an empty line to a process, but not legal
		 * to send an empty line anywhere else.
		 */
		else if (!text || !*text)
			;
		else if (*current_nick == '"')
			send_to_server ("%s", text);
		else if (*current_nick == '/')
		{
			line = m_opendup (current_nick, space_str, text, NULL);
			parse_command (line, 0, empty_str);
			new_free (&line);
		}
		else if (*current_nick == '=')
		{
			if (!dcc_active (current_nick + 1) && !dcc_activeraw (current_nick + 1))
			{
				yell ("No DCC CHAT connection open to %s", current_nick + 1);
				continue;
			}

			old_server = from_server;
			from_server = -1;
			if (dcc_active (current_nick + 1))
			{
				dcc_chat_transmit (current_nick + 1, text, text, command);
				if (hook)
					addtabkey (current_nick, "msg");
			}
			else
				dcc_raw_transmit (current_nick + 1, line, command);

			from_server = old_server;
		}
		else
		{
			if (doing_notice)
			{
				say ("You cannot send a message from within ON NOTICE");
				continue;
			}

			if (!(i = is_channel (current_nick)) && hook)
				addtabkey (current_nick, "msg");

			if (doing_notice || (command && !strcmp (command, "NOTICE")))
				i += 2;

			if (target[i].nick_list)
				malloc_strcat (&target[i].nick_list, ",");
			malloc_strcat (&target[i].nick_list, current_nick);
			if (!target[i].message)
				target[i].message = text;
		}
	}

	for (i = 0; i < 4; i++)
	{
		char *copy = NULL;
		if (!target[i].message)
			continue;

		lastlog_level = set_lastlog_msg_level (target[i].level);
		message_from (target[i].nick_list, target[i].level);

		copy = m_strdup (target[i].message);

		/* log it if logging on */
		if (log)
			logmsg (LOG_SEND_MSG, target[i].nick_list, target[i].message, 0);

		if (i == 1 || i == 3)
		{
			char *channel;
			is_current = is_current_channel (target[i].nick_list, from_server, 0);
			if ((channel = get_current_channel_by_refnum (0)))
			{
				struct channel *chan;
				struct nick_list *nick = NULL;
				if ((chan = lookup_channel (channel, from_server, 0)))
				{
					chan->stats_pubs++;
					if ((nick = find_nicklist_in_channellist (get_server_nickname (from_server), chan, 0)))
						nick->stat_pub++;
				}
			}
		}
		else
			is_current = 1;

		if (hook && do_hook (target[i].hook_type, "%s %s", target[i].nick_list, target[i].message))
		{
			if (is_current)
				put_it ("%s", convert_output_format (target[i].output,
								     "%s %s %s %s", update_clock (GET_TIME), target[i].nick_list, get_server_nickname (from_server), copy));
			else
				put_it ("%s", convert_output_format (target[i].other_output,
								     "%s %s %s %s", update_clock (GET_TIME), target[i].nick_list, get_server_nickname (from_server), copy));
		}
		new_free (&copy);
		send_to_server ("%s %s :%s", target[i].command, target[i].nick_list, target[i].message);
		new_free (&target[i].nick_list);
		target[i].message = NULL;
		set_lastlog_msg_level (lastlog_level);
		message_from (NULL, LOG_CRAP);
	}

	if (hook && get_server_away (curr_scr_win->server) && get_int_var (AUTO_UNMARK_AWAY_VAR))
		parse_line (NULL, "AWAY", empty_str, 0, 0);

	new_free (&free_nick);
	window_display = old_window_display;
	recursion--;
}


/* parse_line: This is the main parsing routine.  It should be called in
 * almost all circumstances over parse_command().
 *
 * parse_line breaks up the line into commands separated by unescaped
 * semicolons if we are in non interactive mode. Otherwise it tries to leave
 * the line untouched.
 *
 * Currently, a carriage return or newline breaks the line into multiple
 * commands too. This is expected to stop at some point when parse_command
 * will check for such things and escape them using the ^P convention.
 * We'll also try to check before we get to this stage and escape them before
 * they become a problem.
 *
 * Other than these two conventions the line is left basically untouched.
 */
extern void 
parse_line (char *name, char *org_line, char *args, int hist_flag, int append_flag)
{
	char *line = NULL, *free_line, *stuff, *s, *t;
	int args_flag = 0;


	malloc_strcpy (&line, org_line);
	free_line = line;

	if (!*org_line)
		send_text (get_target_by_refnum (0), NULL, NULL, 1, 1);
	else if (args)
		do
		{
			if (!line || !*line)
			{
				new_free (&free_line);
				return;
			}
			stuff = expand_alias (line, args, &args_flag, &line);
			if (!line && append_flag && !args_flag && args && *args)
				m_3cat (&stuff, " ", args);

			parse_command (stuff, hist_flag, args);
			if (!line)
				new_free (&free_line);
			new_free (&stuff);
		}
		while (line && *line);
	else
	{
		if (load_depth)
			parse_command (line, hist_flag, args);
		else
		{
			while ((s = line))
			{
				if ((t = sindex (line, "\r\n")))
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				parse_command (s, hist_flag, args);
			}
		}
	}
	new_free (&free_line);
	return;
}

/*
 * parse_command: parses a line of input from the user.  If the first
 * character of the line is equal to irc_variable[CMDCHAR_VAR].value, the
 * line is used as an irc command and parsed appropriately.  If the first
 * character is anything else, the line is sent to the current channel or to
 * the current query user.  If hist_flag is true, commands will be added to
 * the command history as appropriate.  Otherwise, parsed commands will not
 * be added. 
 *
 * Parse_command() only parses a single command.  In general, you will want
 * to use parse_line() to execute things.Parse command recognized no quoted
 * characters or anything (beyond those specific for a given command being
 * executed). 
 */
int 
parse_command (char *line, int hist_flag, char *sub_args)
{
	static unsigned int level = 0;
	unsigned int display, old_display_var;
	char *cmdchars, *com, *this_cmd = NULL;
	int add_to_hist, cmdchar_used = 0;
	int noisy = 1;

	if (!line || !*line)
		return 0;

	DEBUG(XD_CMD, 5, "Executing [%d] %s", level, line);
	level++;

	if (!(cmdchars = get_string_var (CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;

	this_cmd = m_strdup (line);

	add_to_hist = 1;
	display = window_display;
	old_display_var = (unsigned) get_int_var (DISPLAY_VAR);
	for (; *line; line++)
	{
		if (*line == '^' && (!hist_flag || cmdchar_used))
		{
			if (!noisy)
				break;
			noisy = 0;
		}
		else if (strchr (cmdchars, *line))
		{
			cmdchar_used++;
			if (cmdchar_used > 2)
				break;
		}
		else
			break;
	}

	if (!noisy)
		window_display = 0;
	com = line;

	/*
	 * always consider input a command unless we are in interactive mode
	 * and command_mode is off.   -lynx
	 */
	if (hist_flag && !cmdchar_used && !get_int_var (COMMAND_MODE_VAR))
	{
		send_text (get_target_by_refnum (0), line, NULL, 1, 1);
		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_str);
		}
		/* Special handling for ' and : */
	}
	else if (*com == '\'' && get_int_var (COMMAND_MODE_VAR))
	{
		send_text (get_target_by_refnum (0), line + 1, NULL, 1, 1);
		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_str);
		}
	}
	else if ((*com == '@') || (*com == '('))
	{
		/* This kludge fixes a memory leak */
		char *l_ptr;
		/*
		 * This new "feature" masks a weakness in the underlying
		 * grammar that allowed variable names to begin with an
		 * lparen, which inhibited the expansion of $s inside its
		 * name, which caused icky messes with array subscripts.
		 *
		 * Anyhow, we now define any commands that start with an
		 * lparen as being "expressions", same as @ lines.
		 */
		if (*com == '(')
		{
			if ((l_ptr = MatchingBracket (line + 1, '(', ')')))
				*l_ptr++ = 0;
		}

		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_str);
		}
	}
	else
	{
		char *rest, *alias = NULL, *alias_name = NULL;
		int alias_cnt = 0;
		if ((rest = (char *) strchr (com, ' ')) != NULL)
			*(rest++) = (char) 0;
		else
			rest = empty_str;

		upper (com);

		if (alias && alias_cnt < 0)
		{
			if (hist_flag && add_to_hist)
			{
				add_to_history (this_cmd);
				set_input (empty_str);
			}
			parse_line (alias_name, alias, rest, 0, 1);
			new_free (&alias_name);
		}
		else
		{
			/* History */
			if (*com == '!')
			{
				if ((com = do_history (com + 1, rest)) != NULL)
				{
					if (level == 1)
					{
						set_input (com);
						update_input (UPDATE_ALL);
					}
					else
						parse_command (com, 0, sub_args);

					new_free (&com);
				}
				else
					set_input (empty_str);
			}
			else
			{
				if (hist_flag && add_to_hist)
				{
					add_to_history (this_cmd);
					set_input (empty_str);
				}

				/* --- Ewww.. well the rest should go away as tcommand gets used --- */
				t_parse_command (com, rest);
			}
			if (alias)
				new_free (&alias_name);
		}
	}
	if (old_display_var != get_int_var (DISPLAY_VAR))
		window_display = get_int_var (DISPLAY_VAR);
	else
		window_display = display;

	new_free (&this_cmd);
	level--;
	return 0;
}


/*
 * load: the /LOAD command.  Reads the named file, parsing each line as
 * though it were typed in (passes each line to parse_command). 
 Right now, this is broken, as it doesnt handle the passing of
 the '-' flag, which is meant to force expansion of expandos
 with the arguments after the '-' flag.  I think its a lame
 feature, anyhow.  *sigh*.
 */

BUILT_IN_COMMAND (load)
{
	FILE *fp;
	char *filename, *expanded = NULL;
	int flag = 0;
	int paste_level = 0;
	char *start, *current_row = NULL, buffer[BIG_BUFFER_SIZE + 1];
	int no_semicolon = 1;
	char *irc_path;
	int display;
	int ack = 0;

	irc_path = get_string_var (LOAD_PATH_VAR);
	if (!irc_path)
	{
		bitchsay ("LOAD_PATH has not been set");
		return;
	}

	if (load_depth == MAX_LOAD_DEPTH)
	{
		bitchsay ("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		return;
	}
	load_depth++;
	status_update (0);

	/* 
	 * We iterate over the whole list -- if we use the -args flag, the
	 * we will make a note to exit the loop at the bottom after we've
	 * gone through it once...
	 */
	while (args && *args && (filename = next_arg (args, &args)))
	{
		/* 
		   If we use the args flag, then we will get the next
		   filename (via continue) but at the bottom of the loop
		   we will exit the loop 
		 */
		if (my_strnicmp (filename, "-args", strlen (filename)) == 0)
		{
			flag = 1;
			continue;
		}
		else if ((expanded = expand_twiddle (filename)))
		{
			if (!(fp = uzfopen (&expanded, irc_path)))
			{
				/* uzfopen emits an error if the file
				 * is not found, so we dont have to. */
				status_update (1);
				load_depth--;
				new_free (&expanded);
				return;
			}
		/* Reformatted by jfn */
/* *_NOT_* attached, so dont "fix" it */
			{
				int in_comment = 0;
				int comment_line = -1;
				int line = 1;
				int paste_line = -1;

				display = window_display;
				window_display = 0;
				current_row = NULL;

				for (;; line++)
				{
					if (fgets (buffer, BIG_BUFFER_SIZE / 2, fp))
					{
						int len;
						char *ptr;

						for (start = buffer; my_isspace (*start); start++)
							;
						if (!*start || *start == '#')
							continue;

						len = strlen (start);
						/*
						 * this line from stargazer to allow \'s in scripts for continued
						 * lines <spz@specklec.mpifr-bonn.mpg.de>
						 */
						/* 
						   If we have \\ at the end of the line, that
						   should indicate that we DONT want the slash to 
						   escape the newline (hop)

						   We cant just do start[len-2] because we cant say
						   what will happen if len = 1... (a blank line)

						   SO.... 
						   If the line ends in a newline, and
						   If there are at least 2 characters in the line
						   and the 2nd to the last one is a \ and,
						   If there are EITHER 2 characters on the line or
						   the 3rd to the last character is NOT a \ and,
						   If the line isnt too big yet and,
						   If we can read more from the file,
						   THEN -- adjust the length of the string
						 */
						while ((start[len - 1] == '\n') &&
						       (len >= 2 && start[len - 2] == '\\') &&
						       (len < 3 || start[len - 3] != '\\') &&
						       (len < (BIG_BUFFER_SIZE / 2)) &&
						       (fgets (&(start[len - 2]), (BIG_BUFFER_SIZE / 2) - len, fp)))
						{
							len = strlen (start);
							line++;
						}

						if (start[len - 1] == '\n')
							start[--len] = '\0';

						while (start && *start)
						{
							char *optr = start;

							/* Skip slashed brackets */
							while ((ptr = sindex (optr, "{};/")) &&
							       ptr != optr && ptr[-1] == '\\')
								optr = ptr + 1;

							/* 
							 * if no_semicolon is set, we will not attempt
							 * to parse this line, but will continue
							 * grabbing text
							 */
							if (no_semicolon)
								no_semicolon = 0;
							else if ((!ptr || (ptr != start || *ptr == '/')) && current_row)
							{
								if (!paste_level)
								{
									parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_str : NULL, 0, 0);
									new_free (&current_row);
								}
								else if (!in_comment)
									malloc_strcat (&current_row, ";");
							}

							if (ptr)
							{
								char c = *ptr;

								*ptr = '\0';
								if (!in_comment)
									malloc_strcat (&current_row, start);
								*ptr = c;

								switch (c)
								{
									/* switch statement tabbed back */
								case '/':
									{
										/* If we're in a comment, any slashes that arent preceeded by
										   a star is just ignored (cause its in a comment, after all >;) */
										if (in_comment)
										{
											/* ooops! cant do ptr[-1] if ptr == optr... doh! */
											if ((ptr > start) && (ptr[-1] == '*'))
											{
												in_comment = 0;
												comment_line = -1;
											}
											else
												break;
										}
										/* We're not in a comment... should we start one? */
										else
										{
											/* COMMENT_BREAKAGE_VAR */
											if ((ptr[1] == '*') && !get_int_var (COMMENT_BREAKAGE_VAR) && (ptr == start))
												/* This hack (at the request of Kanan) makes it
												   so you can only have comments at the BEGINNING
												   of a line, and any midline comments are ignored.
												   This is required to run lame script packs that
												   do ascii art crap (ie, phoenix, textbox, etc) */
											{
												/* Yep. its the start of a comment. */
												in_comment = 1;
												comment_line = line;
											}
											else
											{
												/* Its NOT a comment. Tack it on the end */
												malloc_strcat (&current_row, "/");

												/* Is the / is at the EOL? */
												if (ptr[1] == '\0')
												{
													/* If we are NOT in a block alias, */
													if (!paste_level)
													{
														/* Send the line off to parse_line */
														parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_str : NULL, 0, 0);
														new_free (&current_row);
														ack = 0;	/* no semicolon.. */
													}
													else
														/* Just add a semicolon and keep going */
														ack = 1;	/* tack on a semicolon */
												}
											}


										}
										no_semicolon = 1 - ack;
										ack = 0;
										break;
									}
								case '{':
									{
										if (in_comment)
											break;

										/* If we are opening a brand new {} pair, remember the line */
										if (!paste_level)
											paste_line = line;

										paste_level++;
										if (ptr == start)
											malloc_strcat (&current_row, " {");
										else
											malloc_strcat (&current_row, "{");
										no_semicolon = 1;
										break;
									}
								case '}':
									{
										if (in_comment)
											break;
										if (!paste_level)
											yell ("Unexpected } in %s, line %d", expanded, line);
										else
										{
											--paste_level;

											if (!paste_level)
												paste_line = -1;
											malloc_strcat (&current_row, "}");	/* } */
											no_semicolon = ptr[1] ? 1 : 0;
										}
										break;
									}
								case ';':
									{
										if (in_comment)
											break;
										if ((*(ptr + 1) == '\0') && (!paste_level))
										{
											parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_str : NULL, 0, 0);
											new_free (&current_row);
										}
										else
											malloc_strcat (&current_row, ";");
										no_semicolon = 1;
										break;
									}
									/* End of reformatting */
								}
								start = ptr + 1;
							}
							else
							{
								if (!in_comment)
									malloc_strcat (&current_row, start);
								start = NULL;
							}
						}
					}
					else
						break;
				}
				if (in_comment)
					yell ("File %s ended with an unterminated comment in line %d", expanded, comment_line);
				if (current_row)
				{
					if (paste_level)
						yell ("Unexpected EOF in %s trying to match '{' at line %d",
						      expanded, paste_line);
					else
						parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_str : NULL, 0, 0);
					new_free (&current_row);
				}
				new_free (&expanded);
				fclose (fp);
				if (get_int_var (DISPLAY_VAR))
					window_display = display;
			}
/* End of reformatting */
		}
		else
			bitchsay ("Unknown user");
		/* 
		 * brute force -- if we are using -args, load ONLY one file
		 * then exit the while loop.  We could have done this
		 * by assigning args to NULL, but that would only waste
		 * cpu cycles by relooping...
		 */
		if (flag)
			break;
	}			/* end of the while loop that allows you to load
				   more then one file at a time.. */
	status_update (1);
	load_depth--;
/*      send_to_server("%s", "WAITFORNOTIFY"); */
}

