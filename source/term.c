/*
 * term.c -- termios and termcap handlers
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1995 Matthew Green
 * Coypright 1996 EPIC Software Labs
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "vars.h"
#include "ircterm.h"
#include "window.h"
#include "screen.h"
#include "output.h"


#ifdef HAVE_SYS_TERMIOS_H
#include <sys/termios.h>
#else
#include <termios.h>
#endif

#include <sys/ioctl.h>

static int tty_des;		/* descriptor for the tty */
static struct termios oldb, newb;

extern char *tgetstr ();
extern int tgetent ();
extern char *getenv ();

static int term_CE_clear_to_eol (void);
static int term_CS_scroll (int, int, int);
static int term_param_ALDL_scroll (int, int, int);
static int term_IC_insert (char);
static int term_IMEI_insert (char);
static int term_DC_delete (void);
static int term_BS_cursor_left (void);
static int term_LE_cursor_left (void);
static int term_ND_cursor_right (void);
static int term_null_function (void);

/*
 * Function variables: each returns 1 if the function is not supported on the
 * current term type, otherwise they do their thing and return 0 
 */
int (*term_scroll) (int, int, int);	/* this is set to the best scroll available */
int (*term_insert) (char);	/* this is set to the best insert available */
int (*term_delete) (void);	/* this is set to the best delete available */
int (*term_cursor_left) (void);	/* this is set to the best left available */
int (*term_cursor_right) (void);	/* this is set to the best right available */
int (*term_clear_to_eol) (void);	/* this is set... figure it out */

/* The termcap variables */
char *CM, *CE, *CL, *CR, *NL, *AL, *DL, *CS, *DC, *IC, *IM, *EI, *SO, *SE,
 *US, *UE, *MD, *ME, *SF, *SR, *ND, *LE, *BL, *BS;
int CO = 79, LI = 24, SG;

/*
 * term_reset_flag: set to true whenever the terminal is reset, thus letter
 * the calling program work out what to do 
 */
int need_redraw = 0;
static int term_echo_flag = 1;
static int li;
static int co;
static char termcap[1024];


/*
 * term_echo: if 0, echo is turned off (all characters appear as blanks), if
 * non-zero, all is normal.  The function returns the old value of the
 * term_echo_flag 
 */
int 
term_echo (int flag)
{
	int echo;

	echo = term_echo_flag;
	term_echo_flag = flag;
	return (echo);
}

/*
 * term_putchar: puts a character to the screen, and displays control
 * characters as inverse video uppercase letters.  NOTE:  Dont use this to
 * display termcap control sequences!  It won't work! 
 *
 * Um... well, it will work if DISPLAY_ANSI_VAR is set to on... (hop)
 */
void 
term_putchar (unsigned char c)
{
	if (term_echo_flag)
	{
		/* Sheer, raving paranoia */
		if (!(newb.c_cflag & CS8) && (c & 0x80))
			c &= ~0x80;

		if (get_int_var (EIGHT_BIT_CHARACTERS_VAR) == 0)
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
		if ((c != 27) || !get_int_var (DISPLAY_ANSI_VAR))
		{
			if (c < 32)
			{
				term_standout_on ();
				c = (c & 127) | 64;
				fputc (c, (current_screen ? current_screen->fpout : stdout));
				term_standout_off ();
			}
			else if (c == '\177')
			{
				term_standout_on ();
				c = '?';
				fputc (c, (current_screen ? current_screen->fpout : stdout));
				term_standout_off ();
			}
			else
				fputc ((int) c, (current_screen ? current_screen->fpout : stdout));
		}
		else
			fputc ((int) c, (current_screen ? current_screen->fpout : stdout));
	}
	else
	{
		c = ' ';
		fputc ((int) c, (current_screen ? current_screen->fpout : stdout));
	}
}

/* term_puts: uses term_putchar to print text */
int 
term_puts (char *str, int len)
{
	int i;

	for (i = 0; *str && (i < len); str++, i++)
		term_putchar (*str);
	return (i);
}

/* putchar_x: the putchar function used by tputs */
int 
putchar_x (int c)
{
	return fputc (c, (current_screen ? current_screen->fpout : stdout));
}

void 
term_flush (void)
{
	fflush ((current_screen ? current_screen->fpout : stdout));
}

/*
 * term_reset: sets terminal attributed back to what they were before the
 * program started 
 */
void 
term_reset (void)
{
	tcsetattr (tty_des, TCSADRAIN, &oldb);

	if (CS)
		tputs_x (tgoto (CS, LI - 1, 0));
	term_move_cursor (0, LI - 1);
	term_flush ();
}

/*
 * term_cont: sets the terminal back to IRCII stuff when it is restarted
 * after a SIGSTOP.  Somewhere, this must be used in a signal() call 
 */
RETSIGTYPE 
term_cont (int unused)
{
	need_redraw = 1;
	tcsetattr (tty_des, TCSADRAIN, &newb);
}

/*
 * term_pause: sets terminal back to pre-program days, then SIGSTOPs itself. 
 */
extern void 
term_pause (char unused, char *not_used)
{
	term_reset ();
	kill (getpid (), SIGSTOP);
}



/*
 * term_init: does all terminal initialization... reads termcap info, sets
 * the terminal to CBREAK, no ECHO mode.   Chooses the best of the terminal
 * attributes to use ..  for the version of this function that is called for
 * wserv, we set the termial to RAW, no ECHO, so that all the signals are
 * ignored.. fixes quite a few problems...  -phone, jan 1993..
 */
int 
term_init (void)
{
	char bp[2048], *term, *ptr;

	if ((term = getenv ("TERM")) == (char *) 0)
	{
		fprintf (stderr, "irc: No TERM variable found!\n");
		return -1;
	}
	if (tgetent (bp, term) < 1)
	{
		fprintf (stderr, "irc: Current TERM variable for %s has no termcap entry.\n", term);
		return -1;
	}

	if ((co = tgetnum ("co")) == -1)
		co = 80;
	if ((li = tgetnum ("li")) == -1)
		li = 24;
	ptr = termcap;

	/*      
	 * Get termcap capabilities
	 */
	SG = tgetnum ("sg");	/* Garbage chars gen by Standout */
	CM = tgetstr ("cm", &ptr);	/* Cursor Movement */
	CL = tgetstr ("cl", &ptr);	/* CLear screen, home cursor */
	CR = tgetstr ("cr", &ptr);	/* Carriage Return */
	NL = tgetstr ("nl", &ptr);	/* New Line */
	CE = tgetstr ("ce", &ptr);	/* Clear to End of line */
	ND = tgetstr ("nd", &ptr);	/* NonDestrucive space (cursor right) */
	LE = tgetstr ("le", &ptr);	/* move cursor to LEft */
	BS = tgetstr ("bs", &ptr);	/* move cursor with Backspace */
	SF = tgetstr ("sf", &ptr);	/* Scroll Forward (up) */
	SR = tgetstr ("sr", &ptr);	/* Scroll Reverse (down) */
	CS = tgetstr ("cs", &ptr);	/* Change Scrolling region */
	AL = tgetstr ("AL", &ptr);	/* Add blank Lines */
	DL = tgetstr ("DL", &ptr);	/* Delete Lines */
	IC = tgetstr ("ic", &ptr);	/* Insert Character */
	IM = tgetstr ("im", &ptr);	/* enter Insert Mode */
	EI = tgetstr ("ei", &ptr);	/* Exit Insert mode */
	DC = tgetstr ("dc", &ptr);	/* Delete Character */
	SO = tgetstr ("so", &ptr);	/* StandOut mode */
	SE = tgetstr ("se", &ptr);	/* Standout mode End */
	US = tgetstr ("us", &ptr);	/* UnderScore mode */
	UE = tgetstr ("ue", &ptr);	/* Underscore mode End */
	MD = tgetstr ("md", &ptr);	/* bold mode (?) */
	ME = tgetstr ("me", &ptr);	/* bold mode End */
	BL = tgetstr ("bl", &ptr);	/* BeLl */

	if (!AL)
	{
		AL = tgetstr ("al", &ptr);
	}
	if (!DL)
	{
		DL = tgetstr ("dl", &ptr);
	}
	if (!CR)
	{
		CR = "\r";
	}
	if (!NL)
	{
		NL = "\n";
	}
	if (!BL)
	{
		BL = "\007";
	}
	if (!SO || !SE)
	{
		SO = empty_string;
		SE = empty_string;
	}
	if (!US || !UE)
	{
		US = empty_string;
		UE = empty_string;
	}
	if (!MD || !ME)
	{
		MD = empty_string;
		ME = empty_string;
	}


	if (!CM || !CL || !CE || !ND || (!LE && !BS) || (!CS && !(AL && DL)))
	{
		fprintf (stderr, "\nYour terminal cannot run IRC II in full screen mode.\n");
		fprintf (stderr, "The following features are missing from your TERM setting.\n");

		if (!CM)
			fprintf (stderr, "\tCursor Movement\n");
		if (!CL)
			fprintf (stderr, "\tClear Screen\n");
		if (!CE)
			fprintf (stderr, "\tClear to end-of-line\n");
		if (!ND)
			fprintf (stderr, "\tCursor right\n");
		if (!LE && !BS)
			fprintf (stderr, "\tCursor left\n");
		if (!CS && !(AL && DL))
			fprintf (stderr, "\tScrolling\n");

		fprintf (stderr, "Try using VT100 emulation or better.\n");
		return -1;
	}

	if (!LE)
		LE = "\010";

	term_clear_to_eol = term_CE_clear_to_eol;
	term_cursor_right = term_ND_cursor_right;
	term_cursor_left = (LE) ? term_LE_cursor_left
		: term_BS_cursor_left;
	term_scroll = (CS) ? term_CS_scroll
		: term_param_ALDL_scroll;
	term_delete = (DC) ? term_DC_delete
		: term_null_function;
	term_insert = (IC) ? term_IC_insert
		: (IM && EI) ? term_IMEI_insert
		: (int (*)(char)) term_null_function;


	/* Set up the terminal discipline */
/*      if ((tty_des = open("/dev/tty", O_RDWR, 0)) == -1) */
	tty_des = 0;

	tcgetattr (tty_des, &oldb);

	newb = oldb;
	newb.c_lflag &= ~(ICANON | ECHO);	/* set equ. of CBREAK, no ECHO */
	newb.c_cc[VMIN] = 1;	/* read() satified after 1 char */
	newb.c_cc[VTIME] = 0;	/* No timer */

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

	if (!use_flow_control)
		newb.c_iflag &= ~IXON;	/* No XON/XOFF */

	tcsetattr (tty_des, TCSADRAIN, &newb);
	return 0;
}


/*
 * term_resize: gets the terminal height and width.  Trys to get the info
 * from the tty driver about size, if it can't... uses the termcap values. If
 * the terminal size has changed since last time term_resize() has been
 * called, 1 is returned.  If it is unchanged, 0 is returned. 
 */
int 
term_resize (void)
{
	static int old_li = -1, old_co = -1;

#if defined (TIOCGWINSZ)
	{
		struct winsize window;

		if (ioctl (tty_des, TIOCGWINSZ, &window) < 0)
		{
			LI = li;
			CO = co;
		}
		else
		{
			if ((LI = window.ws_row) == 0)
				LI = li;
			if ((CO = window.ws_col) == 0)
				CO = co;
		}
	}
#else
	{
		LI = li;
		CO = co;
	}
#endif

	CO--;
	if ((old_li != LI) || (old_co != CO))
	{
		old_li = LI;
		old_co = CO;
		return (1);
	}
	return (0);
}


static int 
term_null_function (void)
{
	return 1;
}

/* term_CE_clear_to_eol(): the clear to eol function, right? */
static int 
term_CE_clear_to_eol (void)
{
	tputs_x (CE);
	return (0);
}

/*
 * term_CS_scroll: should be used if the terminal has the CS capability by
 * setting term_scroll equal to it 
 */
static int 
term_CS_scroll (int line1, int line2, int n)
{
	int i;
	char *thing;

	if (n > 0)
		thing = SF ? SF : NL;
	else if (n < 0)
	{
		if (SR)
			thing = SR;
		else
			return 1;
	}
	else
		return 0;

	tputs_x (tgoto (CS, line2, line1));	/* shouldn't do this each time */
	if (n < 0)
	{
		term_move_cursor (0, line1);
		n = -n;
	}
	else
		term_move_cursor (0, line2);
	for (i = 0; i < n; i++)
		tputs_x (thing);
	tputs_x (tgoto (CS, LI - 1, 0));	/* shouldn't do this each time */
	return (0);
}

/*
 * term_param_ALDL_scroll: Uses the parameterized version of AL and DL 
 */
static int 
term_param_ALDL_scroll (int line1, int line2, int n)
{
	if (n > 0)
	{
		term_move_cursor (0, line1);
		tputs_x (tgoto (DL, n, n));
		term_move_cursor (0, line2 - n + 1);
		tputs_x (tgoto (AL, n, n));
	}
	else if (n < 0)
	{
		n = -n;
		term_move_cursor (0, line2 - n + 1);
		tputs_x (tgoto (DL, n, n));
		term_move_cursor (0, line1);
		tputs_x (tgoto (AL, n, n));
	}
	return (0);
}

/*
 * term_IC_insert: should be used for character inserts if the term has IC by
 * setting term_insert to it. 
 */
static int 
term_IC_insert (char c)
{
	tputs_x (IC);
	term_putchar (c);
	return (0);
}

/*
 * term_IMEI_insert: should be used for character inserts if the term has IM
 * and EI by setting term_insert to it 
 */
static int 
term_IMEI_insert (char c)
{
	tputs_x (IM);
	term_putchar (c);
	tputs_x (EI);
	return (0);
}

/*
 * term_DC_delete: should be used for character deletes if the term has DC by
 * setting term_delete to it 
 */
static int 
term_DC_delete (void)
{
	tputs_x (DC);
	return (0);
}

/* term_ND_cursor_right: got it yet? */
static int 
term_ND_cursor_right (void)
{
	tputs_x (ND);
	return (0);
}

/* term_LE_cursor_left:  shouldn't you move on to something else? */
static int 
term_LE_cursor_left (void)
{
	tputs_x (LE);
	return (0);
}

static int 
term_BS_cursor_left (void)
{
	char c = '\010';

	fputc (c, (current_screen ? current_screen->fpout : stdout));
	return (0);
}

extern void 
copy_window_size (int *lines, int *columns)
{
	*lines = LI;
	*columns = CO;
}

extern int 
term_eight_bit (void)
{
	return (((oldb.c_cflag) & CSIZE) == CS8) ? 1 : 0;
}

extern void 
term_beep (void)
{
	if (get_int_var (BEEP_VAR))
	{
		tputs_x (BL);
		fflush (current_screen ? current_screen->fpout : stdout);
	}
}

extern void 
set_term_eight_bit (int value)
{
	if (value == ON)
	{
		newb.c_cflag |= CS8;
		newb.c_iflag &= ~ISTRIP;
	}
	else
	{
		newb.c_cflag &= ~CS8;
		newb.c_iflag |= ISTRIP;
	}
	tcsetattr (tty_des, TCSADRAIN, &newb);
}
