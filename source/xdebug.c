#ident "@(#)xdebug.c 1.2"
/*
 * xdebug.c - support for *cough* debuging *cough* Xaric.
 * Copyright (C) 1998, 1999, 2000 Rex Feany <laeos@xaric.org>
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
# include "config.h"
#endif

#ifdef XARIC_DEBUG

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif


#include "irc.h"
#include "ircaux.h"
#include "bitlist.h"
#include "util.h"
#include "xdebug.h"
#include "output.h"


/* Each module has an entry here. Please, its gotta be alphabetical order or get_module will fail. */
struct debug_module xd_modules[] = {
	{ "CMD", 0 },
	{ "COMM", 0 },
	{ "EXPR", 0 },
	{ "FUNCTIONS", 0 },
	{ "READLINE", 0 },
	{ "TST", 0 },
	{ NULL, 0 }
};


/* Get index for this module name, return 1 on error, 0 on ok */
static int
get_module (const char *name, enum xd_module_list *ridx)
{
	int count, idx;
	char *rn = alloca(strlen(name) + 1);

	/* Fix case */
	strcpy(rn, name);
	upper(rn);

	bsearch_array(xd_modules, sizeof (struct debug_module), XD_MAX_MODULE, rn, &count, &idx);

	if ( count == 1 )
		count = -1;

	if ( count < 0 ) {
		*ridx = (enum xd_module_list) idx;
		return 0;
	}
	return 1;
}

/* turn a list of bit positions into a mask. we can write to list.  */
static int
to_mask(char *list, xd_switch *ptrmask)
{
	int done = 0, i;
	char *next, *num;
	xd_switch mask = 0;

	num = list;
	do {
		if ( (next = strchr(num, ',')) == NULL )
			done = 1;
		else
			*next++ = '\0';

		if ( (i = atoi(num)) < 0 || i > bits_of(xd_switch) )
			return 1;

		mask |= 1 << i;
		num = next;
	} while ( done == 0 );

	*ptrmask = mask;
	return 0;
}

/* set this module's switched */
static int
xd_set(const char *module, xd_switch value, xd_switch *oldvalue)
{
	enum xd_module_list idx;

	if ( get_module(module, &idx) )
		return 1;

	if ( oldvalue )
		*oldvalue = xd_modules[idx].xd_switched;

	xd_modules[idx].xd_switched = value;
	return 0;
}

/* Get this module's switched */
static int 
xd_get(const char *module, xd_switch *value)
{
	enum xd_module_list idx;

	if ( get_module(module, &idx) )
		return 1;

	*value = xd_modules[idx].xd_switched;
	return 0;
}

/* Saves the current debug settings */
int
xd_save(FILE *fp, int all)
{
	char buffer[BIG_BUFFER_SIZE+1];
	int i, cnt = 0;

	fputs("# debug settings\n", fp);

	for ( i = 0; xd_modules[i].xd_name ; i++ ) {
		if ( xd_modules[i].xd_switched == 0 || !all )
			continue;

		bitlist(buffer, BIG_BUFFER_SIZE, &xd_modules[i].xd_switched, sizeof(xd_switch), 1);

		fprintf(fp, "xdebug %s=%s\n", xd_modules[i].xd_name, buffer);
		cnt++;
	}
	fputc('\n', fp);

	return cnt;
}

/* 
 * parse a human-generated list of debug values. Returns 1 on failure, 0 on success
 * the format is:
 * 	section=1,3,5;section=3,5,6;section+4,3,2;section-4,3,2;section=0
 */
int 
xd_parse(const char *clist)
{
	char *fcn = alloca(strlen(clist) + 1);
	char *what, *sep;
	char ctype;
	int done = 0;
	xd_switch mask, oldval;

	strcpy(fcn, clist);

	do {
		if ( (what = sindex(fcn, "=-+")) == NULL )
			return 1;
	
		ctype = *what;
		*what++ = '\0';

		if ( (sep = strchr(what, ';')) == NULL ) 
			done = 1;
		else
			*sep++ = '\0';

		if ( to_mask(what, &mask) )
			return 1;
		
		if ( xd_get(fcn, &oldval) ) 
			return 1;

		switch (ctype) {
			case '+':
				mask |= oldval;
				break;
			case '=':
				break;
			case '-':	
				mask = (oldval & ~mask);
				break;
			default:
				return 1;
				break;
		}

		if ( xd_set(fcn, mask, NULL) )
			return 1;

		fcn = sep;

	} while ( done == 0 );

	return 0;
}

/* List the status of the world according to debug */
void
xd_list(int all)
{
	char buffer[BIG_BUFFER_SIZE+1];
	static const char dots[] = ".............";
	int i, len;

	say("--- Debug Settings");


	for ( i = 0; xd_modules[i].xd_name ; i++ ) {
		if ( all == 0 && xd_modules[i].xd_switched == 0 )
			continue;

		len = sizeof(dots);
		
		if ( len > strlen(xd_modules[i].xd_name) )
			len -= strlen(xd_modules[i].xd_name);

		bitlist(buffer, BIG_BUFFER_SIZE, &xd_modules[i].xd_switched, sizeof(xd_switch), 1);

		say("> %10.10s %*.*s", xd_modules[i].xd_name, 
				len, len, buffer);

	}
}

/* We need to send a debug message. format it, and send it away. */
void 
xd_message(enum xd_module_list module, xd_switch sw, char *file, int line, char *func, char *format, ...)
{
	int detail = 1;	/* XXX this needs to be a /set i think */
	char buffer[BIG_BUFFER_SIZE+1];
	va_list ma;
	int len = 0;

	va_start(ma, format);

	if ( detail )
		len = snprintf(buffer, BIG_BUFFER_SIZE, "%s@<%s:%d> ", func, file, line);

	vsnprintf(buffer+len, BIG_BUFFER_SIZE-len, format, ma);
	va_end(ma);

	buffer[BIG_BUFFER_SIZE] = '\0';

	yell("dbg: %s", buffer);
}

#endif /* XARIC_DEBUG */
