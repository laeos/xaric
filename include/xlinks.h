/*
 * split.h - split handling / link looker for Xaric
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

#include "tst.h"


/* XXX needs to GO AWAY XXX */
struct nick;

/* This is used for the link looker */
struct server_link {
	struct server_link	*sp_next;

	char 		*sp_name;	/* server that is split */
	char		*sp_link;	/* linked to what server */

	unsigned int	 sp_hopcount;	/* number of hops away */

#ifdef XARIC_LLOOK

/* Status of this link */
#define	SP_OK		0x0000
#define	SP_SPLIT	0x0001

	unsigned int	 sp_status;	/* split or.. ? */
	unsigned int 	 sp_count;	/* number of times we have not found this one */t
#endif

	char 		*sp_time;	/* time of split */

	TST_HEAD	*sp_splitoff;	/* users we lost because of this split */
};


/* -- generic list stuff.. used for splits, link looker, irc map */


/* Is this link in the list */
struct server_link *find_link (struct server_link **list, const char *serv);

/* Add this link to a given list */
struct server_link *add_link (struct server_link **list, const char *serv, const char *link, int hops, const char *splittime);

/* Remove this link from the given list */
void remove_link (struct server_link **list, const char *serv);

/* Flush this list out */
void flush_links (struct server_link **list);


/* -- watch for splits */

/* is this a split quit msg? */
int is_split (const char *nick, const char *chan, const char *reason); 

/* See if this server is split */
struct server_link *check_split (const char *serv);

/* add a split */
struct server_link *add_split (const char *serv, const char *link, unsigned int hops);

/* this user went away */
void add_split_user (struct server_link *spl, struct nick *who);

/* Ok, this server is not split anymore. bye bye */
void flush_split(struct server_link *spl);

/* Who left? NULL == current split */
void show_who_left(char *serv);


/* -- irc server map */

/* Add this link to the map */
void add_to_link_map (const char *s1, const char *s2);

/* Show the link map */
void show_link_map (void);





