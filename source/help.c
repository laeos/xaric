
/*
 * help.c -- Xaric help system
 * (c) 1997 Rex Feany (rfeany@qnet.com) 
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
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "vars.h"
#include "tcommand.h"

struct help_ent_
{
	struct help_ent_ *next;
	char *topic;
	long where;
};


typedef struct help_ent_ help_ent;


#define HASH_SIZE 40

static help_ent *help_list[HASH_SIZE];

static unsigned long
hash_str (char *str)
{
	register u_char *p = (u_char *) str;
	unsigned long hash = 0, g;

	while (*p)
	{
		hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p + 32) : *p);
		if ((g = hash & 0xF0000000))
			hash ^= g >> 24;
		hash &= ~g;
		p++;
	}
	return (hash % HASH_SIZE);
}


static long
find_topic (char *topic)
{
	unsigned long hash;
	help_ent *h;

	hash = hash_str (topic);
	for (h = help_list[hash]; h; h = h->next)
		if (strcasecmp (h->topic, topic) == 0)
			return h->where;
	return (long) -1;
}


static void
add_topic (char *topic, long loc)
{
	unsigned long hash;
	help_ent *h;

	if (find_topic (topic) != -1)
	{
		bitchsay ("Hey! you can only have one help entry for %s!", topic);
		return;
	}

	hash = hash_str (topic);
	h = new_malloc (sizeof (help_ent));

	h->topic = m_strdup (topic);
	h->where = loc;

	h->next = help_list[hash];
	help_list[hash] = h;
}


static void
display_topic (char *topic, FILE * f)
{
	long where;
	char buf[128];


	if ((where = find_topic (topic)) == -1)
	{
		bitchsay ("No help for %s", topic);
		return;
	}

	put_it ("%s", convert_output_format ("%G+-[ %cHelp: %C$0- %G]--------------------------------------", "%s", topic));
	fseek (f, where, SEEK_SET);	/* check for error? */
	fgets (buf, sizeof (buf) - 1, f);
	if (buf)
		buf[strlen (buf) - 1] = '\0';

	while (feof (f) == 0)
	{
		if (isspace (buf[0]) == 0)
			return;
		put_it ("%s", convert_output_format ("%G| %w$0-", "%s", convert_output_format (buf, NULL)));
		fgets (buf, sizeof (buf) - 1, f);
		if (buf)
			buf[strlen (buf) - 1] = '\0';
	}
}

static void
index_help_file (FILE * f)
{
	char buf[256];
	int num;

	rewind (f);
	num = 0;

	fgets (buf, sizeof (buf) - 1, f);
	if (buf)
		buf[strlen (buf) - 1] = '\0';
	while (feof (f) == 0)
	{
		if (isspace (buf[0]) == 0)
		{
			++num;
			add_topic (buf, ftell (f));
		}
		fgets (buf, sizeof (buf) - 1, f);
		if (buf)
			buf[strlen (buf) - 1] = '\0';
	}
}

void
cmd_help (struct command *cmd, char *args)
{
	static FILE *help_file = NULL;

	if (help_file == NULL)
	{
		if (!(help_file = fopen (get_string_var (HELP_VAR), "r")))
		{
			bitchsay ("Unable to open helpfile %s!", get_string_var (HELP_VAR));
			return;
		}
		index_help_file (help_file);
	}
	if (!args || !*args)
	{
		userage (cmd->name, cmd->qhelp);
		return;
	}
	display_topic (args, help_file);
}
