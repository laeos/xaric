#ifndef cmd_orignick_h__
#define cmd_orignick_h__

/*
 * header for cmd_orignick.c
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
 * @(#)cmd_orignick.h 1.1
 *
 */


/* Used when we see a nick change, or something, to speed up the process of
 * grabbing our old nick back. */

void check_orig_nick(char *nick);

#endif /* cmd_orignick_h__ */
