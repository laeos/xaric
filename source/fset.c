#ident "@(#)fset.c 1.10"
/*
 * fset.c - keep track of all our format strings
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


#define FMT_STATIC	0x01	/* Use this to remeber if we need to free() or not */
#define FMT_MODIFIED	0x02	/* Use this to remeber if we changed it or not (for save) */

#define FMT(x)		{ #x, DEF_ ## x, FMT_STATIC },

struct format_set
{
	char *name;
	char *string;
	short flags;
} formatset_array[] = {
	FMT(FORMAT_381)
	FMT(FORMAT_391)
	FMT(FORMAT_443)
	FMT(FORMAT_471)
	FMT(FORMAT_473)
	FMT(FORMAT_474)
	FMT(FORMAT_475)
	FMT(FORMAT_476)

	FMT(FORMAT_ACTION)
	FMT(FORMAT_ACTION_OTHER)
	FMT(FORMAT_ALIAS)
	FMT(FORMAT_ASSIGN)
	FMT(FORMAT_AWAY)
	FMT(FORMAT_BACK)
	FMT(FORMAT_BANS)
	FMT(FORMAT_BANS_HEADER)
	FMT(FORMAT_BWALL)

	FMT(FORMAT_CHANNEL_SIGNOFF)

	FMT(FORMAT_CONNECT)
	FMT(FORMAT_CTCP)
	FMT(FORMAT_CTCP_REPLY)
	FMT(FORMAT_CTCP_UNKNOWN)
	FMT(FORMAT_DCC)
	FMT(FORMAT_DCC_CHAT)
	FMT(FORMAT_DCC_CONNECT)
	FMT(FORMAT_DCC_ERROR)
	FMT(FORMAT_DCC_LOST)
	FMT(FORMAT_DCC_REQUEST)
	
	FMT(FORMAT_DESYNC)

	FMT(FORMAT_DISCONNECT)
	FMT(FORMAT_ENCRYPTED_NOTICE)
	FMT(FORMAT_ENCRYPTED_PRIVMSG)
	FMT(FORMAT_FLOOD)

	FMT(FORMAT_HOOK)

	FMT(FORMAT_IGNORE_INVITE)
	FMT(FORMAT_IGNORE_MSG)
	FMT(FORMAT_IGNORE_MSG_AWAY)
	FMT(FORMAT_IGNORE_NOTICE)
	FMT(FORMAT_IGNORE_WALL)

	FMT(FORMAT_INVITE)
	FMT(FORMAT_INVITE_USER)
	FMT(FORMAT_JOIN)
	FMT(FORMAT_KICK)
	FMT(FORMAT_KICK_USER)
	FMT(FORMAT_KILL)
	FMT(FORMAT_LEAVE)
	FMT(FORMAT_LINKS)
	FMT(FORMAT_LIST)
	FMT(FORMAT_MODE)
	FMT(FORMAT_MODE_CHANNEL)
	FMT(FORMAT_MSG)
	FMT(FORMAT_MSGLOG)
	FMT(FORMAT_MSG_GROUP)
	FMT(FORMAT_NAMES)
	FMT(FORMAT_NAMES_BANNER)

	FMT(FORMAT_NAMES_FOOTER)
	FMT(FORMAT_NAMES_IRCOP)
	FMT(FORMAT_NAMES_NICKCOLOR)
	FMT(FORMAT_NAMES_NONOP)
	FMT(FORMAT_NAMES_OP)
	FMT(FORMAT_NAMES_OPCOLOR)
	FMT(FORMAT_NAMES_VOICE)
	FMT(FORMAT_NAMES_VOICECOLOR)

	FMT(FORMAT_NETADD)
	FMT(FORMAT_NETJOIN)
	FMT(FORMAT_NETSPLIT)
	FMT(FORMAT_NETSPLIT_HEADER)
	FMT(FORMAT_NICKNAME)
	FMT(FORMAT_NICKNAME_OTHER)
	FMT(FORMAT_NICKNAME_USER)
	FMT(FORMAT_NICK_AUTO)
	FMT(FORMAT_NICK_COMP)
	FMT(FORMAT_NICK_MSG)
	FMT(FORMAT_NONICK)
	FMT(FORMAT_NOTE)
	FMT(FORMAT_NOTICE)
	FMT(FORMAT_NOTIFY_OFF)
	FMT(FORMAT_NOTIFY_ON)
	FMT(FORMAT_NOTIFY_SIGNOFF)
	FMT(FORMAT_NOTIFY_SIGNOFF_UH)
	FMT(FORMAT_NOTIFY_SIGNON)
	FMT(FORMAT_NOTIFY_SIGNON_UH)
	FMT(FORMAT_OPER)
	FMT(FORMAT_PUBLIC)
	FMT(FORMAT_PUBLIC_AR)
	FMT(FORMAT_PUBLIC_MSG)
	FMT(FORMAT_PUBLIC_MSG_AR)
	FMT(FORMAT_PUBLIC_NOTICE)
	FMT(FORMAT_PUBLIC_NOTICE_AR)
	FMT(FORMAT_PUBLIC_OTHER)
	FMT(FORMAT_PUBLIC_OTHER_AR)
	FMT(FORMAT_SEND_ACTION)
	FMT(FORMAT_SEND_ACTION_OTHER)
	FMT(FORMAT_SEND_AWAY)
	FMT(FORMAT_SEND_CTCP)
	FMT(FORMAT_SEND_DCC_CHAT)
	FMT(FORMAT_SEND_MSG)
	FMT(FORMAT_SEND_NOTICE)
	FMT(FORMAT_SEND_PUBLIC)
	FMT(FORMAT_SEND_PUBLIC_OTHER)
	FMT(FORMAT_SERVER)
	FMT(FORMAT_SERVER_MSG1)
	FMT(FORMAT_SERVER_MSG1_FROM)
	FMT(FORMAT_SERVER_MSG2)
	FMT(FORMAT_SERVER_MSG2_FROM)
	FMT(FORMAT_SERVER_NOTICE)

	FMT(FORMAT_SET)
	FMT(FORMAT_SET_NOVALUE)
	FMT(FORMAT_SIGNOFF)
	FMT(FORMAT_SILENCE)
	FMT(FORMAT_SMODE)
	FMT(FORMAT_STATUS)
	FMT(FORMAT_STATUS2)
	FMT(FORMAT_TIMER)
	FMT(FORMAT_TOPIC)
	FMT(FORMAT_TOPIC_CHANGE)
	FMT(FORMAT_TOPIC_CHANGE_HEADER)
	FMT(FORMAT_TOPIC_SETBY)
	FMT(FORMAT_TOPIC_UNSET)
	FMT(FORMAT_TRACE_OPER)
	FMT(FORMAT_TRACE_SERVER)
	FMT(FORMAT_TRACE_USER)
	FMT(FORMAT_USAGE)
	FMT(FORMAT_USERMODE)
	FMT(FORMAT_USERS)
	FMT(FORMAT_USERS_HEADER)
	FMT(FORMAT_USERS_USER)
	FMT(FORMAT_VERSION)
	FMT(FORMAT_WALL)
	FMT(FORMAT_WALLOP)
	FMT(FORMAT_WALL_AR)
	FMT(FORMAT_WHO)
	FMT(FORMAT_WHOIS_AWAY)
	FMT(FORMAT_WHOIS_CHANNELS)
	FMT(FORMAT_WHOIS_FOOTER)
	FMT(FORMAT_WHOIS_HEADER)
	FMT(FORMAT_WHOIS_IDLE)
	FMT(FORMAT_WHOIS_NAME)
	FMT(FORMAT_WHOIS_NICK)
	FMT(FORMAT_WHOIS_OPER)
	FMT(FORMAT_WHOIS_SERVER)
	FMT(FORMAT_WHOIS_SIGNON)
	FMT(FORMAT_WHOLEFT_FOOTER)
	FMT(FORMAT_WHOLEFT_HEADER)
	FMT(FORMAT_WHOLEFT_USER)
	FMT(FORMAT_WHOWAS_HEADER)
	FMT(FORMAT_WHOWAS_NICK)
	FMT(FORMAT_WIDELIST)
	FMT(FORMAT_WINDOW_SET)
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
		new_free (&(var->string));
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
		new_free (&name);
		return (var_index);
	}
	else {
		*cnt = 0;
		new_free (&name);
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
