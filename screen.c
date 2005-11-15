#ident "@(#)screen.c 1.9"
/*
 * screen.c
 *
 * Written By Matthew Green, based on portions of window.c
 * by Michael Sandrof.
 *
 * Copyright(c) 1993, 1994.
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "commands.h"
#include "screen.h"
#include "status.h"
#include "window.h"
#include "output.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "ircterm.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "exec.h"
#include "newio.h"
#include "misc.h"

Window *to_window;
Screen *current_screen;
Screen *last_input_screen;

int extended_handled = 0;

/* Our list of screens */
Screen *screen_list = NULL;

static int add_to_display_list(Window *, const char *);
Display *new_display_line(Display *);

/* 
 * add_wait_prompt:  Given a prompt string, a function to call when
 * the prompt is entered.. some other data to pass to the function,
 * and the type of prompt..  either for a line, or a key, we add 
 * this to the prompt_list for the current screen..  and set the
 * input prompt accordingly.
 */
void add_wait_prompt(char *prompt, void (*func) (char *, char *), char *data, int type)
{
    WaitPrompt **AddLoc, *New;

    New = (WaitPrompt *) new_malloc(sizeof(WaitPrompt));
    New->prompt = m_strdup(prompt);
    New->data = m_strdup(data);
    New->type = type;
    New->func = func;
    New->next = NULL;
    for (AddLoc = &current_screen->promptlist; *AddLoc; AddLoc = &(*AddLoc)->next) ;
    *AddLoc = New;
    if (AddLoc == &current_screen->promptlist)
	change_input_prompt(1);
}

void set_current_screen(Screen * screen)
{
    current_screen = (screen && screen->alive) ? screen : screen_list;
}

static void add_to_window(Window * window, const char *str);
static char display_highlight(int flag);
static char display_bold(int flag);
static char display_underline(int flag);
static void display_color(long color1, long color2);

/*
 * add_to_screen: This adds the given null terminated buffer to the screen.
 * That is, it determines which window the information should go to, which
 * lastlog the information should be added to, which log the information
 * should be sent to, etc 
 */
void add_to_screen(char *buffer)
{
    Window *tmp = NULL;

    if (!get_int_var(DISPLAY_ANSI_VAR))
	strcpy(buffer, stripansicodes(buffer));

    if (!current_screen || !curr_scr_win) {
	puts(buffer);
	return;
    }

    if (in_window_command)
	update_all_windows();

    tmp = curr_scr_win;
    if ((who_level == LOG_CURRENT) && (curr_scr_win->server == from_server))
	goto found;

    tmp = to_window;
    if (to_window)
	goto found;

    if (who_from && *who_from) {
	tmp = NULL;
	if (is_channel(who_from)) {
	    while (traverse_all_windows(&tmp)) {
		if (tmp->current_channel && tmp->server == from_server) {
		    if (!my_stricmp(who_from, tmp->current_channel))
			goto found;
		}
	    }
	}

	tmp = NULL;
	while (traverse_all_windows(&tmp)) {

	    if (tmp->query_nick && (((who_level == LOG_MSG || who_level == LOG_NOTICE)
				     && !my_stricmp(who_from, tmp->query_nick) &&
				     from_server == tmp->server) ||
				    (who_level == LOG_DCC &&
				     (*tmp->query_nick == '=' || *tmp->query_nick == '-') &&
				     !my_stricmp(who_from, tmp->query_nick + 1))))
		goto found;

	    if (from_server == tmp->server)
		if (find_in_list((struct list **) &(tmp->nicks), who_from, !USE_WILDCARDS))
		    goto found;

	}

	tmp = NULL;
	while (traverse_all_windows(&tmp)) {
	    if (((from_server == tmp->server) || (from_server == -1)) && (who_level & tmp->window_level))
		goto found;

	}
	if (is_channel(who_from) && from_server != -1) {
	    struct channel *chan;

	    if ((chan = (struct channel *) find_in_list((struct list **) &(server_list[from_server].chan_list),
							who_from, !USE_WILDCARDS))) {
		if (chan->window && from_server == chan->window->server)
		    goto found;
	    }
	}
    }

    tmp = NULL;
    while (traverse_all_windows(&tmp)) {
	if (((from_server == tmp->server) || (from_server == -1)) && (who_level & tmp->window_level))
	    goto found;
    }
    tmp = NULL;
    if (from_server == curr_scr_win->server || from_server == -1)
	tmp = curr_scr_win;
    else {
	while (traverse_all_windows(&tmp)) {
	    if (tmp->server == from_server)
		break;
	}
    }
    if (!tmp)
	tmp = curr_scr_win;

  found:
    add_to_window(tmp, buffer);
}

/*
 * add_to_window: adds the given string to the display.  No kidding. This
 * routine handles the whole ball of wax.  It keeps track of what's on the
 * screen, does the scrolling, everything... well, not quite everything...
 * The CONTINUED_LINE idea thanks to Jan L. Peterson (jlp@hamblin.byu.edu)  
 *
 * At least it used to. Now most of this is done by split_up_line, and this
 * function just dumps it onto the screen. This is because the scrollback
 * functions need to be able to figure out how to split things up too.
 */
static void add_to_window(Window * window, const char *str)
{

    if (do_hook(WINDOW_LIST, "%u %s", window->refnum, str)) {
	char **lines;

	add_to_log(window->log_fp, 0, str);
	add_to_lastlog(window, str);
	display_highlight(OFF);
	display_bold(OFF);

	term_move_cursor(0, window->cursor + window->top);
	for (lines = split_up_line(str); *lines; lines++) {
	    if (add_to_display_list(window, *lines)) {
		rite(window, *lines);
		window->cursor++;
	    }
	}
	/* 
	 * This used to be in rite(), but since rite() is a general
	 * purpose function, and this stuff really is only intended
	 * to hook on "new" output, it really makes more sense to do
	 * this here.  This also avoids the terrible problem of 
	 * recursive calls to split_up_line, which are bad.
	 */
	if (!window->visible) {
	    /* 
	     * This is for archon -- he wanted a way to have 
	     * a hidden window always beep, even if BEEP is off.
	     */
	    if (window->beep_always && strchr(str, '\007')) {
		Window *old_to_window;

		term_beep();
		old_to_window = to_window;
		to_window = curr_scr_win;
		say("Beep in window %d", window->refnum);
		to_window = old_to_window;
	    }

	    /* 
	     * Tell some visible window about our problems 
	     * if the user wants us to.
	     */
	    if (!(window->miscflags & WINDOW_NOTIFIED) && who_level & window->notify_level) {
		window->miscflags |= WINDOW_NOTIFIED;
		if (window->miscflags & WINDOW_NOTIFY) {
		    Window *old_to_window;
		    int lastlog_level;

		    lastlog_level = set_lastlog_msg_level(LOG_CRAP);
		    old_to_window = to_window;
		    to_window = curr_scr_win;
		    say("Activity in window %d", window->refnum);
		    to_window = old_to_window;
		    set_lastlog_msg_level(lastlog_level);
		}
	    }
	}

	cursor_in_display();
	term_flush();
    }

    /* ok, we don't really need to update status every time but... */
    /* it helps to give is consistent status */
    update_all_status(curr_scr_win, NULL, 0);
}

/*
 * split_up_line -- converts a logical line into one or more physical lines
 * accounting for special characters, unprintables, etc.  This function
 * is large and obtuse, so itll take a bit of explaining.
 *
 * Essentialy two copies of the string are made: The original string (str)
 * is walked and is copied byte for byte into "buffer", with extra characters
 * being inserted for the sake of special control characters (like beeps or
 * tabs or whatever).  As each line in "buffer" is created, it is malloc-
 * copied off into "output" which holds the final broken result.  When the
 * entire input has been converted into "buffer" and then copied off into
 * "output", the "output" is returned, and passed off to output_line.
 *
 * As much as possible, i have had tried to avoid extraneous malloc() and
 * free calls which are essentially a waste of CPU time.  Note that this
 * means that an exceptionally long line will not have all of its lines
 * free()d until another line of such length is processed.  That can mean
 * a small amount of space that is not "lost" but will not be reclaimed, 
 * which should not be terribly significant, since it saves a tremendous
 * amount of CPU.
 *
 * Note that there are no limiations on the put any longer.  Neither the
 * size of "buffer" (2048 characters) nor the size of "output" (40 lines)
 * are completely inflexible any longer.  The only true limitation now is
 * a screen width of 2048 characters (absurdly large).
 *
 * XXXX -- If it werent for scrollback, you could roll output_line into this
 * function and simplify the output chain greatly, without having to place
 * arbitrary limits on the output.
 */
#define	MAXIMUM_SPLITS	40
#define MAX_SCREEN_BUFFER (BIG_BUFFER_SIZE * 2)
char **split_up_line(const char *str)
{
    static char **output = NULL;
    static int output_size = 0;
    char buffer[BIG_BUFFER_SIZE + 1];
    const char *ptr;
    char *cont_ptr, *cont = empty_str, c;
    int pos = 0,		/* Current pos in "buffer" */
	col = 0,		/* Current col on display */
	word_break = 0,		/* Last end of word */
	i,			/* Generic counter */
	indent = 0,		/* Start of 2nd word */
	beep_cnt = 0,		/* Controls number of beeps */
	beep_max,		/* what od you think? */
	tab_cnt = 0, tab_max, line = 0;	/* Current pos in "output" */
    int len;			/* Used for counting tabs */
    char *pos_copy;		/* Used for reconstruction */
    int do_indent;
    int newline = 0;
    static int recursion = 0;

    if (recursion)
	abort();
    recursion++;

    /* XXXXX BOGUS! XXXXX */
    if (!output_size) {
	int new = MAXIMUM_SPLITS;
	RESIZE(output, char *, new);

	output_size = new;
    }

    *buffer = 0;

    /* Especially handle blank lines */
    if (!*str)
	str = " ";

    beep_max = get_int_var(BEEP_VAR) ? get_int_var(BEEP_MAX_VAR) > 5 ? 5 : get_int_var(BEEP_MAX_VAR) : -1;
    tab_max = get_int_var(TAB_VAR) ? get_int_var(TAB_MAX_VAR) : -1;
    do_indent = get_int_var(INDENT_VAR);

    if (!(cont_ptr = get_string_var(CONTINUED_LINE_VAR)))
	cont_ptr = empty_str;

    /* 
     * Walk the entire input string and convert it as neccesary.
     * Intermediate results go into 'buffer', and the final destination
     * is 'output'.
     */

    for (ptr = str; *ptr && (pos < BIG_BUFFER_SIZE - 20); ptr++) {
	switch (*ptr) {
	case '\007':		/* bell */
	    {
		beep_cnt++;
		if (beep_max == -1 || beep_cnt > beep_max) {
		    buffer[pos++] = REV_TOG;
		    buffer[pos++] = ((*ptr) & 127) | 64;
		    buffer[pos++] = REV_TOG;
		    col++;
		} else
		    buffer[pos++] = *ptr;
		break;
	    }
	case '\011':		/* tab */
	    {
		tab_cnt++;
		if (tab_max > 0 && tab_cnt > tab_max) {
		    buffer[pos++] = REV_TOG;
		    buffer[pos++] = ((*ptr) & 127) | 64;
		    buffer[pos++] = REV_TOG;
		    col++;
		} else {
		    if (indent == 0)
			indent = -1;
		    word_break = pos;

		    /* 
		     * Tabs only go as far as the right
		     * edge of the screen -- no farther
		     */
		    len = 8 - (col % 8);
		    for (i = 0; i < len; i++) {
			buffer[pos++] = ' ';
			if (col++ >= term_cols)
			    break;
		    }
		}
		break;
	    }
	case '\n':		/* Forced newline */
	    {
		newline = 1;
		if (indent == 0)
		    indent = -1;
		word_break = pos;
		break;
	    }
	case ' ':		/* word break */
	    {
		if (indent == 0)
		    indent = -1;
		word_break = pos;
		buffer[pos++] = *ptr;
		col++;
		break;
	    }
	case UND_TOG:
	case ALL_OFF:
	case REV_TOG:
	case BOLD_TOG:
	    {
		buffer[pos++] = *ptr;
		break;
	    }
	case '\003':		/* ^C colors */
	    {
		buffer[pos++] = *ptr;
		if (ptr[1] && isdigit(ptr[1])) {
		    while (ptr[1] && isdigit(ptr[1])) {
			buffer[pos++] = ptr[1];
			ptr++, i++;
		    }

		    if (ptr[1] == ',') {
			buffer[pos++] = ptr[1];
			ptr++;
			while (ptr[1] && isdigit(ptr[1])) {
			    buffer[pos++] = ptr[1];
			    ptr++, i++;
			}
		    }
		}
		break;
	    }
	case '\033':		/* ESCape */
	    {
		/* 
		 * This is the only character that
		 * term_putchar lets go by unchecked.
		 * so we check it here.  Everything
		 * else doesnt need to be filtered.
		 */
		if (!get_int_var(DISPLAY_ANSI_VAR)) {
		    buffer[pos++] = REV_TOG;
		    buffer[pos++] = '[';
		    buffer[pos++] = REV_TOG;
		    col++;
		    break;
		}
		/* ELSE FALLTHROUGH */
	    }

	    /* Everything else including unfiltered ESCapes */
	default:
	    {
		/* Check if printable... */
		if (!vt100_decode(*ptr)) {
		    if (indent == -1)
			indent = col;
		    col++;
		}

		buffer[pos++] = *ptr;
		break;
	    }
	}

	/* 
	 * Ok.  This test is to see if we've reached the right side
	 * of the display.  If we do, then we have three tasks:
	 *  1. Break off the current line at the end of the last word
	 *  2. Start the new line with the "continued line" string
	 *  3. Put back the stuff already parsed after the break.
	 */
	if (col >= term_cols || newline) {
	    /* one big long line, no word breaks */
	    if (word_break == 0)
		word_break = term_cols - 1;	/* 
						 * MHacker came up with a
						 * possible solution here
						 */ ;

	    /* 
	     * Resize output if neccesary.
	     * XXX Note that this uses -3, so that there are
	     * always at least two lines available when we
	     * exit the for loop -- one for the last line,
	     * and one for the 
	     */
	    if (line >= output_size - 3) {
		int new = output_size + MAXIMUM_SPLITS + 1;
		RESIZE(output, char *, new);

		output_size = new;
	    }

	    /* 1. Break off the current line at the end */
	    /* 
	     * Copy the current line into the return values.
	     */
	    c = buffer[word_break];
	    buffer[word_break] = '\0';
	    malloc_strcpy((char **) &(output[line++]), buffer);
	    buffer[word_break] = c;

	    /* 2. Calculate the "continued line" string */

	    /* 
	     * If the 'continued line' string has not yet
	     * been set, we need to set it here (since at
	     * this point the first line is done, and we cant
	     * do the /SET INDENT until we know how the first
	     * line is layed out.
	     */
	    if (!*cont && do_indent && (indent < term_cols / 3) && (strlen(cont_ptr) < indent)) {
		cont = alloca(indent + 10);
		sprintf(cont, "%-*s", indent, cont_ptr);
	    }
	    /* 
	     * If cont_ptr has not been initialize, do so.
	     * Otherwise we dont do anything.
	     */
	    else if (!*cont && *cont_ptr)
		cont = cont_ptr;

	    /* 3. Paste the new line back together again. */

	    /* 
	     * Skip over the spaces that comprised the break
	     * between the lines.
	     */
	    while (buffer[word_break] == ' ' && word_break < pos)
		word_break++;

	    /* 
	     * Now re-create the new line
	     * and figure out where we are.
	     */
	    buffer[pos] = 0;
	    pos_copy = alloca(strlen(buffer + word_break) + strlen(cont) + 2);
	    strcpy(pos_copy, buffer + word_break);

	    strcpy(buffer, cont);
	    strcat(buffer, pos_copy);
	    col = pos = strlen(buffer);
	    word_break = 0;
	    newline = 0;

	    if (get_int_var(DISPLAY_ANSI_VAR))
		col -= count_ansi(buffer, -1);
	}
    }

    /* 
     * At this point we've completely walked the input source, so we 
     * clean up anything left in the intermediate buffer.  This is the 
     * "typical" case, since most output is only one line long.
     */
    buffer[pos++] = ALL_OFF;
    buffer[pos] = '\0';
    if (*buffer)
	malloc_strcpy((char **) &(output[line++]), buffer);

    new_free(&output[line]);
    recursion--;
    return output;
}

/*
 * This function adds an item to the window's display list.  If the item
 * should be displayed on the screen, then 1 is returned.  If the item is
 * not to be displayed, then 0 is returned.  This function handles all
 * the hold_mode stuff.
 */
static int add_to_display_list(Window * window, const char *str)
{
    /* Add to the display list */

    /* ARGH THIS NEEDS TO BE FIXED */
    /* For some reason display_ip sometimes gets null here. prevent the segv, and scream */
    /* Hmmm.... display_ip /really/ shouldnt be null here, I think the window struct was free'ed before this call. D'oh! */
    if (window->display_ip == NULL) {
	bitchsay("FUCK! display_ip is null.. FIX THIS BUG!!! str=\"%s\"", str);
	return -1;
    }

    window->display_ip->next = new_display_line(window->display_ip);
    malloc_strcpy(&window->display_ip->line, str);
    window->display_ip = window->display_ip->next;
    window->display_buffer_size++;
    window->distance_from_display++;

    /* 
     * Only output the line if hold mode isnt activated
     */
    if (((window->distance_from_display > window->display_size) &&
	 window->scrollback_point) || (window->hold_mode && (++window->held_displayed >= window->display_size))) {
	if (!window->holding_something)
	    window->holding_something = 1;

	window->lines_held++;
	if (window->lines_held >= window->last_lines_held + 10) {
	    update_window_status(window, 0);
	    window->last_lines_held = window->lines_held;
	}
	return 0;
    }

    /* 
     * Before outputting the line, we snip back the top of the
     * display buffer.  (The fact that we do this only when we get
     * to here is what keeps whats currently on the window always
     * active -- once we get out of hold mode or scrollback mode, then
     * we truncate the display buffer at that point.)
     */
    while (window->display_buffer_size > window->display_buffer_max) {
	Display *next = window->top_of_scrollback->next;

	delete_display_line(window->top_of_scrollback);
	window->top_of_scrollback = next;
	window->display_buffer_size--;
    }

    /* 
     * Now we need to see if the top_of_window pointer should be moved
     * up, and if the window is visible, to physically scroll the screen.
     */
    scroll_window(window);

    /* 
     * And tell them its ok to output this line.
     */
    return 1;
}

/*
 * rite: this routine displays a line to the screen adding bold facing when
 * specified by ^Bs, etc.  It also does handle scrolling and paging, if
 * SCROLL is on, and HOLD_MODE is on, etc.  This routine assumes that str
 * already fits on one screen line.  If show is true, str is displayed
 * regardless of the hold mode state.  If redraw is true, it is assumed we a
 * redrawing the screen from the display_ip list, and we should not add what
 * we are displaying back to the display_ip list again. 
 *
 * Note that rite sets display_highlight() to what it was at then end of the
 * last rite().  Also, before returning, it sets display_highlight() to OFF.
 * This way, between susequent rites(), you can be assured that the state of
 * bold face will remain the same and the it won't interfere with anything
 * else (i.e. status line, input line). 
 */
int rite(Window * window, const char *str)
{
    static int high = OFF;
    Screen *old_current_screen = current_screen;

    if (!window->visible)
	return 1;

    set_current_screen(window->screen);
    display_highlight(high);
    output_line((char *) str);
    high = display_highlight(OFF);
    term_cr();
    term_newline();
    set_current_screen(old_current_screen);

    return 0;
}

/*
 * This actually outputs a string to the screen.  Youll notice this function
 * is a lot simpler than it was before, because all of that busiwork it did
 * before was completely bogus.  What happens now is:
 *
 * Any time we get a recognized highlight character, we handle it by 
 * toggling the appropriate setting.  This might waste a few function calls
 * if people have two of the same highlight char next to each other, but
 * that savings isnt worth the maintainance nightmare of the previous code.
 *
 * Any other character that isnt a recognized highlight character is 
 * dispatched to the screen.  Since we use term_putchar(), any special
 * handling of ansi sequences is filtered there.  We do not know, and we
 * do not care about what is actually in the string.  We can (and do) assume
 * that the string passed to us in "str" will fit on the screen at whatever
 * position the cursor is at.
 *
 * Unless the last character on the line is an ALL_OFF character, output_line
 * will *NOT* reset any attributes that may be set during the source of the
 * output.  This is so you can output one or more lines that constitute part
 * of a "logical" line, and any highlights or colors will "bleed" onto the
 * next physical line.  The last such physical line in a logical line without
 * exception MUST end with the ALL_OFF character, or everything will go all
 * higgledy-piggledy.  As long as you do that, everything will look ok.
 */
int output_line(register char *str)
{
    char *ptr = str;
    int beep = 0;

    /* Walk the string looking for special chars. */

    while (*ptr) {
	switch (*ptr) {
	case REV_TOG:
	    {
		display_highlight(TOGGLE);
		break;
	    }
	case UND_TOG:
	    {
		display_underline(TOGGLE);
		break;
	    }
	case BOLD_TOG:
	    {
		display_bold(TOGGLE);
		break;
	    }
	case ALL_OFF:
	    {
		display_underline(OFF);
		display_highlight(OFF);
		display_bold(OFF);	/* This turns it all off */

		if (!ptr[1])	/* We're the end of line */
		    term_bold_off();
		break;
	    }
	case '\003':		/* ^C colors */
	    {
		long c1 = -1, c2 = -1;

		if (ptr[1] && isdigit(ptr[1])) {
		    ptr++;
		    c1 = strtoul(ptr, (char **) &ptr, 10);
		    if (*ptr == ',') {
			ptr++;
			c2 = strtoul(ptr, (char **) &ptr, 10);
		    }
		    ptr--;
		}
		display_color(c1, c2);
		break;
	    }
	case '\007':
	    {
		beep++;
		break;
	    }
	default:
	    {
		term_putchar(*ptr);
		break;
	    }
	}
	ptr++;
    }

    if (beep)
	term_beep();
    term_clear_to_eol();
    return 0;
}

/*
 * scroll_window: Given a pointer to a window, this determines if that window
 * should be scrolled, or the cursor moved to the top of the screen again, or
 * if it should be left alone. 
 */
void scroll_window(Window * window)
{

    if (window->cursor == window->display_size) {
	if (window->holding_something || window->scrollback_point)
	    abort();
	if (window->scroll) {
	    int scroll, i;

	    if ((scroll = get_int_var(SCROLL_LINES_VAR)) <= 0)
		scroll = 1;

	    for (i = 0; i < scroll; i++) {
		window->top_of_display = window->top_of_display->next;
		window->distance_from_display--;
	    }

	    if (window->visible) {
		term_scroll(window->top, window->top + window->cursor - 1, scroll);
		window->cursor -= scroll;
		term_move_cursor(0, window->cursor + window->top);
	    } else
		window->cursor -= scroll;
	} else {
	    window->cursor = 0;
	    if (window->visible)
		term_move_cursor(0, window->top);
	}
    }
    if (window->visible && current_screen->cursor_window)
	term_clear_to_eol();
}

/* display_highlight: turns off and on the display highlight.  */
static char display_highlight(int flag)
{
    static int highlight = OFF;

    if (flag == highlight)
	return (flag);
    switch (flag) {
    case ON:
	{
	    highlight = ON;
	    if (get_int_var(INVERSE_VIDEO_VAR))
		term_standout_on();
	    return (OFF);
	}
    case OFF:
	{
	    highlight = OFF;
	    if (get_int_var(INVERSE_VIDEO_VAR))
		term_standout_off();
	    return (ON);
	}
    case TOGGLE:
	{
	    if (highlight == ON) {
		highlight = OFF;
		if (get_int_var(INVERSE_VIDEO_VAR))
		    term_standout_off();
		return (ON);
	    } else {
		highlight = ON;
		if (get_int_var(INVERSE_VIDEO_VAR))
		    term_standout_on();
		return (OFF);
	    }
	}
    }
    return flag;
}

/* display_bold: turns off and on the display bolding.  */
static char display_bold(int flag)
{
    static int bold = OFF;

    if (flag == bold)
	return (flag);
    switch (flag) {
    case ON:
	bold = ON;
	if (get_int_var(BOLD_VIDEO_VAR))
	    term_bold_on();
	return (OFF);
    case OFF:
	bold = OFF;
	if (get_int_var(BOLD_VIDEO_VAR))
	    term_bold_off();
	return (ON);
    case TOGGLE:
	if (bold == ON) {
	    bold = OFF;
	    if (get_int_var(BOLD_VIDEO_VAR))
		term_bold_off();
	    return (ON);
	} else {
	    bold = ON;
	    if (get_int_var(BOLD_VIDEO_VAR))
		term_bold_on();
	    return (OFF);
	}
    }
    return OFF;
}

/* display_bold: turns off and on the display bolding.  */
static char display_underline(int flag)
{
    static int underline = OFF;

    if (flag == underline)
	return (flag);

    switch (flag) {
    case ON:
	{
	    underline = ON;
	    if (get_int_var(UNDERLINE_VIDEO_VAR))
		term_underline_on();
	    return (OFF);
	}
    case OFF:
	{
	    underline = OFF;
	    if (get_int_var(UNDERLINE_VIDEO_VAR))
		term_underline_off();
	    return (ON);
	}
    case TOGGLE:
	{
	    if (underline == ON) {
		underline = OFF;
		if (get_int_var(UNDERLINE_VIDEO_VAR))
		    term_underline_off();
		return (ON);
	    } else {
		underline = ON;
		if (get_int_var(UNDERLINE_VIDEO_VAR))
		    term_underline_on();
		return (OFF);
	    }
	}
    }
    return OFF;
}

static void display_color(long color1, long color2)
{
    char seq[32];

    if (!get_int_var(MIRCS_VAR))
	return;

    if (color1 > 7) {
	term_bold_on();
	color1 -= 8;
    }
    if (color2 > 7) {
/*              term_blink_on(); */
	color2 -= 8;
    }

    if (color1 != -1) {
	sprintf(seq, "\033[%ldm", color1 + 30);
	tputs_x(seq);
    }
    if (color2 != -1) {
	sprintf(seq, "\033[%ldm", color2 + 40);
	tputs_x(seq);
    }
    if (color1 == -1 && color2 == -1) {
	sprintf(seq, "\033[%d;%dm", 39, 49);
	tputs_x(seq);
	term_bold_off();
    }
}

/*
 * cursor_not_in_display: This forces the cursor out of the display by
 * setting the cursor window to null.  This doesn't actually change the
 * physical position of the cursor, but it will force rite() to reset the
 * cursor upon its next call 
 */
void cursor_not_in_display(void)
{
    current_screen->cursor_window = NULL;
}

/*
 * cursor_in_display: this forces the cursor_window to be the
 * current_screen->current_window. 
 * It is actually only used in hold.c to trick the system into thinking the
 * cursor is in a window, thus letting the input updating routines move the
 * cursor down to the input line.  Dumb dumb dumb 
 */
void cursor_in_display(void)
{
    current_screen->cursor_window = curr_scr_win;
}

/*
 * is_cursor_in_display: returns true if the cursor is in one of the windows
 * (cursor_window is not null), false otherwise 
 */
int is_cursor_in_display(Screen * screen)
{
    return ((screen ? screen : current_screen)->cursor_window ? 1 : 0);
}

/* * * * * * * SCREEN UDPATING AND RESIZING * * * * * * * * */
/*
 * repaint_window: redraw the "m"th through the "n"th part of the window.
 * If end_line is -1, then that means clear the display if any of it appears
 * after the end of the scrollback buffer.
 */
void repaint_window(Window * w, int start_line, int end_line)
{
    Window *window = (Window *) w;
    Display *curr_line;
    int count;
    int clean_display = 0;

    if (!window->visible)
	return;

    if (!window)
	window = curr_scr_win;

    if (end_line == -1) {
	end_line = window->display_size - 1;	/* CDE this fixes a small flash problem */
	clean_display = 1;
    }

    curr_line = window->top_of_display;
    term_move_cursor(0, window->top + start_line);

    for (count = start_line; count <= end_line; count++) {
	if (curr_line->line)
	    rite(window, curr_line->line);
	else
	    term_clear_to_eol();

	if (curr_line == window->display_ip)
	    break;

	curr_line = curr_line->next;
    }

    /* 
     * If "end_line" is -1, then we're supposed to clear off the rest
     * of the display, if possible.  Do that here.
     */
    if (clean_display) {
	for (; count <= end_line; count++) {
	    term_clear_to_eol();
	    term_newline();
	}
	update_window_status(window, 1);
    }

    recalculate_window_cursor(window);	/* XXXX */
    term_flush();
}

/*
 * create_new_screen creates a new screen structure. with the help of
 * this structure we maintain ircII windows that cross screen window
 * boundaries.
 */
Screen *create_new_screen(void)
{
    Screen *new = NULL, **list;
    static int refnumber = 0;
    int i;

    for (list = &screen_list; *list; list = &((*list)->next)) {
	if (!(*list)->alive) {
	    new = *list;
	    break;
	}
    }
    if (!new) {
	new = (Screen *) new_malloc(sizeof(Screen));
	memset(new, 0, sizeof(Screen));
	new->screennum = ++refnumber;
	new->next = screen_list;
	if (screen_list)
	    screen_list->prev = new;
	screen_list = new;
    }
    new->last_window_refnum = 1;
    new->window_list = NULL;
    new->window_list_end = NULL;
    new->cursor_window = NULL;
    new->current_window = NULL;
    new->visible_windows = 0;
    for (i = 0; i <= 9; i++)
	new->meta_hit[i] = 0;

    new->quote_hit = new->digraph_hit = 0;
    new->buffer_pos = new->buffer_min_pos = 0;
    new->input_buffer[0] = '\0';
    new->fdout = 1;
    new->fpout = stdout;
    new->fdin = 0;
    new->fpin = stdin;
    new->alive = 1;
    new->promptlist = NULL;
    new->tty_name = NULL;
    new->li = 24;
    new->co = 79;
    last_input_screen = new;
    return new;
}

void close_all_screen(void)
{
    Screen *screen;

    for (screen = screen_list; screen && screen != current_screen; screen = screen->next) {
	if (screen->alive && screen->fdin != 0) {
	    new_close(screen->fdin);
	}
    }
}

void set_screens(fd_set * rd, fd_set * wd)
{
    Screen *screen;

    for (screen = screen_list; screen; screen = screen->next)
	if (screen->alive)
	    FD_SET(screen->fdin, rd);
}

/* cough cough */
extern int key_pressed;

void do_screens(fd_set * rd)
{
    Screen *screen;

    for (screen = screen_list; screen; screen = screen->next) {
	if (!screen->alive)
	    continue;
	set_current_screen(screen);
	if (FD_ISSET(screen->fdin, rd)) {
	    int server;
	    char loc_buffer[BIG_BUFFER_SIZE + 1];
	    int n, i;

	    /* 
	     * This section of code handles all in put from the terminal(s).
	     * connected to ircII.  Perhaps the idle time *shouldn't* be 
	     * reset unless its not a screen-fd that was closed..
	     */
	    time(&idle_time);

	    server = from_server;
	    from_server = get_window_server(0);
	    last_input_screen = screen;
	    if ((n = read(screen->fdin, loc_buffer, BIG_BUFFER_SIZE)) > 0) {
		extended_handled = 0;
		for (i = 0; i < n; i++)
		    if (!extended_handled)
			edit_char(loc_buffer[i]);
		key_pressed = loc_buffer[0];
	    } else
		irc_exit("Hey!  Where'd my controlling terminal go?", NULL);
	    from_server = server;
	}
    }
}
