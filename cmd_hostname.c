#ident "@(#)cmd_hostname.c 1.6"
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

#include <stdio.h>

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "tcommand.h"

#include "iflist.h"

/* XXX this should/will be part of the network code */
/* XXX this will not work for IPv6 */
static void set_hostname(char *host)
{
    const struct iflist *ifl = iface_find(host);
    int reconn = 0;

    if (ifl == NULL) {
	yell("No such hostname [%s]!", host);
	return;
    }

    if (LocalHostName == NULL || strcmp(LocalHostName, ifl->ifi_host))
	reconn = 1;

    malloc_strcpy(&LocalHostName, ifl->ifi_host);
    memcpy((void *) &LocalHostAddr, &((struct sockaddr_in *) ifl->ifi_addr)->sin_addr.s_addr, sizeof(LocalHostAddr));

    bitchsay("Local host name is now %s", LocalHostName);
    if (reconn)
	t_parse_command("RECONNECT", NULL);
}

/* list interface info */
static void iface_callback(void *data, struct iflist *list)
{

    if (list == NULL && errno == 0) {
	put_it("%s", convert_output_format("%G Unable to find anything!", NULL, NULL));
    } else if (list == NULL) {
	yell("Error fetching interface info: %s", strerror(errno));
    } else {
	int i;

	say("Current hostnames available:");

	for (i = 1; list; list = list->ifi_next, i++) {
	    put_it("%s", convert_output_format("%K[%W$[3]0%K] %B$1 %g[%c$2%g]", "%d %s %s", i, list->ifi_host, list->ifi_name));
	}
    }
}

/**
 * cmd_hostname - change/list avaliable virtual hosts
 * @cmd: normal command struct
 * @args: command argument string
 *
 * Without any arguments, list the avaliable virtual hosts.
 * Given one argument, try to set the default host for outgoing
 * connections.
 **/
void cmd_hostname(struct command *cmd, char *args)
{
    if (args && *args && *args != '#') {
	set_hostname(args);
    } else {
	ifaces(iface_callback, NULL);
    }
}
