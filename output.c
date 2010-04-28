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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "ircaux.h"
#include "ircterm.h"
#include "fset.h"
#include "misc.h"
#include "output.h"
#include "hook.h"
#include "log.h"
#include "input.h"
#include "screen.h"
#include "window.h"

/* make this buffer *much* bigger than needed */
#define LARGE_BIG_BUFFER_SIZE BIG_BUFFER_SIZE

static char putbuf[LARGE_BIG_BUFFER_SIZE + 1];

/* unflash: sends a ^[c to the screen */
/* Must be defined to be useful, cause some vt100s really *do* reset when
   sent this command. >;-) */

/* functions which switch the character set on the console */
/* ibmpc is not available on the xterm */

void charset_ibmpc(void)
{
    fwrite("\033(U", 3, 1, stdout);	/* switch to IBM code page 437 */
}

void charset_lat1(void)
{
    fwrite("\033(B", 3, 1, stdout);	/* switch to Latin-1 (ISO 8859-1) */
}

/* Now that you can send ansi sequences, this is much less inportant.. */
static void unflash(void)
{
#ifdef HARD_UNFLASH
    fwrite("\033c", 5, 1, stdout);	/* hard reset */
#else
    fwrite("\033)0", 6, 1, stdout);	/* soft reset */
#endif
#ifdef LATIN1
    charset_lat1();
#else
    charset_ibmpc();
#endif
}

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
void refresh_screen(unsigned char dumb, char *dumber)
{
    term_clear_screen();
    unflash();
    if (term_resize())
	recalculate_windows();
    else
	redraw_all_windows();
    need_redraw = 0;
    update_all_windows();
    update_input(UPDATE_ALL);
}

/* init_windows:  */
int init_screen(void)
{
    if (term_init())
	return -1;
    term_clear_screen();
    term_resize();
    new_window();
    recalculate_windows();
    update_all_windows();
    init_input();
    term_move_cursor(0, 0);
    return 0;
}

void put_echo(char *str)
{
    add_to_log(irclog_fp, 0, str);
    add_to_screen(str);
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
void put_it(const char *format, ...)
{
    if (window_display && format) {
	va_list args;

	putbuf[0] = '\0';

	va_start(args, format);
	vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
	va_end(args);

	if (*putbuf)
	    put_echo(putbuf);
    }
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void say(const char *format, ...)
{
    int len = 0;

    if (window_display && format) {
	va_list args;

	len = snprintf(putbuf, LARGE_BIG_BUFFER_SIZE, "%s ", line_thing);

	va_start(args, format);
	vsnprintf(&(putbuf[len]), LARGE_BIG_BUFFER_SIZE - len, format, args);
	va_end(args);

	put_echo(putbuf);
    }
}

void bitchsay(const char *format, ...)
{
    int len;

    if (window_display && format) {
	va_list args;

	len = snprintf(putbuf, LARGE_BIG_BUFFER_SIZE, "%s \002Xaric\002: ", line_thing);

	va_start(args, format);
	vsnprintf(&(putbuf[len]), LARGE_BIG_BUFFER_SIZE - len, format, args);
	va_end(args);

	put_echo(putbuf);
    }
}

void yell(const char *format, ...)
{
    if (format) {
	va_list args;

	*putbuf = 0;
	va_start(args, format);
	vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
	va_end(args);

	if (*putbuf)
	    do_hook(YELL_LIST, "%s", putbuf);
	put_echo(putbuf);
    }
}

void log_put_it(const char *topic, const char *format, ...)
{
    if (format) {
	va_list args;

	va_start(args, format);
	vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
	va_end(args);

	message_from(NULL, LOG_CURRENT);
	if (window_display)
	    put_echo(putbuf);
	message_from(NULL, LOG_CRAP);
    }
}

/**
 * put_fmt_str - print a formatted string
 * @fmtstr: format string to use
 * @arg: printf-like format for arguments
 *
 * This should only be used for init.c stuff.
 * Most formats should be in the format table.
 **/
void put_fmt_str(const char *fmtstr, const char *arg, ...)
{
    char str[BIG_BUFFER_SIZE];
    va_list ma;

    assert(fmtstr);

    str[0] = '\0';
    if (arg) {
	va_start(ma, arg);
	vsnprintf(str, BIG_BUFFER_SIZE, arg, ma);
	va_end(ma);
	str[BIG_BUFFER_SIZE - 1] = '\0';
    }
    put_it("%s", convert_output_format(fmtstr, "%s", str));
}

/** 
 * put_fmt - print a formatted string.
 * @fmt: format identifier
 * @arg: printf-like format for arguments
 *
 * Converts the format to a printable string, and then prints it!
 **/
void put_fmt(xformat fmt, const char *arg, ...)
{
    const char *fmts = get_format(fmt);

    /* 
     * XXX This is really horrable. Sometime, convert_output_format
     * needs to be replaced with a better interface. 
     */

    if (fmts) {
	char str[BIG_BUFFER_SIZE];
	va_list ma;

	str[0] = '\0';
	if (arg) {
	    va_start(ma, arg);
	    vsnprintf(str, BIG_BUFFER_SIZE, arg, ma);
	    va_end(ma);
	    str[BIG_BUFFER_SIZE - 1] = '\0';
	}
	put_it("%s", convert_output_format(fmts, "%s", str));
    }
}
