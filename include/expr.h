#ifndef expr_h__
#define expr_h__
/*
 * expr.h - header file for expr.c, expression handling
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

char *expand_alias(char *string, char *args, int *args_flag, char **more_text);
char *alias_special_char(char **buffer, char *ptr, char *args, char *quote_em, int *args_flag);
char *my_next_expr(char **args, char type, int whine);
char *next_expr_failok(char **args, char type);
char *next_expr(char **args, char type);

#endif /* expr_h__ */
