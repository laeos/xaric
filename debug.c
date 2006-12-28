#ident "@(#)debug.c 1.4"
/*
 * debug.c - support for *cough* debuging *cough* Xaric.
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
 * TODO:
 * 	turn newline chars to inverse chars
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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

/* Of course, this code only needs to exist if we are debugging! */
#ifdef XARIC_DEBUG

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "alist.h"
#include "debug.h"

/* Each module has an entry here. Please, its gotta be alphabetical order or get_module will fail. */
struct debug_module xd_modules[] = {
    {"CMD", 0},
    {"COMM", 0},
    {"EXPR", 0},
    {"FUNCTIONS", 0},
    {"READLINE", 0},
    {"TST", 0},
    {NULL, 0}
};

/* Get index for this module name, return 1 on error, 0 on ok */
static int get_module(const char *name, enum xd_module_list *ridx)
{
    int count, idx;
    char *rn = strdup(name);

    /* Fix case */
    upper(rn);

    find_fixed_array_item(xd_modules, sizeof(struct debug_module), XD_MAX_MODULE, rn, &count, &idx);
    if (count == 1)
	count = -1;
    free(rn);

    if (count < 0) {
	*ridx = (enum xd_module_list) idx;
	return 0;
    }
    return 1;
}

/* set this module's level */
static int xd_set(const char *module, xd_level lev, xd_level * oldvalue)
{
    enum xd_module_list idx;

    if (get_module(module, &idx))
	return 1;

    if (oldvalue)
	*oldvalue = xd_modules[idx].xd_level;

    xd_modules[idx].xd_level = lev;
    return 0;
}

/* Saves the current debug settings */
int xd_save(FILE * fp, int all)
{
    int i, cnt = 0;

    fputs("# debug settings\n", fp);

    for (i = 0; xd_modules[i].xd_name; i++) {
	if (xd_modules[i].xd_level || all) {
	    fprintf(fp, "debug %s=%d\n", xd_modules[i].xd_name, xd_modules[i].xd_level);
	    cnt++;
	}
    }
    fputc('\n', fp);

    return cnt;
}

/* 
 * parse a human-generated list of debug values. Returns 1 on failure, 0 on success
 * the format is:
 * 	section=1,3,5;section=3,5,6;section+4,3,2;section-4,3,2;section=0
 */
int xd_parse(const char *clist)
{
    char *save, *fcn = strdup(clist);
    char *what, *sep;
    int retval = 1;
    int done = 0;
    xd_level lev;

    save = fcn;
    do {
	if ((what = strchr(fcn, '=')) == NULL)
	    break;
	*what++ = '\0';

	if ((sep = strchr(what, ',')) == NULL)
	    retval = 0;
	else
	    *sep++ = '\0';

	if ((lev = atoi(what)) < 0)
	    break;

	if (xd_set(fcn, lev, NULL))
	    break;

	fcn = sep;

    } while (retval == 1);

    free(save);

    return ret;
}

/* List the status of the world according to debug */
void xd_list(int all)
{
    int i;

    say("--- Debug Settings");
    for (i = 0; xd_modules[i].xd_name; i++) {
	if (xd_modules[i].xd_level || all) {
	    say("> %10.10s ... %d", xd_modules[i].xd_name, xd_modules[i].xd_level);
	}
    }
}

/* We need to send a debug message. format it, and send it away. */
void xd_message(enum xd_module_list module, xd_level lev, char *file, int line, char *func, char *format, ...)
{
    char buffer[BIG_BUFFER_SIZE + 1];
    va_list ma;
    int len = 0;

    len = snprintf(buffer, BIG_BUFFER_SIZE, "%s@<%s:%d> ", func, file, line);

    va_start(ma, format);
    vsnprintf(buffer + len, BIG_BUFFER_SIZE - len, format, ma);
    va_end(ma);

    buffer[BIG_BUFFER_SIZE] = '\0';

    yell("dbg: %s", buffer);
}

#endif				/* XARIC_DEBUG */
