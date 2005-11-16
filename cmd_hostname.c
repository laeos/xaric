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
#include "server.h"
#include "iflist.h"

static void set_hostname(char *host)
{
    const struct iflist *ifl = iface_find(host);

    if (ifl == NULL) {
	yell("no such hostname [%s]!", host);
    } else if (local_host_name && strcmp(local_host_name, ifl->ifi_host)) {
	bitchsay("%s is already my hostname, yo.", local_host_name);
    } else {
	sa_addr_t *addr;
	sa_rc_t ret;

	malloc_strcpy(&local_host_name, ifl->ifi_host);
	sa_addr_create(&addr);
	ret = sa_addr_s2a(addr, ifl->ifi_addr, ifl->ifi_len);
	if (ret != SA_OK) {
	    sa_addr_destroy(addr);
	    bitchsay("couldn't set local hostname: %s", sa_error(ret));
	} else {
	    bitchsay("local host name is now %s", local_host_name);
	    if (local_host_addr)
		sa_addr_destroy(local_host_addr);
	    local_host_addr = addr;
	    t_parse_command("RECONNECT", NULL);
	}
    }
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
