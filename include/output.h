/*
 * output.h: header for output.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __output_h_
#define __output_h_

#include "fset.h"

	void	put_echo (char *);
	void	put_it (const char *, ...);
	void	send_to_server (const char *, ...);
	void	my_send_to_server (int, const char *, ...);
	void	say (const char *, ...);
	void	bitchsay (const char *, ...);
	void	serversay (int, int, const char *, ...);
	void	yell (const char *, ...);
	void	help_put_it (const char *, const char *, ...);

	void	refresh_screen(unsigned char, char *);
	int	init_screen(void);
	void	put_file(char *);

	void	put_fmt (xformat, const char *, ...);
	void	log_put_it (const char *topic, const char *format, ...);
	void	charset_ibmpc(void);
	void	charset_lat1(void);
	void	charset_graf(void);

extern	FILE	*irclog_fp;

#endif /* __output_h_ */
