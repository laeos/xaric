#ident "@(#)cmd_hostname.c 1.11"
/*
 * cmd_hostname.c : virtual host support 
 * Copyright (C) 2000 Rex Feany <laeos@laeos.net>
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "list.h"
#include "misc.h"
#include "tcommand.h"

#include "xp_ifinfo.h"


static void
set_hostname(char *host)
{
	struct hostent *hp;
	int reconn = 0;

	if (LocalHostName == NULL || strcmp (LocalHostName, host))
		reconn = 1;

	malloc_strcpy (&LocalHostName, host);
	if ((hp = gethostbyname (LocalHostName)))
		memcpy ((void *) &LocalHostAddr, hp->h_addr, sizeof (LocalHostAddr));

	bitchsay ("Local host name is now %s", LocalHostName);
	if (reconn)
		t_parse_command ("RECONNECT", NULL);
}


void
cmd_hostname (struct command *cmd, char *args)
{
	if (args && *args && *args != '#') {
		set_hostname(args);
	} else {
		struct xp_iflist * l = xp_get_iflist();
		struct xp_iflist * c;
		int i;

		if ( l == NULL ) {
			put_it("%s", convert_output_format("%G Unable to find anything!", NULL, NULL));
			return;
		}

		for(c = l, i = 1; c; c = c->ifi_next, i++) {
			put_it("%s", convert_output_format("%K[%W$[3]0%K] %B$1 %g[%c$2%g]", "%d %s %s", i, c->ifi_host, c->ifi_name));
		}
		xp_free_iflist(l);
	}
	return;
}

