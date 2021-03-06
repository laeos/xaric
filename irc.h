#ifndef __irc_h__
#define __irc_h__

#include "defaults.h"

#define XARIC_COMMENT   "\002Let's kill something!\002"

/*
 * Here you can set the in-line quote character, normally backslash, to
 * whatever you want.  Note that we use two backslashes since a backslash is
 * also C's quote character.  You do not need two of any other character.
 */
#define QUOTE_CHAR '\\'

extern char *line_thing;

#define STRING_CHANNEL '+'
#define MULTI_CHANNEL '#'
#define LOCAL_CHANNEL '&'

#define is_channel(x)  ( (x) && ((*(x) == MULTI_CHANNEL) || (*(x) == LOCAL_CHANNEL)))

#include "defaults.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/param.h>

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif				/* HAVE_SYS_TIME_H */
#endif				/* TIME_WITH_SYS_TIME */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#else
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif				/* HAVE_FCNTL_H */
#endif

#include <stdarg.h>
# include <unistd.h>

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#ifdef HAVE_TERMCAP_H
# include <termcap.h>
#endif

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#ifdef HAVE_SSL
# include <openssl/crypto.h>
# include <openssl/x509.h>
# include <openssl/pem.h>
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/rand.h>
# define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))
#endif

#include "irc_std.h"
#include "newio.h"

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'	/* ^V */
#define REV_TOG_STR	"\026"
#define UND_TOG		'\037'	/* ^_ */
#define UND_TOG_STR	"\037"
#define BOLD_TOG	'\002'	/* ^B */
#define BOLD_TOG_STR	"\002"
#define ALL_OFF		'\017'	/* ^O */
#define ALL_OFF_STR	"\017"

#define IRCD_BUFFER_SIZE	512
#define BIG_BUFFER_SIZE		(IRCD_BUFFER_SIZE * 4)

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE	(IRCD_BUFFER_SIZE - 20)
#endif

#define REFNUM_MAX 10

#include "struct.h"

#define NICKNAME_LEN 9
#define NAME_LEN 80
#define REALNAME_LEN 50
#define PATH_LEN 1024

#ifndef MIN
# define MIN(a,b) ((a < b) ? (a) : (b))
#endif

/* doh! */
#if defined(NEED_STRERROR)
# define strerror(x) sys_errlist[x]
#endif

/* flags used by who() and whoreply() for who_mask */
#define WHO_OPS		0x0001
#define WHO_NAME	0x0002
#define WHO_ZERO	0x0004
#define WHO_CHOPS	0x0008
#define WHO_FILE	0x0010
#define WHO_HOST	0x0020
#define WHO_SERVER	0x0040
#define	WHO_HERE	0x0080
#define	WHO_AWAY	0x0100
#define	WHO_NICK	0x0200
#define	WHO_LUSERS	0x0400
#define	WHO_REAL	0x0800
#define WHO_KILL	0x8000

/*
 * declared in irc.c 
 */

extern char *cut_buffer;
extern char oper_command;
extern int irc_port;
extern int current_on_hook;
extern char *joined_nick;
extern char *public_nick;
extern char empty_str[];
extern char zero_str[];
extern char one_str[];
extern char on_str[];
extern char off_str[];
extern char space_str[];

extern char *convertstring;
extern char nickname[];
extern char *ircrc_file;
extern char *bircrc_file;
extern char realname[];
extern char username[];
extern char *send_umode;
extern int away_set;
extern int background;
extern char *my_path;
extern char *args_str;
extern char *invite_channel;
extern int who_mask;
extern char *who_name;
extern char *who_host;
extern char *who_server;
extern char *who_file;
extern char *who_nick;
extern char *who_real;
extern int use_input;
extern time_t idle_time;
extern int waiting_out;
extern int waiting_in;
extern char wait_nick[];
extern char lame_wait_nick[];
extern char **environ;
extern int current_numeric;
extern int quick_startup;
extern int strip_ansi_in_echo;
extern int need_redraw;
extern char *prog_name;

extern struct channel *statchan_list;
extern int cpu_saver;

int wild_match(const char *, const char *);

char get_a_char(void);
void free_display(Window *);
void free_hold(Window *);
void free_lastlog(Window *);
void free_formats(Window *);
void load_scripts(void);
void display_name(int j);

/* signals.c */
void signals_init(void);

/* irc.c */
void get_line_return(char unused, char *not_used);
void get_line(char *prompt, int new_input, void (*func) (char, char *));
char get_a_char(void);
void io(const char *what);
void irc_exit(char *reason, char *formatted);

extern time_t start_time;

char *getenv(const char *);

/* xscandir.c */
int xscandir(char *dir, char *prefix, char **ret);

#define TRACE(fmt, ...) \
    trace("%s/%d: " fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)

void trace(const char *fmt, ...);

#endif				/* __irc_h__ */
