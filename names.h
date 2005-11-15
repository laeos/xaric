/*
 * names.h: Header for names.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __names_h_
#define __names_h_

#include "window.h"
#include "irc.h"

/* from names.c - "iklmnpsta" */
#define MODE_INVITE	((u_long) 0x0001)
#define MODE_KEY	((u_long) 0x0002)
#define MODE_LIMIT	((u_long) 0x0004)
#define MODE_MODERATED	((u_long) 0x0008)
#define MODE_MSGS	((u_long) 0x0010)
#define MODE_PRIVATE	((u_long) 0x0020)
#define MODE_SECRET	((u_long) 0x0040)
#define MODE_TOPIC	((u_long) 0x0080)
#define MODE_ANON	((u_long) 0x0100)

/* for lookup_channel() */
#define	CHAN_NOUNLINK	1
#define CHAN_UNLINK	2

#define GOTNAMES	0x01
#define	GOTMODE		0x02
#define GOTBANS		0x04

void add_userhost_to_channel(char *channel, char *nick, int server, char *userhost);
char *compress_modes(int server, char *channel, char *modes);
char *fetch_userhost(int server, char *nick);
void handle_nickflood(char *old_nick, char *new_nick, register struct nick_list *nick, register struct channel *chan,
		      time_t current_time, int flood_time);

void add_to_join_list(char *, int, int);
void remove_from_join_list(char *, int);
char *get_chan_from_join_list(int);
int get_win_from_join_list(char *, int);
int in_join_list(char *, int);
int got_info(char *, int, int);

int is_channel_mode(char *, int, int);
int is_chanop(char *, char *);
struct channel *lookup_channel(char *, int, int);
char *get_channel_mode(char *, int);

#ifdef	INCLUDE_UNUSED_FUNCTIONS
void set_channel_mode(char *, int, char *);
#endif				/* INCLUDE_UNUSED_FUNCTIONS */
struct channel *add_channel(char *, int);
struct channel *add_to_channel(char *, char *, int, int, int, char *, char *, char *);
void remove_channel(char *, int);
void remove_from_channel(char *, char *, int, int, char *);
int is_on_channel(char *, int, char *);
void list_channels(void);
void reconnect_all_channels(int);
void switch_channels(char, char *);
char *what_channel(char *, int);
char *walk_channels(char *, int, int);
char *real_channel(void);
void rename_nick(char *, char *, int);
void update_channel_mode(char *, char *, int, char *, struct channel *);
void set_channel_window(Window *, char *, int);
char *create_channel_list(Window *);
int get_channel_oper(char *, int);
void channel_server_delete(int);
void change_server_channels(int, int);
void clear_channel_list(int);
void set_waiting_channel(int);
void remove_from_mode_list(char *, int);
int chan_is_connected(char *, int);
int im_on_channel(char *);
char *recreate_mode(struct channel *);
int get_channel_voice(char *, int);
char *get_channel_key(char *, int);

#endif				/* __names_h_ */
