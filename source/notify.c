/*
 * notify.c: a few handy routines to notify you when people enter and leave irc 
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Modified Colten Edwards 96
 * Rewritten Rex Feany '98
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "list.h"
#include "notify.h"
#include "ircaux.h"
#include "whois.h"
#include "hook.h"
#include "server.h"
#include "output.h"
#include "vars.h"
#include "timer.h"
#include "misc.h"
#include "status.h"
#include "fset.h"
#include "tcommand.h"

/* NotifyList: the structure for the notify stuff */
typedef struct notify_stru
{
	struct notify_stru *next;	/* pointer to next notify person */
	char *nick;		/* nickname of person to notify about */
	char *user;
	char *host;
	int flag;		/* 1=person on irc, 0=person not on irc */
	int times;
	time_t lastseen;	/* time of last signon */
	time_t offline;		/* time of last signoff */
	time_t added;		/* time added to list */
}
NotifyList;

extern Server *server_list;
extern int number_of_servers;




/* notify: the NOTIFY command.  Does the whole ball-o-wax */
void
cmd_notify (struct command *cmd, char *args)
{
	char *nick, *ptr;
	int no_nicks = 1;
	int servnum = from_server;
	int shown = 0;
	NotifyList *new;

	bitchsay ("Notify is currently somewhat broken!!");
	while ((nick = next_arg (args, &args)) != NULL)
	{
		no_nicks = 0;
		while (nick)
		{
			shown = 0;
			if ((ptr = strchr (nick, ',')) != NULL)
				*ptr++ = '\0';

			if (*nick == '-')
			{
				nick++;

				if (*nick)
				{
					for (servnum = 0; servnum < number_of_servers; servnum++)
					{
						if ((new = (NotifyList *) remove_from_list ((List **) & (server_list[servnum].notify_list), nick)))
						{
							new_free (&(new->nick));
							new_free (&(new->host));
							new_free (&(new->user));
							new_free ((char **) &new);

							if (!shown)
							{
								bitchsay ("%s removed from notification list", nick);
								shown = 1;
							}
						}
						else
						{
							if (!shown)
							{
								bitchsay ("%s is not on the notification list", nick);
								shown = 1;
							}
						}
					}
				}
				else
				{
					for (servnum = 0; servnum < number_of_servers; servnum++)
					{
						while ((new = server_list[servnum].notify_list))
						{
							server_list[servnum].notify_list = new->next;
							new_free (&new->nick);
							new_free (&(new->user));
							new_free (&new->host);
							new_free ((char **) &new);
						}
					}
					bitchsay ("Notify list cleared");
				}
			}
			else
			{
				/* compatibility */
				if (*nick == '+')
					nick++;

				if (*nick)
				{
					if (strchr (nick, '*'))
						bitchsay ("Wildcards not allowed in NOTIFY nicknames!");
					else
					{
						for (servnum = 0; servnum < number_of_servers; servnum++)
						{
							if ((new = (NotifyList *) remove_from_list ((List **) & server_list[servnum].notify_list, nick)) != NULL)
							{
								new_free (&(new->nick));
								new_free (&(new->user));
								new_free (&(new->host));
								new_free ((char **) &new);
							}
							new = (NotifyList *) new_malloc (sizeof (NotifyList));
							new->nick = m_strdup (nick);
							new->added = time (NULL);
							add_to_list ((List **) & server_list[servnum].notify_list, (List *) new);
						}
						bitchsay ("%s added to the notification list", nick);
					}
				}
				else
					show_notify_list ();
			}
			nick = ptr;
		}
	}
	if (no_nicks)
		show_notify_list ();
	else
		do_serv_notify (from_server);

}

/*
 * do_notify: This simply goes through the notify list, sending out a WHOIS
 * for each person on it.  This uses the fancy whois stuff in whois.c to
 * figure things out.  Look there for more details, if you can figure it out.
 * I wrote it and I can't figure it out.
 *
 * Thank you Michael... leaving me bugs to fix :) Well I fixed them!
 */

void 
do_serv_notify (int servnum)
{
	int size;
	int old_from_server = from_server;
	char buf[BIG_BUFFER_SIZE + 1];
	NotifyList *tmp;

	if (!number_of_servers)
		return;
	if (servnum < 0)
		return;

	tmp = server_list[servnum].notify_list;
	memset (buf, 0, sizeof (buf));
	while (tmp)
	{
		for (*buf = 0, size = 0; tmp; tmp = tmp->next)
		{
			size++;
			if (size < 5)
			{
				strcat (buf, " ");
				strcat (buf, tmp->nick);
			}
			else
				break;
		}

		if (!size)
			continue;
#if 0
		if (is_server_connected (servnum) && *buf)
		{
			servnum = servnum;
			add_to_userhost_queue (buf, got_userhost, "%s", buf);
		}
#endif
	}
	from_server = old_from_server;
	return;
}

void 
do_notify (void)
{
	int i;

	if (!number_of_servers)
		return;

	for (i = 0; i < number_of_servers; i++)
		do_serv_notify (i);
}

/*
 * notify_mark: This marks a given person on the notify list as either on irc
 * (if flag is 1), or not on irc (if flag is 0).  If the person's status has
 * changed since the last check, a message is displayed to that effect.  If
 * the person is not on the notify list, this call is ignored 
 *
 */
void 
notify_mark (char *nick, char *user, char *host, int flag)
{
	NotifyList *tmp;
	time_t now = time (NULL);
	char *user1 = NULL;
	char *host1 = NULL;

	if (from_server != primary_server && from_server != -1)
		return;
	if (!nick)
		return;
	if (user && strcmp (user, "<UNKNOWN>"))
		user1 = user;
	if (host && strcmp (host, "<UNKNOWN>"))
		host1 = host;

	if ((tmp = (NotifyList *) list_lookup ((List **) & server_list[from_server].notify_list, nick, !USE_WILDCARDS, !REMOVE_FROM_LIST)) != NULL)
	{
		if (flag)
		{
			if (tmp->flag != 1)
			{
				/*
				 * copy the correct case of the nick
				 * into our array  ;)
				 */
				malloc_strcpy (&(tmp->nick), nick);

				if (!tmp->user || (user1 && strcmp (tmp->user, user1)))
					malloc_strcpy (&tmp->user, user1);
				if (!tmp->host || (user1 && strcmp (tmp->host, host1)))
					malloc_strcpy (&tmp->host, host1);

				tmp->flag = 1;
				tmp->lastseen = 0;
				tmp->times++;

				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_SIGNON_UH_FSET), "%s %s %s %s", update_clock (GET_TIME), nick, tmp->user, tmp->host));
			}
		}
		else
		{
			if (tmp->flag)
			{
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_SIGNOFF_UH_FSET), "%s %s %s %s", update_clock (GET_TIME), nick, tmp->user, tmp->host));
				tmp->offline += now - tmp->added;
				tmp->lastseen = now - tmp->added;
				new_free (&tmp->host);
				new_free (&tmp->user);
			}
			tmp->flag = 0;
		}
	}
}

/* Rewritten, -lynx */
void 
show_notify_list (void)
{
	NotifyList *tmp;
	int count = 0;


	if (from_server == -1)
		return;

	put_it ("%s", convert_output_format ("%G---[ %COnline Users %G]---", NULL, NULL));
	for (tmp = server_list[from_server].notify_list; tmp; tmp = tmp->next)
	{
		if (tmp->flag)
		{
			if (count == 0)
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_ON_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_ON_FSET), "%s %s@%s %d %l %l", tmp->nick, tmp->user ? tmp->user : "unknown", tmp->host ? tmp->host : "unknown", tmp->times, tmp->offline, tmp->lastseen));
			count++;
		}
	}
	put_it ("%s", convert_output_format ("%G---[ %COffline Users %G]---", NULL, NULL));
	count = 0;
	for (tmp = server_list[from_server].notify_list; tmp; tmp = tmp->next)
	{
		if (!(tmp->flag))
		{
			if (count == 0)
				put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
			put_it ("%s", convert_output_format (get_fset_var (FORMAT_NOTIFY_OFF_FSET), "%s %s@%s %d %l %l", tmp->nick, tmp->user ? tmp->user : "unknown", tmp->host ? tmp->host : "unknown", tmp->times, tmp->offline, tmp->lastseen));
			count++;
		}
	}
}

void 
save_notify (FILE * fp)
{
	int size = 0;
	int count = 0;
	NotifyList *tmp;

	if (number_of_servers && server_list[0].notify_list)
	{
		fprintf (fp, "NOTIFY");
		for (tmp = server_list[0].notify_list; tmp; tmp = tmp->next)
		{
			size++;	/* += (strlen(tmp->nick) + 1); */
			fprintf (fp, " %s", tmp->nick);
			if (size >= 10)
			{
				fprintf (fp, "\n");
				if (tmp->next)
					fprintf (fp, "NOTIFY");
				size = 0;
			}
			count++;
		}
		fprintf (fp, "\n");
	}
	if (count && do_hook (SAVEFILE_LIST, "Notify %d", count))
		bitchsay ("Saved %d Notify entries", count);
}

void 
make_notify_list (int servnum)
{
	NotifyList *tmp;

	server_list[servnum].notify_list = NULL;

	for (tmp = server_list[0].notify_list; tmp; tmp = tmp->next)
	{
		NotifyList *new = (NotifyList *) new_malloc (sizeof (NotifyList));
		new->nick = m_strdup (tmp->nick);
		new->host = m_strdup (tmp->host);
		new->user = m_strdup (tmp->user);
		new->flag = 0;
		add_to_list ((List **) & server_list[servnum].notify_list, (List *) new);
	}
	do_serv_notify (servnum);
}
