/*
 * exec.c: handles exec'd process for IRCII 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#undef VERBOSE_KILL_MSG

#define IN_EXEC_C

#include <unistd.h>

#include "irc.h"
#include "dcc.h"
#include "exec.h"
#include "vars.h"
#include "ircaux.h"
#include "commands.h"
#include "window.h"
#include "screen.h"
#include "hook.h"
#include "input.h"
#include "list.h"
#include "server.h"
#include "output.h"
#include "parse.h"
#include "misc.h"
#include "newio.h"
#include "tcommand.h"

#include <sys/wait.h>

#ifdef HAVE_GETPGID
pid_t getpgid(pid_t pid);
#endif

static int exec_close (int);
void start_process (char *, char *, char *, char *, unsigned int, int);
void kill_process (int, int);
static int delete_process (int);
static void list_processes (void);
static int valid_process_index (int);
static void add_process (char *, char *, int, int, int, int, char *, char *, unsigned int);
static int is_logical_unique (char *);

/* Process: the structure that has all the info needed for each process */
typedef struct
{
	char *name;		/* full process name */
	char *logical;
	pid_t pid;		/* process-id of process */
	int p_stdin;		/* stdin description for process */
	int p_stdout;		/* stdout descriptor for process */
	int p_stderr;		/* stderr descriptor for process */
	int counter;		/* output line counter for process */
	char *redirect;		/* redirection command (MSG, NOTICE) */
	unsigned int refnum;	/* a window for output to go to */
	int server;		/* the server to use for output */
	char *who;		/* nickname used for redirection */
	int exited;		/* true if process has exited */
	int termsig;		/* The signal that terminated
				 * the process */
	int retcode;		/* return code of process */
	List *waitcmds;		/* commands queued by WAIT -CMD */
}
Process;

static Process **process_list = NULL;
static int process_list_size = 0;
static int wait_index = -1;

/*
 * A nice array of the possible signals.  Used by the coredump trapping
 * routines and in the exec.c package 
 */

#ifndef SYS_SIGLIST_DECLARED
#include "sig.inc"
#endif

/*
 * exec_close: silly, eh?  Well, it makes the code look nicer.  Or does it
 * really?  No.  But what the hell 
 */
static int 
exec_close (int des)
{

	if (des == -1)
		return (-1);
	new_close (des);
	if (FD_ISSET (des, &readables))
		FD_CLR (des, &readables);
	return (-1);
}

/*
 * set_wait_process: Sets the given index number so that it will be checked
 * for upon process exit.  This is used by waitcmd() in edit.c.  An index of
 * -1 disables this.
 */
void 
set_wait_process (int proccess)
{
	wait_index = proccess;
}

/*
 * valid_process_index: checks to see if index refers to a valid running
 * process and returns true if this is the case.  Returns false otherwise 
 */
static int 
valid_process_index (int proccess)
{
	if ((proccess < 0) || (proccess >= process_list_size))
		return (0);
	if (process_list[proccess])
		return (1);
	return (0);
}

int 
get_child_exit (int pid)
{
	return (check_wait_status (pid));
}

/*
 * check_wait_status: This waits on child processes, reporting terminations
 * as needed, etc 
 */
int 
check_wait_status (int wanted)
{
	Process *proc;
	int status;
	int pid, i;

	if ((pid = waitpid (wanted, &status, WNOHANG)) > 0)
	{
		if (wanted != -1 && pid == wanted)
		{
			if (WIFEXITED (status))
				return WEXITSTATUS (status);
			if (WIFSTOPPED (status))
				return -(WSTOPSIG (status));
			if (WIFSIGNALED (status))
				return -(WTERMSIG (status));
		}
		errno = 0;	/* reset errno, cause wait3 changes it */
		for (i = 0; i < process_list_size; i++)
		{
			if ((proc = process_list[i]) && proc->pid == pid)
			{
				proc->exited = 1;
				proc->termsig = WTERMSIG (status);
				proc->retcode = WEXITSTATUS (status);
				if ((proc->p_stderr == -1) &&
				    (proc->p_stdout == -1))
					delete_process (i);
				return 0;
			}
		}
	}
	return -1;
}

/*
 * check_process_limits: checks each running process to see if it's reached
 * the user selected maximum number of output lines.  If so, the processes is
 * effectively killed 
 */
void 
check_process_limits (void)
{
	int limit;
	int i;
	Process *proc;

	if ((limit = get_int_var (SHELL_LIMIT_VAR)) && process_list)
	{
		for (i = 0; i < process_list_size; i++)
		{
			if ((proc = process_list[i]) != NULL)
			{
				if (proc->counter >= limit)
				{
					proc->p_stdin = exec_close (proc->p_stdin);
					proc->p_stdout = exec_close (proc->p_stdout);
					proc->p_stderr = exec_close (proc->p_stderr);
					if (proc->exited)
						delete_process (i);
				}
			}
		}
	}
}

/*
 * do_processes: given a set of read-descriptors (as returned by a call to
 * select()), this will determine which of the process has produced output
 * and will hadle it appropriately 
 */
void 
do_processes (fd_set * rd)
{
	int i, flag;
	char exec_buffer[INPUT_BUFFER_SIZE + 1];
	char *ptr;
	Process *proc;
	int old_timeout;

	if (process_list == NULL)
		return;
	old_timeout = dgets_timeout (1);
	for (i = 0; i < process_list_size; i++)
	{
		if ((proc = process_list[i]) && proc->p_stdout != -1)
		{
			if (FD_ISSET (proc->p_stdout, rd))
			{

				/*
				 * This switch case indented back to allow for 80 columns,
				 * phone, jan 1993.
				 */
				switch (dgets (exec_buffer, INPUT_BUFFER_SIZE, proc->p_stdout, NULL))
				{
				case 0:
					if (proc->p_stderr == -1)
					{
						proc->p_stdin = exec_close (proc->p_stdin);
						proc->p_stdout = exec_close (proc->p_stdout);
						if (proc->exited)
							delete_process (i);
					}
					else
						proc->p_stdout = exec_close (proc->p_stdout);
					break;
				case -1:
					if (proc->logical)
						flag = do_hook (EXEC_PROMPT_LIST, "%s %s", proc->logical, exec_buffer);
					else
						flag = do_hook (EXEC_PROMPT_LIST, "%d %s", i, exec_buffer);
					set_prompt_by_refnum (proc->refnum, exec_buffer);
					update_input (UPDATE_ALL);
					/* if (flag == 0) */
					break;
				default:
					message_to (proc->refnum);
					proc->counter++;
					ptr = exec_buffer + strlen (exec_buffer) - 1;
					if ((*ptr == '\n') || (*ptr == '\r'))
					{
						*ptr = (char) 0;
						ptr = exec_buffer + strlen (exec_buffer) - 1;
						if ((*ptr == '\n') || (*ptr == '\r'))
							*ptr = (char) 0;
					}

					if (proc->logical)
						flag = do_hook (EXEC_LIST, "%s %s", proc->logical, exec_buffer);
					else
						flag = do_hook (EXEC_LIST, "%d %s", i, exec_buffer);

					if (flag)
					{
						if (proc->redirect)
						{
							int server;

							server = from_server;
							from_server = proc->server;
							send_text (proc->who, stripansicodes (exec_buffer), proc->redirect, 1, 1);
							from_server = server;
						}
						else
							put_it ("%s", stripansicodes (exec_buffer));
					}
					message_to (0);
					break;
				}

				/* end of special intendation */

			}
		}
		if (process_list && i < process_list_size &&
		    (proc = process_list[i]) && proc->p_stderr != -1)
		{
			if (FD_ISSET (proc->p_stderr, rd))
			{

				/* Do the intendation on this switch as well */

				switch (dgets (exec_buffer, INPUT_BUFFER_SIZE, proc->p_stderr, NULL))
				{
				case 0:
					if (proc->p_stdout == -1)
					{
						proc->p_stderr = exec_close (proc->p_stderr);
						proc->p_stdin = exec_close (proc->p_stdin);
						if (proc->exited)
							delete_process (i);
					}
					else
						proc->p_stderr = exec_close (proc->p_stderr);
					break;
				case -1:
					if (proc->logical)
						flag = do_hook (EXEC_PROMPT_LIST, "%s %s", proc->logical, exec_buffer);
					else
						flag = do_hook (EXEC_PROMPT_LIST, "%d %s", i, exec_buffer);
					set_prompt_by_refnum (proc->refnum, exec_buffer);
					update_input (UPDATE_ALL);
					if (flag == 0)
						break;
				default:
					message_to (proc->refnum);
					(proc->counter)++;
					ptr = exec_buffer + strlen (exec_buffer) - 1;
					if ((*ptr == '\n') || (*ptr == '\r'))
					{
						*ptr = (char) 0;
						ptr = exec_buffer + strlen (exec_buffer) - 1;
						if ((*ptr == '\n') || (*ptr == '\r'))
							*ptr = (char) 0;
					}

					if (proc->logical)
						flag = do_hook (EXEC_ERRORS_LIST, "%s %s", proc->logical, exec_buffer);
					else
						flag = do_hook (EXEC_ERRORS_LIST, "%d %s", i, exec_buffer);

					if (flag)
					{
						if (proc->redirect)
						{
							int server;

							server = from_server;
							from_server = proc->server;
							send_text (proc->who, stripansicodes (exec_buffer), process_list[i]->redirect, 1, 1);
							from_server = server;
						}
						else
							put_it ("%s", stripansicodes (exec_buffer));
					}
					message_to (0);
					break;
				}

				/* End of indentation for 80 columns */

			}
		}
	}

	check_process_limits ();
	(void) dgets_timeout (old_timeout);
}

/*
 * set_process_bits: This will set the bits in a fd_set map for each of the
 * process descriptors. 
 */
void 
set_process_bits (fd_set * rd)
{
	int i;
	Process *proc;

	if (process_list)
	{
		for (i = 0; i < process_list_size; i++)
		{
			if ((proc = process_list[i]) != NULL)
			{
				if (proc->p_stdout != -1)
					FD_SET (proc->p_stdout, rd);
				if (proc->p_stderr != -1)
					FD_SET (proc->p_stderr, rd);
			}
		}
	}
}

/*
 * list_processes: displays a list of all currently running processes,
 * including index number, pid, and process name 
 */
static void 
list_processes (void)
{
	Process *proc;
	int i;

	if (process_list)
	{
		say ("Process List:");
		for (i = 0; i < process_list_size; i++)
		{
			if ((proc = process_list[i]) != NULL)
			{
				if (proc->logical)
					say ("\t%d (%s): %s", i,
					     proc->logical,
					     proc->name);
				else
					say ("\t%d: %s", i,
					     proc->name);
			}
		}
	}
	else
		say ("No processes are running");
}

void 
add_process_wait (int proc_index, char *cmd)
{
	List *new, **posn;

	for (posn = &process_list[proc_index]->waitcmds; *posn != NULL; posn = &(*posn)->next)
		;
	new = (List *) new_malloc (sizeof (List));
	memset (new, 0, sizeof (List));
	*posn = new;
	malloc_strcpy (&new->name, cmd);
}

/*
 * delete_process: Removes the process specifed by index from the process
 * list.  The does not kill the process, close the descriptors, or any such
 * thing.  It only deletes it from the list.  If appropriate, this will also
 * shrink the list as needed 
 */
static int 
delete_process (int process)
{
	int flag;
	List *cmd, *next;

	if (process_list)
	{
		if (process >= process_list_size)
			return (-1);
		if (process_list[process])
		{
			Process *dead;

			dead = process_list[process];
			process_list[process] = NULL;
			if (process == (process_list_size - 1))
			{
				int i;

				for (i = process_list_size - 1;
				     process_list_size;
				     process_list_size--, i--)
				{
					if (process_list[i])
						break;
				}

				if (process_list_size)
					process_list = (Process **)
						RESIZE (process_list, Process *, process_list_size);
				else
				{
					new_free ((char **) &process_list);
					process_list = NULL;
				}
			}
			for (next = dead->waitcmds; next;)
			{
				cmd = next;
				next = next->next;
				parse_line (NULL, cmd->name, empty_string, 0, 0);
				new_free (&cmd->name);
				new_free (&dead->logical);
				new_free (&dead->who);
				new_free (&dead->redirect);
				new_free ((char **) &cmd);
			}
			dead->waitcmds = NULL;
			if (dead->logical)
				flag = do_hook (EXEC_EXIT_LIST, "%s %d %d",
						dead->logical, dead->termsig,
						dead->retcode);
			else
				flag = do_hook (EXEC_EXIT_LIST, "%d %d %d",
				     process, dead->termsig, dead->retcode);
			if (flag)
			{
				if (get_int_var (NOTIFY_ON_TERMINATION_VAR))
				{
					if (dead->termsig)
						say ("Process %d (%s) terminated with signal %s (%d)", process, dead->name, sys_siglist[dead->termsig],
						     dead->termsig);
					else
						say ("Process %d (%s) terminated with return code %d", process, dead->name, dead->retcode);
				}
			}
			new_free (&dead->name);
			new_free (&dead->logical);
			new_free (&dead->who);
			new_free (&dead->redirect);
			new_free ((char **) &dead);
			return (0);
		}
	}
	return (-1);
}

/*
 * add_process: adds a new process to the process list, expanding the list as
 * needed.  It will first try to add the process to a currently unused spot
 * in the process table before it increases it's size. 
 */
static void 
add_process (char *name, char *logical, int pid, int p_stdin, int p_stdout, int p_stderr, char *redirect, char *who, unsigned int refnum)
{
	int i;
	Process *proc;

	if (process_list == NULL)
	{
		process_list = (Process **) new_malloc (sizeof (Process *));
		process_list[0] = NULL;
		process_list_size = 1;
	}
	for (i = 0; i < process_list_size; i++)
	{
		if (!process_list[i])
		{
			proc = process_list[i] = (Process *) new_malloc (sizeof (Process));
			proc->name = m_strdup (name);
			proc->logical = m_strdup (logical);
			proc->pid = pid;
			proc->p_stdin = p_stdin;
			proc->p_stdout = p_stdout;
			proc->p_stderr = p_stderr;
			proc->refnum = refnum;
			if (redirect)
				malloc_strcpy (&(proc->redirect), redirect);
			proc->server = curr_scr_win->server;
			if (who)
				malloc_strcpy (&(process_list[i]->who), who);
			FD_SET (proc->p_stdout, &readables);
			FD_SET (proc->p_stderr, &readables);

			return;
		}
	}
	process_list_size++;
	process_list = (Process **) RESIZE (process_list, Process *, process_list_size);
	process_list[process_list_size - 1] = NULL;
	proc = process_list[i] = (Process *) new_malloc (sizeof (Process));
	proc->name = m_strdup (name);
	proc->logical = m_strdup (logical);

	proc->pid = pid;
	proc->p_stdin = p_stdin;
	proc->p_stdout = p_stdout;
	proc->p_stderr = p_stderr;
	proc->refnum = refnum;
	proc->redirect = NULL;
	if (redirect)
		malloc_strcpy (&(proc->redirect), redirect);
	proc->server = curr_scr_win->server;
	proc->counter = 0;
	proc->exited = 0;
	proc->termsig = 0;
	proc->retcode = 0;
	proc->who = NULL;
	proc->waitcmds = NULL;
	if (who)
		malloc_strcpy (&(proc->who), who);
	FD_SET (proc->p_stdout, &readables);
	FD_SET (proc->p_stderr, &readables);

	return;

}

/*
   * kill_process: sends the given signal to the process specified by the given
   * index into the process table.  After the signal is sent, the process is
   * deleted from the process table 
 */
void 
kill_process (int kill_index, int sig)
{
	if (process_list && (kill_index < process_list_size) && process_list[kill_index])
	{
		pid_t pgid;

		say ("Sending signal %s (%d) to process %d: %s", sys_siglist[sig], sig, kill_index, process_list[kill_index]->name);

#ifdef HAVE_GETPGID
		pgid = getpgid (process_list[kill_index]->pid);
#else
#ifndef GETPGRP_VOID
		pgid = getpgrp (process_list[kill_index]->pid);
#else
		pgid = process_list[kill_index]->pid;
#endif
#endif

#ifndef HAVE_KILLPG
#define killpg(pg, sig) kill(-(pg), (sig))
#endif

		/* The exec'd process should NOT be in our job control session. */
		if (pgid == getpid ())
		{
			yell ("--- exec'd process is in my job control session!  Something is very wrong");
			return;
		}

		killpg (pgid, sig);
		kill (process_list[kill_index]->pid, sig);
	}
	else
		say ("There is no process %d", kill_index);
}

static int 
is_logical_unique (char *logical)
{
	Process *proc;
	int i;

	if (logical)
	{
		for (i = 0; i < process_list_size; i++)
			if ((proc = process_list[i]) && proc->logical &&
			    (my_stricmp (proc->logical, logical) == 0))
				return 0;
	}
	return 1;
}

/*
   * start_process: Attempts to start the given process using the SHELL as set
   * by the user. 
 */
void 
start_process (char *name, char *logical, char *redirect, char *who, unsigned int refnum, int direct)
{
	int p0[2], p1[2], p2[2], pid, cnt;
	char *shell = NULL, *flag, *arg;
	char buffer[BIG_BUFFER_SIZE + 1];

	p0[0] = p0[1] = -1;
	p1[0] = p1[1] = -1;
	p2[0] = p2[1] = -1;
	if (pipe (p0) || pipe (p1) || pipe (p2))
	{
		say ("Unable to start new process: %s", strerror (errno));
		if (p0[0] != -1)
		{
			new_close (p0[0]);
			new_close (p0[1]);
		}
		if (p1[0] != -1)
		{
			new_close (p1[0]);
			new_close (p1[1]);
		}
		if (p2[0] != -1)
		{
			new_close (p2[0]);
			new_close (p2[1]);
		}
		return;
	}
	switch ((pid = fork ()))
	{
	case -1:
		say ("Couldn't start new process!");
		break;
	case 0:
		{
			setsid ();
			close_all_server ();
			close_all_dcc ();
			close_all_exec ();
			close_all_screen ();
			my_signal (SIGINT, (sigfunc *) SIG_IGN, 0);
			my_signal (SIGQUIT, (sigfunc *) SIG_DFL, 0);
			my_signal (SIGSEGV, (sigfunc *) SIG_DFL, 0);
			my_signal (SIGBUS, (sigfunc *) SIG_DFL, 0);
			dup2 (p0[0], 0);	/* stdin */
			dup2 (p1[1], 1);	/* stdout */
			dup2 (p2[1], 2);	/* stderr */
			new_close (p0[0]);
			new_close (p0[1]);
			new_close (p1[0]);
			new_close (p1[1]);
			new_close (p2[0]);
			new_close (p2[1]);

			shell = get_string_var (SHELL_VAR);
#if   0
			setenv ("TERM", "tty", 1);
#endif
			if (direct || !shell)
			{
				char **args = NULL;	/* = (char **) 0 */
				int max;

				cnt = 0;
				max = 5;
				args = (char **) new_malloc (sizeof (char *) * max);
				while ((arg = next_arg (name, &name)) != NULL)
				{
					if (cnt == max)
					{
						max += 5;
						RESIZE (args, char *, max);
					}
					args[cnt++] = arg;
				}
				args[cnt] = NULL;
				setuid (getuid ());	/* If we are setuid, set it back! */
				setgid (getgid ());
				execvp (args[0], args);
			}
			else
			{
				if ((flag = get_string_var (SHELL_FLAGS_VAR)) == NULL)
					flag = empty_string;
				setuid (getuid ());
				setgid (getgid ());
				execl (shell, shell, flag, name, NULL);
			}
			snprintf (buffer, BIG_BUFFER_SIZE, "%s Error starting shell \"%s\": %s\n", line_thing, shell,
				  strerror (errno));
			write (1, buffer, strlen (buffer));
			_exit (-1);
			break;
		}
	default:
		{
			new_close (p0[0]);
			new_close (p1[1]);
			new_close (p2[1]);
			p0[0] = p1[1] = p2[1] = -1;
			add_process (name, logical, pid, p0[1], p1[0], p2[0], redirect, who, refnum);
			break;
		}
	}
}

/*
   * text_to_process: sends the given text to the given process.  If the given
   * process index is not valid, an error is reported and 1 is returned.
   * Otherwise 0 is returned. 
   * Added show, to remove some bad recursion, phone, april 1993
 */
int 
text_to_process (int proc_index, const char *text, int show)
{
	int ref;
	Process *proc;

	if (valid_process_index (proc_index) == 0)
	{
		say ("No such process number %d", proc_index);
		return (1);
	}
	ref = process_list[proc_index]->refnum;
	proc = process_list[proc_index];
	message_to (ref);
	if (show)
		put_it ("%s%s", get_prompt_by_refnum (ref), text);	/* lynx */
	write (proc->p_stdin, text, strlen (text));
	write (proc->p_stdin, "\n", 1);
	set_prompt_by_refnum (ref, empty_string);
	message_to (0);
	return (0);
}

/*
   * is_process_running: Given an index, this returns true if the index referes
   * to a currently running process, 0 otherwise 
 */
int 
is_process_running (int proc_index)
{
	if (process_list && process_list[proc_index])
		return (!process_list[proc_index]->exited);
	return (0);
}

/*
   * lofical_to_index: converts a logical process name to it's approriate index
   * in the process list, or -1 if not found 
 */
int 
logical_to_index (char *logical)
{
	Process *proc;
	int i;

	for (i = 0; i < process_list_size; i++)
	{
		if ((proc = process_list[i]) && proc->logical &&
		    (!my_stricmp (proc->logical, logical)))
			return i;
	}
	return -1;
}

/*
   * get_process_index: parses out a process index or logical name from the
   * given string 
 */
int 
get_process_index (char **args)
{
	char *s;

	if ((s = next_arg (*args, args)) != NULL)
	{
		if (*s == '%')
			s++;
		else
			return (-1);
		if (is_number (s))
			return (my_atol (s));
		else
			return (logical_to_index (s));
	}
	else
		return (-1);
}

/* is_process: checks to see if arg is a valid process specification */
int 
is_process (char *arg)
{
	if (arg && *arg == '%')
	{
		arg++;
		if (is_number (arg) || (logical_to_index (arg) != -1))
			return (1);
	}
	return (0);
}

extern int in_cparse;

/*
 * exec: the /EXEC command.  Handles the whole IRCII process crap. 
 */
void
cmd_exec (struct command *cmd, char *args)
{
#ifdef EXEC_COMMAND
	char *who = NULL, *logical = NULL, *redirect = NULL,	/* = (char *) 0, */
	 *flag;
	unsigned int refnum = 0;
	int sig, len, i, direct = 0, refnum_flag = 0, logical_flag = 0;
	Process *proc;

	if (in_cparse)
		return;
	if (get_int_var (EXEC_PROTECTION_VAR) && (current_on_hook != -1))
	{
		say ("Attempt to use EXEC from within an ON function!");
		say ("Command \"%s\" not executed!", args);
		say ("Please read /HELP SET EXEC_PROTECTION");
		say ("for important details about this!");
		return;
	}
	if (!args || !*args)
	{
		list_processes ();
		return;
	}
	redirect = NULL;
	while ((*args == '-') && (flag = next_arg (args, &args)))
	{
		if (*flag == '-')
		{
			len = strlen (++flag);
			if (my_strnicmp (flag, "OUT", len) == 0)
			{
				redirect = "PRIVMSG";
				if (doing_privmsg)
					redirect = "NOTICE";
				else
					redirect = "PRIVMSG";
				if (!(who = get_current_channel_by_refnum (0)))
				{
					say ("No current channel in this window for -OUT");
					return;
				}
			}
			else if (my_strnicmp (flag, "NAME", len) == 0)
			{
				logical_flag = 1;
				if ((logical = next_arg (args, &args)) == NULL)
				{
					say ("You must specify a logical name");
					return;
				}
			}
			else if (my_strnicmp (flag, "WINDOW", len) == 0)
			{
				refnum_flag = 1;
				refnum = current_refnum ();
			}
			else if (my_strnicmp (flag, "MSG", len) == 0)
			{
				if (doing_privmsg)
					redirect = "NOTICE";
				else
					redirect = "PRIVMSG";
				if ((who = next_arg (args, &args)) == NULL)
				{
					say ("No nicknames specified");
					return;
				}
			}
			else if (my_strnicmp (flag, "CLOSE", len) == 0)
			{
				if ((i = get_process_index (&args)) == -1)
				{
					say ("Missing process number or logical name.");
					return;
				}
				if (is_process_running (i))
				{
					proc = process_list[i];
					proc->p_stdin = exec_close (proc->p_stdin);
					proc->p_stdout = exec_close (proc->p_stdout);
					proc->p_stderr = exec_close (proc->p_stderr);
				}
				else
					say ("No such process running!");
				return;
			}
			else if (my_strnicmp (flag, "DIRECT", len) == 0)
				direct = 1;

			else if (my_strnicmp (flag, "NOTICE", len) == 0)
			{
				redirect = "NOTICE";
				if ((who = next_arg (args, &args)) == NULL)
				{
					say ("No nicknames specified");
					return;
				}
			}
			else if (my_strnicmp (flag, "IN", len) == 0)
			{
				if ((i = get_process_index (&args)) == -1)
				{
					say ("Missing process number or logical name.");
					return;
				}
				text_to_process (i, args, 1);
				return;
			}
			else
			{
				if ((i = get_process_index (&args)) == -1)
				{
					say ("Invalid process specification");
					return;
				}
				if ((sig = my_atol (flag)) > 0)
				{
					if ((sig > 0) && (sig < NSIG))
						kill_process (i, sig);
					else
						say ("Signal number can be from 1 to %d", NSIG);
					return;
				}
				for (sig = 1; sig < NSIG; sig++)
				{
					if (!my_strnicmp (sys_siglist[sig], flag, len))
					{
						kill_process (i, sig);
						return;
					}
				}
				say ("No such signal: %s", flag);
				return;
			}
		}
		else
			break;
	}
	if (is_process (args))
	{
		if ((i = get_process_index (&args)) == -1)
		{
			say ("Invalid process specification");
			return;
		}
		if (valid_process_index (i))
		{
			proc = process_list[i];
			message_to (refnum);
			if (refnum_flag)
			{
				proc->refnum = refnum;
				if (refnum)
					say ("Output from process %d (%s) now going to this window", i, proc->name);
				else
					say ("Output from process %d (%s) not going to any window", i, proc->name);
			}
			malloc_strcpy (&(proc->redirect), redirect);
			malloc_strcpy (&(proc->who), who);

			if (redirect)
				say ("Output from process %d (%s) now going to %s", i, proc->name, who);
			else
				say ("Output from process %d (%s) now going to you", i, proc->name);

			if (logical_flag)
			{
				if (logical)
				{
					if (is_logical_unique (logical))
					{
						malloc_strcpy (&(proc->logical), logical);
						say ("Process %d (%s) is now called %s",
						     i, proc->name, proc->logical);
					}
					else
						say ("The name %s is not unique!", logical);
				}
				else
					say ("The name for process %d (%s) has been removed", i, proc->name);
			}
			message_to (0);
		}
		else
			say ("Invalid process specification");
	}
	else
	{
		if (is_logical_unique (logical))
			start_process (args, logical, redirect, who, refnum, direct);
		else
			say ("The name %s is not unique!", logical);
	}
#else
	bitchsay ("/EXEC command has been disabled.");
#endif
}

/*
 * clean_up_processes: kills all processes in the procss list by first
 * sending a SIGTERM, then a SIGKILL to clean things up 
 */
void 
clean_up_processes (void)
{
	int i;

	if (process_list_size)
	{
		say ("Cleaning up left over processes....");
		for (i = 0; i < process_list_size; i++)
		{
			if (process_list[i])
				kill_process (i, SIGTERM);
		}
		sleep (2);	/* Give them a chance for a graceful exit */
		for (i = 0; i < process_list_size; i++)
		{
			if (process_list[i])
				kill_process (i, SIGKILL);
		}
	}
}

/*
 * close_all_exec:  called when we fork of a wserv process for interaction
 * with screen/X, to close all unnessicary fd's that might cause problems
 * later.
 */
void 
close_all_exec (void)
{
	int i;
	int tmp;

	tmp = window_display;
	window_display = 0;
	for (i = 0; i < process_list_size; i++)
	{
		if (process_list[i])
		{
			if (process_list[i]->p_stdin)
				process_list[i]->p_stdin = exec_close (process_list[i]->p_stdin);
			if (process_list[i]->p_stdout)
				process_list[i]->p_stdout = exec_close (process_list[i]->p_stdout);
			if (process_list[i]->p_stderr)
				process_list[i]->p_stderr = exec_close (process_list[i]->p_stderr);
			delete_process (i);
			kill_process (i, SIGKILL);
		}
	}
	window_display = tmp;
}

void 
exec_server_delete (int i)
{
	int j;

	for (j = 0; j < process_list_size; j++)
		if (process_list[j] && process_list[j]->server >= i)
			process_list[j]->server--;
}
