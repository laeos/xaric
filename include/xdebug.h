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
 * @(#)xdebug.h 1.2
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

#include "bitlist.h"

/* This is a list of all the "modules" we have */
enum xd_module_list 
{
	XD_CMD,
	XD_COMM,
	XD_EXPR,
	XD_FUNCTIONS,
	XD_READLINE,
	XD_TST,
	XD_MAX_MODULE
};

/* the type we use for the "knobs" */
typedef u_int32_t	xd_switch;

struct debug_module 
{
	char 		*xd_name;
	xd_switch	xd_switched;	/* we can use 0 - 31 */
};

/* This is where we keep information about the state of the debug world */
extern struct debug_module xd_modules[];

/* Yes.. this is a gcc extention. So sue me. I don't know anyone that DOES NOT use gcc! */
#define DEBUG(mod, sw, arg...)		\
			assert(sw < bits_of(xd_switch));		 \
			if ( xd_modules[(mod)].xd_switched & (1 << (sw))) \
				xd_message((mod), (sw), __FILE__, __LINE__, __FUNCTION__, arg)

/* Just an easy-ism. #define MODULE_ID in the source file before you use this. */
#define XDEBUG(sw, arg...)	DEBUG(MODULE_ID, sw, arg)

/* Scary looking, but true. */
void xd_message(enum xd_module_list module, xd_switch sw, char *file, int line, char *func, char *format, ...);

/* this lists the current debug settings. only the things that are turned on unless all is non-zero */
void xd_list(int all);

/* Saves the current debug settings */
int xd_save(FILE *fp, int all);

/* parse a human-generated list of debug values. Returns 1 on failure, 0 on succ
 * the format is:
 *	section=1,3,5;section=3,5,6;section+4,3,2;section-4,3,2;section=0
 */
int xd_parse(const char *clist);

#else /* XARIC_DEBUG */

#  define DEBUG(mod, sev, arg...)	
#  define XDEBUG(sev, arg...)

#endif /* XARIC_DEBUG */
#endif /* xdebug_h__ */

