/*
 * screen.h: header for screen.c
 *
 * written by matthew green.
 *
 * copyright (c) 1993, 1994.
 *
 * see the copyright file, or type help ircii copyright
 *
 * @(#)$Id$
 */

#ifndef __screen_h_
#define __screen_h_

#include "window.h"

#define WAIT_PROMPT_LINE        0x01
#define WAIT_PROMPT_KEY         0x02

/* Stuff for the screen/xterm junk */

#define ST_NOTHING      -1
#define ST_SCREEN       0

/* This is here because it happens in so many places */
#define curr_scr_win	current_screen->current_window

	void	clear_window(Window *);
	void	recalculate_window_positions(void);
	int	output_line(register unsigned char *);
	void	recalculate_windows(void);
	Window	*create_additional_screen(void);
	void	scroll_window(Window *);
	Window	*new_window(void);
	void	update_all_windows(void);
	void	add_wait_prompt(char *, void (*) (char *, char *), char *, int);
	void	clear_all_windows(int);
	void	cursor_in_display(void);
	int	is_cursor_in_display(Screen *);
	void	cursor_not_in_display(void);
	void	set_current_screen(Screen *);
	void	redraw_resized(Window *, ShrinkInfo, int);
	void	close_all_screen(void);
	void	scrollback_forwards(char, char *);
	void	scrollback_backwards(char, char *);
	void	scrollback_end(char, char *);
	void	scrollback_start(char, char *);
	void	kill_screen(Screen *);
	int	rite(Window *, const unsigned char *);
	ShrinkInfo	resize_display(Window *);
	void	redraw_window(Window *, int);
	void	redraw_all_windows(void);
	void	add_to_screen(unsigned char *);
	void	do_screens(fd_set *);
	unsigned	char	**split_up_line(const unsigned char *);
	void repaint_window (Window * w, int start_line, int end_line);
	Screen * create_new_screen (void);
	void set_screens (fd_set * rd, fd_set * wd);



		
extern	Window	*to_window;
extern	Screen	*current_screen;
extern	Screen	*main_screen;
extern	Screen	*last_input_screen;
extern	Screen	*screen_list;

#endif /* __screen_h_ */
