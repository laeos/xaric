/*
 * exec.h: header for exec.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef __exec_h_
#define __exec_h_

#include "irc_std.h"
#include <sys/types.h>

	int	get_child_exit(int);
	int	check_wait_status(int);
	void	check_process_limits(void);
	void	do_processes(fd_set *);
	void	set_process_bits(fd_set *);
	int	text_to_process(int, const char *, int);
	void	clean_up_processes(void);
	int	is_process(char *);
	int	get_process_index(char **);
	void	exec_server_delete(int);
	int	is_process_running(int);
	void	add_process_wait(int, char *);
	void	set_wait_process(int);
	void	close_all_exec(void);
	int	logical_to_index(char *);
	void	execcmd(char *, char *, char *, char *);
	void	start_process(char *, char *, char *, char *, unsigned int, int);
	void	kill_process(int, int);
                
	
#endif /* __exec_h_ */
