#ifndef fset_h__
#define fset_h__

/*
 * fset.h - mmm format set def'ns and stuff.
 * Copyright (C) 1998, 1999, 2000 Rex Feany <laeos@laeos.net>
 * 
 * This file is a part of <foo>, a <desc>
 * You can find <foo> at <url>
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
 * %W%
 *
 */

#include "tcommand.h"

/*
 * All of the formats. Index into array in fset.c so be careful!
 */

enum FSET_TYPES
{

/* these are automagicly generated from fset.c by the Makefile. */
#include "xformats_gen.h" 

	NUMBER_OF_FSET
};


char *get_fset_var(enum FSET_TYPES var);
void cmd_fset(struct command *cmd, char *args);
char *make_fstring_var(char *var_name);
int save_formats (FILE * outfile);


#endif /* fset_h__ */
