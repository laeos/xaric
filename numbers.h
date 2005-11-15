/*
 * numbers.h: header for numbers.c
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id$
 */

#ifndef __numbers_h_
#define __numbers_h_

void display_msg(char *, char **);
void numbered_command(char *, int, char **);
int check_sync(int, char *, char *, char *, char *, struct channel *);

	/* Really in notice.c */
void got_initial_version(char **);

#endif				/* __numbers_h_ */
