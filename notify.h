/*
 * notify.h: header for notify.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __notify_h_
#define __notify_h_

	void	show_notify_list(void);
	void	notify(char *, char *, char *, char *);
	void	do_notify(void);
	void	notify_mark(char *, char *, char *, int);
	void	save_notify(FILE *);
	void	make_notify_list(int);
	void	do_serv_notify(int);


#endif /* __notify_h_ */
