#ident "@(#)bitlist.c 1.1"
/*
 * bitlist.c - turn a bitmask into a list of numbers
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "bitlist.h"

/* bitlist
 *
 * return number of chars used, -1 if the buffer is too small.
 *
 */

int 
bitlist(char *buf, size_t len, const void *data, size_t elmsz, size_t numel)
{
	unsigned int numbits = elmsz * 8 *  numel;
	unsigned int pos = 0;
	unsigned int bpos = 0;
	unsigned int wrt;

	assert(buf);
	assert(len);
	assert(data);
	assert(numbits);

	*buf = '\0';
	do {
		if ( bit_isset(data, pos)  == 0 )
			continue;

		if ( (wrt = snprintf(buf+bpos, len, "%d,", pos)) <= 0 )
			break;

		len -= wrt;
		bpos += wrt;

	} while (--numbits && ++pos && len);

	if ( bpos )
		buf[--bpos] = '\0'; /* get rid of the last , */

	if ( (bpos == len - 1) && numbits >= 0)
		return -1;	/* buffer was too small */

	return bpos;
}

