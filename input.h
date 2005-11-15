/*
 * input.h: header for input.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __input_h_
#define __input_h_
char input_pause(char *);
void set_input(char *);
void set_input_prompt(Window *, char *, int);
char *get_input_prompt(void);
char *get_input(void);
void update_input(int);
void init_input(void);
void input_move_cursor(int);
void change_input_prompt(int);
void cursor_to_input(void);

/* keybinding functions */
extern void backward_character(char, char *);
extern void backward_history(char, char *);
extern void clear_screen(char, char *);
extern void command_completion(char, char *);
extern void forward_character(char, char *);
extern void forward_history(char, char *);
extern void highlight_off(char, char *);
extern void input_add_character(char, char *);
extern void input_backspace(char, char *);
extern void input_backward_word(char, char *);
extern void input_beginning_of_line(char, char *);
extern void new_input_beginning_of_line(char, char *);
extern void input_clear_line(char, char *);
extern void input_clear_to_bol(char, char *);
extern void input_clear_to_eol(char, char *);
extern void input_delete_character(char, char *);
extern void input_delete_next_word(char, char *);
extern void input_delete_previous_word(char, char *);
extern void input_delete_to_previous_space(char, char *);
extern void input_end_of_line(char, char *);
extern void input_forward_word(char, char *);
extern void input_transpose_characters(char, char *);
extern void input_yank_cut_buffer(char, char *);
extern void insert_bold(char, char *);
extern void insert_reverse(char, char *);
extern void insert_underline(char, char *);
extern void meta1_char(char, char *);
extern void meta2_char(char, char *);
extern void meta3_char(char, char *);
extern void meta4_char(char, char *);
extern void meta5_char(char, char *);
extern void meta6_char(char, char *);
extern void meta7_char(char, char *);
extern void meta8_char(char, char *);
extern void meta9_char(char, char *);

/*
extern	void 	parse_text 	(char, char *);
extern	void 	quote_char 	(char, char *);
*/
extern void refresh_inputline(char, char *);
extern void send_line(char, char *);
extern void toggle_insert_mode(char, char *);
extern void input_msgreply(char, char *);
extern void input_autoreply(char, char *);

extern void input_msgreplyback(char, char *);
extern void input_autoreplyback(char, char *);

extern void my_scrollback(char, char *);
extern void my_scrollforward(char, char *);
extern void my_scrollend(char, char *);

/*
extern	void 	type_text 	(char, char *);
*/
extern void wholeft(char, char *);
extern void dcc_plist(char, char *);
extern void channel_chops(char, char *);
extern void channel_nonops(char, char *);
extern void change_to_split(char, char *);
extern void do_chelp(char, char *);
extern void join_last_invite(char, char *);
extern void window_swap1(char, char *);
extern void window_swap2(char, char *);
extern void window_swap3(char, char *);
extern void window_swap4(char, char *);
extern void window_swap5(char, char *);
extern void window_swap6(char, char *);
extern void window_swap7(char, char *);
extern void window_swap8(char, char *);
extern void window_swap9(char, char *);
extern void window_swap10(char, char *);
extern void w_help(char, char *);
extern void cpu_saver_on(char, char *);
extern void window_key_balance(char, char *);
extern void window_grow_one(char, char *);
extern void window_key_hide(char, char *);
extern void window_key_kill(char, char *);
extern void window_key_list(char, char *);
extern void window_key_move(char, char *);
extern void window_shrink_one(char, char *);
extern void nick_completion(char, char *);
extern void dcc_ostats(char, char *);
extern void ignore_last_nick(char, char *);

/* used by update_input */
#define NO_UPDATE 0
#define UPDATE_ALL 1
#define UPDATE_FROM_CURSOR 2
#define UPDATE_JUST_CURSOR 3

#endif				/* __input_h_ */
