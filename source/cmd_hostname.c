#ident "$Id$"
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


#include <stdio.h>

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "list.h"
#include "misc.h"
#include "tcommand.h"

typedef struct _virtuals_struc
{
	struct _virtuals_struc *next;
	char *hostname;
}
Virtuals;


void
cmd_hostname (struct command *cmd, char *args)
{
	struct hostent *hp;
	Virtuals *virtuals = NULL;

	if (args && *args && *args != '#')
	{
		int reconn = 0;
		if (LocalHostName == NULL || strcmp (LocalHostName, args))
			reconn = 1;
		malloc_strcpy (&LocalHostName, args);
		if ((hp = gethostbyname (LocalHostName)))
			memcpy ((void *) &LocalHostAddr, hp->h_addr, sizeof (LocalHostAddr));

		bitchsay ("Local host name is now %s", LocalHostName);
		if (reconn)
			t_parse_command ("RECONNECT", NULL);
	}
	else
	{
#if !defined(__linux__) && !defined(BSD)
		bitchsay ("Local Host Name is [%s]", (LocalHostName) ? LocalHostName : hostname);
#else
		char filename[81];
		char comm[200];
		FILE *fptr;
		char *p, *q;
		unsigned long ip;
		Virtuals *new = NULL;
		struct hostent *host;
		int i;
		char *newhost = NULL;
#if defined(BSD)
		char device[80];
#endif

		tmpnam (filename);
#if defined(BSD)
		if (!(p = path_search ("netstat", "/sbin:/usr/sbin:/bin:/usr/bin")))
		{
			yell ("No Netstat to be found");
			return;
		}
		sprintf (comm, "%s -in >%s", p, filename);
#else
		sprintf (comm, "/sbin/ifconfig -a >%s", filename);
#endif
		system (comm);

		if ((fptr = fopen (filename, "r")) == NULL)
		{
			unlink (filename);
			return;
		}
#if defined(BSD)
		fgets (comm, 200, fptr);
		fgets (comm, 200, fptr);
		p = next_arg (comm, &q);
		strncpy (device, p, 79);
		bitchsay ("Looking for hostnames on device %s", device);
#endif
		while ((fgets (comm, 200, fptr)))
		{
#if defined(__linux__)
			if ((p = strstr (comm, "inet addr")))
			{
				p += 10;
				q = strchr (p, ' ');
				*q = 0;
				if ((p && !*p) || (p && !strcmp (p, "127.0.0.1")))
					continue;
#else /*(BSD) */
			if (!strncmp (comm, device, strlen (device)))
			{
				p = comm;
				p += 24;
				while (*p && *p == ' ')
					p++;
				q = strchr (p, ' ');
				*q = 0;
				if ((p && !*p) || (p && !strcmp (p, "127.0.0.1")))
					continue;
#endif
				ip = inet_addr (p);
				if ((host = gethostbyaddr ((char *) &ip, sizeof (ip), AF_INET)))
				{
					new = (Virtuals *) new_malloc (sizeof (Virtuals));
					new->hostname = m_strdup (host->h_name);
					add_to_list ((List **) & virtuals, (List *) new);
				}

			}
		}
		fclose (fptr);
		unlink (filename);
		for (new = virtuals, i = 1; virtuals; i++)
		{
			new = virtuals;
			virtuals = virtuals->next;
			if (i == 1)
				put_it ("%s", convert_output_format ("$G Current hostnames available", NULL, NULL));
			put_it ("%s", convert_output_format ("%K[%W$[3]0%K] %B$1", "%d %s", i, new->hostname));
			if (args && *args)
			{
				if (*args == '#')
					args++;
				if (args && *args && (i == my_atol (args)))
					malloc_strcpy (&newhost, new->hostname);

			}
			new_free (&new->hostname);
			new_free (&new);
		}
		if (newhost)
		{
			malloc_strcpy (&LocalHostName, newhost);
			if ((hp = gethostbyname (LocalHostName)))
				memcpy ((void *) &LocalHostAddr, hp->h_addr, sizeof (LocalHostAddr));

			bitchsay ("Local host name is now [%s]", LocalHostName);
			new_free (&newhost);
			t_parse_command ("RECONNECT", NULL);
		}
#endif
	}
}
