#ident "@(#)cmd_who.c 1.7"
/*
 * cmd_who.c : the who command 
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
#include "misc.h"
#include "screen.h"
#include "tcommand.h"

void oh_my_wait (void);		/* in command.c for now XXX */


int doing_who = 0;


void
cmd_who (struct command *cmd, char *args)
{
	char *arg, *channel = NULL;
	int no_args = 1, len;

	doing_who = 1;

	who_mask = 0;
	new_free (&who_name);
	new_free (&who_host);
	new_free (&who_server);
	new_free (&who_file);
	new_free (&who_nick);
	new_free (&who_real);
	while ((arg = next_arg (args, &args)) != NULL)
	{
		lower (arg);
		no_args = 0;
		if ((*arg == '-' || *arg == '/') && (!isdigit (*(arg + 1))))
		{
			arg++;
			if ((len = strlen (arg)) == 0)
			{
				userage (cmd->name, cmd->qhelp);
				return;
			}
			if (!strncmp (arg, "o", 1))	/* OPS */
				who_mask |= WHO_OPS;
			else if (!strncmp (arg, "l", 1))	/* LUSERS */
				who_mask |= WHO_LUSERS;
			else if (!strncmp (arg, "c", 1))	/* CHOPS */
				who_mask |= WHO_CHOPS;
			else if (!strncmp (arg, "ho", 2))	/* HOSTS */
			{
				if ((arg = next_arg (args, &args)) != NULL)
				{
					who_mask |= WHO_HOST;
					malloc_strcpy (&who_host, arg);
					channel = who_host;
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
			}
			else if (!strncmp (arg, "he", 2))	/* here */
				who_mask |= WHO_HERE;
			else if (!strncmp (arg, "a", 1))	/* away */
				who_mask |= WHO_AWAY;
			else if (!strncmp (arg, "s", 1))	/* servers */
			{
				if ((arg = next_arg (args, &args)) != NULL)
				{
					who_mask |= WHO_SERVER;
					malloc_strcpy (&who_server, arg);
					channel = who_server;
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
			}
			else if (!strncmp (arg, "na", 2))	/* names */
			{
				if ((arg = next_arg (args, &args)) != NULL)
				{
					who_mask |= WHO_NAME;
					malloc_strcpy (&who_name, arg);
					channel = who_name;
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
			}
			else if (!strncmp (arg, "r", 1))	/* real name */
			{
				if ((arg = next_arg (args, &args)) != NULL)
				{
					who_mask |= WHO_REAL;
					malloc_strcpy (&who_real, arg);
					channel = who_real;
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
			}
			else if (!strncmp (arg, "ni", 2))	/* nick */
			{
				if ((arg = next_arg (args, &args)) != NULL)
				{
					who_mask |= WHO_NICK;
					malloc_strcpy (&who_nick, arg);
					channel = who_nick;
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
				/* WHO -FILE by Martin 'Efchen' Friedrich */
			}
			else if (!strncmp (arg, "f", 1))	/* file */
			{
				who_mask |= WHO_FILE;
				if ((arg = next_arg (args, &args)) != NULL)
				{
					malloc_strcpy (&who_file, arg);
				}
				else
				{
					userage (cmd->name, cmd->qhelp);
					return;
				}
			}
			else
			{
				userage (cmd->name, cmd->qhelp);
				return;
			}
		}
		else if (!strcmp (arg, "*"))
		{
			channel = get_current_channel_by_refnum (0);
			if (!channel || *channel == '0')

			{
				not_on_a_channel (curr_scr_win);
				return;
			}
		}
		else
			channel = arg;
	}
	if (no_args)
	{
		channel = get_current_channel_by_refnum (0);
		if (!channel || !*channel)
		{
			not_on_a_channel (curr_scr_win);
			return;
		}
		send_to_server ("WHO %s", channel);
		return;
	}
	if (no_args)
		bitchsay ("No arguements specified");
	else
	{
		if (!channel && who_mask & WHO_OPS)
			channel = "*.*";
		send_to_server ("%s %s %c", "WHO", channel ? channel :
				empty_str, (who_mask & WHO_OPS) ?
				'o' : '\0');
	}

}
