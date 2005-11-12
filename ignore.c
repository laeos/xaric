
/*
 * ignore.c: handles the ingore command for irc 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "ignore.h"
#include "ircaux.h"
#include "list.h"
#include "input.h"
#include "screen.h"
#include "misc.h"
#include "vars.h"
#include "output.h"
#include "tcommand.h"

int ignore_usernames = 0;
char *highlight_char = NULL;

static int remove_ignore (char *);
static void ignore_list (char *);
static char *cut_n_fix_glob (char *);

/* ignored_nicks: pointer to the head of the ignore list */
Ignore *ignored_nicks = NULL;

static Ignore *new_ignore = NULL;

#define IGNORE_REMOVE 1
#define IGNORE_DONT 2
#define IGNORE_HIGH -1
#define IGNORE_CGREP -2


void 
add_channel_grep (char *channel, char *what, int flag)
{
	Ignore *new;
	int count;
	char *chan, *ptr;
	char *new_str, *p;
	while (channel)
	{
		new_str = p = m_strdup (what);
		if ((ptr = strchr (channel, ',')) != NULL)
			*ptr = '\0';

		chan = make_channel (channel);
		if (!(new = (Ignore *) list_lookup ((struct list **) & ignored_nicks, chan, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
		{
			Ignore *tmp, *old;
			char *s;
			if ((new = (Ignore *) remove_from_list ((struct list **) & ignored_nicks, channel)) != NULL)
			{
				new_free (&(new->nick));
				for (tmp = new->except; tmp; tmp = old)
				{
					old = tmp->next;
					new_free (&(tmp->nick));
					new_free (&(tmp));
				}
				for (tmp = new->looking; tmp; tmp = old)
				{
					old = tmp->next;
					new_free (&(tmp->nick));
					new_free (&(tmp));
				}
				new_free ((char **) &new);
			}
			new = (Ignore *) new_malloc (sizeof (Ignore));
			new->nick = m_strdup (chan);
			while ((s = new_next_arg (new_str, &new_str)))
			{
				tmp = (Ignore *) new_malloc (sizeof (Ignore));
				tmp->nick = m_strdup (s);
				add_to_list ((struct list **) & new->looking, (struct list *) tmp);
			}
			add_to_list ((struct list **) & ignored_nicks, (struct list *) new);
			new->cgrep = flag;
		}
		if (ptr)
			*(ptr++) = ',';
		channel = ptr;
		new_free (&p);
	}
	for (new = ignored_nicks, count = 1; new; new = new->next, count++)
		new->num = count;
	if (*what)
		*what = 0;
}

/*
 * ignore_nickname: adds nick to the ignore list, using type as the type of
 * ignorance to take place.  
 */
void 
ignore_nickname (char *nick, long type, int flag)
{
	Ignore *new, *newc;
	char *msg, *ptr;
	char *new_nick = NULL;
	char buffer[BIG_BUFFER_SIZE + 1];
	int count;

	if (type == -1)
		return;
	while (nick)
	{
		if ((ptr = strchr (nick, ',')) != NULL)
			*ptr = '\0';

		if (*nick)
		{
			new_nick = is_channel (nick) ? m_strdup (nick) : cut_n_fix_glob (nick);

			if (!(new = (Ignore *) list_lookup ((struct list **) & ignored_nicks, new_nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
			{
				if (flag == IGNORE_REMOVE)
				{
					say ("%s is not on the ignorance list", nick);
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
					continue;
				}
				else
				{
					if ((new = (Ignore *) remove_from_list ((struct list **) & ignored_nicks, nick)) != NULL)
					{
						Ignore *tmp, *old;
						new_free (&(new->nick));
						new_free (&new->looking);
						for (tmp = new->except; tmp; tmp = old)
						{
							old = tmp->next;
							new_free (&(tmp->nick));
							new_free (&(tmp));
						}
						for (tmp = new->looking; tmp; tmp = old)
						{
							old = tmp->next;
							new_free (&(tmp->nick));
							new_free (&(tmp));
						}
						new_free ((char **) &new);
					}
					new = (Ignore *) new_malloc (sizeof (Ignore));
					new->nick = new_nick;
					add_to_list ((struct list **) & ignored_nicks, (struct list *) new);
				}
				for (newc = ignored_nicks, count = 1; newc; newc = newc->next, count++)
					newc->num = count;

			}
			new_ignore = new;
			switch (flag)
			{
			case IGNORE_REMOVE:
				new->type &= (~type);
				new->high &= (~type);
				new->dont &= (~type);
				new->cgrep &= (~type);
				msg = "Not ignoring";
				break;
			case IGNORE_DONT:
				new->dont |= type;
				new->type &= (~type);
				new->high &= (~type);
				new->cgrep &= (~type);
				msg = "Never ignoring";
				break;
			case IGNORE_HIGH:
				new->high |= type;
				new->type &= (~type);
				new->dont &= (~type);
				new->cgrep &= (~type);
				msg = "Highlighting";
				break;
			case IGNORE_CGREP:
				new->cgrep |= type;
				new->high &= (~type);
				new->type &= (~type);
				new->dont &= (~type);
				msg = "Channel Grep";
				break;
			default:
				new->type |= type;
				new->high &= (~type);
				new->dont &= (~type);
				new->cgrep &= (~type);
				msg = "Ignoring";
				break;
			}
			if (type == IGNORE_ALL)
			{
				switch (flag)
				{
				case IGNORE_REMOVE:
					remove_ignore (new->nick);
					break;
				case IGNORE_HIGH:
					say ("Highlighting ALL messages from %s", new->nick);
					break;
				case IGNORE_CGREP:
					say ("Grepping ALL messages from %s", new->nick);
					break;
				case IGNORE_DONT:
					say ("Never ignoring messages from %s", new->nick);
					break;
				default:
					say ("Ignoring ALL messages from %s", new->nick);
					break;
				}
				return;
			}
			else if (type)
			{
				strcpy (buffer, msg);
				if (type & IGNORE_MSGS)
					strcat (buffer, " MSGS");
				if (type & IGNORE_PUBLIC)
					strcat (buffer, " PUBLIC");
				if (type & IGNORE_WALLS)
					strcat (buffer, " WALLS");
				if (type & IGNORE_WALLOPS)
					strcat (buffer, " WALLOPS");
				if (type & IGNORE_INVITES)
					strcat (buffer, " INVITES");
				if (type & IGNORE_NOTICES)
					strcat (buffer, " NOTICES");
				if (type & IGNORE_NOTES)
					strcat (buffer, " NOTES");
				if (type & IGNORE_CTCPS)
					strcat (buffer, " CTCPS");
				if (type & IGNORE_CRAP)
					strcat (buffer, " CRAP");
				if (type & IGNORE_KICKS)
					strcat (buffer, " KICKS");
				if (type & IGNORE_MODES)
					strcat (buffer, " MODES");
				if (type & IGNORE_SMODES)
					strcat (buffer, " SMODES");
				if (type & IGNORE_JOINS)
					strcat (buffer, " JOINS");
				if (type & IGNORE_TOPICS)
					strcat (buffer, " TOPICS");
				if (type & IGNORE_QUITS)
					strcat (buffer, " QUITS");
				if (type & IGNORE_PARTS)
					strcat (buffer, " PARTS");
				if (type & IGNORE_NICKS)
					strcat (buffer, " NICKS");
				if (type & IGNORE_PONGS)
					strcat (buffer, " PONGS");
				if (type & IGNORE_SPLITS)
					strcat (buffer, " SPLITS");
				say ("%s from %s", buffer, new->nick);
			}
		}
		if (ptr)
			*(ptr++) = ',';
		nick = ptr;
	}
}

/*
 * remove_ignore: removes the given nick from the ignore list and returns 0.
 * If the nick wasn't in the ignore list to begin with, 1 is returned. 
 */
static int 
remove_ignore (char *nick)
{
	Ignore *tmp, *new, *old;
	char *new_nick = NULL;
	int count = 0;

	new_nick = (is_channel (nick) ? m_strdup (nick) : cut_n_fix_glob (nick));

	/*
	 * Look for an exact match first.
	 */
	if ((tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, new_nick, !USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
	{
		say ("%s removed from ignorance list", tmp->nick);
		new_free (&(tmp->nick));
		for (new = tmp->except; new; new = old)
		{
			old = new->next;
			new_free (&(new->nick));
			new_free (&(new));
		}
		for (new = tmp->looking; new; new = old)
		{
			old = new->next;
			new_free (&(new->nick));
			new_free (&(new));
		}
		new_free ((char **) &tmp);
		count++;
	}

	/*
	 * Otherwise clear everything that matches.
	 */
	else
		while ((tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, new_nick, USE_WILDCARDS, REMOVE_FROM_LIST)) != NULL)
		{
			say ("%s removed from ignorance list", tmp->nick);
			new_free (&(tmp->nick));
			for (new = tmp->except; new; new = old)
			{
				old = new->next;
				new_free (&(new->nick));
				new_free (&(new));
			}
			for (new = tmp->looking; new; new = old)
			{
				old = new->next;
				new_free (&(new->nick));
				new_free (&(new));
			}
			new_free ((char **) &tmp);

			count++;
		}

	if (!count)
		say ("%s is not in the ignorance list!", new_nick);

	new_free (&new_nick);
	return count;
}

/*
 * is_ignored: checks to see if nick is being ignored (poor nick).  Checks
 * against type to see if ignorance is to take place.  If nick is marked as
 * IGNORE_ALL or ignorace types match, 1 is returned, otherwise 0 is
 * returned.  
 */
int 
is_ignored (char *nick, long type)
{
	Ignore *tmp;

	if (ignored_nicks)
	{
		if ((tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, nick, USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
		{
			if (tmp->dont & type)
				return (DONT_IGNORE);
			if (tmp->type & type)
				return (IGNORED);
			if (tmp->high & type)
				return (HIGHLIGHTED);
			if (tmp->high & type)
				return (CHANNEL_GREP);
		}
	}
	return (0);
}

int 
check_is_ignored (char *nick)
{
	Ignore *tmp;

	if (ignored_nicks)
	{
		if ((tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, nick, USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
			return 1;
	}
	return 0;
}

/* ignore_list: shows the entired ignorance list */
static void 
ignore_list (char *nick)
{
	Ignore *tmp;
	int len = 0;
	char buffer[BIG_BUFFER_SIZE + 1];
	if (ignored_nicks)
	{
		say ("Ignorance List:");
		if (nick)
			len = strlen (nick);
		for (tmp = ignored_nicks; tmp; tmp = tmp->next)
		{

			if (nick)
			{
				if (strncmp (nick, tmp->nick, len))
					continue;
			}
			*buffer = 0;
			if (tmp->type == IGNORE_ALL)
				strmopencat (buffer, BIG_BUFFER_SIZE, " ALL", NULL);
			else if (tmp->high == IGNORE_ALL)
				strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "ALL", highlight_char, NULL);
			else if (tmp->dont == IGNORE_ALL)
				strmopencat (buffer, BIG_BUFFER_SIZE, " DONT-ALL", NULL);
			else
			{
				if (tmp->type & IGNORE_PUBLIC)
					strmcat (buffer, " PUBLIC", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_PUBLIC)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "PUBLIC", highlight_char, NULL);
				else if (tmp->dont & IGNORE_PUBLIC)
					strmcat (buffer, " DONT-PUBLIC", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_MSGS)
					strmcat (buffer, " MSGS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_MSGS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "MSG", highlight_char, NULL);
				else if (tmp->dont & IGNORE_MSGS)
					strmcat (buffer, " DONT-MSGS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_WALLS)
					strmcat (buffer, " WALLS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "WALL", highlight_char, NULL);
				else if (tmp->dont & IGNORE_WALLS)
					strmcat (buffer, " DONT-WALLS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_WALLOPS)
					strmcat (buffer, " WALLOPS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_WALLOPS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "WALLOPS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_WALLOPS)
					strmcat (buffer, " DONT-WALLOPS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_INVITES)
					strmcat (buffer, " INVITES", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_INVITES)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "INVITES", highlight_char, NULL);
				else if (tmp->dont & IGNORE_INVITES)
					strmcat (buffer, " DONT-INVITES", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_NOTICES)
					strmcat (buffer, " NOTICES", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTICES)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "NOTICES", highlight_char, NULL);
				else if (tmp->dont & IGNORE_NOTICES)
					strmcat (buffer, " DONT-NOTICES", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_NOTES)
					strmcat (buffer, " NOTES", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NOTES)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "NOTES", highlight_char, NULL);
				else if (tmp->dont & IGNORE_NOTES)
					strmcat (buffer, " DONT-NOTES", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_CTCPS)
					strmcat (buffer, " CTCPS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CTCPS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "CTCPS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_CTCPS)
					strmcat (buffer, " DONT-CTCPS", BIG_BUFFER_SIZE);


				if (tmp->type & IGNORE_KICKS)
					strmcat (buffer, " KICKS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_KICKS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "KICKS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_KICKS)
					strmcat (buffer, " DONT-KICKS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_MODES)
					strmcat (buffer, " MODES", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_MODES)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "MODES", highlight_char, NULL);
				else if (tmp->dont & IGNORE_MODES)
					strmcat (buffer, " DONT-MODES", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_SMODES)
					strmcat (buffer, " SMODES", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_SMODES)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "SMODES", highlight_char, NULL);
				else if (tmp->dont & IGNORE_SMODES)
					strmcat (buffer, " DONT-SMODES", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_JOINS)
					strmcat (buffer, " JOINS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_JOINS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "JOINS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_JOINS)
					strmcat (buffer, " DONT-JOINS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_TOPICS)
					strmcat (buffer, " TOPICS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_TOPICS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "TOPICS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_TOPICS)
					strmcat (buffer, " DONT-TOPICS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_QUITS)
					strmcat (buffer, " QUITS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_QUITS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "QUITS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_QUITS)
					strmcat (buffer, " DONT-QUITS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_PARTS)
					strmcat (buffer, " PARTS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_PARTS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "PARTS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_PARTS)
					strmcat (buffer, " DONT-PARTS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_NICKS)
					strmcat (buffer, " NICKS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_NICKS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "NICKS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_NICKS)
					strmcat (buffer, " DONT-NICKS", BIG_BUFFER_SIZE);


				if (tmp->type & IGNORE_PONGS)
					strmcat (buffer, " PONGS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_PONGS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "PONGS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_PONGS)
					strmcat (buffer, " DONT-PONGS", BIG_BUFFER_SIZE);


				if (tmp->type & IGNORE_SPLITS)
					strmcat (buffer, " SPLITS", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_SPLITS)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "SPLITS", highlight_char, NULL);
				else if (tmp->dont & IGNORE_SPLITS)
					strmcat (buffer, " DONT-SPLITS", BIG_BUFFER_SIZE);

				if (tmp->type & IGNORE_CRAP)
					strmcat (buffer, " CRAP", BIG_BUFFER_SIZE);
				else if (tmp->high & IGNORE_CRAP)
					strmopencat (buffer, BIG_BUFFER_SIZE, " ", highlight_char, "CRAP", highlight_char, NULL);
				else if (tmp->dont & IGNORE_CRAP)
					strmcat (buffer, " DONT-CRAP", BIG_BUFFER_SIZE);

			}
			if (tmp->except)
			{
				Ignore *new;
				strmcat (buffer, " EXCEPT ", BIG_BUFFER_SIZE);
				for (new = tmp->except; new; new = new->next)
					strmopencat (buffer, BIG_BUFFER_SIZE, new->nick, " ", NULL);
			}
			if (tmp->looking && tmp->cgrep)
			{
				Ignore *new;
				strmcat (buffer, " CGREP ", BIG_BUFFER_SIZE);
				for (new = tmp->looking; new; new = new->next)
					strmopencat (buffer, BIG_BUFFER_SIZE, "[", new->nick, "] ", NULL);
			}
			put_it ("%s", convert_output_format ("  %K[%G$[-2]0%K] %C$[-25]1%W:%n $2-", "%d %s %s", tmp->num, tmp->nick, buffer));
		}
	}
	else
		bitchsay ("There are no nicknames or channels being ignored");
}

long 
ignore_type (char *type, int len)
{
	long ret = -1;
	if (!type || !*type)
		return -1;
	if (!strncmp (type, "ALL", len))
		ret = IGNORE_ALL;
	else if (!strncmp (type, "MSGS", len))
		ret = IGNORE_MSGS;
	else if (!strncmp (type, "PUBLIC", len))
		ret = IGNORE_PUBLIC;
	else if (!strncmp (type, "WALLS", len))
		ret = IGNORE_WALLS;
	else if (!strncmp (type, "WALLOPS", len))
		ret = IGNORE_WALLOPS;
	else if (!strncmp (type, "INVITES", len))
		ret = IGNORE_INVITES;
	else if (!strncmp (type, "NOTICES", len))
		ret = IGNORE_NOTICES;
	else if (!strncmp (type, "NOTES", len))
		ret = IGNORE_NOTES;
	else if (!strncmp (type, "CTCPS", len))
		ret = IGNORE_CTCPS;
	else if (!strncmp (type, "KICKS", len))
		ret = IGNORE_KICKS;
	else if (!strncmp (type, "MODES", len))
		ret = IGNORE_MODES;
	else if (!strncmp (type, "SMODES", len))
		ret = IGNORE_SMODES;
	else if (!strncmp (type, "JOINS", len))
		ret = IGNORE_JOINS;
	else if (!strncmp (type, "TOPICS", len))
		ret = IGNORE_TOPICS;
	else if (!strncmp (type, "QUITS", len))
		ret = IGNORE_QUITS;
	else if (!strncmp (type, "CRAP", len))
		ret = IGNORE_CRAP;
	else if (!strncmp (type, "PARTS", len))
		ret = IGNORE_PARTS;
	else if (!strncmp (type, "NICKS", len))
		ret = IGNORE_NICKS;
	else if (!strncmp (type, "PONGS", len))
		ret = IGNORE_PONGS;
	else if (!strncmp (type, "SPLITS", len))
		ret = IGNORE_SPLITS;
	else if (!strncmp (type, "NONE", len))
		ret = 0;
	return ret;
}

int 
ignore_exception (Ignore * old, char *args)
{
	Ignore *new = NULL;
	int flag = 0;
	if (args && !(new = (Ignore *) list_lookup ((struct list **) & old->except, args, !USE_WILDCARDS, !REMOVE_FROM_LIST)))
	{
		new = new_malloc (sizeof (Ignore));
		malloc_strcpy (&new->nick, args);
		add_to_list ((struct list **) & old->except, (struct list *) new);
		flag = DONT_IGNORE;
		bitchsay (" EXCEPT %s", new->nick);
	}
	return flag;
}

/*
 * ignore: does the /IGNORE command.  Figures out what type of ignoring the
 * user wants to do and calls the proper ignorance command to do it. 
 */
void
cmd_ignore (struct command *cmd, char *args)
{
	char *nick, *type;
	int len, flag, no_flags;
	long ret;

	if ((nick = next_arg (args, &args)) != NULL)
	{
		no_flags = 1;
		while ((type = next_arg (args, &args)) != NULL)
		{
			no_flags = 0;
			upper (type);
			switch (*type)
			{
			case '^':
				flag = IGNORE_DONT;
				type++;
				break;
			case '-':
				flag = IGNORE_REMOVE;
				type++;
				break;
			case '+':
				flag = IGNORE_HIGH;
				type++;
				break;
			case '%':
				flag = IGNORE_CGREP;
				type++;
				break;
			default:
				flag = 0;
				break;
			}
			if (!(len = strlen (type)))
			{
				say ("You must specify one of the following:");
				say ("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES CTCPS KICKS MODES SMODES JOINS TOPICS QUITS PARTS NICKS PONGS SQUITS CRAP NONE");
				return;
			}
			if (!strncmp (type, "NONE", len))
			{
				char *ptr;

				while (nick)
				{
					if ((ptr = strchr (nick, ',')) != NULL)
						*ptr = 0;
					if (*nick)
						remove_ignore (nick);
					if (ptr)
						*(ptr++) = ',';
					nick = ptr;
				}
			}
			else if (!strncmp (type, "EXCEPT", len) && new_ignore)
			{
				while ((nick = next_arg (args, &args)))
					flag = ignore_exception (new_ignore, nick);
			}
			else if ((flag == IGNORE_CGREP) && ((ret = ignore_type (type, len)) != -1))
				add_channel_grep (nick, args, ret);
			else if ((ret = ignore_type (type, len)) != -1)
				ignore_nickname (nick, ret, flag);
			else
			{
				bitchsay ("You must specify one of the following:");
				say ("\tALL MSGS PUBLIC WALLS WALLOPS INVITES \
NOTICES NOTES CTCPS KICKS MODES SMODES JOINS TOPICS QUITS PARTS NICKS PONGS SQUITS CRAP NONE");
			}
		}
		if (no_flags)
			ignore_list (nick);
		new_ignore = NULL;
	}
	else
		ignore_list (NULL);
}

/*
 * set_highlight_char: what the name says..  the character to use
 * for highlighting..  either BOLD, INVERSE, or UNDERLINE..
 */
void 
set_highlight_char (Window * win, char *s, int unused)
{
	int len = strlen (s);

	if (!my_strnicmp (s, "BOLD", len))
		malloc_strcpy (&highlight_char, BOLD_TOG_STR);
	else if (!my_strnicmp (s, "INVERSE", len))
		malloc_strcpy (&highlight_char, REV_TOG_STR);
	else if (!my_strnicmp (s, "UNDERLINE", len))
		malloc_strcpy (&highlight_char, UND_TOG_STR);
	else
		malloc_strcpy (&highlight_char, s);
}

/* check_ignore -- replaces the old double_ignore
 *   Why did i change the name?
 *      * double_ignore isnt really applicable any more becuase it doesnt
 *        do two ignore lookups, it only does one.
 *      * This function doesnt look anything like the old double_ignore
 *      * This function works for the new *!*@* patterns stored by
 *        ignore instead of the old nick and userhost patterns.
 * (jfn may 1995)
 */
int 
check_ignore (char *nick, char *userhost, char *channel, long type, char *str)
{
	char *nickuserhost = NULL;
	Ignore *tmp;

	malloc_sprintf (&nickuserhost, "%s!%s", nick ? nick : "*", userhost ? userhost : "*");

	if (ignored_nicks)
	{
		if ((tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, nickuserhost, USE_WILDCARDS, !REMOVE_FROM_LIST)))
		{
			if (tmp->except && list_lookup ((struct list **) & tmp->except, nickuserhost, USE_WILDCARDS, !REMOVE_FROM_LIST))
			{
				new_free (&nickuserhost);
				return (DONT_IGNORE);
			}

			new_free (&nickuserhost);
			if (tmp->dont & type)
				return (DONT_IGNORE);
			if (tmp->type & type)
				return (IGNORED);
			if (tmp->high & type)
				return (HIGHLIGHTED);
		}
		new_free (&nickuserhost);
		if (channel && is_channel (channel) && (tmp = (Ignore *) list_lookup ((struct list **) & ignored_nicks, channel, USE_WILDCARDS, !REMOVE_FROM_LIST)))
		{
			if (tmp->dont & type)
				return (DONT_IGNORE);
			if (tmp->type & type)
				return (IGNORED);
			if (tmp->high & type)
				return (HIGHLIGHTED);
			if ((tmp->cgrep & type) && str)
			{
				Ignore *t;
				for (t = tmp->looking; t; t = t->next)
				{
					if (stristr (str, t->nick))
						return DONT_IGNORE;
				}
				return IGNORED;
			}
		}
	}
	new_free (&nickuserhost);
	return (DONT_IGNORE);
}

/* Written by hop in April 1995 -- taken from SIRP */
/* MALLOCED */
static char *
cut_n_fix_glob (char *nickuserhost)
{
	char *nick, *userhost = NULL, *user = NULL, *host = NULL;
	char *final_stuff;
	char *copy = NULL;

	/* patch by texaco makes this work right */
	copy = m_strdup (nickuserhost);
	nick = copy;

	if ((userhost = strchr (copy, '!')))
	{
		/* NICK IS CORRECT HERE */

		*userhost++ = 0;

		/* doh! */
		user = userhost;

		if ((host = sindex (userhost, "@")))
			/* USER IS CORRECT HERE */
			*host++ = 0;

		else if (sindex (userhost, "."))
		{
			user = "*";
			host = userhost;
		}
		/* fixed by sheik */
		if (!user)
			user = "*";
		if (!host)
			host = "*";
	}
	else
	{
		user = copy;
		if ((host = sindex (user, "@")))
		{
			nick = "*";
			*host++ = 0;
		}
		else
		{
			if (sindex (user, "."))
			{
				nick = "*";
				host = user;
				user = "*";
			}
			else
			{
				nick = user;
				user = "*";
				host = "*";
			}
		}
	}
	final_stuff = m_opendup (nick, "!", user, "@", host, NULL);
	new_free (&copy);
	return final_stuff;
}

void 
tremove_ignore (char *stuff, char *line)
{
	int count = 0;
	Ignore *new, *newc, *ex;
	char *p;
	int except = 0;
	if (!line)
		return;
	while ((p = next_arg (line, &line)))
	{
		for (new = ignored_nicks; new; new = newc)
		{
			newc = new->next;
			if (matchmcommand (p, new->num) || !my_stricmp (new->nick, p))
			{
				for (ex = new->except; ex; ex = ex->next)
					except++;
				remove_ignore (new->nick);
				count++;
			}
		}
	}
	if (count)
		bitchsay ("Removed %d ignores and %d exceptions", count, except);
	else
		bitchsay ("No matching ignorance");
	for (new = ignored_nicks, count = 1; new; new = new->next, count++)
		new->num = count;
}

void
cmd_tignore (struct command *cmd, char *args)
{
	ignore_list (NULL);
	if (ignored_nicks)
	{
		if (args && *args)
			tremove_ignore (NULL, args);
		else
			add_wait_prompt ("Which ignore to delete (-2, 2-5, ...) ? ", tremove_ignore, args, WAIT_PROMPT_LINE);
	}
}
