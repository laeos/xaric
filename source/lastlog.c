/*
 * lastlog.c: handles the lastlog features of irc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"

#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "hook.h"
#include "status.h"
#include "fset.h"
#include "tcommand.h"

/*
 * lastlog_level: current bitmap setting of which things should be stored in
 * the lastlog.  The LOG_MSG, LOG_NOTICE, etc., defines tell more about this 
 */
static unsigned long lastlog_level;
static unsigned long notify_level;

static unsigned long msglog_level = 0;
unsigned long beep_on_level = 0;


FILE *logptr = NULL;

/*
 * msg_level: the mask for the current message level.  What?  Did he really
 * say that?  This is set in the set_lastlog_msg_level() routine as it
 * compared to the lastlog_level variable to see if what ever is being added
 * should actually be added 
 */
static unsigned long msg_level = LOG_CRAP;

static char *levels[] =
{
	"CRAP", "PUBLIC", "MSGS", "NOTICES",
	"WALLS", "WALLOPS", "NOTES", "OPNOTES",
	"SNOTES", "ACTIONS", "DCC", "CTCP",
	"USERLOG1", "USERLOG2", "USERLOG3", "USERLOG4",
	"USERLOG5", "BEEP", "SEND_MSG", "KILL"
};

#define NUMBER_OF_LEVELS (sizeof(levels) / sizeof(char *))


/* set_lastlog_msg_level: sets the message level for recording in the lastlog */
unsigned long 
set_lastlog_msg_level (unsigned long level)
{
	unsigned long old;

	old = msg_level;
	msg_level = level;
	return (old);
}

/*
 * bits_to_lastlog_level: converts the bitmap of lastlog levels into a nice
 * string format.  Note that this uses the global buffer, so watch out 
 */
char *
bits_to_lastlog_level (unsigned long level)
{
	static char buffer[281];	/* this *should* be enough for this */
	int i;
	unsigned long p;

	if (level == LOG_ALL)
		strcpy (buffer, "ALL");
	else if (level == 0)
		strcpy (buffer, "NONE");
	else
	{
		*buffer = '\0';
		for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
		{
			if (level & p)
			{
				strmcat (buffer, levels[i], 280);
				strmcat (buffer, " ", 280);
			}
		}
	}
	return (buffer);
}

unsigned long 
parse_lastlog_level (char *str)
{
	char *ptr, *rest;
	int len, i;
	unsigned long p, level;
	int neg;

	level = 0;
	while ((str = next_arg (str, &rest)) != NULL)
	{
		while (str)
		{
			if ((ptr = strchr (str, ',')) != NULL)
				*ptr++ = '\0';
			if ((len = strlen (str)) != 0)
			{
				if (my_strnicmp (str, "ALL", len) == 0)
					level = LOG_ALL;
				else if (my_strnicmp (str, "NONE", len) == 0)
					level = 0;
				else
				{
					if (*str == '-')
					{
						str++;
						len--;
						neg = 1;
					}
					else
						neg = 0;
					for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
					{
						if (!my_strnicmp (str, levels[i], len))
						{
							if (neg)
								level &= (LOG_ALL ^ p);
							else
								level |= p;
							break;
						}
					}
					if (i == NUMBER_OF_LEVELS)
						say ("Unknown lastlog level: %s",
						     str);
				}
			}
			str = ptr;
		}
		str = rest;
	}
	return (level);
}

/*
 * set_lastlog_level: called whenever a "SET LASTLOG_LEVEL" is done.  It
 * parses the settings and sets the lastlog_level variable appropriately.  It
 * also rewrites the LASTLOG_LEVEL variable to make it look nice 
 */
void 
set_lastlog_level (Window * win, char *str, int unused)
{
	lastlog_level = parse_lastlog_level (str);
	set_string_var (LASTLOG_LEVEL_VAR, bits_to_lastlog_level (lastlog_level));
	curr_scr_win->lastlog_level = lastlog_level;
}

/*
 * set_msglog_level: called whenever a "SET MSGLOG_LEVEL" is done.  It
 * parses the settings and sets the msglog_level variable appropriately.  It
 * also rewrites the MSGLOG_LEVEL variable to make it look nice 
 */
void 
set_msglog_level (Window * win, char *str, int unused)
{
	msglog_level = parse_lastlog_level (str);
	set_string_var (MSGLOG_LEVEL_VAR, bits_to_lastlog_level (msglog_level));
}

void 
remove_from_lastlog (Window * window)
{
	Lastlog *tmp, *end_holder;

	if (window->lastlog_tail)
	{
		end_holder = window->lastlog_tail;
		tmp = window->lastlog_tail->prev;
		window->lastlog_tail = tmp;
		if (tmp)
			tmp->next = NULL;
		else
			window->lastlog_head = window->lastlog_tail;
		window->lastlog_size--;
		new_free (&end_holder->msg);
		new_free ((char **) &end_holder);
	}
	else
		window->lastlog_size = 0;
}

/*
 * set_lastlog_size: sets up a lastlog buffer of size given.  If the lastlog
 * has gotten larger than it was before, all previous lastlog entry remain.
 * If it get smaller, some are deleted from the end. 
 */
void 
set_lastlog_size (Window * win_unused, char *unused, int size)
{
	int i, diff;
	Window *win = NULL;

	while (traverse_all_windows (&win))
	{
		if (win->lastlog_size > size)
		{
			diff = win->lastlog_size - size;
			for (i = 0; i < diff; i++)
				remove_from_lastlog (win);
		}
		win->lastlog_max = size;
	}
}

/*
 * lastlog: the /LASTLOG command.  Displays the lastlog to the screen. If
 * args contains a valid integer, only that many lastlog entries are shown
 * (if the value is less than lastlog_size), otherwise the entire lastlog is
 * displayed 
 */
void
cmd_lastlog (struct command *cmd, char *args)
{
	int cnt, from = 0, p, i, level = 0, msg_level, len, mask = 0, header = 1,
	  lines = 0;
	Lastlog *start_pos;
	char *match = NULL, *arg;
	char *blah = NULL;
	FILE *fp = NULL;

	message_from (NULL, LOG_CURRENT);
	cnt = curr_scr_win->lastlog_size;

	while ((arg = new_next_arg (args, &args)) != NULL)
	{
		if (*arg == '-')
		{
			arg++;
			if (!(len = strlen (arg)))
			{
				header = 0;
				continue;
			}
			else if (!my_strnicmp (arg, "MAX", len))
			{
				char *ptr = NULL;
				ptr = new_next_arg (args, &args);
				if (ptr)
					lines = atoi (ptr);
				if (lines < 0)
					lines = 0;
			}
			else if (!my_strnicmp (arg, "LITERAL", len))
			{
				if (match)
				{
					say ("Second -LITERAL argument ignored");
					(void) new_next_arg (args, &args);
					continue;
				}
				if ((match = new_next_arg (args, &args)) != NULL)
					continue;
				say ("Need pattern for -LITERAL");
				return;
			}
			else if (!my_strnicmp (arg, "BEEP", len))
			{
				if (match)
				{
					say ("-BEEP is exclusive; ignored");
					continue;
				}
				else
					match = "\007";
			}
#if 0
			else if (!my_strnicmp (arg, "CLEAR", len))
			{
				free_lastlog (curr_scr_win);
				say ("Cleared lastlog");
				return;
			}
#endif
			else if (!my_strnicmp (arg, "FILE", len))
			{
				if (args && *args)
				{
					char *filename;
					filename = next_arg (args, &args);
					if (!(fp = fopen (filename, "w")))
					{
						bitchsay ("cannot open file %s", filename);
						return;
					}
				}
				else
				{
					bitchsay ("Filename needed for save");
					return;
				}
			}
			else
			{
				for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
				{
					if (my_strnicmp (levels[i], arg, len) == 0)
					{
						mask |= p;
						break;
					}
				}
				if (i == NUMBER_OF_LEVELS)
				{
					bitchsay ("Unknown flag: %s", arg);
					message_from (NULL, LOG_CRAP);
					return;
				}
			}
		}
		else
		{
			if (level == 0)
			{
				if (match || isdigit (*arg))
				{
					cnt = atoi (arg);
					level++;
				}
				else
					match = arg;
			}
			else if (level == 1)
			{
				from = atoi (arg);
				level++;
			}
		}
	}
	start_pos = curr_scr_win->lastlog_head;
	for (i = 0; (i < from) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->next)
		if (!mask || (mask & start_pos->level))
			i++;

	level = curr_scr_win->lastlog_level;
	msg_level = set_lastlog_msg_level (0);
	if (start_pos == NULL)
		start_pos = curr_scr_win->lastlog_tail;
	else
		start_pos = start_pos->prev;

	/* Let's not get confused here, display a seperator.. -lynx */
	strip_ansi_in_echo = 0;
	if (header && !fp)
		say ("Lastlog:");

	if (match)
	{
#ifdef __GNUC__

		blah = (char *) alloca (strlen (match) + 4);
		sprintf (blah, "*%s*", match);
#else
		malloc_sprintf (&blah, "*%s*", match);
#endif
	}
	for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->prev)
	{
		if (!mask || (mask & start_pos->level))
		{
			i++;
			if (!match || wild_match (blah, start_pos->msg))
			{
				if (!fp)
				{
					put_it ("%s", !get_int_var (LASTLOG_ANSI_VAR) ? stripansicodes (start_pos->msg) : start_pos->msg);
				}
				else
					fprintf (fp, "%s\n", !get_int_var (LASTLOG_ANSI_VAR) ? stripansicodes (start_pos->msg) : start_pos->msg);
				if (lines == 0)
					continue;
				else if (lines == 1)
					break;
				lines--;
			}
		}
	}
#ifndef __GNUC__
	new_free (&blah);
#endif
	if (header && !fp)
		say ("End of Lastlog");
	strip_ansi_in_echo = 1;
	curr_scr_win->lastlog_level = level;
	message_from (NULL, LOG_CRAP);
	set_lastlog_msg_level (msg_level);
}

/*
 * add_to_lastlog: adds the line to the lastlog.  If the LASTLOG_CONVERSATION
 * variable is on, then only those lines that are user messages (private
 * messages, channel messages, wall's, and any outgoing messages) are
 * recorded, otherwise, everything is recorded 
 */
void 
add_to_lastlog (Window * window, const char *line)
{
	Lastlog *new;

	if (window == NULL)
		window = curr_scr_win;
	if (window->lastlog_level & msg_level)
	{
		/* no nulls or empty lines (they contain "> ") */
		if (line && (strlen (line) > 2))
		{
			new = (Lastlog *) new_malloc (sizeof (Lastlog));
			new->next = window->lastlog_head;
			new->prev = NULL;
			new->level = msg_level;
			new->msg = NULL;
			new->time = time (NULL);
			malloc_strcpy (&(new->msg), line);

			if (window->lastlog_head)
				window->lastlog_head->prev = new;
			window->lastlog_head = new;

			if (window->lastlog_tail == NULL)
				window->lastlog_tail = window->lastlog_head;

			if (window->lastlog_size++ >= window->lastlog_max)
				remove_from_lastlog (window);
		}
	}
}

unsigned long 
real_notify_level (void)
{
	return (notify_level);
}

unsigned long 
real_lastlog_level (void)
{
	return (lastlog_level);
}

void 
set_notify_level (Window * win, char *str, int unused)
{
	notify_level = parse_lastlog_level (str);
	set_string_var (NOTIFY_LEVEL_VAR, bits_to_lastlog_level (notify_level));
	curr_scr_win->notify_level = notify_level;
}

int 
logmsg (unsigned long log_type, char *from, char *string, int flag)
{
	char *timestr;
	time_t t;
	char *filename = NULL;
	char *expand = NULL;
	char *type = NULL;
	unsigned char **lines = NULL;

	context;

	if (!get_string_var (MSGLOGFILE_VAR))
		return 0;

	t = time (NULL);
	timestr = update_clock (GET_TIME);

	switch (flag)
	{
	case 0:
		if (!(type = bits_to_lastlog_level (log_type)))
			type = "Unknown";
		if (!do_hook (MSGLOG_LIST, "%s %s %s %s", timestr, type, from, string ? string : ""))
			break;
		if (!logptr)
			return 0;
		if (msglog_level & log_type)
		{
			lines = split_up_line (stripansicodes (convert_output_format (get_fset_var (FORMAT_MSGLOG_FSET) ? get_fset_var (FORMAT_MSGLOG_FSET) : "[$[10]0] [$1] - $2-", "%s %s %s %s", type, timestr, from, string)));
			for (; *lines; lines++)
				fprintf (logptr, "%s\n", *lines);
			fflush (logptr);
		}
		break;
	case 1:
		malloc_sprintf (&filename, "%s", get_string_var (MSGLOGFILE_VAR));
		expand = expand_twiddle (filename);
		new_free (&filename);
		if (!do_hook (MSGLOG_LIST, "%s %s %s %s", timestr, "On", expand, ""))
		{
			new_free (&expand);
			return 1;
		}
		if (logptr)
		{
			new_free (&expand);
			return 1;
		}
		if (!(logptr = fopen (expand, get_int_var (APPEND_LOG_VAR) ? "at" : "wt")))
		{
			set_int_var (MSGLOG_VAR, 0);
			new_free (&expand);
			return 0;
		}

		fprintf (logptr, "MsgLog started [%s]\n", my_ctime (t));
		fflush (logptr);
		if (string)
		{
			int i;

			i = logmsg (LOG_CURRENT, from, string, 0);
			return i;
		}
		bitchsay ("Now logging messages to: %s", expand);
		new_free (&expand);
		break;
	case 2:
		if (!do_hook (MSGLOG_LIST, "%s %s %s %s", timestr, "Off", "", ""))
			return 1;
		if (!logptr)
			return 1;
		fprintf (logptr, "MsgLog ended [%s]\n", my_ctime (t));
		fclose (logptr);
		logptr = NULL;
		break;
	case 3:
		return logptr ? 1 : 0;
		break;
	case 4:
		if (!logptr)
			return 1;
		fprintf (logptr, "[TimeStamp %s]\n", my_ctime (t));
		fflush (logptr);
		break;
	default:
		bitchsay ("Bad Flag passed to logmsg");
		return 0;
	}
	return 1;
}

void 
set_beep_on_msg (Window * win, char *str, int unused)
{
	beep_on_level = parse_lastlog_level (str);
	set_string_var (BEEP_ON_MSG_VAR, bits_to_lastlog_level (beep_on_level));
}

void
cmd_awaylog (struct command *cmd, char *args)
{
	if (args && *args)
	{
		msglog_level = parse_lastlog_level (args);
		set_string_var (MSGLOG_LEVEL_VAR, bits_to_lastlog_level (msglog_level));
		put_it ("%s", convert_output_format ("$G Away logging set to: $0-", "%s", get_string_var (MSGLOG_LEVEL_VAR)));
	}
	else
		put_it ("%s", convert_output_format ("$G Away logging currently: $0-", "%s", bits_to_lastlog_level (msglog_level)));
}
