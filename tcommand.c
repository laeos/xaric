#ident "@(#)tcommand.c 1.5"
/*
 * tcommand.c : new implementation of command,searching,aliases for Xaric.
 * (c) 1998 Rex Feany <laeos@ptw.com> 
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
#include <string.h>
#include <stdlib.h>

#include "irc.h"
#include "irc_std.h"
#include "ircterm.h"
#include "ircaux.h"
#include "vars.h"
#include "tcommand.h"
#include "output.h"
#include "input.h"
#include "misc.h"

struct tnode *t_commands;

/* t_parse_command: given a command, and its line, we find it and exec it */
void t_parse_command(char *command, char *line)
{
    struct command *cmd = t_search(t_commands, command);

    if (cmd == NULL)
	bitchsay("Unknown command '%s'!", command);
    else if (cmd == C_AMBIG)
	bitchsay("Ambiguous command '%s'!", command);
    else
	cmd->fcn(cmd, line);
}

/*
 * t_compleate_command:
 *
 *   Given a command fragment, display matches and return NULL if ambig,
 *   or return a static string of full command if not ambig and there
 *   is a match.
 *
 */

static void t_print_cmd(struct command *cmd, char *buf)
{
    strmcat(buf + 1, cmd->name, BIG_BUFFER_SIZE);
    strmcat(buf + 1, " ", BIG_BUFFER_SIZE);
    if (++buf[0] == 4) {
	put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buf + 1));
	buf[1] = 0;
	buf[0] = 0;
    }

}

static char *t_compleate_command(char *command)
{
    struct command *cmd = t_search(t_commands, command);

    if (cmd == NULL)
	return NULL;
    if (cmd == C_AMBIG) {
	char buf[BIG_BUFFER_SIZE + 1];

	bitchsay("Commands:");

	buf[0] = 0;
	buf[1] = 0;

	t_traverse(t_commands, command, t_print_cmd, buf);
	if (buf[0])
	    put_it("%s", convert_output_format("$G $[15]0 $[15]1 $[15]2 $[15]3", "%s", buf + 1));

	return C_AMBIG;
    } else
	return cmd->name;
}

/*
 * command_completion: builds lists of commands and aliases that match the
 * given command and displays them for the user's lookseeing 
 */
void command_completion(char unused, char *not_used)
{
    char *line = NULL, *com, *cmdchars, *rest, *excom, firstcmdchar[2] = "/";
    char buffer[BIG_BUFFER_SIZE + 1];

    malloc_strcpy(&line, get_input());
    if ((com = next_arg(line, &rest)) != NULL) {
	if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
	    cmdchars = DEFAULT_CMDCHARS;
	if (strchr(cmdchars, *com)) {
	    com++;
	    *firstcmdchar = *cmdchars;

	    if (*com && (strchr(cmdchars, *com) || strchr("$", *com)))
		com++;

	    upper(com);

	    excom = t_compleate_command(com);

	    if (excom == NULL || excom == C_AMBIG)
		term_beep();
	    else {
		snprintf(buffer, BIG_BUFFER_SIZE, "%s%s %s", firstcmdchar, excom, rest);
		set_input(buffer);
		update_input(UPDATE_ALL);
	    }
	} else
	    term_beep();
    } else
	term_beep();
    new_free(&line);
}

/* -- alias / binding type support --------------------------------------- */

struct bound_struct {
    struct command *cmd;
    char *args;
    int use;
};

void t_bind_exec(struct command *cmd, char *line)
{
    char *realargs = NULL;
    int len;
    struct bound_struct *bnd = (struct bound_struct *) cmd->data;

    len = (line ? strlen(line) : 0) + (bnd->args ? strlen(bnd->args) : 0);

    if (len) {
	realargs = malloc(len + 1);
	if (bnd->args)
	    strcpy(realargs, bnd->args);
	if (line)
	    strcat(realargs, line);
    }
    bnd->cmd->fcn(bnd->cmd, realargs);
    if (len)
	free(realargs);
}

/* t_bind: make 'string' a new command that does 'command args' */
int t_bind(char *string, char *command, char *args)
{
    struct command *newcmd;
    struct bound_struct *bnd;
    struct command *cmd = t_search(t_commands, command);

    if (cmd == NULL || cmd == C_AMBIG)
	return -1;

    /* XXX TODO fix it so we can bind to a bind */
    if (cmd->data) {
	yell("Error: can't bind to a bound function! FIXME!");
	return -1;
    }

    t_commands = t_remove(t_commands, string, &newcmd);

    if ((newcmd == NULL) || (newcmd->data == NULL)) {
	newcmd = malloc(sizeof(struct command));
	bnd = malloc(sizeof(struct bound_struct));
	newcmd->data = (void *) bnd;
    } else {
	free(newcmd->name);
	bnd = (struct bound_struct *) newcmd->data;
	if (bnd->args)
	    free(bnd->args);
    }

    bnd->cmd = cmd;
    bnd->args = NULL;
    if (args)
	bnd->args = strdup(args);

    newcmd->fcn = t_bind_exec;
    newcmd->name = strdup(string);
    newcmd->qhelp = "bounded command";

    t_commands = t_insert(t_commands, string, newcmd);
    return 0;
}

int t_unbind(char *command)
{
    struct bound_struct *bnd;
    struct command *cmd = NULL;

    t_commands = t_remove(t_commands, command, &cmd);

    if (cmd) {
	bnd = (struct bound_struct *) cmd->data;
	if (bnd) {
	    if (bnd->args)
		free(bnd->args);
	    free(bnd);
	    free(cmd->name);
	    free(cmd);
	}
    }
    return 0;
}
