#ident "@(#)pnt.c 1.2"
/*
 * pnt.c : text formatting routines.
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "irc.h"
#include "pnt.h"
#include "vars.h"
#include "status.h"

/* Index into color_str */
enum color_attributes {	
	BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE, BLACKB,
	BLUEB, GREENB, CYANB, REDB, MAGENTAB, YELLOWB, WHITEB, NO_COLOR,
	BACK_BLACK, BACK_RED, BACK_GREEN, BACK_YELLOW, BACK_BLUE, BACK_MAGENTA,
	BACK_CYAN, BACK_WHITE, BACK_BBLACK, BACK_BRED, BACK_BGREEN,
	BACK_BYELLOW, BACK_BBLUE, BACK_BMAGENTA, BACK_BCYAN, BACK_BWHITE,
	REVERSE_COLOR, BOLD_COLOR, BLINK_COLOR, UNDERLINE_COLOR
};


/* Mmmm.. list of escape codes */
static char *color_str[] =
{
#ifdef CONFIG_REVERSE_WHITE_BLACK
	"\e[0m",    "\e[0;34m", "\e[0;32m", "\e[0;36m", "\e[0;31m", "\e[0;35m", "\e[0;33m", "\e[0;30m",
	"\e[1;37m", "\e[1;34m", "\e[1;32m", "\e[1;36m", "\e[1;31m", "\e[1;35m", "\e[1;33m", "\e[1;30m", "\e[0m",
	"\e[0;47m", "\e[0;41m", "\e[0;42m", "\e[0;43m", "\e[0;44m", "\e[0;45m", "\e[0;46m", "\e[0;40m",
	"\e[1;47m", "\e[1;41m", "\e[1;42m", "\e[1;43m", "\e[1;44m", "\e[1;45m", "\e[1;46m", "\e[1;40m",
	"\e[7m",    "\e[1m",    "\e[5m",    "\e[4m"
#else
	"\e[0;30m", "\e[0;34m", "\e[0;32m", "\e[0;36m", "\e[0;31m", "\e[0;35m", "\e[0;33m", "\e[0m",
	"\e[1;30m", "\e[1;34m", "\e[1;32m", "\e[1;36m", "\e[1;31m", "\e[1;35m", "\e[1;33m", "\e[1;37m", "\e[0m",
	"\e[0;40m", "\e[0;41m", "\e[0;42m", "\e[0;43m", "\e[0;44m", "\e[0;45m", "\e[0;46m", "\e[0;47m",
	"\e[1;40m", "\e[1;41m", "\e[1;42m", "\e[1;43m", "\e[1;44m", "\e[1;45m", "\e[1;46m", "\e[1;47m",
	"\e[7m",    "\e[1m",    "\e[5m",    "\e[4m"
#endif
};


/* Parse the argument list */
static __inline__ void
parse_arglist (char *buf, int len, va_list ma)
{
	enum arg_types type = va_arg(ma, enum arg_types);
	char *str;
	char c;
	int l = 0;

	while ( type != A_END ) {
		switch (type) {
			case A_STR:
				str = va_arg(ma, char *);

				if ( str == NULL )
					break;

				strncpy(buf, str, len);
				l = strlen(str);
				break;
			case A_CHAR:
				c = va_arg(ma, char);

				if ( c == 0 )
					break;

				*buf = c;
				l = 1;
				break;
			case A_INT:
				l = snprintf(buf, len, "%d", va_arg(ma, int));
				break;
			case A_HEX:
				l = snprintf(buf, len, "%x", va_arg(ma, int));
				break;
			case A_FLOAT:
				l = snprintf(buf, len, "%f", va_arg(ma, float));
				break;
			case A_TIME:
				str = update_clock(GET_TIME);
				strncpy(buf, str, len);
				l = strlen(str);
				break;
			case A_ERRNO:
				str = strerror(errno);
				strncpy(buf, str, len);
				l = strlen(str);
				break;

			default:
				ircpanic("Unknown format type [%d]", type);
				break;
		}
		buf += l;
		len -= l + 1;
		if ( len > 0 )
			*++buf = ' ';
		else
			break;
		type = va_arg(ma, enum arg_types);
	}
	if ( len == 0 )
		--buf;

	*buf = '\0';
}

static __inline__ enum color_attributes
parse_color (int c, int *boldish)
{
	int bold = *boldish;
	enum color_attributes this_color = BLACK;

	switch (c) {
		case 'n':
			this_color = NO_COLOR;
			break;
		case 'W':
			this_color = WHITEB;
			break;
		case 'w':
			this_color = WHITE;
			break;
		case 'K':
			this_color = BLACKB;
			break;
		case 'k':
			this_color = BLACK;
			break;
		case 'G':
			this_color = GREENB;
			break;
		case 'g':
			this_color = GREEN;
			break;
		case 'Y':
			this_color = YELLOWB;
			break;
		case 'y':
			this_color = YELLOW;
			break;
		case 'C':
			this_color = CYANB;
			break;
		case 'c':
			this_color = CYAN;
			break;
		case 'B':
			this_color = BLUEB;
			break;
		case 'b':
			this_color = BLUE;
			break;
		case 'P':
		case 'M':
			this_color = MAGENTAB;
			break;
		case 'p':
		case 'm':
			this_color = MAGENTA;
			break;
		case 'R':
			this_color = REDB;
			break;
		case 'r':
			this_color = RED;
			break;
		case '0':
			this_color = bold ? BACK_BBLACK : BACK_BLACK;
			bold = 0;
			break;
		case '1':
			this_color = bold ? BACK_BRED : BACK_RED;
			bold = 0;
			break;
		case '2':
			this_color = bold ? BACK_BGREEN : BACK_GREEN;
			bold = 0;
			break;
		case '3':
			this_color = bold ? BACK_BYELLOW : BACK_YELLOW;
			bold = 0;
			break;
		case '4':
			this_color = bold ? BACK_BBLUE : BACK_BLUE;
			bold = 0;
			break;
		case '5':
			this_color = bold ? BACK_BMAGENTA : BACK_MAGENTA;
			bold = 0;
			break;
		case '6':
			this_color = bold ? BACK_BCYAN : BACK_CYAN;
			bold = 0;
			break;
		case '7':
			this_color = bold ? BACK_BWHITE : BACK_WHITE;
			bold = 0;
			break;
		case '8':
			this_color = REVERSE_COLOR;
			bold = 0;
			break;
		case '9':
			this_color = BOLD_COLOR;
			bold ^= 1;
			break;
		case 'F':
			this_color = BLINK_COLOR;
			break;
		case 'U':
			this_color = UNDERLINE_COLOR;
			break;
		default:
			/* bad */
			this_color = BACK_BYELLOW;
			break;
	}
	*boldish = bold;
	return this_color;
}


#define ARGLEN 		(2 * BIG_BUFFER_SIZE + 1)

/* handle color/variable/function expantion */
int
vsnpnt(char *buf, int len, const char *fmt, va_list ma)
{
	int do_color = get_int_var (DISPLAY_ANSI_VAR);	
	enum color_attributes this_color = BLACK;
	char args[ARGLEN];
	const char *ptr = fmt;
	int lencpy = len;
	int bold = 0;
	int tmp;


	if ( ptr == NULL )
		return -1;


	args[0] = '\0';
	parse_arglist(args, ARGLEN, ma);

	buf[0] = '\0';

	while (*ptr) {
		switch (*ptr) {
			case '%':
				++ptr;
				if ( *ptr == '%' ) {
					*buf++ = *ptr;
					--len;
				} 
				else if (do_color) {
					this_color = parse_color(*ptr, &bold);
					strncpy(buf, color_str[this_color], len);
					tmp = strlen(buf);

					buf += tmp;
					len -= tmp;
				}
				++ptr;
				continue;

			case '$':

				/* not quite yet */

				break;

			default:
				*buf++ = *ptr++;
				--len;
		}

	}

	tmp = strlen(color_str[NO_COLOR]);

	if ( len > tmp )
		strcat (buf, color_str[NO_COLOR]);
	else
		strcat(buf - (tmp - len + 1), color_str[NO_COLOR]);

	return lencpy - len;
}

int
snpnt(char *buf, int len, const char *fmt, ...)
{
	int retval = 0;
	va_list ma;

	va_start(ma, fmt);
	retval = vsnpnt(buf, len, fmt, ma);
	va_end(ma);

	return retval;
}

