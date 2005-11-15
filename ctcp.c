#ident "@(#)ctcp.c 1.11"
/*
 * ctcp.c:handles the client-to-client protocol(ctcp). 
 *
 * Written By Michael Sandrof 
 * Copyright(c) 1990, 1995 Michael Sandroff and others
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Serious cleanup by jfn (August 1996)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include <pwd.h>


#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#include <stdarg.h>

#include "ctcp.h"
#include "dcc.h"
#include "commands.h"
#include "flood.h"
#include "hook.h"
#include "ignore.h"
#include "ircaux.h"
#include "lastlog.h"
#include "list.h"
#include "names.h"
#include "output.h"
#include "parse.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "misc.h"
#include "hash.h"
#include "fset.h"
#include "tcommand.h"

static void split_CTCP (char *, char *, char *);

/*
 * ctcp_entry: the format for each ctcp function.   note that the function
 * described takes 4 parameters, a pointer to the ctcp entry, who the message
 * was from, who the message was to (nickname, channel, etc), and the rest of
 * the ctcp message.  it can return null, or it can return a malloced string
 * that will be inserted into the oringal message at the point of the ctcp.
 * if null is returned, nothing is added to the original message
 */


struct _CtcpEntry;

typedef char *(*CTCP_Handler)(struct _CtcpEntry *, char *, char *, char *);

typedef	struct _CtcpEntry
{
	char		*name;  /* name of ctcp datatag */
	int		id;	/* index of this ctcp command */
	int		flag;	/* Action modifiers */
	char		*desc;  /* description returned by ctcp clientinfo */
	CTCP_Handler 	func;	/* function that does the dirty deed */
	CTCP_Handler 	repl;	/* Function that is called for reply */
}	CtcpEntry;


/* forward declarations for the built in CTCP functions */

static char *do_null (CtcpEntry *, char *, char *, char *);
static char *do_version (CtcpEntry *, char *, char *, char *);
static char *do_clientinfo (CtcpEntry *, char *, char *, char *);
static char *do_ping (CtcpEntry *, char *, char *, char *);
static char *do_echo (CtcpEntry *, char *, char *, char *);
static char *do_userinfo (CtcpEntry *, char *, char *, char *);
static char *do_finger (CtcpEntry *, char *, char *, char *);
static char *do_time (CtcpEntry *, char *, char *, char *);
static char *do_atmosphere (CtcpEntry *, char *, char *, char *);
static char *do_dcc (CtcpEntry *, char *, char *, char *);
static char *do_utc (CtcpEntry *, char *, char *, char *);
static char *do_dcc_reply (CtcpEntry *, char *, char *, char *);
static char *do_ping_reply (CtcpEntry *, char *, char *, char *);

static CtcpEntry ctcp_cmd[] =
{
	{"UTC", CTCP_UTC, CTCP_INLINE | CTCP_NOLIMIT,
	 "substitutes the local timezone",
	 do_utc, do_utc},
	{"ACTION", CTCP_ACTION, CTCP_SPECIAL | CTCP_NOLIMIT,
	 "contains action descriptions for atmosphere",
	 do_atmosphere, do_atmosphere},

	{"DCC", CTCP_DCC, CTCP_SPECIAL | CTCP_TELLUSER,
	 "requests a direct_client_connection",
	 do_dcc, do_dcc_reply},

	{"VERSION", CTCP_VERSION, CTCP_REPLY | CTCP_TELLUSER,
	 "shows client type, version and environment",
	 do_version, NULL},

	{"CLIENTINFO", CTCP_CLIENTINFO, CTCP_REPLY | CTCP_TELLUSER,
	 "gives information about available CTCP commands",
	 do_clientinfo, NULL},
	{"USERINFO", CTCP_USERINFO, CTCP_REPLY | CTCP_TELLUSER,
	 "returns user settable information",
	 do_userinfo, NULL},
	{"ERRMSG", CTCP_ERRMSG, CTCP_REPLY | CTCP_TELLUSER,
	 "returns error messages",
	 do_echo, NULL},
	{"FINGER", CTCP_FINGER, CTCP_REPLY | CTCP_TELLUSER,
	 "shows real name, login name and idle time of user",
	 do_finger, NULL},
	{"TIME", CTCP_TIME, CTCP_REPLY | CTCP_TELLUSER,
	 "tells you the time on the user's host",
	 do_time, NULL},
	{"PING", CTCP_PING, CTCP_REPLY | CTCP_TELLUSER,
	 "returns the arguments it receives",
	 do_ping, do_ping_reply},
	{"ECHO", CTCP_ECHO, CTCP_REPLY | CTCP_TELLUSER,
	 "returns the arguments it receives",
	 do_echo, NULL},
	{"SOUND", CTCP_SOUND, 0,
	 "stupid ctcp sound stuff...",
	 do_null, NULL},
	{"TROUT", CTCP_TROUT, 0,
	 "stupid ctcp trout stuff...",
	 do_null, NULL},

	{NULL, CTCP_CUSTOM, CTCP_REPLY | CTCP_TELLUSER,
	 NULL,
	 NULL, NULL}
};



/* CDE do ops and unban logging */

static char *ctcp_type[] =
{
	"PRIVMSG",
	"NOTICE"
};


/*
 * in_ctcp_flag is set to true when IRCII is handling a CTCP request.  This
 * is used by the ctcp() sending function to force NOTICEs to be used in any
 * CTCP REPLY 
 */
int in_ctcp_flag = 0;

int not_warned = 0;

#define CTCP_HANDLER(x) \
	static char * x (CtcpEntry *ctcp, char *from, char *to, char *cmd)

/**************************** CTCP PARSERS ****************************/

/********** INLINE EXPANSION CTCPS ***************/

CTCP_HANDLER (do_utc)
{

	if (!cmd || !*cmd)
		return m_strdup (empty_str);

	return m_strdup (my_ctime (my_atol (cmd)));
}


/*
 * do_atmosphere: does the CTCP ACTION command --- done by lynX
 * Changed this to make the default look less offensive to people
 * who don't like it and added a /on ACTION. This is more in keeping
 * with the design philosophy behind IRCII
 */
CTCP_HANDLER (do_atmosphere)
{
	int old;
	char *ptr1 = cmd;

	if (!cmd || !*cmd)
		return NULL;


	old = set_lastlog_msg_level (LOG_ACTION);
	if (get_int_var (MIRCS_VAR))
		ptr1 = mircansi (cmd);
	if (is_channel (to))
	{
		message_from (to, LOG_ACTION);
		if (is_current_channel (to, from_server, 0))
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_ACTION_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ptr1));
		else
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_ACTION_OTHER_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ptr1));
	}
	else
	{
		message_from (from, LOG_ACTION);
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_ACTION_PRIV_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ptr1));
	}

	message_from (NULL, LOG_CRAP);
	set_lastlog_msg_level (old);
	return NULL;
}

/*
 * do_dcc: Records data on an incoming DCC offer. Makes sure it's a
 *      user->user CTCP, as channel DCCs don't make any sense whatsoever
 */
CTCP_HANDLER (do_dcc)
{
	char *type;
	char *description;
	char *inetaddr;
	char *port;
	char *size;
	char *extra_flags;

	if (my_stricmp (to, get_server_nickname (from_server)))
		return NULL;

	if (!(type = next_arg (cmd, &cmd)) ||
	    !(description = next_arg (cmd, &cmd)) ||
	    !(inetaddr = next_arg (cmd, &cmd)) ||
	    !(port = next_arg (cmd, &cmd)))
		return NULL;

	size = next_arg (cmd, &cmd);
	extra_flags = next_arg (cmd, &cmd);

	register_dcc_offer (from, type, description, inetaddr, port, size, extra_flags, FromUserHost);
	return NULL;
}


/*************** REPLY-GENERATING CTCPS *****************/

/*
 * do_clientinfo: performs the CLIENTINFO CTCP.  If cmd is empty, returns the
 * list of all CTCPs currently recognized by IRCII.  If an arg is supplied,
 * it returns specific information on that CTCP.  If a matching CTCP is not
 * found, an ERRMSG ctcp is returned 
 */
CTCP_HANDLER (do_clientinfo)
{
	int i;

	if (cmd && *cmd)
	{
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			if (!my_stricmp (cmd, ctcp_cmd[i].name))
			{
				send_ctcp (CTCP_NOTICE, from, CTCP_CLIENTINFO,
					   "%s %s",
					ctcp_cmd[i].name, ctcp_cmd[i].desc);
				return NULL;
			}
		}
		send_ctcp (CTCP_NOTICE, from, CTCP_ERRMSG,
			   "%s: %s is not a valid function",
			   ctcp_cmd[CTCP_CLIENTINFO].name, cmd);
	}
	else
	{
		char buffer[BIG_BUFFER_SIZE + 1];
		*buffer = '\0';

		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			strmcat (buffer, ctcp_cmd[i].name, BIG_BUFFER_SIZE);
			strmcat (buffer, " ", BIG_BUFFER_SIZE);
		}
		send_ctcp (CTCP_NOTICE, from, CTCP_CLIENTINFO,
			   "%s :Use CLIENTINFO <COMMAND> to get more specific information",
			   buffer);
	}
	return NULL;
}

/* do_version: does the CTCP VERSION command */
CTCP_HANDLER (do_version)
{
	char *version_reply = NULL;
	char *the_unix = "unknown";
	char *the_version = "unknown";
	/*
	 * The old way seemed lame to me... let's show system name and
	 * release information as well.  This will surely help out
	 * experts/gurus answer newbie questions.  -- Jake [WinterHawk] Khuon
	 *
	 */


#if defined HAVE_UNAME
	struct utsname un;

	if (uname (&un) == 0)
	{
		the_version = un.release;
		the_unix = un.sysname;
	}
#endif
	malloc_strcpy (&version_reply, stripansicodes (convert_output_format (get_fset_var (FORMAT_VERSION_FSET), "%s %s %s", PACKAGE_VERSION, the_unix, the_version)));
	send_ctcp (CTCP_NOTICE, from, CTCP_VERSION, "%s (%s)", version_reply, get_string_var (CLIENTINFO_VAR));
	new_free (&version_reply);
	return NULL;
}

/* do_time: does the CTCP TIME command --- done by Veggen */
CTCP_HANDLER (do_time)
{
	send_ctcp (CTCP_NOTICE, from, CTCP_TIME, "%s", my_ctime (time (NULL)));
	return NULL;
}

/* do_userinfo: does the CTCP USERINFO command */
CTCP_HANDLER (do_userinfo)
{
	send_ctcp (CTCP_NOTICE, from, CTCP_USERINFO, "%s", get_string_var (USERINFO_VAR));
	return NULL;
}

/* just a nop.. so we can filter out stupid-but-plenty mirc ctcp hacks */
/* Yeah, this is now kind of mean. but. *smile* */
CTCP_HANDLER (do_null)
{
	return NULL;
}

/*
 * do_echo: does the CTCP ERRMSG and CTCP ECHO commands. Does
 * not send an error for ERRMSG and if the CTCP was sent to a channel.
 */
CTCP_HANDLER (do_echo)
{
	if (!is_channel (to))
	{
		if (strlen (cmd) > 60)
		{
			yell ("WARNING ctcp echo request longer than 60 chars. truncating");
			cmd[60] = 0;
		}
		send_ctcp (CTCP_NOTICE, from, ctcp->id, "%s", cmd);
	}
	return NULL;
}

CTCP_HANDLER (do_ping)
{
	send_ctcp (CTCP_NOTICE, from, CTCP_PING, "%s", cmd ? cmd : empty_str);
	return NULL;
}


/* 
 * Does the CTCP FINGER reply 
 */
CTCP_HANDLER (do_finger)
{
	struct passwd *pwd;
	time_t diff;
	char *tmp;
	char *ctcpuser, *ctcpfinger;



	diff = time (NULL) - idle_time;

	if (!(pwd = getpwuid (getuid ())))
		return NULL;

#ifndef GECOS_DELIMITER
#define GECOS_DELIMITER ','
#endif

	if ((tmp = strchr (pwd->pw_gecos, GECOS_DELIMITER)) != NULL)
		*tmp = '\0';

/* 
 * Three optionsn for handling CTCP replies
 *  + Fascist Bastard Way -- normal, non-hackable fashion
 *  + Winterhawk way (default) -- allows hacking through IRCUSER and
 *      IRCFINGER environment variables
 *  + hop way -- returns a blank always
 */
	/* 
	 * It would be pretty pointless to allow for customisable
	 * usernames if they can track via IRCNAME from the
	 * /etc/passwd file...  We therefore need to either disable
	 * CTCP_FINGER or also make it customisable.  Let's do the
	 * latter because it invokes less suspicion in the long run
	 *                              -- Jake [WinterHawk] Khuon
	 */
	if ((ctcpuser = getenv ("IRCUSER")))
		strmcpy (pwd->pw_name, ctcpuser, NAME_LEN);
	if ((ctcpfinger = getenv ("IRCFINGER")))
		strmcpy (pwd->pw_gecos, ctcpfinger, NAME_LEN);

	send_ctcp (CTCP_NOTICE, from, CTCP_FINGER,
		   "%s (%s@%s) Idle %ld second%s",
		pwd->pw_gecos, pwd->pw_name, hostname, diff, plural (diff));
	return NULL;
}


/* 
 * If we recieve a CTCP DCC REJECT in a notice, then we want to remove
 * the offending DCC request
 */
CTCP_HANDLER (do_dcc_reply)
{
	char *subcmd = NULL;
	char *type = NULL;

	if (is_channel (to))
		return NULL;

	if (cmd && *cmd)
		subcmd = next_arg (cmd, &cmd);
	if (cmd && *cmd)
		type = next_arg (cmd, &cmd);

	if (subcmd && type && cmd && !strcmp (subcmd, "REJECT"))
		dcc_reject (from, type, cmd);

	return NULL;
}


/*
 * Handles CTCP PING replies.
 */
CTCP_HANDLER (do_ping_reply)
{
	char *sec, *usec = NULL;
	struct timeval t;
	time_t tsec = 0, tusec = 0, orig;

	if (!cmd || !*cmd)
		return NULL;	/* This is a fake -- cant happen. */

	orig = my_atol (cmd);

	get_time (&t);
	if (orig < start_time || orig > t.tv_sec)
		return NULL;

	/* We've already checked 'cmd' here, so its safe. */
	sec = cmd;
	tsec = t.tv_sec - my_atol (sec);

	if ((usec = strchr (sec, ' ')))
	{
		*usec++ = 0;
		tusec = t.tv_usec - my_atol (usec);
	}

	/*
	 * 'cmd', a variable passed in from do_notice_ctcp()
	 * points to a buffer which is MUCH larger than the
	 * string 'cmd' points at.  So this is safe, even
	 * if it looks "unsafe".
	 */
	sprintf (cmd, "%5.3f seconds", (float) (tsec + (tusec / 1000000.0)));
	return NULL;
}


/************************************************************************/
/*
 * do_ctcp: a re-entrant form of a CTCP parser.  The old one was lame,
 * so i took a hatchet to it so it didnt suck.
 */
char *
do_ctcp (char *from, char *to, char *str)
{
	int flag;
	int lastlog_level;
	char local_ctcp_buffer[BIG_BUFFER_SIZE + 1], the_ctcp[IRCD_BUFFER_SIZE + 1],
	  last[IRCD_BUFFER_SIZE + 1];
	char *ctcp_command, *ctcp_argument;
	int i;
	char *ptr = NULL;
	int allow_ctcp_reply = 1;
	int delim_char = charcount (str, CTCP_DELIM_CHAR);

	if (delim_char < 2)
		return str;	/* No CTCPs. */
	if (delim_char > 8)
		allow_ctcp_reply = 0;	/* Historical limit of 4 CTCPs */

	flag = check_ignore (from, FromUserHost, to, IGNORE_CTCPS, NULL);

	if (!in_ctcp_flag)
		in_ctcp_flag = 1;
	allow_ctcp_reply = 1;
	strmcpy (local_ctcp_buffer, str, IRCD_BUFFER_SIZE - 2);

	for (;; strmcat (local_ctcp_buffer, last, IRCD_BUFFER_SIZE - 2))
	{
		split_CTCP (local_ctcp_buffer, the_ctcp, last);

		if (!*the_ctcp)
			break;	/* all done! */
		/*
		 * Apply some integrety rules:
		 * -- If we've already replied to a CTCP, ignore it.
		 * -- If user is ignoring sender, ignore it.
		 * -- If we're being flooded, ignore it.
		 * -- If CTCP was a global msg, ignore it.
		 */
		/*
		 * Yes, this intentionally ignores "unlimited" CTCPs like
		 * UTC and SED.  Ultimately, we have to make sure that
		 * CTCP expansions dont overrun any buffers that might
		 * contain this string down the road.  So by allowing up to
		 * 4 CTCPs, we know we cant overflow -- but if we have more
		 * than 40, it might overflow, and its probably a spam, so
		 * no need to shed tears over ignoring them.  Also makes
		 * the sanity checking much simpler.
		 */
		if (!allow_ctcp_reply)
			continue;

		/*
		 * Check to see if the user is ignoring person.
		 */

		if (flag == IGNORED)
		{
			allow_ctcp_reply = 0;
			continue;
		}

		ctcp_command = the_ctcp;
		ctcp_argument = strchr (the_ctcp, ' ');
		if (ctcp_argument)
			*ctcp_argument++ = 0;
		else
			ctcp_argument = empty_str;

		/* Global messages -- just drop the CTCP */
		if (*to == '$' || (*to == '#' && !lookup_channel (to, from_server, 0)))
		{
			allow_ctcp_reply = 0;
			continue;
		}
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
			if (!strcmp (ctcp_command, ctcp_cmd[i].name))
				break;

		if (ctcp_cmd[i].id == CTCP_ACTION)
			check_flooding (from, CTCP_ACTION_FLOOD, ctcp_argument, is_channel (to) ? to : NULL);
		else if (ctcp_cmd[i].id == CTCP_DCC)
			check_flooding (from, CTCP_FLOOD, ctcp_argument, is_channel (to) ? to : NULL);
		else
		{
			check_flooding (from, CTCP_FLOOD, ctcp_argument, is_channel (to) ? to : NULL);
			if (get_int_var (NO_CTCP_FLOOD_VAR) && (time (NULL) - server_list[from_server].ctcp_last_reply_time < get_int_var (CTCP_DELAY_VAR)) /* && ctcp_cmd[i].id !=CTCP_DCC */ )
			{
				if (get_int_var (FLOOD_WARNING_VAR))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_FLOOD_FSET), "%s %s %s %s %s", update_clock (GET_TIME), ctcp_command, from, FromUserHost, to));
				time (&server_list[from_server].ctcp_last_reply_time);
				allow_ctcp_reply = 0;
				continue;
			}
		}
		/* Did the CTCP search work? */
		if (i == NUMBER_OF_CTCPS)
		{
			/*
			 * Offer it to the user.
			 * Maybe they know what to do with it.
			 */

			if (do_hook (CTCP_LIST, "%s %s %s %s", from, to, ctcp_command, ctcp_argument))
			{
				if (allow_ctcp_reply && get_int_var (CTCP_VERBOSE_VAR))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_CTCP_UNKNOWN_FSET),
									     "%s %s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument ? ctcp_argument : empty_str));
			}
			allow_ctcp_reply = 0;
			continue;
		}
		ptr = ctcp_cmd[i].func (ctcp_cmd + i, from, to, ctcp_argument);

		if (!(ctcp_cmd[i].flag & CTCP_NOLIMIT))
		{
			time (&server_list[from_server].ctcp_last_reply_time);
			allow_ctcp_reply = 0;
		}

		/*
		 * We've only gotten to this point if its a valid CTCP
		 * query and we decided to parse it.
		 */

		/* If its an inline CTCP paste it back in */
		if ((ctcp_cmd[i].flag & CTCP_INLINE))
			strmcat (local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
		/* If its interesting, tell the user. */
		if ((ctcp_cmd[i].flag & CTCP_TELLUSER))
		{
			if (do_hook (CTCP_LIST, "%s %s %s %s", from, to, ctcp_command, ctcp_argument))
			{
				/* Set up the window level/logging */
				lastlog_level = set_lastlog_msg_level (LOG_CTCP);
				message_from (from, LOG_CTCP);

				if (get_int_var (CTCP_VERBOSE_VAR))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_CTCP_FSET),
									     "%s %s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument ? ctcp_argument : empty_str));
				/* Reset the window level/logging */
				message_from (NULL, LOG_CRAP);
				set_lastlog_msg_level (lastlog_level);
			}
		}

		new_free (&ptr);
	}

	if (in_ctcp_flag == 1)
		in_ctcp_flag = 0;
	if (*local_ctcp_buffer)
		return strcpy (str, local_ctcp_buffer);
	else
		return empty_str;
}



/*
 * do_notice_ctcp: a re-entrant form of a CTCP reply parser.
 */
char *
do_notice_ctcp (char *from, char *to, char *str)
{
	int flag;
	int lastlog_level;
	char local_ctcp_buffer[BIG_BUFFER_SIZE + 1], the_ctcp[IRCD_BUFFER_SIZE + 1],
	  last[IRCD_BUFFER_SIZE + 1];
	char *ctcp_command, *ctcp_argument;
	int i;
	char *ptr;
	char *tbuf = NULL;
	int allow_ctcp_reply = 1;

	int delim_char = charcount (str, CTCP_DELIM_CHAR);

	if (delim_char < 2)
		return str;	/* No CTCPs. */
	if (delim_char > 8)
		allow_ctcp_reply = 0;	/* Ignore all the CTCPs. */

	flag = check_ignore (from, FromUserHost, to, IGNORE_CTCPS, NULL);
	if (!in_ctcp_flag)
		in_ctcp_flag = -1;

	tbuf = stripansi (str);
	strmcpy (local_ctcp_buffer, tbuf, IRCD_BUFFER_SIZE - 2);
	new_free (&tbuf);

	for (;; strmcat (local_ctcp_buffer, last, IRCD_BUFFER_SIZE - 2))
	{
		split_CTCP (local_ctcp_buffer, the_ctcp, last);
		if (!*the_ctcp)
			break;	/* all done! */

		if (!allow_ctcp_reply)
			continue;

		if (flag == IGNORED)
		{
			allow_ctcp_reply = 0;
			continue;
		}

		/* Global messages -- just drop the CTCP */
		if (*to == '$' || (*to == '#' && !lookup_channel (to, from_server, 0)))
		{
			allow_ctcp_reply = 0;
			continue;
		}

		ctcp_command = the_ctcp;
		ctcp_argument = strchr (the_ctcp, ' ');
		if (ctcp_argument)
			*ctcp_argument++ = 0;
		else
			ctcp_argument = empty_str;

		/* Find the correct CTCP and run it. */
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
			if (!strcmp (ctcp_command, ctcp_cmd[i].name))
				break;

		/* 
		 * If we've already parsed one, and there is a limit
		 * on this CTCP, then just punt it.
		 */
		if (i < NUMBER_OF_CTCPS && ctcp_cmd[i].repl)
		{
			if ((ptr = ctcp_cmd[i].repl (ctcp_cmd + i, from, to, ctcp_argument)))
			{
				strmcat (local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
				new_free (&ptr);
				continue;
			}
		}
		/* Toss it at the user.  */
		if (do_hook (CTCP_REPLY_LIST, "%s %s %s", from, ctcp_command, ctcp_argument))
		{
			/* Set up the window level/logging */
			lastlog_level = set_lastlog_msg_level (LOG_CTCP);
			message_from (from, LOG_CTCP);

			put_it ("%s", convert_output_format (get_fset_var (FORMAT_CTCP_REPLY_FSET), "%s %s %s %s %s", update_clock (GET_TIME), from, FromUserHost, ctcp_command, ctcp_argument));
			/* Reset the window level/logging */
			message_from (NULL, LOG_CTCP);
			set_lastlog_msg_level (lastlog_level);
		}

		allow_ctcp_reply = 0;
	}

	if (in_ctcp_flag == -1)
		in_ctcp_flag = 0;

	return strcpy (str, local_ctcp_buffer);
}



/* in_ctcp: simply returns the value of the ctcp flag */
int 
in_ctcp (void)
{
	return in_ctcp_flag;
}



/*
 * This is no longer directly sends information to its target.
 * As part of a larger attempt to consolidate all data transmission
 * into send_text, this function was modified so as to use send_text().
 * This function can send both direct CTCP requests, as well as the
 * appropriate CTCP replies.  By its use of send_text(), it can send
 * CTCPs to DCC CHAT and irc nickname peers, and handles encryption
 * transparantly.  This greatly reduces the logic, complexity, and
 * possibility for error in this function.
 */
void 
send_ctcp (int type, char *to, int datatag, char *format,...)
{
	char putbuf[BIG_BUFFER_SIZE + 1], putbuf2[BIG_BUFFER_SIZE + 1];

	if (in_on_who)
		return;

	if (format)
	{
		va_list args;
		va_start (args, format);
		vsprintf (putbuf, format, args);
		va_end (args);
		snprintf (putbuf2, BIG_BUFFER_SIZE, "%c%s %s%c", CTCP_DELIM_CHAR, ctcp_cmd[datatag].name, putbuf, CTCP_DELIM_CHAR);
	}
	else
		snprintf (putbuf2, BIG_BUFFER_SIZE, "%c%s%c", CTCP_DELIM_CHAR, ctcp_cmd[datatag].name, CTCP_DELIM_CHAR);


	send_text (to, putbuf2, ctcp_type[type], 0, 0);
}


/*
 * quote_it: This quotes the given string making it sendable via irc.  A
 * pointer to the length of the data is required and the data need not be
 * null terminated (it can contain nulls).  Returned is a malloced, null
 * terminated string.   
 */
char *
ctcp_quote_it (char *str, int len)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	char *ptr;
	int i;

	ptr = buffer;
	for (i = 0; i < len; i++)
	{
		switch (str[i])
		{
		case CTCP_DELIM_CHAR:
			*ptr++ = CTCP_QUOTE_CHAR;
			*ptr++ = 'a';
			break;
		case '\n':
			*ptr++ = CTCP_QUOTE_CHAR;
			*ptr++ = 'n';
			break;
		case '\r':
			*ptr++ = CTCP_QUOTE_CHAR;
			*ptr++ = 'r';
			break;
		case CTCP_QUOTE_CHAR:
			*ptr++ = CTCP_QUOTE_CHAR;
			*ptr++ = CTCP_QUOTE_CHAR;
			break;
		case '\0':
			*ptr++ = CTCP_QUOTE_CHAR;
			*ptr++ = '0';
			break;
		default:
			*ptr++ = str[i];
			break;
		}
	}
	*ptr = '\0';
	return m_strdup (buffer);
}

#if 0
/*
 * ctcp_unquote_it: This takes a null terminated string that had previously
 * been quoted using ctcp_quote_it and unquotes it.  Returned is a malloced
 * space pointing to the unquoted string.  NOTE: a trailing null is added for
 * convenied, but the returned data may contain nulls!.  The len is modified
 * to contain the size of the data returned. 
 */
static char *
ctcp_unquote_it (char *str, int *len)
{
	char *buffer;
	char *ptr;
	char c;
	int i, new_size = 0;

	buffer = (char *) new_malloc ((sizeof (char) * *len) + 1);
	ptr = buffer;
	i = 0;
	while (i < *len)
	{
		if ((c = str[i++]) == CTCP_QUOTE_CHAR)
		{
			switch (c = str[i++])
			{
			case CTCP_QUOTE_CHAR:
				*ptr++ = CTCP_QUOTE_CHAR;
				break;
			case 'a':
				*ptr++ = CTCP_DELIM_CHAR;
				break;
			case 'n':
				*ptr++ = '\n';
				break;
			case 'r':
				*ptr++ = '\r';
				break;
			case '0':
				*ptr++ = '\0';
				break;
			default:
				*ptr++ = c;
				break;
			}
		}
		else
			*ptr++ = c;
		new_size++;
	}
	*ptr = '\0';
	*len = new_size;
	return (buffer);
}
#endif

int 
get_ctcp_val (char *str)
{
	int i;

	for (i = 0; i < NUMBER_OF_CTCPS; i++)
		if (!strcmp (str, ctcp_cmd[i].name))
			return i;

	/*
	 * This is *dangerous*, but it works.  The only place that
	 * calls this function is edit.c:ctcp(), and it immediately
	 * calls send_ctcp().  So the pointer that is being passed
	 * to us is globally allocated at a level higher then ctcp().
	 * so it wont be bogus until some time after ctcp() returns,
	 * but at that point, we dont care any more.
	 */
	ctcp_cmd[CTCP_CUSTOM].name = str;
	return CTCP_CUSTOM;
}



/*
 * XXXX -- some may call this a hack, but if youve got a better
 * way to handle this job, id love to use it.
 */
static void 
split_CTCP (char *raw_message, char *ctcp_dest, char *after_ctcp)
{
	char *ctcp_start, *ctcp_end;

	*ctcp_dest = *after_ctcp = 0;
	ctcp_start = strchr (raw_message, CTCP_DELIM_CHAR);
	if (!ctcp_start)
		return;		/* No CTCPs present. */

	*ctcp_start++ = 0;
	ctcp_end = strchr (ctcp_start, CTCP_DELIM_CHAR);
	if (!ctcp_end)
	{
		*--ctcp_start = CTCP_DELIM_CHAR;
		return;		/* Thats _not_ a CTCP. */
	}

	*ctcp_end++ = 0;
	strmcpy (ctcp_dest, ctcp_start, IRCD_BUFFER_SIZE - 2);
	strmcpy (after_ctcp, ctcp_end, IRCD_BUFFER_SIZE - 2);

	return;			/* All done! */
}
