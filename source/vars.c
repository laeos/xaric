#ident "@(#)vars.c 1.15"
/*
 * vars.c: All the dealing of the irc variables are handled here. 
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

#include "status.h"
#include "window.h"
#include "lastlog.h"
#include "log.h"
#include "history.h"
#include "notify.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "whois.h"
#include "ircterm.h"
#include "output.h"
#include "screen.h"
#include "hook.h"
#include "list.h"
#include "server.h"
#include "window.h"
#include "misc.h"
#include "hash2.h"
#include "fset.h"
#include "format.h"
#include "tcommand.h"
#include "util.h"
#include "xmalloc.h"



extern char *auto_str;

extern Screen *screen_list, *current_screen;

int loading_global = 0;

extern ChannelList default_statchan;
extern char three_stars[];


enum VAR_TYPES find_variable (char *, int *);
static void exec_warning (Window *, char *, int);
static void input_warning (Window *, char *, int);
static void eight_bit_characters (Window *, char *, int);
static void set_realname (Window *, char *, int);
static void set_numeric_string (Window *, char *, int);
static void set_user_mode (Window *, char *, int);
static void reinit_screen (Window *, char *, int);
static void reinit_status (Window *, char *, int);
int save_formats (FILE *);

/*
 * irc_variable: all the irc variables used.  Note that the integer and
 * boolean defaults are set here, which the string default value are set in
 * the init_variables() procedure 
 */

static IrcVariable irc_variable[] =
{
	{"ALWAYS_SPLIT_BIGGEST", BOOL_TYPE_VAR, DEFAULT_ALWAYS_SPLIT_BIGGEST, NULL, NULL, 0, 0},
	{"APPEND_LOG", BOOL_TYPE_VAR, 1, NULL, NULL, 0, 0},
	{"AUTO_NSLOOKUP", BOOL_TYPE_VAR, DEFAULT_AUTO_NSLOOKUP, NULL, NULL, 0, 0},
	{"AUTO_RECONNECT", BOOL_TYPE_VAR, 1, NULL, NULL, 0, 0},
	{"AUTO_REJOIN", INT_TYPE_VAR, DEFAULT_AUTO_REJOIN, NULL, NULL, 0, 0},
	{"AUTO_UNBAN", INT_TYPE_VAR, 60 * 10, NULL, NULL, 0, 0},
	{"AUTO_UNMARK_AWAY", BOOL_TYPE_VAR, DEFAULT_AUTO_UNMARK_AWAY, NULL, NULL, 0, 0},
	{"AUTO_WHOWAS", BOOL_TYPE_VAR, DEFAULT_AUTO_WHOWAS, NULL, NULL, 0, 0},
	{"BANTYPE", INT_TYPE_VAR, DEFAULT_BANTYPE, NULL, NULL, 0, 0},
	{"BEEP", BOOL_TYPE_VAR, DEFAULT_BEEP, NULL, NULL, 0, 0},
	{"BEEP_MAX", INT_TYPE_VAR, DEFAULT_BEEP_MAX, NULL, NULL, 0, 0},
	{"BEEP_ON_MSG", STR_TYPE_VAR, 0, NULL, set_beep_on_msg, 0, 0},
	{"BEEP_WHEN_AWAY", INT_TYPE_VAR, DEFAULT_BEEP_WHEN_AWAY, NULL, NULL, 0, 0},
	{"BOLD_VIDEO", BOOL_TYPE_VAR, DEFAULT_BOLD_VIDEO, NULL, NULL, 0, 0},
	{"CHANNEL_NAME_WIDTH", INT_TYPE_VAR, DEFAULT_CHANNEL_NAME_WIDTH, NULL, update_all_status, 0, 0},
	{"CLIENT_INFORMATION", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"CLOCK", BOOL_TYPE_VAR, DEFAULT_CLOCK, NULL, update_all_status, 0, 0},
	{"CLOCK_24HOUR", BOOL_TYPE_VAR, DEFAULT_CLOCK_24HOUR, NULL, reset_clock, 0, 0},
	{"CMDCHARS", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"COMMAND_MODE", BOOL_TYPE_VAR, DEFAULT_COMMAND_MODE, NULL, NULL, 0, 0},
	{"COMMENT_BREAKAGE", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"COMPRESS_MODES", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"CONNECT_TIMEOUT", INT_TYPE_VAR, DEFAULT_CONNECT_TIMEOUT, NULL, NULL, 0, 0},
	{"CONTINUED_LINE", STR_TYPE_VAR, 0, NULL, set_continued_lines, 0, 0},
	{"CTCP_DELAY", INT_TYPE_VAR, 3, NULL, NULL, 0, 0},
	{"CTCP_FLOOD_PROTECTION", BOOL_TYPE_VAR, DEFAULT_CTCP_FLOOD_PROTECTION, NULL, NULL, 0, 0},
	{"CTCP_VERBOSE", BOOL_TYPE_VAR, DEFAULT_VERBOSE_CTCP, NULL, NULL, 0, 0},
	{"DCC_AUTORENAME", BOOL_TYPE_VAR, 1, NULL, NULL, 0, 0},
	{"DCC_BAR_TYPE", INT_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"DCC_BLOCK_SIZE", INT_TYPE_VAR, DEFAULT_DCC_BLOCK_SIZE, NULL, NULL, 0, 0},
	{"DCC_DLDIR", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"DCC_FLOOD_AFTER", INT_TYPE_VAR, 3, NULL, NULL, 0, 0},
	{"DCC_FLOOD_RATE", INT_TYPE_VAR, 4, NULL, NULL, 0, 0},
	{"DEFAULT_REASON", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"DEOPFLOOD", BOOL_TYPE_VAR, DEFAULT_DEOPFLOOD, NULL, NULL, 0, 0},
	{"DEOPFLOOD_TIME", INT_TYPE_VAR, DEFAULT_DEOPFLOOD_TIME, NULL, NULL, 0, 0},
	{"DEOP_ON_DEOPFLOOD", INT_TYPE_VAR, DEFAULT_DEOP_ON_DEOPFLOOD, NULL, NULL, 0, 0},
	{"DEOP_ON_KICKFLOOD", INT_TYPE_VAR, DEFAULT_DEOP_ON_KICKFLOOD, NULL, NULL, 0, 0},
	{"DISPLAY", BOOL_TYPE_VAR, DEFAULT_DISPLAY, NULL, NULL, 0, 0},
	{"DISPLAY_ANSI", BOOL_TYPE_VAR, DEFAULT_DISPLAY_ANSI, NULL, reinit_screen, 0, 0},
	{"EIGHT_BIT_CHARACTERS", BOOL_TYPE_VAR, DEFAULT_EIGHT_BIT_CHARACTERS, NULL, eight_bit_characters, 0, 0},
	{"EXEC_PROTECTION", BOOL_TYPE_VAR, DEFAULT_EXEC_PROTECTION, NULL, exec_warning, 0, VF_NODAEMON},
	{"FAKE_SPLIT_PATS", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},

	{"FLOOD_AFTER", INT_TYPE_VAR, DEFAULT_FLOOD_AFTER, NULL, NULL, 0, 0},
	{"FLOOD_KICK", BOOL_TYPE_VAR, DEFAULT_FLOOD_KICK, NULL, NULL, 0, 0},
	{"FLOOD_PROTECTION", BOOL_TYPE_VAR, DEFAULT_FLOOD_PROTECTION, NULL, NULL, 0, 0},
	{"FLOOD_RATE", INT_TYPE_VAR, DEFAULT_FLOOD_RATE, NULL, NULL, 0, 0},
	{"FLOOD_TIME", INT_TYPE_VAR, DEFAULT_FLOOD_TIME, NULL, NULL, 0, 0},
	{"FLOOD_USERS", INT_TYPE_VAR, DEFAULT_FLOOD_USERS, NULL, NULL, 0, 0},
	{"FLOOD_WARNING", BOOL_TYPE_VAR, DEFAULT_FLOOD_WARNING, NULL, NULL, 0, 0},

	{"HELP", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"HIDE_PRIVATE_CHANNELS", BOOL_TYPE_VAR, DEFAULT_HIDE_PRIVATE_CHANNELS, NULL, update_all_status, 0, 0},
	{"HIGHLIGHT_CHAR", STR_TYPE_VAR, 0, NULL, set_highlight_char, 0, 0},
	{"HISTORY", INT_TYPE_VAR, DEFAULT_HISTORY, NULL, set_history_size, 0, VF_NODAEMON},
	{"HOLD_MODE", BOOL_TYPE_VAR, DEFAULT_HOLD_MODE, NULL, reset_line_cnt, 0, 0},
	{"IGNORE_TIME", INT_TYPE_VAR, 600, NULL, NULL, 0, 0},
	{"INDENT", BOOL_TYPE_VAR, DEFAULT_INDENT, NULL, NULL, 0, 0},
	{"INPUT_ALIASES", BOOL_TYPE_VAR, DEFAULT_INPUT_ALIASES, NULL, NULL, 0, 0},
	{"INPUT_PROMPT", STR_TYPE_VAR, 0, NULL, set_input_prompt, 0, 0},
	{"INPUT_PROTECTION", BOOL_TYPE_VAR, DEFAULT_INPUT_PROTECTION, NULL, input_warning, 0, 0},
	{"INSERT_MODE", BOOL_TYPE_VAR, DEFAULT_INSERT_MODE, NULL, update_all_status, 0, 0},
	{"INVERSE_VIDEO", BOOL_TYPE_VAR, DEFAULT_INVERSE_VIDEO, NULL, NULL, 0, 0},
	{"JOINFLOOD", BOOL_TYPE_VAR, DEFAULT_JOINFLOOD, NULL, NULL, 0, 0},
	{"JOINFLOOD_TIME", INT_TYPE_VAR, DEFAULT_JOINFLOOD_TIME, NULL, NULL, 0, 0},
	{"KICKFLOOD", BOOL_TYPE_VAR, DEFAULT_KICKFLOOD, NULL, NULL, 0, 0},
	{"KICKFLOOD_TIME", INT_TYPE_VAR, DEFAULT_KICKFLOOD_TIME, NULL, NULL, 0, 0},
	{"KICK_ON_DEOPFLOOD", INT_TYPE_VAR, DEFAULT_KICK_ON_DEOPFLOOD, NULL, NULL, 0, 0},
	{"KICK_ON_JOINFLOOD", INT_TYPE_VAR, DEFAULT_KICK_ON_JOINFLOOD, NULL, NULL, 0, 0},
	{"KICK_ON_KICKFLOOD", INT_TYPE_VAR, DEFAULT_KICK_ON_KICKFLOOD, NULL, NULL, 0, 0},
	{"KICK_ON_NICKFLOOD", INT_TYPE_VAR, DEFAULT_KICK_ON_NICKFLOOD, NULL, NULL, 0, 0},
	{"KICK_ON_PUBFLOOD", INT_TYPE_VAR, DEFAULT_KICK_ON_PUBFLOOD, NULL, NULL, 0, 0},
	{"LASTLOG", INT_TYPE_VAR, DEFAULT_LASTLOG, NULL, set_lastlog_size, 0, 0},
	{"LASTLOG_ANSI", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"LASTLOG_LEVEL", STR_TYPE_VAR, 0, NULL, set_lastlog_level, 0, 0},
	{"LLOOK", BOOL_TYPE_VAR, DEFAULT_LLOOK, NULL, NULL, 0, 0},
	{"LLOOK_DELAY", INT_TYPE_VAR, DEFAULT_LLOOK_DELAY, NULL, NULL, 0, 0},
	{"LOAD_PATH", STR_TYPE_VAR, 0, NULL, NULL, 0, VF_NODAEMON},
	{"LOG", BOOL_TYPE_VAR, DEFAULT_LOG, NULL, logger, 0, 0},
	{"LOGFILE", STR_TYPE_VAR, 0, NULL, set_log_file, 0, VF_NODAEMON},
	{"MIRCS", BOOL_TYPE_VAR, 1, NULL, NULL, 0, 0},
	{"MODE_STRIPPER", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"MSGCOUNT", INT_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"MSGLOG", BOOL_TYPE_VAR, DEFAULT_MSGLOG, NULL, NULL, 0, 0},
	{"MSGLOG_FILE", STR_TYPE_VAR, 0, NULL, NULL, 0, 0 | VF_EXPAND_PATH},
	{"MSGLOG_LEVEL", STR_TYPE_VAR, 0, NULL, set_msglog_level, 0, 0},
	{"NEXT_SERVER_ON_LOCAL_KILL", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},

	{"NICKFLOOD", BOOL_TYPE_VAR, DEFAULT_NICKFLOOD, NULL, NULL, 0, 0},
	{"NICKFLOOD_TIME", INT_TYPE_VAR, DEFAULT_NICKFLOOD_TIME, NULL, NULL, 0, 0},
	{"NICK_COMPLETION", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"NICK_COMPLETION_CHAR", CHAR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"NICK_COMPLETION_TYPE", INT_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"NOTIFY_LEVEL", STR_TYPE_VAR, 0, NULL, set_notify_level, 0, 0},
	{"NOTIFY_ON_TERMINATION", BOOL_TYPE_VAR, DEFAULT_NOTIFY_ON_TERMINATION, NULL, NULL, 0, VF_NODAEMON},
	{"NO_CTCP_FLOOD", BOOL_TYPE_VAR, DEFAULT_NO_CTCP_FLOOD, NULL, NULL, 0, 0},
	{"NUM_BANMODES", INT_TYPE_VAR, 4, NULL, NULL, 0, 0},
	{"NUM_OF_WHOWAS", INT_TYPE_VAR, 4, NULL, NULL, 0, 0},
	{"NUM_OPMODES", INT_TYPE_VAR, 3, NULL, NULL, 0, 0},
	{"OPER_MODES", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"ORIGNICK_TIME", INT_TYPE_VAR, 10, NULL, NULL, 0, 0},
	{"PAD_CHAR", CHAR_TYPE_VAR, DEFAULT_PAD_CHAR, NULL, NULL, 0, 0},
	{"PUBFLOOD", BOOL_TYPE_VAR, DEFAULT_PUBFLOOD, NULL, NULL, 0, 0},
	{"PUBFLOOD_TIME", INT_TYPE_VAR, DEFAULT_PUBFLOOD_TIME, NULL, NULL, 0, 0},
	{"REALNAME", STR_TYPE_VAR, 0, 0, set_realname, 0, VF_NODAEMON},
	{"REVERSE_STATUS", BOOL_TYPE_VAR, 0, NULL, reinit_status, 0, 0},
	{"SAVEFILE", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"SCROLL", BOOL_TYPE_VAR, DEFAULT_SCROLL, NULL, set_scroll, 0, 0},
	{"SCROLL_LINES", INT_TYPE_VAR, DEFAULT_SCROLL_LINES, NULL, set_scroll_lines, 0, 0},
	{"SEND_AWAY_MSG", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"SEND_IGNORE_MSG", BOOL_TYPE_VAR, DEFAULT_SEND_IGNORE_MSG, NULL, NULL, 0, 0},
	{"SERVER_PROMPT", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"SHELL", STR_TYPE_VAR, 0, NULL, NULL, 0, VF_NODAEMON},
	{"SHELL_FLAGS", STR_TYPE_VAR, 0, NULL, NULL, 0, VF_NODAEMON},
	{"SHELL_LIMIT", INT_TYPE_VAR, DEFAULT_SHELL_LIMIT, NULL, NULL, 0, VF_NODAEMON},
	{"SHOW_CHANNEL_NAMES", BOOL_TYPE_VAR, DEFAULT_SHOW_CHANNEL_NAMES, NULL, NULL, 0, 0},
	{"SHOW_CTCP_IDLE", BOOL_TYPE_VAR, DEFAULT_SHOW_CTCP_IDLE, NULL, NULL, 0, 0},
	{"SHOW_END_OF_MSGS", BOOL_TYPE_VAR, DEFAULT_SHOW_END_OF_MSGS, NULL, NULL, 0, 0},
	{"SHOW_NUMERICS", BOOL_TYPE_VAR, DEFAULT_SHOW_NUMERICS, NULL, NULL, 0, 0},
	{"SHOW_NUMERICS_STR", STR_TYPE_VAR, 0, NULL, set_numeric_string, 0, 0},
	{"SHOW_STATUS_ALL", BOOL_TYPE_VAR, DEFAULT_SHOW_STATUS_ALL, NULL, update_all_status, 0, 0},
	{"SHOW_WHO_HOPCOUNT", BOOL_TYPE_VAR, DEFAULT_SHOW_WHO_HOPCOUNT, NULL, NULL, 0, 0},
	{"SIGNOFF_REASON", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"STATUS_AWAY", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_CHANNEL", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_CHANOP", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_CLOCK", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_DCCCOUNT", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_FORMAT", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_FORMAT2", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_HOLD", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_HOLD_LINES", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_INSERT", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_LAG", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_MODE", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_MSGCOUNT", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_NOTIFY", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_NO_REPEAT", BOOL_TYPE_VAR, DEFAULT_STATUS_NO_REPEAT, NULL, NULL, 0, 0},
	{"STATUS_OPER", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_OPER_KILLS", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_OVERWRITE", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_QUERY", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_SERVER", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_TOPIC", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_UMODE", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_VOICE", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"STATUS_WINDOW", STR_TYPE_VAR, 0, NULL, build_status, 0, 0},
	{"SUPPRESS_SERVER_MOTD", BOOL_TYPE_VAR, DEFAULT_SUPPRESS_SERVER_MOTD, NULL, NULL, 0, VF_NODAEMON},
	{"TAB", BOOL_TYPE_VAR, DEFAULT_TAB, NULL, NULL, 0, 0},
	{"TAB_MAX", INT_TYPE_VAR, DEFAULT_TAB_MAX, NULL, NULL, 0, 0},

	{"UNDERLINE_VIDEO", BOOL_TYPE_VAR, DEFAULT_UNDERLINE_VIDEO, NULL, NULL, 0, 0},
	{"USERMODE", STR_TYPE_VAR, 0, NULL, set_user_mode, 0, 0},
	{"USER_INFORMATION", STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{"WINDOW_QUIET", BOOL_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{NULL, 0, 0, NULL, NULL, 0, 0}
};


/*
 * get_string_var: returns the value of the string variable given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
char *
get_string_var (enum VAR_TYPES var)
{
	return (irc_variable[var].string);
}

/*
 * get_int_var: returns the value of the integer string given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
int 
get_int_var (enum VAR_TYPES var)
{
	return (irc_variable[var].integer);
}

/*
 * set_string_var: sets the string variable given as an index into the
 * variable table to the given string.  If string is null, the current value
 * of the string variable is freed and set to null 
 */
void 
set_string_var (enum VAR_TYPES var, char *string)
{
	if (string)
		malloc_strcpy (&(irc_variable[var].string), string);
	else
		xfree (&(irc_variable[var].string));
}

/*
 * set_int_var: sets the integer value of the variable given as an index into
 * the variable table to the given value 
 */
void 
set_int_var (enum VAR_TYPES var, unsigned int value)
{
	irc_variable[var].integer = value;
}

/*
 * init_variables: initializes the string variables that can't really be
 * initialized properly above 
 */
void 
init_variables ()
{
	int old_display = window_display;
	char *foo;


#ifdef XARIC_DEBUG
	int i;

	for (i = 1; i < NUMBER_OF_VARIABLES - 1; i++)
		if (strcmp (irc_variable[i - 1].name, irc_variable[i].name) >= 0)
			ircpanic ("Variable [%d] (%s) is out of order.", i, irc_variable[i].name);
#endif

	window_display = 0;
	line_thing = m_strdup(three_stars);

	set_string_var (HELP_VAR, DEFAULT_HELP_FILE);

	set_string_var (MSGLOGFILE_VAR, DEFAULT_MSGLOGFILE);
	set_string_var (MSGLOG_LEVEL_VAR, DEFAULT_MSGLOG_LEVEL);
	set_int_var (LLOOK_DELAY_VAR, DEFAULT_LLOOK_DELAY);
	set_int_var (MSGCOUNT_VAR, 0);

	set_string_var (DEFAULT_REASON_VAR, DEFAULT_KICK_REASON);
	set_string_var (DCC_DLDIR_VAR, DEFAULT_DCC_DLDIR);
	set_string_var (SHOW_NUMERICS_STR_VAR, three_stars);

	set_string_var (CMDCHARS_VAR, DEFAULT_CMDCHARS);
	set_string_var (LOGFILE_VAR, DEFAULT_LOGFILE);
	set_string_var (SHELL_VAR, DEFAULT_SHELL);
	set_string_var (SHELL_FLAGS_VAR, DEFAULT_SHELL_FLAGS);
	set_string_var (CONTINUED_LINE_VAR, DEFAULT_CONTINUED_LINE);
	set_string_var (INPUT_PROMPT_VAR, DEFAULT_INPUT_PROMPT);
	set_string_var (HIGHLIGHT_CHAR_VAR, DEFAULT_HIGHLIGHT_CHAR);
	set_string_var (LASTLOG_LEVEL_VAR, DEFAULT_LASTLOG_LEVEL);
	set_string_var (NOTIFY_LEVEL_VAR, DEFAULT_NOTIFY_LEVEL);
	set_string_var (REALNAME_VAR, realname);

	set_string_var (STATUS_FORMAT_VAR, DEFAULT_STATUS_FORMAT);
	set_string_var (STATUS_FORMAT2_VAR, DEFAULT_STATUS_FORMAT2);

	set_string_var (STATUS_DCCCOUNT_VAR, DEFAULT_STATUS_DCCCOUNT);

	set_string_var (STATUS_AWAY_VAR, DEFAULT_STATUS_AWAY);
	set_string_var (STATUS_CHANNEL_VAR, DEFAULT_STATUS_CHANNEL);
	set_string_var (STATUS_CHANOP_VAR, DEFAULT_STATUS_CHANOP);
	set_string_var (STATUS_CLOCK_VAR, DEFAULT_STATUS_CLOCK);
	set_string_var (STATUS_HOLD_VAR, DEFAULT_STATUS_HOLD);
	set_string_var (STATUS_HOLD_LINES_VAR, DEFAULT_STATUS_HOLD_LINES);
	set_string_var (STATUS_INSERT_VAR, DEFAULT_STATUS_INSERT);
	set_string_var (STATUS_LAG_VAR, DEFAULT_STATUS_LAG);
	set_string_var (STATUS_MODE_VAR, DEFAULT_STATUS_MODE);
	set_string_var (STATUS_MSGCOUNT_VAR, DEFAULT_STATUS_MSGCOUNT);
	set_string_var (STATUS_OPER_VAR, DEFAULT_STATUS_OPER);
	set_string_var (STATUS_VOICE_VAR, DEFAULT_STATUS_VOICE);
	set_string_var (STATUS_NOTIFY_VAR, DEFAULT_STATUS_NOTIFY);
	set_string_var (STATUS_OPER_KILLS_VAR, DEFAULT_STATUS_OPER_KILLS);
	set_string_var (STATUS_OVERWRITE_VAR, DEFAULT_STATUS_OVERWRITE);
	set_string_var (STATUS_QUERY_VAR, DEFAULT_STATUS_QUERY);
	set_string_var (STATUS_SERVER_VAR, DEFAULT_STATUS_SERVER);
	set_string_var (STATUS_TOPIC_VAR, DEFAULT_STATUS_TOPIC);
	set_string_var (STATUS_UMODE_VAR, DEFAULT_STATUS_UMODE);
	set_string_var (STATUS_WINDOW_VAR, DEFAULT_STATUS_WINDOW);
	set_string_var (SIGNOFF_REASON_VAR, DEFAULT_SIGNOFF_REASON);

	set_string_var (USERINFO_VAR, DEFAULT_USERINFO);

	set_string_var (USERMODE_VAR, !send_umode ? DEFAULT_USERMODE : send_umode);
	set_user_mode (curr_scr_win, !send_umode ? DEFAULT_USERMODE : send_umode, 0);

	/* An ugly workaround, because somewhere deep this string is used
	   in a next_arg or similer call, and gets written to */
	foo = m_strdup (DEFAULT_BEEP_ON_MSG);
	set_beep_on_msg (curr_scr_win, foo, 0);
	xfree (&foo);

	set_string_var (CLIENTINFO_VAR, XARIC_COMMENT);
	set_string_var (FAKE_SPLIT_PATS_VAR, "*fuck* *shit* *suck* *dick* *penis* *cunt* *haha* *fake* *split* *ass* *hehe* *bogus* *yawn* *leet* *blow* *screw* *dumb* *fbi*");

	set_string_var (SERVER_PROMPT_VAR, "[1;32m[%s][0m");

	set_string_var (OPER_MODES_VAR, DEFAULT_OPERMODE);

	set_lastlog_size (curr_scr_win, NULL, irc_variable[LASTLOG_VAR].integer);
	set_history_size (curr_scr_win, NULL, irc_variable[HISTORY_VAR].integer);

	set_highlight_char (curr_scr_win, irc_variable[HIGHLIGHT_CHAR_VAR].string, 0);
	set_lastlog_level (curr_scr_win, irc_variable[LASTLOG_LEVEL_VAR].string, 0);
	set_notify_level (curr_scr_win, irc_variable[NOTIFY_LEVEL_VAR].string, 0);
	set_msglog_level (curr_scr_win, irc_variable[MSGLOG_LEVEL_VAR].string, 0);

	set_input_prompt (curr_scr_win, DEFAULT_INPUT_PROMPT, 0);
	build_status (curr_scr_win, NULL, 0);
	window_display = old_display;
}


/*
 * set_var_value: Given the variable structure and the string representation
 * of the value, this sets the value in the most verbose and error checking
 * of manors.  It displays the results of the set and executes the function
 * defined in the var structure 
 */

void 
set_var_value (int var_index, char *value)
{
	char *rest;
	IrcVariable *var;
	int old;

	var = &(irc_variable[var_index]);
	switch (var->type)
	{
	case BOOL_TYPE_VAR:
		if (value && *value && (value = next_arg (value, &rest)))
		{
			old = var->integer;
			if (do_boolean (value, &(var->integer)))
			{
				say ("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (old != var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (var->func)
				(var->func) (curr_scr_win, NULL, var->integer);
			say ("Value of %s set to %s", var->name,
			     var->integer ? var_settings[ON]
			     : var_settings[OFF]);
		}
		else
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_SET_FSET), "%s %s", var->name, var->integer ? var_settings[ON] : var_settings[OFF]));
		break;
	case CHAR_TYPE_VAR:
		if (!value)
		{
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			var->integer = ' ';
			if (var->func)
				(var->func) (curr_scr_win, NULL, var->integer);
			say ("Value of %s set to '%c'", var->name, var->integer);
		}

		else if (value && *value && (value = next_arg (value, &rest)))
		{
			if (strlen (value) > 1)
				say ("Value of %s must be a single character",
				     var->name);
			else
			{
				if (!(var->int_flags & VIF_CHANGED))
				{
					if (var->integer != *value)
						var->int_flags |= VIF_CHANGED;
				}
				var->integer = *value;
				if (var->func)
					(var->func) (curr_scr_win, NULL, var->integer);
				say ("Value of %s set to '%c'", var->name,
				     var->integer);
			}
		}
		else
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_SET_FSET), "%s %c", var->name, var->integer));
		break;
	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg (value, &rest)))
		{
			int val;

			if (!is_number (value))
			{
				say ("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = my_atol (value)) < 0)
			{
				say ("Value of %s must be greater than 0", var->name);
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer != val)
					var->int_flags |= VIF_CHANGED;
			}
			var->integer = val;
			if (var->func)
				(var->func) (curr_scr_win, NULL, var->integer);
			say ("Value of %s set to %d", var->name, var->integer);
		}
		else
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_SET_FSET), "%s %d", var->name, var->integer));
		break;
	case STR_TYPE_VAR:
		if (value)
		{
			if (*value)
			{
				char *temp = NULL;

				if (var->flags & VF_EXPAND_PATH)
				{
					temp = expand_twiddle (value);
					if (temp)
						value = temp;
					else
						say ("SET: no such user");
				}
				if ((!var->int_flags & VIF_CHANGED))
				{
					if ((var->string && !value) ||
					    (!var->string && value) ||
					    my_stricmp (var->string, value))
						var->int_flags |= VIF_CHANGED;
				}
				malloc_strcpy (&(var->string), value);
				if (temp)
					xfree (&temp);
			}
			else
			{
				put_it ("%s", convert_output_format (get_fset_var (var->string ? FORMAT_SET_FSET : FORMAT_SET_NOVALUE_FSET), "%s %s", var->name, var->string));
				return;
			}
		}
		else
			xfree (&(var->string));
		if (var->func && !(var->int_flags & VIF_PENDING))
		{
			var->int_flags |= VIF_PENDING;
			(var->func) (curr_scr_win, var->string, 0);
			var->int_flags &= ~VIF_PENDING;
		}
		say ("Value of %s set to %s", var->name, var->string ?
		     var->string : "<EMPTY>");
		break;
	}

}


/*
 * set_variable: The SET command sets one of the irc variables.  The args
 * should consist of "variable-name setting", where variable name can be
 * partial, but non-ambbiguous, and setting depends on the variable being set 
 */
void
cmd_set (struct command *cmd, char *args)
{
	char *var;
	int cnt = 0;
	int hook = 0;

	enum VAR_TYPES var_index;

	if ((var = next_arg (args, &args)) != NULL)
	{
		if (!my_strnicmp (var, "FORMAT", 2) || !my_strnicmp (var, "-FORMAT", 3))
		{
			char *t = NULL;
			malloc_sprintf (&t, "%s%s%s", var, args && *args ? " " : empty_string, args && *args ? args : empty_string);
			t_parse_command ("FSET", t);
			xfree (&t);
			return;
		}
		if (*var == '-')
		{
			var++;
			args = NULL;
		}
		upper (var);
		bsearch_array(irc_variable, sizeof (IrcVariable), NUMBER_OF_VARIABLES, var, &cnt, (int *) &var_index);

		if (cnt == 1)
			cnt = -1;
		if ((cnt >= 0) || (!(irc_variable[var_index].int_flags & VIF_PENDING)))
			hook = 1;
		if (cnt < 0)
		{
			irc_variable[var_index].int_flags |= VIF_PENDING;
		}

		if (hook)
			hook = do_hook (SET_LIST, "%s %s", var, args ? args : empty_string);
		else
			hook = 1;
		if (cnt < 0)
		{
			irc_variable[var_index].int_flags &= ~VIF_PENDING;
		}
		if (hook)
		{
			if (cnt < 0)
				set_var_value (var_index, args);
			else if (cnt == 0)
				say ("No such variable \"%s\"", var);
			else
			{
				say ("%s is ambiguous", var);
				for (cnt += var_index; var_index < cnt; var_index++)
					set_var_value (var_index, empty_string);
			}
		}
	}
	else
	{
		for (var_index = 0; var_index < NUMBER_OF_VARIABLES; var_index++)
			set_var_value (var_index, empty_string);
	}
}

/*
 * save_variables: this writes all of the IRCII variables to the given FILE
 * pointer in such a way that they can be loaded in using LOAD or the -l switch 
 */
void 
save_variables (FILE * fp, int do_all)
{
	IrcVariable *var;
	int count = 0;

	for (var = irc_variable; var->name; var++)
	{
		if (!(var->int_flags & VIF_CHANGED))
			continue;
		if ((do_all == 1) || !(var->int_flags & VIF_GLOBAL))
		{
			if (strcmp (var->name, "DISPLAY") == 0 || strcmp (var->name, "CLIENT_INFORMATION") == 0)
				continue;
			count++;
			fprintf (fp, "SET ");
			switch (var->type)
			{
			case BOOL_TYPE_VAR:
				fprintf (fp, "%s %s\n", var->name, var->integer ?
				      var_settings[ON] : var_settings[OFF]);
				break;
			case CHAR_TYPE_VAR:
				fprintf (fp, "%s %c\n", var->name, var->integer);
				break;
			case INT_TYPE_VAR:
				fprintf (fp, "%s %u\n", var->name, var->integer);
				break;
			case STR_TYPE_VAR:
				if (var->string)
					fprintf (fp, "%s %s\n", var->name,
						 var->string);
				else
					fprintf (fp, "-%s\n", var->name);
				break;
			}
		}
	}
	bitchsay ("Saved %d variables", count);
}

char *
make_string_var (char *var_name)
{
	int cnt, msv_index;
	char *ret = NULL;

	upper (var_name);
	if ((bsearch_array (irc_variable, sizeof (IrcVariable), NUMBER_OF_VARIABLES, var_name, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	switch (irc_variable[msv_index].type)
	{
	case STR_TYPE_VAR:
		ret = m_strdup (irc_variable[msv_index].string);
		break;
	case INT_TYPE_VAR:
		ret = m_strdup (ltoa (irc_variable[msv_index].integer));
		break;
	case BOOL_TYPE_VAR:
		ret = m_strdup (var_settings[irc_variable[msv_index].integer]);
		break;
	case CHAR_TYPE_VAR:
		ret = m_dupchar (irc_variable[msv_index].integer);
		break;
	}
	return ret;
}

/* exec_warning: a warning message displayed whenever EXEC_PROTECTION is turned off.  */
static void 
exec_warning (Window * win, char *unused, int value)
{
	if (value == OFF)
	{
		bitchsay ("Warning!  You have turned EXEC_PROTECTION off");
		bitchsay ("Please read the /HELP SET EXEC_PROTECTION documentation");
	}
}

static void 
input_warning (Window * win, char *unused, int value)
{
	if (value == OFF)
	{
		bitchsay ("Warning!  You have turned INPUT_PROTECTION off");
		bitchsay ("Please read the /HELP ON INPUT, and /HELP SET INPUT_PROTECTION documentation");
	}
}

/* returns the size of the character set */
int 
charset_size (void)
{
	return get_int_var (EIGHT_BIT_CHARACTERS_VAR) ? 256 : 128;
}

static void 
eight_bit_characters (Window * win, char *unused, int value)
{
	if (value == ON && !term_eight_bit ())
		say ("Warning!  Your terminal says it does not support eight bit characters");
	set_term_eight_bit (value);
}

static void 
set_realname (Window * win, char *value, int unused)
{
	if (value)
		strmcpy (realname, value, REALNAME_LEN);
	else
		strmcpy (realname, empty_string, REALNAME_LEN);
}

static void 
set_numeric_string (Window * win, char *value, int unused)
{
	malloc_strcpy (&line_thing, value ? value : three_stars);
}

static void 
set_user_mode (Window * win, char *value, int unused)
{
	malloc_strcpy (&send_umode, value);
}

void 
clear_sets (void)
{
	int i = 0;
	for (i = 0; irc_variable[i].name; i++)
		xfree (&irc_variable->string);
}

static void 
reinit_screen (Window * win, char *unused, int value)
{
	set_input_prompt (curr_scr_win, NULL, 0);
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
	update_all_windows ();
	update_all_status (curr_scr_win, NULL, 0);
	update_input (UPDATE_ALL);
}

static void 
reinit_status (Window * win, char *unused, int value)
{
	update_all_windows ();
	update_all_status (curr_scr_win, NULL, 0);
}

