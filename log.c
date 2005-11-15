/*
 * log.c: handles the irc session logging functions 
 *
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

#include "irc.h"

#include <sys/stat.h>

#include "log.h"
#include "vars.h"
#include "screen.h"
#include "output.h"
#include "ircaux.h"

FILE *irclog_fp;
extern char *stripansicodes(char *);

void do_log(int flag, char *logfile, FILE ** fp)
{
    time_t t = time(NULL);

    if (flag) {
	if (*fp)
	    say("Logging is already on");
	else {
	    if (!logfile)
		return;
	    if (!(logfile = expand_twiddle(logfile))) {
		say("SET LOGFILE: No such user");
		return;
	    }

	    if ((*fp = fopen(logfile, get_int_var(APPEND_LOG_VAR) ? "a" : "w")) != NULL) {
		say("Starting logfile %s", logfile);
		chmod(logfile, S_IREAD | S_IWRITE);
		fprintf(*fp, "IRC log started %.24s\n", ctime(&t));
		fflush(*fp);
	    } else {
		say("Couldn't open logfile %s: %s", logfile, strerror(errno));
		*fp = NULL;
	    }
	    new_free(&logfile);
	}
    } else {
	if (*fp) {
	    fprintf(*fp, "IRC log ended %.24s\n", ctime(&t));
	    fflush(*fp);
	    fclose(*fp);
	    *fp = NULL;
	    say("Logfile ended");
	}
    }
}

/* logger: if flag is 0, logging is turned off, else it's turned on */
void logger(Window * win, char *unused, int flag)
{
    char *logfile;

    if ((logfile = get_string_var(LOGFILE_VAR)) == NULL) {
	say("You must set the LOGFILE variable first!");
	set_int_var(LOG_VAR, 0);
	return;
    }
    do_log(flag, logfile, &irclog_fp);
    if (!irclog_fp && flag)
	set_int_var(LOG_VAR, 0);
}

/*
 * set_log_file: sets the log file name.  If logging is on already, this
 * closes the last log file and reopens it with the new name.  This is called
 * automatically when you SET LOGFILE. 
 */
void set_log_file(Window * win, char *filename, int unused)
{
    char *expanded;

    if (filename) {
	if (strcmp(filename, get_string_var(LOGFILE_VAR)))
	    expanded = expand_twiddle(filename);
	else
	    expanded = expand_twiddle(get_string_var(LOGFILE_VAR));
	set_string_var(LOGFILE_VAR, expanded);
	new_free(&expanded);
	if (irclog_fp) {
	    logger(curr_scr_win, NULL, 0);
	    logger(curr_scr_win, NULL, 1);
	}
    }
}

/*
 * add_to_log: add the given line to the log file.  If no log file is open
 * this function does nothing. 
 */
void add_to_log(FILE * fp, time_t t, const char *line)
{
    if (fp) {
	fprintf(fp, "%s\n", !get_int_var(LASTLOG_ANSI_VAR) ? stripansicodes((char *) line) : line);
	fflush(fp);
    }
}
