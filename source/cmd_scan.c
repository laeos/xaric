#ident "@(#)cmd_scan.c 1.7"
/*
 * cmd_scan.c : the /scan command
 *
 * Written By Michael Sandrof
 * Portions are based on EPIC.
 * Modified by panasync (Colten Edwards) 1995-97
 * Copyright(c) 1990
 * Modified for Xaric by Rex Feany <laeos@ptw.com> 1998
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

#include <stdio.h>

#include "irc.h"
#include "ircaux.h"
#include "whois.h"
#include "output.h"
#include "status.h"
#include "hash2.h"
#include "misc.h"
#include "screen.h"
#include "tcommand.h"
#include "fset.h"




void
cmd_scan (struct command *cmd, char *args)
{
	const char *fmt;
	int voice = 0, ops = 0, nops = 0, ircops = 0, all = 0;
	char *channel = NULL;
	ChannelList *chan;
	NickList *nick, *snick = NULL;
	char *s;
	char *buffer = NULL;
	int count = 0;
	int server;

	if (!my_stricmp (cmd->name, "scanv"))
		voice = 1;
	else if (!my_stricmp (cmd->name, "scano"))
		ops = 1;
	else if (!my_stricmp (cmd->name, "scann"))
		nops = 1;
	else if (!my_stricmp (cmd->name, "scani"))
		ircops = 1;
	else
		all = 1;

	while (args && *args)
	{
		s = next_arg (args, &args);
		if (is_channel (s))
			channel = s;
		else if (s && all)
		{
			all = 0;
			if (*s == 'v')
				voice = 1;
			else if (*s == 'o')
				ops = 1;
			else if (*s == 'n')
				nops = 1;
			else if (*s == 'i')
				ircops = 1;
		}
	}
	if (!(chan = prepare_command (&server, channel, NO_OP)))
		return;

	for (nick = next_nicklist (chan, NULL); nick; nick = next_nicklist (chan, nick))
	{
		if (voice && nick->voice)
			count++;
		else if (ops && nick->chanop)
			count++;
		else if (nops && !nick->chanop)
			count++;
		else if (ircops && nick->ircop)
			count++;
		else if (all)
			count++;
	}

	snick = sorted_nicklist (chan);
	if (voice)
		fmt = get_format (FORMAT_NAMES_VOICE_FSET);
	else if (ops)
		fmt = get_format (FORMAT_NAMES_OP_FSET);
	else if (ircops)
		fmt = get_format (FORMAT_NAMES_IRCOP_FSET);
	else if (nops)
		fmt = get_format (FORMAT_NAMES_NONOP_FSET);
	else
		fmt = get_format (FORMAT_NAMES_FSET);

	put_it ("%s", convert_output_format (fmt, "%s %s %d %s", update_clock (GET_TIME), chan->channel, count, space));
	if (count)
	{
		count = 0;
		for (nick = snick; nick; nick = nick->next)
		{
			if (all && (nick->chanop || nick->voice))
				malloc_strcat (&buffer, convert_output_format (get_format (nick->chanop ? FORMAT_NAMES_OPCOLOR_FSET : FORMAT_NAMES_VOICECOLOR_FSET), "%c %s", nick->chanop ? '@' : '+', nick->nick));
			else if (all)
				malloc_strcat (&buffer, convert_output_format (get_format (FORMAT_NAMES_NICKCOLOR_FSET), "%c %s", '$', nick->nick));
			else if (voice && nick->voice)
				malloc_strcat (&buffer, convert_output_format (get_format (FORMAT_NAMES_VOICECOLOR_FSET), "%c %s", '+', nick->nick));
			else if (ops && nick->chanop)
				malloc_strcat (&buffer, convert_output_format (get_format (FORMAT_NAMES_OPCOLOR_FSET), "%c %s", '@', nick->nick));
			else if (nops && !nick->chanop)
				malloc_strcat (&buffer, convert_output_format (get_format (FORMAT_NAMES_NICKCOLOR_FSET), "%c %s", '$', nick->nick));
			else if (ircops && nick->ircop)
				malloc_strcat (&buffer, convert_output_format (get_format (FORMAT_NAMES_OPCOLOR_FSET), "%c %s", '*', nick->nick));
			else
				continue;
			malloc_strcat (&buffer, space);
			if (count++ == 4)
			{
				if (get_format (FORMAT_NAMES_BANNER_FSET))
					put_it ("%s%s", convert_output_format (get_format (FORMAT_NAMES_BANNER_FSET), NULL, NULL), buffer);
				else
					put_it ("%s", buffer);
				new_free (&buffer);
				count = 0;
			}
		}
		if (count && buffer)
		{
			if (get_format (FORMAT_NAMES_BANNER_FSET))
				put_it ("%s%s", convert_output_format (get_format (FORMAT_NAMES_BANNER_FSET), NULL, NULL), buffer);
			else
				put_it ("%s", buffer);
		}
		if (get_format (FORMAT_NAMES_FOOTER_FSET))
			put_it ("%s", convert_output_format (get_format (FORMAT_NAMES_FOOTER_FSET), NULL, NULL));
		new_free (&buffer);
	}
	clear_sorted_nicklist (&snick);
}
