#ifndef bitlist_h__
#define bitlist_h__
/*
 * bitlist.h 
 * Copyright (C) 1998, 1999, 2000 Rex Feany <laeos@laeos.net>
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
 * @(#)bitlist.h 1.1
 *
 */

/* silly bit ops */
#define bits_of(t)	((sizeof(t)) * 8)

#define bit_isset(d, bit)	((*((u_int32_t *)(d)) + (bit) / bits_of(u_int32_t))  & ( 1 << ((bit) % bits_of(u_int32_t))))

/* put a list in buf, of the form "1,3,23" of all the "set" bits in data */
int bitlist(char *buf, size_t len, const void *data, size_t elmsz, size_t numel);

#endif /* bitlist_h__ */
