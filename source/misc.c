/* 
 *  Copyright Colten Edwards (c) 1996
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#if defined(sparc) && defined(sun4c)
#include <sys/rusage.h>
#endif


#include "irc.h"
#include "server.h"
#include "commands.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "whois.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "list.h"
#include "timer.h"
#include "misc.h"
#include "flood.h"
#include "parse.h"
#include "whowas.h"
#include "hash2.h"
#include "fset.h"


char *alias_special_char (char **, char *, char *, char *, int *);

extern time_t start_time;
extern int in_server_ping;

int split_watch = 0;
int serv_action = 0;
int first_time = 0;


char *convertstring = NULL;
extern int in_cparse;

extern char *mircansi (char *);


ChannelList default_statchan =
{0};

extern NickTab *tabkey_array;

SocketList sockets[FD_SETSIZE] =
{
	{0, 0, 0, NULL}};

extern Ignore *ignored_nicks;

#ifdef REVERSE_WHITE_BLACK
char *color_str[] =
{
	"[0m", "[0;34m", "[0;32m", "[0;36m", "[0;31m", "[0;35m", "[0;33m", "[0;30m",
	"[1;37m", "[1;34m", "[1;32m", "[1;36m", "[1;31m", "[1;35m", "[1;33m", "[1;30m", "[0m",
	"[0;47m", "[0;41m", "[0;42m", "[0;43m", "[0;44m", "[0;45m", "[0;46m", "[0;40m",
	"[1;47m", "[1;41m", "[1;42m", "[1;43m", "[1;44m", "[1;45m", "[1;46m", "[1;40m",
	"[7m", "[1m", "[5m", "[4m"};

#else

char *color_str[] =
{
	"[0;30m", "[0;34m", "[0;32m", "[0;36m", "[0;31m", "[0;35m", "[0;33m", "[0m",
	"[1;30m", "[1;34m", "[1;32m", "[1;36m", "[1;31m", "[1;35m", "[1;33m", "[1;37m", "[0m",
	"[0;40m", "[0;41m", "[0;42m", "[0;43m", "[0;44m", "[0;45m", "[0;46m", "[0;47m",
	"[1;40m", "[1;41m", "[1;42m", "[1;43m", "[1;44m", "[1;45m", "[1;46m", "[1;47m",
	"[7m", "[1m", "[5m", "[4m"};
#endif

irc_server *tmplink = NULL, *server_last = NULL, *split_link = NULL, *map = NULL;

#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))

char *awaymsg = NULL;

char *
convert_time (time_t ltime)
{
	time_t days = 0, hours = 0, minutes = 0, seconds = 0;
	static char buffer[100];

	context;
	*buffer = '\0';
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime % 60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	sprintf (buffer, "%2ldd %2ldh %2ldm %2lds", days, hours, minutes, seconds);
	return (*buffer ? buffer : empty_string);
}

int 
check_serverlag (void *args)
{
	char *servern = (char *) args;
	context;
	if (servern && *servern)
	{
		int i;
		for (i = 0; i < number_of_servers; i++)
		{
			if ((!my_stricmp (servern, get_server_itsname (i)) || !my_stricmp (servern, get_server_name (i))) && is_server_connected (i))
			{
				server_list[i].lag_time = time (NULL);
				send_to_server ("PING %lu %s", server_list[i].lag_time, servern);
				in_server_ping++;
				server_list[i].lag = -1;
				break;
			}
		}
	}
	return 0;
}

int 
timer_unban (void *args)
{
	char *p = (char *) args;
	char *channel;
	ChannelList *chan;
	char *ban;
	char *serv;
	int server = from_server;
	context;
	serv = next_arg (p, &p);
	if (my_atol (serv) != server)
		server = my_atol (serv);
	if (server < 0 || server > number_of_servers || !server_list[server].connected)
		server = from_server;
	channel = next_arg (p, &p);
	ban = next_arg (p, &p);
	if ((chan = (ChannelList *) find_in_list ((List **) & server_list[server].chan_list, channel, 0)) && ban_is_on_channel (ban, chan))
		my_send_to_server (server, "MODE %s -b %s", channel, ban);
	new_free (&serv);
	return 0;
}

char *
clear_server_flags (char *userhost)
{
	register char *uh = userhost;
	while (uh && (*uh == '~' || *uh == '#' || *uh == '+' || *uh == '-' || *uh == '=' || *uh == '^'))
		uh++;
	return uh;
}

/*  
 * (max server send) and max mirc color change is 256
 * so 256 * 8 should give us a safety margin for hackers.
 * BIG_BUFFER is 1024 * 3 is 3072 whereas 256*8 is 2048
 */

/*static char newline[3*BIG_BUFFER_SIZE+1]; */
static char newline1[3 * BIG_BUFFER_SIZE + 1];

char *
mircansi (char *line)
{
/* mconv v1.00 (c) copyright 1996 Ananda, all rights reserved.  */
/* -----------------------------------------------------------  */
/* mIRC->ansi color code convertor:     12.26.96                */
/* map of mIRC color values to ansi color codes                 */
/* format: ansi fg color        ansi bg color                   */
/* modified Colten Edwards                                      */
	struct
	{
		char *fg, *bg;
	}
	codes[16] =
	{

		{
			"[1;37m", "[47m"
		}
		,		/* white                */
		{
			"[0;30m", "[40m"
		}
		,		/* black (grey for us)  */
		{
			"[0;34m", "[44m"
		}
		,		/* blue                 */
		{
			"[0;32m", "[42m"
		}
		,		/* green                */
		{
			"[0;31m", "[41m"
		}
		,		/* red                  */
		{
			"[0;33m", "[43m"
		}
		,		/* brown                */

		{
			"[0;35m", "[45m"
		}
		,		/* magenta              */
		{
			"[1;31m", "[41m"
		}
		,		/* bright red           */
		{
			"[1;33m", "[43m"
		}
		,		/* yellow               */

		{
			"[1;32m", "[42m"
		}
		,		/* bright green         */
		{
			"[0;36m", "[46m"
		}
		,		/* cyan                 */
		{
			"[1;36m", "[46m"
		}
		,		/* bright cyan          */
		{
			"[1;34m", "[44m"
		}
		,		/* bright blue          */
		{
			"[1;35m", "[45m"
		}
		,		/* bright magenta       */
		{
			"[1;30m", "[40m"
		}
		,		/* dark grey            */
		{
			"[0;37m", "[47m"
		}		/* grey                 */
	};
	register char *sptr = line, *dptr = newline1;
	short code;

	if (!*line)
		return empty_string;
	*newline1 = 0;
	while (*sptr)
	{
		if (*sptr == '' && isdigit (sptr[1]))
		{
			sptr++;
			code = atoi (sptr);
			if (code > 15 || code <= 0)
				continue;
			while (isdigit (*sptr))
				sptr++;
			strcpy (dptr, codes[code].fg);
			while (*dptr)
				dptr++;
			if (*sptr == ',')
			{
				sptr++;
				code = atoi (sptr);
				if (code >= 0 && code <= 15)
				{
					strcpy (dptr, codes[code].bg);
					while (*dptr)
						dptr++;
				}
				while (isdigit (*sptr))
					sptr++;
			}
		}
		else if (*sptr == '')
		{
			strcpy (dptr, "[0m");
			while (*dptr)
				dptr++;
			sptr++;
		}
		else
			*dptr++ = *sptr++;
	}
	*dptr = 0;
	return newline1;
}

/* Borrowed with permission from FLiER */
char *
stripansicodes (const char *line)
{
	register char *tstr;
	register char *nstr;
	int gotansi = 0;

	tstr = (char *) line;
	nstr = newline1;
	while (*tstr)
	{
		if (*tstr == 0x1B)
			gotansi = 1;
		if (gotansi && isalpha (*tstr))
			gotansi = 0;
		else if (!gotansi)
		{
			*nstr = *tstr;
			nstr++;
		}
		tstr++;
	}
	*nstr = 0;
	return newline1;
}

char *
stripansi (char *line)
{
	register char *cp;
	char *newline;
	newline = m_strdup (line);
	for (cp = newline; *cp; cp++)
		if (*cp < 31 && *cp > 13)
			if (*cp != 1 && *cp != 15 && *cp != 22)
				*cp = (*cp & 127) | 64;
	return newline;
}


int 
check_split (char *nick, char *reason, char *chan)
{
	char *bogus = get_string_var (FAKE_SPLIT_PATS_VAR);
	char *Reason = m_strdup (reason);
	char *tmp;
	context;
	tmp = Reason;
	if (word_count (Reason) > 3)
		goto fail_split;
	if (match ("%.% %.%", Reason) && !strstr (Reason, "))"))
	{
		char *host1 = next_arg (Reason, &Reason);
		char *host2 = next_arg (Reason, &Reason);
		if (!my_stricmp (host1, host2))
			goto fail_split;
		if (match (host1, "*..*") || match (host2, "*..*"))
			goto fail_split;
		if (bogus)
		{
			char *copy = NULL;
			char *b_check;
			char *temp;
			malloc_strcpy (&copy, bogus);
			temp = copy;
			while ((b_check = next_arg (copy, &copy)))
			{
				if (match (b_check, host1) || match (b_check, host2))
				{
					new_free (&temp);
					goto fail_split;
				}
			}
			new_free (&temp);
		}
		new_free (&tmp);
		return 1;
	}
      fail_split:
	new_free (&tmp);
	return 0;
}

void 
clear_array (NickTab ** tmp)
{
	NickTab *t, *q;
	context;
	for (t = *tmp; t;)
	{
		q = t->next;
		new_free (&t->nick);
		new_free (&t->type);
		new_free ((char **) &t);
		t = q;
	}
	*tmp = NULL;
}

void 
userage (char *command, char *use)
{

	context;
	if (do_hook (USAGE_LIST, "%s %s", command, use ? use : "No Help Available for this command"))
		put_it ("%s", convert_output_format (get_fset_var (FORMAT_USAGE_FSET), "%s %s", command, convert_output_format (use ? use : "%WNo Help available for this command", NULL, NULL)));
}

char *
random_str (int min, int max)
{
	int i, ii;
	static char str[BIG_BUFFER_SIZE + 1];


	context;
	i = getrandom (min, max);
	for (ii = 0; ii < i; ii++)
		str[ii] = (char) getrandom (97, 122);
	str[ii] = '\0';
	return str;
}

int 
rename_file (char *old_file, char **new_file)
{
	char *tmp = NULL, *new_f = NULL;
	char c = 'a';
	FILE *fp;


	context;
	if (get_string_var (DCC_DLDIR_VAR))
		malloc_sprintf (&tmp, "%s/%%c%s", get_string_var (DCC_DLDIR_VAR), *new_file);
	else
		malloc_sprintf (&tmp, "%%c%s", *new_file);
	malloc_sprintf (&new_f, tmp, c);
	while ((fp = fopen (new_f, "r")) != NULL)
	{
		fclose (fp);
		c++;
		sprintf (new_f, tmp, c);
	}
	if (fp != NULL)
		fclose (fp);
	new_free (&tmp);
	new_free (&new_f);
	malloc_sprintf (new_file, "%c%s", c, *new_file);
	return 0;
}

int 
isme (char *nick)
{
	return ((my_stricmp (nick, get_server_nickname (from_server)) == 0) ? 1 : 0);
}


void 
clear_link (irc_server ** serv1)
{
	irc_server *temp = *serv1, *hold;

	while (temp != NULL)
	{
		hold = temp->next;
		new_free (&temp->name);
		new_free (&temp->link);
		new_free (&temp->time);
		new_free ((char **) &temp);
		temp = hold;
	}
	*serv1 = NULL;
}

irc_server *
add_server (irc_server ** serv1, char *channel, char *arg, int hops, char *time)
{
	irc_server *serv2;
	serv2 = (irc_server *) new_malloc (sizeof (irc_server));
	serv2->next = *serv1;
	malloc_strcpy (&serv2->name, channel);
	malloc_strcpy (&serv2->link, arg);
	serv2->hopcount = hops;
	serv2->time = m_strdup (time);
	*serv1 = serv2;
	return serv2;
}

int 
find_server (irc_server * serv1, char *channel)
{
	register irc_server *temp;

	for (temp = serv1; temp; temp = temp->next)
	{
		if (!my_stricmp (temp->name, channel))
			return 1;
	}
	return 0;
}

void 
add_split_server (char *name, char *link, int hops)
{
	irc_server *temp;
	temp = add_server (&split_link, name, link, hops, update_clock (GET_TIME));
	temp->status = SPLIT;
}

irc_server *
check_split_server (char *server)
{
	register irc_server *temp;
	for (temp = split_link; temp; temp = temp->next)
		if (!my_stricmp (temp->name, server))
			return temp;
	return NULL;
}

void 
remove_split_server (char *server)
{
	irc_server *temp;

	if ((temp = (irc_server *) remove_from_list ((List **) & split_link, server)))
	{
		new_free (&temp->name);
		new_free (&temp->link);
		new_free (&temp->time);
		new_free ((char **) &temp);
	}
}

void 
parse_364 (char *channel, char *args, char *subargs)
{
	if (!*channel || !*args || from_server < 0)
		return;

	add_server (&tmplink, channel, args, atol (subargs), update_clock (GET_TIME));
}

void 
parse_365 (char *channel, char *args, char *subargs)
{
	register irc_server *serv1;

	for (serv1 = server_last; serv1; serv1 = serv1->next)
	{
		if (!find_server (tmplink, serv1->name))
		{
			if (!(serv1->status & SPLIT))
				serv1->status = SPLIT;
			if (serv1->count)
				continue;
			serv1->time = m_strdup (update_clock (GET_TIME));
			if (do_hook (LLOOK_SPLIT_LIST, "%s %s %d %s", serv1->name, serv1->link, serv1->hopcount, serv1->time))
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETSPLIT_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
			serv1->count++;
		}
		else
		{
			if (serv1->status & SPLIT)
			{
				serv1->status = ~SPLIT;
				if (do_hook (LLOOK_JOIN_LIST, "%s %s %d %s", serv1->name, serv1->link, serv1->hopcount, serv1->time))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETJOIN_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
		}
	}
	for (serv1 = tmplink; serv1; serv1 = serv1->next)
	{
		if (!find_server (server_last, serv1->name))
		{
			if (first_time == 1)
			{
				if (do_hook (LLOOK_ADDED_LIST, "%s %s %d", serv1->name, serv1->link, serv1->hopcount))
					put_it ("%s", convert_output_format (get_fset_var (FORMAT_NETADD_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
			add_server (&server_last, serv1->name, serv1->link, serv1->hopcount, update_clock (GET_TIME));
		}
	}
	first_time = 1;
	clear_link (&tmplink);
}

void 
log_toggle (int flag, ChannelList * chan)
{
	char *logfile;

	if (((logfile = get_string_var (MSGLOGFILE_VAR)) == NULL))
	{
		bitchsay ("You must set the MSGLOGFILE first!");
		set_int_var (MSGLOG_VAR, 0);
		return;
	}
	logmsg (LOG_CURRENT, NULL, NULL, flag ? 1 : 2);
}

void 
not_on_a_channel (Window * win)
{
	if (win)
		message_to (win->refnum);
	bitchsay ("You're not on a channel!");
	message_to (0);
}

int 
are_you_opped (char *channel)
{
	return is_chanop (channel, get_server_nickname (from_server));
}

void 
error_not_opped (char *channel)
{
	say ("You're not opped on %s", channel);
}

int 
freadln (FILE * stream, char *lin)
{
	char *p;

	do
		p = fgets (lin, BIG_BUFFER_SIZE / 2, stream);
	while (p && (*lin == '#'));

	if (!p)
		return 0;
	chop (lin, 1);
	return 1;
}

/* wtf is this doing? */
char *
get_reason (char *nick)
{
	static char reason[BIG_BUFFER_SIZE / 2 + 1];
	char *temp = get_string_var (DEFAULT_REASON_VAR);

	strncpy (reason, stripansicodes (convert_output_format (temp, "%s %s", nick ? nick : "error", get_server_nickname (from_server))), sizeof (reason) - 1);
	*reason = '\0';

	return reason;
}

char *
get_signoffreason (char *nick)
{
	static char reason[BIG_BUFFER_SIZE / 2 + 1];

	*reason = '\0';

	strncpy (reason, stripansicodes (convert_output_format (get_string_var (SIGNOFF_REASON_VAR), "%s %s", nick ? nick : "error", get_server_nickname (from_server))), sizeof (reason) - 1);
	return reason;
}


char *
rights (char *string, int num)
{
	if (strlen (string) < num)
		return string;
	return (string + strlen (string) - num);
}

int 
numchar (char *string, char c)
{
	int num = 0;

	while (*string)
	{
		if (tolower (*string) == tolower (c))
			num++;
		string++;
	}
	return num;
}

char *
cluster (char *hostname)
{
	static char result[BIG_BUFFER_SIZE + 1];
	char temphost[BIG_BUFFER_SIZE + 1];
	char *host;

	if (!hostname)
		return NULL;
	host = temphost;
	*result = 0;
	memset (result, 0, sizeof (result));
	memset (temphost, 0, sizeof (temphost));
	if (strchr (hostname, '@'))
	{
		if (*hostname == '~')
			hostname++;
		strcpy (result, hostname);
		*strchr (result, '@') = '\0';
		if (strlen (result) > 9)
		{
			result[8] = '*';
			result[9] = '\0';
		}
		strcat (result, "@");
		if (!(hostname = strchr (hostname, '@')))
			return NULL;
		hostname++;
	}
	strcpy (host, hostname);

	if (*host && isdigit (*(host + strlen (host) - 1)))
	{
		/* Thanks icebreak for this small patch which fixes this function */
		int i;
		char *tmp;
		char count = 0;

		tmp = host;
		while ((tmp - host) < strlen (host))
		{
			if ((tmp = strchr (tmp, '.')) == NULL)
				break;
			count++;
			tmp++;
		}
		tmp = host;
		for (i = 0; i < count; i++)
			tmp = strchr (tmp, '.') + 1;
		*tmp = '\0';
		strcat (result, host);
		strcat (result, "*");
	}
	else
	{
		char *tmp;
		int num;

		num = 1;
		tmp = rights (host, 3);
		if (my_stricmp (tmp, "com") &&
		    my_stricmp (tmp, "edu") &&
		    my_stricmp (tmp, "net") &&
		    (stristr (host, "com") ||
		     stristr (host, "edu")))
			num = 2;
		while (host && *host && (numchar (host, '.') > num))
		{
			if ((host = strchr (host, '.')) != NULL)
				host++;
			else
				return (char *) NULL;
		}
		strcat (result, "*");
		if (my_stricmp (host, temphost))
			strcat (result, ".");
		strcat (result, host);
	}
	return result;
}

void 
set_socket_read (fd_set * rd, fd_set * wr)
{
	register int i;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (sockets[i].is_read)
			FD_SET (i, rd);
		if (sockets[i].is_write)
			FD_SET (i, wr);
	}
}

void 
scan_sockets (fd_set * rd, fd_set * wr)
{
	register int i;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (sockets[i].is_read && FD_ISSET (i, rd))
			(sockets[i].func) (i);
		if (sockets[i].is_write && FD_ISSET (i, wr))
			(sockets[i].func) (i);
	}
}

static int cparse_recurse = -1;

char *
convert_output_format (const char *format, const char *str,...)
{
	static unsigned char buffer[10 * BIG_BUFFER_SIZE + 1];
	char buffer2[2 * BIG_BUFFER_SIZE + 1];
	enum color_attributes this_color = BLACK;
	register unsigned char *t;
	register unsigned char *s;
	char *copy = NULL;
	char *tmpc = NULL;
	char *p;
	int old_who_level = who_level;
	int bold = 0;
	va_list args;
	int arg_flags;
	int do_color = get_int_var (DISPLAY_ANSI_VAR);

	malloc_strcpy (&copy, format);
	memset (buffer2, 0, BIG_BUFFER_SIZE / 4);

	if (cparse_recurse < 10)
		cparse_recurse++;
	if (str /* && !in_cparse */ )
	{

		p = (char *) str;
		va_start (args, str);
		while (p && *p)
		{
			if (*p == '%')
			{
				switch (*++p)
				{
				case 's':
					{
						char *s = (char *) va_arg (args, char *);
						if (s)
							strcat (buffer2, s);
						break;
					}
				case 'd':
					{
						int d = (int) va_arg (args, int);
						strcat (buffer2, ltoa ((long) d));
						break;
					}
				case 'c':
					{
						char c = (char) va_arg (args, int);
						buffer2[strlen (buffer2)] = c;
						break;
					}
				case 'u':
					{
						unsigned int d = (unsigned int) va_arg (args, unsigned int);
						strcat (buffer2, ltoa (d));
						break;
					}
				case 'l':
					{
						unsigned long d = (unsigned long) va_arg (args, unsigned long);
						strcat (buffer2, ltoa (d));
						break;
					}
				case '%':
					{
						buffer2[strlen (buffer2)] = '%';
						p++;
						break;
					}
				default:
					strcat (buffer2, "%");
					buffer2[strlen (buffer2)] = *p;
				}
				p++;
			}
			else
			{
				buffer2[strlen (buffer2)] = *p;
				p++;
			}
		}
		va_end (args);
	}
	else if ( /*in_cparse && */ str)
		strcpy (buffer2, str);

	s = buffer + (BIG_BUFFER_SIZE * cparse_recurse);
	memset (s, 0, BIG_BUFFER_SIZE / 4);
	tmpc = copy;
	if (!tmpc)
		goto done;
	while (*tmpc)
	{
		if (*tmpc == '%')
		{
			tmpc++;
			switch (*tmpc)
			{
			case '%':
				*s++ = *tmpc;
				break;
			case 'n':
				this_color = NO_COLOR;
				break;
			case 'W':
				this_color = WHITEB;
				break;
			case 'w':
				this_color = WHITE;
				break;
			case 'K':
				this_color = BLACKB;
				break;
			case 'k':
				this_color = BLACK;
				break;
			case 'G':
				this_color = GREENB;
				break;
			case 'g':
				this_color = GREEN;
				break;
			case 'Y':
				this_color = YELLOWB;
				break;
			case 'y':
				this_color = YELLOW;
				break;
			case 'C':
				this_color = CYANB;
				break;
			case 'c':
				this_color = CYAN;
				break;
			case 'B':
				this_color = BLUEB;
				break;
			case 'b':
				this_color = BLUE;
				break;
			case 'P':
			case 'M':
				this_color = MAGENTAB;
				break;
			case 'p':
			case 'm':
				this_color = MAGENTA;
				break;
			case 'R':
				this_color = REDB;
				break;
			case 'r':
				this_color = RED;
				break;
			case '0':
				this_color = bold ? BACK_BBLACK : BACK_BLACK;
				bold = 0;
				break;
			case '1':
				this_color = bold ? BACK_BRED : BACK_RED;
				bold = 0;
				break;
			case '2':
				this_color = bold ? BACK_BGREEN : BACK_GREEN;
				bold = 0;
				break;
			case '3':
				this_color = bold ? BACK_BYELLOW : BACK_YELLOW;
				bold = 0;
				break;
			case '4':
				this_color = bold ? BACK_BBLUE : BACK_BLUE;
				bold = 0;
				break;
			case '5':
				this_color = bold ? BACK_BMAGENTA : BACK_MAGENTA;
				bold = 0;
				break;
			case '6':
				this_color = bold ? BACK_BCYAN : BACK_CYAN;
				bold = 0;
				break;
			case '7':
				this_color = bold ? BACK_BWHITE : BACK_WHITE;
				bold = 0;
				break;
			case '8':
				this_color = REVERSE_COLOR;
				bold = 0;
				break;
			case '9':
				this_color = BOLD_COLOR;
				bold ^= 1;
				break;
			case 'F':
				this_color = BLINK_COLOR;
				break;
			case 'U':
				this_color = UNDERLINE_COLOR;
				break;
			default:
				*s++ = *tmpc;
				continue;
			}
			if (do_color)
			{
				for (t = color_str[(int) this_color]; *t; t++, s++)
					*s = *t;
			}
			tmpc++;
			continue;
		}
		else if (*tmpc == '$')
		{
			char *new_str = NULL;
			tmpc++;
			in_cparse++;
			tmpc = alias_special_char (&new_str, tmpc, buffer2, NULL, &arg_flags);
			in_cparse--;
			if (new_str)
				strcat (s, new_str);
			new_free (&new_str);
			while (*s)
			{
				if (*s == 255)
					*s = ' ';
				s++;
			}
			if (!tmpc)
				break;
			continue;
		}
		else
			*s = *tmpc;
		tmpc++;
		s++;
	}
	*s = 0;
      done:
	s = buffer + (BIG_BUFFER_SIZE * cparse_recurse);
	if (*s)
		strcat (s, color_str[NO_COLOR]);
	who_level = old_who_level;
	new_free (&copy);

	cparse_recurse--;
	return s;
}


int 
matchmcommand (char *origline, int count)
{
	int startnum = 0;
	int endnum = 0;
	char *tmpstr;
	char tmpbuf[BIG_BUFFER_SIZE];

	strcpy (tmpbuf, origline);
	tmpstr = tmpbuf;
	if (*tmpstr == '*')
		return (1);
	while (tmpstr && *tmpstr)
	{
		startnum = 0;
		endnum = 0;
		if (tmpstr && *tmpstr && *tmpstr == '-')
		{
			while (tmpstr && *tmpstr && !isdigit (*tmpstr))
				tmpstr++;
			endnum = atoi (tmpstr);
			startnum = 1;
			while (tmpstr && *tmpstr && isdigit (*tmpstr))
				tmpstr++;
		}
		else
		{
			while (tmpstr && *tmpstr && !isdigit (*tmpstr))
				tmpstr++;
			startnum = atoi (tmpstr);
			while (tmpstr && *tmpstr && isdigit (*tmpstr))
				tmpstr++;
			if (tmpstr && *tmpstr && *tmpstr == '-')
			{
				while (tmpstr && *tmpstr && !isdigit (*tmpstr))
					tmpstr++;
				endnum = atoi (tmpstr);
				if (!endnum)
					endnum = 1000;
				while (tmpstr && *tmpstr && isdigit (*tmpstr))
					tmpstr++;
			}
		}
		if (count == startnum || (count >= startnum && count <= endnum))
			return (1);
	}
	if (count == startnum || (count >= startnum && count <= endnum))
		return (1);
	return (0);
}

ChannelList *
prepare_command (int *active_server, char *channel, int need_op)
{
	int server = 0;
	ChannelList *chan = NULL;

	if (!channel && !curr_scr_win->current_channel)
	{
		context;
		if (need_op != 3)
			not_on_a_channel (curr_scr_win);
		return NULL;
	}
	server = curr_scr_win->server;
	*active_server = server;
	if (!(chan = lookup_channel (channel ? channel : curr_scr_win->current_channel, server, 0)))
	{
		context;
		if (need_op != 3)
			not_on_a_channel (curr_scr_win);
		return NULL;
	}
	if (need_op == NEED_OP && chan && !chan->chop)
	{
		context;
		error_not_opped (chan->channel);
		return NULL;
	}
	return chan;
}

/* XXX nasty!!! */
char *
make_channel (char *chan)
{
	static char buffer[BIG_BUFFER_SIZE + 1];

	*buffer = 0;
	if (*chan != '#' && *chan != '&' && *chan != '+' && *chan != '*')
		snprintf (buffer, IRCD_BUFFER_SIZE - 2, "#%s", chan);
	else
		strncpy (buffer, chan, BIG_BUFFER_SIZE);
	return buffer;
}

void 
add_to_irc_map (char *server1, char *distance)
{
	irc_server *tmp, *insert, *prev;
	int dist = 0;
	if (distance)
		dist = atoi (distance);
	tmp = (irc_server *) new_malloc (sizeof (irc_server));
	malloc_strcpy (&tmp->name, server1);
	tmp->hopcount = dist;
	if (!map)
	{
		map = tmp;
		return;
	}
	for (insert = map, prev = map; insert && insert->hopcount < dist;)
	{
		prev = insert;
		insert = insert->next;
	}
	if (insert && insert->hopcount >= dist)
	{
		tmp->next = insert;
		if (insert == map)
			map = tmp;
		else
			prev->next = tmp;
	}
	else
		prev->next = tmp;
}

void 
show_server_map (void)
{
	int prevdist = 0;
	irc_server *tmp;
	char tmp1[80];
	char tmp2[BIG_BUFFER_SIZE + 1];
	char *ascii = "-> ";


	if (map)
		prevdist = map->hopcount;

	for (tmp = map; tmp; tmp = map)
	{
		map = tmp->next;
		if (!tmp->hopcount || tmp->hopcount != prevdist)
			strmcpy (tmp1, convert_output_format ("%K[%G$0%K]", "%d", tmp->hopcount), 79);
		else
			*tmp1 = 0;
		snprintf (tmp2, BIG_BUFFER_SIZE, "$G %%W$[-%d]1%%c $0 %s", tmp->hopcount * 3, tmp1);
		put_it ("%s", convert_output_format (tmp2, "%s %s", tmp->name, prevdist != tmp->hopcount ? ascii : empty_string));
		prevdist = tmp->hopcount;
		new_free (&tmp->name);
		new_free ((char **) &tmp);
	}
}

extern int timed_server (void *);
extern int in_timed_server;
void 
check_server_connect (int server)
{
	if ((from_server == -1) || (!in_timed_server && server_list[from_server].last_msg + 50 < time (NULL)))
	{
		add_timer ("", 10, 1, timed_server, m_strdup ("0"), NULL);
		in_timed_server++;
	}
}

char *
country (char *hostname)
{
	typedef struct _domain
	{
		char *code;
		char *country;
	}
	Domain;
	static Domain domain[] =
	{
		{"AD", "Andorra"},
		{"AE", "United Arab Emirates"},
		{"AF", "Afghanistan"},
		{"AG", "Antigua and Barbuda"},
		{"AI", "Anguilla"},
		{"AL", "Albania"},
		{"AM", "Armenia"},
		{"AN", "Netherlands Antilles"},
		{"AO", "Angola"},
		{"AQ", "Antarctica (pHEAR)"},
		{"AR", "Argentina"},
		{"AS", "American Samoa"},
		{"AT", "Austria"},
		{"AU", "Australia"},
		{"AW", "Aruba"},
		{"AZ", "Azerbaijan"},
		{"BA", "Bosnia and Herzegovina"},
		{"BB", "Barbados"},
		{"BD", "Bangladesh"},
		{"BE", "Belgium"},
		{"BF", "Burkina Faso"},
		{"BG", "Bulgaria"},
		{"BH", "Bahrain"},
		{"BI", "Burundi"},
		{"BJ", "Benin"},
		{"BM", "Bermuda"},
		{"BN", "Brunei Darussalam"},
		{"BO", "Bolivia"},
		{"BR", "Brazil"},
		{"BS", "Bahamas"},
		{"BT", "Bhutan"},
		{"BV", "Bouvet Island"},
		{"BW", "Botswana"},
		{"BY", "Belarus"},
		{"BZ", "Belize"},
		{"CA", "Canada"},
		{"CC", "Cocos Islands"},
		{"CF", "Central African Republic"},
		{"CG", "Congo"},
		{"CH", "Switzerland"},
		{"CI", "Cote D'ivoire"},
		{"CK", "Cook Islands"},
		{"CL", "Chile"},
		{"CM", "Cameroon"},
		{"CN", "China"},
		{"CO", "Colombia"},
		{"CR", "Costa Rica"},
		{"CS", "Former Czechoslovakia"},
		{"CU", "Cuba"},
		{"CV", "Cape Verde"},
		{"CX", "Christmas Island"},
		{"CY", "Cyprus"},
		{"CZ", "Czech Republic"},
		{"DE", "Germany"},
		{"DJ", "Djibouti"},
		{"DK", "Denmark"},
		{"DM", "Dominica"},
		{"DO", "Dominican Republic"},
		{"DZ", "Algeria"},
		{"EC", "Ecuador"},
		{"EE", "Estonia"},
		{"EG", "Egypt"},
		{"EH", "Western Sahara"},
		{"ER", "Eritrea"},
		{"ES", "Spain"},
		{"ET", "Ethiopia"},
		{"FI", "Finland"},
		{"FJ", "Fiji"},
		{"FK", "Falkland Islands"},
		{"FM", "Micronesia"},
		{"FO", "Faroe Islands"},
		{"FR", "France"},
		{"FX", "France, Metropolitan"},
		{"GA", "Gabon"},
		{"GB", "Great Britain"},
		{"GD", "Grenada"},
		{"GE", "Georgia"},
		{"GF", "French Guiana"},
		{"GH", "Ghana"},
		{"GI", "Gibraltar"},
		{"GL", "Greenland"},
		{"GM", "Gambia"},
		{"GN", "Guinea"},
		{"GP", "Guadeloupe"},
		{"GQ", "Equatorial Guinea"},
		{"GR", "Greece"},
		{"GS", "S. Georgia and S. Sandwich Isls."},
		{"GT", "Guatemala"},
		{"GU", "Guam"},
		{"GW", "Guinea-Bissau"},
		{"GY", "Guyana"},
		{"HK", "Hong Kong"},
		{"HM", "Heard and McDonald Islands"},
		{"HN", "Honduras"},
		{"HR", "Croatia"},
		{"HT", "Haiti"},
		{"HU", "Hungary"},
		{"ID", "Indonesia"},
		{"IE", "Ireland"},
		{"IL", "Israel"},
		{"IN", "India"},
		{"IO", "British Indian Ocean Territory"},
		{"IQ", "Iraq"},
		{"IR", "Iran"},
		{"IS", "Iceland"},
		{"IT", "Italy"},
		{"JM", "Jamaica"},
		{"JO", "Jordan"},
		{"JP", "Japan"},
		{"KE", "Kenya"},
		{"KG", "Kyrgyzstan"},
		{"KH", "Cambodia"},
		{"KI", "Kiribati"},
		{"KM", "Comoros"},
		{"KN", "St. Kitts and Nevis"},
		{"KP", "North Korea"},
		{"KR", "South Korea"},
		{"KW", "Kuwait"},
		{"KY", "Cayman Islands"},
		{"KZ", "Kazakhstan"},
		{"LA", "Laos"},
		{"LB", "Lebanon"},
		{"LC", "Saint Lucia"},
		{"LI", "Liechtenstein"},
		{"LK", "Sri Lanka"},
		{"LR", "Liberia"},
		{"LS", "Lesotho"},
		{"LT", "Lithuania"},
		{"LU", "Luxembourg"},
		{"LV", "Latvia"},
		{"LY", "Libya"},
		{"MA", "Morocco"},
		{"MC", "Monaco"},
		{"MD", "Moldova"},
		{"MG", "Madagascar"},
		{"MH", "Marshall Islands"},
		{"MK", "Macedonia"},
		{"ML", "Mali"},
		{"MM", "Myanmar"},
		{"MN", "Mongolia"},
		{"MO", "Macau"},
		{"MP", "Northern Mariana Islands"},
		{"MQ", "Martinique"},
		{"MR", "Mauritania"},
		{"MS", "Montserrat"},
		{"MT", "Malta"},
		{"MU", "Mauritius"},
		{"MV", "Maldives"},
		{"MW", "Malawi"},
		{"MX", "Mexico"},
		{"MY", "Malaysia"},
		{"MZ", "Mozambique"},
		{"NA", "Namibia"},
		{"NC", "New Caledonia"},
		{"NE", "Niger"},
		{"NF", "Norfolk Island"},
		{"NG", "Nigeria"},
		{"NI", "Nicaragua"},
		{"NL", "Netherlands"},
		{"NO", "Norway"},
		{"NP", "Nepal"},
		{"NR", "Nauru"},
		{"NT", "Neutral Zone (pHEAR)"},
		{"NU", "Niue"},
		{"NZ", "New Zealand"},
		{"OM", "Oman"},
		{"PA", "Panama"},
		{"PE", "Peru"},
		{"PF", "French Polynesia"},
		{"PG", "Papua New Guinea"},
		{"PH", "Philippines"},
		{"PK", "Pakistan"},
		{"PL", "Poland"},
		{"PM", "St. Pierre and Miquelon"},
		{"PN", "Pitcairn"},
		{"PR", "Puerto Rico"},
		{"PT", "Portugal"},
		{"PW", "Palau"},
		{"PY", "Paraguay"},
		{"QA", "Qatar"},
		{"RE", "Reunion"},
		{"RO", "Romania"},
		{"RU", "Russian Federation"},
		{"RW", "Rwanda"},
		{"SA", "Saudi Arabia"},
		{"Sb", "Solomon Islands"},
		{"SC", "Seychelles"},
		{"SD", "Sudan"},
		{"SE", "Sweden"},
		{"SG", "Singapore"},
		{"SH", "St. Helena"},
		{"SI", "Slovenia"},
		{"SJ", "Svalbard and Jan Mayen Islands"},
		{"SK", "Slovak Republic"},
		{"SL", "Sierra Leone"},
		{"SM", "San Marino"},
		{"SN", "Senegal"},
		{"SO", "Somalia"},
		{"SR", "Suriname"},
		{"ST", "Sao Tome and Principe"},
		{"SU", "Former USSR"},
		{"SV", "El Salvador"},
		{"SY", "Syria"},
		{"SZ", "Swaziland"},
		{"TC", "Turks and Caicos Islands"},
		{"TD", "Chad"},
		{"TF", "French Southern Territories"},
		{"TG", "Togo"},
		{"TH", "Thailand"},
		{"TJ", "Tajikistan"},
		{"TK", "Tokelau"},
		{"TM", "Turkmenistan"},
		{"TN", "Tunisia"},
		{"TO", "Tonga"},
		{"TP", "East Timor"},
		{"TR", "Turkey"},
		{"TT", "Trinidad and Tobago"},
		{"TV", "Tuvalu"},
		{"TW", "Taiwan"},
		{"TZ", "Tanzania"},
		{"UA", "Ukraine"},
		{"UG", "Uganda"},
		{"UK", "United Kingdom"},
		{"UM", "US Minor Outlying Islands"},
		{"US", "United States of America"},
		{"UY", "Uruguay"},
		{"UZ", "Uzbekistan"},
		{"VA", "Vatican City State"},
		{"VC", "St. Vincent and the grenadines"},
		{"VE", "Venezuela"},
		{"VG", "British Virgin Islands"},
		{"VI", "US Virgin Islands"},
		{"VN", "Vietnam"},
		{"VU", "Vanuatu"},
		{"WF", "Wallis and Futuna Islands"},
		{"WS", "Samoa"},
		{"YE", "Yemen"},
		{"YT", "Mayotte"},
		{"YU", "Yugoslavia"},
		{"ZA", "South Africa"},
		{"ZM", "Zambia"},
		{"ZR", "Zaire"},
		{"ZW", "Zimbabwe"},
		{"COM", "Internic Commercial"},
		{"EDU", "Educational Institution"},
		{"GOV", "Government"},
		{"INT", "International"},
		{"MIL", "Military"},
		{"NET", "Internic Network"},
		{"ORG", "Internic Non-Profit Organization"},
		{"RPA", "Old School ARPAnet"},
		{"ATO", "Nato Fiel"},
		{"MED", "United States Medical"},
		{"ARPA", "Reverse DNS"},
		{NULL, NULL}
	};
	static char unknown[] = "unknown";
	char *p;
	int i = 0;
	if (!hostname || !*hostname || isdigit (hostname[strlen (hostname) - 1]))
		return unknown;
	if ((p = strrchr (hostname, '.')))
		p++;
	else
		p = hostname;
	for (i = 0; domain[i].code; i++)
		if (!my_stricmp (p, domain[i].code))
			return domain[i].country;
	return unknown;
}

char *
do_nslookup (char *host)
{
	struct hostent *temp;
	struct in_addr temp1;

	if (!host)
		return NULL;

	if (isdigit (*(host + strlen (host) - 1)))
	{
		temp1.s_addr = inet_addr (host);
		alarm (1);
		temp = gethostbyaddr ((char *) &temp1.s_addr, sizeof (temp1.s_addr), AF_INET);
		alarm (0);
	}
	else
	{
		alarm (1);
		temp = gethostbyname (host);
		alarm (0);
	}
	if (do_hook (NSLOOKUP_LIST, "%s %s %s", host, temp ? temp->h_name : "", temp ? (char *) inet_ntoa (*(struct in_addr *) temp->h_addr) : ""))
	{
		if (!temp)
			bitchsay ("Error looking up %s", host);
		else
			bitchsay ("%s is %s (%s)", host, temp->h_name, (char *) inet_ntoa (*(struct in_addr *) temp->h_addr));
	}
	return ((char *) (temp ? temp->h_name : host));
}

int 
charcount (char *string, char what)
{
	int x = 0;
	char *place = string;

	while (*place)
		if (*place++ == what)
			x++;

	return x;
}

#ifdef HAVE_PTHREADS
#include <pthread.h>
/*------------------
 | Experemental.. nslookup now splits off a thread to do the work
 */

void *
do_nslookup_blah (void *arg)
{
	struct hostent *temp;
	struct in_addr temp1;
	char *host = (char *) arg;

	if (!host)
		pthread_exit (NULL);

	if (isdigit (*(host + strlen (host) - 1)))
	{
		temp1.s_addr = inet_addr (host);
		temp = gethostbyaddr ((char *) &temp1.s_addr, sizeof (temp1.s_addr), AF_INET);
	}
	else
	{
		temp = gethostbyname (host);
	}
	if (do_hook (NSLOOKUP_LIST, "%s %s %s", host, temp ? temp->h_name : "", temp ? (char *) inet_ntoa (*(struct in_addr *) temp->h_addr) : ""))
	{
		if (!temp)
			bitchsay ("Error looking up %s", host);
		else
			bitchsay ("%s is %s (%s)", host, temp->h_name, (char *) inet_ntoa (*(struct in_addr *) temp->h_addr));
	}
	new_free (&arg);
	pthread_exit (NULL);
	return NULL;
}

/* arg == hostname */
void 
do_nslookup_thread (void *arg)
{
	pthread_t b;

	if (arg == NULL)
		return;

	pthread_create (&b, NULL, do_nslookup_blah, m_strdup (arg));
	pthread_detach (b);
}

#endif /* HAVE_PTHREADS */
