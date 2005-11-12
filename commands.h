/*
 * edit.h: header for edit.c 
 *
 */

#ifndef __edit_h_
#define __edit_h_

#include "irc_std.h"

extern	char	*sent_nick;
extern	char	*sent_body;
extern	char	*recv_nick;

	void	send_text(char *, char *, char *, int, int);
	void	eval_inputlist(char *, char *);
	int	parse_command(char *, int, char *);
	void	parse_line(char *, char *, char *, int, int);
	void	edit_char(unsigned char);
	void	execute_timer(void);
	void	ison_now(char *, char *);
	void	quote_char(char, char *);
	void	type_text(char, char *);
	void	parse_text(char, char *);
	int	check_wait_command(char *);
	void	ExecuteTimers(void);
	void load (char *command, char *args, char *subargs, char *helparg);
	

		
#define AWAY_ONE 0
#define AWAY_ALL 1

#define TRACE_OPER	0x01
#define TRACE_SERVER	0x02
#define TRACE_USER	0x04

#define STATS_LINK	0x001
#define STATS_CLASS	0x002
#define STATS_ILINE	0x004
#define STATS_KLINE	0x008
#define STATS_YLINE	0x010
#define STATS_OLINE	0x020
#define STATS_HLINE	0x040
#define STATS_UPTIME	0x080
#define STATS_MLINE	0x100

#endif /* __edit_h_ */
