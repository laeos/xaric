#ident "$Id$"
/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"
#include <sys/stat.h>

#include <stdarg.h>

#include "output.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "ircterm.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "hook.h"
#include "ctcp.h"
#include "log.h"
#include "misc.h"

int in_help = 0;

/* make this buffer *much* bigger than needed */

#define LARGE_BIG_BUFFER_SIZE BIG_BUFFER_SIZE

static char putbuf[LARGE_BIG_BUFFER_SIZE + 1];

/* unflash: sends a ^[c to the screen */
/* Must be defined to be useful, cause some vt100s really *do* reset when
   sent this command. >;-) */

/* functions which switch the character set on the console */
/* ibmpc is not available on the xterm */

void 
charset_ibmpc (void)
{
	fwrite ("\033(U", 3, 1, stdout);	/* switch to IBM code page 437 */
}

void 
charset_lat1 (void)
{
	fwrite ("\033(B", 3, 1, stdout);	/* switch to Latin-1 (ISO 8859-1) */
}

/* currently not used. */

void 
charset_graf (void)
{
	fwrite ("\033(0", 3, 1, stdout);	/* switch to DEC VT100 pseudographics */
}



/* Now that you can send ansi sequences, this is much less inportant.. */
void 
unflash (void)
{
#ifdef HARD_UNFLASH
	fwrite ("\033c", 5, 1, stdout);		/* hard reset */
#else
	fwrite ("\033)0", 6, 1, stdout);	/* soft reset */
#endif
#ifdef LATIN1
	charset_lat1 ();
#else
	charset_ibmpc ();
#endif
}

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
void 
refresh_screen (unsigned char dumb, char *dumber)
{
	extern int need_redraw;
	term_clear_screen ();
	unflash ();
	if (term_resize ())
		recalculate_windows ();
	else
		redraw_all_windows ();
	need_redraw = 0;
	update_all_windows ();
	update_input (UPDATE_ALL);
}

/* init_windows:  */
int 
init_screen (void)
{
	if (term_init ())
		return -1;
	term_clear_screen ();
	term_resize ();
	new_window ();
	recalculate_windows ();
	update_all_windows ();
	init_input ();
	term_move_cursor (0, 0);
	return 0;
}

void 
put_echo (char *str)
{
	add_to_log (irclog_fp, 0, str);
	add_to_screen (str);
}


/*
 * put_it: the irc display routine.  Use this routine to display anything to
 * the main irc window.  It handles sending text to the display or stdout as
 * needed, add stuff to the lastlog and log file, etc.  Things NOT to do:
 * Dont send any text that contains \n, very unpredictable.  Tabs will also
 * screw things up.  The calling routing is responsible for not overwriting
 * the 1K buffer allocated.  
 *
 * For Ultrix machines, you can't call put_it() with floating point arguements.
 * It just doesn't work.  - phone, jan 1993.
 */
void 
put_it (const char *format,...)
{
	if (window_display && format)
	{
		va_list args;
		memset (putbuf, 0, 200);
		va_start (args, format);
		vsnprintf (putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end (args);
		if (*putbuf)
			put_echo (putbuf);
	}
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void 
say (const char *format,...)
{
	int len = 0;

	if (window_display && format)
	{
		va_list args;
		va_start (args, format);

		len = strlen (line_thing);

		vsnprintf (&(putbuf[len + 1]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end (args);
		strcpy (putbuf, line_thing);
		putbuf[len] = ' ';

		if (strip_ansi_in_echo)
		{
			register char *ptr;
			for (ptr = putbuf + 4; *ptr; ptr++)
				if (*ptr < 31 && *ptr > 13)
					if (*ptr != 15 && *ptr != 22)
						*ptr = (*ptr & 127) | 64;
		}
		put_echo (putbuf);
	}
}

void 
bitchsay (const char *format,...)
{
	int len;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		sprintf (putbuf, "%s \002%s\002: ", line_thing, version);
		len = strlen (putbuf);
		vsnprintf (&(putbuf[len]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end (args);
		if (strip_ansi_in_echo)
		{
			register char *ptr;
			for (ptr = putbuf + len; *ptr; ptr++)
				if (*ptr < 31 && *ptr > 13)
					if (*ptr != 15 && *ptr != 22)
						*ptr = (*ptr & 127) | 64;
		}
		put_echo (putbuf);
	}
}

void 
yell (const char *format,...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		*putbuf = 0;
		vsnprintf (putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end (args);
		if (*putbuf)
			do_hook (YELL_LIST, "%s", putbuf);
		put_echo (putbuf);
	}
}

void 
log_put_it (const char *topic, const char *format,...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf (putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end (args);

		in_help = 1;
		message_from (NULL, LOG_CURRENT);
		if (window_display)
			put_echo (putbuf);
		message_from (NULL, LOG_CRAP);
		in_help = 0;
	}
}


