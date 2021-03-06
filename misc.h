/* 
 * Copyright Colten Edwards 1997.
 * various miscellaneous routines needed for irc functions
 */

#ifndef _misc_h
#define _misc_h

#include "whois.h"

#define KICKLIST		0x01
#define LEAVELIST		0x02
#define JOINLIST		0x03
#define CHANNELSIGNOFFLIST	0x04
#define PUBLICLIST		0x05
#define PUBLICOTHERLIST		0x06
#define PUBLICNOTICELIST	0x07
#define NOTICELIST		0x08
#define TOPICLIST		0x09
#define MODEOPLIST		0x0a
#define MODEDEOPLIST		0x0b
#define MODEBANLIST		0x0c
#define MODEUNBANLIST		0x0d
#define NICKLIST		0x0e

enum color_attributes {
    BLACK = 0, BLUE, GREEN, CYAN, RED, MAGENTA, YELLOW, WHITE,
    BLACKB, BLUEB, GREENB, CYANB, REDB, MAGENTAB, YELLOWB, WHITEB, NO_COLOR,
    BACK_BLACK, BACK_RED, BACK_GREEN, BACK_YELLOW,
    BACK_BLUE, BACK_MAGENTA, BACK_CYAN, BACK_WHITE,
    BACK_BBLACK, BACK_BRED, BACK_BGREEN, BACK_BYELLOW,
    BACK_BBLUE, BACK_BMAGENTA, BACK_BCYAN, BACK_BWHITE,
    REVERSE_COLOR, BOLD_COLOR, BLINK_COLOR, UNDERLINE_COLOR
};

#define DONT_CARE 3
#define NEED_OP 1
#define NO_OP 0

extern int split_watch;
void clear_link(irc_server **);
extern irc_server *tmplink, *server_last;

#define MAX_LAST_MSG 10
extern LastMsg last_msg[MAX_LAST_MSG];
extern LastMsg last_dcc[MAX_LAST_MSG];
extern LastMsg last_notice[MAX_LAST_MSG];
extern LastMsg last_servermsg[MAX_LAST_MSG];
extern LastMsg last_sent_msg[MAX_LAST_MSG];
extern LastMsg last_sent_notice[MAX_LAST_MSG];
extern LastMsg last_sent_topic[1];
extern LastMsg last_sent_wall[1];
extern LastMsg last_topic[1];
extern LastMsg last_wall[MAX_LAST_MSG];
extern LastMsg last_invite_channel[1];
extern LastMsg last_ctcp[1];
extern LastMsg last_sent_ctcp[1];

irc_server *add_server(irc_server ** serv1, char *channel, char *arg, int hops, char *thetime);
int find_server(irc_server * serv1, char *channel);
void parse_364(char *, char *, char *);
void parse_365(char *channel, char *args, char *subargs);
char *rights(char *string, int num);
int numchar(char *string, char c);
void add_to_irc_map(char *server1, char *distance);
void show_server_map(void);
void *do_nslookup_blah(void *arg);
void do_nslookup_thread(void *arg);

void update_stats(int, char *, struct nick_list *, struct channel *, int);
int check_split(char *, char *, char *);
void userage(const char *, const char *);
void stats_k_grep_end(void);
char *stripansicodes(const char *);
char *stripansi(char *);
NickTab *gettabkey(int, char *);
void addtabkey(const char *, const char *);
void clear_array(NickTab **);
char *random_str(int, int);
int check_serverlag(void *);
void do_clones(fd_set *, fd_set *);
struct channel *prepare_command(int *, char *, int);
int rename_file(char *, char **);

char *clear_server_flags(char *);

int logmsg(unsigned long, char *, char *, int);
void log_toggle(int, struct channel *);

char *do_nslookup(char *);
char *cluster(char *);
int caps_fucknut(char *);

void do_reconnect(char *);
void start_finger(WhoisStuff *, char *, char *);

int are_you_opped(char *);
void error_not_opped(char *);
void not_on_a_channel(Window *);

char *get_reason(char *);
char *get_signoffreason(char *);
int isme(char *);

char *convert_output_format(const char *, const char *, ...) __A(2);
int matchmcommand(char *, int);
int charcount(char *string, char what);
char *convert_time(time_t);
void flush_channel_stats(void);
char *make_channel(char *);
void add_split_server(char *, char *, int);
irc_server *check_split_server(char *);
void remove_split_server(char *server);
int timer_unban(void *);
void check_server_connect(int);
const char *country(const char *);
char *str_terminate_at(char *string, const char *chars);
char *mircansi(char *);

	/* in cmd_modes.h */
void flush_mode_all(struct channel *chan);

#endif
