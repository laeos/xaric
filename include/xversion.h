#ifndef xversion_h__
#define xversion_h__

/*
 * xversion.h - include version information
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
 * @(#)xversion.h 1.9
 *
 */

#define _XVERSION_C_AS_HEADER_
#include "../source/xversion.c"
#undef _XVERSION_C_AS_HEADER_

/* In xintro.h, this has the copyright info in it */
void display_intro (void);

#endif /* xversion_h__ */
