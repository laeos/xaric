#ident "@(#)fset.c 1.12"
/*
 * xformats.c - keep track of all our format strings
 * Copyright (C) 2000 Rex Feany (laeos@laeos.net)
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
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "irc.h"
#include "ircaux.h"
#include "screen.h"
#include "output.h"
#include "misc.h"
#include "tcommand.h"
#include "fset.h"
#include "format.h"
#include "util.h"
#include "xmalloc.h"


#define FMT_STATIC	0x01	/* Use this to remeber if we need to free() or not */
#define FMT_MODIFIED	0x02	/* Use this to remeber if we changed it or not (for save) */

struct format_set
{
	char *name;
	char *string;
	short flags;
} formatset_array[] = {
	/*--start--*/
	{ "FORMAT_381",			DEF_FORMAT_381, 		FMT_STATIC },
	{ "FORMAT_391", 		DEF_FORMAT_391, 		FMT_STATIC },
	{ "FORMAT_443", 		DEF_FORMAT_443, 		FMT_STATIC },
	{ "FORMAT_471", 		DEF_FORMAT_471, 		FMT_STATIC },
	{ "FORMAT_473", 		DEF_FORMAT_473, 		FMT_STATIC },
	{ "FORMAT_474", 		DEF_FORMAT_474, 		FMT_STATIC },
	{ "FORMAT_475",			DEF_FORMAT_475, 		FMT_STATIC },
	{ "FORMAT_476",			DEF_FORMAT_476, 		FMT_STATIC },
	{ "FORMAT_ACTION", 		DEF_FORMAT_ACTION, 		FMT_STATIC },
	{ "FORMAT_ACTION_OTHER",	DEF_FORMAT_ACTION_OTHER, 	FMT_STATIC },
	{ "FORMAT_ALIAS",		DEF_FORMAT_ALIAS, 		FMT_STATIC },
	{ "FORMAT_ASSIGN",		DEF_FORMAT_ASSIGN, 		FMT_STATIC },
	{ "FORMAT_AWAY",		DEF_FORMAT_AWAY, 		FMT_STATIC },
	{ "FORMAT_BACK",		DEF_FORMAT_BACK, 		FMT_STATIC },
	{ "FORMAT_BANS",		DEF_FORMAT_BANS, 		FMT_STATIC },
	{ "FORMAT_BANS_HEADER", 	DEF_FORMAT_BANS_HEADER,		FMT_STATIC },
	{ "FORMAT_BWALL",		DEF_FORMAT_BWALL, 		FMT_STATIC },
	{ "FORMAT_CHANNEL_SIGNOFF",	DEF_FORMAT_CHANNEL_SIGNOFF, 	FMT_STATIC },
	{ "FORMAT_CONNECT",		DEF_FORMAT_CONNECT, 		FMT_STATIC },
	{ "FORMAT_CTCP",		DEF_FORMAT_CTCP, 		FMT_STATIC },
	{ "FORMAT_CTCP_REPLY",		DEF_FORMAT_CTCP_REPLY, 		FMT_STATIC },
	{ "FORMAT_CTCP_UNKNOWN",	DEF_FORMAT_CTCP_UNKNOWN, 	FMT_STATIC },
	{ "FORMAT_DCC",			DEF_FORMAT_DCC, 		FMT_STATIC },
	{ "FORMAT_DCC_CHAT",		DEF_FORMAT_DCC_CHAT, 		FMT_STATIC },
	{ "FORMAT_DCC_CONNECT",		DEF_FORMAT_DCC_CONNECT, 	FMT_STATIC },
	{ "FORMAT_DCC_ERROR",		DEF_FORMAT_DCC_ERROR, 		FMT_STATIC },
	{ "FORMAT_DCC_LOST",		DEF_FORMAT_DCC_LOST, 		FMT_STATIC },
	{ "FORMAT_DCC_REQUEST",		DEF_FORMAT_DCC_REQUEST, 	FMT_STATIC },
	{ "FORMAT_DESYNC", 		DEF_FORMAT_DESYNC, 		FMT_STATIC },
	{ "FORMAT_DISCONNECT",		DEF_FORMAT_DISCONNECT, 		FMT_STATIC },
	{ "FORMAT_ENCRYPTED_NOTICE",	DEF_FORMAT_ENCRYPTED_NOTICE, 	FMT_STATIC },
	{ "FORMAT_ENCRYPTED_PRIVMSG",	DEF_FORMAT_ENCRYPTED_PRIVMSG, 	FMT_STATIC },
	{ "FORMAT_FLOOD", 		DEF_FORMAT_FLOOD, 		FMT_STATIC },
	{ "FORMAT_HOOK",		DEF_FORMAT_HOOK, 		FMT_STATIC },
	{ "FORMAT_IGNORE_INVITE",	DEF_FORMAT_IGNORE_INVITE, 	FMT_STATIC },
	{ "FORMAT_IGNORE_MSG", 		DEF_FORMAT_IGNORE_MSG, 		FMT_STATIC },
	{ "FORMAT_IGNORE_MSG_AWAY", 	DEF_FORMAT_IGNORE_MSG_AWAY,	FMT_STATIC },
	{ "FORMAT_IGNORE_NOTICE", 	DEF_FORMAT_IGNORE_NOTICE, 	FMT_STATIC },
	{ "FORMAT_IGNORE_WALL", 	DEF_FORMAT_IGNORE_WALL, 	FMT_STATIC },
	{ "FORMAT_INVITE", 		DEF_FORMAT_INVITE, 		FMT_STATIC },
	{ "FORMAT_INVITE_USER", 	DEF_FORMAT_INVITE_USER, 	FMT_STATIC },
	{ "FORMAT_JOIN", 		DEF_FORMAT_JOIN, 		FMT_STATIC },
	{ "FORMAT_KICK", 		DEF_FORMAT_KICK, 		FMT_STATIC },
	{ "FORMAT_KICK_USER", 		DEF_FORMAT_KICK_USER, 		FMT_STATIC },
	{ "FORMAT_KILL", 		DEF_FORMAT_KILL, 		FMT_STATIC },
	{ "FORMAT_LEAVE", 		DEF_FORMAT_LEAVE, 		FMT_STATIC },
	{ "FORMAT_LINKS", 		DEF_FORMAT_LINKS, 		FMT_STATIC },
	{ "FORMAT_LIST", 		DEF_FORMAT_LIST, 		FMT_STATIC },
	{ "FORMAT_MODE", 		DEF_FORMAT_MODE, 		FMT_STATIC },
	{ "FORMAT_MODE_CHANNEL", 	DEF_FORMAT_MODE_CHANNEL, 	FMT_STATIC },
	{ "FORMAT_MSG", 		DEF_FORMAT_MSG, 		FMT_STATIC },
	{ "FORMAT_MSGLOG", 		DEF_FORMAT_MSGLOG, 		FMT_STATIC },
	{ "FORMAT_MSG_GROUP", 		DEF_FORMAT_MSG_GROUP, 		FMT_STATIC },
	{ "FORMAT_NAMES", 		DEF_FORMAT_NAMES, 		FMT_STATIC },
	{ "FORMAT_NAMES_BANNER", 	DEF_FORMAT_NAMES_BANNER, 	FMT_STATIC },
	{ "FORMAT_NAMES_FOOTER", 	DEF_FORMAT_NAMES_FOOTER, 	FMT_STATIC },
	{ "FORMAT_NAMES_IRCOP", 	DEF_FORMAT_NAMES_IRCOP, 	FMT_STATIC },
	{ "FORMAT_NAMES_NICKCOLOR", 	DEF_FORMAT_NAMES_NICKCOLOR, 	FMT_STATIC },
	{ "FORMAT_NAMES_NONOP", 	DEF_FORMAT_NAMES_NONOP, 	FMT_STATIC },
	{ "FORMAT_NAMES_OP", 		DEF_FORMAT_NAMES_OP, 		FMT_STATIC },
	{ "FORMAT_NAMES_OPCOLOR", 	DEF_FORMAT_NAMES_OPCOLOR,	FMT_STATIC },
	{ "FORMAT_NAMES_VOICE", 	DEF_FORMAT_NAMES_VOICE, 	FMT_STATIC },
	{ "FORMAT_NAMES_VOICECOLOR",	DEF_FORMAT_NAMES_VOICECOLOR, 	FMT_STATIC },
	{ "FORMAT_NETADD", 		DEF_FORMAT_NETADD, 		FMT_STATIC },
	{ "FORMAT_NETJOIN",		DEF_FORMAT_NETJOIN, 		FMT_STATIC },
	{ "FORMAT_NETSPLIT", 		DEF_FORMAT_NETSPLIT,		FMT_STATIC },
	{ "FORMAT_NETSPLIT_HEADER", 	DEF_FORMAT_NETSPLIT_HEADER,	FMT_STATIC },
	{ "FORMAT_NICKNAME", 		DEF_FORMAT_NICKNAME, 		FMT_STATIC },
	{ "FORMAT_NICKNAME_OTHER", 	DEF_FORMAT_NICKNAME_OTHER, 	FMT_STATIC },
	{ "FORMAT_NICKNAME_USER", 	DEF_FORMAT_NICKNAME_USER, 	FMT_STATIC },
	{ "FORMAT_NICK_AUTO", 		DEF_FORMAT_NICK_AUTO, 		FMT_STATIC },
	{ "FORMAT_NICK_COMP", 		DEF_FORMAT_NICK_COMP, 		FMT_STATIC },
	{ "FORMAT_NICK_MSG", 		DEF_FORMAT_NICK_MSG, 		FMT_STATIC },
	{ "FORMAT_NONICK", 		DEF_FORMAT_NONICK, 		FMT_STATIC },
	{ "FORMAT_NOTE", 		DEF_FORMAT_NOTE, 		FMT_STATIC },
	{ "FORMAT_NOTICE", 		DEF_FORMAT_NOTICE, 		FMT_STATIC },
	{ "FORMAT_NOTIFY_OFF", 		DEF_FORMAT_NOTIFY_OFF, 		FMT_STATIC },
	{ "FORMAT_NOTIFY_ON", 		DEF_FORMAT_NOTIFY_ON,		FMT_STATIC },
	{ "FORMAT_NOTIFY_SIGNOFF", 	DEF_FORMAT_NOTIFY_SIGNOFF, 	FMT_STATIC },
	{ "FORMAT_NOTIFY_SIGNOFF_UH", 	DEF_FORMAT_NOTIFY_SIGNOFF_UH, 	FMT_STATIC },
	{ "FORMAT_NOTIFY_SIGNON", 	DEF_FORMAT_NOTIFY_SIGNON, 	FMT_STATIC },
	{ "FORMAT_NOTIFY_SIGNON_UH", 	DEF_FORMAT_NOTIFY_SIGNON_UH, 	FMT_STATIC },
	{ "FORMAT_OPER", 		DEF_FORMAT_OPER, 		FMT_STATIC },
	{ "FORMAT_PUBLIC", 		DEF_FORMAT_PUBLIC, 		FMT_STATIC },
	{ "FORMAT_PUBLIC_AR", 		DEF_FORMAT_PUBLIC_AR, 		FMT_STATIC },
	{ "FORMAT_PUBLIC_MSG", 		DEF_FORMAT_PUBLIC_MSG, 		FMT_STATIC },
	{ "FORMAT_PUBLIC_MSG_AR", 	DEF_FORMAT_PUBLIC_MSG_AR, 	FMT_STATIC },
	{ "FORMAT_PUBLIC_NOTICE", 	DEF_FORMAT_PUBLIC_NOTICE, 	FMT_STATIC },
	{ "FORMAT_PUBLIC_NOTICE_AR", 	DEF_FORMAT_PUBLIC_NOTICE_AR, 	FMT_STATIC },
	{ "FORMAT_PUBLIC_OTHER", 	DEF_FORMAT_PUBLIC_OTHER, 	FMT_STATIC },
	{ "FORMAT_PUBLIC_OTHER_AR", 	DEF_FORMAT_PUBLIC_OTHER_AR, 	FMT_STATIC },
	{ "FORMAT_SEND_ACTION", 	DEF_FORMAT_SEND_ACTION, 	FMT_STATIC },
	{ "FORMAT_SEND_ACTION_OTHER",	DEF_FORMAT_SEND_ACTION_OTHER, 	FMT_STATIC },
	{ "FORMAT_SEND_AWAY", 		DEF_FORMAT_SEND_AWAY, 		FMT_STATIC },
	{ "FORMAT_SEND_CTCP", 		DEF_FORMAT_SEND_CTCP, 		FMT_STATIC },
	{ "FORMAT_SEND_DCC_CHAT", 	DEF_FORMAT_SEND_DCC_CHAT, 	FMT_STATIC },
	{ "FORMAT_SEND_MSG", 		DEF_FORMAT_SEND_MSG, 		FMT_STATIC },
	{ "FORMAT_SEND_NOTICE", 	DEF_FORMAT_SEND_NOTICE, 	FMT_STATIC },
	{ "FORMAT_SEND_PUBLIC", 	DEF_FORMAT_SEND_PUBLIC, 	FMT_STATIC },
	{ "FORMAT_SEND_PUBLIC_OTHER",	DEF_FORMAT_SEND_PUBLIC_OTHER, 	FMT_STATIC },
	{ "FORMAT_SERVER", 		DEF_FORMAT_SERVER, 		FMT_STATIC },
	{ "FORMAT_SERVER_MSG1", 	DEF_FORMAT_SERVER_MSG1, 	FMT_STATIC },
	{ "FORMAT_SERVER_MSG1_FROM", 	DEF_FORMAT_SERVER_MSG1_FROM,	FMT_STATIC },
	{ "FORMAT_SERVER_MSG2", 	DEF_FORMAT_SERVER_MSG2, 	FMT_STATIC },
	{ "FORMAT_SERVER_MSG2_FROM", 	DEF_FORMAT_SERVER_MSG2_FROM, 	FMT_STATIC },
	{ "FORMAT_SERVER_NOTICE", 	DEF_FORMAT_SERVER_NOTICE, 	FMT_STATIC },
	{ "FORMAT_SET", 		DEF_FORMAT_SET, 		FMT_STATIC },
	{ "FORMAT_SET_NOVALUE", 	DEF_FORMAT_SET_NOVALUE, 	FMT_STATIC },
	{ "FORMAT_SIGNOFF", 		DEF_FORMAT_SIGNOFF, 		FMT_STATIC },
	{ "FORMAT_SILENCE", 		DEF_FORMAT_SILENCE, 		FMT_STATIC },
	{ "FORMAT_SMODE", 		DEF_FORMAT_SMODE, 		FMT_STATIC },
	{ "FORMAT_STATUS", 		DEF_FORMAT_STATUS,		FMT_STATIC },
	{ "FORMAT_STATUS2", 		DEF_FORMAT_STATUS2, 		FMT_STATIC },
	{ "FORMAT_TIMER", 		DEF_FORMAT_TIMER, 		FMT_STATIC },
	{ "FORMAT_TOPIC", 		DEF_FORMAT_TOPIC, 		FMT_STATIC },
	{ "FORMAT_TOPIC_CHANGE", 	DEF_FORMAT_TOPIC_CHANGE, 	FMT_STATIC },
	{ "FORMAT_TOPIC_CHANGE_HEADER", DEF_FORMAT_TOPIC_CHANGE_HEADER, FMT_STATIC },
	{ "FORMAT_TOPIC_SETBY", 	DEF_FORMAT_TOPIC_SETBY, 	FMT_STATIC },
	{ "FORMAT_TOPIC_UNSET", 	DEF_FORMAT_TOPIC_UNSET, 	FMT_STATIC },
	{ "FORMAT_TRACE_OPER", 		DEF_FORMAT_TRACE_OPER, 		FMT_STATIC },
	{ "FORMAT_TRACE_SERVER",	DEF_FORMAT_TRACE_SERVER, 	FMT_STATIC },
	{ "FORMAT_TRACE_USER", 		DEF_FORMAT_TRACE_USER, 		FMT_STATIC },
	{ "FORMAT_USAGE", 		DEF_FORMAT_USAGE, 		FMT_STATIC },
	{ "FORMAT_USERMODE", 		DEF_FORMAT_USERMODE, 		FMT_STATIC },
	{ "FORMAT_USERS", 		DEF_FORMAT_USERS, 		FMT_STATIC },
	{ "FORMAT_USERS_HEADER", 	DEF_FORMAT_USERS_HEADER, 	FMT_STATIC },
	{ "FORMAT_USERS_USER", 		DEF_FORMAT_USERS_USER, 		FMT_STATIC },
	{ "FORMAT_VERSION", 		DEF_FORMAT_VERSION, 		FMT_STATIC },
	{ "FORMAT_WALL", 		DEF_FORMAT_WALL, 		FMT_STATIC },
	{ "FORMAT_WALLOP", 		DEF_FORMAT_WALLOP, 		FMT_STATIC },
	{ "FORMAT_WALL_AR",		DEF_FORMAT_WALL_AR, 		FMT_STATIC },
	{ "FORMAT_WHO", 		DEF_FORMAT_WHO, 		FMT_STATIC },
	{ "FORMAT_WHOIS_AWAY", 		DEF_FORMAT_WHOIS_AWAY, 		FMT_STATIC },
	{ "FORMAT_WHOIS_CHANNELS", 	DEF_FORMAT_WHOIS_CHANNELS, 	FMT_STATIC },
	{ "FORMAT_WHOIS_FOOTER", 	DEF_FORMAT_WHOIS_FOOTER, 	FMT_STATIC },
	{ "FORMAT_WHOIS_HEADER", 	DEF_FORMAT_WHOIS_HEADER, 	FMT_STATIC },
	{ "FORMAT_WHOIS_IDLE", 		DEF_FORMAT_WHOIS_IDLE, 		FMT_STATIC },
	{ "FORMAT_WHOIS_NAME", 		DEF_FORMAT_WHOIS_NAME, 		FMT_STATIC },
	{ "FORMAT_WHOIS_NICK", 		DEF_FORMAT_WHOIS_NICK, 		FMT_STATIC },
	{ "FORMAT_WHOIS_OPER", 		DEF_FORMAT_WHOIS_OPER, 		FMT_STATIC },
	{ "FORMAT_WHOIS_SERVER", 	DEF_FORMAT_WHOIS_SERVER, 	FMT_STATIC },
	{ "FORMAT_WHOIS_SIGNON", 	DEF_FORMAT_WHOIS_SIGNON, 	FMT_STATIC },
	{ "FORMAT_WHOLEFT_FOOTER", 	DEF_FORMAT_WHOLEFT_FOOTER, 	FMT_STATIC },
	{ "FORMAT_WHOLEFT_HEADER", 	DEF_FORMAT_WHOLEFT_HEADER, 	FMT_STATIC },
	{ "FORMAT_WHOLEFT_USER", 	DEF_FORMAT_WHOLEFT_USER, 	FMT_STATIC },
	{ "FORMAT_WHOWAS_HEADER", 	DEF_FORMAT_WHOWAS_HEADER, 	FMT_STATIC },
	{ "FORMAT_WHOWAS_NICK", 	DEF_FORMAT_WHOWAS_NICK, 	FMT_STATIC },
	{ "FORMAT_WIDELIST", 		DEF_FORMAT_WIDELIST, 		FMT_STATIC },
	{ "FORMAT_WINDOW_SET", 		DEF_FORMAT_WINDOW_SET, 		FMT_STATIC },
	/*--end--*/
	{NULL, NULL}
};

char *
get_fset_var (enum FSET_TYPES var)
{
	return (formatset_array[var].string);
}

static void 
set_fset_var_value (int var_index, char *value)
{
	struct format_set *var = &(formatset_array[var_index]);

	
	if ( value && *value == '\0' ) {
		put_it("%s", convert_output_format(get_fset_var(FORMAT_SET_FSET), "%s %s", var->name, var->string ? var->string : empty_string));
		return;
	}	

	if ( var->flags & FMT_STATIC ) 
		var->string = NULL;

	var->flags |= FMT_MODIFIED;

	if (value)
		malloc_strcpy (&(var->string), value);
	else
		xfree (&(var->string));
	say ("Value of %s set to %s", var->name, var->string ? var->string : "<EMPTY>");
}

static int 
find_fset_variable (struct format_set * array, char *org_name, int *cnt)
{
	struct format_set *v, *first;
	int len, var_index;
	char *name = NULL;

	malloc_strcpy (&name, org_name);
	upper (name);
	len = strlen (name);
	var_index = 0;
	for (first = array; first->name; first++, var_index++) {
		if (strncmp (name, first->name, len) == 0) {
			*cnt = 1;
			break;
		}
	}
	if (first->name) {
		if (strlen (first->name) != len) {
			v = first;
			for (v++; v->name; v++, (*cnt)++) {
				if (strncmp (name, v->name, len) != 0)
					break;
			}
		}
		xfree (&name);
		return (var_index);
	}
	else {
		*cnt = 0;
		xfree (&name);
		return (-1);
	}
}

static inline void 
fset_variable_casedef (char *name, int cnt, int var_index, char *args)
{
	for (cnt += var_index; var_index < cnt; var_index++)
		set_fset_var_value (var_index, args);
}

static inline void 
fset_variable_noargs (char *name)
{
	int var_index = 0;
	for (var_index = 0; var_index < NUMBER_OF_FSET; var_index++)
		set_fset_var_value (var_index, empty_string);
}

void
cmd_fset (struct command *cmd, char *args)
{
	char *var;
	char *name = NULL;
	int cnt, var_index;

	if ((var = next_arg (args, &args)) != NULL)
	{
		if (*var == '-')
		{
			var++;
			args = NULL;
		}
		var_index = find_fset_variable (formatset_array, var, &cnt);
		switch (cnt)
		{
		case 0:
			say ("No such variable \"%s\"", var);
			return;
		case 1:
			set_fset_var_value (var_index, args);
			return;
		default:
			say ("%s is ambiguous", var);
			fset_variable_casedef (name, cnt, var_index, empty_string);
			return;
		}
	}
	fset_variable_noargs (name);
}

int 
save_formats (FILE * outfile)
{
	int i;
	int count = 0;

	for (i = 0; i < NUMBER_OF_FSET; i++)
		if ( formatset_array[i].flags&FMT_MODIFIED ) {
			if (formatset_array[i].string)
				fprintf (outfile, "FSET %s %s\n", formatset_array[i].name, formatset_array[i].string);
			else
				fprintf (outfile, "FSET -%s\n", formatset_array[i].name);
			count++;
		}

	bitchsay ("Saved %d formats.", count);
	return 0;
}

char *
make_fstring_var (char *var_name)
{
	int cnt, msv_index;

	upper (var_name);
	if ((bsearch_array (formatset_array, sizeof (struct format_set), NUMBER_OF_FSET, var_name, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	return m_strdup (formatset_array[msv_index].string);
}
