#ident "@(#)cmd_save.c 1.10"
/*
 * cmd_save.c : save Xaric settings 
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
#include "output.h"
#include "misc.h"
#include "screen.h"
#include "hook.h"
#include "vars.h"
#include "fset.h"
#include "notify.h"
#include "keys.h"
#include "tcommand.h"
#include "util.h"


#define SFLAG_BIND      0x0001
#define SFLAG_ON        0x0002
#define SFLAG_SET       0x0004
#define SFLAG_NOTIFY    0x0008
#define SFLAG_DIGRAPH   0x0010
#define SFLAG_ALL	0xffff

static int save_which;

static void
really_save (char *file, char *line)
{
	FILE *fp;
	int save_do_all = 0;

	if (*line != 'y' && *line != 'Y')
		return;
	if ((fp = fopen (file, "w")) != NULL)
	{
		if (save_which & SFLAG_BIND)
			save_bindings (fp, save_do_all);
		if (save_which & SFLAG_ON)
			save_hooks (fp, save_do_all);
		if (save_which & SFLAG_NOTIFY)
			save_notify (fp);
		if (save_which & SFLAG_SET)
			save_variables (fp, save_do_all);
		fclose (fp);
		bitchsay ("Xaric settings saved to %s", file);
	}
	else
		bitchsay ("Error opening %s: %s", file, strerror (errno));
}

void
save_all (char *fname)
{
	save_which = SFLAG_ALL;
	really_save (fname, "y");
}


/* save_settings: saves the current state of IRCII to a file */
void
cmd_save (struct command *cmd, char *args)
{
	char *arg = NULL;
	int all = 1;
	char buffer[BIG_BUFFER_SIZE + 1];

	save_which = 0;
	while ((arg = next_arg (args, &args)) != NULL)
	{
		if ('-' == *arg || '/' == *arg)
		{
			all = 0;
			arg++;
			if (0 == my_strnicmp ("B", arg, 1))
				save_which |= SFLAG_BIND;
			else if (0 == my_strnicmp ("O", arg, 1))
				save_which |= SFLAG_ON;
			else if (0 == my_strnicmp ("S", arg, 1))
				save_which |= SFLAG_SET;
			else if (0 == my_strnicmp ("N", arg, 1))
				save_which |= SFLAG_NOTIFY;
			else if (0 == my_strnicmp ("ALL", arg, 3))
				save_which = SFLAG_ALL;
			else
			{
				userage (cmd->name, cmd->qhelp);
				return;
			}
			continue;
		}
		if (!(arg = expand_twiddle (ircrc_file)))
		{
			bitchsay ("Unknown user");
			return;
		}
	}
	if (all)
		save_which = SFLAG_ALL;
	snprintf (buffer, BIG_BUFFER_SIZE, "Really write %s? ", arg ? arg : ircrc_file);
	add_wait_prompt (buffer, really_save, arg ? arg : ircrc_file, WAIT_PROMPT_LINE);
}
