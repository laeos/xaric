#ident "@(#)fset.c 1.8"
/*
 * fset.c - keep track of all our format strings
 * Modified by Rex Feany (laeos@laeos.net) for Xaric
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

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "irc.h"
#include "ircaux.h"
#include "screen.h"
#include "output.h"
#include "misc.h"
#include "vars.h"
#include "fset.h"
#include "alist.h"
#include "tcommand.h"
#include "format.h" 	/* default formats */


static struct format_set
{
	const char *name;
	char *value;
	char *old;
	int modified;
	int len;

} formatset_array[] = {

/*--start--*/
{"FORMAT_381",			DEF_FORMAT_381, 		NULL, 0, 0 },
{"FORMAT_391", 			DEF_FORMAT_391, 		NULL, 0, 0 },
{"FORMAT_443",	 		DEF_FORMAT_443, 		NULL, 0, 0 },
{"FORMAT_471",			DEF_FORMAT_471,			NULL, 0, 0 },
{"FORMAT_473",			DEF_FORMAT_473,			NULL, 0, 0 },
{"FORMAT_474",			DEF_FORMAT_474,			NULL, 0, 0 },
{"FORMAT_475",			DEF_FORMAT_475,			NULL, 0, 0 },
{"FORMAT_476",			DEF_FORMAT_476,			NULL, 0, 0 },
{"FORMAT_ACTION",		DEF_FORMAT_ACTION,		NULL, 0, 0 },
{"FORMAT_ACTION_OTHER",		DEF_FORMAT_ACTION_OTHER,	NULL, 0, 0 },
{"FORMAT_ALIAS",		DEF_FORMAT_ALIAS,		NULL, 0, 0 },
{"FORMAT_ASSIGN",		DEF_FORMAT_ASSIGN,		NULL, 0, 0 },
{"FORMAT_AWAY",			DEF_FORMAT_AWAY,		NULL, 0, 0 },
{"FORMAT_BACK",			DEF_FORMAT_BACK,		NULL, 0, 0 },
{"FORMAT_BANS",			DEF_FORMAT_BANS,		NULL, 0, 0 },
{"FORMAT_BANS_HEADER",		DEF_FORMAT_BANS_HEADER,		NULL, 0, 0 },
{"FORMAT_BWALL",		DEF_FORMAT_BWALL,		NULL, 0, 0 },
{"FORMAT_CHANNEL_SIGNOFF",	DEF_FORMAT_CHANNEL_SIGNOFF,	NULL, 0, 0 },
{"FORMAT_CONNECT",		DEF_FORMAT_CONNECT,		NULL, 0, 0 },
{"FORMAT_CTCP",			DEF_FORMAT_CTCP,		NULL, 0, 0 },
{"FORMAT_CTCP_REPLY",		DEF_FORMAT_CTCP_REPLY,		NULL, 0, 0 },
{"FORMAT_CTCP_UNKNOWN",		DEF_FORMAT_CTCP_UNKNOWN,	NULL, 0, 0 },
{"FORMAT_DCC",			DEF_FORMAT_DCC,			NULL, 0, 0 },
{"FORMAT_DCC_CHAT",		DEF_FORMAT_DCC_CHAT,		NULL, 0, 0 },
{"FORMAT_DCC_CONNECT",		DEF_FORMAT_DCC_CONNECT,		NULL, 0, 0 },
{"FORMAT_DCC_ERROR",		DEF_FORMAT_DCC_ERROR,		NULL, 0, 0 },
{"FORMAT_DCC_LOST",		DEF_FORMAT_DCC_LOST,		NULL, 0, 0 },
{"FORMAT_DCC_REQUEST",		DEF_FORMAT_DCC_REQUEST,		NULL, 0, 0 },
{"FORMAT_DESYNC",		DEF_FORMAT_DESYNC,		NULL, 0, 0 },
{"FORMAT_DISCONNECT",		DEF_FORMAT_DISCONNECT,		NULL, 0, 0 },
{"FORMAT_ENCRYPTED_NOTICE",	DEF_FORMAT_ENCRYPTED_NOTICE,	NULL, 0, 0 },
{"FORMAT_ENCRYPTED_PRIVMSG",	DEF_FORMAT_ENCRYPTED_PRIVMSG,	NULL, 0, 0 },
{"FORMAT_FLOOD",		DEF_FORMAT_FLOOD,		NULL, 0, 0 },
{"FORMAT_HOOK",			DEF_FORMAT_HOOK,		NULL, 0, 0 },
{"FORMAT_IGNORE_INVITE",	DEF_FORMAT_IGNORE_INVITE,	NULL, 0, 0 },
{"FORMAT_IGNORE_MSG",		DEF_FORMAT_IGNORE_MSG,		NULL, 0, 0 },
{"FORMAT_IGNORE_MSG_AWAY",	DEF_FORMAT_IGNORE_MSG_AWAY,	NULL, 0, 0 },
{"FORMAT_IGNORE_NOTICE",	DEF_FORMAT_IGNORE_NOTICE,	NULL, 0, 0 },
{"FORMAT_IGNORE_WALL",		DEF_FORMAT_IGNORE_WALL,		NULL, 0, 0 },
{"FORMAT_INVITE",		DEF_FORMAT_INVITE,		NULL, 0, 0 },
{"FORMAT_INVITE_USER",		DEF_FORMAT_INVITE_USER,		NULL, 0, 0 },
{"FORMAT_JOIN",			DEF_FORMAT_JOIN,		NULL, 0, 0 },
{"FORMAT_KICK",			DEF_FORMAT_KICK,		NULL, 0, 0 },
{"FORMAT_KICK_USER",		DEF_FORMAT_KICK_USER,		NULL, 0, 0 },
{"FORMAT_KILL",			DEF_FORMAT_KILL,		NULL, 0, 0 },
{"FORMAT_LEAVE",		DEF_FORMAT_LEAVE,		NULL, 0, 0 },
{"FORMAT_LINKS",		DEF_FORMAT_LINKS,		NULL, 0, 0 },
{"FORMAT_LIST",			DEF_FORMAT_LIST,		NULL, 0, 0 },
{"FORMAT_MODE",			DEF_FORMAT_MODE,		NULL, 0, 0 },
{"FORMAT_MODE_CHANNEL",		DEF_FORMAT_MODE_CHANNEL,	NULL, 0, 0 },
{"FORMAT_MSG",			DEF_FORMAT_MSG,			NULL, 0, 0 },
{"FORMAT_MSGLOG",		DEF_FORMAT_MSGLOG,		NULL, 0, 0 },
{"FORMAT_MSG_GROUP",		DEF_FORMAT_MSG_GROUP,		NULL, 0, 0 },
{"FORMAT_NAMES",		DEF_FORMAT_NAMES,		NULL, 0, 0 },
{"FORMAT_NAMES_BANNER",		DEF_FORMAT_NAMES_BANNER,	NULL, 0, 0 },
{"FORMAT_NAMES_FOOTER",		DEF_FORMAT_NAMES_FOOTER,	NULL, 0, 0 },
{"FORMAT_NAMES_IRCOP",		DEF_FORMAT_NAMES_IRCOP,		NULL, 0, 0 },
{"FORMAT_NAMES_NICKCOLOR",	DEF_FORMAT_NAMES_NICKCOLOR,	NULL, 0, 0 },
{"FORMAT_NAMES_NONOP",		DEF_FORMAT_NAMES_NONOP,		NULL, 0, 0 },
{"FORMAT_NAMES_OP",		DEF_FORMAT_NAMES_OP,		NULL, 0, 0 },
{"FORMAT_NAMES_OPCOLOR",	DEF_FORMAT_NAMES_OPCOLOR,	NULL, 0, 0 },
{"FORMAT_NAMES_VOICE",		DEF_FORMAT_NAMES_VOICE,		NULL, 0, 0 },
{"FORMAT_NAMES_VOICECOLOR",	DEF_FORMAT_NAMES_VOICECOLOR,	NULL, 0, 0 },
{"FORMAT_NETADD",		DEF_FORMAT_NETADD,		NULL, 0, 0 },
{"FORMAT_NETJOIN",		DEF_FORMAT_NETJOIN,		NULL, 0, 0 },
{"FORMAT_NETSPLIT",		DEF_FORMAT_NETSPLIT,		NULL, 0, 0 },
{"FORMAT_NETSPLIT_HEADER",	DEF_FORMAT_NETSPLIT_HEADER,	NULL, 0, 0 },
{"FORMAT_NICKNAME",		DEF_FORMAT_NICKNAME,		NULL, 0, 0 },
{"FORMAT_NICKNAME_OTHER",	DEF_FORMAT_NICKNAME_OTHER,	NULL, 0, 0 },
{"FORMAT_NICKNAME_USER",	DEF_FORMAT_NICKNAME_USER,	NULL, 0, 0 },
{"FORMAT_NICK_AUTO",		DEF_FORMAT_NICK_AUTO,		NULL, 0, 0 },
{"FORMAT_NICK_COMP",		DEF_FORMAT_NICK_COMP,		NULL, 0, 0 },
{"FORMAT_NICK_MSG",		DEF_FORMAT_NICK_MSG,		NULL, 0, 0 },
{"FORMAT_NONICK",		DEF_FORMAT_NONICK,		NULL, 0, 0 },
{"FORMAT_NOTE",			DEF_FORMAT_NOTE,		NULL, 0, 0 },
{"FORMAT_NOTICE",		DEF_FORMAT_NOTICE,		NULL, 0, 0 },
{"FORMAT_NOTIFY_OFF",		DEF_FORMAT_NOTIFY_OFF,		NULL, 0, 0 },
{"FORMAT_NOTIFY_ON",		DEF_FORMAT_NOTIFY_ON,		NULL, 0, 0 },
{"FORMAT_NOTIFY_SIGNOFF",	DEF_FORMAT_NOTIFY_SIGNOFF,	NULL, 0, 0 },
{"FORMAT_NOTIFY_SIGNOFF_UH",	DEF_FORMAT_NOTIFY_SIGNOFF_UH,	NULL, 0, 0 },
{"FORMAT_NOTIFY_SIGNON",	DEF_FORMAT_NOTIFY_SIGNON,	NULL, 0, 0 },
{"FORMAT_NOTIFY_SIGNON_UH",	DEF_FORMAT_NOTIFY_SIGNON_UH,	NULL, 0, 0 },
{"FORMAT_OPER",			DEF_FORMAT_OPER,		NULL, 0, 0 },
{"FORMAT_PUBLIC",		DEF_FORMAT_PUBLIC,		NULL, 0, 0 },
{"FORMAT_PUBLIC_MSG",		DEF_FORMAT_PUBLIC_MSG,		NULL, 0, 0 },
{"FORMAT_PUBLIC_NOTICE",	DEF_FORMAT_PUBLIC_NOTICE,	NULL, 0, 0 },
{"FORMAT_PUBLIC_OTHER",		DEF_FORMAT_PUBLIC_OTHER,	NULL, 0, 0 },
{"FORMAT_SCANDIR_LIST_FOOTER",	DEF_FORMAT_SCANDIR_LIST_FOOTER,	NULL, 0, 0 },
{"FORMAT_SCANDIR_LIST_HEADER",	DEF_FORMAT_SCANDIR_LIST_HEADER,	NULL, 0, 0 },
{"FORMAT_SCANDIR_LIST_LINE",	DEF_FORMAT_SCANDIR_LIST_LINE,	NULL, 0, 0 },
{"FORMAT_SEND_ACTION",		DEF_FORMAT_SEND_ACTION,		NULL, 0, 0 },
{"FORMAT_SEND_ACTION_OTHER",	DEF_FORMAT_SEND_ACTION_OTHER,	NULL, 0, 0 },
{"FORMAT_SEND_AWAY",		DEF_FORMAT_SEND_AWAY,		NULL, 0, 0 },
{"FORMAT_SEND_CTCP",		DEF_FORMAT_SEND_CTCP,		NULL, 0, 0 },
{"FORMAT_SEND_DCC_CHAT",	DEF_FORMAT_SEND_DCC_CHAT,	NULL, 0, 0 },
{"FORMAT_SEND_MSG",		DEF_FORMAT_SEND_MSG,		NULL, 0, 0 },
{"FORMAT_SEND_NOTICE",		DEF_FORMAT_SEND_NOTICE,		NULL, 0, 0 },
{"FORMAT_SEND_PUBLIC",		DEF_FORMAT_SEND_PUBLIC,		NULL, 0, 0 },
{"FORMAT_SEND_PUBLIC_OTHER",	DEF_FORMAT_SEND_PUBLIC_OTHER,	NULL, 0, 0 },
{"FORMAT_SERVER",		DEF_FORMAT_SERVER,		NULL, 0, 0 },
{"FORMAT_SERVER_MSG1",		DEF_FORMAT_SERVER_MSG1,		NULL, 0, 0 },
{"FORMAT_SERVER_MSG1_FROM",	DEF_FORMAT_SERVER_MSG1_FROM,	NULL, 0, 0 },
{"FORMAT_SERVER_MSG2",		DEF_FORMAT_SERVER_MSG2,		NULL, 0, 0 },
{"FORMAT_SERVER_MSG2_FROM",	DEF_FORMAT_SERVER_MSG2_FROM,	NULL, 0, 0 },
{"FORMAT_SERVER_NOTICE",	DEF_FORMAT_SERVER_NOTICE,	NULL, 0, 0 },
{"FORMAT_SET",			DEF_FORMAT_SET,			NULL, 0, 0 },
{"FORMAT_SET_NOVALUE",		DEF_FORMAT_SET_NOVALUE,		NULL, 0, 0 },
{"FORMAT_SIGNOFF",		DEF_FORMAT_SIGNOFF,		NULL, 0, 0 },
{"FORMAT_SILENCE",		DEF_FORMAT_SILENCE,		NULL, 0, 0 },
{"FORMAT_SMODE",		DEF_FORMAT_SMODE,		NULL, 0, 0 },
{"FORMAT_STATUS",		DEF_FORMAT_STATUS,		NULL, 0, 0 },
{"FORMAT_STATUS2",		DEF_FORMAT_STATUS2,		NULL, 0, 0 },
{"FORMAT_TIMER",		DEF_FORMAT_TIMER,		NULL, 0, 0 },
{"FORMAT_TOPIC",		DEF_FORMAT_TOPIC,		NULL, 0, 0 },
{"FORMAT_TOPIC_CHANGE",		DEF_FORMAT_TOPIC_CHANGE,	NULL, 0, 0 },
{"FORMAT_TOPIC_CHANGE_HEADER",	DEF_FORMAT_TOPIC_CHANGE_HEADER,	NULL, 0, 0 },
{"FORMAT_TOPIC_SETBY",		DEF_FORMAT_TOPIC_SETBY,		NULL, 0, 0 },
{"FORMAT_TOPIC_UNSET",		DEF_FORMAT_TOPIC_UNSET,		NULL, 0, 0 },
{"FORMAT_TRACE_OPER",		DEF_FORMAT_TRACE_OPER,		NULL, 0, 0 },
{"FORMAT_TRACE_SERVER",		DEF_FORMAT_TRACE_SERVER,	NULL, 0, 0 },
{"FORMAT_TRACE_USER",		DEF_FORMAT_TRACE_USER,		NULL, 0, 0 },
{"FORMAT_USAGE",		DEF_FORMAT_USAGE,		NULL, 0, 0 },
{"FORMAT_USERMODE",		DEF_FORMAT_USERMODE,		NULL, 0, 0 },
{"FORMAT_USERS",		DEF_FORMAT_USERS,		NULL, 0, 0 },
{"FORMAT_USERS_HEADER",		DEF_FORMAT_USERS_HEADER,	NULL, 0, 0 },
{"FORMAT_USERS_USER",		DEF_FORMAT_USERS_USER,		NULL, 0, 0 },
{"FORMAT_VERSION",		DEF_FORMAT_VERSION,		NULL, 0, 0 },
{"FORMAT_WALL",			DEF_FORMAT_WALL,		NULL, 0, 0 },
{"FORMAT_WALLOP",		DEF_FORMAT_WALLOP,		NULL, 0, 0 },
{"FORMAT_WHO",			DEF_FORMAT_WHO,			NULL, 0, 0 },
{"FORMAT_WHOIS_AWAY",		DEF_FORMAT_WHOIS_AWAY,		NULL, 0, 0 },
{"FORMAT_WHOIS_CHANNELS",	DEF_FORMAT_WHOIS_CHANNELS,	NULL, 0, 0 },
{"FORMAT_WHOIS_FOOTER",		DEF_FORMAT_WHOIS_FOOTER,	NULL, 0, 0 },
{"FORMAT_WHOIS_HEADER",		DEF_FORMAT_WHOIS_HEADER,	NULL, 0, 0 },
{"FORMAT_WHOIS_IDLE",		DEF_FORMAT_WHOIS_IDLE,		NULL, 0, 0 },
{"FORMAT_WHOIS_NAME",		DEF_FORMAT_WHOIS_NAME,		NULL, 0, 0 },
{"FORMAT_WHOIS_NICK",		DEF_FORMAT_WHOIS_NICK,		NULL, 0, 0 },
{"FORMAT_WHOIS_OPER",		DEF_FORMAT_WHOIS_OPER,		NULL, 0, 0 },
{"FORMAT_WHOIS_SERVER",		DEF_FORMAT_WHOIS_SERVER,	NULL, 0, 0 },
{"FORMAT_WHOIS_SIGNON",		DEF_FORMAT_WHOIS_SIGNON,	NULL, 0, 0 },
{"FORMAT_WHOLEFT_FOOTER",	DEF_FORMAT_WHOLEFT_FOOTER,	NULL, 0, 0 },
{"FORMAT_WHOLEFT_HEADER",	DEF_FORMAT_WHOLEFT_HEADER,	NULL, 0, 0 },
{"FORMAT_WHOLEFT_USER",		DEF_FORMAT_WHOLEFT_USER,	NULL, 0, 0 },
{"FORMAT_WHOWAS_HEADER",	DEF_FORMAT_WHOWAS_HEADER,	NULL, 0, 0 },
{"FORMAT_WHOWAS_NICK",		DEF_FORMAT_WHOWAS_NICK,		NULL, 0, 0 },
{"FORMAT_WIDELIST",		DEF_FORMAT_WIDELIST,		NULL, 0, 0 },
{"FORMAT_WINDOW_SET",		DEF_FORMAT_WINDOW_SET,		NULL, 0, 0 },
/*--end--*/

{NULL, NULL}
};


/*** Private ***/

/* reset a format to its original value */
static inline void 
reset_var (xformat fmt)
{
	if (formatset_array[fmt].modified) {
		new_free(&formatset_array[fmt].value);
		formatset_array[fmt].value = formatset_array[fmt].old;
		formatset_array[fmt].modified = 0;
		formatset_array[fmt].len = 0;
		say ("Reset [%s]", formatset_array[fmt].name);
	}
}

/* reset every format to there original values */
static inline void 
reset_all (void)
{
	xformat i;

	for (i = 0; i < NUMBER_OF_FSET; i++) 
		if (formatset_array[i].modified) 
			reset_var(i);
}

/* find a variable by name */
/* return location, or -1 if there were none found */
static xformat
find_format (const char *name, int *cnt)
{
	int loc;

	assert(name);
	assert(cnt);

	*cnt = 0;

	if ((find_fixed_array_item(formatset_array, sizeof (struct format_set), NUMBER_OF_FSET, name, cnt, &loc) == NULL))
		return -1;

	if ( *cnt < 0 )
		*cnt = -*cnt;

	assert(loc < NUMBER_OF_FSET);

	return loc;
}

/* set a format */
static void 
set_format (xformat loc, const char *value)
{
	struct format_set *var = &(formatset_array[loc]);
	
	if ( value && *value == '\0' ) {
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_SET_FSET), "%s %s", var->name, var->value ? var->value : empty_string));
		return;
	}

	if ( var->modified == 0 ) {
		var->old = var->value;
		var->value = NULL;
		var->modified = 1;
	}

	var->len = 0;
	if (value)
		malloc_strcpy (&(var->value), value);
	else if (var->value)
		new_free (&(var->value));
	say ("Value of %s set to %s", var->name, var->value ? var->value : "<EMPTY>");
}

static void 
set_format_range (xformat loc, int cnt, const char *arg)
{
	int end = loc + cnt;

	assert(loc >= 0 && loc < NUMBER_OF_FSET);
	assert(cnt >= 0 && cnt < NUMBER_OF_FSET);
	assert(end >= 0 && end < NUMBER_OF_FSET);

	for ( ; loc < end ; loc++ )
		set_format(loc, arg);
}



/*** Public ***/


/**
 * get_format_len - return printable length of format 
 * @which: what format
 *
 * When the format is displayed, how many printable
 * character spaces will it use? 
 **/
unsigned int
get_format_len(xformat which)
{
	char *conv;

	/* if already calculated, or NULL value */
	if (formatset_array[which].len || !formatset_array[which].value)
		return formatset_array[which].len;

	/* XXX */
	conv = convert_output_format(formatset_array[which].value, "", NULL);
	conv = stripansicodes(conv);

	return (formatset_array[which].len = strlen(conv));
}

/**
 * get_format - fetch a format string
 * @which: The ID of the format string.
 *
 * Return the format string for a given format.
 **/
const char *
get_format(xformat which) 
{
	return formatset_array[which].value;
}

/**
 * save_formats - write out the changed formats.
 * @outfile: file handle to write commands to.
 *
 * Write out any changes made to the format table in a format that 
 * we can load later.
 **/
int 
save_formats (FILE * outfile)
{
	int count = 0;
	xformat i;
	
	assert(outfile);

	for (i = 0; i < NUMBER_OF_FSET; i++)
		if ( formatset_array[i].modified ) {
			count++;
			if (formatset_array[i].value)
				fprintf (outfile, "FSET %s %s\n", formatset_array[i].name, formatset_array[i].value);
			else
				fprintf (outfile, "FSET -%s\n", formatset_array[i].name);
		}

	bitchsay ("Saved %d formats.", count);
	return 0;
}

/** 
 * get_format_byname - given a name, return the format string.
 * @name: Which format to return.
 *
 * Return a malloc'ed copy of  the format string 
 * given a name, like "FORMAT_FOO"
 **/
char *
get_format_byname (const char *name)
{
	int cnt;
	int loc;

	assert(name);

	if ((find_fixed_array_item(formatset_array, sizeof (struct format_set), NUMBER_OF_FSET, name, &cnt, &loc) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;

	assert(loc >= 0 && loc < NUMBER_OF_FSET);
	return m_strdup(formatset_array[loc].value);
}

/**
 * cmd_freset - reset modified formats to there compiled-in defaults
 * @cmd: command struct
 * @args: command arguments
 *
 * Takes either one or zero arguments.
 * With no arguments, resets all variables to there defaults.
 * With one argument of a variable name, reset that variable.
 **/
void
cmd_freset(struct command *cmd, char *args)
{
	char *var;
	int cnt = 0;

	assert(cmd);

	if ((var = next_arg (args, &args)) != NULL) {
		xformat loc = find_format (var, &cnt);

		switch (cnt) {
			case 0:
				say ("No such variable \"%s\"", var);
				break;
			case 1:
				reset_var(loc);
				break;
			default:
				say ("%s is ambiguous", var);
				set_format_range(loc, cnt, empty_string);
				break;
		}

	} else {
		reset_all();
	}
}

/**
 * cmd_fset - modify or query the format table
 * @cmd: command struct
 * @args: command arguments
 *
 * Takes zero, one, or two arguments.
 * With no arguments, print out the whole format table.
 * With one argument, display the value of any matching formats.
 * With one argument prefixed with '-' unset the format.
 * Two arguments, the format and the value to change it to.
 **/
void
cmd_fset (struct command *cmd, char *args)
{
	char *var;
	int cnt;

	assert(cmd);

	if ((var = next_arg (args, &args)) != NULL)
	{
		xformat loc;

		if (*var == '-') {
			var++;
			args = NULL;
		}
		loc = find_format (var, &cnt);
		switch (cnt) {
			case 0:
				say ("No such variable \"%s\"", var);
				break;
			case 1:
				set_format(loc, args);
				break;
			default:
				say ("%s is ambiguous", var);
				set_format_range(loc, cnt, empty_string);
				break;
		}
	} else 
		set_format_range(0, NUMBER_OF_FSET - 1, empty_string);
}

