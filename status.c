#ident "@(#)status.c 1.11"
/*
 * status.c: handles the status line updating, etc for IRCII 
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

#include "ircterm.h"
#include "status.h"
#include "server.h"
#include "vars.h"
#include "hook.h"
#include "input.h"
#include "commands.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "names.h"
#include "ircaux.h"
#include "misc.h"
#include "hash2.h"
#include "fset.h"

#define MY_BUFFER 150

extern time_t start_time;
extern char *DCC_get_current_transfer (void);
extern char *ltoa (long);
extern int get_count_stat, send_count_stat;

static char *convert_format (char *, int);
static char *status_nickname (Window *);
static char *status_query_nick (Window *);
static char *status_right_justify (Window *);
static char *status_chanop (Window *);
static char *status_channel (Window *);
static char *status_server (Window *);
static char *status_mode (Window *);
static char *status_umode (Window *);
static char *status_insert_mode (Window *);
static char *status_overwrite_mode (Window *);
static char *status_away (Window *);
static char *status_oper (Window *);
static char *status_lag (Window *);
static char *status_dcc (Window *);
static char *status_msgcount (Window *);
static char *status_hold (Window *);
static char *status_version (Window *);
static char *status_clock (Window *);
static char *status_hold_lines (Window *);
static char *status_window (Window *);
static char *status_refnum (Window *);
static char *status_topic (Window *);
static char *status_null_function (Window *);
static char *status_notify_windows (Window *);
static char *convert_sub_format (char *, char);
static char *status_voice (Window *);
static char *status_dcccount (Window *);
static char *status_position (Window *);

#define cparse(format, str) convert_output_format(get_fset_var(format), "%s", str)

char *time_format = NULL;	/* XXX Bogus XXX */
char *strftime_24hour = "%R";
char *strftime_12hour = "%I:%M%p";



/*----------------------------*/
/* New stuff                  */

static char *mode_format;
static char *umode_format;
static char *topic_format;
static char *query_format;
static char *clock_format;
static char *hold_lines_format;
static char *channel_format;
static char *server_format;
static char *notify_format;
static char *status_lag_format;
static char *msgcount_format;
static char *dcccount_format;
static char *status_format[2];

#define MAX_FUNCTIONS 33

static char *(*status_func[2][MAX_FUNCTIONS]) _ ((Window *));
static int func_cnt[3];


static char *status_format[2];


/*-----------------------------*/

/* update_clock: figures out the current time and returns it in a nice format */
char *
update_clock (int flag)
{
	static char time_str[61];
	static int min = -1, hour = -1;
	time_t t;
	struct tm *time_val;

	t = time (NULL);
	time_val = localtime (&t);
	if (flag == RESET_TIME || time_val->tm_min != min || time_val->tm_hour != hour)
	{
		int server = from_server;
		from_server = primary_server;

		if (time_format)	/* XXXX Bogus XXXX */
			strftime (time_str, 60, time_format, time_val);
		else if (get_int_var (CLOCK_24HOUR_VAR))
			strftime (time_str, 60, strftime_24hour, time_val);
		else
			strftime (time_str, 60, strftime_12hour, time_val);

		if ((time_val->tm_min != min) || (time_val->tm_hour != hour))
		{
			hour = time_val->tm_hour;
			min = time_val->tm_min;
			do_hook (TIMER_LIST, "%02d:%02d", hour, min);
		}
		if ((t - start_time) > 20)
			check_serverlag (get_server_itsname (from_server));
		from_server = server;
		return (time_str);
	}
	if (flag == GET_TIME)
		return (time_str);
	else
		return (NULL);
}

void 
reset_clock (Window * win, char *unused, int unused1)
{
	update_clock (RESET_TIME);
	update_all_status (curr_scr_win, NULL, 0);
}

/*
 * convert_sub_format: This is used to convert the formats of the
 * sub-portions of the status line to a format statement specially designed
 * for that sub-portions.  convert_sub_format looks for a single occurence of
 * %c (where c is passed to the function). When found, it is replaced by "%s"
 * for use is a sprintf.  All other occurences of % followed by any other
 * character are left unchanged.  Only the first occurence of %c is
 * converted, all subsequence occurences are left unchanged.  This routine
 * mallocs the returned string. 
 */
static char *
convert_sub_format (char *format, char c)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	static char bletch[] = "%% ";
	char *ptr = NULL;
	int dont_got_it = 1;

	if (format == NULL)
		return (NULL);
	*buffer = (char) 0;
	memset (buffer, 0, sizeof (buffer));
	while (format)
	{
		if ((ptr = (char *) strchr (format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat (buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if ((*ptr == c) /* && dont_got_it */ )
			{
				dont_got_it = 0;
				strmcat (buffer, "%s", BIG_BUFFER_SIZE);
			}
			else
			{
				bletch[2] = *ptr;
				strmcat (buffer, bletch, BIG_BUFFER_SIZE);
			}
			ptr++;
		}
		else
			strmcat (buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	malloc_strcpy (&ptr, buffer);
	return (ptr);
}

static char *
convert_format (char *format, int k)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	char *ptr;
	int *cp;
	int doit = 1;

	memset (buffer, 0, sizeof (buffer));
	while (format)
	{
		if ((ptr = (char *) strchr (format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat (buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';

			cp = &func_cnt[k];

			if (*cp < MAX_FUNCTIONS)
			{
				switch (*(ptr++))
				{
				case '&':
					new_free (&dcccount_format);
					status_func[k][(*cp)++] = status_dcccount;
					dcccount_format = convert_sub_format (get_string_var (STATUS_DCCCOUNT_VAR), '&');
					break;

				case '%':
					strmcat (buffer, "%", BIG_BUFFER_SIZE);
					doit = 0;
					break;
				case '>':
					status_func[k][(*cp)++] = status_right_justify;
					break;

				case '^':
					new_free (&msgcount_format);
					msgcount_format = convert_sub_format (get_string_var (STATUS_MSGCOUNT_VAR), '^');
					strmcat (buffer, "%s", BIG_BUFFER_SIZE);
					status_func[k][(*cp)++] = status_msgcount;
					break;

				case '@':
					status_func[k][(*cp)++] = status_chanop;
					break;

				case '+':
					new_free (&mode_format);
					mode_format = convert_sub_format (get_string_var (STATUS_MODE_VAR), '+');
					status_func[k][(*cp)++] = status_mode;
					break;
				case '-':
					new_free (&topic_format);
					topic_format = convert_sub_format (get_string_var (STATUS_TOPIC_VAR), '-');
					status_func[k][(*cp)++] = status_topic;
					break;
				case 'L':
					new_free (&umode_format);
					umode_format = convert_sub_format (get_string_var (STATUS_UMODE_VAR), '#');
					status_func[k][(*cp)++] = status_umode;
					break;
				case '=':
					status_func[k][(*cp)++] = status_voice;
					break;
				case '*':
					status_func[k][(*cp)++] = status_oper;
					break;

				case 'A':
					status_func[k][(*cp)++] = status_away;
					break;
				case 'B':
					new_free (&hold_lines_format);
					hold_lines_format = convert_sub_format (get_string_var (STATUS_HOLD_LINES_VAR), 'B');
					status_func[k][(*cp)++] = status_hold_lines;
					break;
				case 'C':
					new_free (&channel_format);
					channel_format = convert_sub_format (get_string_var (STATUS_CHANNEL_VAR), 'C');
					status_func[k][(*cp)++] = status_channel;
					break;
				case 'D':
					status_func[k][(*cp)++] = status_dcc;
					break;
				case 'E':
					break;
				case 'F':
					new_free (&notify_format);
					notify_format = convert_sub_format (get_string_var (STATUS_NOTIFY_VAR), 'F');
					status_func[k][(*cp)++] = status_notify_windows;
					break;
				case 'G':
					break;
				case 'H':
					status_func[k][(*cp)++] = status_hold;
					break;
				case 'I':
					status_func[k][(*cp)++] = status_insert_mode;
					break;
				case '#':
					new_free (&status_lag_format);
					status_lag_format = convert_sub_format (get_string_var (STATUS_LAG_VAR), 'L');
					status_func[k][(*cp)++] = status_lag;
					break;
				case 'N':
					status_func[k][(*cp)++] = status_nickname;
					break;
				case 'O':
					status_func[k][(*cp)++] = status_overwrite_mode;
					break;
				case 'P':
					status_func[k][(*cp)++] = status_position;
					break;
				case 'Q':
					new_free (&query_format);
					query_format = convert_sub_format (get_string_var (STATUS_QUERY_VAR), 'Q');
					status_func[k][(*cp)++] = status_query_nick;
					break;
				case 'R':
					status_func[k][(*cp)++] = status_refnum;
					break;
				case 'S':
					new_free (&server_format);
					server_format = convert_sub_format (get_string_var (STATUS_SERVER_VAR), 'S');
					status_func[k][(*cp)++] = status_server;
					break;
				case 'T':
					new_free (&clock_format);
					clock_format = convert_sub_format (get_string_var (STATUS_CLOCK_VAR), 'T');
					status_func[k][(*cp)++] = status_clock;
					break;
				case 'V':
					status_func[k][(*cp)++] = status_version;
					break;
				case 'W':
					status_func[k][(*cp)++] = status_window;
					break;
				}
				if (doit)
					strmcat (buffer, "%s", BIG_BUFFER_SIZE);
				else
					doit = 1;

			}
			else
				ptr++;
		}
		else
			strmcat (buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	return m_strdup (buffer);
}

char *alias_special_char (char **, char *, char *, char *, int *);


static void 
fix_status_buffer (char *buffer, int ansi)
{
	char rhs_buffer[3 * BIG_BUFFER_SIZE + 1];
	int in_rhs = 0, ch_lhs = 0, ch_rhs = 0, pr_lhs = 0, pr_rhs = 0,
	  start_rhs = -1, i;

	int len = 0;
	if (ansi)
		len = count_ansi (buffer, -1);
	/*
	 * Count out the characters.
	 * Walk the entire string, looking for nonprintable
	 * characters.  We count all the printable characters
	 * on both sides of the %> tag.
	 */
	for (i = 0; buffer[i]; i++)
	{
		/*
		 * The FIRST such %> tag is used.
		 * Using multiple %>s is bogus.
		 */
		if (buffer[i] == '\f' && start_rhs == -1)
		{
			start_rhs = i;
			in_rhs = 1;
		}
		/*
		 * Is it NOT a printable character?
		 */
		else if (buffer[i] == REV_TOG ||
			 buffer[i] == UND_TOG ||
			 buffer[i] == ALL_OFF ||
			 buffer[i] == BOLD_TOG ||
			 (ansi && vt100_decode (buffer[i])))
		{
			if (!in_rhs)
				ch_lhs++;
			else
				ch_rhs++;
		}
		/*
		 * So it is a printable character.
		 */
		else
		{
			if (!in_rhs)
				ch_lhs++, pr_lhs++;
			else
				ch_rhs++, pr_rhs++;
		}

		/*
		 * Dont allow more than CO printable characters
		 */
		if (pr_lhs + pr_rhs >= term_cols + len)
		{
			buffer[i] = 0;
			break;
		}
	}
	/*
	 * Now if we have a rhs, then we have to adjust it.
	 */
	if (start_rhs != -1)
	{
		strcpy (rhs_buffer, buffer + start_rhs + 1);

		/*
		 * now we put it back.
		 */
		if (pr_lhs + pr_rhs < term_cols)
			sprintf (buffer + start_rhs, "%*s%s",
				 term_cols - 1 - pr_lhs - pr_rhs, empty_str,
				 rhs_buffer);
		else
			strcpy (buffer + start_rhs, rhs_buffer);
	}
	else
	{
		int chars = term_cols - pr_lhs - 1;
		int count;
		char c;
		if (get_int_var (STATUS_NO_REPEAT_VAR))
			c = ' ';
		else
			c = buffer[ch_lhs - 1];

		for (count = 0; count <= chars; count++)
			buffer[ch_lhs + count] = c;
		buffer[ch_lhs + chars] = 0;
	}

}

void 
build_status (Window * win, char *format, int unused)
{
	int i, k;
	char *f = NULL;


	if (!win)
		return;

	for (k = 0; k < 2; k++)
	{
		new_free (&(status_format[k]));
		func_cnt[k] = 0;
		for (i = 0; i < MAX_FUNCTIONS; i++)
			status_func[k][i] = status_null_function;
		switch (k)
		{
		case 0:
			f = get_string_var (STATUS_FORMAT_VAR);
			break;
		case 1:
			f = get_string_var (STATUS_FORMAT2_VAR);
			break;
		}

		if (f)
			status_format[k] = convert_format (f, k);

		for (i = func_cnt[k]; i < MAX_FUNCTIONS; i++)
			status_func[k][i] = status_null_function;
	}
	update_all_status (win, NULL, 0);
}

void 
make_status (Window * win)
{
	char buffer[3 * BIG_BUFFER_SIZE + 1];
	char *func_value[MAX_FUNCTIONS + 10] =
	{NULL};
	char **output;

	int len = 1, status_line, ansi = get_int_var (DISPLAY_ANSI_VAR);

	/* The second status line is only displayed in the bottom window
	 * and should always be displayed, no matter what SHOW_STATUS_ALL
	 * is set to - krys
	 */
	for (status_line = 0; status_line < win->w_status_size; status_line++)
	{
		int position = 0;
		int line = status_line, i;
		if (!status_format[line])
			continue;
		for (i = 0; i < MAX_FUNCTIONS; i++)
			func_value[i] = (status_func[line][i]) (win);
		len = 1;

		if (get_int_var (REVERSE_STATUS_VAR))
			buffer[0] = get_int_var (REVERSE_STATUS_VAR) ? REV_TOG : ' ';
		else
			len = 0;

		snprintf (buffer + len, BIG_BUFFER_SIZE * 3,
			  status_format[line],
			  func_value[0], func_value[1], func_value[2],
			  func_value[3], func_value[4], func_value[5],
			  func_value[6], func_value[7], func_value[8],
			  func_value[9], func_value[10], func_value[11],
			  func_value[12], func_value[13], func_value[14],
			  func_value[15], func_value[16], func_value[17],
			  func_value[18], func_value[19], func_value[20],
			  func_value[21], func_value[22], func_value[23],
			  func_value[24], func_value[25], func_value[26],
			  func_value[27], func_value[28], func_value[29],
			  func_value[30], func_value[31], func_value[32],
			  func_value[33], func_value[34], func_value[35],
			  func_value[36], func_value[37], func_value[38]);

		/*  Patched 26-Mar-93 by Aiken
		 *  make_window now right-justifies everything 
		 *  after a %>
		 *  it's also more efficient.
		 */

		fix_status_buffer (buffer, ansi);
		output = split_up_line (buffer);

		if (!win->status_line[status_line] ||
		    strcmp (output[0], win->status_line[status_line]))

		{
			Screen *old_current_screen;
			char *st = NULL;

			malloc_strcpy (&win->status_line[status_line], buffer);
			old_current_screen = current_screen;
			set_current_screen (win->screen);
			if (ansi)
				st = cparse ((line == 1) ? FORMAT_STATUS2_FSET : FORMAT_STATUS_FSET, output[0]);

			term_move_cursor (position, win->bottom + status_line);

			output_line (st ? st : output[0]);

			cursor_in_display ();
			set_current_screen (old_current_screen);
		}
	}

	cursor_to_input ();
}

static char *
status_nickname (Window * window)
{

	if ((connected_to_server) && !get_int_var (SHOW_STATUS_ALL_VAR)
	    && (!window->current_channel) &&
	    (window->screen->current_window != window))
		return empty_str;
	else
		return (get_server_nickname (window->server));
}

static char *
status_server (Window * window)
{
	char *name;
	static char my_buffer[MY_BUFFER + 1] = "\0";
	if (connected_to_server)
	{
		if (window->server != -1)
		{
			if (server_format)
			{
				name = get_server_name (window->server);
				snprintf (my_buffer, MY_BUFFER, server_format, name);
			}
			else
				*my_buffer = 0;
		}
		else
			strcpy (my_buffer, " No Server");
	}
	else
		*my_buffer = 0;
	return (my_buffer);
}

static char *
status_query_nick (Window * window)
{
	static char my_buffer[MY_BUFFER + 1] = "\0";

	if (window->query_nick && query_format)
	{
		snprintf (my_buffer, MY_BUFFER, query_format, window->query_nick);
		return my_buffer;
	}
	else
		return (empty_str);
}

static char *
status_right_justify (Window * window)
{
	static char my_buffer[] = "\f";

	return (my_buffer);
}


static char *
status_notify_windows (Window * window)
{
	int doneone = 0;
	char buf2[MY_BUFFER + 2];
	static char my_buffer[MY_BUFFER + 1] = "\0";
	Window *old_window;

	if (get_int_var (SHOW_STATUS_ALL_VAR) ||
	    window == window->screen->current_window)
	{
		old_window = window;
		*buf2 = '\0';
#ifdef NEW_NOTIFY
		strcat (buf2, "[");
#endif
		window = NULL;
		while (traverse_all_windows (&window))
		{
#ifdef NEW_NOTIFY
			doneone++;
			if (window->visible)
				strmcat (buf2, "*", MY_BUFFER);
			else if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (who_level & window->notify_level)
				{
					strmcat (buf2, "%G", MY_BUFFER);
					strmcat (buf2, ltoa (window->refnum), MY_BUFFER);
				}
				else
				{
					strmcat (buf2, "%R", MY_BUFFER);
					strmcat (buf2, ltoa (window->refnum), MY_BUFFER);
				}
			}
			else
				strmcat (buf2, ltoa (window->refnum), MY_BUFFER);
#else
			if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (doneone++)
					strmcat (buf2, ",", MY_BUFFER);
				strmcat (buf2, ltoa (window->refnum), MY_BUFFER);
			}
#endif
		}
		window = old_window;
	}
#ifdef NEW_NOTIFY
	strmcat (buf2, "]", MY_BUFFER);
#endif
	if (doneone && notify_format)
	{
#ifdef NEW_NOTIFY
		char *p;
		p = convert_output_format (buf2, NULL, NULL);
		chop (p, 4);
		snprintf (my_buffer, MY_BUFFER, notify_format, p);
#else
		snprintf (my_buffer, MY_BUFFER, notify_format, buf2);
#endif
		return (my_buffer);
	}
	return empty_str;
}

static char *
status_clock (Window * window)
{
	static char my_buf[MY_BUFFER + 1] = "\0";

	if ((get_int_var (CLOCK_VAR) && clock_format) &&
	    (get_int_var (SHOW_STATUS_ALL_VAR) ||
	     (window == window->screen->current_window)))
		snprintf (my_buf, MY_BUFFER, clock_format, update_clock (GET_TIME));
	else
		*my_buf = 0;
	return (my_buf);
}

static char *
status_mode (Window * window)
{
	char *mode = NULL;
	static char my_buffer[MY_BUFFER + 1] = "\0";
	if (window->current_channel)
	{
		mode = get_channel_mode (window->current_channel, window->server);
		if (mode && *mode && mode_format)
			snprintf (my_buffer, MY_BUFFER, mode_format, mode);
		else
			*my_buffer = 0;
	}
	else
		*my_buffer = 0;
	return (my_buffer);
}


static char *
status_umode (Window * window)
{
	char localbuf[MY_BUFFER + 1];
	static char my_buffer[MY_BUFFER + 1] = "\0";

	if ((connected_to_server) && !get_int_var (SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		*localbuf = 0;
	else
	{
		if (window->server >= 0)
			strmcpy (localbuf, get_umode (window->server), MY_BUFFER);
		else
			*localbuf = 0;
	}

	if (*localbuf != '\0' && umode_format)
		snprintf (my_buffer, MY_BUFFER, umode_format, localbuf);
	else
		*my_buffer = 0;
	return (my_buffer);
}

static char *
status_chanop (Window * window)
{
	char *text;
	if (window->current_channel && get_channel_oper (window->current_channel, window->server) &&
	    (text = get_string_var (STATUS_CHANOP_VAR)))
		return (text);
	else
		return (empty_str);
}


static char *
status_hold_lines (Window * window)
{
	int num;
	static char my_buffer[MY_BUFFER + 1] = "\0";

	if ((num = (window->lines_held / 10) * 10))
	{
		snprintf (my_buffer, MY_BUFFER, hold_lines_format, ltoa (num));
		return (my_buffer);
	}
	else
		return (empty_str);
}

static char *
status_msgcount (Window * window)
{
	static char my_buffer[MY_BUFFER + 1] = "\0";

	if (get_int_var (MSGCOUNT_VAR) && msgcount_format)
	{
		snprintf (my_buffer, MY_BUFFER, msgcount_format, ltoa (get_int_var (MSGCOUNT_VAR)));
		return (my_buffer);
	}
	else
		return (empty_str);
}

static char *
status_channel (Window * window)
{
	char channel[IRCD_BUFFER_SIZE + 1];
	static char my_buffer[IRCD_BUFFER_SIZE + 1] = "\0";

	if (window->current_channel /* && chan_is_connected(s, window->server) */ )
	{
		int num;
		if (get_int_var (HIDE_PRIVATE_CHANNELS_VAR) &&
		    is_channel_mode (window->current_channel,
				     MODE_PRIVATE | MODE_SECRET,
				     window->server))
			strmcpy (channel, "*private*", IRCD_BUFFER_SIZE);
		else
			strmcpy (channel, window->current_channel, IRCD_BUFFER_SIZE);

		if ((num = get_int_var (CHANNEL_NAME_WIDTH_VAR)) &&
		    ((int) strlen (channel) > num))
			channel[num] = (char) 0;
		snprintf (my_buffer, IRCD_BUFFER_SIZE, channel_format, channel);
		return (my_buffer);
	}
	return (empty_str);
}


static char *
status_insert_mode (Window * window)
{
	char *text;

	if (get_int_var (INSERT_MODE_VAR) && (get_int_var (SHOW_STATUS_ALL_VAR)
			     || (window->screen->current_window == window)))
		if ((text = get_string_var (STATUS_INSERT_VAR)))
			return text;
	return (empty_str);
}

static char *
status_overwrite_mode (Window * window)
{
	char *text;

	if (!get_int_var (INSERT_MODE_VAR) && (get_int_var (SHOW_STATUS_ALL_VAR)
			     || (window->screen->current_window == window)))
	{
		if ((text = get_string_var (STATUS_OVERWRITE_VAR)))
			return text;
	}
	return (empty_str);
}

static char *
status_away (Window * window)
{
	char *text;

	if (window && connected_to_server && !get_int_var (SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		return empty_str;
	else if (window)
	{
		if (window->server != -1 && server_list[window->server].away &&
		    (text = get_string_var (STATUS_AWAY_VAR)))
			return text;
		else
			return empty_str;
	}
	return empty_str;
}

static char *
status_hold (Window * window)
{
	char *text;

	if (window->holding_something && (text = get_string_var (STATUS_HOLD_VAR)))
		return (text);
	return (empty_str);
}

static char *
status_lag (Window * window)
{
	static char my_buffer[MY_BUFFER + 1] = "\0";
	char *p = NULL;
	if ((window->server > -1) && status_lag_format)
	{
		if (get_server_lag (window->server) > -1)
		{
			p = m_sprintf ("%2d", get_server_lag (window->server));
			snprintf (my_buffer, MY_BUFFER, status_lag_format, p);
			new_free (&p);
		}
		else
			snprintf (my_buffer, MY_BUFFER, status_lag_format, "??");
		return (my_buffer);
	}
	return empty_str;
}

static char *
status_topic (Window * window)
{
	static char my_buffer[MY_BUFFER + 1] = "\0";
	char tmp_buffer[MY_BUFFER + 1];
	ChannelList *chan;
	if (window && window->current_channel && topic_format)
	{
		if ((chan = lookup_channel (window->current_channel, window->server, 0)))
		{
			strmcpy (tmp_buffer, chan->topic ? chan->topic : "No Topic", MY_BUFFER - strlen (topic_format));
			snprintf (my_buffer, MY_BUFFER, topic_format, stripansicodes (tmp_buffer));
			return (my_buffer);
		}
	}
	return empty_str;
}

static char *
status_oper (Window * window)
{
	char *text;

	if (get_server_operator (window->server) &&
	    (text = get_string_var (STATUS_OPER_VAR)) &&
	    (get_int_var (SHOW_STATUS_ALL_VAR) ||
	     connected_to_server ||
	     (window->screen->current_window == window)))
		return (text);
	return (empty_str);
}

static char *
status_voice (Window * window)
{
	char *text;
	if (window->current_channel &&
	    get_channel_voice (window->current_channel, window->server) &&
	    !get_channel_oper (window->current_channel, window->server) &&
	    (text = get_string_var (STATUS_VOICE_VAR)))
		return text;
	return empty_str;
}

static char *
status_window (Window * window)
{
	char *text;

	if ((text = get_string_var (STATUS_WINDOW_VAR)) &&
	    (number_of_windows () > 1) && (window->screen->current_window == window))
		return (text);
	return (empty_str);
}

static char *
status_refnum (Window * window)
{
	static char my_buffer[MY_BUFFER + 1];
	strcpy (my_buffer, window->name ? window->name : ltoa (window->refnum));
	return (my_buffer);
}

static char *
status_version (Window * window)
{
	if ((connected_to_server) && !get_int_var (SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		return (empty_str);
	return ((char *) PACKAGE_VERSION);
}

static char *
status_dcccount (Window * window)
{
	static char my_buffer[MY_BUFFER + 1];

	if (dcccount_format)
	{
		char tmp[30];
		strcpy (tmp, ltoa (send_count_stat));
		snprintf (my_buffer, MY_BUFFER, dcccount_format, ltoa (get_count_stat), tmp);
		return my_buffer;
	}
	return empty_str;
}

static char *
status_null_function (Window * window)
{
	return empty_str;
}


static char *
status_dcc (Window * window)
{
	return DCC_get_current_transfer ();
}

static char *
status_position (Window * window)
{
	static char my_buffer[81];

	snprintf (my_buffer, 80, "(%d-%d)", window->lines_scrolled_back, window->distance_from_display);
	return my_buffer;
}
