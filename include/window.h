/*
 * window.h: header file for window.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __window_h_
#define __window_h_

#include "irc_std.h"
#include "lastlog.h"

/* used by the update flag to determine what needs updating */
#define REDRAW_DISPLAY_FULL 1
#define REDRAW_DISPLAY_FAST 2
#define UPDATE_STATUS 4
#define REDRAW_STATUS 8

#define	LT_UNLOGGED	0
#define	LT_LOGHEAD	1
#define	LT_LOGTAIL	2

/* var_settings indexes */
#define OFF 0
#define ON 1
#define TOGGLE 2

	Window 	*new_window 			(void);
	void	delete_window			(Window *);
	int	traverse_all_windows		(Window **);
	void	add_to_invisible_list		(Window *);
	Window	*add_to_window_list		(Window *);
	void	remove_from_window_list		(Window *);
	void	recalculate_window_positions	(void);
	void	redraw_all_windows		(void);
	void	recalculate_windows		(void);
	void	rebalance_windows		(void);
	void	update_all_windows		(void);
	void	set_current_window		(Window *);
	void	hide_window			(Window *);
	void	swap_last_window		(char, char *);
	void	next_window			(char, char *);
	void	swap_next_window		(char, char *);
	void	previous_window			(char, char *);
	void	swap_previous_window		(char, char *);
	void	back_window			(char, char *);
	Window 	*get_window_by_refnum		(unsigned);
	Window	*get_window_by_name		(char *);
	char	*get_refnum_by_window		(const Window *);
	int	is_window_visible		(char *);
	char	*get_status_by_refnum		(unsigned, unsigned);
	void	update_window_status		(Window *, int);
	void	update_all_status		(Window *, char *, int);
	void	status_update			(int);
	void	set_prompt_by_refnum		(unsigned, char *);
	char 	*get_prompt_by_refnum		(unsigned);
	char	*get_target_by_refnum		(unsigned);
	char	*query_nick			(void);
	char	*query_host			(void);
	char	*query_cmd			(void);
	void	set_query_nick			(char *, char *, char *);
	int	is_current_channel		(char *, int, int);
	char	*set_current_channel_by_refnum		(unsigned, char *);
	char	*get_current_channel_by_refnum		(unsigned);
	int	is_bound_to_window		(const Window *, const char *);
	Window	*get_window_bound_channel	(const char *);
	int	is_bound_anywhere		(const char *);
	int	is_bound			(const char *, int);
	void    unbind_channel 			(const char *, int);
	char	*get_bound_channel		(Window *);
	int	get_window_server		(unsigned);
	void	set_window_server		(int, int, int);
	void	window_check_servers		(void);
	void	set_level_by_refnum		(unsigned, int);
	void	message_to			(unsigned);
	void	save_message_from		(char **, int *);
	void	restore_message_from		(char *, int);
	void	message_from			(char *, int);
	int	message_from_level		(int);
	void	clear_all_windows		(int);
	void	clear_window_by_refnum		(unsigned);
	void	set_scroll			(Window *, char *, int);
	void	set_scroll_lines		(Window *, char *, int);
	void	set_continued_lines		(Window *, char *, int);
	unsigned current_refnum			(void);
	int	number_of_windows		(void);
	void	delete_display_line		(Display *);
	Display *new_display_line		(Display *);
	void	scrollback_backwards_lines	(int);
	void	scrollback_forwards_lines	(int);
	void	scrollback_backwards		(char, char *);
	void	scrollback_forwards		(char, char *);
	void	scrollback_end			(char, char *);
	void	scrollback_start		(char, char *);
	void	hold_mode			(Window *, int, int);
	void	unstop_all_windows		(char, char *);
	void	reset_line_cnt			(Window *, char *, int);
	void	toggle_stop_screen		(char, char *);
	void	flush_everything_being_held	(Window *);
	void	unhold_a_window			(Window *);
	char *	get_target_cmd_by_refnum	(u_int);
	void	recalculate_window_cursor	(Window *);
	int	is_window_name_unique		(char *);
	int	get_visible_by_refnum		(char *);
	void	resize_window			(int, Window *, int);
	Window *window_list			(Window *, char **, char *);
	void	move_window			(Window *, int);
	void	show_window			(Window *);
			
	BUILT_IN_COMMAND(windowcmd);

	char *	get_status_by_refnum (u_int, unsigned);
	
extern	Window	*invisible_list;
extern	int	who_level;
extern	char	*who_from;
extern	int	in_window_command;
extern	unsigned int	window_display;

#define WINDOW_NOTIFY	((unsigned) 0x0001)
#define WINDOW_NOTIFIED	((unsigned) 0x0002)

#endif /* __window_h_ */
