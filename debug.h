#ifndef xdebug_h__
#define xdebug_h__
/*
 * xdebug.h - debug.h seemed wrong. 
 * Copyright (C) 2000 Rex Feany <laeos@xaric.org>
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
 * @(#)debug.h 1.2
 *
 */

#ifdef XARIC_DEBUG

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

/* This is a list of all the "modules" we have */
enum xd_module_list {
	XD_CMD,
	XD_COMM,
	XD_EXPR,
	XD_FUNCTIONS,
	XD_READLINE,
	XD_TST,
	XD_MAX_MODULE
};

/* the type we use for the "knobs" */
typedef u_int32_t	xd_level;

struct debug_module {
	char 		*xd_name;
	xd_level	 xd_level;	/* some "level"  0, least info */
};

/* This is where we keep information about the state of the debug world */
extern struct debug_module xd_modules[];

/* debugging turned on for this module, level ? */
#define xdON(mod, lev)		(xd_modules[(mod)].xd_level >= (lev))

#define DEBUG(mod, lev, arg...)	\
		if (xdON(mod, lev))	\
			xd_message((mod), (lev), __FILE__, __LINE__, __FUNCTION__, arg)

/*  #define MODULE_ID in the source file before you use this. */
#define XDEBUG(lev, arg...)	DEBUG(MODULE_ID, lev, arg)

/* Scary looking, but true. */
void xd_message(enum xd_module_list module, xd_level lev, char *file, int line, char *func, char *format, ...);

/* this lists the current debug settings. only the things that are turned on unless all is non-zero */
void xd_list(int all);

/* Saves the current debug settings */
int xd_save(FILE *fp, int all);

/* parse a human-generated list of debug values. Returns 1 on failure, 0 on succ
 * the format is:
 *	section=level  ie, foo=4
 */
int xd_parse(const char *clist);

#else /* XARIC_DEBUG */

#  define DEBUG(mod, sev, arg...)	
#  define XDEBUG(sev, arg...)

#endif /* XARIC_DEBUG */
#endif /* xdebug_h__ */
