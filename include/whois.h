#ifndef __whois_h_
#define __whois_h_

/*
 * whois.h: header for whois.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */


typedef	struct	WhoisStuffStru
{
	char	*nick;
	char	*user;
	char	*host;
	char	*channel;
	char	*channels;
	char	*name;
	char	*server;
	char	*server_stuff;
	char	*away;
	int	oper;
	int	chop;
	int	not_on;
}	WhoisStuff;

typedef	struct	WhoisQueueStru
{
	char	*nick;			/* nickname of whois'ed person(s) */
	char	*text;			/* additional text */
	int	type;			/* Type of WHOIS queue entry */
	/*
	 * called with func((WhoisStuff *)stuff,(char *) nick, (char *) text) 
	 */
	void	 (*func) ();
	struct	WhoisQueueStru	*next;/* next element in queue */
}	WhoisQueue;


void	add_to_whois_queue(char *, void (*)(WhoisStuff *, char *, char *), char *, ...);
void	add_to_userhost_queue(char *, void (*func)(WhoisStuff *, char *, char *), char *, ...);
void	got_userhost(WhoisStuff *, char *, char *);
void	got_my_userhost(WhoisStuff *, char *, char *);
void	add_ison_to_whois(char *, void (*) ());
void	whois_name(char *, char **);
void	whowas_name(char *, char **);
void	whois_channels(char *, char **);
void	whois_server(char *, char **);
void	whois_oper(char *, char **);
void	whois_lastcom(char *, char **);
void	whois_nickname(WhoisStuff *, char *, char *);
void	whois_ignore_msgs(WhoisStuff *, char *, char *);
void	whois_ignore_notices(WhoisStuff *, char *, char *);
void	whois_ignore_walls(WhoisStuff *, char *, char *);
void	whois_ignore_invites(WhoisStuff *, char *, char *);
void	whois_notify(WhoisStuff *stuff, char *, char *);
void	whois_new_wallops(WhoisStuff *, char *, char *);
void	clean_whois_queue(void);
void	set_beep_on_msg(Window *, char *, int);
void    userhost_cmd_returned(WhoisStuff *, char *, char *);
void	user_is_away(char *, char **);
void	userhost_returned(char *, char **);
void	ison_returned(char *, char **);
void	whois_chop(char *, char **);
void	end_of_whois(char *, char **);
void	whoreply(char *, char **);
void	convert_to_whois(void);
void	ison_notify(char *, char *);
void	no_such_nickname(char *, char **);
extern	unsigned long beep_on_level;
extern	char	*redirect_format;

#define	WHOIS_WHOIS	0x01
#define	WHOIS_USERHOST	0x02
#define	WHOIS_ISON	0x04
#define WHOIS_ISON2	0x08
#define WHOIS_WHOWAS	0x10

#define USERHOST_USERHOST ((void (*)(WhoisStuff *, char *, char *)) 1)

#endif /* __whois_h_ */
