#ident "@(#)xintro.c 1.1"
/*
 * xintro.c - first things that are displayed when the client is started.
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "irc.h"
#include "struct.h"
#include "output.h"
#include "misc.h"
#include "util.h"
#include "xaric_version.h"

void
display_intro (void)
{
	int i = strip_ansi_in_echo;
	strip_ansi_in_echo = 0;

	put_it (empty_string);

	put_it(convert_output_format("%g***%n", NULL));
	put_it(convert_output_format("%g***%C $0-", "%s", XARIC_Hello));
	put_it(convert_output_format("%g***%n", NULL));
	put_it(convert_output_format("%g***%n Copyright (C) 1998, 1999, 2000 Rex G. Feany <laeos@laeos.ne>", NULL));
	put_it(convert_output_format("%g***%n Xaric is free software, covered by the GNU General Public License, and you are", NULL));
	put_it(convert_output_format("%g***%n welcome to change it and/or distribute copies of it under certain conditions.", NULL));
	put_it(convert_output_format("%g***%n Type \"/help copying\" to see the conditions.", NULL));
	put_it(convert_output_format("%g***%n There is absolutely no warranty for Xaric.  Type \"/help warranty\" for details.", NULL));
	put_it(convert_output_format("%g***%n", NULL));

	strip_ansi_in_echo = i;
}
