#ifndef xformats_h__
#define xformats_h__

/*
 * xformats.h - mmm format set def'ns and stuff.
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
 * %W%
 *
 */

#include "tcommand.h"

/*
 * All of the formats. Index into array in xformats.c so be careful!
 */

typedef enum
{

/* these are automagicly generated from xformats.c by the Makefile. */
#include "xformats_gen.h" 

	NUMBER_OF_FSET
} xformat;


const char *get_format(xformat which);
char *make_fstring_var(char *var_name);
int save_formats (FILE * outfile);

void cmd_fset(struct command *cmd, char *args);
void cmd_freset(struct command *cmd, char *args);


#endif /* xformats_h__ */
