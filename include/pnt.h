#ifndef pnt_h__
#define pnt_h__

/*
 * pnt.h - pnt.c text formatting stuff
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
 * $Id$
 *
 */

#include <stdarg.h>

/* Argument types */

enum arg_types {
	A_END = 0,
	A_STR,
	A_CHAR,
	A_INT,
	A_HEX,
	A_FLOAT,
	A_TIME,		/* the curent time (takes no arguments) */
	A_ERRNO,	/* strerror() of the current errno (takes no arguments) */
	NUM_TYPES
};


/* Take the standard format strings with color codes, etc and format them. */
int vsnpnt(char *buf, int len, const char *fmt, va_list ma);
int snpnt(char *buf, int len, const char *fmt, ...);


#endif /* pnt_h__ */
