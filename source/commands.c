#ident "@(#)commands.c 1.9"
/*
 * commands.c: This is really a mishmash of function and such that deal with IRCII
 * commands (both normal and keybinding commands) 
 *
 * Written By Michael Sandrof
 * Portions are based on EPIC.
 * Modified by panasync (Colten Edwards) 1995-97
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include <sys/stat.h>

#ifdef ESIX
#include <lan/net_types.h>
#endif

#include "parse.h"
#include "ircterm.h"
#include "server.h"
#include "commands.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "timer.h"
#include "list.h"
#include "misc.h"
#include "hash2.h"
#include "fset.h"
#include "notice.h"
#include "tcommand.h"
#include "util.h"
#include "expr.h"



extern int doing_notice;


/* The maximum number of recursive LOAD levels allowed */
#define MAX_LOAD_DEPTH 10

/* recv_nick: the nickname of the last person to send you a privmsg */
char *recv_nick = NULL;

/* sent_nick: the nickname of the last person to whom you sent a privmsg */
char *sent_nick = NULL;
char *sent_body = NULL;

/* Used to keep down the nesting of /LOADs and to determine if we
 * should activate the warning for /ONs if the NOVICE variable is set.
 */
int load_depth = 0;



/* Used to prevent global messaging */
extern int doing_who;


/*
 * irc_command: all the availble irc commands:  Note that the first entry has
 * a zero length string name and a null server command... this little trick
 * makes "/ blah blah blah" to always be sent to a channel, bypassing queries,
 * etc.  Neato.  This list MUST be sorted.
 */


#if 0
static IrcCommand irc_command[] =
{
	{"LOAD", "LOAD", load, 0, "filename\n- Loads filename as a irc script"},
	{"ON", NULL, oncmd, 0, scripting_command},
	{"SHOOK", "Shook", shook, 0, NULL},
};
#endif


struct target_type
{
	char *nick_list;
	char *message;
	int hook_type;
	char *command;
	char *format;
	int level;
	char *output;
	char *other_output;
};


int current_target = 0;

/*
 * The whole shebang.
 *
 * The message targets are parsed and collected into one of 4 buckets.
 * This is not too dissimilar to what was done before, except now i 
 * feel more comfortable about how the code works.
 *
 * Bucket 0 -- Unencrypted PRIVMSGs to nicknames
 * Bucket 1 -- Unencrypted PRIVMSGs to channels
 * Bucket 2 -- Unencrypted NOTICEs to nicknames
 * Bucket 3 -- Unencrypted NOTICEs to channels
 *
 * All other messages (encrypted, and DCC CHATs) are dispatched 
 * immediately, and seperately from all others.  All messages that
 * end up in one of the above mentioned buckets get sent out all
 * at once.
 */
void 
send_text (char *nick_list, char *text, char *command, int hook, int log)
{
	int i, old_server;
	int lastlog_level;
	int is_current = 0;
	static int recursion = 0;
	char *current_nick, *next_nick, *free_nick;
	char *line;
	int old_window_display = window_display;

	struct target_type target[4] =
	{
		{NULL, NULL, SEND_MSG_LIST, "PRIVMSG", "*%s*> %s", LOG_MSG, NULL, NULL},
		{NULL, NULL, SEND_PUBLIC_LIST, "PRIVMSG", "%s> %s", LOG_PUBLIC, NULL, NULL},
		{NULL, NULL, SEND_NOTICE_LIST, "NOTICE", "-%s-> %s", LOG_NOTICE, NULL, NULL},
		{NULL, NULL, SEND_NOTICE_LIST, "NOTICE", "-%s-> %s", LOG_NOTICE, NULL, NULL}
	};


	target[0].output = get_fset_var (FORMAT_SEND_MSG_FSET);
	target[1].output = get_fset_var (FORMAT_SEND_PUBLIC_FSET);
	target[1].other_output = get_fset_var (FORMAT_SEND_PUBLIC_OTHER_FSET);
	target[2].output = get_fset_var (FORMAT_SEND_NOTICE_FSET);
	target[3].output = get_fset_var (FORMAT_SEND_NOTICE_FSET);
	target[3].other_output = get_fset_var (FORMAT_SEND_NOTICE_FSET);

	if (recursion)
		hook = 0;
	window_display = hook;
	recursion++;
	free_nick = next_nick = m_strdup (nick_list);

	while ((current_nick = next_nick))
	{
		if ((next_nick = strchr (current_nick, ',')))
			*next_nick++ = 0;

		if (!*current_nick)
			continue;

		if (*current_nick == '%')
		{
			if ((i = get_process_index (&current_nick)) == -1)
				say ("Invalid process specification");
			else
				text_to_process (i, text, 1);
		}
		/*
		 * This test has to be here because it is legal to
		 * send an empty line to a process, but not legal
		 * to send an empty line anywhere else.
		 */
		else if (!text || !*text)
			;
		else if (*current_nick == '"')
			send_to_server ("%s", text);
		else if (*current_nick == '/')
		{
			line = m_opendup (current_nick, space_string, text, NULL);
			parse_command (line, 0, empty_string);
			new_free (&line);
		}
		else if (*current_nick == '=')
		{
			if (!dcc_active (current_nick + 1) && !dcc_activeraw (current_nick + 1))
			{
				yell ("No DCC CHAT connection open to %s", current_nick + 1);
				continue;
			}

			old_server = from_server;
			from_server = -1;
			if (dcc_active (current_nick + 1))
			{
				dcc_chat_transmit (current_nick + 1, text, text, command);
				if (hook)
					addtabkey (current_nick, "msg");
			}
			else
				dcc_raw_transmit (current_nick + 1, line, command);

			from_server = old_server;
		}
		else
		{
			if (doing_notice)
			{
				say ("You cannot send a message from within ON NOTICE");
				continue;
			}

			if (!(i = is_channel (current_nick)) && hook)
				addtabkey (current_nick, "msg");

			if (doing_notice || (command && !strcmp (command, "NOTICE")))
				i += 2;

			if (target[i].nick_list)
				malloc_strcat (&target[i].nick_list, ",");
			malloc_strcat (&target[i].nick_list, current_nick);
			if (!target[i].message)
				target[i].message = text;
		}
	}

	for (i = 0; i < 4; i++)
	{
		char *copy = NULL;
		if (!target[i].message)
			continue;

		lastlog_level = set_lastlog_msg_level (target[i].level);
		message_from (target[i].nick_list, target[i].level);

		copy = m_strdup (target[i].message);

		/* log it if logging on */
		if (log)
			logmsg (LOG_SEND_MSG, target[i].nick_list, target[i].message, 0);

		if (i == 1 || i == 3)
		{
			char *channel;
			is_current = is_current_channel (target[i].nick_list, from_server, 0);
			if ((channel = get_current_channel_by_refnum (0)))
			{
				ChannelList *chan;
				NickList *nick = NULL;
				if ((chan = lookup_channel (channel, from_server, 0)))
				{
					chan->stats_pubs++;
					if ((nick = find_nicklist_in_channellist (get_server_nickname (from_server), chan, 0)))
						nick->stat_pub++;
				}
			}
		}
		else
			is_current = 1;

		if (hook && do_hook (target[i].hook_type, "%s %s", target[i].nick_list, target[i].message))
		{
			if (is_current)
				put_it ("%s", convert_output_format (target[i].output,
								     "%s %s %s %s", update_clock (GET_TIME), target[i].nick_list, get_server_nickname (from_server), copy));
			else
				put_it ("%s", convert_output_format (target[i].other_output,
								     "%s %s %s %s", update_clock (GET_TIME), target[i].nick_list, get_server_nickname (from_server), copy));
		}
		new_free (&copy);
		send_to_server ("%s %s :%s", target[i].command, target[i].nick_list, target[i].message);
		new_free (&target[i].nick_list);
		target[i].message = NULL;
		set_lastlog_msg_level (lastlog_level);
		message_from (NULL, LOG_CRAP);
	}

	if (hook && get_server_away (curr_scr_win->server) && get_int_var (AUTO_UNMARK_AWAY_VAR))
		parse_line (NULL, "AWAY", empty_string, 0, 0);

	new_free (&free_nick);
	window_display = old_window_display;
	recursion--;
}


/* parse_line: This is the main parsing routine.  It should be called in
 * almost all circumstances over parse_command().
 *
 * parse_line breaks up the line into commands separated by unescaped
 * semicolons if we are in non interactive mode. Otherwise it tries to leave
 * the line untouched.
 *
 * Currently, a carriage return or newline breaks the line into multiple
 * commands too. This is expected to stop at some point when parse_command
 * will check for such things and escape them using the ^P convention.
 * We'll also try to check before we get to this stage and escape them before
 * they become a problem.
 *
 * Other than these two conventions the line is left basically untouched.
 */
extern void 
parse_line (char *name, char *org_line, char *args, int hist_flag, int append_flag)
{
	char *line = NULL, *free_line, *stuff, *s, *t;
	int args_flag = 0;


	malloc_strcpy (&line, org_line);
	free_line = line;

	if (!*org_line)
		send_text (get_target_by_refnum (0), NULL, NULL, 1, 1);
	else if (args)
		do
		{
			if (!line || !*line)
			{
				new_free (&free_line);
				return;
			}
			stuff = expand_alias (line, args, &args_flag, &line);
			if (!line && append_flag && !args_flag && args && *args)
				m_3cat (&stuff, " ", args);

			parse_command (stuff, hist_flag, args);
			if (!line)
				new_free (&free_line);
			new_free (&stuff);
		}
		while (line && *line);
	else
	{
		if (load_depth)
			parse_command (line, hist_flag, args);
		else
		{
			while ((s = line))
			{
				if ((t = sindex (line, "\r\n")))
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				parse_command (s, hist_flag, args);
			}
		}
	}
	new_free (&free_line);
	return;
}

/*
 * parse_command: parses a line of input from the user.  If the first
 * character of the line is equal to irc_variable[CMDCHAR_VAR].value, the
 * line is used as an irc command and parsed appropriately.  If the first
 * character is anything else, the line is sent to the current channel or to
 * the current query user.  If hist_flag is true, commands will be added to
 * the command history as appropriate.  Otherwise, parsed commands will not
 * be added. 
 *
 * Parse_command() only parses a single command.  In general, you will want
 * to use parse_line() to execute things.Parse command recognized no quoted
 * characters or anything (beyond those specific for a given command being
 * executed). 
 */
int 
parse_command (char *line, int hist_flag, char *sub_args)
{
	static unsigned int level = 0;
	unsigned int display, old_display_var;
	char *cmdchars, *com, *this_cmd = NULL;
	int add_to_hist, cmdchar_used = 0;
	int noisy = 1;

	if (!line || !*line)
		return 0;

	if (get_int_var (DEBUG_VAR) & DEBUG_COMMANDS)
		yell ("Executing [%d] %s", level, line);
	level++;

	if (!(cmdchars = get_string_var (CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;

	this_cmd = m_strdup (line);

	add_to_hist = 1;
	display = window_display;
	old_display_var = (unsigned) get_int_var (DISPLAY_VAR);
	for (; *line; line++)
	{
		if (*line == '^' && (!hist_flag || cmdchar_used))
		{
			if (!noisy)
				break;
			noisy = 0;
		}
		else if (strchr (cmdchars, *line))
		{
			cmdchar_used++;
			if (cmdchar_used > 2)
				break;
		}
		else
			break;
	}

	if (!noisy)
		window_display = 0;
	com = line;

	/*
	 * always consider input a command unless we are in interactive mode
	 * and command_mode is off.   -lynx
	 */
	if (hist_flag && !cmdchar_used && !get_int_var (COMMAND_MODE_VAR))
	{
		send_text (get_target_by_refnum (0), line, NULL, 1, 1);
		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_string);
		}
		/* Special handling for ' and : */
	}
	else if (*com == '\'' && get_int_var (COMMAND_MODE_VAR))
	{
		send_text (get_target_by_refnum (0), line + 1, NULL, 1, 1);
		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_string);
		}
	}
	else if ((*com == '@') || (*com == '('))
	{
		/* This kludge fixes a memory leak */
		char *l_ptr;
		/*
		 * This new "feature" masks a weakness in the underlying
		 * grammar that allowed variable names to begin with an
		 * lparen, which inhibited the expansion of $s inside its
		 * name, which caused icky messes with array subscripts.
		 *
		 * Anyhow, we now define any commands that start with an
		 * lparen as being "expressions", same as @ lines.
		 */
		if (*com == '(')
		{
			if ((l_ptr = MatchingBracket (line + 1, '(', ')')))
				*l_ptr++ = 0;
		}

		if (hist_flag && add_to_hist)
		{
			add_to_history (this_cmd);
			set_input (empty_string);
		}
	}
	else
	{
		char *rest, *alias = NULL, *alias_name = NULL;
		int alias_cnt = 0;
		if ((rest = (char *) strchr (com, ' ')) != NULL)
			*(rest++) = (char) 0;
		else
			rest = empty_string;

		upper (com);

		if (alias && alias_cnt < 0)
		{
			if (hist_flag && add_to_hist)
			{
				add_to_history (this_cmd);
				set_input (empty_string);
			}
			parse_line (alias_name, alias, rest, 0, 1);
			new_free (&alias_name);
		}
		else
		{
			/* History */
			if (*com == '!')
			{
				if ((com = do_history (com + 1, rest)) != NULL)
				{
					if (level == 1)
					{
						set_input (com);
						update_input (UPDATE_ALL);
					}
					else
						parse_command (com, 0, sub_args);

					new_free (&com);
				}
				else
					set_input (empty_string);
			}
			else
			{
				if (hist_flag && add_to_hist)
				{
					add_to_history (this_cmd);
					set_input (empty_string);
				}

				/* --- Ewww.. well the rest should go away as tcommand gets used --- */
				t_parse_command (com, rest);
			}
			if (alias)
				new_free (&alias_name);
		}
	}
	if (old_display_var != get_int_var (DISPLAY_VAR))
		window_display = get_int_var (DISPLAY_VAR);
	else
		window_display = display;

	new_free (&this_cmd);
	level--;
	return 0;
}


/*
 * load: the /LOAD command.  Reads the named file, parsing each line as
 * though it were typed in (passes each line to parse_command). 
 Right now, this is broken, as it doesnt handle the passing of
 the '-' flag, which is meant to force expansion of expandos
 with the arguments after the '-' flag.  I think its a lame
 feature, anyhow.  *sigh*.
 */

BUILT_IN_COMMAND (load)
{
	FILE *fp;
	char *filename, *expanded = NULL;
	int flag = 0;
	int paste_level = 0;
	char *start, *current_row = NULL, buffer[BIG_BUFFER_SIZE + 1];
	int no_semicolon = 1;
	char *my_irc_path;
	int display;
	int ack = 0;

	my_irc_path = get_string_var (LOAD_PATH_VAR);
	if (!my_irc_path)
	{
		bitchsay ("LOAD_PATH has not been set");
		return;
	}

	if (load_depth == MAX_LOAD_DEPTH)
	{
		bitchsay ("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		return;
	}
	load_depth++;
	status_update (0);

	/* 
	 * We iterate over the whole list -- if we use the -args flag, the
	 * we will make a note to exit the loop at the bottom after we've
	 * gone through it once...
	 */
	while (args && *args && (filename = next_arg (args, &args)))
	{
		/* 
		   If we use the args flag, then we will get the next
		   filename (via continue) but at the bottom of the loop
		   we will exit the loop 
		 */
		if (my_strnicmp (filename, "-args", strlen (filename)) == 0)
		{
			flag = 1;
			continue;
		}
		else if ((expanded = expand_twiddle (filename)))
		{
			if (!(fp = uzfopen (&expanded, my_irc_path)))
			{
				/* uzfopen emits an error if the file
				 * is not found, so we dont have to. */
				status_update (1);
				load_depth--;
				new_free (&expanded);
				return;
			}
		/* Reformatted by jfn */
/* *_NOT_* attached, so dont "fix" it */
			{
				int in_comment = 0;
				int comment_line = -1;
				int line = 1;
				int paste_line = -1;

				display = window_display;
				window_display = 0;
				current_row = NULL;

				for (;; line++)
				{
					if (fgets (buffer, BIG_BUFFER_SIZE / 2, fp))
					{
						int len;
						char *ptr;

						for (start = buffer; my_isspace (*start); start++)
							;
						if (!*start || *start == '#')
							continue;

						len = strlen (start);
						/*
						 * this line from stargazer to allow \'s in scripts for continued
						 * lines <spz@specklec.mpifr-bonn.mpg.de>
						 */
						/* 
						   If we have \\ at the end of the line, that
						   should indicate that we DONT want the slash to 
						   escape the newline (hop)

						   We cant just do start[len-2] because we cant say
						   what will happen if len = 1... (a blank line)

						   SO.... 
						   If the line ends in a newline, and
						   If there are at least 2 characters in the line
						   and the 2nd to the last one is a \ and,
						   If there are EITHER 2 characters on the line or
						   the 3rd to the last character is NOT a \ and,
						   If the line isnt too big yet and,
						   If we can read more from the file,
						   THEN -- adjust the length of the string
						 */
						while ((start[len - 1] == '\n') &&
						       (len >= 2 && start[len - 2] == '\\') &&
						       (len < 3 || start[len - 3] != '\\') &&
						       (len < (BIG_BUFFER_SIZE / 2)) &&
						       (fgets (&(start[len - 2]), (BIG_BUFFER_SIZE / 2) - len, fp)))
						{
							len = strlen (start);
							line++;
						}

						if (start[len - 1] == '\n')
							start[--len] = '\0';

						while (start && *start)
						{
							char *optr = start;

							/* Skip slashed brackets */
							while ((ptr = sindex (optr, "{};/")) &&
							       ptr != optr && ptr[-1] == '\\')
								optr = ptr + 1;

							/* 
							 * if no_semicolon is set, we will not attempt
							 * to parse this line, but will continue
							 * grabbing text
							 */
							if (no_semicolon)
								no_semicolon = 0;
							else if ((!ptr || (ptr != start || *ptr == '/')) && current_row)
							{
								if (!paste_level)
								{
									parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0);
									new_free (&current_row);
								}
								else if (!in_comment)
									malloc_strcat (&current_row, ";");
							}

							if (ptr)
							{
								char c = *ptr;

								*ptr = '\0';
								if (!in_comment)
									malloc_strcat (&current_row, start);
								*ptr = c;

								switch (c)
								{
									/* switch statement tabbed back */
								case '/':
									{
										/* If we're in a comment, any slashes that arent preceeded by
										   a star is just ignored (cause its in a comment, after all >;) */
										if (in_comment)
										{
											/* ooops! cant do ptr[-1] if ptr == optr... doh! */
											if ((ptr > start) && (ptr[-1] == '*'))
											{
												in_comment = 0;
												comment_line = -1;
											}
											else
												break;
										}
										/* We're not in a comment... should we start one? */
										else
										{
											/* COMMENT_BREAKAGE_VAR */
											if ((ptr[1] == '*') && !get_int_var (COMMENT_BREAKAGE_VAR) && (ptr == start))
												/* This hack (at the request of Kanan) makes it
												   so you can only have comments at the BEGINNING
												   of a line, and any midline comments are ignored.
												   This is required to run lame script packs that
												   do ascii art crap (ie, phoenix, textbox, etc) */
											{
												/* Yep. its the start of a comment. */
												in_comment = 1;
												comment_line = line;
											}
											else
											{
												/* Its NOT a comment. Tack it on the end */
												malloc_strcat (&current_row, "/");

												/* Is the / is at the EOL? */
												if (ptr[1] == '\0')
												{
													/* If we are NOT in a block alias, */
													if (!paste_level)
													{
														/* Send the line off to parse_line */
														parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0);
														new_free (&current_row);
														ack = 0;	/* no semicolon.. */
													}
													else
														/* Just add a semicolon and keep going */
														ack = 1;	/* tack on a semicolon */
												}
											}


										}
										no_semicolon = 1 - ack;
										ack = 0;
										break;
									}
								case '{':
									{
										if (in_comment)
											break;

										/* If we are opening a brand new {} pair, remember the line */
										if (!paste_level)
											paste_line = line;

										paste_level++;
										if (ptr == start)
											malloc_strcat (&current_row, " {");
										else
											malloc_strcat (&current_row, "{");
										no_semicolon = 1;
										break;
									}
								case '}':
									{
										if (in_comment)
											break;
										if (!paste_level)
											yell ("Unexpected } in %s, line %d", expanded, line);
										else
										{
											--paste_level;

											if (!paste_level)
												paste_line = -1;
											malloc_strcat (&current_row, "}");	/* } */
											no_semicolon = ptr[1] ? 1 : 0;
										}
										break;
									}
								case ';':
									{
										if (in_comment)
											break;
										if ((*(ptr + 1) == '\0') && (!paste_level))
										{
											parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0);
											new_free (&current_row);
										}
										else
											malloc_strcat (&current_row, ";");
										no_semicolon = 1;
										break;
									}
									/* End of reformatting */
								}
								start = ptr + 1;
							}
							else
							{
								if (!in_comment)
									malloc_strcat (&current_row, start);
								start = NULL;
							}
						}
					}
					else
						break;
				}
				if (in_comment)
					yell ("File %s ended with an unterminated comment in line %d", expanded, comment_line);
				if (current_row)
				{
					if (paste_level)
						yell ("Unexpected EOF in %s trying to match '{' at line %d",
						      expanded, paste_line);
					else
						parse_line (NULL, current_row, flag ? args : get_int_var (INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0);
					new_free (&current_row);
				}
				new_free (&expanded);
				fclose (fp);
				if (get_int_var (DISPLAY_VAR))
					window_display = display;
			}
/* End of reformatting */
		}
		else
			bitchsay ("Unknown user");
		/* 
		 * brute force -- if we are using -args, load ONLY one file
		 * then exit the while loop.  We could have done this
		 * by assigning args to NULL, but that would only waste
		 * cpu cycles by relooping...
		 */
		if (flag)
			break;
	}			/* end of the while loop that allows you to load
				   more then one file at a time.. */
	status_update (1);
	load_depth--;
/*      send_to_server("%s", "WAITFORNOTIFY"); */
}
