
/*
 * debug.c -- controll the values of x_debug.
 *
 * Written by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs
 * See the COPYRIGHT file for more information
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "tcommand.h"

unsigned long x_debug = 0;

struct debug_opts
{
	char *command;
	int flag;
};

static struct debug_opts opts[] =
{
	{"LOCAL_VARS", DEBUG_LOCAL_VARS},
	{"CTCPS", DEBUG_CTCPS},
	{"DCC_SEARCH", DEBUG_DCC_SEARCH},
	{"OUTBOUND", DEBUG_OUTBOUND},
	{"INBOUND", DEBUG_INBOUND},
	{"DCC_XMIT", DEBUG_DCC_XMIT},
	{"WAITS", DEBUG_WAITS},
	{"MEMORY", DEBUG_MEMORY},
	{"SERVER_CONNECT", DEBUG_SERVER_CONNECT},
	{"CRASH", DEBUG_CRASH},
	{"ALL", DEBUG_ALL},
	{NULL, 0},
};



void
cmd_debug (struct command *cmd, char *args)
{
	int cnt;
	int remove = 0;
	char *this_arg;

	if (!args || !*args)
	{
		char buffer[540];
		char *q;
		int i = 0;

		buffer[0] = 0;
		strmcat (buffer, "[-][+][option(s)] ", 511);
		q = &buffer[strlen (buffer)];
		for (i = 0; opts[i].command; i++)
		{
			if (q)
				strmcat (q, ", ", 511);
			strmcat (q, opts[i].command, 511);
		}

		userage ("XDEBUG", buffer);
		return;
	}

	while (args && *args)
	{
		this_arg = upper (next_arg (args, &args));
		if (*this_arg == '-')
			remove = 1, this_arg++;
		else if (*this_arg == '+')
			this_arg++;

		for (cnt = 0; opts[cnt].command; cnt++)
		{
			if (!strcmp (this_arg, opts[cnt].command))
			{
				if (remove)
					x_debug &= ~opts[cnt].flag;
				else
					x_debug |= opts[cnt].flag;
				break;
			}
		}
		if (!opts[cnt].command)
			say ("Unrecognized XDEBUG option '%s'", this_arg);
	}
}
