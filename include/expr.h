#ifndef expr_h__
#define expr_h__

/*
 * expr.h - header for expr.c (alias.c+expr.c)
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
 * %W%
 *
 */



/*
 * This function is a general purpose interface to alias expansion.
 * The second argument is the text to be expanded.
 * The third argument are the command line expandoes $0, $1, etc.
 * The fourth argument is a flag whether $0, $1, etc are used
 * The fifth argument, if set, controls whether only the first "command"
 *   will be expanded.  If set, this argument will be set to the "rest"
 *   of the commands (after the first semicolon, or the null).  If NULL,
 *   then the entire text will be expanded.
 */
char *expand_alias (char *, char *, int *, char **);


/* figure out what to do with the stuff after a '$' in the text. See
 * expr.c for more information. 
 */
char *alias_special_char(char **buffer, char *ptr, char *args, char *quote_em, int *args_flag);

/*
 * This function is in functions.c
 * This function allows you to execute a primitive "BUILT IN" expando.
 * These are the $A, $B, $C, etc expandoes.
 * The argument is the character of the expando (eg, 'A', 'B', etc)
 *
 * This is in functions.c
 */
char *built_in_alias(char);



char *my_next_expr(char **args, char typ, int whine);
char *next_expr_failok(char **args, char typ);
char *next_expr(char **args, char typ);


#endif /* expr_h__ */
