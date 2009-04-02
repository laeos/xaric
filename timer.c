/*
 * timer.c -- handles timers in ircII
 * Copyright 1993, 1996 Matthew Green
 * This file organized/adapted by Jeremy Nelson
 *
 * This used to be in edit.c, and it used to only allow you to
 * register ircII commands to be executed later.  I needed more
 * generality then that, specifically the ability to register
 * any function to be called, so i pulled it all together into
 * this file and called it timer.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "ircaux.h"
#include "lastlog.h"
#include "timer.h"
#include "hook.h"
#include "output.h"
#include "commands.h"
#include "misc.h"
#include "vars.h"
#include "fset.h"
#include "tcommand.h"

static void show_timer(const char *command);
void delete_all_timers(void);

/*
 * timercmd: the bit that handles the TIMER command.  If there are no
 * arguements, then just list the currently pending timers, if we are
 * give a -DELETE flag, attempt to delete the timer from the list.  Else
 * consider it to be a timer to add, and add it.
 */
void cmd_timer(struct command *cmd, char *args)
{
    char *waittime, *flag;
    char *want = empty_str;
    char *ptr;
    int repeat = 0;
    long events = 1;

    if (*args == '-' || *args == '/') {
	flag = next_arg(args, &args);

	if (!my_strnicmp(flag + 1, "D", 1)) {	/* DELETE */
	    if (!(ptr = next_arg(args, &args)))
		say("%s: Need a timer reference number for -DELETE", cmd->name);
	    else {
		if (!my_strnicmp(ptr, "A", 1))
		    delete_all_timers();
		else
		    delete_timer(ptr);
	    }

	    return;
	} else if (!my_strnicmp(flag + 1, "REP", 3)) {
	    char *na = next_arg(args, &args);

	    repeat = 0;
	    if (!na || !*na) {
		say("%s: Missing arguement to -REPEAT", cmd->name);
		return;
	    }
	    if (!strcmp(na, "*") || !strcmp(na, "-1"))
		repeat = -1, events = -1;
	    else if ((events = my_atol(na)) == 0)
		return;
	} else if (!my_strnicmp(flag + 1, "REF", 3)) {	/* REFNUM */
	    want = next_arg(args, &args);
	    if (!want || !*want) {
		say("%s: Missing argument to -REFNUM", cmd->name);
		return;
	    }
	} else {
	    say("%s: %s no such flag", cmd->name, flag);
	    return;
	}
    }

    /* else check to see if we have no args -> list */

    if (!(waittime = next_arg(args, &args)))
	show_timer(cmd->name);
    else
	add_timer(want, atol(waittime), events, NULL, args, NULL);
    return;
}

/*
 * This is put here on purpose -- we dont want any of the above functions
 * to have any knowledge of this struct.
 */
static TimerList *PendingTimers;
static char *schedule_timer(TimerList * ntimer);

static char *current_exec_timer = empty_str;

/*
 * ExecuteTimers:  checks to see if any currently pending timers have
 * gone off, and if so, execute them, delete them, etc, setting the
 * current_exec_timer, so that we can't remove the timer while its
 * still executing.
 *
 * changed the behavior: timers will not hook while we are waiting.
 */
extern void ExecuteTimers(void)
{
    time_t now;
    TimerList *current;
    int old_in_on_who;
    static int parsingtimer = 0;

    /* We do NOT want to parse timers while waiting cause it gets icky recursive */
    if (waiting_out > waiting_in || parsingtimer || !PendingTimers)
	return;

    parsingtimer = 1;
    time(&now);
    while (PendingTimers && PendingTimers->time <= now) {
	current = PendingTimers;
	PendingTimers = PendingTimers->next;
	old_in_on_who = in_on_who;

	/* 
	 * If a callback function was registered, then
	 * we use it.  If no callback function was registered,
	 * then we use ''parse_line''.
	 */
	current_exec_timer = current->ref;
	if (current->callback)
	    (*current->callback) (current->command);
	else
	    parse_line("TIMER", (char *) current->command, current->subargs, 0, 0);

	current_exec_timer = empty_str;
	in_on_who = old_in_on_who;

	/* 
	 * Clean up or reschedule the timer
	 */
	if (current->events < 0 || (current->events != 1)) {
	    if (current->events != -1)
		current->events--;
	    current->time += current->interval;
	    schedule_timer(current);
	} else {
	    if (!current->callback) {
		new_free((char **) &current->command);
		new_free((char **) &current->subargs);
	    }
	    new_free(&current);
	}
    }
    parsingtimer = 0;
}

/*
 * show_timer:  Display a list of all the TIMER commands that are
 * pending to be executed.
 */
static void show_timer(const char *command)
{
    TimerList *tmp;
    time_t current, time_left;
    int count = 0;

    for (tmp = PendingTimers; tmp; tmp = tmp->next)
	if (!tmp->callback)
	    count++;

    if (count == 0) {
	say("%s: No commands pending to be executed", command);
	return;
    }

    time(&current);
    put_it("%s", convert_output_format(get_fset_var(FORMAT_TIMER_FSET), "%s %s %s %s", "Timer", "Seconds", "Events", "Command"));
    for (tmp = PendingTimers; tmp; tmp = tmp->next) {
	time_left = tmp->time - current;
	if (time_left < 0)
	    time_left = 0;
#if 0
	if (tmp->callback)
	    continue;
#endif
	put_it("%s",
	       convert_output_format(get_fset_var(FORMAT_TIMER_FSET), "%s %l %d %s", tmp->ref, time_left, tmp->events,
				     tmp->callback ? "(internal callback)" : (tmp->command ? tmp->command : "")));
    }
}

/*
 * create_timer_ref:  returns the lowest unused reference number for a timer
 *
 * This will never return 0 for a refnum because that is what atol() returns
 * on case of error, so that it can never happen that a timer has a refnum
 * of zero which would be tripped if the user did say,
 *      /TIMER -refnum foobar 3 blah blah blah
 * which should elicit an error, not be silently punted.
 */
static int create_timer_ref(char *refnum_want, char *refnum_gets)
{
    TimerList *tmp;
    int refnum = 0;

    /* Max of 10 characters. */
    if (strlen(refnum_want) > REFNUM_MAX)
	refnum_want[REFNUM_MAX] = 0;

    /* If the user doesnt care */
    if (!strcmp(refnum_want, empty_str)) {
	/* Find the lowest refnum available */
	for (tmp = PendingTimers; tmp; tmp = tmp->next) {
	    if (refnum < my_atol(tmp->ref))
		refnum = my_atol(tmp->ref);
	}
	strmcpy(refnum_gets, ltoa(refnum + 1), REFNUM_MAX);
    } else {
	/* See if the refnum is available */
	for (tmp = PendingTimers; tmp; tmp = tmp->next) {
	    if (!my_stricmp(tmp->ref, refnum_want))
		return -1;
	}
	strmcpy(refnum_gets, refnum_want, REFNUM_MAX);
    }

    return 0;
}

/*
 * Deletes a refnum.  This does cleanup only if the timer is a 
 * user-defined timer, otherwise no clean up is done (the caller
 * is responsible to handle it)  This shouldnt output an error,
 * it should be more general and return -1 and let the caller
 * handle it.  Probably will be that way in a future release.
 */
extern int delete_timer(char *ref)
{
    TimerList *tmp, *prev;

    if (current_exec_timer != empty_str) {
	say("You may not remove a TIMER from itself");
	return -1;
    }

    for (prev = tmp = PendingTimers; tmp; prev = tmp, tmp = tmp->next) {
	/* can only delete user created timers */
	if (!my_stricmp(tmp->ref, ref)) {
	    if (tmp == prev)
		PendingTimers = PendingTimers->next;
	    else
		prev->next = tmp->next;
	    if (!tmp->callback) {
		new_free((char **) &tmp->command);
		new_free((char **) &tmp->subargs);
	    }
	    new_free(&tmp);
	    return 0;
	}
    }
    say("TIMER: Can't delete %s, no such refnum", ref);
    return -1;
}

void delete_all_timers(void)
{
    while (PendingTimers)
	delete_timer(PendingTimers->ref);
    return;
}

/*
 * You call this to register a timer callback.
 *
 * The arguments:
 *  refnum_want: The refnum requested.  This should only be sepcified
 *               by the user, functions wanting callbacks should specify
 *               the value -1 which means "dont care".
 * The rest of the arguments are dependant upon the value of "callback"
 *      -- if "callback" is NULL then:
 *  callback:    NULL
 *  what:        some ircII commands to run when the timer goes off
 *  subargs:     what to use to expand $0's, etc in the 'what' variable.
 *
 *      -- if "callback" is non-NULL then:
 *  callback:    function to call when timer goes off
 *  what:        argument to pass to "callback" function.  Should be some
 *               non-auto storage, perhaps a struct or a malloced char *
 *               array.  The caller is responsible for disposing of this
 *               area when it is called, since the timer mechanism does not
 *               know anything of the nature of the argument.
 * subargs:      should be NULL, its ignored anyhow.
 */
char *add_timer(char *refnum_want, long when, long events, int (callback) (void *), char *what, char *subargs)
{
    TimerList *ntimer;
    char refnum_got[REFNUM_MAX + 1] = "";

    if (when == 0)
	return NULL;
    ntimer = (TimerList *) new_malloc(sizeof(TimerList));
    ntimer->in_on_who = in_on_who;
    ntimer->time = time(NULL) + when;
    ntimer->interval = when;
    ntimer->events = events;
    if (create_timer_ref(refnum_want, refnum_got) == -1) {
	say("TIMER: Refnum %s already exists", refnum_want);
	new_free(&ntimer);
	return NULL;
    }

    strcpy(ntimer->ref, refnum_got);
    ntimer->callback = callback;
    if (callback) {
	ntimer->command = (void *) what;
	ntimer->subargs = (void *) NULL;
    } else {
	ntimer->command = m_strdup(what);
	ntimer->subargs = m_strdup(subargs);
    }

    /* we've created it, now put it in order */
    return schedule_timer(ntimer);
}

static char *schedule_timer(TimerList * ntimer)
{
    TimerList **slot;

    /* we've created it, now put it in order */
    for (slot = &PendingTimers; *slot; slot = &(*slot)->next) {
	if ((*slot)->time > ntimer->time)
	    break;
    }
    ntimer->next = *slot;
    *slot = ntimer;
    return ntimer->ref;
}

/*
 * TimerTimeout:  Called from irc_io to help create the timeout
 * part of the call to select.
 */
time_t TimerTimeout(void)
{
    time_t current;
    time_t timeout_in;

    if (!PendingTimers)
	return 100000;		/* Just larger than the maximum of 60 */
    time(&current);
    timeout_in = PendingTimers->time - current;
    return (timeout_in < 0) ? 0 : timeout_in;
}
