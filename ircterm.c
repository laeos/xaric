#ident "@(#)ircterm.c 1.10"
/*
 * ircterm.c - terminal-related functions for xaric
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1995 Matthew Green
 * Coypright 1996 EPIC Software Labs
 * 
 * Modified for Xaric by Rex G. Feany <laeos@laeos.net>
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
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif

#ifdef HAVE_SYS_TERMIOS_H
# include <sys/termios.h>
#else
# include <termios.h>
#endif

#include "irc.h"
#include "vars.h"
#include "screen.h" /* current_screen */
#include "ircterm.h"
#include "defaults.h"

/*
 * Function variables: each returns 1 if the function is not supported on the
 * current term type, otherwise they do their thing and return 0 
 */
int (*term_scroll) (int, int, int);	/* best scroll available */
int (*term_insert) (char);		/* best insert available */
int (*term_delete) (void);		/* best delete available */
int (*term_cursor_left) (void);		/* best left available */

/* Current terminal size */
int term_rows;
int term_cols;

/* The termcap variables.  Not all of these need be shared. */
const char *tc_CM, *tc_CE, *tc_CL, *tc_CR, *tc_NL, *tc_AL, *tc_DL, *tc_CS, 
	*tc_DC, *tc_IC, *tc_IM, *tc_EI, *tc_SO, *tc_SE, *tc_US, *tc_UE, 
	*tc_MD, *tc_ME, *tc_SF, *tc_SR, *tc_ND, *tc_LE, *tc_BL, *tc_BS;

/* These variables not shared! */
static int tc_CO, tc_LI, tc_SG;

/* Private things */

/* descriptor for the tty */
static int tty_des;

/* Original and Our terminal settings */
static struct termios oldb, newb;

/* true: ^Q/^S used for flow control */
static int use_flow_control = DEFAULT_USE_FLOW_CONTROL; 

/* To display, or not to display. Ah, the questions. */
static int term_echo_flag = 1;

/* Our copy of the termcap data */
static char termcap[1024];


/* Prototypes */
static int term_CS_scroll (int, int, int);
static int term_param_ALDL_scroll (int, int, int);
static int term_IC_insert (char);
static int term_IMEI_insert (char);
static int term_DC_delete (void);
static int term_BS_cursor_left (void);
static int term_LE_cursor_left (void);
static int term_null_function (void);

/* these are missing on some systems */
extern char *tgetstr ();
extern int tgetent ();
extern char *getenv ();
extern char *tgoto(const char *, int, int);


/* Setup the low level terminal disipline */
static void
setup_tty (void)
{
	tty_des = 0;

	tcgetattr (tty_des, &oldb);

	newb = oldb;

	/* set equ. of CBREAK, no ECHO */
	newb.c_lflag &= ~(ICANON | ECHO);	

	/* read() satified after 1 char */
	newb.c_cc[VMIN] = 1;	

	/* No timer */
	newb.c_cc[VTIME] = 0;	

	/* XON/XOFF? */
	if (! use_flow_control)
		newb.c_iflag &= ~IXON;	

#if !defined(_POSIX_VDISABLE)
#if defined(HAVE_FPATHCONF)
#define _POSIX_VDISABLE fpathconf(tty_des, _PC_VDISABLE)
#else
#define _POSIX_VDISABLE 0
#endif
#endif

	newb.c_cc[VQUIT] = _POSIX_VDISABLE;
#ifdef VDSUSP
	newb.c_cc[VDSUSP] = _POSIX_VDISABLE;
#endif
#ifdef VSUSP
	newb.c_cc[VSUSP] = _POSIX_VDISABLE;
#endif

	tcsetattr (tty_des, TCSADRAIN, &newb);
}

/* Actually do the flush */
static inline void
term_flush_int (void)
{
	fflush (current_screen ? current_screen->fpout : stdout);
}

static int 
term_null_function (void)
{
	return 1;
}

/* used if the terminal has the CS capability */
static int 
term_CS_scroll (int line1, int line2, int n)
{
	const char *thing;
	int i;

	if (n > 0)
		thing = tc_SF ? tc_SF : tc_NL;
	else if (n < 0) {
		if (tc_SR)
			thing = tc_SR;
		else
			return 1;
	}
	else
		return 0;

	/* shouldn't do this each time */
	tputs_x (tgoto (tc_CS, line2, line1));	
	if (n < 0) {
		term_move_cursor (0, line1);
		n = -n;
	}
	else
		term_move_cursor (0, line2);

	for (i = 0; i < n; i++)
		tputs_x (thing);

	/* shouldn't do this each time */
	tputs_x (tgoto (tc_CS, term_rows - 1, 0));
	return 0;
}

/* Uses the parameterized version of AL and DL */
static int 
term_param_ALDL_scroll (int line1, int line2, int n)
{
	if (n > 0) {
		term_move_cursor (0, line1);
		tputs_x (tgoto (tc_DL, n, n));
		term_move_cursor (0, line2 - n + 1);
		tputs_x (tgoto (tc_AL, n, n));
	} else if (n < 0) {
		n = -n;
		term_move_cursor (0, line2 - n + 1);
		tputs_x (tgoto (tc_DL, n, n));
		term_move_cursor (0, line1);
		tputs_x (tgoto (tc_AL, n, n));
	}
	return 0;
}

/* used for character inserts if the term has IC */
static int 
term_IC_insert (char c)
{
	tputs_x (tc_IC);
	term_putchar (c);
	return 0;
}

/* used for character inserts if the term has IM and EI */
static int 
term_IMEI_insert (char c)
{
	tputs_x (tc_IM);
	term_putchar (c);
	tputs_x (tc_EI);
	return 0;
}

/* used for character deletes if the term has DC */
static int 
term_DC_delete (void)
{
	tputs_x (tc_DC);
	return 0;
}

/* shouldn't you move on to something else? */
static int 
term_LE_cursor_left (void)
{
	tputs_x (tc_LE);
	return 0;
}

static int 
term_BS_cursor_left (void)
{
	char c = '\010';

	term_putchar_raw(c);
	return (0);
}

/**
 * term_init - does all terminal initialization
 *
 * Reads termcap info, sets the terminal to CBREAK, no ECHO mode.   Chooses the
 * best of the terminal attributes to use.
 **/
int 
term_init (void)
{
	char bp[2048], *term, *ptr;

	if ((term = getenv ("TERM")) == (char *) 0) {
		fprintf (stderr, "%s: No TERM variable found!\n", prog_name);
		return -1;
	}
	if (tgetent (bp, term) < 1) {
		fprintf (stderr, "%s: Current TERM variable for %s has no termcap entry.\n", prog_name, term);
		return -1;
	}

	if ((tc_CO = tgetnum ("co")) == -1)
		tc_CO = 80;
	if ((tc_LI = tgetnum ("li")) == -1)
		tc_LI = 24;
	ptr = termcap;

	/*  Get termcap capabilities */
	tc_SG = tgetnum ("sg");		/* Garbage chars gen by Standout */
	tc_CM = tgetstr ("cm", &ptr);	/* Cursor Movement */
	tc_CL = tgetstr ("cl", &ptr);	/* CLear screen, home cursor */
	tc_CR = tgetstr ("cr", &ptr);	/* Carriage Return */
	tc_NL = tgetstr ("nl", &ptr);	/* New Line */
	tc_CE = tgetstr ("ce", &ptr);	/* Clear to End of line */
	tc_ND = tgetstr ("nd", &ptr);	/* NonDestrucive space (cursor right) */
	tc_LE = tgetstr ("le", &ptr);	/* move cursor to LEft */
	tc_BS = tgetstr ("bs", &ptr);	/* move cursor with Backspace */
	tc_SF = tgetstr ("sf", &ptr);	/* Scroll Forward (up) */
	tc_SR = tgetstr ("sr", &ptr);	/* Scroll Reverse (down) */
	tc_CS = tgetstr ("cs", &ptr);	/* Change Scrolling region */
	tc_AL = tgetstr ("AL", &ptr);	/* Add blank Lines */
	tc_DL = tgetstr ("DL", &ptr);	/* Delete Lines */
	tc_IC = tgetstr ("ic", &ptr);	/* Insert Character */
	tc_IM = tgetstr ("im", &ptr);	/* enter Insert Mode */
	tc_EI = tgetstr ("ei", &ptr);	/* Exit Insert mode */
	tc_DC = tgetstr ("dc", &ptr);	/* Delete Character */
	tc_SO = tgetstr ("so", &ptr);	/* StandOut mode */
	tc_SE = tgetstr ("se", &ptr);	/* Standout mode End */
	tc_US = tgetstr ("us", &ptr);	/* UnderScore mode */
	tc_UE = tgetstr ("ue", &ptr);	/* Underscore mode End */
	tc_MD = tgetstr ("md", &ptr);	/* bold mode (?) */
	tc_ME = tgetstr ("me", &ptr);	/* bold mode End */
	tc_BL = tgetstr ("bl", &ptr);	/* BeLl */

	/* why are these done again? */
	if (! tc_AL)
		tc_AL = tgetstr ("al", &ptr);
	if (! tc_DL)
		tc_DL = tgetstr ("dl", &ptr);
	if (! tc_CR)
		tc_CR = "\r";
	if (! tc_NL)
		tc_NL = "\n";
	if (! tc_BL)
		tc_BL = "\007";
	if (! tc_LE)
		tc_LE = "\010";

	if (!tc_SO || !tc_SE) {
		tc_SO = empty_str;
		tc_SE = empty_str;
	}
	if (!tc_US || !tc_UE) {
		tc_US = empty_str;
		tc_UE = empty_str;
	}
	if (!tc_MD || !tc_ME) {
		tc_MD = empty_str;
		tc_ME = empty_str;
	}

	if (! (tc_CM && tc_CL && tc_CE && tc_ND && (tc_LE || tc_BS) && (tc_CS || (tc_AL && tc_DL)))) {
		fputc('\n', stderr);
		fprintf (stderr, "%s: Your terminal cannot run Xaric in full screen mode.\n", prog_name);
		fprintf (stderr, "%s: The following features are missing from your TERM setting.\n", prog_name);

		if (!tc_CM)
			fprintf (stderr, "\tCursor Movement\n");
		if (!tc_CL)
			fprintf (stderr, "\tClear Screen\n");
		if (!tc_CE)
			fprintf (stderr, "\tClear to end-of-line\n");
		if (!tc_ND)
			fprintf (stderr, "\tCursor right\n");
		if (! (tc_LE || tc_BS))
			fprintf (stderr, "\tCursor left\n");
		if (!tc_CS && !(tc_AL && tc_DL))
			fprintf (stderr, "\tScrolling\n");

		fprintf (stderr, "%s: Try using VT100 emulation or better.\n", prog_name);
		return -1;
	}


	term_cursor_left = (tc_LE) ? term_LE_cursor_left : term_BS_cursor_left;
	term_scroll = (tc_CS) ? term_CS_scroll : term_param_ALDL_scroll;
	term_delete = (tc_DC) ? term_DC_delete : term_null_function;

	term_insert = (tc_IC) ? term_IC_insert
		: (tc_IM && tc_EI) ? term_IMEI_insert
		: (int (*)(char)) term_null_function;

	setup_tty();

	return 0;
}

/**
 * term_resize - check to see if physical screen size has changed.
 *
 * Gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned. 
 **/
int 
term_resize (void)
{
	static int old_li = -1, old_co = -1;

#if defined (TIOCGWINSZ)
	{
		struct winsize window;

		if (ioctl (tty_des, TIOCGWINSZ, &window) < 0) {
			term_cols = tc_CO;
			term_rows = tc_LI;
		} else {
			if ((term_rows = window.ws_row) == 0)
				term_rows = tc_LI;
			if ((term_cols = window.ws_col) == 0)
				term_cols = tc_CO;
		}
	}
#else
	{
		term_rows = tc_LI;
		term_cols = tc_CO;
	}
#endif

	term_cols--;
	if ((old_li != term_rows) || (old_co != term_cols)) {
		old_li = term_rows;
		old_co = term_cols;
		return 1;
	}
	return 0;
}

/**
 * term_reset - restore terminal attributes
 *
 * Sets terminal attributes back to what they were before the
 * program started 
 **/
void 
term_reset (void)
{
	tcsetattr (tty_des, TCSADRAIN, &oldb);

	if (tc_CS)
		tputs_x (tgoto (tc_CS, term_rows - 1, 0));

	term_move_cursor (0, term_rows - 1);
	term_flush_int ();
}

/**
 * term_continue - restore terminal
 *
 * Sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.
 **/
void
term_continue (void)
{
	need_redraw = 1;
	tcsetattr (tty_des, TCSADRAIN, &newb);
}

/**
 * term_pause - sets terminal back to pre-program days, then SIGSTOPs itself. 
 **/
void 
term_pause (void)
{
	term_reset ();
	kill (getpid (), SIGSTOP);
}

/**
 * term_echo - toggle display of characters
 * @flag: ON, OFF, TOGGLE of terminal echo
 *
 * If OFF, echo is turned off (all characters appear as blanks), if
 * ON, all is normal. The function returns the old value of the
 * term_echo_flag 
 **/
int 
term_echo (int flag)
{
	int echo;

	if (flag == TOGGLE)
		flag = !term_echo_flag;

	echo = term_echo_flag;
	term_echo_flag = flag;

	return echo;
}

/**
 * term_putchar - puts a character to the screen
 * @c: what to display, of course!
 *
 *
 * Displays control characters as inverse video uppercase letters.  
 * NOTE: Dont use this to display termcap control sequences!  
 *       It might not work! 
 **/
void 
term_putchar (unsigned char c)
{
	if (! term_echo_flag) {
		term_putchar_raw(' ');
		return;
	}
		
	/* Sheer, raving paranoia */
	if (!(newb.c_cflag & CS8) && (c & 0x80))
		c &= ~0x80;

	if (get_bool_var (EIGHT_BIT_CHARACTERS_VAR) == 0)
		if (c & 128)
			c &= ~128;

	/* 
	 * The only control character in ascii sequences
	 * is 27, which is the escape -- all the rest of
	 * them are printable (i think).  So we should
	 * print all the rest of the control seqs as
	 * reverse like normal (idea from Genesis K.)
	 *
	 * Why do i know im going to regret this?
	 */
	if ((c != 27) || !get_bool_var (DISPLAY_ANSI_VAR)) {
		if (c < 32) {
			term_standout_on ();
			c = (c & 127) | 64;
			term_putchar_raw(c);
			term_standout_off ();
		} else if (c == '\177') {
			term_standout_on ();
			term_putchar_raw('?');
			term_standout_off ();
		}
		else
			term_putchar_raw(c);
	} else
		term_putchar_raw(c);

}

/**
 * term_puts - uses term_putchar to print text 
 **/
int 
term_puts (char *str, int len)
{
	int i;

	for (i = 0; *str && (i < len); str++, i++)
		term_putchar (*str);
	return i;
}

/**
 * term_raw_putchar - put raw character to terminal (contrast term_putchar)
 **/
int
term_putchar_raw (int c)
{
	return fputc (c, (current_screen ? current_screen->fpout : stdout));
}

/**
 * term_flush - flush pending data to terminal
 *
 * Forces data to be written to the terminal device.
 **/
void 
term_flush (void)
{
	term_flush_int();
}

/**
 * term_beep - ring the terminal bell
 *
 * Checks the user variable BEEP befor actually beeping.
 **/
void 
term_beep (void)
{
	if (get_bool_var (BEEP_VAR)) {
		tputs_x (tc_BL);
		term_flush_int();
	}
}

/**
 * term_set_flow_control - turn on, off, or toggle flow control
 * @value: ON, OFF, TOGGLE.
 **/
void
term_set_flow_control (int value)
{
	if (value == TOGGLE)
		value = !use_flow_control;

	if (value == ON) {
		use_flow_control = 1;
		newb.c_iflag |= IXON;
	} else {
		use_flow_control = 0;
		newb.c_iflag &= ~IXON;
	}
	tcsetattr(tty_des, TCSADRAIN, &newb);
}

/**
 * term_eight_bit - can we display eight bit characters?
 **/
int 
term_eight_bit (void)
{
	return (((oldb.c_cflag) & CSIZE) == CS8) ? 1 : 0;
}

/**
 * term_set_eight_bit - set the character size
 * @value: ON, OFF, TOGGLE.
 **/
void 
term_set_eight_bit (int value)
{
	if (value == TOGGLE)
		value = !term_eight_bit();

	if (value == ON) {
		newb.c_cflag |= CS8;
		newb.c_iflag &= ~ISTRIP;
	} else {
		newb.c_cflag &= ~CS8;
		newb.c_iflag |= ISTRIP;
	}
	tcsetattr (tty_des, TCSADRAIN, &newb);
}
