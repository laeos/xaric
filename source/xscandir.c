#ident "@(#)xscandir.c 1.7"
/*
 * xscandir.c - wrapper for scandir 
 * Copyright (C) 1999, 2000 Rex Feany <laeos@laeos.net>
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

#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif


/* Small parts shamelessly stolen from ircII help.c */
#if defined(HAVE_DIRENT_H) || defined(_POSIX_SOURCE)
# include <dirent.h>
# define NLENGTH(d) (strlen((d)->d_name))
#else /* DIRENT || _POSIX_SOURCE */
# define dirent direct
# define NLENGTH(d) ((d)->d_namlen)
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* HAVE_SYS_NDIR_H */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* HAVE_SYS_DIR_H */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif /* HAVE_NDIR_H */
#endif /* HAVE_DIRENT_H || _POSIX_VERSION */

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "struct.h"
#include "window.h"
#include "tcommand.h"
#include "util.h"
#include "ircterm.h"

#include "xformats.h"
#include "xmalloc.h"

/* sneeky */
typedef int (*scan_cmp_func)(const void *, const void *);
typedef int (*select_func)(const struct dirent *d);


/* used by scandir to sort help entries */
static int
dcompare (const struct dirent **d1, const struct dirent **d2)
{
	return strcasecmp( (*d1)->d_name, (*d2)->d_name );
}


/* we use these to keep track of what we should look for */
static char *prefix_str;
static int   prefix_len;

/* might as well find the longest, its easy */
static int the_longest;

/* used by scandir to select help entries, using regular expressions */
static int
dselect (const struct dirent *d)
{
	int t;

	if ( *(d->d_name) == '.' )
		return 0;

	if ( prefix_len ) {
		if ( strncmp(prefix_str, d->d_name, prefix_len) )
			return 0;
	}

	t = NLENGTH(d);

	if(t > the_longest) 
		the_longest = t;

	return 1;
}

/* return the name from the dentry, for list */
static const char *
d_name(void *ent)
{
	return ((struct dirent *)ent)->d_name;
}

/*
 * This is a wrapper for scandir.
 * It matches, it lists, and it pretty prints. If there is only
 * one match, we return the name in **ret (xmalloc'ed of course)
 * we return the return value from scandir. 
 *
 * if an error occurs, we return a negative value,
 * and set **ret to a text error message.
 *
 */

int 
xscandir(char *dir, char *prefix, char **ret)
{
	struct dirent **list = NULL;
	int retval = 0;
	int cnt = 0;
	int i;


	assert(dir);

	prefix_len = 0;

	if ( prefix ) {
		prefix_str = prefix;
		prefix_len = strlen(prefix);
	}

	the_longest = 0;
	cnt = retval = scandir(dir, &list, dselect, (scan_cmp_func) dcompare);

	switch (retval) {

		case -1:
			*ret = strerror(errno);
			break;
		case 0:
			break;

		case 1: 
			*ret = m_strdup( (*list)->d_name );
			break;

		default:
			/* hrm. if we get an exact prefix match, we want it */
			if (strcmp(prefix, (*list)->d_name) == 0)  {
				*ret = m_strdup( (*list)->d_name );
				retval = 1;
				break;
			}

			put_it("%s", convert_output_format(get_format(FORMAT_SCANDIR_LIST_HEADER_FSET), NULL, NULL));
			display_list_cl((XP_TST_ARRAY *) list, d_name, FORMAT_SCANDIR_LIST_LINE_FSET, retval, the_longest);
			put_it("%s", convert_output_format(get_format(FORMAT_SCANDIR_LIST_FOOTER_FSET), NULL, NULL));
			break;
	}

	for (i = 0; i < cnt; i++)
		free(list[i]);
	free(list);

	return retval;
}

