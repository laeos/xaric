/*
 * lastlog.h: header for lastlog.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __lastlog_h_
#define __lastlog_h_

#define LOG_NONE	0x000000
#define LOG_CURRENT	0x000000
#define LOG_CRAP	0x000001
#define LOG_PUBLIC	0x000002
#define LOG_MSG		0x000004
#define LOG_NOTICE	0x000008
#define LOG_WALL	0x000010
#define LOG_WALLOP	0x000020
#define LOG_NOTES	0x000040
#define LOG_OPNOTE	0x000080
#define	LOG_SNOTE	0x000100
#define	LOG_ACTION	0x000200
#define	LOG_DCC		0x000400
#define LOG_CTCP	0x000800
#define	LOG_USER1	0x001000
#define LOG_USER2	0x002000
#define LOG_USER3	0x004000
#define LOG_USER4	0x008000
#define LOG_USER5	0x010000
#define LOG_BEEP	0x020000
#define LOG_SEND_MSG	0x080000
#define LOG_KILL	0x100000

#define LOG_ALL (LOG_CRAP | LOG_PUBLIC | LOG_MSG | LOG_NOTICE | LOG_WALL | \
		LOG_WALLOP | LOG_NOTES | LOG_OPNOTE | LOG_SNOTE | LOG_ACTION | \
		LOG_CTCP | LOG_DCC | LOG_USER1 | LOG_USER2 | LOG_USER3 | \
		LOG_USER4 | LOG_USER5 | LOG_BEEP | LOG_SEND_MSG)

# define LOG_DEFAULT	LOG_NONE

	void	set_lastlog_level(Window *, char *, int);
unsigned long	set_lastlog_msg_level(unsigned long);
	void	set_lastlog_size(Window *, char *, int);
	void	set_notify_level(Window *, char *, int);
	void	set_msglog_level(Window *, char *, int);
	void	lastlog(char *, char *, char *, char *);
	void	add_to_lastlog(Window *, const char *);
	char	*bits_to_lastlog_level(unsigned long);
unsigned long	real_lastlog_level(void);
unsigned long	real_notify_level(void);
unsigned long	parse_lastlog_level(char *);
	int	islogged(Window *);
extern        void    remove_from_lastlog     (Window *);

#endif /* __lastlog_h_ */
