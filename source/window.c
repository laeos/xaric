/*
 * window.c: Handles the Main Window stuff for irc.  This includes proper
 * scrolling, saving of screen memory, refreshing, clearing, etc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 * Modified 1996 Colten Edwards
 */


#include "irc.h"

#include "screen.h"
#include "window.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "ircterm.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "status.h"
#include "output.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "misc.h"
#include "tcommand.h"

/* Resize relatively or absolutely? */
#define RESIZE_REL 1
#define RESIZE_ABS 2

Window *invisible_list = NULL;	/* list of hidden windows */

char *who_from = NULL;
int who_level = LOG_CRAP;	/* Log level of message being displayed */

int in_window_command = 0;	/* set to true if we are in window().  This
				 * is used if a put_it() is called within the
				 * window() command.  We make sure all
				 * windows are fully updated before doing the
				 * put_it(). 
				 */

/*
 * window_display: this controls the display, 1 being ON, 0 being OFF.  The
 * DISPLAY var sets this. 
 */
unsigned int window_display = 1;

/*
 * status_update_flag: if 1, the status is updated as normal.  If 0, then all
 * status updating is suppressed 
 */
int status_update_flag = 1;


static void remove_from_invisible_list (Window * window);
void remove_from_window_list (Window * window);
static void swap_window (Window * v_window, Window * window);
static Window *get_next_window (void);
static Window *get_previous_window (void);
static void revamp_window_levels (Window * window);
void clear_window (Window * window);
static void resize_window_display (Window * window);
Screen *create_new_screen (void);
void repaint_window (Window *, int, int);

/*
 * new_window: This creates a new window on the screen.  It does so by either
 * splitting the current window, or if it can't do that, it splits the
 * largest window.  The new window is added to the window list and made the
 * current window 
 */
Window *
new_window (void)
{
	Window *new;
	Window *tmp;
	static int no_screens = 1;
	unsigned int new_refnum = 1;

	if (no_screens)

	{
		set_current_screen (create_new_screen ());
		main_screen = current_screen;
		no_screens = 0;
	}

	new = (Window *) new_malloc (sizeof (Window));

	tmp = NULL;
	while (traverse_all_windows (&tmp))
	{
		if (tmp->refnum == new_refnum)
		{
			new_refnum++;
			tmp = NULL;
		}
	}
	new->refnum = new_refnum;

	if (curr_scr_win)
		new->server = curr_scr_win->server;
	else
		new->server = primary_server;


	if (current_screen->visible_windows)
		new->window_level = LOG_NONE;

	new->scroll = get_int_var (SCROLL_VAR);
	new->lastlog_level = real_lastlog_level ();
	new->lastlog_max = get_int_var (LASTLOG_VAR);

	new->status_line[0] = NULL;
	new->status_line[1] = NULL;
	new->double_status = 0;

	new->display_size = 1;
	new->old_size = 1;
	new->visible = 1;
	new->screen = current_screen;
	new->notify_level = real_notify_level ();

	new->display_buffer_max = 1024;
	new->hold_mode = get_int_var (HOLD_MODE_VAR);
	new->current_channel = NULL;

	if (add_to_window_list (new))
	{
		set_current_window (new);
		resize_window_display (new);
	}
	else
		new_free (&new);

	term_flush ();
	return (new);
}

/*
 * delete_window: This deletes the given window.  It frees all data and
 * structures associated with the window, and it adjusts the other windows so
 * they will display correctly on the screen. 
 */
void 
delete_window (Window * window)
{
	char buffer[BIG_BUFFER_SIZE + 1];

	if (window == NULL)
		window = curr_scr_win;
	if (window->visible && (current_screen->visible_windows == 1))
	{
		if (invisible_list)
		{
			swap_window (window, invisible_list);
			window = invisible_list;
		}
		else
		{
			if (!get_int_var (WINDOW_QUIET_VAR))
				say ("You can't kill the last window!");
			return;
		}
	}
	if (window->name)
		strmcpy (buffer, window->name, BIG_BUFFER_SIZE - 1);
	else
		sprintf (buffer, "%u", window->refnum);

	if (current_screen->last_window_refnum == window->refnum)
		current_screen->last_window_refnum = curr_scr_win->refnum;

	new_free (&window->query_nick);
	new_free (&window->query_host);
	new_free (&window->query_cmd);
	new_free (&window->current_channel);
	new_free (&window->bind_channel);
	new_free (&window->waiting_channel);
	new_free (&window->logfile);
	new_free (&window->name);

	/*
	 * Free off the display
	 */
	{
		Display *next;
		while (window->top_of_scrollback)
		{
			next = window->top_of_scrollback->next;
			new_free (&window->top_of_scrollback->line);
			new_free ((char **) &window->top_of_scrollback);
			window->display_buffer_size--;
			window->top_of_scrollback = next;
		}
		window->display_ip = NULL;
		if (window->display_buffer_size != 0)
			ircpanic ("Ack. fix this.");
	}

	/*
	 * Free off the lastlog
	 */
	while (window->lastlog_size)
		remove_from_lastlog (window);

	/*
	 * Free off the nick list
	 */
	{
		NickList *next;

		while (window->nicks)
		{
			next = window->nicks->next;
			new_free (&window->nicks->nick);
			new_free ((char **) &window->nicks);
			window->nicks = next;
		}
	}

	if (window->visible)
		remove_from_window_list (window);
	else
		remove_from_invisible_list (window);

	/*
	 * status lines
	 */
	new_free (&window->status_line[0]);
	new_free (&window->status_line[1]);

	new_free ((char **) &window);
	window_check_servers ();
	do_hook (WINDOW_KILL_LIST, "%s", buffer);
}

/*
 * traverse_all_windows: Based on the idea from phone that you should
 * be able to do more than one iteration simultaneously.
 *
 * To initialize, *ptr should be NULL.  The function will return 1 each time
 * *ptr is set to the next valid window.  When the function returns 0, then
 * you have iterated all windows.
 */
int 
traverse_all_windows (Window ** ptr)
{
	/*
	 * If this is the first time through...
	 */
	if (!*ptr)
	{
		/*
		 * If there are no windows or screens, punt it.
		 */
		if (!screen_list || !screen_list->window_list)
			return 0;

		/*
		 * Start with the first window on first screen
		 */
		*ptr = screen_list->window_list;
	}

	/*
	 * As long as there is another window on this screen, keep going.
	 */
	else if ((*ptr)->next)
	{
		*ptr = (*ptr)->next;
	}

	/*
	 * If there are no more windows on this screen, but we do belong to
	 * a screen (eg, we're not invisible), try the next screen
	 */
	else if ((*ptr)->screen)
	{
		/*
		 * Skip any dead screens
		 */
		Screen *ns = (*ptr)->screen->next;
		while (ns && !ns->alive)
			ns = ns->next;

		/*
		 * If there are no other screens, then if there is a list
		 * of hidden windows, try that.  Otherwise we're done.
		 */
		if (!ns || !ns->window_list)
		{
			if (invisible_list)
				*ptr = invisible_list;
			else
				return 0;
		}

		/*
		 * There is another screen, move to it.
		 */
		else
			*ptr = ns->window_list;
	}

	/*
	 * Otherwise there are no other windows, and we're not on a screen
	 * (eg, we're hidden), so we're all done here.
	 */
	else
		return 0;

	/*
	 * If we get here, we're in business!
	 */
	return 1;
}

static void 
remove_from_invisible_list (Window * window)
{
	window->visible = 1;
	window->screen = current_screen;
	window->miscflags &= ~WINDOW_NOTIFIED;
	if (window->prev)
		window->prev->next = window->next;
	else
		invisible_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
}

void 
add_to_invisible_list (Window * window)
{
	if ((window->next = invisible_list) != NULL)
		invisible_list->prev = window;
	invisible_list = window;
	window->prev = NULL;
	window->visible = 0;
	window->screen = NULL;
}

/*
 * add_to_window_list: This inserts the given window into the visible window
 * list (and thus adds it to the displayed windows on the screen).  The
 * window is added by splitting the current window.  If the current window is
 * too small, the next largest window is used.  The added window is returned
 * as the function value or null is returned if the window couldn't be added 
 */
extern Window *
add_to_window_list (Window * new)
{
	Window *biggest = NULL, *tmp;

	current_screen->visible_windows++;
	if (curr_scr_win == NULL)
	{
		current_screen->window_list_end = current_screen->window_list = new;
		recalculate_windows ();
	}
	else
	{
		/* split current window, or find a better window to split */
		if ((curr_scr_win->display_size < 4) || get_int_var (ALWAYS_SPLIT_BIGGEST_VAR))
		{
			int size = 0;
			for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
			{
				if (tmp->display_size > size)
				{
					size = tmp->display_size;
					biggest = tmp;
				}
			}
			if ((biggest == NULL) || (size < 4))
			{
				if (!get_int_var (WINDOW_QUIET_VAR))
					say ("Not enough room for another window!");
				current_screen->visible_windows--;
				return NULL;
			}
		}
		else
			biggest = curr_scr_win;
		if ((new->prev = biggest->prev) != NULL)
			new->prev->next = new;
		else
			current_screen->window_list = new;
		new->next = biggest;
		biggest->prev = new;
		new->top = biggest->top;
		new->bottom = (biggest->top + biggest->bottom) / 2 - new->double_status;
		biggest->top = new->bottom + new->double_status + 1;
		new->display_size = new->bottom - new->top;
		biggest->display_size = biggest->bottom - biggest->top;
		new->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		biggest->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	}
	return new;
}

/*
 * remove_from_window_list: this removes the given window from the list of
 * visible windows.  It closes up the hole created by the windows abnsense in
 * a nice way 
 */
void 
remove_from_window_list (Window * window)
{
	Window *other;

	/* find adjacent visible window to close up the screen */
	for (other = window->next; other; other = other->next)
	{
		if (other->visible)
		{
			other->top = window->top;
			break;
		}
	}
	if (other == NULL)
	{
		for (other = window->prev; other; other = other->prev)
		{
			if (other->visible)
			{
				other->bottom = window->bottom +
					window->double_status - other->double_status;
				break;
			}
		}
	}
	/* remove window from window list */
	if (window->prev)
		window->prev->next = window->next;
	else
		current_screen->window_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
	else
		current_screen->window_list_end = window->prev;
	if (window->visible)
	{
		current_screen->visible_windows--;
		other->display_size = other->bottom - other->top;
		if (window == curr_scr_win)
			set_current_window (NULL);
		if (window->refnum == current_screen->last_window_refnum)
			current_screen->last_window_refnum =
				curr_scr_win->refnum;
	}
}

/*
 * recalculate_window_positions: This runs through the window list and
 * re-adjusts the top and bottom fields of the windows according to their
 * current positions in the window list.  This doesn't change any sizes of
 * the windows 
 */
void 
recalculate_window_positions (void)
{
	Window *tmp = current_screen->window_list;
	int top;

	top = 0;
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		tmp->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		tmp->top = top;
		tmp->bottom = top + tmp->display_size;
		top += tmp->display_size + 1 + tmp->double_status;
	}
}

/*
 * swap_window: This swaps the given window with the current window.  The
 * window passed must be invisible.  Swapping retains the positions of both
 * windows in their respective window lists, and retains the dimensions of
 * the windows as well 
 */
static void 
swap_window (Window * v_window, Window * window)
{
	Window tmp, *prev, *next;
	int top, bottom, size;

	if (!window)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("The window to be swapped in does not exist.");
		return;
	}

	if (window->visible || !v_window->visible)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("You can only SWAP a hidden window with a visible window.");
		return;
	}

	prev = v_window->prev;
	next = v_window->next;

	current_screen->last_window_refnum = v_window->refnum;
	current_screen->last_window_refnum = v_window->refnum;
	remove_from_invisible_list (window);

	tmp = *v_window;
	*v_window = *window;
	v_window->top = tmp.top;

	if (v_window->top < 0)
		v_window->top = 0;

	v_window->bottom = tmp.bottom + tmp.double_status - v_window->double_status;
	v_window->display_size = tmp.display_size +
		tmp.double_status - v_window->double_status;
	v_window->prev = prev;
	v_window->next = next;

	top = window->top;
	bottom = window->bottom;
	size = window->display_size;
	*window = tmp;
	window->top = top;
	window->bottom = bottom - tmp.double_status;
	window->display_size = size;

	add_to_invisible_list (window);

	v_window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	recalculate_windows ();
}

/*
 * move_window: This moves a window offset positions in the window list. This
 * means, of course, that the window will move on the screen as well 
 */
void 
move_window (Window * window, int offset)
{
	Window *tmp, *last;
	int win_pos, pos;

	if (offset == 0)
		return;
	last = NULL;
	for (win_pos = 0, tmp = current_screen->window_list; tmp;
	     tmp = tmp->next, win_pos++)
	{
		if (window == tmp)
			break;
		last = tmp;
	}
	if (tmp == NULL)
		return;
	if (last == NULL)
		current_screen->window_list = tmp->next;
	else
		last->next = tmp->next;
	if (tmp->next)
		tmp->next->prev = last;
	else
		current_screen->window_list_end = last;
	if (offset < 0)
		win_pos = (current_screen->visible_windows + offset + win_pos) %
			current_screen->visible_windows;
	else
		win_pos = (offset + win_pos) % current_screen->visible_windows;
	last = NULL;
	for (pos = 0, tmp = current_screen->window_list;
	     pos != win_pos; tmp = tmp->next, pos++)
		last = tmp;
	if (last == NULL)
		current_screen->window_list = window;
	else
		last->next = window;
	if (tmp)
		tmp->prev = window;
	else
		current_screen->window_list_end = window;
	window->prev = last;
	window->next = tmp;
	recalculate_window_positions ();
}

/*
 * resize_window: if 'how' is RESIZE_REL, then this will increase or decrease
 * the size of the given window by offset lines (positive offset increases,
 * negative decreases).  If 'how' is RESIZE_ABS, then this will set the 
 * absolute size of the given window.
 * Obviously, with a fixed terminal size, this means that some other window
 * is going to have to change size as well.  Normally, this is the next
 * window in the window list (the window below the one being changed) unless
 * the window is the last in the window list, then the previous window is
 * changed as well 
 */
void 
resize_window (int how, Window * window, int offset)
{
	Window *other;
	int after, window_size, other_size;

	if (window == NULL)
		window = curr_scr_win;
	if (!window->visible)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("You cannot change the size of hidden windows!");
		return;
	}
	other = window;
	after = 1;
	do
	{
		if (other->next)
			other = other->next;
		else
		{
			other = current_screen->window_list;
			after = 0;
		}

		if (other == window)
		{
			say ("Can't change the size of this window!");
			return;
		}
	}
	while (other->absolute_size);

	if (how == RESIZE_REL)
	{
		window_size = window->display_size + offset;
		other_size = other->display_size - offset;
	}
	else
		/* absolute size */
	{
		/* 
		 * How much its growing/shrinking by.  if
		 * offset > display_size, then window_size < 0.
		 * and other window is shrinking.  If offset < display_size,
		 * the window_size > 0, and other_window is growing.
		 */
		window_size = offset;
		offset -= window->display_size;
		other_size = other->display_size - offset;
	}

	if ((window_size < 3) || (other_size < 3))
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("Not enough room to resize this window!");
		return;
	}
	if (after)
	{
		window->bottom += offset;
		other->top += offset;
	}
	else
	{
		window->top -= offset;
		other->bottom -= offset;
	}
	window->display_size = window_size;
	other->display_size = other_size;
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	other->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	recalculate_window_positions ();
	term_flush ();
}

/*
 * resize_display: After determining that the screen has changed sizes, this
 * resizes all the internal stuff.  If the screen grew, this will add extra
 * empty display entries to the end of the display list.  If the screen
 * shrank, this will remove entries from the end of the display list.  By
 * doing this, we try to maintain as much of the display as possible. 
 *
 * This has now been improved so that it returns enough information for
 * redraw_resized to redisplay the contents of the window without having
 * to redraw too much.
 */
void 
resize_window_display (Window * window)
{
	int cnt, i;
	Display *tmp;

	/*
	 * This is called in new_window to initialize the
	 * display the first time
	 */
	if (!window->top_of_scrollback)
	{
		window->top_of_scrollback = new_display_line (NULL);
		window->top_of_scrollback->line = NULL;
		window->top_of_scrollback->next = NULL;
		window->display_buffer_size = 1;
		window->display_ip = window->top_of_scrollback;
		window->top_of_display = window->top_of_scrollback;
		window->old_size = 1;
	}


	/*
	 * Find out how much the window has changed by
	 */
	cnt = window->display_size - window->old_size;

	tmp = window->top_of_display;

	/*
	 * If it got bigger, move the top_of_display back.
	 */
	if (tmp == window->display_ip)
		;
	else if (cnt > 0)
	{
		for (i = 0; i < cnt; i++)
		{
			if (!tmp || !tmp->prev)
				break;
			tmp = tmp->prev;
		}
	}

	/*
	 * If it got smaller, then move the top_of_display up
	 */
	else if (cnt < 0)
	{
		/* Use any whitespace we may have lying around */
		cnt += (window->old_size - window->distance_from_display);
		for (i = 0; i > cnt; i--)
		{
			if (tmp == window->display_ip)
				break;
			tmp = tmp->next;
		}
	}

	window->top_of_display = tmp;

	/*
	 * Look for where the cursor is.  If we're in scrollback or the
	 * window got smaller, or we're just plain lost, then assume
	 * we're at the bottom of the screen or something.
	 */
	window->cursor = 0;
	while (tmp != window->display_ip)
		window->cursor++, tmp = tmp->next;

	if (window->cursor > window->display_size)
		window->cursor = window->display_size;

	/*
	 * Mark the window for redraw and store the new window size.
	 */
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->old_size = window->display_size;
	recalculate_window_cursor (window);
	return;
}

/*
 * redraw_all_windows: This basically clears and redraws the entire display
 * portion of the screen.  All windows and status lines are draws.  This does
 * nothing for the input line of the screen.  Only visible windows are drawn 
 */
void 
redraw_all_windows (void)
{
	Window *tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		tmp->update = REDRAW_STATUS | REDRAW_DISPLAY_FAST;
}

/*
 * Rebalance_windows: this is called when you want all the windows to be
 * rebalanced, except for those who have a set size.
 */
void 
rebalance_windows (void)
{
	Window *tmp;
	int each, extra;
	int window_resized = 0, window_count = 0;


	/*
	 * Two passes -- first figure out how much we need to balance,
	 * and how many windows there are to balance
	 */
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size)
			continue;
		window_resized += tmp->display_size;
		window_count++;
	}

	each = window_resized / window_count;
	extra = window_resized % window_count;

	/*
	 * And then go through and fix everybody
	 */
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size)
			;
		else
		{
			tmp->display_size = each;
			if (extra)
				tmp->display_size++, extra--;
		}
	}
	recalculate_window_positions ();
}



/*
 * recalculate_windows: this is called when the terminal size changes (as
 * when an xterm window size is changed).  It recalculates the sized and
 * positions of all the windows.  Currently, all windows are rebalanced and
 * window size proportionality is lost 
 */
void 
recalculate_windows (void)
{
	int old_li = 1;
	int excess_li = 0;
	Window *tmp;
	int window_count = 0;
	int window_resized = 0;
	int offset;
	int split = 0;


	/*
	 * If its a new window, just set it and be done with it.
	 */
	if (!curr_scr_win)
	{
		current_screen->window_list->top = 0;
		current_screen->window_list->display_size = LI - 2;
		current_screen->window_list->bottom = LI - 2;
		old_li = LI;
		return;
	}

	/* 
	 * Expanding the screen takes two passes.  In the first pass,
	 * We figure out how many windows will be resized.  If none can
	 * be rebalanced, we add the whole shebang to the last one.
	 */
	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		old_li += tmp->display_size + tmp->double_status + 1;
		if (tmp->absolute_size && (window_count || tmp->next))
			continue;
		window_resized += tmp->display_size;
		window_count++;
	}

	excess_li = LI - old_li - split;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size && tmp->next)
			;
		else
		{
			/*
			 * The number of lines this window gets is:
			 * The number of lines available for resizing times 
			 * the percentage of the resizeable screen the window 
			 * covers.
			 */
			if (tmp->next)
				offset = (tmp->display_size * excess_li) /
					window_resized;
			else
				offset = excess_li;
			tmp->display_size += offset;
			if (tmp->display_size < 0)
				tmp->display_size = 1;
			excess_li -= offset;
		}
	}

	recalculate_window_positions ();
#if 0
	int base_size, size, top, extra;
	Window *tmp = current_screen->window_list;


	base_size = ((LI - 1) / current_screen->visible_windows) - 1;
	extra = (LI - 1) - ((base_size + 1) * current_screen->visible_windows);
	top = 0;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		tmp->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		if (extra)
		{
			extra--;
			size = base_size + 1;
		}
		else
			size = base_size;

		tmp->display_size = size - tmp->double_status;

		if (tmp->display_size <= 0)
			tmp->display_size = 1;
		tmp->top = top;
		tmp->bottom = top + size - tmp->double_status;
		top += size + 1;
	}
#endif
}

/*
 * update_all_windows: This goes through each visible window and draws the
 * necessary portions according the the update field of the window. 
 */
void 
update_all_windows ()
{
	Window *tmp;
	int fast_window, full_window, r_status, u_status;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->display_size != tmp->old_size)
			resize_window_display (tmp);
		if (tmp->update)
		{
			fast_window = tmp->update & REDRAW_DISPLAY_FAST;
			full_window = tmp->update & REDRAW_DISPLAY_FULL;
			r_status = tmp->update & REDRAW_STATUS;
			u_status = tmp->update & UPDATE_STATUS;
			if (full_window || fast_window)
				repaint_window (tmp, 0, -1);

			if (r_status)
				update_window_status (tmp, 1);
			else if (u_status)
				update_window_status (tmp, 0);
		}
		tmp->update = 0;
	}
	for (tmp = invisible_list; tmp; tmp = tmp->next)
	{
		if (tmp->display_size != tmp->old_size)
			resize_window_display (tmp);
		tmp->update = 0;
	}
	update_input (UPDATE_JUST_CURSOR);
	term_flush ();
}

/*
 * set_current_window: This sets the "current" window to window.  It also
 * keeps track of the last_current_screen->current_window by setting it to the
 * previous current window.  This assures you that the new current window is
 * visible.
 * If not, a new current window is chosen from the window list 
 */
void 
set_current_window (Window * window)
{
	Window *tmp;
	unsigned int refnum;

	refnum = current_screen->last_window_refnum;
	if (curr_scr_win)
	{
		curr_scr_win->update |= UPDATE_STATUS;
		current_screen->last_window_refnum = curr_scr_win->refnum;
	}
	if ((window == NULL) || (!window->visible))
	{
		if ((tmp = get_window_by_refnum (refnum)) && (tmp->visible))
			curr_scr_win = tmp;
		else
			curr_scr_win = get_next_window ();
	}
	else
		curr_scr_win = window;
	curr_scr_win->update |= UPDATE_STATUS;
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
	update_input (UPDATE_ALL);
}

/*
 * goto_window: This will switch the current window to the window numbered
 * "which", where which is 0 through the number of visible windows on the
 * screen.  The which has nothing to do with the windows refnum. 
 */
static void 
goto_window (int which)
{
	Window *tmp;
	int i;


	if (which == 0)
		return;
	if ((which < 0) || (which > current_screen->visible_windows))
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("GOTO: Illegal value");
		return;
	}
	tmp = current_screen->window_list;
	for (i = 1; tmp && (i != which); tmp = tmp->next, i++)
		;
	set_current_window (tmp);
}

/*
 * hide_window: sets the given window to invisible and recalculates remaing
 * windows to fill the entire screen 
 */
void 
hide_window (Window * window)
{
	if (current_screen->visible_windows == 1)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("You can't hide the last window.");
		return;
	}
	if (window->visible)
	{
		remove_from_window_list (window);
		add_to_invisible_list (window);
/*              window->display_size = LI - 2 - window->double_status; */
		window->top = 0;
		window->bottom = window->display_size;
		set_current_window (NULL);
	}
}

/*
 * swap_last_window:  This swaps the current window with the last window
 * that was hidden.
 */

void 
swap_last_window (char key, char *ptr)
{
	if (invisible_list == NULL)
	{
		/* say("There are no hidden windows"); */
		/* Not sure if we need to warn   - phone. */
		return;
	}
	swap_window (curr_scr_win, invisible_list);
	message_from (NULL, LOG_CRAP);
	update_all_windows ();
	cursor_to_input ();
}

/*
 * swap_next_window:  This swaps the current window with the next hidden
 * window.
 */

void 
swap_next_window (char key, char *ptr)
{
	Window *tmp = NULL;
	Window *next = NULL;
	Window *smallest = NULL;

	if (invisible_list == NULL)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("There are no hidden windows");
		return;
	}

	smallest = curr_scr_win;
	while (traverse_all_windows (&tmp))
	{
		if (!tmp->visible)
		{
			if (tmp->refnum < smallest->refnum)
				smallest = tmp;
			if ((tmp->refnum > curr_scr_win->refnum)
			    && (!next || tmp->refnum < next->refnum))
				next = tmp;
		}
	}
	if (!next)
		next = smallest;

	swap_window (curr_scr_win, next);
	message_from (NULL, LOG_CRAP);
	update_all_windows ();
	update_all_status (curr_scr_win, NULL, 0);
	cursor_to_input ();
}

/*
 * swap_previous_window:  This swaps the current window with the next 
 * hidden window.
 */

void 
swap_previous_window (char key, char *ptr)
{
	Window *tmp;
	int previous = 0;
	int largest;

	if (invisible_list == NULL)
	{
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("There are no hidden windows");
		return;
	}
	tmp = NULL;
	largest = curr_scr_win->refnum;
	while (traverse_all_windows (&tmp))
	{
		if (!tmp->visible)
		{
			if (tmp->refnum > largest)
				largest = tmp->refnum;
			if ((tmp->refnum < curr_scr_win->refnum)
			    && (previous < tmp->refnum))
				previous = tmp->refnum;
		}
	}
	if (previous)
		tmp = get_window_by_refnum (previous);
	else
		tmp = get_window_by_refnum (largest);
	swap_window (curr_scr_win, tmp);
	message_from (NULL, LOG_CRAP);
	update_all_windows ();
	update_all_status (curr_scr_win, NULL, 0);
	cursor_to_input ();
}

/* show_window: This makes the given window visible.  */
void 
show_window (Window * window)
{
	if (window->visible)
	{
		set_current_window (window);
		return;
	}
	remove_from_invisible_list (window);
	if (add_to_window_list (window))
		set_current_window (window);
	else
		add_to_invisible_list (window);
}

/* 
 * XXX i have no idea if this belongs here.
 */
char *
get_status_by_refnum (u_int refnum, unsigned line)
{
	Window *the_window;

	if ((the_window = get_window_by_refnum (refnum)))
	{
		if (line > the_window->double_status)
			return empty_string;

		return the_window->status_line[line];
	}
	else
		return empty_string;
}

/*
 * get_window_by_refnum: Given a reference number to a window, this returns a
 * pointer to that window if a window exists with that refnum, null is
 * returned otherwise.  The "safe" way to reference a window is throught the
 * refnum, since a window might be delete behind your back and and Window
 * pointers might become invalid.  
 */
Window *
get_window_by_refnum (unsigned int refnum)
{
	Window *tmp = NULL;

	if (refnum)
	{
		while (traverse_all_windows (&tmp))
		{
			if (tmp->refnum == refnum)
				return tmp;
		}
	}
	else
		return curr_scr_win;
	return NULL;
}

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
int 
get_visible_by_refnum (char *args)
{
	char *arg;
	Window *tmp;

	if ((arg = next_arg (args, &args)) != NULL)
	{
		if (is_number (arg))
		{
			if ((tmp = get_window_by_refnum (my_atol (arg))) != NULL)
				return tmp->visible;
		}
		if ((tmp = get_window_by_name (arg)) != NULL)
			return tmp->visible;

		return -1;
	}
	return -1;
}

/*
 * get_window_by_name: returns a pointer to a window with a matching logical
 * name or null if no window matches 
 */
Window *
get_window_by_name (char *name)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
	{
		if (tmp->name && (my_stricmp (tmp->name, name) == 0))
			return tmp;
	}
	return NULL;
}


/*
 * get_next_window: This returns a pointer to the next *visible* window in
 * the window list.  It automatically wraps at the end of the list back to
 * the beginning of the list 
 */
static Window *
get_next_window (void)
{
	Window *last = curr_scr_win;
	Window *new = curr_scr_win;

	do
	{
		if (new->next)
			new = new->next;
		else
			new = current_screen->window_list;
	}
	while (new->skip && new != last);
	return new;
}

/*
 * get_previous_window: this returns the previous *visible* window in the
 * window list.  This automatically wraps to the last window in the window
 * list 
 */
static Window *
get_previous_window (void)
{
	Window *last = curr_scr_win;
	Window *new = curr_scr_win;

	do
	{
		if (new->prev)
			new = new->prev;
		else
			new = current_screen->window_list_end;
	}
	while (new->skip && new != last);

	return new;
}

/*
 * next_window: This switches the current window to the next visible window 
 */
void 
next_window (char key, char *ptr)
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window (get_next_window ());
	update_all_windows ();
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
}


/*
 * previous_window: This switches the current window to the previous visible
 * window 
 */
void 
previous_window (char key, char *ptr)
{
	if (current_screen->visible_windows == 1)
		return;
	set_current_window (get_previous_window ());
	update_all_windows ();
	set_input_prompt (curr_scr_win, get_string_var (INPUT_PROMPT_VAR), 0);
}



/*
 * update_window_status: This updates the status line for the given window.
 * If the refresh flag is true, the entire status line is redrawn.  If not,
 * only some of the changed portions are redrawn 
 */
void 
update_window_status (Window * window, int refresh)
{
	if ((!window->visible) || !status_update_flag || never_connected)
		return;
	if (window == NULL)
		window = curr_scr_win;
	if (refresh)
	{
		new_free (&(window->status_line[0]));
		new_free (&(window->status_line[1]));
	}
	make_status (window);
}

/*
 * update_all_status: This updates all of the status lines for all of the
 * windows.  By updating, it only draws from changed portions of the status
 * line to the right edge of the screen 
 */
void 
update_all_status (Window * win, char *unused, int unused1)
{
	Window *window;
	Screen *screen;

	if (!status_update_flag || never_connected)
		return;
	for (screen = screen_list; screen; screen = screen->next)
	{
		if (!screen->alive)
			continue;
		for (window = screen->window_list; window; window = window->next)
			if (window->visible)
				make_status (window);
	}
	update_input (UPDATE_JUST_CURSOR);
	term_flush ();
}


/*
 * status_update: sets the status_update_flag to whatever flag is.  This also
 * calls update_all_status(), which will update the status line if the flag
 * was true, otherwise it's just ignored 
 */
void 
status_update (int flag)
{
	status_update_flag = flag;
	update_all_status (curr_scr_win, NULL, 0);
	cursor_to_input ();
}


/*
 * set_prompt_by_refnum: changes the prompt for the given window.  A window
 * prompt will be used as the target in place of the query user or current
 * channel if it is set 
 */
void 
set_prompt_by_refnum (unsigned int refnum, char *prompt)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	malloc_strcpy (&(tmp->prompt), prompt);
}

/* get_prompt_by_refnum: returns the prompt for the given window refnum */
char *
get_prompt_by_refnum (unsigned int refnum)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	if (tmp->prompt)
		return (tmp->prompt);
	else
		return (empty_string);
}


/* query_nick: Returns the query nick for the current channel */
char *
query_nick (void)
{
	return (curr_scr_win->query_nick);
}

/* query_nick: Returns the query nick for the current channel */
char *
query_host (void)
{
	return (curr_scr_win->query_host);
}

char *
query_cmd (void)
{
	return (curr_scr_win->query_cmd);
}

/*
 * get_target_by_refnum: returns the target for the window with the given
 * refnum (or for the current window).  The target is either the query nick
 * or current channel for the window 
 */
char *
get_target_by_refnum (unsigned int refnum)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	if (tmp->query_nick)
		return (tmp->query_nick);
	else if (tmp->current_channel)
		return (tmp->current_channel);
	else
		return (NULL);
}

char *
get_target_cmd_by_refnum (unsigned int refnum)
{
	Window *tmp;
	if (!(tmp = get_window_by_refnum (refnum)))
		tmp = curr_scr_win;
	if (tmp->query_cmd)
		return (tmp->query_cmd);
	return NULL;
}

/* set_query_nick: sets the query nick for the current channel to nick */
void 
set_query_nick (char *nick, char *host, char *cmd)
{
	char *ptr;
	NickList *tmp;
	if (!nick && !host && cmd)
	{
		curr_scr_win->query_cmd = m_strdup (cmd);
		return;
	}
	if (curr_scr_win->query_nick)
	{
		char *nick;

		nick = curr_scr_win->query_nick;
/* next_in_comma_list() */
		while (nick)
		{
			if (!curr_scr_win->nicks)
				continue;
			if ((ptr = (char *) strchr (nick, ',')) != NULL)
				*(ptr++) = 0;
			if ((tmp = (NickList *) remove_from_list ((List **) & (curr_scr_win->nicks), nick)) != NULL)
			{
				new_free (&tmp->nick);
				new_free (&tmp->host);	/* CDE why was this not done */
				new_free ((char **) &tmp);
			}
			nick = ptr;
		}
		new_free (&curr_scr_win->query_nick);
		new_free (&curr_scr_win->query_host);
		new_free (&curr_scr_win->query_cmd);
	}
	if (nick)
	{
		malloc_strcpy (&(curr_scr_win->query_nick), nick);
		malloc_strcpy (&(curr_scr_win->query_host), host);
		if (cmd)
			malloc_strcpy (&(curr_scr_win->query_cmd), cmd);
		curr_scr_win->update |= UPDATE_STATUS;
		while (nick)
		{
			if ((ptr = (char *) strchr (nick, ',')) != NULL)
				*(ptr++) = 0;
			tmp = (NickList *) new_malloc (sizeof (NickList));
			tmp->nick = NULL;
			malloc_strcpy (&tmp->nick, nick);
			malloc_strcpy (&tmp->host, host);
			add_to_list ((List **) & (curr_scr_win->nicks), (List *) tmp);
			nick = ptr;
		}
	}
	update_window_status (curr_scr_win, 0);
}

/*
 * is_current_channel: Returns true is channel is a current channel for any
 * window.  If the delete flag is not 0, then unset channel as the current
 * channel and attempt to replace it by a non-current channel or the 
 * current_channel of window specified by value of delete
 */
int 
is_current_channel (char *channel, int server, int delete)
{

	Window *tmp = NULL;
	int found = 0;

	while (traverse_all_windows (&tmp))
	{
		if (tmp->current_channel &&
		    !my_stricmp (channel, tmp->current_channel) &&
		    (tmp->server == from_server))
		{
			found = 1;
			if (delete)
			{
				new_free (&(tmp->current_channel));
				tmp->update |= UPDATE_STATUS;
			}
		}
	}
	return found;
}

/*
 * set_current_channel_by_refnum: This sets the current channel for the current
 * window. It returns the current channel as it's value.  If channel is null,
 * the * current channel is not changed, but simply reported by the function
 * result.  This treats as a special case setting the current channel to
 * channel "0".  This frees the current_channel for the
 * current_screen->current_window, * setting it to null 
 */
char *
set_current_channel_by_refnum (unsigned int refnum, char *channel)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	if (channel)
		if (strcmp (channel, zero) == 0)
			channel = NULL;
	malloc_strcpy (&tmp->current_channel, channel);
	new_free (&tmp->waiting_channel);
	tmp->update |= UPDATE_STATUS;
	set_channel_window (tmp, channel, tmp->server);
	return (channel);
}

/* get_current_channel_by_refnum: returns the current channel for window refnum */
char *
get_current_channel_by_refnum (unsigned int refnum)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	return (tmp->current_channel);
}

char *
get_refnum_by_window (const Window * w)
{
	return ltoa (w->refnum);
}


int 
is_bound_to_window (const Window * window, const char *channel)
{
	return (window->bind_channel ?
		!my_stricmp (window->bind_channel, channel) : 0);
}

Window *
get_window_bound_channel (const char *channel)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
		if (tmp->bind_channel && !my_stricmp (tmp->bind_channel, channel))
			return tmp;

	return NULL;
}

int 
is_bound_anywhere (const char *channel)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
		if (tmp->bind_channel && !my_stricmp (tmp->bind_channel, channel))
			return 1;
	return 0;
}

extern int 
is_bound (const char *channel, int server)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
		if (tmp->server == server && tmp->bind_channel
		    && !my_stricmp (tmp->bind_channel, channel))
			return 1;
	return 0;
}

void 
unbind_channel (const char *channel, int server)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
		if (tmp->server == server && tmp->bind_channel
		    && !my_stricmp (tmp->bind_channel, channel))
		{
			new_free (&tmp->bind_channel);
			/* ARGH! COREDUMPS! */
			tmp->bind_channel = NULL;
			return;
		}
}

char *
get_bound_channel (Window * window)
{
	Window *tmp = NULL;

	while (traverse_all_windows (&tmp))
		if (tmp == window)
			return tmp->bind_channel;
	return empty_string;
}

/*
 * get_window_server: returns the server index for the window with the given
 * refnum 
 */
int 
get_window_server (unsigned int refnum)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	return (tmp->server);
}

/*
 * set_window_server:  This sets the server of the given window to server. 
 * If refnum is -1 then we are setting the primary server and all windows
 * that are set to the current primary server are changed to server.  The misc
 * flag is ignored in this case.  If refnum is not -1, then that window is
 * set to the given server.  If the misc flag is set as well, then all windows
 * with the same server as renum are set to the new server as well 
 * if refnum == -2, then, we are setting the server group passed in the misc
 * variable to the server.
 */
void 
set_window_server (int refnum, int server, int misc)
{
	Window *tmp, *window;
	int old;

	if (refnum == -1)
	{
		tmp = NULL;
		while (traverse_all_windows (&tmp))
		{
			if (tmp->server == primary_server)
				tmp->server = server;
		}
		window_check_servers ();
		primary_server = server;
	}
	else
	{
		if ((window = get_window_by_refnum (refnum)) == NULL)
			window = curr_scr_win;
		old = window->server;
		if (misc)
		{
			tmp = NULL;
			while (traverse_all_windows (&tmp))
			{
				if (tmp->server == old)
					tmp->server = server;
			}
		}
		else
			window->server = server;
		window_check_servers ();
	}
}

/*
 * window_check_servers: this checks the validity of the open servers vs the
 * current window list.  Every open server must have at least one window
 * associated with it.  If a window is associated with a server that's no
 * longer open, that window's server is set to the primary server.  If an
 * open server has no assicatiate windows, that server is closed.  If the
 * primary server is no more, a new primary server is picked from the open
 * servers 
 */

void 
window_check_servers (void)
{
	Window *tmp;
	int cnt, max, i, not_connected, prime = -1;

	connected_to_server = 0;
	max = number_of_servers;
	for (i = 0; i < max; i++)
	{
		not_connected = !is_server_open (i);
		cnt = 0;
		tmp = NULL;
		while (traverse_all_windows (&tmp))
		{
			if (tmp->server == i)
			{
				if (not_connected)
					tmp->server = primary_server;
				else
				{
					prime = tmp->server;
					cnt++;
				}
			}
		}
		if (cnt == 0)
		{
			close_server (i, empty_string);
/*                      add_timer("", 10, timed_server, m_sprintf("%d", i), NULL); */
		}
		else
			connected_to_server++;
	}
	if (!is_server_open (primary_server))
	{
		tmp = NULL;
		while (traverse_all_windows (&tmp))
			if (tmp->server == primary_server)
				tmp->server = prime;
		primary_server = prime;
	}
	update_all_status (curr_scr_win, NULL, 0);
	cursor_to_input ();
}

/*
 * set_level_by_refnum: This sets the window level given a refnum.  It
 * revamps the windows levels as well using revamp_window_levels() 
 */
void 
set_level_by_refnum (unsigned int refnum, int level)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	tmp->window_level = level;
	revamp_window_levels (tmp);
}

/*
 * revamp_window_levels: Given a level setting for the current window, this
 * makes sure that that level setting is unused by any other window. Thus
 * only one window in the system can be set to a given level.  This only
 * revamps levels for windows with servers matching the given window 
 * it also makes sure that only one window has the level `DCC', as this is
 * not dependant on a server.
 */
void 
revamp_window_levels (Window * window)
{
	Window *tmp;
	int got_dcc;

	got_dcc = (LOG_DCC & window->window_level) ? 1 : 0;
	tmp = NULL;
	while (traverse_all_windows (&tmp))
	{
		if (tmp == window)
			continue;
		if (LOG_DCC & tmp->window_level)
		{
			if (0 != got_dcc)
				tmp->window_level &= ~LOG_DCC;
			got_dcc = 1;
		}
		if (window->server == tmp->server)
			tmp->window_level ^= (tmp->window_level & window->window_level);
	}
}

/*
 * message_to: This allows you to specify a window (by refnum) as a
 * destination for messages.  Used by EXEC routines quite nicely 
 */
void 
message_to (unsigned int refnum)
{
	to_window = (refnum) ? get_window_by_refnum (refnum) : NULL;
}

/*
 * save_message_from: this is used to save (for later restoration) the
 * who_from variable.  This comes in handy very often when a routine might
 * call another routine that might change who_from. 
 */
void 
save_message_from (char **saved_who_from, int *saved_who_level)
{
	*saved_who_from = who_from;
	*saved_who_level = who_level;
}

/* restore_message_from: restores a previously saved who_from variable */
void 
restore_message_from (char *saved_who_from, int saved_who_level)
{
	who_from = saved_who_from;
	who_level = saved_who_level;
}

/*
 * message_from: With this you can the who_from variable and the who_level
 * variable, used by the display routines to decide which window messages
 * should go to.  
 */
void 
message_from (char *who, int level)
{
#if 0
	malloc_strcpy (&who_from, who);
#endif
	who_from = who;
	who_level = level;
}

/*
 * message_from_level: Like set_lastlog_msg_level, except for message_from.
 * this is needed by XECHO, because we could want to output things in more
 * than one level.
 */
int 
message_from_level (int level)
{
	int temp;

	temp = who_level;
	who_level = level;
	return temp;
}


/*
 * clear_window: This clears the display list for the given window, or
 * current window if null is given.  
 */
void 
clear_window (Window * window)
{
	if (window == NULL)
		window = curr_scr_win;
	window->top_of_display = window->display_ip;
	window->cursor = 0;
	window->held_displayed = 0;
	repaint_window (window, 0, -1);
	update_window_status (window, 1);
}

/* clear_all_windows: This clears all *visible* windows */
void 
clear_all_windows (int unhold)
{
	Window *tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
	{
		if (unhold)
			hold_mode (tmp, OFF, 1);
		clear_window (tmp);
	}
}

/*
 * clear_window_by_refnum: just like clear_window(), but it uses a refnum. If
 * the refnum is invalid, the current window is cleared. 
 */
void 
clear_window_by_refnum (unsigned int refnum)
{
	Window *tmp;

	if ((tmp = get_window_by_refnum (refnum)) == NULL)
		tmp = curr_scr_win;
	clear_window (tmp);
}


/*
 * set_scroll_lines: called by /SET SCROLL_LINES to check the scroll lines
 * value 
 */
void 
set_scroll_lines (Window * win, char *unused, int size)
{
	if (size == 0)
	{
		set_int_var (SCROLL_VAR, 0);
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else if (size > curr_scr_win->display_size)
	{
		say ("Maximum lines that may be scrolled is %d",
		     curr_scr_win->display_size);
		set_int_var (SCROLL_LINES_VAR, curr_scr_win->display_size);
	}
}

/*
 * set_scroll: called by /SET SCROLL to make sure the SCROLL_LINES variable
 * is set correctly 
 */
void 
set_scroll (Window * win, char *unused, int value)
{
	int old_value;

	if (value && (get_int_var (SCROLL_LINES_VAR) == 0))
	{
		put_it ("You must set SCROLL_LINES to a positive value first!");
		if (curr_scr_win)
			curr_scr_win->scroll = 0;
	}
	else
	{
		if (curr_scr_win)
		{
			old_value = curr_scr_win->scroll;
			curr_scr_win->scroll = value;
			if (old_value && !value)
				scroll_window (curr_scr_win);
		}
	}
}

/*
 * set_continued_lines: checks the value of CONTINUED_LINE for validity,
 * altering it if its no good 
 */
void 
set_continued_lines (Window * win, char *value, int unused)
{
	if (value && ((int) strlen (value) > (CO / 2)))
		value[CO / 2] = '\0';
}

/* current_refnum: returns the reference number for the current window */
unsigned int 
current_refnum (void)
{
	return (curr_scr_win->refnum);
}

int 
number_of_windows (void)
{
	return (current_screen->visible_windows);
}

/*
 * is_window_name_unique: checks the given name vs the names of all the
 * windows and returns true if the given name is unique, false otherwise 
 */
int 
is_window_name_unique (char *name)
{
	Window *tmp = NULL;

	if (name)
	{
		while (traverse_all_windows (&tmp))
		{
			if (tmp->name && (my_stricmp (tmp->name, name) == 0))
				return (0);
		}
	}
	return (1);
}

#define WIN_FORM "%-4s %*.*s %*.*s %*.*s %-9.9s %-10.10s %s%s"
static void 
list_a_window (Window * window, int len)
{
	int cnw = get_int_var (CHANNEL_NAME_WIDTH_VAR);

	say (WIN_FORM, ltoa (window->refnum),
	     12, 12, get_server_nickname (window->server),
	     len, len, window->name ? window->name : "<None>",
	     cnw, cnw, window->current_channel ? window->current_channel : "<None>",
	     window->query_nick ? window->query_nick : "<None>",
	     get_server_itsname (window->server),
	     bits_to_lastlog_level (window->window_level),
	     window->visible ? empty_string : " Hidden");
}

/* below is stuff used for parsing of WINDOW command */

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
static Window *
get_window (char *name, char **args)
{
	char *arg;
	Window *tmp;

	if ((arg = next_arg (*args, args)) != NULL)
	{
		if (is_number (arg))
		{
			if ((tmp = get_window_by_refnum (my_atol (arg))) != NULL)
				return (tmp);
		}
		if ((tmp = get_window_by_name (arg)) != NULL)
			return (tmp);
		if (!get_int_var (WINDOW_QUIET_VAR))
			say ("%s: No such window: %s", name, arg);
	}
	else
		say ("%s: Please specify a window refnum or name", name);
	return (NULL);
}

/*
 * get_invisible_window: parses out an invisible window by reference number.
 * Returns the pointer to the window, or null.  The args can also be "LAST"
 * indicating the top of the invisible window list (and thus the last window
 * made invisible) 
 */
static Window *
get_invisible_window (char *name, char **args)
{
	char *arg;
	Window *tmp;

	if ((arg = next_arg (*args, args)) != NULL)
	{
		if (my_strnicmp (arg, "LAST", strlen (arg)) == 0)
		{
			if (invisible_list == NULL)
			{
				if (!get_int_var (WINDOW_QUIET_VAR))
					say ("%s: There are no hidden windows", name);
			}
			return (invisible_list);
		}
		if ((tmp = get_window (name, &arg)) != NULL)
		{
			if (!tmp->visible)
				return (tmp);
			else if (!get_int_var (WINDOW_QUIET_VAR))
			{
				if (tmp->name)
					say ("%s: Window %s is not hidden!",
					     name, tmp->name);
				else
					say ("%s: Window %d is not hidden!",
					     name, tmp->refnum);
			}
		}
	}
	else
		say ("%s: Please specify a window refnum or LAST", name);
	return (NULL);
}

/* get_number: parses out an integer number and returns it */
static int 
get_number (char *name, char **args, char *msg)
{
	char *arg;

	if ((arg = next_arg (*args, args)) != NULL)
		return (my_atol (arg));
	else
	{
		if (msg)
			say ("%s: %s", name, msg);
		else
			say ("%s: You must specify the number of lines", name);
	}
	return (0);
}

/*
 * get_boolean: parses either ON, OFF, or TOGGLE and sets the var
 * accordingly.  Returns 0 if all went well, -1 if a bogus or missing value
 * was specified 
 */
static int 
get_boolean (char *name, char **args, int *var)
{
	char *arg;

	if (((arg = next_arg (*args, args)) == NULL) || do_boolean (arg, var))
	{
		say ("Value for %s must be ON, OFF, or TOGGLE", name);
		return (-1);
	}
	else
	{
		say ("Window %s is %s", name, var_settings[*var]);
		return (0);
	}
}



/*
 * /WINDOW ADD nick<,nick>
 * Adds a list of one or more nicknames to the current list of usupred
 * targets for the current window.  These are matched up with the nick
 * argument for message_from().
 */
static Window *
window_add (Window * window, char **args, char *usage)
{
	char *ptr;
	NickList *new;
	char *arg = next_arg (*args, args);

	if (!arg)
		say ("ADD: Add nicknames to be redirected to this window");

	else
		while (arg)
		{
			if ((ptr = strchr (arg, ',')))
				*ptr++ = 0;
			if (!find_in_list ((List **) & window->nicks, arg, !USE_WILDCARDS))
			{
				say ("Added %s to window name list", arg);
				new = (NickList *) new_malloc (sizeof (NickList));
				new->nick = m_strdup (arg);
				add_to_list ((List **) & (window->nicks), (List *) new);
			}
			else
				say ("%s already on window name list", arg);

			arg = ptr;
		}

	return window;
}

/*
 * /WINDOW BACK
 * Changes the current window pointer to the window that was most previously
 * the current window.  If that window is now hidden, then it is swapped with
 * the current window.
 */
static Window *
window_back (Window * window, char **args, char *usage)
{
	Window *tmp;

	tmp = get_window_by_refnum (current_screen->last_window_refnum);
	if (tmp->visible)
		set_current_window (tmp);
	else
	{
		swap_window (curr_scr_win, tmp);
		message_from (NULL, LOG_CRAP);
		update_all_windows ();
		update_all_status (curr_scr_win, NULL, 0);
		cursor_to_input ();
	}
	return window;
}

/*
 * /WINDOW BALANCE
 * Causes all of the windows on the current screen to be adjusted so that 
 * the largest window on the screen is no more than one line larger than
 * the smallest window on the screen.
 */
static Window *
window_balance (Window * window, char **args, char *usage)
{
	rebalance_windows ();
	return window;
}

/*
 * /WINDOW BEEP_ALWAYS ON|OFF
 * Indicates that when this window is HIDDEN (sorry, thats not what it seems
 * like it should do, but that is what it does), beeps to this window should
 * not be suppressed like they normally are for hidden windows.  The beep
 * occurs EVEN IF /set beep is OFF.
 */
static Window *
window_beep_always (Window * window, char **args, char *usage)
{
	if (get_boolean ("BEEP_ALWAYS", args, &window->beep_always))
		return NULL;
	return window;
}

/*
 * /WINDOW BIND <#channel>
 * Indicates that the window should be "bound" to the specified channel.
 * "binding" a channel to a window means that the channel will always 
 * belong to this window, no matter what.  For example, if a channel is
 * bound to a window, you can do a /join #channel in any window, and it 
 * will always "join" in this window.  This is especially useful when
 * you are disconnected from a server, because when you reconnect, the client
 * often loses track of which channel went to which window.  Binding your
 * channels gives the client a hint where channels belong.
 *
 * You can rebind a channel to a new window, even after it has already
 * been bound elsewhere.
 */
static Window *
window_bind (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		if (!is_channel (arg))
			say ("BIND: %s is not a valid channel name", arg);
		else
		{
			if (is_bound (arg, from_server) && is_current_channel (arg, from_server, 1))
				unbind_channel (arg, from_server);

			malloc_strcpy (&window->bind_channel, arg);
			if (is_on_channel (arg, from_server, get_server_nickname (window->server)))
				set_current_channel_by_refnum (0, arg);
			say ("Window is bound to channel %s", arg);
		}
	}
	else if ((arg = get_bound_channel (window)))
		say ("Window is bound to channel %s", arg);
	else
		say ("Window is not bound to any channel");

	return window;
}

/*
 * /WINDOW CHANNEL <#channel>
 * Directs the client to make a specified channel the current channel for
 * the window -- it will JOIN the channel if you are not already on it.
 * If the channel you wish to activate is bound to a different window, you
 * will be notified.  If you are already on the channel in another window,
 * then the channel's window will be switched.  If you do not specify a
 * channel, or if you specify the channel "0", then the window will drop its
 * connection to whatever channel it is in.
 */
static Window *
window_channel (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		if (is_bound (arg, from_server))
			say ("Channel %s is already bound elsewhere", arg);
		else if (is_current_channel (arg, from_server, 1) ||
			 is_on_channel (arg, from_server, get_server_nickname (window->server)))
		{
			say ("You are now talking to channel %s", arg);
			set_current_channel_by_refnum (0, arg);
		}
		else if (arg[0] == '0' && arg[1] == 0)
			set_current_channel_by_refnum (0, NULL);
		else
		{
			int server = from_server;
			from_server = window->server;
			send_to_server ("JOIN %s", arg);
			malloc_strcpy (&window->waiting_channel, arg);
			from_server = server;
		}
	}
	else
		set_current_channel_by_refnum (0, zero);

	return window;
}


/*
 * /WINDOW DESCRIBE
 * Directs the client to tell you a bit about the current window.
 * This is the 'default' argument to the /window command.
 */
static Window *
window_describe (Window * window, char **args, char *usage)
{
	if (window->name)
		say ("Window %s (%u)", window->name, window->refnum);
	else
		say ("Window %u", window->refnum);

	say ("\tServer: %s",
	     window->server == -1 ?
	     get_server_name (window->server) : "<None>");
	say ("\tGeometry Info: [%d %d %d %d %d %d]",
	     window->top, window->bottom,
	     window->held_displayed, window->display_size,
	     window->cursor, window->distance_from_display);

	say ("\tCO, LI are [%d %d]", CO, LI);
	say ("\tCurrent channel: %s",
	     window->current_channel ?
	     window->current_channel : "<None>");

	if (window->waiting_channel)
		say ("\tWaiting channel: %s",
		     window->waiting_channel);

	if (window->bind_channel)
		say ("\tBound channel: %s",
		     window->bind_channel);
	say ("\tQuery User: %s",
	     window->query_nick ?
	     window->query_nick : "<None>");
	say ("\tPrompt: %s",
	     window->prompt ?
	     window->prompt : "<None>");
	say ("\tSecond status line is %s", var_settings[window->double_status]);
	say ("\tScrolling is %s", var_settings[window->scroll]);
	say ("\tLogging is %s", var_settings[window->log]);

	if (window->logfile)
		say ("\tLogfile is %s", window->logfile);
	else
		say ("\tNo logfile given");

	say ("\tNotification is %s",
	     var_settings[window->miscflags & WINDOW_NOTIFY]);
	say ("\tHold mode is %s",
	     var_settings[window->hold_mode]);
	say ("\tWindow level is %s",
	     bits_to_lastlog_level (window->window_level));
	say ("\tLastlog level is %s",
	     bits_to_lastlog_level (window->lastlog_level));
	say ("\tNotify level is %s",
	     bits_to_lastlog_level (window->notify_level));

	if (window->nicks)
	{
		NickList *tmp;
		say ("\tName list:");
		for (tmp = window->nicks; tmp; tmp = tmp->next)
			say ("\t  %s", tmp->nick);
	}

	return window;
}

/*
 * /WINDOW DOUBLE ON|OFF
 * This directs the client to enable or disable the supplimentary status bar.
 * When the "double status bar" is enabled, the status format for the second
 * bar is taken from STATUS_FORMAT2.
 *
 */
static Window *
window_double (Window * window, char **args, char *usage)
{
	int current = window->double_status;

	if (get_boolean ("DOUBLE", args, &window->double_status))
		return NULL;

	window->display_size += current - window->double_status;
	recalculate_window_positions ();
	redraw_all_windows ();
	build_status (window, NULL, 0);
	return window;
}

/*
 * /WINDOW FIXED (ON|OFF)
 *
 * When this is ON, then this window will never be used as the implicit
 * partner for a window resize.  That is to say, if you /window grow another
 * window, this window will not be considered for the corresponding shrink.
 * You may /window grow a fixed window, but if you do not have other nonfixed
 * windows, the grow will fail.
 */
static Window *
window_fixed (Window * window, char **args, char *usage)
{
	if (get_boolean ("FIXED", args, &window->absolute_size))
		return NULL;
	return window;
}

/*
 * /WINDOW GOTO refnum
 * This switches the current window selection to the window as specified
 * by the numbered refnum.
 */
static Window *
window_goto (Window * window, char **args, char *usage)
{
	goto_window (get_number ("GOTO", args, NULL));
	from_server = get_window_server (0);
	return curr_scr_win;
}

/*
 * /WINDOW GROW lines
 * This directs the client to expand the specified window by the specified
 * number of lines.  The number of lines should be a positive integer, and
 * the window's growth must not cause another window to be smaller than
 * the minimum of 3 lines.
 */
static Window *
window_grow (Window * window, char **args, char *usage)
{
	resize_window (RESIZE_REL, window, get_number ("GROW", args, NULL));
	return window;
}

/*
 * /WINDOW HIDE
 * This directs the client to remove the specified window from the current
 * (visible) screen and place the window on the client's invisible list.
 * A hidden window has no "screen", and so can not be seen, and does not
 * have a size.  It can be unhidden onto any screen.
 */
static Window *
window_hide (Window * window, char **args, char *usage)
{
	hide_window (window);
	return curr_scr_win;
}

/*
 * /WINDOW HIDE_OTHERS
 * This directs the client to place *all* windows on the current screen,
 * except for the current window, onto the invisible list.
 */
static Window *
window_hide_others (Window * window, char **args, char *usage)
{
	Window *tmp, *cur, *next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
			hide_window (tmp);
		tmp = next;
	}
	return curr_scr_win;
}

/*
 * /WINDOW HOLD_MODE
 * This arranges for the window to "hold" any output bound for it once
 * a full page of output has been completed.  Setting the global value of
 * HOLD_MODE is truly bogus and should be changed. XXXX
 */
static Window *
window_hold_mode (Window * window, char **args, char *usage)
{
	if (get_boolean ("HOLD_MODE", args, &window->hold_mode))
		return NULL;

	set_int_var (HOLD_MODE_VAR, window->hold_mode);
	return window;
}

/*
 * /WINDOW KILL
 * This arranges for the current window to be destroyed.  Once a window
 * is killed, it cannot be recovered.  Because every server must have at
 * least one window "connected" to it, if you kill the last window for a
 * server, the client will drop your connection to that server automatically.
 */
static Window *
window_kill (Window * window, char **args, char *usage)
{
	delete_window (window);
	return curr_scr_win;
}

/*
 * /WINDOW KILL_OTHERS
 * This arranges for all windows on the current screen, other than the 
 * current window to be destroyed.  Obviously, the current window will be
 * the only window left on the screen.  Connections to servers other than
 * the server for the current window will be implicitly closed.
 */
static Window *
window_kill_others (Window * window, char **args, char *usage)
{
	Window *tmp, *cur, *next;

	cur = curr_scr_win;
	tmp = current_screen->window_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != cur)
			delete_window (tmp);
		tmp = next;
	}
	return curr_scr_win;
}

/*
 * /WINDOW KILLSWAP
 * This arranges for the current window to be replaced by the last window
 * to be hidden, and also destroys the current window.
 */
static Window *
window_killswap (Window * window, char **args, char *usage)
{
	if (invisible_list)
	{
		swap_last_window (0, NULL);
		delete_window (get_window_by_refnum (current_screen->last_window_refnum));
	}
	else
		say ("There are no hidden windows!");

	return curr_scr_win;
}

/*
 * /WINDOW LAST
 * This changes the current window focus to the window that was most recently
 * the current window *but only if that window is still visible*.  If the 
 * window is no longer visible (having been HIDDEN), then the next window
 * following the current window will be made the current window.
 */
static Window *
window_last (Window * window, char **args, char *usage)
{
	set_current_window (NULL);
	return curr_scr_win;
}

/*
 * /WINDOW LASTLOG <size>
 * This changes the size of the window's lastlog buffer.  The default value
 * for a window's lastlog is the value of /set LASTLOG, but each window may
 * be independantly tweaked with this command.
 */
static Window *
window_lastlog (Window * window, char **args, char *usage)
{
	char *arg = next_arg (*args, args);

	if (arg)
	{
		int size = my_atol (arg);
		if (window->lastlog_size > size)
		{
			int i, diff;
			diff = window->lastlog_size - size;
			for (i = 0; i < diff; i++)
				remove_from_lastlog (window);
		}
		window->lastlog_max = size;
	}
	say ("Lastlog size is %d", window->lastlog_max);
	return window;
}

/*
 * /WINDOW LASTLOG_LEVEL <level-description>
 * This changes the types of lines that will be placed into this window's
 * lastlog.  It is useful to note that the window's lastlog will contain
 * a subset (possibly a complete subset) of the lines that have appeared
 * in the window.  This setting allows you to control which lines are
 * "thrown away" by the window.
 */
static Window *
window_lastlog_level (Window * window, char **args, char *usage)
{
	char *arg = next_arg (*args, args);;

	if (arg)
		window->lastlog_level = parse_lastlog_level (arg);
	say ("Lastlog level is %s", bits_to_lastlog_level (window->lastlog_level));
	return window;
}

/*
 * /WINDOW LEVEL <level-description>
 * This changes the types of output that will appear in the specified window.
 * Note that for the given set of windows connected to a server, each level
 * may only appear once in that set.  When you add a level to a given window,
 * then it will be removed from whatever window currently holds it.  The
 * exception to this is the "DCC" level, which may only be set to one window
 * for the entire client.
 */
static Window *
window_level (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		window->window_level = parse_lastlog_level (arg);
		revamp_window_levels (window);
	}
	say ("Window level is %s", bits_to_lastlog_level (window->window_level));
	return window;
}

/*
 * /WINDOW LIST
 * This lists all of the windows known to the client, and a breif summary
 * of their current state.
 */
Window *
window_list (Window * window, char **args, char *usage)
{
	Window *tmp = NULL;
	int len = 6;
	int cnw = get_int_var (CHANNEL_NAME_WIDTH_VAR);

	while (traverse_all_windows (&tmp))
	{
		if (tmp->name && (strlen (tmp->name) > len))
			len = strlen (tmp->name);
	}

	say (WIN_FORM, "Ref",
	     12, 12, "Nick",
	     len, len, "Name",
	     cnw, cnw, "Channel",
	     "Query",
	     "Server",
	     "Level",
	     empty_string);

	tmp = NULL;
	while (traverse_all_windows (&tmp))
		list_a_window (tmp, len);

	return window;
}

/*
 * /WINDOW LOG ON|OFF
 * This sets the current state of the logfile for the given window.  When the
 * logfile is on, then any lines that appear on the window are written to the
 * logfile 'as-is'.  The name of the logfile can be controlled with
 * /WINDOW LOGFILE.  The default logfile name is <windowname>.<target|refnum>
 */
static Window *
window_log (Window * window, char **args, char *usage)
{
	char *logfile;
	int add_ext = 1;
	char buffer[BIG_BUFFER_SIZE + 1];

	if (get_boolean ("LOG", args, &window->log))
		return NULL;

	if ((logfile = window->logfile))
		add_ext = 0;
	else if (!(logfile = get_string_var (LOGFILE_VAR)))
		logfile = empty_string;

	strmcpy (buffer, logfile, BIG_BUFFER_SIZE);

	if (add_ext)
	{
		char *title = empty_string;

		strmcat (buffer, ".", BIG_BUFFER_SIZE);
		if ((title = window->current_channel))
			strmcat (buffer, title, BIG_BUFFER_SIZE);
		else if ((title = window->query_nick))
			strmcat (buffer, title, BIG_BUFFER_SIZE);
		else
		{
			strmcat (buffer, "Window_", BIG_BUFFER_SIZE);
			strmcat (buffer, ltoa (window->refnum), BIG_BUFFER_SIZE);
		}
	}

	do_log (window->log, buffer, &window->log_fp);
	if (!window->log_fp)
		window->log = 0;

	return window;
}

/*
 * /WINDOW LOGFILE <filename>
 * This sets the current value of the log filename for the given window.
 * When you activate the log (with /WINDOW LOG ON), then any output to the
 * window also be written to the filename specified.
 */
static Window *
window_logfile (Window * window, char **args, char *usage)
{
	char *arg = next_arg (*args, args);

	if (arg)
	{
		malloc_strcpy (&window->logfile, arg);
		say ("Window LOGFILE set to %s", arg);
	}
	else if (window->logfile)
		say ("Window LOGFILE is %s", window->logfile);
	else
		say ("Window LOGFILE is not set.");

	return window;
}

static Window *
window_move (Window * window, char **args, char *usage)
{
	move_window (window, get_number ("MOVE", args, NULL));
	return window;
}

static Window *
window_name (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		if (is_window_name_unique (arg))
		{
			malloc_strcpy (&window->name, arg);
			window->update |= UPDATE_STATUS;
		}
		else
			say ("%s is not unique!", arg);
	}
	else
		say ("You must specify a name for the window!");

	return window;
}

static Window *
window_new (Window * window, char **args, char *usage)
{
	Window *tmp;
	if ((tmp = new_window ()))
		window = tmp;

	return window;
}

static Window *
window_next (Window * window, char **args, char *usage)
{
	swap_next_window (0, NULL);
	return window;
}

static Window *
window_notify (Window * window, char **args, char *usage)
{
	window->miscflags ^= WINDOW_NOTIFY;
	say ("Notification when hidden set to %s",
	     window->miscflags & WINDOW_NOTIFY ? on : off);
	return window;
}

static Window *
window_notify_level (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
		window->notify_level = parse_lastlog_level (arg);
	say ("Window notify level is %s", bits_to_lastlog_level (window->notify_level));
	return window;
}

static Window *
window_number (Window * window, char **args, char *usage)
{
	Window *tmp;
	char *arg;
	int i;

	if ((arg = next_arg (*args, args)))
	{
		if ((i = my_atol (arg)) > 0)
		{
			if ((tmp = get_window_by_refnum (i)))
				tmp->refnum = window->refnum;
			window->refnum = i;
			update_all_status (curr_scr_win, NULL, 0);
		}
		else
			say ("Window number must be greater than 0");
	}
	else
		say ("Window number missing");

	return window;
}

/*
 * /WINDOW POP
 * This changes the current window focus to the most recently /WINDOW PUSHed
 * window that still exists.  If the window is hidden, then it will be made
 * visible.  Any windows that are found along the way that have been since
 * KILLed will be ignored.
 */
static Window *
window_pop (Window * window, char **args, char *usage)
{
	int refnum;
	WindowStack *tmp;
	Window *win = NULL;

	while (current_screen->window_stack)
	{
		refnum = current_screen->window_stack->refnum;
		tmp = current_screen->window_stack->next;
		new_free ((char **) &current_screen->window_stack);
		current_screen->window_stack = tmp;

		win = get_window_by_refnum (refnum);
		if (!win)
			continue;

		if (win->visible)
			set_current_window (win);
		else
			show_window (win);
	}

	if (!current_screen->window_stack && !win)
		say ("The window stack is empty!");

	return win;
}

static Window *
window_previous (Window * window, char **args, char *usage)
{
	swap_previous_window (0, NULL);
	return window;
}

static Window *
window_prompt (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		malloc_strcpy (&window->prompt, arg);
		window->update |= UPDATE_STATUS;
	}
	else
		say ("You must specify a prompt for the window!");

	return window;
}

static Window *
window_push (Window * window, char **args, char *usage)
{
	WindowStack *new;

	new = (WindowStack *) new_malloc (sizeof (WindowStack));
	new->refnum = window->refnum;
	new->next = current_screen->window_stack;
	current_screen->window_stack = new;
	return window;
}

static Window *
window_refnum (Window * window, char **args, char *usage)
{
	Window *tmp;
	if ((tmp = get_window ("REFNUM", args)))
	{
		if (tmp->screen && tmp->screen != window->screen)
			say ("Window is in another screen!");
		else if (tmp->visible)
		{
			set_current_window (tmp);
			window = tmp;
		}
		else
			say ("Window is not visible!");
	}
	else
	{
		say ("No such window!");
		window = NULL;
	}
	return window;
}

static Window *
window_remove (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		char *ptr;
		NickList *new;

		while (arg)
		{
			if ((ptr = strchr (arg, ',')) != NULL)
				*ptr++ = 0;

			if ((new = (NickList *) remove_from_list ((List **) & (window->nicks), arg)))
			{
				say ("Removed %s from window name list", new->nick);
				new_free (&new->nick);
				new_free ((char **) &new);
			}
			else
				say ("%s is not on the list for this window!", arg);

			arg = ptr;
		}
	}
	else
		say ("REMOVE: Do something!  Geez!");

	return window;
}

static Window *
window_server (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		int i = find_server_refnum (arg, NULL);

		if (!connect_to_server_by_refnum (i, -1))
		{
			set_window_server (window->refnum, from_server, 0);
			window->window_level = LOG_ALL;
			if (window->current_channel)
				new_free (&window->current_channel);
			update_all_status (window, NULL, 0);
		}
	}
	else
		say ("SERVER: You must specify a server");

	return window;
}

static Window *
window_show (Window * window, char **args, char *usage)
{
	Window *tmp;

	if ((tmp = get_window ("SHOW", args)))
	{
		show_window (tmp);
		window = curr_scr_win;
	}
	return window;
}

static Window *
window_skip (Window * window, char **args, char *usage)
{
	if (get_boolean ("SKIP", args, &window->skip))
		return NULL;

	return window;
}

static Window *
window_show_all (Window * window, char **args, char *usage)
{
	while (invisible_list)
		show_window (invisible_list);
	return window;
}

static Window *
window_shrink (Window * window, char **args, char *usage)
{
	resize_window (RESIZE_REL, window, -get_number ("SHRINK", args, NULL));
	return window;
}

static Window *
window_size (Window * window, char **args, char *usage)
{
	resize_window (RESIZE_ABS, window, get_number ("SIZE", args, NULL));
	return window;
}

static Window *
window_stack (Window * window, char **args, char *usage)
{
	WindowStack *last, *tmp, *crap;
	Window *win;
	int len = 4;

	win = NULL;
	while (traverse_all_windows (&win))
	{
		if (win->name && (strlen (win->name) > len))
			len = strlen (win->name);
	}

	say ("Window stack:");
	last = NULL;
	tmp = current_screen->window_stack;
	while (tmp)
	{
		if ((win = get_window_by_refnum (tmp->refnum)) != NULL)
		{
			list_a_window (win, len);
			last = tmp;
			tmp = tmp->next;
		}
		else
		{
			crap = tmp->next;
			new_free ((char **) &tmp);
			if (last)
				last->next = crap;
			else
				current_screen->window_stack = crap;
			tmp = crap;
		}
	}

	return window;
}

static Window *
window_swap (Window * window, char **args, char *usage)
{
	Window *tmp;

	if ((tmp = get_invisible_window ("SWAP", args)))
		swap_window (window, tmp);

	return window;
}

static Window *
window_unbind (Window * window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg (*args, args)))
	{
		if (is_bound (arg, from_server))
		{
			say ("Channel %s is no longer bound", arg);
			unbind_channel (arg, from_server);
		}
		else
			say ("Channel %s is not bound", arg);
	}
	else
	{
		say ("Channel %s is no longer bound", window->bind_channel);
		new_free (&window->bind_channel);
	}
	return window;
}

typedef Window *(*window_func) (Window *, char **args, char *usage);

typedef struct window_ops_T
{
	char *command;
	window_func func;
	char *usage;
}
window_ops;


static Window *window_help (Window *, char **, char *);

static window_ops options[] =
{
	{"ADD", window_add, NULL},
	{"BACK", window_back, NULL},
	{"BALANCE", window_balance, NULL},
	{"BEEP_ALWAYS", window_beep_always, NULL},
	{"BIND", window_bind, NULL},
	{"CHANNEL", window_channel, NULL},
	{"DESCRIBE", window_describe, NULL},
	{"DOUBLE", window_double, NULL},
	{"FIXED", window_fixed, NULL},
	{"GOTO", window_goto, NULL},
	{"GROW", window_grow, NULL},
	{"HELP", window_help, NULL},
	{"HIDE", window_hide, NULL},
	{"HIDE_OTHERS", window_hide_others, NULL},
	{"HOLD_MODE", window_hold_mode, NULL},
	{"KILL", window_kill, NULL},
	{"KILL_OTHERS", window_kill_others, NULL},
	{"KILLSWAP", window_killswap, NULL},
	{"LAST", window_last, NULL},
	{"LASTLOG", window_lastlog, NULL},
	{"LASTLOG_LEVEL", window_lastlog_level, NULL},
	{"LEVEL", window_level, NULL},
	{"LIST", window_list, NULL},
	{"LOG", window_log, NULL},
	{"LOGFILE", window_logfile, NULL},
	{"MOVE", window_move, NULL},
	{"NAME", window_name, NULL},
	{"NEW", window_new, NULL},
	{"NEXT", window_next, NULL},
	{"NOTIFY", window_notify, NULL},
	{"NOTIFY_LEVEL", window_notify_level, NULL},
	{"NUMBER", window_number, NULL},
	{"POP", window_pop, NULL},
	{"PREVIOUS", window_previous, NULL},
	{"PROMPT", window_prompt, NULL},
	{"PUSH", window_push, NULL},
	{"REFNUM", window_refnum, NULL},
	{"REMOVE", window_remove, NULL},
	{"SERVER", window_server, NULL},
	{"SHOW", window_show, NULL},
	{"SHOW_ALL", window_show_all, NULL},
	{"SHRINK", window_shrink, NULL},
	{"SIZE", window_size, NULL},
	{"SKIP", window_skip, NULL},
	{"STACK", window_stack, NULL},
	{"SWAP", window_swap, NULL},
	{"UNBIND", window_unbind, NULL},
	{NULL, NULL, NULL}
};


static Window *
window_help (Window * window, char **args, char *usage)
{
	int i, c = 0;
	char buffer[BIG_BUFFER_SIZE + 1];
	char *arg = NULL;
	*buffer = 0;
	if (!(arg = next_arg (*args, args)))
	{
		for (i = 0; options[i].command; i++)
		{
			strmcat (buffer, options[i].command, BIG_BUFFER_SIZE);
			strmcat (buffer, " ", BIG_BUFFER_SIZE);
			if (++c == 5)
			{
				put_it ("%s", convert_output_format ("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
				*buffer = 0;
				c = 0;
			}
		}
		if (c)
			put_it ("%s", convert_output_format ("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
		userage ("window help", "%R[%ncommand%R]%n to get help on specific commands");
	}
	return window;
}

void
cmd_window (struct command *cmd, char *args)
{
	char *arg;
	int nargs = 0;
	Window *window;

	in_window_command = 1;
	message_from (NULL, LOG_CURRENT);
	window = curr_scr_win;

	while ((arg = next_arg (args, &args)))
	{
		int i;
		int len = strlen (arg);

		for (i = 0; options[i].func; i++)
		{
			if (!my_strnicmp (arg, options[i].command, len))
			{
				window = options[i].func (window, &args, options[i].usage);
				nargs++;
				if (!window)
					args = NULL;
				break;
			}
		}

		if (!options[i].func)
			yell ("WINDOW: Invalid option: [%s]", arg);
	}

	if (!nargs)
		window_describe (curr_scr_win, NULL, NULL);

	in_window_command = 0;
	message_from (NULL, LOG_CRAP);
	build_status (window, NULL, 0);
	update_all_windows ();
	cursor_to_input ();
}



/* * * * * * * * * * * SCROLLBACK BUFFER * * * * * * * * * * * * * * */
/* 
 * XXXX Dont you DARE touch this XXXX 
 *
 * Most of the time, a delete_display_line() is followed somewhat
 * immediately by a new_display_line().  So most of the time we just
 * cache that one item and re-use it.  That saves us thousands of
 * malloc()s.  In the cases where its not, then we just do things the
 * normal way.  
 */
static Display *recycle = NULL;

void 
delete_display_line (Display * stuff)
{
	if (recycle == stuff)
		ircpanic ("error in delete_display_line");
	if (recycle)
		new_free ((char **) &recycle);
	recycle = stuff;
	new_free (&stuff->line);
}

Display *
new_display_line (Display * prev)
{
	Display *stuff;

	if (recycle)
	{
		stuff = recycle;
		recycle = NULL;
	}
	else
		stuff = (Display *) new_malloc (sizeof (Display));

	stuff->line = NULL;
	stuff->prev = prev;
	stuff->next = NULL;
	return stuff;
}

/* * * * * * * * * * * Scrollback functionality * * * * * * * * * * */
void 
scrollback_backwards_lines (int lines)
{
	Window *window = curr_scr_win;
	Display *new_top = window->top_of_display;
	int new_lines;

	if (new_top == window->top_of_scrollback)
	{
		term_beep ();
		return;
	}

	if (!window->scrollback_point)
		window->scrollback_point = window->top_of_display;

	for (new_lines = 0; new_lines < lines; new_lines++)
	{
		if (new_top == window->top_of_scrollback)
			break;
		new_top = new_top->prev;
	}

	window->lines_scrolled_back += new_lines;
	window->top_of_display = new_top;
	window->lines_scrolled_back += new_lines;

	recalculate_window_cursor (window);
	repaint_window (window, 0, -1);
	cursor_not_in_display ();
	update_input (UPDATE_JUST_CURSOR);
}

void 
scrollback_forwards_lines (int lines)
{
	Window *window = curr_scr_win;
	Display *new_top = window->top_of_display;
	int new_lines = 0;

	if (new_top == window->display_ip || !window->scrollback_point)
	{
		term_beep ();
		return;
	}

	for (new_lines = 0; new_lines < lines; new_lines++)
	{
		if (new_top == window->display_ip)
			break;
		new_top = new_top->next;
	}

	window->lines_scrolled_back -= new_lines;
	window->top_of_display = new_top;
	window->lines_scrolled_back -= new_lines;

	recalculate_window_cursor (window);
	repaint_window (window, 0, -1);
	cursor_not_in_display ();
	update_input (UPDATE_JUST_CURSOR);
	if (window->lines_scrolled_back <= 0)
		scrollback_end (0, NULL);
}

void 
scrollback_forwards (char dumb, char *dumber)
{
	scrollback_forwards_lines (curr_scr_win->display_size / 2);
}

void 
scrollback_backwards (char dumb, char *dumber)
{
	scrollback_backwards_lines (curr_scr_win->display_size / 2);
}


void 
scrollback_end (char dumb, char *dumber)
{
	Window *window = curr_scr_win;

	if (!window->scrollback_point)
	{
		term_beep ();
		return;
	}

	/* Adjust the top of window only if we would move forward. */
	if (window->lines_scrolled_back > 0)
		window->top_of_display = window->scrollback_point;

	window->lines_scrolled_back = 0;
	window->scrollback_point = NULL;
	repaint_window (window, 0, -1);
	cursor_not_in_display ();
	update_input (UPDATE_JUST_CURSOR);

	while (window->holding_something)
		unhold_a_window (window);
}

void 
scrollback_start (char dumb, char *dumber)
{
	Window *window = curr_scr_win;

	if (window->display_buffer_size <= window->display_size)
	{
		term_beep ();
		return;
	}

	if (!window->scrollback_point)
		window->scrollback_point = window->top_of_display;

	while (window->top_of_display != window->top_of_scrollback)
	{
		window->top_of_display = window->top_of_display->prev;
		window->lines_scrolled_back++;
	}

	repaint_window (window, 0, -1);
	cursor_not_in_display ();
	update_input (UPDATE_JUST_CURSOR);
}


/* HOLD MODE STUFF */
/*
 * hold_mode: sets the "hold mode".  Really.  If the update flag is true,
 * this will also update the status line, if needed, to display the hold mode
 * state.  If update is false, only the internal flag is set.  
 */
void 
hold_mode (Window * window, int flag, int update)
{
	if (window == NULL)
		window = curr_scr_win;

	if (flag != ON && window->scrollback_point)
		return;

	if (flag == TOGGLE)
	{
		if (window->hold_mode == OFF)
			window->hold_mode = ON;
		else
			window->hold_mode = OFF;
	}
	else
		window->hold_mode = flag;


	if (update)
	{
		if (window->lines_held != window->last_lines_held)
		{
			window->last_lines_held = window->lines_held;
			update_window_status (window, 0);
			if (window->update | UPDATE_STATUS)
				window->update -= UPDATE_STATUS;
			cursor_in_display ();
			update_input (NO_UPDATE);
		}
	}
	else
		window->last_lines_held = -1;
}

void 
unstop_all_windows (char dumb, char *dumber)
{
	Window *tmp;

	for (tmp = current_screen->window_list; tmp; tmp = tmp->next)
		hold_mode (tmp, OFF, 1);
}

/*
 * reset_line_cnt: called by /SET HOLD_MODE to reset the line counter so we
 * always get a held screen after the proper number of lines 
 */
void 
reset_line_cnt (Window * win, char *unused, int value)
{
	curr_scr_win->hold_mode = value;
	curr_scr_win->held_displayed = 0;
}

/* toggle_stop_screen: the BIND function TOGGLE_STOP_SCREEN */
void 
toggle_stop_screen (char unused, char *not_used)
{
	hold_mode ((Window *) 0, TOGGLE, 1);
	update_all_windows ();
}

/* 
 * If "scrollback_point" is set, then anything below the bottom of the screen
 * at that point gets nuked.
 * If "scrollback_point" is not set, anything below the current position of
 * the screen gets nuked.
 */
void 
flush_everything_being_held (Window * window)
{
	Display *ptr;
	int count;

	if (!window)
		window = curr_scr_win;

	count = window->display_size;

	if (window->scrollback_point)
		ptr = window->scrollback_point;
	else
		ptr = window->top_of_display;

	while (count--)
	{
		ptr = ptr->next;
		if (ptr == window->display_ip)
			return;	/* Nothing to flush */
	}

	ptr->next = window->display_ip;
	window->display_ip->prev = ptr;

	while (ptr != window->display_ip)
	{
		Display *next = ptr->next;
		delete_display_line (ptr);
		window->lines_held--;
		ptr = next;
	}

	if (window->lines_held != 0)
		ircpanic ("erf. fix this.");

	window->holding_something = 0;
	hold_mode (window, OFF, 1);
}


/*
 * This adjusts the viewport up one full screen.  This calls rite() 
 * indirectly, because repaint_window() uses rite() to do the work.
 * This belongs somewhere else.
 */
void 
unhold_a_window (Window * window)
{
	int amount = window->display_size;

	if (window->holding_something &&
	    (window->distance_from_display < window->display_size))
	{
		window->holding_something = 0;
		window->lines_held = 0;
	}

	if (!window->lines_held || window->scrollback_point)
		return;		/* Right. */

	if (window->lines_held < amount)
		amount = window->lines_held;

	window->lines_held -= amount;
	window->held_displayed = 0;

	if (!window->lines_held)
		window->holding_something = 0;

	if (!amount)
		return;		/* Whatever */

	while (amount--)
		window->top_of_display = window->top_of_display->next;

	repaint_window (window, 0, -1);
}

void 
recalculate_window_cursor (Window * window)
{
	Display *tmp;

	window->cursor = window->distance_from_display = 0;
	for (tmp = window->top_of_display; tmp != window->display_ip;
	     tmp = tmp->next)
		window->cursor++, window->distance_from_display++;

	if (window->cursor > window->display_size)
		window->cursor = window->display_size;
}
