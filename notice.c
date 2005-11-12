/*
 * notice.c: special stuff for parsing NOTICEs
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1991
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "commands.h"
#include "whois.h"
#include "ctcp.h"
#include "window.h"
#include "lastlog.h"
#include "flood.h"
#include "vars.h"
#include "ircaux.h"
#include "hook.h"
#include "ignore.h"
#include "server.h"
#include "funny.h"
#include "output.h"
#include "names.h"
#include "parse.h"
#include "notify.h"
#include "misc.h"
#include "screen.h"
#include "status.h"
#include "notice.h"
#include "hash.h"
#include "input.h"
#include "fset.h"

extern char *FromUserHost;
static void parse_server_notice (char *, char *);
int doing_notice = 0;



static void 
parse_server_notice (char *from, char *line)
{
	int lastlog_level = 0;
	int flag = 0;
	int up_status = 0;



	if (!from || !*from)
		from = server_list[from_server].itsname ?
			server_list[from_server].itsname :
			server_list[from_server].name;
	if (!strncmp (line, "*** Notice --", 13))
	{
		message_from (NULL, LOG_OPNOTE);
		lastlog_level = set_lastlog_msg_level (LOG_OPNOTE);
	}

	message_from (NULL, LOG_SNOTE);
	lastlog_level = set_lastlog_msg_level (LOG_SNOTE);

	if (*line != '*' && *line != '#' && strncmp (line, "MOTD ", 4))
		flag = 1;
	else
		flag = 0;

	if (do_hook (SERVER_NOTICE_LIST, flag ? "%s *** %s" : "%s %s", from, line))
	{
		if (strstr (line, "***"))
		{
			if (do_hook (SERVER_NOTICE_LIST, flag ? "%s *** %s" : "%s %s", from, line))
			{
				char *for_;
				for_ = next_arg (line, &line);
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_SERVER_NOTICE_FSET), "%s %s %s", update_clock (GET_TIME), from, stripansicodes (line)));
			}
		}
		else
		{
			if (do_hook (SERVER_NOTICE_LIST, flag ? "%s *** %s" : "%s %s", from, line))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_SERVER_NOTICE_FSET), "%s %s %s", update_clock (GET_TIME), from, stripansicodes (line)));
		}
	}
	if (up_status)
		update_all_status (curr_scr_win, NULL, 0);

	if (lastlog_level)
	{
		set_lastlog_msg_level (lastlog_level);
		message_from (NULL, lastlog_level);
	}
}

void 
parse_notice (char *from, char **Args)
{
	int level, type;
	char *to;
	int no_flooding;
	int flag;
	char *high, not_from_server = 1;
	char *line;

	struct nick_list *nick = NULL;
	struct channel *tmpc;


	PasteArgs (Args, 1);
	to = Args[0];
	line = Args[1];
	if (!to || !line)
		return;
	doing_notice = 1;

	if (*to)
	{
		if (is_channel (to))
		{
			message_from (to, LOG_NOTICE);
			type = PUBLIC_NOTICE_LIST;
		}
		else
		{
			message_from (from, LOG_NOTICE);
			type = NOTICE_LIST;
		}
		if ((tmpc = lookup_channel (to, from_server, CHAN_NOUNLINK)))
			nick = find_nicklist_in_channellist (from, tmpc, 0);
		if (from && *from && strcmp (get_server_itsname (from_server), from))
		{
			char *newline = NULL;

			switch ((flag = check_ignore (from, FromUserHost, to, IGNORE_NOTICES, line)))
			{
			case IGNORED:
				{
					doing_notice = 0;
					return;
				}
			case HIGHLIGHTED:
				high = highlight_char;
				break;
			default:
				high = empty_str;
			}
			/*
			 * only dots in servernames, right ?
			 *
			 * But a server name doesn't nessicarily have to have
			 * a 'dot' in it..  - phone, jan 1993.
			 */
			if (strchr (from, '.'))
				not_from_server = 0;
			line = do_notice_ctcp (from, to, line);
			if (!line || !*line)
			{
				doing_notice = 0;
				return;
			}
			level = set_lastlog_msg_level (LOG_NOTICE);
			no_flooding = check_flooding (from, NOTICE_FLOOD, line, NULL);

			if (sed == 1)
			{
				if (do_hook (ENCRYPTED_NOTICE_LIST, "%s %s %s", from, to, line))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_ENCRYPTED_NOTICE_FSET), "%s %s %s %s", update_clock (GET_TIME), from, FromUserHost, line));
				sed = 0;
			}
			else
			{
				char *free_me = NULL;
				free_me = newline = stripansi (line);

				if (no_flooding)
				{
					if (match ("[*Wall*", line))
					{
						char *channel = NULL, *p,
						 *q;
						if (do_hook (type, "%s %s", from, line))
						{
							q = p = next_arg (newline, &newline);
							if ((p = strchr (p, '/')))
							{
								channel = m_strdup (++p);
								if ((p = strchr (channel, ']')))
									*p++ = 0;
								q = channel;
							}
							put_it ("%s", convert_output_format (get_fset_var (FORMAT_BWALL_FSET), "%s %s %s %s %s", update_clock (GET_TIME), q, from, FromUserHost, newline));
						}
						logmsg (LOG_WALL, from, line, 0);
						new_free (&channel);
					}
					else if (type == NOTICE_LIST)
					{
						logmsg (LOG_NOTICE, from, line, 0);
						if (do_hook (type, "%s %s", from, line))
							put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTICE_FSET), "%s %s %s %s", update_clock (GET_TIME), from, FromUserHost, newline));
					}
					else
					{
						if (do_hook (type, "%s %s %s", from, to, line))
							put_it ("%s", convert_output_format (get_fset_var (FORMAT_PUBLIC_NOTICE_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, newline));
						if (beep_on_level & LOG_NOTICE)
							beep_em (1);
						logmsg (LOG_NOTICE, from, line, 0);
					}
				}
				new_free (&free_me);
				set_lastlog_msg_level (level);
				if (not_from_server)
					notify_mark (from, NULL, NULL, 0);
			}
		}
		else
			parse_server_notice (from, line);
	}
	else
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_SERVER_MSG2_FSET), "%s %s %s", update_clock (GET_TIME), from, line + 1));
	doing_notice = 0;
	message_from (NULL, LOG_CRAP);
}


void 
load_scripts (void)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	static int done = 0;
	if (!done++)
	{
		never_connected = 0;
		strmcpy (buffer, ircrc_file, BIG_BUFFER_SIZE);
		strmcat (buffer, " ", BIG_BUFFER_SIZE);
		load (NULL, buffer, NULL, NULL);
	}
	if (get_server_away (from_server))
		set_server_away (from_server, get_server_away (from_server));
}

/*
 * got_initial_version: this is called when ircii gets the serial
 * number 004 reply.  We do this becuase the 004 numeric gives us the
 * server name and version in a very easy to use fashion, and doesnt
 * rely on the syntax or construction of the 002 numeric.
 *
 * Hacked as neccesary by jfn, May 1995
 */
extern void
got_initial_version (char **ArgList)
{
	char *server, *version;

	server = ArgList[0];
	version = ArgList[1];

	if (!strncmp (version, "2.8", 3))
	{
		if (strstr (version, "mu") || strstr (version, "me"))
			set_server_version (from_server, Server_u2_8);
		else
			set_server_version (from_server, Server2_8);
	}
	else if (!strncmp (version, "u2.9", 4))
		set_server_version (from_server, Server_u2_9);
	else if (!strncmp (version, "u2.10", 4))
		set_server_version (from_server, Server_u2_10);
	else if (!strncmp (version, "u3.0", 4))
		set_server_version (from_server, Server_u3_0);
	else
		set_server_version (from_server, Server2_8);

	malloc_strcpy (&server_list[from_server].version_string, version);
	set_server_itsname (from_server, server);
	reconnect_all_channels (from_server);
	message_from (NULL, LOG_CRAP);
	reinstate_user_modes ();

	update_all_status (curr_scr_win, NULL, 0);
	do_hook (CONNECT_LIST, "%s %d", get_server_name (from_server), get_server_port (from_server));
}
