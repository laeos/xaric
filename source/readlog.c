#ident "@(#)readlog.c 1.9"
/* 
 * Copyright Colten Edwards 1996
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include <sys/stat.h>

#include "ircterm.h"
#include "server.h"
#include "vars.h"
#include "ircaux.h"
#include "input.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "misc.h"
#include "tcommand.h"
#include "util.h"

FILE *msg_fp = NULL;

static Window *msg_window = NULL;
static int finished_msg_paging = 0;
static Screen *msg_screen = NULL;
static int use_msg_window = 0;
static void log_prompt (char *name, char *line);
static void set_msg_screen (Screen *);
static char *(*read_log_func) (char *, int, FILE *);
void log_put_it (const char *topic, const char *format,...);

void
cmd_remove_log (struct command *cmd, char *args)
{
	char *expand;
	char *filename = NULL;
	int old_display = window_display;

	int reset_logptr = 0;

	if ((get_string_var (MSGLOGFILE_VAR) == NULL))
		return;
	malloc_sprintf (&filename, "%s", get_string_var (MSGLOGFILE_VAR));
	expand = expand_twiddle (filename);
	new_free (&filename);
	window_display = 0;
	reset_logptr = logmsg (LOG_CURRENT, NULL, NULL, 3);
	log_toggle (0, NULL);
	window_display = old_display;
	if (unlink (expand))
	{
		bitchsay ("Error unlinking: %s", expand);
		new_free (&expand);
		return;
	}
	window_display = 0;
	set_int_var (MSGCOUNT_VAR, 0);
	if (reset_logptr)
		log_toggle (1, NULL);
	window_display = old_display;
	bitchsay ("Removed %s.", expand);
	new_free (&expand);
}

static int in_read_log = 0;

void
cmd_readlog (struct command *cmd, char *args)
{
	char *expand;
	struct stat stat_buf;
	char *filename = NULL;
	static char buffer[BIG_BUFFER_SIZE + 1];

	read_log_func = fgets;
	if (!get_string_var (MSGLOGFILE_VAR))
		if (!args || (args && !*args))
			return;
	if (msg_window)
		return;

	if (*cmd->name != 'R')
		in_read_log = 1;
	if (*cmd->name == 'M')
		read_log_func = rfgets;
	if (args && *args)
		malloc_sprintf (&filename, "%s", args);
	else
		malloc_sprintf (&filename, "%s", get_string_var (MSGLOGFILE_VAR));

	expand = expand_twiddle (filename);
	new_free (&filename);
	stat (expand, &stat_buf);
	strcpy (buffer, expand);

	if (stat_buf.st_mode & S_IFDIR)
		return;

	if ((msg_fp = fopen (expand, "r")) == NULL)
	{
		log_put_it (expand, "%s Error Opening Log file %s", line_thing, expand);
		new_free (&expand);
		msg_fp = NULL;
		return;
	}
	if (read_log_func == rfgets)
		fseek (msg_fp, 0, SEEK_END);
	new_free (&expand);
	msg_window = curr_scr_win;
	msg_screen = current_screen;
	log_prompt (buffer, NULL);
}

/*
 * show_help:  show's a page of text from a help_fp. If it gets to the end,
 * (in either case it will eventally), it closes the file, and returns 0
 * to indicate this.
 */
static int 
show_log (Window * window, char *name)
{
	Window *old_window;
	int rows = 0;
	char line[300];

	if (window)
	{
		old_window = curr_scr_win;
		curr_scr_win = window;
	}
	else
	{
		old_window = NULL;
		window = curr_scr_win;
	}
	rows = window->display_size - (window->double_status + 2);
	while (--rows)
	{
		if ((*read_log_func) (line, 299, msg_fp))
		{
			if (*(line + strlen (line) - 1) == '\n')
				*(line + strlen (line) - 1) = (char) 0;
			log_put_it (name, "%s", line);
		}
		else
		{
			if (msg_fp)
				fclose (msg_fp);
			set_msg_screen (NULL);
			msg_fp = NULL;
			return (0);
		}
	}
	return (1);
}

static void 
remove_away_log (char *stuff, char *line)
{
	if ((line && toupper (*line) == 'Y'))
		t_parse_command ("REMLOG", NULL);
	in_read_log = 0;
}

static void 
set_msg_screen (Screen * screen)
{
	msg_screen = screen;
	if (!msg_screen && msg_window)
	{
		if (use_msg_window)
		{
			int display = window_display;

			window_display = 0;
			delete_window (msg_window);
			window_display = display;
		}
		msg_window = NULL;
		update_all_windows ();
	}
}

/*
 * log_prompt: The main procedure called to display the log file
 * currently being accessed.  Using add_wait_prompt(), it sets it
 * self up to be recalled when the next page is asked for.   If
 * called when we have finished paging the log file, we exit, as
 * there is nothing left to show.  If line is 'q' or 'Q', exit the
 * log pager, clean up, etc..  If all is cool for now, we call
 * show_help, and either if its finished, exit, or prompt for the
 * next page.   
 */

static void 
log_prompt (char *name, char *line)
{

	if (line && ((*line == 'q') || (*line == 'Q')))
	{
		finished_msg_paging = 1;
		if (msg_fp)
			fclose (msg_fp);
		msg_fp = NULL;
		set_msg_screen (NULL);
		if (!in_read_log)
			add_wait_prompt ("Delete msg log [y/N]? ", remove_away_log, "", WAIT_PROMPT_LINE);
		return;
	}

	if (show_log (msg_window, name))
	{
		add_wait_prompt ("*** Hit any key for more, 'q' to quit ***",
				 log_prompt, name, WAIT_PROMPT_KEY);
	}
	else
	{
		if (msg_fp)
			fclose (msg_fp);
		set_msg_screen (NULL);
		msg_fp = NULL;
		if (!in_read_log)
			add_wait_prompt ("Delete msg log [y/N]? ", remove_away_log, "", WAIT_PROMPT_LINE);
	}
}
