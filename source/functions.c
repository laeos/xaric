#ident "@(#)functions.c 1.13"
/*
 * functions.c -- Built-in functions for ircII
 *
 * Written by Michael Sandrof
 * Copyright(c) 1990 Michael Sandrof
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "irc.h"
#include "dcc.h"
#include "commands.h"
#include "files.h"
#include "history.h"
#include "hook.h"
#include "input.h"
#include "ircaux.h"
#include "names.h"
#include "output.h"
#include "list.h"
#include "parse.h"
#include "screen.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "ircterm.h"
#include "notify.h"
#include "misc.h"
#include "numbers.h"
#include "ignore.h"
#include "hash2.h"
#include "expr.h"
#include "util.h"
#include "xaric_version.h"

#include <sys/stat.h>

static char *alias_detected (void);
static char *alias_sent_nick (void);
static char *alias_recv_nick (void);
static char *alias_msg_body (void);
static char *alias_joined_nick (void);
static char *alias_public_nick (void);
static char *alias_dollar (void);
static char *alias_channel (void);
static char *alias_server (void);
static char *alias_query_nick (void);
static char *alias_target (void);
static char *alias_nick (void);
static char *alias_invite (void);
static char *alias_cmdchar (void);
static char *alias_line (void);
char *alias_away (void);
static char *alias_oper (void);
static char *alias_chanop (void);
static char *alias_modes (void);
static char *alias_buffer (void);
static char *alias_time (void);
static char *alias_currdir (void);
static char *alias_current_numeric (void);
static char *alias_show_userhost (void);
static char *alias_show_realname (void);
static char *alias_online (void);
static char *alias_idle (void);
static char *alias_version_str (void);
static char *alias_version_str1 (void);
static char *alias_line_thing (void);
static char *alias_uptime (void);

typedef struct
{
	char name;
	char *(*func) (void);
}
BuiltIns;

static BuiltIns built_in[] =
{
	{'.', alias_sent_nick},
	{',', alias_recv_nick},
	{':', alias_joined_nick},
	{';', alias_public_nick},
	{'`', alias_uptime},
	{'$', alias_dollar},
	{'A', alias_away},
	{'B', alias_msg_body},
	{'C', alias_channel},
	{'D', alias_detected},
	{'E', alias_idle},
	{'F', alias_online},
	{'G', alias_line_thing},
	{'H', alias_current_numeric},
	{'I', alias_invite},
	{'J', alias_version_str},
	{'K', alias_cmdchar},
	{'L', alias_line},
	{'M', alias_modes},
	{'N', alias_nick},
	{'O', alias_oper},
	{'P', alias_chanop},
	{'Q', alias_query_nick},
	{'S', alias_server},
	{'T', alias_target},
	{'U', alias_buffer},
	{'W', alias_currdir},
	{'X', alias_show_userhost},
	{'Y', alias_show_realname},
	{'Z', alias_time},
	{'a', alias_version_str1},
	{0, NULL}
};



int in_cparse = 0;
extern time_t start_time;



char *
built_in_alias (char c)
{
	BuiltIns *tmp;

	for (tmp = built_in; tmp->name; tmp++)
		if (c == tmp->name)
			return tmp->func ();

	return m_strdup (empty_string);
}

/* built in expando functions */
static char *
alias_version_str1 (void)
{
	return m_strdup (version);
}
static char *
alias_line (void)
{
	return m_strdup (get_input ());
}
static char *
alias_buffer (void)
{
	return m_strdup (cut_buffer);
}
static char *
alias_time (void)
{
	return m_strdup (update_clock (GET_TIME));
}
static char *
alias_dollar (void)
{
	return m_strdup ("$");
}
static char *
alias_detected (void)
{
	return m_strdup (last_notify_nick);
}
static char *
alias_nick (void)
{
	return m_strdup (curr_scr_win->server != -1 ? get_server_nickname (curr_scr_win->server) : empty_string);
}
char *
alias_away (void)
{
	return m_strdup (get_server_away (curr_scr_win->server));
}
static char *
alias_sent_nick (void)
{
	return m_strdup ((sent_nick) ? sent_nick : empty_string);
}
static char *
alias_recv_nick (void)
{
	return m_strdup ((recv_nick) ? recv_nick : empty_string);
}
static char *
alias_msg_body (void)
{
	return m_strdup ((sent_body) ? sent_body : empty_string);
}
static char *
alias_joined_nick (void)
{
	return m_strdup ((joined_nick) ? joined_nick : empty_string);
}
static char *
alias_public_nick (void)
{
	return m_strdup ((public_nick) ? public_nick : empty_string);
}
static char *
alias_show_realname (void)
{
	return m_strdup (realname);
}
static char *
alias_version_str (void)
{
	return m_strdup (XARIC_VersionStr);
}
static char *
alias_invite (void)
{
	return m_strdup ((invite_channel) ? invite_channel : empty_string);
}
static char *
alias_oper (void)
{
	return m_strdup (get_server_operator (from_server) ? get_string_var (STATUS_OPER_VAR) : empty_string);
}
static char *
alias_online (void)
{
	return m_sprintf ("%ld", (long) start_time);
}
static char *
alias_idle (void)
{
	return m_sprintf ("%ld", time (NULL) - idle_time);
}
static char *
alias_show_userhost (void)
{
	return m_sprintf ("%s@%s", username, hostname);
}
static char *
alias_current_numeric (void)
{
	return m_sprintf ("%03d", -current_numeric);
}
static char *
alias_line_thing (void)
{
	return m_sprintf ("%s", line_thing);
}
static char *
alias_uptime (void)
{
	return m_sprintf ("%s", convert_time (time (NULL) - start_time));
}
static char *
alias_currdir (void)
{
	char *tmp = (char *) new_malloc (MAXPATHLEN + 1);
	return getcwd (tmp, MAXPATHLEN);
}

static char *
alias_channel (void)
{
	char *tmp;
	return m_strdup ((tmp = get_current_channel_by_refnum (0)) ? tmp : "0");
}

static char *
alias_server (void)
{
	return m_strdup ((parsing_server_index != -1) ?
			 get_server_itsname (parsing_server_index) :
			 (get_window_server (0) != -1) ?
		 get_server_itsname (get_window_server (0)) : empty_string);
}

static char *
alias_query_nick (void)
{
	char *tmp;
	return m_strdup ((tmp = query_nick ())? tmp : empty_string);
}

static char *
alias_target (void)
{
	char *tmp;
	return m_strdup ((tmp = get_target_by_refnum (0)) ? tmp : empty_string);
}

static char *
alias_cmdchar (void)
{
	char *cmdchars, tmp[2];

	if ((cmdchars = get_string_var (CMDCHARS_VAR)) == NULL)
		cmdchars = DEFAULT_CMDCHARS;
	tmp[0] = cmdchars[0];
	tmp[1] = 0;
	return m_strdup (tmp);
}

static char *
alias_chanop (void)
{
	char *tmp;
	return m_strdup (((tmp = get_current_channel_by_refnum (0)) && get_channel_oper (tmp, from_server)) ?
			 "@" : empty_string);
}

static char *
alias_modes (void)
{
	char *tmp;
	return m_strdup ((tmp = get_current_channel_by_refnum (0)) ?
			 get_channel_mode (tmp, from_server) : empty_string);
}


