#ident "@(#)input.c 1.11"
/*
 * input.c: does the actual input line stuff... keeps the appropriate stuff
 * on the input line, handles insert/delete of characters/words... the whole
 * ball o wax 
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

#include "commands.h"
#include "exec.h"
#include "history.h"
#include "hook.h"
#include "input.h"
#include "ircaux.h"
#include "keys.h"
#include "screen.h"
#include "server.h"
#include "ircterm.h"
#include "list.h"
#include "vars.h"
#include "misc.h"
#include "screen.h"
#include "output.h"
#include "dcc.h"
#include "whowas.h"
#include "window.h"
#include "status.h"
#include "hash2.h"
#include "tcommand.h"
#include "expr.h"
#include "util.h"

#include "xformats.h"
#include "xmalloc.h"


#include <sys/ioctl.h>

void get_history (int);

static char new_nick[NICKNAME_LEN + 1] = "";
static char *input_lastmsg = NULL;

extern NickTab *getnextnick (char *, char *, char *);
extern int extended_handled;
extern char *getchannick (char *, char *);
extern char *last_split_server;

NickTab *tabkey_array = NULL;



const int WIDTH = 10;

/* input_prompt: contains the current, unexpanded input prompt */
static char *input_prompt = NULL;
static int old_ansi = 0;

/* input_line: the actual screen line where the input goes */
static int input_line;

/* str_start: position in buffer of first visible character in the input line */
static int str_start = 0;

enum I_STATE
{
	STATE_NORMAL = 0,
	STATE_COMPLETE,
	STATE_TABKEY,
	STATE_TABKEYNEXT,
	STATE_CNICK,
	STATE_CNICKNEXT
}
in_completion = STATE_NORMAL;

/* These are sanity macros.  The file was completely unreadable before 
 * i put these in here.  I make no apologies for them.
 */
#define INPUT_BUFFER current_screen->input_buffer
#define MIN_POS current_screen->buffer_min_pos
#define THIS_POS current_screen->buffer_pos
#define THIS_CHAR INPUT_BUFFER[THIS_POS]
#define MIN_CHAR INPUT_BUFFER[MIN_POS]
#define PREV_CHAR INPUT_BUFFER[THIS_POS-1]
#define NEXT_CHAR INPUT_BUFFER[THIS_POS+1]
#define ADD_TO_INPUT(x) strmcat(INPUT_BUFFER, (x), INPUT_BUFFER_SIZE-1);
#define INPUT_VISIBLE INPUT_BUFFER[str_start]

/*
 * upper_mark and lower_mark: marks the upper and lower positions in the
 * input buffer which will cause the input display to switch to the next or
 * previous bunch of text 
 */
static int lower_mark;
static int upper_mark;

/* zone: the amount of editable visible text on the screen */
static int zone;

/* cursor: position of the cursor in the input line on the screen */
static int cursor = 0;


static int 
safe_puts (char *str, int len)
{
	int i = 0;
	while (*str && i < len)
		term_putchar (*str++), i++;
	return i;
}

/* cursor_to_input: move the cursor to the input line, if not there already */
extern void 
cursor_to_input (void)
{
	Screen *old_current_screen;


	old_current_screen = current_screen;
	for (current_screen = screen_list; current_screen; current_screen = current_screen->next)
	{
		if (current_screen->alive && is_cursor_in_display (NULL))
		{
			term_move_cursor (cursor, input_line);
			cursor_not_in_display ();
			term_flush ();
		}
	}
	set_current_screen (old_current_screen);
}

/*
 * update_input: does varying amount of updating on the input line depending
 * upon the position of the cursor and the update flag.  If the cursor has
 * move toward one of the edge boundaries on the screen, update_cursor()
 * flips the input line to the next (previous) line of text. The update flag
 * may be: 
 *
 * NO_UPDATE - only do the above bounds checking. 
 *
 * UPDATE_JUST_CURSOR - do bounds checking and position cursor where is should
 * be. 
 *
 * UPDATE_FROM_CURSOR - does all of the above, and makes sure everything from
 * the cursor to the right edge of the screen is current (by redrawing it). 
 *
 * UPDATE_ALL - redraws the entire line 
 */
extern void 
update_input (int update)
{
	int old_start;
	static int co = 0, li = 0;
	char *ptr;
	int len, free_it = 1, cnt, ansi_count, max;

	char *prompt;

	cursor_to_input ();

	if (current_screen->promptlist)
		prompt = current_screen->promptlist->prompt;
	else
		prompt = input_prompt;
	if (prompt)
	{
		if (update != NO_UPDATE)
		{
			char *inp_ptr = NULL;
			int args_used;

			if (is_process (get_target_by_refnum (0)))
			{
				ptr = (char *) get_prompt_by_refnum (0);
				free_it = 0;
			}
			else if (!get_int_var (DISPLAY_ANSI_VAR))
				ptr = expand_alias (stripansicodes (prompt), empty_string, &args_used, NULL);
			else
				ptr = expand_alias (prompt, empty_string, &args_used, NULL);
			if (*ptr && ((my_strnicmp (ptr, "Password:", 9) == 0) || (my_strnicmp (ptr, "Operator Password:", 18) == 0) ||
			  (my_strnicmp (ptr, "Server Password:", 16) == 0)))
				term_echo (0);
			else
				term_echo (1);
			len = strlen (ptr);
			if (strncmp (ptr, current_screen->input_buffer, len) || !len)
			{
				malloc_strcpy (&inp_ptr, INPUT_BUFFER + MIN_POS);
				strmcpy (INPUT_BUFFER, ptr, INPUT_BUFFER_SIZE);
				THIS_POS += (len - MIN_POS);
				MIN_POS = strlen (ptr);
				ADD_TO_INPUT (inp_ptr);
				xfree (&inp_ptr);
				update = UPDATE_ALL;
			}

			if (free_it)
				xfree (&ptr);
		}
	}
	else
		term_echo (1);

	if ((li != LI) || (co != CO))
	{
		/* resized?  Keep it simple and reset everything */
		input_line = LI - 1;
		zone = CO - (WIDTH * 2) + 4;
		lower_mark = WIDTH;
		upper_mark = CO - WIDTH;
		cursor = current_screen->buffer_min_pos;
		current_screen->buffer_pos = current_screen->buffer_min_pos;
		str_start = 0;
		li = LI;
		co = CO;
	}
	old_start = str_start;
	ansi_count = count_ansi (current_screen->input_buffer, zone);
	if (old_ansi != ansi_count || current_screen->buffer_pos - ansi_count > zone)
	{
		lower_mark = WIDTH;
		upper_mark = CO - WIDTH;
		str_start = 0;
	}
	ansi_count = count_ansi (&(current_screen->input_buffer[str_start]), zone);

	while ((current_screen->buffer_pos - ansi_count < lower_mark) && lower_mark > WIDTH)
	{
		upper_mark = lower_mark - ansi_count;
		lower_mark -= (zone + ansi_count);
		str_start -= (zone + ansi_count);
		if (str_start < zone)
		{
			str_start = 0;
			ansi_count = count_ansi (&(current_screen->input_buffer[str_start]), zone);
			lower_mark -= ansi_count;
			upper_mark -= ansi_count;
		}
	}
	while (current_screen->buffer_pos - ansi_count >= upper_mark)
	{
		lower_mark = upper_mark + ansi_count;
		upper_mark += zone + ansi_count;
		str_start += zone + ansi_count;
		if (ansi_count)
			ansi_count = 0;
	}

	/* we need to count ansi characters again, this time in the part of
	   the string we are gonna display in a few moments */
	ansi_count = count_ansi (&(current_screen->input_buffer[str_start]), zone);
	old_ansi = count_ansi (current_screen->input_buffer, zone);
	/* we need to substract number of ansi characters from cursor position
	   since those are not visible, otherwise we'd display cursor in
	   wrong place
	 */
	cursor = current_screen->buffer_pos - str_start - ansi_count;
	if ((old_start != str_start) || (update == UPDATE_ALL))
	{
		term_move_cursor (0, input_line);
		if ((str_start == 0) && (MIN_POS > 0))
		{
			int echo;

			echo = term_echo (1);
			if (MIN_POS > (CO - WIDTH))
				len = CO - WIDTH - 1 /* + ansi_count */ ;
			else
				len = MIN_POS;
			cnt = /*term_puts */ safe_puts (&(INPUT_BUFFER[str_start]), len);
			term_echo (echo);
			cnt += /*term_puts */ safe_puts (&(current_screen->input_buffer[
				  str_start + len]), CO - len + ansi_count);
		}
		else
			cnt = /*term_puts */ safe_puts (&(INPUT_BUFFER[str_start]), CO);
		term_clear_to_eol ();
		term_move_cursor (cursor, input_line);
	}
	else if (update == UPDATE_FROM_CURSOR)
	{
		term_move_cursor (cursor, input_line);
		cnt = cursor;
		max = CO - (current_screen->buffer_pos - str_start) + ansi_count;
		if ((len = strlen (&(THIS_CHAR))) > max)
			len = max;
		cnt += /*term_puts */ safe_puts (&(THIS_CHAR), len);
		term_clear_to_eol ();
		term_move_cursor (cursor, input_line);
	}
	else if (update == UPDATE_JUST_CURSOR)
		term_move_cursor (cursor, input_line);
	term_flush ();
}

extern void 
change_input_prompt (int direction)
{
	if (!current_screen->promptlist)
	{
		strcpy (INPUT_BUFFER, current_screen->saved_input_buffer);
		THIS_POS = current_screen->saved_buffer_pos;
		MIN_POS = current_screen->saved_min_buffer_pos;
		*current_screen->saved_input_buffer = '\0';
		current_screen->saved_buffer_pos = 0;
		current_screen->saved_min_buffer_pos = 0;
		update_input (UPDATE_ALL);
	}

	else if (direction == -1)
		update_input (UPDATE_ALL);

	else if (!current_screen->promptlist->next)
	{
		strcpy (current_screen->saved_input_buffer, INPUT_BUFFER);
		current_screen->saved_buffer_pos = THIS_POS;
		current_screen->saved_min_buffer_pos = MIN_POS;
		*INPUT_BUFFER = '\0';
		THIS_POS = MIN_POS = 0;
		update_input (UPDATE_ALL);
	}
}

/* input_move_cursor: moves the cursor left or right... got it? */
extern void 
input_move_cursor (int dir)
{
	cursor_to_input ();
	if (dir)
	{
		if (THIS_CHAR)
		{
			THIS_POS++;
			term_cursor_right ();
		}
	}
	else
	{
		if (THIS_POS > MIN_POS)
		{
			THIS_POS--;
			term_cursor_left ();
		}
	}
	update_input (NO_UPDATE);
}

/*
 * set_input: sets the input buffer to the given string, discarding whatever
 * was in the input buffer before 
 */
void 
set_input (char *str)
{
	strmcpy (INPUT_BUFFER + MIN_POS, str, INPUT_BUFFER_SIZE - MIN_POS);
	THIS_POS = strlen (INPUT_BUFFER);
}

/*
 * get_input: returns a pointer to the input buffer.  Changing this will
 * actually change the input buffer.  This is a bad way to change the input
 * buffer tho, cause no bounds checking won't be done 
 */
char *
get_input (void)
{
	return (&(MIN_CHAR));
}

/* init_input: initialized the input buffer by clearing it out */
extern void 
init_input (void)
{
	*INPUT_BUFFER = (char) 0;
	THIS_POS = MIN_POS;
}

/* get_input_prompt: returns the current input_prompt */
extern char *
get_input_prompt (void)
{
	return (input_prompt);
}

/*
 * set_input_prompt: sets a prompt that will be displayed in the input
 * buffer.  This prompt cannot be backspaced over, etc.  It's a prompt.
 * Setting the prompt to null uses no prompt 
 */
void 
set_input_prompt (Window * win, char *prompt, int unused)
{
	if (prompt)
	{
		if (input_prompt && !strcmp (prompt, input_prompt))
			return;
		malloc_strcpy (&input_prompt, convert_output_format (prompt, NULL, NULL));
	}
	else
	{
		if (!input_prompt)
			return;
		malloc_strcpy (&input_prompt, empty_string);
	}
	update_input (UPDATE_ALL);
}


/* 
 * Why did i put these in this file?  I dunno.  But i do know that the ones 
 * in edit.c didnt have to be here, and i knew the ones that were here DID 
 * have to be here, so i just moved them all to here, so that they would all
 * be in the same place.  Easy enough. (jfn, june 1995)
 */

/*
 * input_forward_word: move the input cursor forward one word in the input
 * line 
 */
extern void 
input_forward_word (char unused, char *not_used)
{
	cursor_to_input ();

	while ((my_isspace (THIS_CHAR) || ispunct (THIS_CHAR)) && (THIS_CHAR))
		THIS_POS++;
	while (!(ispunct (THIS_CHAR) || my_isspace (THIS_CHAR)) && (THIS_CHAR))
		THIS_POS++;
	update_input (UPDATE_JUST_CURSOR);
}

/* input_backward_word: move the cursor left on word in the input line */
extern void 
input_backward_word (char unused, char *not_used)
{
	cursor_to_input ();
	while ((THIS_POS > MIN_POS) && (my_isspace (PREV_CHAR) || ispunct (PREV_CHAR)))
		THIS_POS--;
	while ((THIS_POS > MIN_POS) && !(ispunct (PREV_CHAR) || my_isspace (PREV_CHAR)))
		THIS_POS--;

	update_input (UPDATE_JUST_CURSOR);
}

/* input_delete_character: deletes a character from the input line */
extern void 
input_delete_character (char unused, char *not_used)
{
	cursor_to_input ();
	if (THIS_CHAR)
	{
		char *ptr = NULL;
		int pos;

		malloc_strcpy (&ptr, &(NEXT_CHAR));
		strcpy (&(THIS_CHAR), ptr);
		xfree (&ptr);
		if (term_delete ())
			update_input (UPDATE_FROM_CURSOR);
		else
		{
			pos = str_start + CO - 1;
			pos += count_ansi (&(current_screen->input_buffer[str_start]), zone);
			if (pos < strlen (INPUT_BUFFER))
			{
				term_move_cursor (CO - 1, input_line);
				term_putchar (INPUT_BUFFER[pos]);
				term_move_cursor (cursor, input_line);
			}
			update_input (NO_UPDATE);
		}
	}
	in_completion = STATE_NORMAL;
}

/* input_backspace: does a backspace in the input buffer */
extern void 
input_backspace (char key, char *blah)
{
	cursor_to_input ();
	if (THIS_POS > MIN_POS)
	{
		char *ptr = NULL;
		int pos;


		malloc_strcpy (&ptr, &(THIS_CHAR));
		strcpy (&(PREV_CHAR), ptr);
		xfree (&ptr);
		THIS_POS--;
		term_cursor_left ();
		if (THIS_CHAR)
		{
			if (term_delete ())
				update_input (UPDATE_FROM_CURSOR);
			{
				pos = str_start + CO - 1;
				pos += count_ansi (&(current_screen->input_buffer[str_start]), zone);
				if (pos < strlen (INPUT_BUFFER))
				{
					term_move_cursor (CO - 1, input_line);
					term_putchar (INPUT_BUFFER[pos]);
				}
				update_input (UPDATE_JUST_CURSOR);
			}
		}
		else
		{
			term_putchar (' ');
			term_cursor_left ();
			update_input (NO_UPDATE);
		}
	}
	in_completion = STATE_NORMAL;
	*new_nick = 0;
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
extern void 
input_beginning_of_line (char unused, char *not_used)
{
	cursor_to_input ();
	THIS_POS = MIN_POS;
	update_input (UPDATE_JUST_CURSOR);
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
extern void 
new_input_beginning_of_line (char unused, char *not_used)
{
	cursor_to_input ();
	THIS_POS = MIN_POS;
	update_input (UPDATE_JUST_CURSOR);
	extended_handled = 1;
}

/*
 * input_end_of_line: moves the input cursor to the last character in the
 * input buffer 
 */
extern void 
input_end_of_line (char unused, char *not_used)
{
	cursor_to_input ();
	THIS_POS = strlen (INPUT_BUFFER);
	update_input (UPDATE_JUST_CURSOR);
}

extern void 
input_delete_to_previous_space (char key, char *blah)
{
	int old_pos;
	char c;

	cursor_to_input ();
	old_pos = THIS_POS;
	c = THIS_CHAR;

	while (!my_isspace (THIS_CHAR) && THIS_POS >= MIN_POS)
		THIS_POS--;

	if (THIS_POS < old_pos)
	{
		strcpy (&(NEXT_CHAR), &(INPUT_BUFFER[old_pos]));
		THIS_POS++;
	}

	update_input (UPDATE_FROM_CURSOR);
}


/*
 * input_delete_previous_word: deletes from the cursor backwards to the next
 * space character. 
 */
extern void 
input_delete_previous_word (char unused, char *not_used)
{
	int old_pos;
	char c;

	cursor_to_input ();
	old_pos = THIS_POS;
	while ((THIS_POS > MIN_POS) && (my_isspace (PREV_CHAR) || ispunct (PREV_CHAR)))
		THIS_POS--;
	while ((THIS_POS > MIN_POS) && !(ispunct (PREV_CHAR) || my_isspace (PREV_CHAR)))
		THIS_POS--;
	c = INPUT_BUFFER[old_pos];
	INPUT_BUFFER[old_pos] = (char) 0;
	malloc_strcpy (&cut_buffer, &THIS_CHAR);
	INPUT_BUFFER[old_pos] = c;
	strcpy (&(THIS_CHAR), &(INPUT_BUFFER[old_pos]));
	update_input (UPDATE_FROM_CURSOR);
}

/*
 * input_delete_next_word: deletes from the cursor to the end of the next
 * word 
 */
extern void 
input_delete_next_word (char unused, char *not_used)
{
	int pos;
	char *ptr = NULL, c;

	cursor_to_input ();
	pos = THIS_POS;
	while ((my_isspace (INPUT_BUFFER[pos]) || ispunct (INPUT_BUFFER[pos])) && INPUT_BUFFER[pos])
		pos++;
	while (!(ispunct (INPUT_BUFFER[pos]) || my_isspace (INPUT_BUFFER[pos])) && INPUT_BUFFER[pos])
		pos++;
	c = INPUT_BUFFER[pos];
	INPUT_BUFFER[pos] = (char) 0;
	malloc_strcpy (&cut_buffer, &(THIS_CHAR));
	INPUT_BUFFER[pos] = c;
	malloc_strcpy (&ptr, &(INPUT_BUFFER[pos]));
	strcpy (&(THIS_CHAR), ptr);
	xfree (&ptr);
	update_input (UPDATE_FROM_CURSOR);
}

/*
 * input_add_character: adds the character c to the input buffer, repecting
 * the current overwrite/insert mode status, etc 
 */
extern void 
input_add_character (char c, char *unused)
{
	int display_flag = NO_UPDATE;

	cursor_to_input ();
	if (THIS_POS < INPUT_BUFFER_SIZE)
	{
		if (get_int_var (INSERT_MODE_VAR))
		{
			if (THIS_CHAR)
			{
				char *ptr = NULL;

				ptr = alloca (strlen (&(THIS_CHAR)));
				strcpy (ptr, &(THIS_CHAR));

				THIS_CHAR = c;
				NEXT_CHAR = 0;
				ADD_TO_INPUT (ptr);
				if (term_insert (c))
				{
					term_putchar (c);
					if (NEXT_CHAR)
						display_flag = UPDATE_FROM_CURSOR;
					else
						display_flag = NO_UPDATE;
				}
			}
			else
			{
				THIS_CHAR = c;
				NEXT_CHAR = 0;
				term_putchar (c);
			}
		}
		else
		{
			if (THIS_CHAR == 0)
				NEXT_CHAR = 0;
			THIS_CHAR = c;
			term_putchar (c);
		}
		THIS_POS++;
		update_input (display_flag);
	}
	if (in_completion == STATE_COMPLETE && c == ' ' && input_lastmsg)
	{
		xfree (&input_lastmsg);
		*new_nick = 0;
		in_completion = STATE_NORMAL;
	}
}

/* input_clear_to_eol: erases from the cursor to the end of the input buffer */
extern void 
input_clear_to_eol (char unused, char *not_used)
{
	cursor_to_input ();
	malloc_strcpy (&cut_buffer, &(THIS_CHAR));
	THIS_CHAR = 0;
	term_clear_to_eol ();
	update_input (NO_UPDATE);
}

/*
 * input_clear_to_bol: clears from the cursor to the beginning of the input
 * buffer 
 */
extern void 
input_clear_to_bol (char unused, char *not_used)
{
	char *ptr = NULL;
	cursor_to_input ();
	malloc_strcpy (&cut_buffer, &(MIN_CHAR));
	cut_buffer[THIS_POS - MIN_POS] = (char) 0;
	malloc_strcpy (&ptr, &(THIS_CHAR));
	MIN_CHAR = (char) 0;
	ADD_TO_INPUT (ptr);
	xfree (&ptr);
	THIS_POS = MIN_POS;
	term_move_cursor (MIN_POS, input_line);
	term_clear_to_eol ();
	update_input (UPDATE_FROM_CURSOR);
}

/*
 * input_clear_line: clears entire input line
 */
extern void 
input_clear_line (char unused, char *not_used)
{
	cursor_to_input ();
	malloc_strcpy (&cut_buffer, INPUT_BUFFER + MIN_POS);
	MIN_CHAR = (char) 0;
	THIS_POS = MIN_POS;
	term_move_cursor (current_screen->buffer_min_pos -
	       count_ansi (current_screen->input_buffer, zone), input_line);
	term_clear_to_eol ();
	update_input (NO_UPDATE);
}

/*
 * input_transpose_characters: swaps the positions of the two characters
 * before the cursor position 
 */
extern void 
input_transpose_characters (char unused, char *not_used)
{
	cursor_to_input ();
	if (current_screen->buffer_pos > MIN_POS)
	{
		u_char c1[3] =
		{0};
		int pos, end_of_line = 0;

		if (THIS_CHAR)
			pos = THIS_POS;
		else if (strlen (get_input ()) > MIN_POS + 2)
		{
			pos = THIS_CHAR - 1;
			end_of_line = 1;
		}
		else
			return;

		c1[0] = INPUT_BUFFER[pos];
		c1[1] = INPUT_BUFFER[pos] = INPUT_BUFFER[pos - 1];
		INPUT_BUFFER[pos - 1] = c1[0];
		term_cursor_left ();
		if (end_of_line)
			term_cursor_left ();

		term_putchar (c1[0]);
		term_putchar (c1[1]);
		if (!end_of_line)
			term_cursor_left ();
		update_input (NO_UPDATE);
	}
}


extern void 
refresh_inputline (char unused, char *not_used)
{
	update_input (UPDATE_ALL);
}

/*
 * input_yank_cut_buffer: takes the contents of the cut buffer and inserts it
 * into the input line 
 */
extern void 
input_yank_cut_buffer (char unused, char *not_used)
{
	char *ptr = NULL;

	if (cut_buffer)
	{
		malloc_strcpy (&ptr, &(THIS_CHAR));
		/* Ooops... */
		THIS_CHAR = 0;
		ADD_TO_INPUT (cut_buffer);
		ADD_TO_INPUT (ptr);
		xfree (&ptr);
		update_input (UPDATE_FROM_CURSOR);
		THIS_POS += strlen (cut_buffer);
		if (THIS_POS > INPUT_BUFFER_SIZE)
			THIS_POS = INPUT_BUFFER_SIZE;
		update_input (UPDATE_JUST_CURSOR);
	}
}


/* used with input_move_cursor */
#define RIGHT 1
#define LEFT 0

/* BIND functions: */
extern void 
forward_character (char dumb, char *dumber)
{
	input_move_cursor (RIGHT);
}

extern void 
backward_character (char dumb, char *dumber)
{
	input_move_cursor (LEFT);
}

extern void 
backward_history (char dumb, char *dumber)
{
	get_history (PREV);
}

extern void 
forward_history (char dumb, char *dumber)
{
	get_history (NEXT);
}

extern void 
toggle_insert_mode (char dumb, char *dumber)
{
	int tog = get_int_var (INSERT_MODE_VAR);
	tog ^= 1;
	set_int_var (INSERT_MODE_VAR, tog);
}


extern void 
input_msgreply (char dumb, char *dumber)
{
	char *cmdchar;
	char *line, *cmd, *t;
	char *snick;
	NickTab *nick = NULL;
	int got_space = 0;

	if (!(cmdchar = get_string_var (CMDCHARS_VAR)))
		cmdchar = DEFAULT_CMDCHARS;

	t = line = m_strdup (get_input ());
	if (t)
		got_space = strchr (t, ' ') ? 1 : 0;
	cmd = next_arg (line, &line);
	snick = next_arg (line, &line);
	if ((cmd && *cmd == *cmdchar && got_space) || !cmd)
	{

		if (cmd && *cmd == *cmdchar)
			cmd++;
		if (in_completion == STATE_NORMAL && snick)
			strncpy (new_nick, snick, sizeof (new_nick) - 1);

		if ((nick = getnextnick (new_nick, input_lastmsg, snick)))
		{
			if (nick->nick && *(nick->nick))
			{
				snick = nick->nick;
				malloc_strcpy (&input_lastmsg, nick->nick);
			}
		}
		if (nick)
		{
			char *tmp = NULL;
			input_clear_line ('\0', NULL);
			if (get_format (FORMAT_NICK_MSG_FSET))
				malloc_strcpy (&tmp, stripansicodes (convert_output_format (get_format (FORMAT_NICK_MSG_FSET), "%s%s %s %s", cmdchar, nick->type ? nick->type : cmd ? cmd : "msg", nick->nick, line ? line : empty_string)));
			else
				malloc_sprintf (&tmp, "%s%s %s %s", cmdchar, nick->type ? nick->type : cmd ? cmd : "msg", nick->nick, line ? line : empty_string);
			set_input (tmp);
			xfree (&tmp);
		}
		else
			command_completion (0, NULL);
	}
	else
		command_completion (0, NULL);
	update_input (UPDATE_ALL);
	xfree (&t);
}

#if 0 /* somebody please remove me if i am not needed XXX */
static void 
add_autonick_input (char *nick, char *line)
{
	char *tmp1 = NULL;
	input_clear_line ('\0', NULL);
	if ((do_hook (AR_REPLY_LIST, "%s", nick)))
	{
		if (get_format (FORMAT_NICK_AUTO_FSET))
			malloc_strcpy (&tmp1, stripansicodes (convert_output_format (get_format (FORMAT_NICK_AUTO_FSET), "%s %s", nick, line ? line : empty_string)));
		else
			malloc_sprintf (&tmp1, "%s: %s", nick, line);
		set_input (tmp1);
		xfree (&tmp1);
	}
	update_input (UPDATE_ALL);
}
#endif

extern void 
send_line (char dumb, char *dumber)
{
	int server;
	WaitPrompt *OldPrompt;

	server = from_server;
	from_server = get_window_server (0);
	unhold_a_window (curr_scr_win);
	if (current_screen->promptlist && current_screen->promptlist->type == WAIT_PROMPT_LINE)
	{
		OldPrompt = current_screen->promptlist;
		(*OldPrompt->func) (OldPrompt->data, get_input ());
		set_input (empty_string);
		current_screen->promptlist = OldPrompt->next;
		xfree (&OldPrompt->data);
		xfree (&OldPrompt->prompt);
		xfree ((char **) &OldPrompt);
		change_input_prompt (-1);
	}
	else
	{
		char *line, *tmp = NULL;

		line = get_input ();
		if (line && (*line != get_int_var (CMDCHARS_VAR)) && get_int_var (NICK_COMPLETION_VAR))
		{
			char auto_comp_char = ':';
			if (!(auto_comp_char = (char) get_int_var (NICK_COMPLETION_CHAR_VAR)))
				auto_comp_char = ':';

			/* possible nick completion */
			if (strchr (line, auto_comp_char))
			{
				char *p;
				ChannelList *chan;
				NickList *nick;
				char *channel;
				malloc_strcpy (&tmp, line);
				p = strchr (tmp, auto_comp_char);
				*p++ = 0;
				if (*tmp && *p && (channel = get_current_channel_by_refnum (0)))
				{
					chan = lookup_channel (channel, from_server, 0);
					for (nick = next_nicklist (chan, NULL); nick; nick = next_nicklist (chan, nick))
						if (!my_strnicmp (tmp, nick->nick, strlen (tmp)))
							break;
					if (nick)
					{
						if (get_format (FORMAT_NICK_COMP_FSET))
							malloc_strcpy (&tmp, stripansicodes (convert_output_format (get_format (FORMAT_NICK_COMP_FSET), "%s %s", nick->nick, p)));
						else
							malloc_sprintf (&tmp, "%s%c %s", nick->nick, auto_comp_char, p);
					}
					else
						malloc_strcpy (&tmp, line);
				}
				else
					malloc_strcpy (&tmp, line);
			}
			else
				malloc_strcpy (&tmp, line);
		}
		else
			malloc_strcpy (&tmp, line);
		if (do_hook (INPUT_LIST, "%s", tmp))
		{
			if (get_int_var (INPUT_ALIASES_VAR))
				parse_line (NULL, tmp, empty_string, 1, 0);
			else
				parse_line (NULL, tmp, NULL, 1, 0);
		}
		update_input (UPDATE_ALL);
		xfree (&tmp);
	}
	xfree (&input_lastmsg);
	*new_nick = 0;
	in_completion = STATE_NORMAL;
	from_server = server;
}



extern void 
meta9_char (char dumb, char *dumber)
{
	current_screen->meta_hit[9] = 1;
}

extern void 
meta8_char (char dumb, char *dumber)
{
	current_screen->meta_hit[8] = 1;
}

extern void 
meta7_char (char dumb, char *dumber)
{
	current_screen->meta_hit[7] = 1;
}

extern void 
meta6_char (char dumb, char *dumber)
{
	current_screen->meta_hit[6] = 1;
}

extern void 
meta5_char (char dumb, char *dumber)
{
	current_screen->meta_hit[5] = 1;
}

extern void 
meta4_char (char dumb, char *dumber)
{
	current_screen->meta_hit[4] = 1 - current_screen->meta_hit[4];
}

extern void 
meta3_char (char dumb, char *dumber)
{
	current_screen->meta_hit[3] = 1;
}

extern void 
meta2_char (char dumb, char *dumber)
{
	current_screen->meta_hit[2] = 1;
}

extern void 
meta1_char (char dumb, char *dumber)
{
	current_screen->meta_hit[1] = 1;
}

extern void 
quote_char (char c, char *dumber)
{
	current_screen->quote_hit = 1;
}

/* These four functions are boomerang functions, which allow the highlight
 * characters to be bound by simply having these functions put in the
 * appropriate characters when you press any key to which you have bound
 * that highlight character. >;-)
 */
extern void 
insert_bold (char c, char *dumber)
{
	input_add_character (BOLD_TOG, dumber);
}

extern void 
insert_reverse (char c, char *dumber)
{
	input_add_character (REV_TOG, dumber);
}

extern void 
insert_underline (char c, char *dumber)
{
	input_add_character (UND_TOG, dumber);
}

extern void 
highlight_off (char c, char *dumber)
{
	input_add_character (ALL_OFF, dumber);
}

/* type_text: the BIND function TYPE_TEXT */
extern void 
type_text (char c, char *str)
{
	if (!str)
		return;
	for (; *str; str++)
		input_add_character (*str, empty_string);
}

/*
 * clear_screen: the CLEAR_SCREEN function for BIND.  Clears the screen and
 * starts it if it is held 
 */
extern void 
clear_screen (char c, char *str)
{
	hold_mode (NULL, OFF, 1);
	clear_window_by_refnum (0);
}

/* parse_text: the bindable function that executes its string */
extern void 
parse_text (char c, char *str)
{
	parse_line (NULL, str, empty_string, 0, 0);
}


/*
 * edit_char: handles each character for an input stream.  Not too difficult
 * to work out.
 */
extern void 
edit_char (u_char key)
{
	void (*func) (char, char *) = NULL;
	char *ptr = NULL;
	u_char extended_key;
	WaitPrompt *oldprompt;
	int meta_hit = 0, meta_not_hit;
	int i;


	/* were we waiting for a keypress? */
	if (current_screen->promptlist && current_screen->promptlist->type == WAIT_PROMPT_KEY)
	{
		unsigned char key_[2] = "\0";
		key_[0] = key;
		oldprompt = current_screen->promptlist;

		(*oldprompt->func) (oldprompt->data, key_);

		set_input (empty_string);
		current_screen->promptlist = oldprompt->next;
		xfree (&oldprompt->data);
		xfree (&oldprompt->prompt);
		xfree ((char **) &oldprompt);
		change_input_prompt (-1);
		return;
	}

	extended_key = key;


	/* Check to see if its an eight bit char and if we allow it */
	if (!get_int_var (EIGHT_BIT_CHARACTERS_VAR))
		key &= 0x7f;	/* mask out non-ascii crap */


	/* Check to see if this is a meta-key */
	for (i = 1; i <= 9; i++)
	{
		if (current_screen->meta_hit[i])
		{
			if (keys[i][key])
			{
				func = key_names[keys[i][key]->key_index].func;
				ptr = keys[i][key]->stuff;
			}
			current_screen->meta_hit[i] = 0;
			meta_hit = 1;
			break;
		}
	}
	if (!meta_hit)
	{
		if (keys[0][key])
		{
			func = key_names[keys[0][key]->key_index].func;
			ptr = keys[0][key]->stuff;
		}
	}

	/* is there a meta key that isnt still outstanding? */
	meta_not_hit = 1;
	for (i = 1; i <= 3; i++)
		meta_not_hit = meta_not_hit && !current_screen->meta_hit[i];
	for (i = 5; i <= 9; i++)
		meta_not_hit = meta_not_hit && !current_screen->meta_hit[i];

	if (meta_not_hit)
	{
		/* did we just hit the quote character? */
		if (current_screen->quote_hit)
		{
			current_screen->quote_hit = 0;
			input_add_character (extended_key, empty_string);
		}

		/* nope. none of these.  just a regular character */
		else if (func)
			func (extended_key, ptr ? ptr : empty_string);
	}
	else
		term_beep ();	/* two metas in a row gets a beep */
}

extern void 
my_scrollback (char dumb, char *dumber)
{
	scrollback_backwards (dumb, dumber);
	extended_handled = 1;
}

extern void 
my_scrollforward (char dumb, char *dumber)
{
	scrollback_forwards (dumb, dumber);
	extended_handled = 1;
}

extern void 
my_scrollend (char dumb, char *dumber)
{
	scrollback_end (dumb, dumber);
	extended_handled = 1;
}

extern void 
dcc_plist (char dumb, char *dumber)
{
	dcc_glist (NULL, NULL);
	extended_handled = 1;
}


extern int in_window_command;

static void 
handle_swap (int windownum)
{
	char *p = NULL;

	malloc_sprintf (&p, "SWAP %d", windownum);
	t_parse_command ("WINDOW", p);
	set_channel_window (curr_scr_win, get_current_channel_by_refnum (curr_scr_win->refnum), curr_scr_win->server);
	xfree (&p);
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
	update_input (UPDATE_ALL);
	update_all_windows ();
}

extern void 
window_swap1 (char dumb, char *dumber)
{
	handle_swap (1);
	extended_handled = 1;
}

extern void 
window_swap2 (char dumb, char *dumber)
{
	handle_swap (2);
	extended_handled = 1;
}

extern void 
window_swap3 (char dumb, char *dumber)
{
	handle_swap (3);
	extended_handled = 1;
}

extern void 
window_swap4 (char dumb, char *dumber)
{
	handle_swap (4);
	extended_handled = 1;
}

extern void 
window_swap5 (char dumb, char *dumber)
{
	handle_swap (5);
	extended_handled = 1;
}

extern void 
window_swap6 (char dumb, char *dumber)
{
	handle_swap (6);
	extended_handled = 1;
}

extern void 
window_swap7 (char dumb, char *dumber)
{
	handle_swap (7);
	extended_handled = 1;
}
extern void 
window_swap8 (char dumb, char *dumber)
{
	handle_swap (8);
	extended_handled = 1;
}
extern void 
window_swap9 (char dumb, char *dumber)
{
	handle_swap (9);
	extended_handled = 1;
}
extern void 
window_swap10 (char dumb, char *dumber)
{
	handle_swap (10);
	extended_handled = 1;
}

extern void 
change_to_split (char dumb, char *dumber)
{
	if (!last_split_server)
		return;
	t_parse_command ("SERVER", last_split_server);
}

extern void 
join_last_invite (char dumb, char *dumber)
{
	if (invite_channel)
		send_to_server ("JOIN %s", invite_channel);
	else
		bitchsay ("You haven't been invited to a channel yet");
}

extern void 
wholeft (char dumb, char *dumber)
{
	show_wholeft (NULL);
}

extern void 
window_key_balance (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	recalculate_windows ();
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
window_grow_one (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	resize_window (1, curr_scr_win, 1);
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
window_key_hide (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	hide_window (curr_scr_win);
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
window_key_kill (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	delete_window (curr_scr_win);
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
window_key_list (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	window_list (curr_scr_win, NULL, NULL);
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
window_key_move (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	move_window (curr_scr_win, 1);
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}


extern void 
window_shrink_one (char dumb, char *dumber)
{
	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	resize_window (1, curr_scr_win, -1);
	update_all_windows ();
	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
}

extern void 
ignore_last_nick (char dumb, char *dumber)
{
	NickTab *nick;
	char *tmp1;
	if ((nick = gettabkey (1, NULL)))
	{
		set_input (empty_string);
		tmp1 = m_sprintf ("%sig %s", get_string_var (CMDCHARS_VAR), nick->nick);
		set_input (tmp1);
		xfree (&tmp1);
	}
	update_input (UPDATE_ALL);
}

extern void 
nick_completion (char dumb, char *dumber)
{
	char *q, *line;
	int i = -1;
	char *nick = NULL, *tmp;

	q = line = m_strdup (&current_screen->input_buffer[MIN_POS]);
	if (in_completion == STATE_NORMAL)
	{
		i = word_count (line);
		nick = extract (line, i - 1, i);
	}
	if (nick)
		line[strlen (line) - strlen (nick)] = 0;
	else
		*line = 0;
	if ((tmp = getchannick (input_lastmsg, nick && *nick ? nick : NULL)))
	{
		malloc_strcat (&q, tmp);
		set_input (q);
		update_input (UPDATE_ALL);
		malloc_strcpy (&input_lastmsg, tmp);
		in_completion = STATE_COMPLETE;
	}
	xfree (&q);
	xfree (&nick);
}

char *
getchannick (char *oldnick, char *nick)
{
	ChannelList *chan;
	char *channel, *tnick = NULL;
	NickList *cnick;
	channel = get_current_channel_by_refnum (0);
	if (channel)
	{
		if (!(chan = lookup_channel (channel, from_server, 0)) || !(cnick = next_nicklist (chan, NULL)))
		{
			in_completion = STATE_NORMAL;
			return NULL;
		}
		/* 
		 * we've never been here before so return first nick 
		 * user hasn't entered anything on the line.
		 */
		if (!oldnick && !nick && cnick)
		{
			in_completion = STATE_CNICK;
			return cnick->nick;
		}
		/*
		 * user has been here before so we attempt to find the correct
		 * first nick to start from.
		 */
		if (oldnick)
		{
			/* find the old nick so we have a frame of reference */
			for (cnick = next_nicklist (chan, NULL); cnick; cnick = next_nicklist (chan, cnick))
			{
				if (!my_strnicmp (cnick->nick, oldnick, strlen (oldnick)))
				{
					tnick = cnick->nick;
					if ((cnick = next_nicklist (chan, cnick)))
						tnick = cnick->nick;
					break;

				}

			}
		}
		/*
		 * if the user has put something on the line
		 * we attempt to pattern match here.
		 */
		if (nick && in_completion == STATE_NORMAL)
		{
			/* 
			 * if oldnick was the last one in the channel 
			 * cnick will be NULL;
			 */
			if (!cnick)
			{
				cnick = next_nicklist (chan, NULL);
				tnick = cnick->nick;
			}
			/* we have a new nick */
			else if (next_nicklist (chan, cnick))
			{
				/* 
				 * if there's more than one nick, start 
				 * scanning.
				 */
				for (; cnick; cnick = next_nicklist (chan, cnick))
				{
					if (!my_strnicmp (cnick->nick, nick, strlen (nick)))
					{
						tnick = cnick->nick;
						break;
					}
				}
			}
			else
				tnick = cnick->nick;
		}
		else if (in_completion == STATE_CNICK)
		{
			/*
			 * else we've been here before so
			 * attempt to continue through the nicks 
			 */
			if (!cnick)
				cnick = next_nicklist (chan, NULL);
			tnick = cnick->nick;
		}
	}
	if (tnick)
		in_completion = STATE_CNICK;

	return tnick;
}

void 
addtabkey (char *nick, char *mytype)
{
	NickTab *tmp, *new;

	tmp = tabkey_array;

	if (!tmp || !(new = (NickTab *) remove_from_list ((List **) & tmp, nick)))
	{
		new = (NickTab *) xmalloc (sizeof (NickTab));
		malloc_strcpy (&new->nick, nick);
		if (mytype)
			malloc_strcpy (&new->type, mytype);
	}
	/*
	 * most recent nick is at the top of the list 
	 */
	new->next = tmp;
	tmp = new;
	tabkey_array = tmp;
}


NickTab *
gettabkey (int direction, char *nick)
{
	NickTab *tmp, *new;


	new = tmp = tabkey_array;

	if (nick)
	{
		for (; tmp; tmp = tmp->next)
			if (!my_strnicmp (nick, tmp->nick, strlen (nick)))
				return tmp;
		return NULL;
	}
	tmp = new;
	if (!tmp)
		return NULL;

	switch (direction)
	{
	case 1:
	default:
		{
			/*
			 * need at least two nicks in the list
			 */
			if (new->next)
			{
				/*
				 * reset top of array
				 */
				tabkey_array = new->next;

				/*
				 * set the current nick next pointer to NULL
				 * and then reset top of list.
				 */

				new->next = NULL;
				tmp = tabkey_array;

				/*
				 * find the last nick in the list
				 * so we can make the old top pointer 
				 * point to the item
				 */
				while (tmp)
					if (tmp->next)
						tmp = tmp->next;
					else
						break;
				/* set the pointer and then return. */
				tmp->next = new;
			}
			break;
		}
	case -1:
		{
			if (new && new->next)
			{
				tmp = new;
				while (tmp)
					if (tmp->next && tmp->next->next)
						tmp = tmp->next;
					else
						break;
				/* 
				 * tmp now points at last two items in list 
				 * now just swap some pointers.
				 */
				new = tmp->next;
				tmp->next = NULL;
				new->next = tabkey_array;
				tabkey_array = new;
			}
			break;
		}
	}
	if (new && new->nick)
		return new;
	return NULL;
}

NickTab *
getnextnick (char *input_nick, char *oldnick, char *nick)
{
	ChannelList *chan;
	NickList *cnick = NULL;
	NickTab *tmp = tabkey_array;
	int server = from_server;
	static NickTab sucks =
	{NULL};

	if (tmp && (in_completion == STATE_NORMAL || in_completion == STATE_TABKEY))
	{


		if (!oldnick && !nick && tmp)
		{
			in_completion = STATE_TABKEY;
			return tmp;
		}
		if (oldnick)
		{
			for (; tmp; tmp = tmp->next)
			{
				if (!my_strnicmp (oldnick, tmp->nick, strlen (oldnick)))
					break;
			}
			/* nick was not in the list. oops didn't come from here */
			if (!tmp && in_completion == STATE_TABKEY)
				tmp = tabkey_array;
			else if (tmp)
				tmp = tmp->next;
		}
		if (nick && in_completion != STATE_TABKEY)
		{
			if (tmp && tmp->next)
			{
				for (; tmp; tmp = tmp->next)
					if (!my_strnicmp (nick, tmp->nick, strlen (nick)))
						break;
			}
		}
		if (tmp)
		{
			in_completion = STATE_TABKEY;
			return tmp;
		}
	}

	if ((chan = prepare_command (&server, NULL, 3)))
	{
		cnick = next_nicklist (chan, NULL);
		/* 
		 * we've never been here before so return first nick 
		 * user hasn't entered anything on the line.
		 */
		if (!oldnick && !nick && cnick)
		{
			in_completion = STATE_CNICK;
			sucks.nick = cnick->nick;
			return &sucks;
		}
		/*
		 * user has been here before so we attempt to find the correct
		 * first nick to start from.
		 */
		if (oldnick)
		{
			/* find the old nick so we have a frame of reference */
			for (; cnick; cnick = next_nicklist (chan, cnick))
			{
				if (!my_strnicmp (cnick->nick, oldnick, strlen (oldnick)))
				{
					cnick = next_nicklist (chan, cnick);
					break;
				}
			}
		}
		/*
		 * if the user has put something on the line
		 * we attempt to pattern match here.
		 */
		if (input_nick)
		{
			/* 
			 * if oldnick was the last one in the channel 
			 * cnick will be NULL;
			 */
			if (!cnick && oldnick)
				cnick = next_nicklist (chan, NULL);
			/* we have a new nick */
			else if (cnick)
			{
				/* 
				 * if there's more than one nick, start 
				 * scanning.
				 */
				for (; cnick; cnick = next_nicklist (chan, cnick))
				{
					if (!my_strnicmp (cnick->nick, input_nick, strlen (input_nick)) || !strcasecmp (cnick->nick, input_nick))
						break;
				}
			}
		}
	}
	if (!cnick)
		in_completion = STATE_NORMAL;
	else
		in_completion = STATE_CNICK;
	if (cnick)
		sucks.nick = cnick->nick;
	return sucks.nick ? &sucks : NULL;
}
