/*
 * hook.c: Does those naughty hook functions. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"

#include "hook.h"
#include "vars.h"
#include "ircaux.h"
#include "alias.h"
#include "list.h"
#include "window.h"
#include "server.h"
#include "output.h"
#include "commands.h"
#include "parse.h"
#include "fset.h"
#include "misc.h"
#include <stdarg.h>

#define SILENT	0
#define QUIET	1
#define NORMAL	2
#define NOISY	3

/*
 * The various ON levels: SILENT means the DISPLAY will be OFF and it will
 * suppress the default action of the event, QUIET means the display will be
 * OFF but the default action will still take place, NORMAL means you will be
 * notified when an action takes place and the default action still occurs,
 * NOISY means you are notified when an action occur plus you see the action
 * in the display and the default actions still occurs 
 */
static char *noise_level[] =
{"SILENT", "QUIET", "NORMAL", "NOISY"};

#define	HS_NOGENERIC	0x1000
#define HF_NORECURSE	0x0002
#define HF_GLOBAL	0x0004

int in_on_who = 0;

NumericList *numeric_list = NULL;
/* hook_functions: the list of all hook functions available */
HookFunc hook_functions[] =
{
	{"ACTION", NULL, 3, 0, 0},
	{"AR_PUBLIC", NULL, 3, 0, 0},
	{"AR_PUBLIC_OTHER", NULL, 3, 0, 0},
	{"AR_REPLY", NULL, 1, 0, 0},
	{"BANS", NULL, 4, 0, 0},
	{"BANS_HEADER", NULL, 4, 0, 0},

	{"CHANNEL_NICK", NULL, 3, 0, 0},
	{"CHANNEL_SIGNOFF", NULL, 3, 0, 0},
	{"CHANNEL_STATS", NULL, 32, 0, 0},
	{"CHANNEL_SWITCH", NULL, 1, 0, 0},
	{"CHANNEL_SYNCH", NULL, 2, 0, 0},
	{"CLONE_READ", NULL, 4, 0, 0},
	{"CONNECT", NULL, 1, 0, 0},
	{"CTCP", NULL, 4, 0, 0},
	{"CTCP_REPLY", NULL, 3, 0, 0},
	{"DCC_CHAT", NULL, 2, 0, 0},
	{"DCC_CONNECT", NULL, 2, 0, 0},
	{"DCC_ERROR", NULL, 6, 0, 0},
	{"DCC_HEADER", NULL, 7, 0, 0},
	{"DCC_LOST", NULL, 2, 0, 0},
	{"DCC_POST", NULL, 7, 0, 0},
	{"DCC_RAW", NULL, 3, 0, 0},
	{"DCC_REQUEST", NULL, 4, 0, 0},
	{"DCC_STAT", NULL, 7, 0, 0},
	{"DCC_STATF", NULL, 7, 0, 0},
	{"DCC_STATF1", NULL, 5, 0, 0},
	{"DESYNC_MESSAGE", NULL, 2, 0, 0},
	{"DISCONNECT", NULL, 1, 0, 0},
	{"ENCRYPTED_NOTICE", NULL, 3, 0, 0},
	{"ENCRYPTED_PRIVMSG", NULL, 3, 0, 0},
	{"EXEC", NULL, 2, 0, 0},
	{"EXEC_ERRORS", NULL, 2, 0, 0},
	{"EXEC_EXIT", NULL, 3, 0, 0},
	{"EXEC_PROMPT", NULL, 2, 0, 0},
	{"EXIT", NULL, 1, 0, 0},
	{"FLOOD", NULL, 3, 0, 0},
	{"HELP", NULL, 2, 0, 0},
	{"HELPSUBJECT", NULL, 2, 0, 0},
	{"HELPTOPIC", NULL, 1, 0, 0},
	{"HOOK", NULL, 1, 0, 0},
	{"IDLE", NULL, 1, 0, 0},
	{"INPUT", NULL, 1, 0, 0},
	{"INVITE", NULL, 2, 0, 0},
	{"JOIN", NULL, 3, 0, 0},
	{"JOIN_ME", NULL, 1, 0, 0},
	{"KICK", NULL, 3, 0, 0},
	{"LEAVE", NULL, 2, 0, 0},
	{"LIST", NULL, 3, 0, 0},
	{"LLOOK_ADDED", NULL, 2, 0, 0},
	{"LLOOK_JOIN", NULL, 2, 0, 0},
	{"LLOOK_SPLIT", NULL, 2, 0, 0},
	{"MODE", NULL, 3, 0, 0},
	{"MODE_STRIPPED", NULL, 3, 0, 0},
	{"MSG", NULL, 2, 0, 0},
	{"MSG_GROUP", NULL, 3, 0, 0},
	{"MSGLOG", NULL, 4, 0, 0},
	{"NAMES", NULL, 2, 0, 0},
	{"NICKNAME", NULL, 2, 0, 0},
	{"NOTE", NULL, 3, 0, 0},
	{"NOTICE", NULL, 2, 0, 0},
	{"NOTIFY_SIGNOFF", NULL, 1, 0, 0},
	{"NOTIFY_SIGNOFF_UH", NULL, 3, 0, 0},
	{"NOTIFY_SIGNON", NULL, 1, 0, 0},
	{"NOTIFY_SIGNON_UH", NULL, 3, 0, 0},
	{"NSLOOKUP", NULL, 3, 0, 0},
	{"ODD_SERVER_STUFF", NULL, 3, 0, 0},
	{"PUBLIC", NULL, 3, 0, 0},
	{"PUBLIC_MSG", NULL, 3, 0, 0},
	{"PUBLIC_NOTICE", NULL, 3, 0, 0},
	{"PUBLIC_OTHER", NULL, 3, 0, 0},
	{"RAW_IRC", NULL, 1, 0, 0},
	{"SAVEFILE", NULL, 2, 0, 0},
	{"SAVEFILEPOST", NULL, 2, 0, 0},
	{"SAVEFILEPRE", NULL, 2, 0, 0},
	{"SEND_ACTION", NULL, 2, 0, HF_NORECURSE},
	{"SEND_DCC_CHAT", NULL, 2, 0, HF_NORECURSE},
	{"SEND_MSG", NULL, 2, 0, HF_NORECURSE},
	{"SEND_NOTICE", NULL, 2, 0, HF_NORECURSE},
	{"SEND_PUBLIC", NULL, 2, 0, HF_NORECURSE},
	{"SEND_TO_SERVER", NULL, 3, 0, 0},
	{"SERVER_NOTICE_FAKES", NULL, 3, 0, 0},
	{"SERVER_NOTICE_FAKES_MYCHANNEL", NULL, 3, 0, 0},
	{"SERVER_NOTICE_FOREIGN_KILL", NULL, 4, 0, 0},
	{"SERVER_NOTICE_KILL", NULL, 4, 0, 0},
	{"SERVER_NOTICE", NULL, 1, 0, 0},
	{"SERVER_NOTICE_LOCAL_KILL", NULL, 4, 0, 0},
	{"SERVER_NOTICE_SERVER_KILL", NULL, 4, 0, 0},
	{"SET", NULL, 2, 0, 0},
	{"SHOWIDLE_HEADER", NULL, 2, 0, 0},
	{"SHOWIDLE", NULL, 4, 0, 0},
	{"SIGNOFF", NULL, 1, 0, 0},
	{"SILENCE", NULL, 2, 0, 0},
	{"STAT", NULL, 5, 0, 0},
	{"STAT_HEADER", NULL, 5, 0, 0},
	{"STATUS_UPDATE", NULL, 2, 0, 0},
	{"TIMER", NULL, 1, 0, 0},
	{"TOPIC", NULL, 2, 0, 0},
	{"USAGE", NULL, 2, 0, 0},
	{"USERS", NULL, 7, 0, 0},
	{"USERS_HEADER", NULL, 7, 0, 0},
	{"USERS_SERVER", NULL, 2, 0, 0},
	{"USERS_SERVER_HEADER", NULL, 2, 0, 0},
	{"WALL", NULL, 2, 0, 0},
	{"WALLOP", NULL, 3, 0, 0},
	{"WHO", NULL, 6, 0, 0},
	{"WHOLEFT", NULL, 6, 0, 0},
	{"WHOLEFT_HEADER", NULL, 6, 0, 0},
	{"WIDELIST", NULL, 1, 0, 0},
	{"WINDOW", NULL, 2, 0, HF_NORECURSE},
	{"WINDOW_KILL", NULL, 1, 0, 0},
	{"YELL", NULL, 1, 0, 0}
};

static char *
fill_it_out (char *str, int params)
{
	char buffer[BIG_BUFFER_SIZE + 1];
	char *arg, *free_ptr = NULL, *ptr;
	int i = 0;

	malloc_strcpy (&free_ptr, str);
	ptr = free_ptr;
	*buffer = (char) 0;

	while ((arg = next_arg (ptr, &ptr)) != NULL)
	{
		if (*buffer)
			strmcat (buffer, " ", BIG_BUFFER_SIZE);
		strmcat (buffer, arg, BIG_BUFFER_SIZE);
		if (++i == params)
			break;
	}

	for (; i < params; i++)
		strmcat (buffer, (i < params - 1) ? " %" : " *", BIG_BUFFER_SIZE);

	if (ptr && *ptr)
	{
		strmcat (buffer, " ", BIG_BUFFER_SIZE);
		strmcat (buffer, ptr, BIG_BUFFER_SIZE);
	}
	malloc_strcpy (&free_ptr, buffer);
	return (free_ptr);
}


/*
 * This crap here is used so we can use the list manip stuff.  Maybe 
 * we should fix the problem instead of using nasty hacks like this.
 */

struct CmpInfoStruc
{
	int ServerRequired;
	int SkipSerialNum;
	int SerialNumber;
	int Flags;
}
cmp_info;

#define	CIF_NOSERNUM	0x0001
#define	CIF_SKIP	0x0002

int cmpinfodone = 0;

/*
 * setup_struct and Add_Remove_Check are used by the list manipulation
 * functions for adding and removing hooks from the hook list.
 */
static void 
setup_struct (int ServReq, int SkipSer, int SerNum, int flags)
{
	cmp_info.ServerRequired = ServReq;
	cmp_info.SkipSerialNum = SkipSer;
	cmp_info.SerialNumber = SerNum;
	cmp_info.Flags = flags;
}

static int 
Add_Remove_Check (Hook * Item, char *Name)
{
	int comp;

	if (cmp_info.SerialNumber != Item->sernum)
		return (Item->sernum > cmp_info.SerialNumber) ? 1 : -1;
	if ((comp = my_stricmp (Item->nick, Name)) != 0)
		return comp;
	if (Item->server != cmp_info.ServerRequired)
		return (Item->server > cmp_info.ServerRequired) ? 1 : -1;
	return 0;
}


static void 
add_numeric_hook (int numeric, char *nick, char *stuff, int noisy, int not, int server, int sernum, int flexible)
{
	NumericList *entry;
	Hook *new;
	char buf[4];

	sprintf (buf, "%3.3u", numeric);
	if ((entry = (NumericList *) find_in_list ((List **) & numeric_list, buf, 0)) ==
	    NULL)
	{
		entry = (NumericList *) new_malloc (sizeof (NumericList));
		memset (entry, 0, sizeof (NumericList));
		malloc_strcpy (&(entry->name), buf);
		add_to_list ((List **) & numeric_list, (List *) entry);
	}

	setup_struct ((server == -1) ? -1 : (server & ~HS_NOGENERIC), sernum - 1, sernum, 0);
	if ((new = (Hook *) remove_from_list_ext ((List **) & (entry->list), nick, (int (*)(List *, char *)) Add_Remove_Check)) != NULL)
	{
		new->not = 1;
		new_free (&(new->nick));
		new_free (&(new->stuff));
		new_free ((char **) &new);
	}
	new = (Hook *) new_malloc (sizeof (Hook));
	memset (new, 0, sizeof (Hook));
	new->noisy = noisy;
	new->server = server;
	new->sernum = sernum;
	new->not = not;
	new->flexible = flexible;
	malloc_strcpy (&new->nick, nick);
	malloc_strcpy (&new->stuff, stuff);
	upper (new->nick);
	add_to_list_ext ((List **) & (entry->list), (List *) new, (int (*)(List *, List *)) Add_Remove_Check);
}

/*
 * add_hook Given an index into the hook_functions array, this adds a new
 * entry to the list as specified by the rest of the parameters.  The new
 * entry is added in alphabetical order (by nick). 
 */
static void 
add_hook (int which, char *nick, char *stuff, int noisy, int not, int server, int sernum, int flexible)
{
	Hook *new;

	if (which < 0)
	{
		add_numeric_hook (-which, nick, stuff, noisy, not, server,
				  sernum, flexible);
		return;
	}
	setup_struct ((server == -1) ? -1 : (server & ~HS_NOGENERIC), sernum - 1, sernum, 0);
	if ((new = (Hook *) remove_from_list_ext ((List **) & (hook_functions[which].list), nick, (int (*)(List *, char *)) Add_Remove_Check)) != NULL)
	{
		new->not = 1;
		new_free (&(new->nick));
		new_free (&(new->stuff));
		new_free ((char **) &new);
	}
	new = (Hook *) new_malloc (sizeof (Hook));
	memset (new, 0, sizeof (Hook));
	new->noisy = noisy;
	new->server = server;
	new->sernum = sernum;
	new->not = not;
	new->flexible = flexible;
	malloc_strcpy (&new->nick, nick);
	malloc_strcpy (&new->stuff, stuff);
	upper (new->nick);
	add_to_list_ext ((List **) & (hook_functions[which].list), (List *) new, (int (*)(List *, List *)) Add_Remove_Check);
}

/* show_hook shows a single hook */
extern void 
show_hook (Hook * list, char *name)
{
	char *hooks = get_fset_var (FORMAT_HOOK_FSET);
	char *text = NULL;
	if (list->stuff)
		text = stripansi (list->stuff);
	if (list->server != -1)
	{
		if (hooks)
			put_it ("%s", convert_output_format (hooks, "%s %c %s %c %s %s %d Server %d %s",
			    name, (list->flexible ? '\'' : '"'), list->nick,
					      (list->flexible ? '\'' : '"'),
					     (list->not ? "nothing" : text),
				     noise_level[list->noisy], list->sernum,
					       list->server & ~HS_NOGENERIC,
							     (list->server & HS_NOGENERIC) ? " Exclusive" : empty_string));
		else
			say ("On %s from %c%s%c do %s [%s] <%d> (Server %d)%s",
			     name, (list->flexible ? '\'' : '"'), list->nick,
			     (list->flexible ? '\'' : '"'),
			     (list->not ? "nothing" : text),
			     noise_level[list->noisy], list->sernum,
			     list->server & ~HS_NOGENERIC,
			     (list->server & HS_NOGENERIC) ? " Exclusive" : empty_string);
	}
	else
	{
		if (hooks)
			put_it ("%s", convert_output_format (hooks, "%s %c %s %c %s %s %d",
			    name, (list->flexible ? '\'' : '"'), list->nick,
					      (list->flexible ? '\'' : '"'),
					     (list->not ? "nothing" : text),
						   noise_level[list->noisy],
							     list->sernum));
		else
			say ("On %s from %c%s%c do %s [%s] <%d>",
			     name, (list->flexible ? '\'' : '"'), list->nick,
			     (list->flexible ? '\'' : '"'),
			     (list->not ? "nothing" : text),
			     noise_level[list->noisy],
			     list->sernum);
	}
	new_free (&text);
}

/*
 * show_numeric_list: If numeric is 0, then all numeric lists are displayed.
 * If numeric is non-zero, then that particular list is displayed.  The total
 * number of entries displayed is returned 
 */
static int 
show_numeric_list (int numeric)
{
	NumericList *tmp;
	Hook *list;
	char buf[4];
	int cnt = 0;

	if (numeric)
	{
		sprintf (buf, "%3.3u", numeric);
		if ((tmp = (NumericList *) find_in_list ((List **) & numeric_list, buf, 0))
		    != NULL)
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook (list, tmp->name);
		}
	}
	else
	{
		for (tmp = numeric_list; tmp; tmp = tmp->next)
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook (list, tmp->name);
		}
	}
	return (cnt);
}

/*
 * show_list: Displays the contents of the list specified by the index into
 * the hook_functions array.  This function returns the number of entries in
 * the list displayed 
 */
static int 
show_list (int which)
{
	Hook *list;
	int cnt = 0;

	/* Less garbage when issueing /on without args. (lynx) */
	for (list = hook_functions[which].list; list; list = list->next, cnt++)
		show_hook (list, hook_functions[which].name);
	return (cnt);
}

/*
 * do_hook: This is what gets called whenever a MSG, INVITES, WALL, (you get
 * the idea) occurs.  The nick is looked up in the appropriate list. If a
 * match is found, the stuff field from that entry in the list is treated as
 * if it were a command. First it gets expanded as though it were an alias
 * (with the args parameter used as the arguments to the alias).  After it
 * gets expanded, it gets parsed as a command.  This will return as its value
 * the value of the noisy field of the found entry, or -1 if not found. 
 */
/* huh-huh.. this sucks.. im going to re-write it so that it works */
extern int 
do_hook (int which, char *format,...)
{
	Hook *tmp, **list;
	char buffer[BIG_BUFFER_SIZE * 10 + 1], *name = NULL;
	int RetVal = 1;
	unsigned int display;
	int i, old_in_on_who;
	Hook *hook_array[2048];
	int hook_num = 0;
	static int hook_level = 0;

	hook_level++;
	*buffer = 0;

	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf (buffer, BIG_BUFFER_SIZE * 10, format, args);
		va_end (args);
	}
	if (which < 0)
	{
		NumericList *hook;
		char foo[10];

		sprintf (foo, "%3.3u", -which);
		if ((hook = (NumericList *) find_in_list ((List **) & numeric_list, foo, 0)) != NULL)
		{
			name = hook->name;
			list = &hook->list;
		}
		else
			list = NULL;
	}
	else
	{
		if (hook_functions[which].mark && (hook_functions[which].flags & HF_NORECURSE))
			list = NULL;
		else
		{
			list = &(hook_functions[which].list);
			name = hook_functions[which].name;
		}
	}
	if (!list)
		return 1;

	if (which >= 0)
		hook_functions[which].mark++;

	/* not attached, so dont "fix" it */
	{
		int currser = 0, oldser = 0;
		int currmatch = 0, oldmatch = 0;
		Hook *bestmatch = NULL;
		int nomorethisserial = 0;

		for (tmp = *list; tmp; tmp = tmp->next)
		{
			char *tmpnick = NULL;
			int sa;

			currser = tmp->sernum;
			if (currser != oldser)	/* new serial number */
			{
				oldser = currser;
				currmatch = oldmatch = nomorethisserial = 0;
				if (bestmatch)
					hook_array[hook_num++] = bestmatch;
				bestmatch = NULL;
			}

			if (nomorethisserial)
				continue;
			/* if there is a specific server
			   hook and it doesnt match, then
			   we make sure nothing from
			   this serial number gets hooked */

			if ((tmp->server != -1) &&
			    (tmp->server & HS_NOGENERIC) &&
			    (tmp->server != (from_server & HS_NOGENERIC)))
			{
				nomorethisserial = 1;
				bestmatch = NULL;
				continue;
			}

			if (tmp->flexible)
				tmpnick = expand_alias (tmp->nick, empty_string, &sa, NULL);
			else
				tmpnick = tmp->nick;

			currmatch = wild_match (tmpnick, buffer);
			if (currmatch > oldmatch)
			{
				oldmatch = currmatch;
				bestmatch = tmp;
			}
			if (tmp->flexible)
				new_free (&tmpnick);
		}
		if (bestmatch)
			hook_array[hook_num++] = bestmatch;
	}

	for (i = 0; i < hook_num; i++)
	{
		char *foo, *saved_who_from = NULL;
		int saved_who_level;

		if (!(tmp = hook_array[i]))
		{
			if (which >= 0)
				hook_functions[which].mark--;
			return RetVal;
		}
		if (tmp->not)
			continue;

		current_on_hook = which;
		if (tmp->noisy > QUIET)
			say ("%s activated by %c%s%c", name, (tmp->flexible ? '\'' : '"'), buffer, (tmp->flexible ? '\'' : '"'));
		display = window_display;
		if (tmp->noisy < NOISY)
			window_display = 0;

		save_message_from (&saved_who_from, &saved_who_level);
		old_in_on_who = in_on_who;

#ifdef ENFORCE_STRICTER_PROTOCOL
		if (which == WHO_LIST || (which <= -311 && which >= -318))
			in_on_who = 1;
#else
		in_on_who = 0;
#endif
		if (!tmp->noisy && !tmp->sernum)
			RetVal = 0;
		if (tmp->stuff && *tmp->stuff)
		{
			char *name2 = alloca (strlen (name) + 1);
			strcpy (name2, name);

			foo = alloca (strlen (tmp->stuff) + 1);
			strcpy (foo, tmp->stuff);
			parse_line (name2, foo, buffer, 0, 0);
		}
		in_on_who = old_in_on_who;
		window_display = display;
		current_on_hook = -1;
		restore_message_from (saved_who_from, saved_who_level);
	}
	if (which >= 0)
		hook_functions[which].mark--;

	return RetVal;
}

static void 
remove_numeric_hook (int numeric, char *nick, int server, int sernum, int quiet)
{
	NumericList *hook;
	Hook *tmp, *next;
	char buf[5];

	sprintf (buf, "%3.3u", numeric);
	if ((hook = (NumericList *) find_in_list ((List **) & numeric_list, buf, 0)) != NULL)
	{
		if (nick)
		{
			setup_struct ((server == -1) ? -1 : (server & ~HS_NOGENERIC), sernum - 1, sernum, 0);
			if ((tmp = (Hook *) remove_from_list ((List **) & (hook->list), nick)) != NULL)
			{
				if (!quiet)
					say ("%c%s%c removed from %s list",
					 (tmp->flexible ? '\'' : '"'), nick,
					 (tmp->flexible ? '\'' : '"'), buf);
				tmp->not = 1;
				new_free (&(tmp->nick));
				new_free (&(tmp->stuff));
				new_free ((char **) &tmp);
				if (hook->list == NULL)
				{
					if ((hook = (NumericList *) remove_from_list ((List **) & numeric_list, buf)) != NULL)
					{
						new_free (&(hook->name));
						new_free ((char **) &hook);
					}
				}
				return;
			}
		}
		else
		{
			remove_from_list ((List **) & numeric_list, buf);
			for (tmp = hook->list; tmp; tmp = next)
			{
				next = tmp->next;
				tmp->not = 1;
				new_free (&(tmp->nick));
				new_free (&(tmp->stuff));
				new_free ((char **) &tmp);
			}
			hook->list = NULL;
			new_free ((char **) &hook->name);
			new_free ((char **) &hook);
			if (!quiet)
				say ("The %s list is empty", buf);
			return;
		}
	}
	if (quiet)
		return;
	if (nick)
		say ("\"%s\" is not on the %s list", nick, buf);
	else
		say ("The %s list is empty", buf);
}

extern void 
flush_on_hooks (void)
{
	int x;
	int old_display = window_display;

	window_display = 0;
	for (x = 1; x < 999; x++)
		remove_numeric_hook (x, NULL, 1, x, 1);
	for (x = 0; x < NUMBER_OF_LISTS; x++)
		remove_hook (x, NULL, 1, 0, 1);		/* the 4th arg should be 0, not x */
	window_display = old_display;
}

extern void 
remove_hook (int which, char *nick, int server, int sernum, int quiet)
{
	Hook *tmp, *next;

	if (which < 0)
	{
		remove_numeric_hook (-which, nick, server, sernum, quiet);
		return;
	}
	if (nick)
	{
		setup_struct ((server == -1) ? -1 : (server & ~HS_NOGENERIC), sernum - 1, sernum, 0);

		if ((tmp = (Hook *) remove_from_list_ext (
			     (List **) & (hook_functions[which].list), nick,
		       (int (*)(List *, char *)) Add_Remove_Check)) != NULL)
		{
			if (!quiet)
				say ("%c%s%c removed from %s list",
				     (tmp->flexible ? '\'' : '"'), nick,
				     (tmp->flexible ? '\'' : '"'),
				     hook_functions[which].name);
			tmp->not = 1;
			new_free (&(tmp->nick));
			new_free (&(tmp->stuff));
			new_free ((char **) &tmp);
		}
		else if (!quiet)
			say ("\"%s\" is not on the %s list", nick,
			     hook_functions[which].name);
	}
	else
	{
		Hook *prev = NULL;
		Hook *top = NULL;

		for (tmp = hook_functions[which].list; tmp; prev = tmp, tmp = next)
		{
			next = tmp->next;

			/* If given a non-zero sernum, then we clean out
			 * only those hooks that are at that level. */
			if (sernum && tmp->sernum != sernum)
			{
				if (!top)
					top = tmp;
				continue;
			}

			if (prev)
				prev->next = tmp->next;
			tmp->not = 1;
			new_free (&(tmp->nick));
			new_free (&(tmp->stuff));
			new_free ((char **) &tmp);
		}
		hook_functions[which].list = top;
		if (!quiet)
		{
			if (sernum)
				say ("The %s <%d> list is empty", hook_functions[which].name, sernum);
			else
				say ("The %s list is empty", hook_functions[which].name);
		}
	}
}

static void 
write_hook (FILE * fp, Hook * hook, char *name)
{
	char *stuff = NULL;
	char flexi = '"';

	if (hook->server != -1)
		return;

	if (hook->flexible)
		flexi = '\'';

	switch (hook->noisy)
	{
	case SILENT:
		stuff = "^";
		break;
	case QUIET:
		stuff = "-";
		break;
	case NORMAL:
		stuff = empty_string;
		break;
	case NOISY:
		stuff = "+";
		break;
	}

	if (hook->sernum)
		fprintf (fp, "ON #%s%s %d", stuff, name, hook->sernum);
	else
		fprintf (fp, "ON %s%s", stuff, name);

	fprintf (fp, " %c%s%c %s\n", flexi, hook->nick, flexi, hook->stuff);
}

/*
 * save_hooks: for use by the SAVE command to write the hooks to a file so it
 * can be interpreted by the LOAD command 
 */
extern void 
save_hooks (FILE * fp, int do_all)
{
	Hook *list;
	NumericList *numeric;
	int which;

	for (which = 0; which < NUMBER_OF_LISTS; which++)
	{
		for (list = hook_functions[which].list; list; list = list->next)
			if (!list->global ||do_all)
				write_hook (fp, list, hook_functions[which].name);
	}
	for (numeric = numeric_list; numeric; numeric = numeric->next)
	{
		for (list = numeric->list; list; list = list->next)
			if (!list->global)
				write_hook (fp, list, numeric->name);
	}
}

#define INVALID_HOOKNUM -1001

/*
 * find_hook: returns the numerical value for a specified hook name
 */
int 
find_hook (char *name)
{
	int which = INVALID_HOOKNUM, i, len, cnt;

	if (!(len = strlen (name)))
	{
		say ("You must specify an event type!");
		return INVALID_HOOKNUM;
	}

	upper (name);

	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!strncmp (name, hook_functions[i].name, len))
		{
			if (strlen (hook_functions[i].name) == len)
			{
				cnt = 1;
				which = i;
				break;
			}
			else
			{
				cnt++;
				which = i;
			}
		}
		else if (cnt)
			break;
	}

	if (cnt == 0)
	{
		if (is_number (name))
		{
			which = my_atol (name);

			if ((which < 1) || (which > 999))
			{
				say ("Numerics must be between 001 and 999");
				return INVALID_HOOKNUM;
			}
			which = -which;
		}
		else
		{
			say ("No such ON function: %s", name);
			return INVALID_HOOKNUM;
		}
	}
	else if (cnt > 1)
	{
		say ("Ambiguous ON function: %s", name);
		return INVALID_HOOKNUM;
	}

	return which;
}

 /* 
    * shook: the SHOOK command -- this probably doesnt belong here,
    * and shook is probably a stupid name
  */
BUILT_IN_COMMAND (shook)
{
	int which;
	char *arg;

	if (!args || !*args)
	{
		userage (command, helparg);
		return;
	}

	arg = next_arg (args, &args);

	if ((which = find_hook (arg)) == INVALID_HOOKNUM)
		return;
	else
		do_hook (which, "%s", args);
}

/* on: The ON command */
BUILT_IN_COMMAND (oncmd)
{
#if 0
	char *func, *nick, *serial;
	/* int noisy = NORMAL, not = 0, remove = 0, -not used */
	int noisy, not, server, sernum, remove, which = 0;
	int flexible;
	char type;

	if ((func = next_arg (args, &args)) != NULL)
	{
		if (*func == '#')
		{
			if (!(serial = next_arg (args, &args)))
			{
				say ("No serial number specified");
				return;
			}
			sernum = my_atol (serial);
			func++;
		}
		else
			sernum = 0;

		switch (*func)
		{
		case '&':
			server = from_server;
			func++;
			break;
		case '@':
			server = from_server | HS_NOGENERIC;
			func++;
			break;
		default:
			server = -1;
			break;
		}

		switch (*func)
		{
		case '-':
			noisy = QUIET;
			func++;
			break;
		case '^':
			noisy = SILENT;
			func++;
			break;
		case '+':
			noisy = NOISY;
			func++;
			break;
		default:
			noisy = NORMAL;
			break;
		}


		if ((which = find_hook (func)) == INVALID_HOOKNUM)
			return;

		/* XXX This is bogus XXX */
		if (which == INPUT_LIST && get_int_var (INPUT_PROTECTION_VAR))
		{
			say ("You cannot use /ON INPUT with INPUT_PROTECTION set");
			say ("Please read /HELP ON INPUT, and /HELP SET INPUT_PROTECTION");
			return;
		}

		remove = 0;
		not = 0;

		switch (*args)
		{
		case '-':
			remove = 1;
			args++;
			break;
		case '^':
			not = 1;
			args++;
			break;
		}

		if ((nick = new_new_next_arg (args, &args, &type)) != NULL)
		{
			if (which < 0)
				nick = fill_it_out (nick, 1);
			else
				nick = fill_it_out (nick, hook_functions[which].params);

			if (type == '\'')
				flexible = 1;
			else
				flexible = 0;

			if (remove)
			{
				if (strlen (nick) == 0)
					say ("No expression specified");
				else
					remove_hook (which, nick, server, sernum, 0);
			}
			else
				/* Indent this bit back a couple of tabs - phone */

			{
				if (not)
					args = empty_string;

				if (*nick)
				{
					char *exp;

					while (my_isspace (*args))
						args++;

					if (*args == '{')
					{
						if (!(exp = next_expr (&args, '{')))
						{
							say ("Unmatched brace in ON");
							new_free (&nick);
							return;
						}
					}
					else
						exp = args;

					add_hook (which, nick, exp, noisy, not, server, sernum, flexible);
					if (which < 0)
						say ("On %3.3u from %c%s%c do %s [%s] <%d>",
						   -which, type, nick, type,
						     (not ? "nothing" : exp),
						noise_level[noisy], sernum);
					else
						say ("On %s from %c%s%c do %s [%s] <%d>",
						 hook_functions[which].name,
						     type, nick, type,
						     (not ? "nothing" : exp),
						noise_level[noisy], sernum);
					new_free (&nick);
				}
			}
			/* End of doovie intentation */
		}
		else
		{
			if (remove)
				remove_hook (which, NULL, server, sernum, 0);
			else
			{
				/* the help files say that that an /on 0
				 * shows all the numeric /ONs.  Since the
				 * ACTION hook is "number 0", there is no
				 * way to tell right here whether the user
				 * user specified an ACTION or a "show me 
				 * it all". blah. ("feature" rmd from helps)
				 */
				if (which < 0)
				{
					if (show_numeric_list (-which) == 0)
						say ("The %3.3u list is empty.", -which);
				}
				else if (show_list (which) == 0)
					say ("The %s list is empty.", hook_functions[which].name);
			}
		}
	}
	else
	{
		int total = 0;

		say ("ON listings:");
		for (which = 0; which < NUMBER_OF_LISTS; which++)
			total += show_list (which);
		total += show_numeric_list (0);
		if (total == 0)
			say ("All ON lists are empty.");
	}
#endif
}
