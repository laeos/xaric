#ident "@(#)util.c 1.3"
/*
 * util.c - various utility functions
 * Copyright (c) 2000 Rex Feany (laeos@xaric.org)
 *
 * This file is a part of Xaric, a silly IRC client.
 * You can find Xaric at http://www.xaric.org/.
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
 *
 * Many of these were stolen right from ircII:
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990, 1991
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>


#include "irc.h"
#include "util.h"
#include "ircaux.h"

/* Used with the constants in x_util.h, and maybe do_boolean */
const char *var_settings[] = { "OFF", "ON", "TOGGLE" };

/* Silly strings used around different places */
char zero_string[] = "0";
char one_string[] = "1";
char empty_string[] = "";
char space_string[] = " ";


/* is true if c is a valid nick name character (not including the first char) */
#define is_legal(c) ((((c) >= 'A') && ((c) <= '}')) || \
		     (((c) >= '0') && ((c) <= '9')) || \
		     (((c) == '-') || ((c) == '_')))


/* Check if this is a valid nick */
int
is_nick (const char *nick)
{
	if (!nick || *nick == '-' || isdigit (*nick))
		return 0;

	while ( *nick && is_legal(*nick) )
		nick++;

	return !*nick;
}

/* Check if this is a valid DCC nick (i.e. =nick) */
int 
is_dcc_nick (const char *nick)
{
	if (!nick || *nick != '=')
		return 0;

	return is_nick(++nick);
}

/* Check if this is a valid channel name */
int 
is_channel (const char *chan)
{
	int len = CHANNEL_MAX;

	if (!(chan && (*chan == '#' || *chan == '&') && *++chan))
		return 0;

	while ( *chan && --len && *chan != ' ' && *chan != '\007' && *chan != ',' )
		chan++;

	return !*chan;
}

/* Returns true if the given string is a number, false otherwise */
int 
is_number (const char *str)
{
	while (*str == ' ')
		str++;
	if (*str == '-')
		str++;
	if (*str) {
		for (; *str; str++) {
			if (!isdigit ((*str)))
				return 0;
		}
		return 1;
	}
	else
		return 0;
}

/* Expand ~ in path names. taken from ircII */
char *
expand_twiddle (const char *str)
{
	char buffer[BIG_BUFFER_SIZE + 1];

	if (*str == '~') {
		str++;
		if (*str == '/' || !*str) {
			snprintf(buffer, BIG_BUFFER_SIZE, "%s%s", my_path, str);
		}
		else {
			char *rest;
			struct passwd *entry;
			char *copy = strdup(str);

			if ((rest = strchr (copy, '/')) != NULL)
				*rest++ = '\0';
			if ((entry = getpwnam (copy)) != NULL) {

				/* gross.  XXX */
				snprintf(buffer, BIG_BUFFER_SIZE, "%s%c%s", 
							entry->pw_dir,
							rest ? '/' : '\0',
							rest ? rest : empty_string);
			}
			else
				return NULL;
		}
		return m_strdup(buffer);
	}
	else
		return m_strdup(str);

	return NULL;
}

/* just a handy thing.  Returns 1 if the str is not ON, OFF, or TOGGLE */
int 
do_boolean (const char *str, int *value)
{
	if (strcasecmp (str, on_string) == 0)
		*value = 1;
	else if (strcasecmp (str, off_string) == 0)
		*value = 0;
	else if (strcasecmp (str, toggle_string) == 0) {
		if (*value)
			*value = 0;
		else
			*value = 1;
	}
	else
		return 1;
	return 0;
}


/* convert a "long" to a string */
/* Does this belong here? */
char *
ltoa (long foo)
{
	static char buffer[BIG_BUFFER_SIZE + 1];
	char *pos = buffer + BIG_BUFFER_SIZE - 1;
	unsigned long absv;
	int negative;

	absv = (foo < 0) ? (unsigned long) -foo : (unsigned long) foo;
	negative = (foo < 0) ? 1 : 0;

	buffer[BIG_BUFFER_SIZE] = 0;
	for (; absv > 9; absv /= 10)
		*pos-- = (absv % 10) + '0';
	*pos = (absv) + '0';

	if (negative)
		*--pos = '-';

	return pos;
}


/* Convert string to all uppercase, in place. */
char *
upper (char *str)
{
	register char *ptr = NULL;

	if (str) {
		ptr = str;
		for (; *str; str++) {
			if (islower (*str))
				*str = toupper (*str);
		}
	}
	return (ptr);
}

/* Convert string to all lowercase, in place. */
char *
lower (char *str)
{
	register char *ptr = NULL;

	if (str) {
		ptr = str;
		for (; *str; str++) {
			if (isupper (*str))
				*str = tolower (*str);
		}
	}
	return (ptr);
}


/* Given a string, remove stuff at the end */
char *
terminate (char *string, char *chars) 
{
	char *ptr;

	if (!string || !chars)
		return "";
	while (*chars)
	{
		if ((ptr = strrchr(string, *chars)) != NULL)
			*ptr = '\0';
		chars++;
	}
	return string;
}
