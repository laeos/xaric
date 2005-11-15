#ident "@(#)signals.c 1.6"
/*
 * signals.c: handle signals for xaric!
 * written by matthew green, 1993.
 *
 * i stole bits of this from w. richard stevens' `advanced programming
 * in the unix environment' -mrg
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "vars.h"
#include "output.h"
#include "ircaux.h"
#include "irc_std.h"
#include "ircterm.h"

/* Make sure X is a valid signal handler function pointer */
#define VALID_SIGHAND(x)	((x) != SIG_DFL && \
				 (x) != SIG_IGN && \
				 (x) != SIG_ERR && \
				 (x) != NULL)

/* Keep track of how many times ctrl-c was hit. */
volatile int cntl_c_hit = 0;

/* And if we get a sigchld */
volatile int got_sigchild = 0;

/* Make sure we call the librep signal handlers. */
/* Not inside an ifdef because that makes it too complicated. */
static sigfunc *old_segv __attribute__ ((unused));
static sigfunc *old_sigint __attribute__ ((unused));
static sigfunc *old_sigchild __attribute__ ((unused));

/* ... but this is, we don't have to call them if we arnt using librep. */
#ifdef HAVE_REP
# define OLD_HANDLER(h, sig)	(*(h)) (sig)	/* Call the old sig handler */
#else
# define OLD_HANDLER(h, sig)
#endif

/*
 * Signal handlers for Xaric
 */

/* sig_refresh_screen: the signal-callable version of refresh_screen */
static RETSIGTYPE sig_refresh_screen(int unused)
{
    refresh_screen(0, NULL);
}

/* sig_continue: reset term paramaters and redraw screen */
static RETSIGTYPE sig_continue(int unused)
{
    term_continue();
}

/* sig_quit_irc: cleans up and leaves */
static RETSIGTYPE sig_quit_irc(int unused)
{
    irc_exit(get_string_var(SIGNOFF_REASON_VAR), NULL);
}

/* sig_nothing: don't do anything! */
static RETSIGTYPE sig_nothing(int sig)
{
    /* nothing to do! */
}

/* sig_coredump: handle segfaults in a nice way */
static RETSIGTYPE sig_coredump(int sig)
{
    static volatile int segv_recurse = 0;

    const char message[] =
	"\n\n\r\nXaric has been terminated by a signal\n\n"
	"If this is a bug in Xaric, please send an email to\n"
	"<bugs@xaric.org> with as much detail as possible about\n"
	"what you did to trigger the bug. Make sure to include\n"
	"the version of Xaric you are using, and the type of system\n" "in your report.\n\n\n\r";

    if (segv_recurse++)
	_exit(1);

    write(STDERR_FILENO, message, sizeof(message));

    OLD_HANDLER(old_segv, sig);

    irc_exit("Wow! A bug?", NULL);
}

/* sig_int: if somehow everything else freezes up, hitting ctrl-c
 *          five times should kill the program.  */
static RETSIGTYPE sig_int(int sig)
{
    OLD_HANDLER(old_sigint, sig);

    if (cntl_c_hit++ >= 4)
	irc_exit("User abort with 5 Ctrl-C's", NULL);
    else if (cntl_c_hit > 1)
	kill(getpid(), SIGALRM);
}

/* sig_child: so we can reap our dead children */
static RETSIGTYPE sig_child(int sig)
{
    got_sigchild++;

    OLD_HANDLER(old_sigchild, sig);
}

/**
 * my_signal - set up a signal handler using sigaction
 * @sig_no: The signal we wish to modify
 * @sig_handler: function to use, or SIG_IGN/etc.
 * Returns: old signal handler.
 *
 * Install a signal handler with sigaction. Returns
 * the old signal handler. If SA_RESTART is supported,
 * it is turned on for every signal but SIGALRM and SIGINT.
 **/
sigfunc *my_signal(int sig_no, sigfunc * sig_handler)
{
    struct sigaction sa, osa;

    if (sig_no < 0)
	return NULL;

    sa.sa_handler = sig_handler;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, sig_no);

    /* this is ugly, but the `correct' way.  i hate c. -mrg */
    sa.sa_flags = 0;
#if defined(SA_RESTART) || defined(SA_INTERRUPT)
    if (SIGALRM == sig_no || SIGINT == sig_no) {
#if defined(SA_INTERRUPT)
	sa.sa_flags |= SA_INTERRUPT;
#endif				/* SA_INTERRUPT */
    } else {
#if defined(SA_RESTART)
	sa.sa_flags |= SA_RESTART;
#endif				/* SA_RESTART */
    }
#endif				/* SA_RESTART || SA_INTERRUPT */

    if (0 > sigaction(sig_no, &sa, &osa))
	return ((sigfunc *) SIG_ERR);

    return ((sigfunc *) osa.sa_handler);
}

/**
 * signals_init - install our signal handlers
 *
 * This installs all of our signal handlers. 
 * It also maintains a copy of old signal handlers
 * when the rep library is used, so we can notify
 * the library of events.
 *
 * Make sure to call me once, after the reo library initilization
 * but before we do anything.
 **/
void signals_init(void)
{

#ifdef XARIC_DEBUG
    static int once = 0;

    if (once++)
	ircpanic("second call to signals_init() ??");
#endif

    my_signal(SIGBUS, sig_coredump);
    old_segv = my_signal(SIGSEGV, sig_coredump);

    my_signal(SIGQUIT, SIG_IGN);
    my_signal(SIGPIPE, SIG_IGN);

    my_signal(SIGHUP, sig_quit_irc);
    my_signal(SIGTERM, sig_quit_irc);

    my_signal(SIGALRM, sig_nothing);
    my_signal(SIGCONT, sig_continue);
    my_signal(SIGWINCH, sig_refresh_screen);

    old_sigint = my_signal(SIGINT, sig_int);
    old_sigchild = my_signal(SIGCHLD, sig_child);

    /* do this test now, instead of in each of the signal handlers. */
    if (!VALID_SIGHAND(old_segv))
	old_segv = sig_nothing;
    if (!VALID_SIGHAND(old_sigint))
	old_sigint = sig_nothing;
    if (!VALID_SIGHAND(old_sigchild))
	old_sigchild = sig_nothing;
}
