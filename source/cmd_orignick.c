#ident "@(#)cmd_orignick.c 1.5"
/*
 * cmd_hostname.c : virtual host support 
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
#include "list.h"
#include "whois.h"
#include "misc.h"
#include "input.h"
#include "vars.h"
#include "server.h"
#include "window.h"
#include "struct.h"
#include "screen.h"
#include "tcommand.h"
#include "timer.h"

static char *org_nick;

static void
change_orig_nick (void)
{
	change_server_nickname(from_server, org_nick);
	bitchsay("Regained nick [%s]", org_nick);
	new_free(&org_nick);
	update_all_status(curr_scr_win, NULL, 0);
	update_input(UPDATE_ALL);
}

static int delay_gain_nick(void *);

static void gain_nick (WhoisStuff *stuff, char *nick, char *args)
{
	if (!org_nick)
		return;
	if (!stuff || (stuff->nick  && !strcmp(stuff->user, "<UNKNOWN>") && !strcmp(stuff->host, "<UNKNOWN>")))
	{
		change_orig_nick();
		return;
	}
	if (get_int_var(ORIGNICK_TIME_VAR) > 0)
		add_timer("", get_int_var(ORIGNICK_TIME_VAR), 1, delay_gain_nick, NULL, NULL);
}

static int delay_gain_nick(void *arg)
{
	
	if (org_nick && from_server != -1)
		add_to_userhost_queue(org_nick, gain_nick, "%s", org_nick);
	return 0;
}

void check_orig_nick(char *nick)
{
	if (org_nick && !my_stricmp(nick, org_nick))
		change_orig_nick();
}

void cmd_orig_nick(struct command *cmd, char *args)
{
	char *nick;

	if (!args || !*args) {
		userage (cmd->name, cmd->qhelp);
		return;
	}
	nick = next_arg(args, &args);
	if (nick && *nick == '-') {
		if (!org_nick)
			bitchsay("Not trying to gain a nick");
		else {
			bitchsay("Removing gain nick [%s]", org_nick);
			new_free(&org_nick);
		}
	}
	else {
		if (nick && org_nick && !my_stricmp(nick, org_nick))
			bitchsay("Already attempting that nick %s", nick);
		else if ((nick = check_nickname(nick))) {
			malloc_strcpy(&org_nick, nick);
			add_to_userhost_queue(org_nick, gain_nick, "%s", org_nick);
			bitchsay("Trying to regain nick [%s]", org_nick);
		}
		else
			bitchsay("Nickname was all bad chars");
	}
}

