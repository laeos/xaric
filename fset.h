#ifndef fset_h__
#define fset_h__
/*
 * fset.h - mmm format set def'ns and stuff.
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
 * @(#)fset.h 1.5
 *
 */

#include "tcommand.h"

/*
 * All of the formats. Index into array in xformats.c so be careful!
 */

typedef enum {

/* these are automagicly generated from fset.c by the Makefile. */
#include "fset_gen.h"

    NUMBER_OF_FSET
} xformat;

#define get_fset_var	get_format

const char *get_format(xformat which);
unsigned int get_format_len(xformat which);
int save_formats(FILE * outfile);
char *get_format_byname(const char *name);
void cmd_freset(struct command *cmd, char *args);
void cmd_fset(struct command *cmd, char *args);

#endif				/* fset_h__ */
