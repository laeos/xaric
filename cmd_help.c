#ident "@(#)cmd_help.c 1.8"
/*
 * cmd_help.c -- Xaric help system
 * Copyright (c) 2000 Rex Feany (laeos@laeos.net) 
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

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "struct.h"
#include "window.h"
#include "screen.h"
#include "tcommand.h"
#include "vars.h"

#define HELP_LINE_LEN		81	/* how long of a line do we read? */

/* current topic for display */
static FILE *current_fp = NULL;

/* Help window, if we create one */
static Window *help_window = NULL;

/* Should I show the directory list? */
static int dir_list = 0;

/* private function prototypes */

static void create_help_window(void);
static void destroy_help_window(void);
static int display_some_topic(void);
static void switch_space(char *str);
static void move_down(char *data);
static void dir_prompt_callback(char *data, char *line);
static void add_dir_callback(char *dir);
static void pager_callback(char *data, char *line);
static void end_callback(char *data, char *line);
static void add_pager_callback(char *dir);
static void add_end_callback(char *dir);
static void display_topic(char *topic);

static void create_help_window(void)
{
    if (get_int_var(HELP_WINDOW_VAR) && help_window == NULL) {
	help_window = new_window();
	update_all_windows();
    }
}

static void destroy_help_window(void)
{
    if (help_window) {
	delete_window(help_window);
	help_window = NULL;
	update_all_windows();
    }
}

/* This just displays as much of the file as we can, returns 1 if EOF */
static int display_some_topic(void)
{
    int rows = -1;
    char line[HELP_LINE_LEN];
    Window *window = to_window ? to_window : curr_scr_win;

    if (get_int_var(HELP_PAGER_VAR)) {
	rows = window->display_size;
    }

    do {
	if (fgets(line, HELP_LINE_LEN, current_fp) == NULL)
	    return 1;

	if (*line == '#' || *line == '!')
	    continue;

	str_terminate_at(line, "\r\n");

	if (*line)
	    put_it("%s", convert_output_format(line, NULL));
	else
	    put_it(" ");

    } while (--rows);

    return 0;
}

/* Change all the spaces into '/' */
static void switch_space(char *str)
{
    while (*str) {
	if (*str == ' ')
	    *str = '/';
	str++;
    }
}

/* Remove the last directory componant */
static void move_down(char *data)
{
    if (data[0]) {
	char *l = strrchr(data, ' ');

	if (l)
	    *l = '\0';
	else
	    data[0] = '\0';

	add_dir_callback(data);
    } else
	destroy_help_window();

}

/* when we get back from the "help" prompt" */
static void dir_prompt_callback(char *data, char *line)
{
    if (*line == '\0') {
	move_down(data);
    } else if (*line == '?') {
	end_callback(data, empty_str);
    } else if (*line != 'Q' && *line != 'q') {
	char *f = alloca(strlen(data) + strlen(line) + 2);

	sprintf(f, "%s %s", data, line);

	display_topic(f);
    } else {
	destroy_help_window();
    }
}

static void add_dir_callback(char *dir)
{
    const static char *help = "Help? ";
    char buf[HELP_LINE_LEN];

    buf[0] = '\0';

    if (strlen(dir))
	snprintf(buf, HELP_LINE_LEN - strlen(help), "[%s] ", dir);
    strcat(buf, help);

    add_wait_prompt(buf, dir_prompt_callback, dir, WAIT_PROMPT_LINE);
}

static void pager_callback(char *data, char *line)
{
    if (*line != 'Q' && *line != 'q') {
	Window *to_old = to_window;
	int retval = 0;

	to_window = help_window;
	retval = display_some_topic();
	to_window = to_old;

	if (retval == 0) {
	    add_pager_callback(data);
	    return;
	}

	/* display_topic tells is to show the directory list or not */
	if (dir_list) {
	    dir_list = 0;
	    add_end_callback(data);
	} else {
	    add_dir_callback(data);
	}

    } else {
	destroy_help_window();
    }

    fclose(current_fp);
    current_fp = NULL;
}

static void end_callback(char *data, char *line)
{

    if (*line != 'Q' && *line != 'q') {
	char buf[MAXPATHLEN];
	char *help_me = get_string_var(HELP_VAR);
	int retval;
	char *name;

	if (help_me == NULL) {
	    bitchsay("No help directory set!");
	    return;
	}

	snprintf(buf, MAXPATHLEN, "%s/%s", help_me, data);
	switch_space(buf);

	retval = xscandir(buf, empty_str, &name);

	switch (retval) {

	case -1:
	    bitchsay("Error looking at directory '%s'!", buf);
	    break;

	case 0:
	    bitchsay("No help files found in '%s'!", buf);
	    move_down(data);
	    break;

	case 1:
	    new_free(&name);
	    /* fall through */
	default:
	    add_dir_callback(data);
	    break;
	}
    }
}

static void add_pager_callback(char *dir)
{
    add_wait_prompt("*** Hit any key for more, 'q' to quit ***", pager_callback, dir, WAIT_PROMPT_LINE);
}

static void add_end_callback(char *dir)
{
    add_wait_prompt("*** Hit any key to end ***", end_callback, dir, WAIT_PROMPT_LINE);
}

static int is_directory(char *path)
{
    struct stat sbf;

    if (stat(path, &sbf) < 0)
	return 0;

    return S_ISDIR(sbf.st_mode);
}

static void display_topic(char *topic)
{
    char *help_me = get_string_var(HELP_VAR);
    char path[MAXPATHLEN];
    char dir[MAXPATHLEN];
    char query[MAXPATHLEN];
    char *next;
    char *name;

    if (help_me == NULL) {
	bitchsay("No help directory set!");
	return;
    }

    create_help_window();
    strncpy(query, topic, MAXPATHLEN);
    strncpy(path, help_me, MAXPATHLEN);
    dir[0] = '\0';

    next = next_arg(topic, &topic);

    if (next == NULL)
	next = empty_str;

    /* walk down the arguments, finding each file/directory */
    do {
	int retval = xscandir(path, next, &name);

	switch (retval) {

	case -1:
	    destroy_help_window();
	    bitchsay("help error: %s (while trying %s)", name, path);
	    return;

	case 0:
	    bitchsay("No help found for '%s'", next);
	    add_dir_callback(dir);
	    return;

	case 1:
	    strcat(path, "/");
	    strcat(path, name);

	    if (is_directory(path)) {
		if (dir[0])
		    strcat(dir, " ");
		strcat(dir, name);
	    } else {
		current_fp = fopen(path, "r");

		if (current_fp) {
		    pager_callback(dir, empty_str);
		} else {
		    bitchsay("help: unable to open '%s'", path);
		    add_dir_callback(dir);
		}
		return;
	    }
	    break;

	default:
	    if (retval < 0) {
		destroy_help_window();
		bitchsay("Error: %s near character %s", query, -retval);
	    } else {
		add_dir_callback(dir);
	    }
	    return;
	}
    } while ((next = next_arg(topic, &topic)));

    /* Ok, we're at the end of the arguments, and we haven't displayed anything */
    strcat(path, "/");
    strcat(path, name);
    current_fp = fopen(path, "r");

    if (current_fp) {
	dir_list = 1;
	pager_callback(dir, empty_str);
    } else {
	bitchsay("help: unable to open '%s'", path);
	add_dir_callback(dir);
    }
}

/**
 * cmd_help - xaric help system
 * @cmd: command struct (unused)
 * @args: what to display help on
 *
 * This command displays help topics.
 **/
void cmd_help(struct command *cmd, char *args)
{
    display_topic(args);
}
