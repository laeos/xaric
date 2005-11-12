#ident "%W%"
/*
 * numbers.c:handles all those strange numeric response dished out by that
 * wacky, nutty program we call ircd 
 *
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "dcc.h"
#include "input.h"
#include "commands.h"
#include "ircaux.h"
#include "vars.h"
#include "lastlog.h"
#include "list.h"
#include "hook.h"
#include "server.h"
#include "whois.h"
#include "numbers.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "names.h"
#include "whois.h"
#include "funny.h"
#include "parse.h"
#include "misc.h"
#include "status.h"
#include "struct.h"
#include "timer.h"
#include "fset.h"

static void channel_topic (char *, char **, int);
static void not_valid_channel (char *, char **);
static void cannot_join_channel (char *, char **);

static int number_of_bans = 0;

extern void parse_364 (char *, char *, char *);
extern void parse_365 (char *, char *, char *);
extern void add_to_irc_map (char *, char *);
extern void show_server_map (void);
extern int stats_k_grep (char **);
extern int doing_who;
extern void remove_from_server_list (int);


#ifdef HAVE_GETTIMEOFDAY
extern struct timeval in_sping;
#else
extern time_t in_sping;
#endif


/*
 * numeric_banner: This returns in a static string of either "xxx" where
 * xxx is the current numeric, or "***" if SHOW_NUMBERS is OFF 
 */
static char *
numeric_banner (void)
{
	static char thing[4];
	if (!get_int_var (SHOW_NUMERICS_VAR))
		return line_thing;
	sprintf (thing, "%3.3u", -current_numeric);
	return thing;
}

int 
check_sync (int comm, char *channel, char *nick, char *whom, char *bantime, ChannelList * chan)
{
	ChannelList *tmp = NULL;
	BanList *new;

	if (!chan)
		if (!(tmp = lookup_channel (channel, from_server, CHAN_NOUNLINK)))
			return -1;	/* channel lookup problem */
	if (!in_join_list (channel, from_server))
		return -1;

	if (tmp == NULL)
		tmp = chan;

	switch (comm)
	{
	case 367:		/* bans on channel */
		{
			if (tmp)
			{
				new = (BanList *) new_malloc (sizeof (BanList));
				malloc_strcpy (&new->ban, nick);
				if (bantime)
					new->time = strtoul (bantime, NULL, 10);
				else
					new->time = time (NULL);
				if (whom)
					new->setby = m_strdup (whom);
				add_to_list ((struct list **) & tmp->bans, (struct list *) new);
			}
		}
	default:
		break;
	}
	return 0;
}

static int 
handle_stats (int comm, char *from, char **ArgList)
{
	char *format_str = NULL;
	char *format2_str = NULL;
	char *args;
	args = PasteArgs (ArgList, 0);
	switch (comm)
	{
/*
   ^on ^raw_irc "% 211 *" {suecho \(L:line\) $[20]sklnick($3) : $4|$5|$6|$7|$8|$strip(: 9)|$strip(: $10);suecho $[8]11 $[30]sklhost($3)}
   ^on ^raw_irc "% 212 *" {suecho $[9]3: $4 $5}
   ^on ^raw_irc "% 213 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
   ^on ^raw_irc "% 214 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
   ^on ^raw_irc "% 215 *" {suecho \($3:line\) \($[20]4\)\($[20]6\) \($[5]7\)\($strip(: $[4]8)\)}
   ^on ^raw_irc "% 216 *" {suecho \($3:line\) \($[20]4\) \($[15]6\)}
   ^on ^raw_irc "% 218 *" {suecho \($3:line\) \($[5]4\) \($[5]5\) \($[5]6\) \($[5]7\) \($strip(: $[8]8)\)}
   ^on ^raw_irc "% 242 *" {suecho $0 has been up for $5- }
   ^on ^raw_irc "% 243 *" {suecho \($3:line\) \($[20]4\) \($[9]6\)}
   ^on ^raw_irc "% 244 *" {suecho \($3:line\) \($[25]4\) \($[25]6\)}
 */

/*   c - Shows C/N lines
 * ^ b - Shows B lines
 * ^ d - Shows D lines
 * ^ e - Shows E lines
 * ^ f - Shows F lines
 *   g - Shows G lines
 *   h - Shows H/L lines
 *   i - Shows I lines
 *   K - Shows K lines (or matched klines)
 *   k - Shows temporary K lines (or matched temp klines)
 *   L - Shows IP and generic info about [nick]
 *   l - Shows hostname and generic info about [nick]
 *   m - Shows commands and their usage
 *   o - Shows O/o lines
 *   p - Shows opers connected and their idle times
 *   r - Shows resource usage by ircd (only in DEBUGMODE)
 * * t - Shows generic server stats
 *   u - Shows server uptime
 *   v - Shows connected servers and their idle times
 *   y - Shows Y lines
 * * z - Shows memory stats
 *   ? - Shows connected servers and sendq info about them
 */
	case 211:
		format_str = "L:line $[20]left($rindex([ $1) $1)  :$[-5]2 $[-5]3 $[-6]4 $[-5]5 $[-6]6 $[-8]strip(: $7) $strip(: $8)";
		format2_str = "$[8]9 $[30]mid(${rindex([ $1)+1} ${rindex(] $1) - $rindex([ $1) + 1} $1)";
		break;
	case 212:
		format_str = "$[10]1 $[-10]2 $[-10]3";
		break;
	case 213:
	case 214:
	case 215:
		format_str = "$1:line $[25]2 $[25]4 $[-5]5 $[-5]6";
		break;
	case 216:
		format_str = "$1:line $[25]2 $[20]4  $3";
		break;
	case 218:
		format_str = "$1:line $[-5]2 $[-6]3 $[-6]4 $[-6]5 $[-10]6";
		break;
	case 241:
		format_str = "$1:line $2-";
		break;
	case 242:
		format_str = "%G$0%n has been up for %W$3-";
		break;
	case 243:
		format_str = "$1:line $[25]2 $[9]4";
		break;
	case 244:
		format_str = "$1:line $[25]2 $[25]4";
	default:
		break;
	}
	if (format_str && args)
		put_it ("%s", convert_output_format (format_str, "%s %s", from, args));
	if (format2_str && args)
		put_it ("%s", convert_output_format (format2_str, "%s %s", from, args));
	return 0;
}

static int 
trace_oper (char *from, char **ArgList)
{
	char *rest = PasteArgs (ArgList, 0);
	put_it ("%s", convert_output_format (get_format (FORMAT_TRACE_OPER_FSET), "%s %s", from, rest));
	return 0;
}

static int 
trace_server (char *from, char **ArgList)
{
	char *rest = PasteArgs (ArgList, 0);
	put_it ("%s", convert_output_format (get_format (FORMAT_TRACE_SERVER_FSET), "%s %s", from, rest));
	return 0;
}

static int 
trace_user (char *from, char **ArgList)
{
	char *rest = PasteArgs (ArgList, 0);
	put_it ("%s", convert_output_format (get_format (FORMAT_TRACE_USER_FSET), "%s %s", from, rest));
	return 0;
}

static int 
check_server_sync (char *from, char **ArgList)
{
	static char *desync = NULL;

	if (!desync || (desync && !match (desync, from)))
	{
		if (!match (from, get_server_name (from_server)))
		{
			malloc_strcpy (&desync, from);
			if (do_hook (DESYNC_MESSAGE_LIST, "%s %s", from, ArgList[0]))
				put_it ("%s", convert_output_format (get_format (FORMAT_DESYNC_FSET), "%s %s %s", update_clock (GET_TIME), ArgList[0], from));
			return 1;
		}
	}
	return 0;
}

/*
 * display_msg: handles the displaying of messages from the variety of
 * possible formats that the irc server spits out.  you'd think someone would
 * simplify this 
 */
void 
display_msg (char *from, char **ArgList)
{
	char *ptr;
	char *rest;
	int drem;

	rest = PasteArgs (ArgList, 0);
	if (from && (my_strnicmp (get_server_itsname (from_server), from,
			   strlen (get_server_itsname (from_server))) == 0))
		from = NULL;
	if ((ptr = strchr (rest, ':')))
		*ptr++ = 0;
	drem = (int) (from) /*&& (!get_int_var(SUPPRESS_FROM_REMOTE_SERVER)) */ ;

	put_it ("%s %s%s%s%s%s%s",
		numeric_banner (),
		strlen (rest) ? rest : empty_str,
/*                strlen(rest) && ptr ? ":"      : empty_str, */
		strlen (rest) ? space_str : empty_str,
		ptr ? ptr : empty_str,
		drem ? "(from " : empty_str,
		drem ? from : empty_str,
		drem ? ")" : empty_str
		);
}

static void 
channel_topic (char *from, char **ArgList, int what)
{
	char *topic, *channel;
	ChannelList *chan;

	if (ArgList[1] && is_channel (ArgList[0]))
	{
		channel = ArgList[0];
		topic = ArgList[1];
		message_from (channel, LOG_CRAP);
		if (what == 333 && ArgList[2])
			put_it ("%s", convert_output_format (get_format (FORMAT_TOPIC_SETBY_FSET), "%s %s %s %l", update_clock (GET_TIME), channel, topic, strtoul (ArgList[2], NULL, 10)));
		else if (what != 333)
		{
			if ((chan = lookup_channel (channel, from_server, 0)))
				malloc_strcpy (&chan->topic, topic);
			put_it ("%s", convert_output_format (get_format (FORMAT_TOPIC_FSET), "%s %s %s", update_clock (GET_TIME), channel, topic));

		}
	}
	else
	{
		PasteArgs (ArgList, 0);
		message_from (NULL, LOG_CURRENT);
		put_it ("%s", convert_output_format (get_format (FORMAT_TOPIC_FSET), "%s %s", update_clock (GET_TIME), ArgList[0]));
	}
}

static void 
not_valid_channel (char *from, char **ArgList)
{
	char *channel;
	char *s;

	if (!(channel = ArgList[0]) || !ArgList[1])
		return;
	PasteArgs (ArgList, 1);
	s = get_server_name (from_server);
	if (0 == my_strnicmp (s, from, strlen (s)))
	{
		remove_channel (channel, from_server);
		put_it ("%s", convert_output_format (get_format (FORMAT_SERVER_MSG2_FSET), "%s %s %s", update_clock (GET_TIME), channel, ArgList[1]));
	}
}

/* from ircd .../include/numeric.h */
/*
   #define ERR_CHANNELISFULL    471
   #define ERR_INVITEONLYCHAN   473
   #define ERR_BANNEDFROMCHAN   474
   #define ERR_BADCHANNELKEY    475
   #define ERR_BADCHANMASK      476
 */
static void 
cannot_join_channel (char *from, char **ArgList)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	const char *f = NULL;
	char *chan;

	if (ArgList[0])
		chan = ArgList[0];
	else
		chan = get_chan_from_join_list (from_server);

	remove_from_join_list (chan, from_server);
	if (!is_on_channel (chan, from_server, get_server_nickname (from_server)))
		remove_channel (chan, from_server);
	else
		return;


	PasteArgs (ArgList, 0);
	strcpy (buffer, ArgList[0]);
	switch (-current_numeric)
	{
	case 437:
		strcat (buffer, " (Channel is temporarily unavailable)");
		break;
	case 471:
		strcat (buffer, " (Channel is full)");
		f = get_format (FORMAT_471_FSET);
		break;
	case 473:
		strcat (buffer, " (You must be invited)");
		f = get_format (FORMAT_473_FSET);
		break;
	case 474:
		strcat (buffer, " (You are banned)");
		f = get_format (FORMAT_474_FSET);
		break;
	case 475:
		strcat (buffer, " (Bad channel key)");
		f = get_format (FORMAT_475_FSET);
		break;
	case 476:
		strcat (buffer, " (Bad channel mask)");
		f = get_format (FORMAT_476_FSET);
		break;
	default:
		return;
	}
	if (f)
		put_it ("%s", convert_output_format (f, "%s %s", update_clock (GET_TIME), ArgList[0]));
	else
		put_it ("%s %s", numeric_banner (), buffer);
}

static int 
handle_server_ping (int comm, char *from, char **ArgList)
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval diff =
	{0};
#else
	time_t diff = 0;
#endif
	char buf[80];

	get_time (&diff);
	*buf = 0;

#ifdef HAVE_GETTIMEOFDAY
	if (!in_sping.tv_sec)
		return 0;
	if (comm == 351)
	{
		sprintf (buf, "%2.4f second%s", time_diff (in_sping, diff), (int) time_diff (in_sping, diff) ? "s" : empty_str);
#else
	if (!in_sping)
		return 0;
	if (comm == 351)
	{
		sprintf (buf, "%d second%s", diff - in_sping, diff - in_sping ? "s" : empty_str);
#endif

		put_it ("%s", convert_output_format ("$G Server PONG from %W$0%n $1-", "%s %s", from, buf));
#ifdef HAVE_GETTIMEOFDAY
		memset (&in_sping, 0, sizeof (struct timeval));
#else
		in_sping = 0;
#endif
	}
	else
		put_it ("%s", convert_output_format ("No such server to ping", NULL, NULL));
	return 1;
}

static int 
handle_server_stats (char *from, char **ArgList, int comm)
{
	static int norm = 0, invisible = 0, servers = 0, ircops = 0, unknown = 0,
	  chans = 0, local_users = 0, total_users;
	char tmp[80];
	int ret = 1;
	char *line = alloca (strlen (ArgList[0]) + 1);
	strcpy (line, ArgList[0]);
	switch (comm)
	{
	case 251:		/* number of users */
		BreakArgs (line, NULL, ArgList, 1);
		if (ArgList[2] && ArgList[5] && ArgList[8])
		{
			servers = ircops = unknown = chans = 0;
			norm = my_atol (ArgList[2]);
			invisible = my_atol (ArgList[5]);
			servers = my_atol (ArgList[8]);
		}
		break;
	case 252:		/* number of ircops */
		if (ArgList[0])
		{
			ircops = my_atol (ArgList[0]);
		}
		break;
	case 253:		/* number of unknown */
		if (ArgList[0])
		{
			unknown = my_atol (ArgList[0]);
		}
		break;

	case 254:		/* number of channels */
		if (ArgList[0])
		{
			chans = my_atol (ArgList[0]);
		}
		break;
	case 255:		/* number of local print it out */
		BreakArgs (line, NULL, ArgList, 1);
		if (ArgList[2])
			local_users = my_atol (ArgList[2]);
		total_users = norm + invisible;
		if (total_users)
		{
			sprintf (tmp, "%3.0f", (float) (((float) local_users / (float) total_users) * 100));
			put_it ("%s", convert_output_format ("$G %K[%nlocal users on irc%K(%n\002$0\002%K)]%n $1%%", "%d %s", local_users, tmp));
			sprintf (tmp, "%3.0f", (float) (((float) norm / (float) total_users) * 100));
			put_it ("%s", convert_output_format ("$G %K[%nglobal users on irc%K(%n\002$0\002%K)]%n $1%%", "%d %s", norm, tmp));
			sprintf (tmp, "%3.0f", (float) (((float) invisible / (float) total_users) * 100));
			put_it ("%s", convert_output_format ("$G %K[%ninvisible users on irc%K(%n\002$0\002%K)]%n $1%%", "%d %s", invisible, tmp));
			sprintf (tmp, "%3.0f", (float) (((float) ircops / (float) total_users) * 100));
			put_it ("%s", convert_output_format ("$G %K[%nircops on irc%K(%n\002$0\002%K)]%n $1%%", "%d %s", ircops, tmp));
		}
		put_it ("%s", convert_output_format ("$G %K[%ntotal users on irc%K(%n\002$0\002%K)]%n", "%d", total_users));
		put_it ("%s", convert_output_format ("$G %K[%nunknown connections%K(%n\002$0\002%K)]%n", "%d", unknown));

		put_it ("%s", convert_output_format ("$G %K[%ntotal servers on irc%K(%n\002$0\002%K)]%n %K(%navg. $1 users per server%K)", "%d %d", servers, (servers) ? (int) (total_users / servers) : 0));

		put_it ("%s", convert_output_format ("$G %K[%ntotal channels created%K(%n\002$0\002%K)]%n %K(%navg. $1 users per channel%K)", "%d %d", chans, (chans) ? (int) (total_users / chans) : 0));
		break;
	case 250:
		{
			char *p;
			BreakArgs (line, NULL, ArgList, 1);
			if (ArgList[3] && ArgList[4])
			{
				p = ArgList[4];
				p++;
				put_it ("%s", convert_output_format ("$G %K[%nHighest client connection count%K(%n\002$0\002%K) %K(%n\002$1\002%K)]%n", "%s %s", ArgList[3], p));
			}
			break;
		}
	default:
		ret = 0;
		break;
	}
	return ret;
}

static void 
whohas_nick (WhoisStuff * stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp (stuff->user, "<UNKNOWN>"))
		return;
	put_it ("%s", convert_output_format ("$G Your nick %R[%n$0%R]%n is owned by %W$1@$2", "%s %s %s", nick, stuff->user, stuff->host));
	return;
}


/*
 * numbered_command: does (hopefully) the right thing with the numbered
 * responses from the server.  I wasn't real careful to be sure I got them
 * all, but the default case should handle any I missed (sorry) 
 */
void 
numbered_command (char *from, int comm, char **ArgList)
{
	char *user;
	char none_of_these = 0;
	int flag, lastlog_level;

	if (!ArgList[1] || !from || !*from)
		return;

	user = (*ArgList[0]) ? ArgList[0] : NULL;

	lastlog_level = set_lastlog_msg_level (LOG_CRAP);
	message_from (NULL, LOG_CRAP);
	ArgList++;
	current_numeric = -comm;	/* must be negative of numeric! */

	switch (comm)
	{
	case 001:		/* #define RPL_WELCOME          001 */
		{
			yell ("For more information about \002Xaric\002 type \002/about\002");
			set_server2_8 (from_server, 1);
			accept_server_nickname (from_server, user);
			set_server_motd (from_server, 1);
			server_is_connected (from_server, 1);
			load_scripts ();
			PasteArgs (ArgList, 0);
			if (do_hook (current_numeric, "%s %s", from, *ArgList))
				display_msg (from, ArgList);
			if (send_umode && *send_umode == '+')
				send_to_server ("MODE %s %s", user, send_umode);
			break;
		}
	case 004:		/* #define RPL_MYINFO           004 */
		{
			got_initial_version (ArgList);
			load_scripts ();
			PasteArgs (ArgList, 0);
			if (do_hook (current_numeric, "%s %s", from, *ArgList))
				display_msg (from, ArgList);
			break;
		}
	case 250:
	case 251:
	case 252:
	case 253:
	case 254:
	case 255:
		if (do_hook (current_numeric, "%s %s", from, *ArgList))
		{

			if (!handle_server_stats (from, ArgList, comm))
				display_msg (from, ArgList);
		}
		break;

	case 301:		/* #define RPL_AWAY             301 */
		user_is_away (from, ArgList);
		break;

	case 302:		/* #define RPL_USERHOST         302 */
		userhost_returned (from, ArgList);
		break;

	case 303:		/* #define RPL_ISON             303 */
		ison_returned (from, ArgList);
		break;

	case 311:		/* #define RPL_WHOISUSER        311 */
		whois_name (from, ArgList);
		break;

	case 312:		/* #define RPL_WHOISSERVER      312 */
		whois_server (from, ArgList);
		break;

	case 313:		/* #define RPL_WHOISOPERATOR    313 */
		whois_oper (from, ArgList);
		break;

	case 314:		/* #define RPL_WHOWASUSER       314 */
		whowas_name (from, ArgList);
		break;

	case 316:		/* #define RPL_WHOISCHANOP      316 */
		break;

	case 317:		/* #define RPL_WHOISIDLE        317 */
		whois_lastcom (from, ArgList);
		break;

	case 318:		/* #define RPL_ENDOFWHOIS       318 */
		end_of_whois (from, ArgList);
		break;

	case 319:		/* #define RPL_WHOISCHANNELS    319 */
		whois_channels (from, ArgList);
		break;

	case 321:		/* #define RPL_LISTSTART        321 */
		ArgList[0] = "Channel\0Users\0Topic";
		ArgList[1] = ArgList[0] + 8;
		ArgList[2] = ArgList[1] + 6;
		ArgList[3] = NULL;
		funny_list (from, ArgList);
		break;

	case 322:		/* #define RPL_LIST             322 */
		if (is_server_connected (from_server) && strcmp (from, server_list[from_server].itsname))
		{
			say ("322 crash attempt from hacked server %s", from);
			break;
		}
		funny_list (from, ArgList);
		break;

	case 324:		/* #define RPL_CHANNELMODEIS    324 */
		funny_mode (from, ArgList);
		break;

	case 340:		/* #define RPL_INVITING_OTHER   340 */
		{
			if (ArgList[2])
			{
				message_from (ArgList[0], LOG_CRAP);
				if (do_hook (current_numeric, "%s %s %s %s", from, ArgList[0], ArgList[1], ArgList[2]))
					put_it ("%s %s has invited %s to channel %s", numeric_banner (), ArgList[0], ArgList[1], ArgList[2]);
			}
			break;
		}

	case 341:		/* #define RPL_INVITING         341 */
		if (ArgList[1])
		{
			message_from (ArgList[1], LOG_CRAP);
			if (do_hook (current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
				put_it ("%s", convert_output_format (get_format (FORMAT_INVITE_USER_FSET), "%s %s %s", update_clock (GET_TIME), ArgList[0], ArgList[1]));
		}
		break;

	case 352:		/* #define RPL_WHOREPLY         352 */
		whoreply (NULL, ArgList);
		break;

	case 353:		/* #define RPL_NAMREPLY         353 */
		funny_namreply (from, ArgList);
		break;

	case 366:		/* #define RPL_ENDOFNAMES       366 */
		{
			int need_display = 1;
			char *tmp = NULL, *chan;
			PasteArgs (ArgList, 1);
			message_from (ArgList[0], LOG_CURRENT);
			if (get_int_var (SHOW_END_OF_MSGS_VAR))
				need_display = do_hook (current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]);
			message_from (NULL, LOG_CRAP);
			malloc_strcpy (&tmp, ArgList[0]);
			chan = strtok (tmp, " ");

			/* do we really need this check? -laeos */
			if (!in_join_list (chan, from_server))
			{
				PasteArgs (ArgList, 0);
				if (get_int_var (SHOW_END_OF_MSGS_VAR) && need_display)
					display_msg (from, ArgList);
			}
			else
				got_info (chan, from_server, GOTNAMES);
			new_free (&tmp);
		}
		break;

	case 381:		/* #define RPL_YOUREOPER        381 */
		PasteArgs (ArgList, 0);
		if (!is_server_connected (from_server))
			say ("Odd Server stuff from %s: %s", from, ArgList[0]);
		else if (do_hook (current_numeric, "%s %s", from, *ArgList))
		{
			if (get_format (FORMAT_381_FSET))
				put_it ("%s",
					convert_output_format (
					     get_format (FORMAT_381_FSET),
								 "%s %s %s",
				  update_clock (GET_TIME), from, *ArgList));
			else
				display_msg (from, ArgList);
		}
		if (get_string_var (OPER_MODES_VAR))
			send_to_server ("MODE %s %s", get_server_nickname (from_server), get_string_var (OPER_MODES_VAR));
		break;

	case 401:		/* #define ERR_NOSUCHNICK       401 */
		if (ArgList[0] && !check_dcc_list (ArgList[0]))
			no_such_nickname (from, ArgList);
		break;

	case 421:		/* #define ERR_UNKNOWNCOMMAND   421 */
		PasteArgs (ArgList, 0);

		if ((flag = do_hook (current_numeric, "%s %s", from, *ArgList)))
			display_msg (from, ArgList);
		break;

	case 432:		/* #define ERR_ERRONEUSNICKNAME 432 */
		PasteArgs (ArgList, 0);
		if (do_hook (current_numeric, "%s %s", from, *ArgList))
			display_msg (from, ArgList);
		fudge_nickname (from_server);
		break;
	case 433:		/* #define ERR_NICKNAMEINUSE    433 */
		if (is_server_connected (from_server) && server_list[from_server].itsname && strcmp (from, server_list[from_server].itsname))
		{
			say ("433 nicknameinuse attempt from hacked server %s", from);
			break;
		}
		fudge_nickname (from_server);
		if (server_list[from_server].itsname)
			add_to_whois_queue (ArgList[0], whohas_nick, "%s", ArgList[0]);
		PasteArgs (ArgList, 0);
		if (!never_connected && do_hook (current_numeric, "%s", *ArgList))
			display_msg (from, ArgList);
		break;
	case 451:		/* #define ERR_NOTREGISTERED    451 */
		/*
		 * Sometimes the server doesn't catch the USER line, so
		 * here we send a simplified version again  -lynx 
		 */
		register_server (from_server, NULL);

		PasteArgs (ArgList, 0);
		if (do_hook (current_numeric, "%s %s", from, *ArgList))
			display_msg (from, ArgList);
		break;

	case 462:		/* #define ERR_ALREADYREGISTRED 462 */
		change_server_nickname (from_server, NULL);
		PasteArgs (ArgList, 0);
		if (do_hook (current_numeric, "%s %s", from, *ArgList))
			display_msg (from, ArgList);
		break;

	case 463:		/* CDE added for ERR_NOPERMFORHOST */
		{
			PasteArgs (ArgList, 0);
			if (do_hook (current_numeric, "%s %s", from, *ArgList))
				display_msg (from, ArgList);
			close_server (from_server, empty_str);
			window_check_servers ();
			if (from_server == primary_server)
				get_connected (from_server + 1);
			break;
		}
	case 464:		/* #define ERR_PASSWDMISMATCH   464 */
		if (is_server_connected (from_server) && server_list[from_server].itsname && strcmp (from, server_list[from_server].itsname))
		{
			say ("464 crash attempt from hacked server %s", from);
			break;
		}
		PasteArgs (ArgList, 0);
		if (!is_server_connected (from_server))
		{
			say ("Odd Server stuff from %s: %s", from, ArgList[0]);
			return;
		}
		flag = do_hook (current_numeric, "%s %s", from, ArgList[0]);
		if (oper_command)
		{
			if (flag)
				display_msg (from, ArgList);
			oper_command = 0;
		}
		else
		{
			char server_num[8];

			say ("Password required for connection to server %s",
			     get_server_name (from_server));
			close_server (from_server, empty_str);
			strcpy (server_num, ltoa (from_server));
			add_wait_prompt ("Server Password:", password_sendline,
					 server_num, WAIT_PROMPT_LINE);
		}
		break;

	case 465:		/* #define ERR_YOUREBANNEDCREEP 465 */
		{
			int klined = from_server;

			if (is_server_connected (from_server) && server_list[from_server].itsname && strcmp (from, server_list[from_server].itsname))
			{
				say ("465 crash attempt from hacked server %s", from);
				break;
			}
			PasteArgs (ArgList, 0);
			if (!is_server_connected (from_server))
				return;
			if (do_hook (current_numeric, "%s %s", from, ArgList[0]))
				display_msg (from, ArgList);

			close_server (from_server, empty_str);
			window_check_servers ();
			if (server_list_size () > 1)
				remove_from_server_list (klined);
			if (klined == primary_server && (server_list_size () > 0))
				get_connected (klined);
			break;
		}

	case 484:
		{
			if (do_hook (current_numeric, "%s %s", from, ArgList[0]))
				display_msg (from, ArgList);
			set_server_flag (from_server, USER_MODE_R, 1);
			break;
		}
	case 367:
		number_of_bans++;
		if (check_sync (comm, ArgList[0], ArgList[1], ArgList[2], ArgList[3], NULL) == 0)
			break;
		if (ArgList[2])
		{
			time_t tme = (time_t) strtoul (ArgList[3], NULL, 10);
			if (do_hook (current_numeric, "%s %s %s %s %s",
				     from, ArgList[0], ArgList[1], ArgList[2], ArgList[3]))
				put_it ("%s", convert_output_format (get_format (FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_bans, ArgList[0], ArgList[1], ArgList[2], tme));
		}
		else if (do_hook (current_numeric, "%s %s %s", from, ArgList[0], ArgList[1]))
			put_it ("%s", convert_output_format (get_format (FORMAT_BANS_FSET), "%d %s %s %s %l", number_of_bans, ArgList[0], ArgList[1], "unknown", time (NULL)));
		break;
	case 368:		/* #define RPL_ENDOFBANLIST     368 */
		{
			if (!got_info (ArgList[0], from_server, GOTBANS))
				break;

			if (get_int_var (SHOW_END_OF_MSGS_VAR))
			{
				if (do_hook (current_numeric, "%s %d %s", from, number_of_bans, *ArgList))
				{
					put_it ("%s Total Number of Bans on %s [%d]",
					      numeric_banner (), ArgList[0],
						number_of_bans);
				}
			}
			number_of_bans = 0;
			break;
		}
	case 364:
#if 0
		if (is_server_connected (from_server) && server_list[from_server].itsname && strcmp (from, server_list[from_server].itsname))
		{
			say ("364 links attempt from hacked server %s", from);
			break;
		}
#endif
		if (comm == 364)
		{
			if (get_int_var (LLOOK_VAR) && (server_list[from_server].link_look == 1))
			{
				parse_364 (ArgList[0], ArgList[1], ArgList[2] ? ArgList[2] : "0");
				break;
			}
			else if (server_list[from_server].link_look == 2)
			{
				add_to_irc_map (ArgList[0], ArgList[2]);
				break;
			}
			if ((do_hook (current_numeric, "%s %s %s", ArgList[0], ArgList[1], ArgList[2])))
			{
				if (get_format (FORMAT_LINKS_FSET))
				{
					put_it ("%s", convert_output_format (get_format (FORMAT_LINKS_FSET), "%s %s %s", ArgList[0], ArgList[1], ArgList[2] ? ArgList[2] : "0"));
					break;
				}
			}
		}
	case 365:		/* #define RPL_ENDOFLINKS       365 */
		if (comm == 365)
		{
			if (get_int_var (LLOOK_VAR) && (server_list[from_server].link_look == 1))
			{
				parse_365 (ArgList[0], ArgList[1], NULL);
				server_list[from_server].link_look = 0;
				break;
			}
			else if (server_list[from_server].link_look == 2)
			{
				show_server_map ();
				server_list[from_server].link_look = 0;
				break;
			}
		}
	case 404:
	case 442:
	case 482:
		if (comm == 404 || comm == 482 || comm == 442)
		{
			if (check_server_sync (from, ArgList))
			{
				send_to_server ("WHO -server %s %s", from, ArgList[0]);
				break;
			}
		}
	case 204:
		if (server_list[from_server].trace_flags & TRACE_OPER)
		{
			trace_oper (from, ArgList);
			break;
		}
		else if (server_list[from_server].trace_flags)
			break;
	case 203:
	case 205:
		if (server_list[from_server].trace_flags & TRACE_USER)
		{
			trace_user (from, ArgList);
			break;
		}
		else if (server_list[from_server].trace_flags)
			break;
	case 206:
		if (server_list[from_server].trace_flags & TRACE_SERVER)
		{
			trace_server (from, ArgList);
			break;
		}
		else if (server_list[from_server].trace_flags)
			break;
	case 200:
	case 207:
	case 209:
		if (server_list[from_server].trace_flags)
			break;

	case 262:
	case 402:
		if (comm == 402 && handle_server_ping (comm, from, ArgList))
			break;
		if ((comm == 262 || comm == 402) && server_list[from_server].trace_flags)
		{
			server_list[from_server].trace_flags = 0;
			break;
		}
		/*
		 * The following accumulates the remaining arguments
		 * in ArgSpace for hook detection. We can't use
		 * PasteArgs here because we still need the arguments
		 * separated for use elsewhere.
		 */
	default:
		{
			char *ArgSpace = NULL;
			int i, len, do_message_from = 0;


			for (i = len = 0; ArgList[i]; len += strlen (ArgList[i++]))
				;
			len += (i - 1);
			ArgSpace = new_malloc (len + 1);
			ArgSpace[0] = '\0';
			/* this is cheating */

			if (ArgList[0] && is_channel (ArgList[0]))
				do_message_from = 1;
			for (i = 0; ArgList[i]; i++)
			{
				if (i)
					strcat (ArgSpace, " ");
				strcat (ArgSpace, ArgList[i]);
			}
			if (do_message_from)
				message_from (ArgList[0], LOG_CRAP);
			if (!do_hook (current_numeric, "%s %s", from, ArgSpace))
			{
				new_free (&ArgSpace);
				if (do_message_from)
					message_from (NULL, lastlog_level);
				return;
			}
			if (do_message_from)
				message_from (NULL, lastlog_level);
			new_free (&ArgSpace);
			none_of_these = 1;
		}
	}
	/* the following do not hurt the ircII if intercepted by a hook */
	if (none_of_these)
	{
		switch (comm)
		{
		case 211:
		case 212:
		case 213:
		case 214:
		case 215:
		case 216:
		case 218:
		case 241:
		case 242:
		case 243:
		case 244:
			if (server_list[from_server].stats_flags && ((comm > 210 && comm < 219) || (comm > 240 && comm < 245)))
			{
				handle_stats (comm, from, ArgList);
				break;
			}
			else if (!server_list[from_server].stats_flags && comm == 242)
			{
/*              case 242:                #define RPL_STATSUPTIME      242 */
				PasteArgs (ArgList, 0);
				if (from && !my_strnicmp (get_server_itsname (from_server),
							  from, strlen (get_server_itsname (from_server))))
					from = NULL;
				if (from)
					put_it ("%s %s from (%s)", numeric_banner (),
						*ArgList, from);
				else
					put_it ("%s %s", numeric_banner (), *ArgList);
				break;
			}
			else
			{
				PasteArgs (ArgList, 0);
				put_it ("%s %s %s", numeric_banner (), from, *ArgList);
			}
			break;
		case 219:	/* #define RPL_ENDOFSTATS       219 */
#if 0
			if (!server_list[from_server].stats_flags && comm == 219)
				stats_k_grep_end ();
#endif
			if (server_list[from_server].stats_flags && comm == 219)
				server_list[from_server].stats_flags = 0;
			break;
		case 221:	/* #define RPL_UMODEIS          221 */
			put_it ("%s Your user mode is [%s]", numeric_banner (), ArgList ? ArgList[0] : " ");
			break;

		case 272:	/* ENDOFSILENCE */
			PasteArgs (ArgList, 0);
			put_it ("%s", convert_output_format (get_format (FORMAT_SILENCE_FSET), update_clock (GET_TIME), ArgList[0]));
			break;

		case 329:	/* #define CREATION_TIME        329 */
			{
				time_t tme;
				char *this_sucks;
				ChannelList *chan = NULL;

				if (!ArgList[1] || !*ArgList[1])
					break;
				sscanf (ArgList[1], "%lu", &tme);
				this_sucks = ctime (&tme);
				this_sucks[strlen (this_sucks) - 1] = '\0';

				message_from (ArgList[0], LOG_CRAP);
				put_it ("%s Channel %s was created at %s", numeric_banner (),
					ArgList[0], this_sucks);
				if ((chan = lookup_channel (ArgList[0], from_server, 0)))
					chan->channel_create.tv_sec = tme;
				message_from (NULL, LOG_CURRENT);
				break;
			}
		case 332:	/* #define RPL_TOPIC            332 */
			channel_topic (from, ArgList, 332);
			break;
		case 333:
			channel_topic (from, ArgList, 333);
			break;

		case 351:	/* #define RPL_VERSION          351 */
			if (!handle_server_ping (comm, from, ArgList))
			{
				PasteArgs (ArgList, 2);
				put_it ("%s", convert_output_format (get_format (FORMAT_SERVER_FSET), "Server %s %s %s", ArgList[0], ArgList[1], ArgList[2]));
			}
			break;

		case 364:	/* #define RPL_LINKS            364 */
			{

				if (ArgList[2])
				{
					PasteArgs (ArgList, 2);
					put_it ("%s %-20s %-20s %s", numeric_banner (),
					ArgList[0], ArgList[1], ArgList[2]);
				}
				else
				{
					PasteArgs (ArgList, 1);
					put_it ("%s %-20s %s", numeric_banner (),
						ArgList[0], ArgList[1]);
				}
			}
			break;
		case 377:
		case 372:	/* #define RPL_MOTD             372 */
			if (!get_int_var (SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd (from_server))
			{
				PasteArgs (ArgList, 0);
				put_it ("%s %s", numeric_banner (), ArgList[0]);
			}
			break;

		case 375:	/* #define RPL_MOTDSTART        375 */
			if (!get_int_var (SUPPRESS_SERVER_MOTD_VAR) ||
			    !get_server_motd (from_server))
			{
				PasteArgs (ArgList, 0);
				put_it ("%s %s", numeric_banner (), ArgList[0]);
			}
			break;

		case 376:	/* #define RPL_ENDOFMOTD        376 */
			if (get_int_var (SHOW_END_OF_MSGS_VAR) &&
			    (!get_int_var (SUPPRESS_SERVER_MOTD_VAR) ||
			     !get_server_motd (from_server)))
			{
				PasteArgs (ArgList, 0);
				put_it ("%s %s", numeric_banner (), ArgList[0]);
			}
			set_server_motd (from_server, 0);
			break;

		case 384:	/* #define RPL_MYPORTIS         384 */
			PasteArgs (ArgList, 0);
			put_it ("%s %s %s", numeric_banner (), ArgList[0], user);
			break;

		case 385:	/* #define RPL_NOTOPERANYMORE   385 */
			set_server_operator (from_server, 0);
			display_msg (from, ArgList);
			update_all_status (curr_scr_win, NULL, 0);
			break;

		case 403:	/* #define ERR_NOSUCHCHANNEL    403 */
			not_valid_channel (from, ArgList);
			break;

		case 432:	/* #define ERR_ERRONEUSNICKNAME 432 */
			display_msg (from, ArgList);
			reset_nickname ();
			break;

		case 471:	/* #define ERR_CHANNELISFULL    471 */
		case 473:	/* #define ERR_INVITEONLYCHAN   473 */
		case 474:	/* #define ERR_BANNEDFROMCHAN   474 */
		case 475:	/* #define ERR_BADCHANNELKEY    475 */
		case 476:	/* #define ERR_BADCHANMASK      476 */
			cannot_join_channel (from, ArgList);
			break;

#define RPL_CLOSEEND         363
#define RPL_SERVLISTEND      235
		case 315:	/* #define RPL_ENDOFWHO         315 */
			{
				if (doing_who)
					doing_who--;
				if (get_int_var (SHOW_END_OF_MSGS_VAR))
					if (do_hook (current_numeric, "%s %s", from, ArgList[0]))
						put_it ("%s %s %s", numeric_banner (), from, ArgList[0]);
			}
		case 316:
			break;

		case 323:	/* #define RPL_LISTEND          323 */
			funny_print_widelist ();

		case 232:	/* #define RPL_ENDOFSERVICES    232 */
		case 365:	/* #define RPL_ENDOFLINKS       365 */
			if (comm == 365 && get_int_var (LLOOK_VAR) && server_list[from_server].link_look)
			{
				parse_365 (ArgList[0], ArgList[1], NULL);
				server_list[from_server].link_look--;
				break;
			}
		case 369:	/* #define RPL_ENDOFWHOWAS      369 */
		case 374:	/* #define RPL_ENDOFINFO        374 */
		case 394:	/* #define RPL_ENDOFUSERS       394 */
			if (get_int_var (SHOW_END_OF_MSGS_VAR))
			{
				PasteArgs (ArgList, 0);
				if (do_hook (current_numeric, "%s %s", from, *ArgList))
					display_msg (from, ArgList);
			}
			break;
		default:
			display_msg (from, ArgList);
		}
	}
	set_lastlog_msg_level (lastlog_level);
}
