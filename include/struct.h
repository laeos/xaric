/*
 * struct.h: header file for structures needed for prototypes
 *
 * Written by Scott Reynolds, based on code by Michael Sandrof
 * Heavily modified by Colten Edwards for BitchX
 *
 * Copyright(c) 1997
 *
 */

#ifndef __struct_h_
#define	__struct_h_


#include "hash.h"

typedef struct 
{
	int is_read;
	int is_write;
	long flags;
	void (*func)(int);
} SocketList;


/* IrcCommand: structure for each command in the command table */
typedef	struct
{
	char	*name;					/* what the user types */
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func)(char *, char *, char *, char *);	/* function that is the command */
	unsigned	flags;
	char	*help;
}	IrcCommand;


typedef struct _last_msg_stru
{
	struct _last_msg_stru *next;
	char    *from;
	char	*uh;
	char	*to;
	char    *last_msg;
	char	*time;
	int     count;
} LastMsg;
                                
/* NotifyList: structure for the users on your Notify list */
typedef struct  mynotifylist_stru
{
	struct	mynotifylist_stru	*next;	/* pointer to next notify entry */
	char    *nick;				/* Notify Nick */
}	MyNotifyList;

/* invitetoList: structure for the invitetolist list */
typedef struct  invitetolist_stru
{
	struct	invitetolistlist_stru	*next;	/* pointer to next entry */
	char    *channel;			/* channel */
	int     times;				/* times I have been invited */
	time_t  time;				/* time of last invite */
}	InviteToList;

typedef struct server_split
{
	struct server_split *next;
	char *name;	/* name of this server. */
	char *link;	/* linked to what server */
	int status;	/* split or not */
	int count;	/* number of times we have not found this one */
	int hopcount; 	/* number of hops away */
	char *time; 	/* time of split */
} irc_server;

/*
 * ctcp_entry: the format for each ctcp function.   note that the function
 * described takes 4 parameters, a pointer to the ctcp entry, who the message
 * was from, who the message was to (nickname, channel, etc), and the rest of
 * the ctcp message.  it can return null, or it can return a malloced string
 * that will be inserted into the oringal message at the point of the ctcp.
 * if null is returned, nothing is added to the original message

 */
struct _CtcpEntry;

typedef char *(*CTCP_Handler)(struct _CtcpEntry *, char *, char *, char *);

typedef	struct _CtcpEntry
{
	char		*name;  /* name of ctcp datatag */
	int		id;	/* index of this ctcp command */
	int		flag;	/* Action modifiers */
	char		*desc;  /* description returned by ctcp clientinfo */
	CTCP_Handler 	func;	/* function that does the dirty deed */
	CTCP_Handler 	repl;	/* Function that is called for reply */
}	CtcpEntry;


struct transfer_struct {
	unsigned short packet_id;
	unsigned char byteorder;
	u_32int_t byteoffset;
}; 


typedef struct _File_Stat {
	struct _File_Stat *next;
	char *filename;
	long filesize;
} FileStat;

typedef struct _File_List {
	struct _File_List *next;
	char * description;
	char * notes;
	FileStat *filename;
	char * nick;
	int packnumber;
	int numberfiles;
	double filesize;
	double minspeed;
	int gets;
	time_t timequeue;
} FileList;

typedef	struct	DCC_struct
{
	struct DCC_struct *next;
	char		*user;
	char		*userhost;
	unsigned int	flags;
	int		read;
	int		write;
	int		file;

	u_32int_t	filesize;

	int		dccnum;
	int		eof;
	char		*description;
	char		*othername;
	struct in_addr	remote;
	u_short		remport;
	u_32int_t	bytes_read;
	u_32int_t	bytes_sent;

	int				window_sent;
	int				window_max;

	int		in_dcc_chat;
	int		echo;
	struct timeval	lasttime;
	struct timeval	starttime;
	char		*buffer;
	char		*cksum;
	u_32int_t	packets_total;
	u_32int_t	packets_transfer;
	struct transfer_struct transfer_orders;
	void (*dcc_handler)(struct DCC_struct *, char *);

}	DCC_list;

/* Hold: your general doubly-linked list type structure */

typedef struct HoldStru
{
	char	*str;
	struct	HoldStru	*next;
	struct	HoldStru	*prev;
	int	logged;
}	Hold;

typedef struct	lastlog_stru
{
	int	level;
	char	*msg;
	time_t	time;
	struct	lastlog_stru	*next;
	struct	lastlog_stru	*prev;
}	Lastlog;

struct	ScreenStru;	/* ooh! */


/* NickList: structure for the list of nicknames of people on a channel */
typedef struct nick_stru
{
	struct	nick_stru	*next;	/* pointer to next nickname entry */
	char	*nick;			/* nickname of person on channel */
	char	*host;

	int	chanop;			/* True if the given nick has chanop */
	int	away;
	int	voice;
	int	ircop;
	
	time_t	idle_time;

	int	floodcount;
	time_t	floodtime;

	int	nickcount;
	time_t  nicktime;

	int	kickcount;
	time_t	kicktime;

	int	joincount;
	time_t	jointime;

	int	dopcount;
	time_t	doptime;

	int	bancount;
	time_t	bantime;


	time_t	created;

	int	stat_kicks;		/* Total kicks done by user */
	int	stat_dops;		/* Total deops done by user */
	int	stat_ops;		/* Total ops done by user */
	int	stat_bans;		/* Total bans done by user */
	int	stat_unbans;		/* Total unbans done by user */
	int	stat_nicks;		/* Total nicks done by user */
	int	stat_pub;		/* Total publics sent by user */
	int	stat_topics;		/* Total topics set by user */

	int	sent_reop;
	time_t	sent_reop_time;
	
	int	sent_deop;
	time_t	sent_deop_time;
	int	need_userhost;		/* on join we send a userhost for this nick */	
}	NickList;

typedef	struct	DisplayStru
{
	char	*line;
	int	linetype;
	struct	DisplayStru	*next;
	struct	DisplayStru	*prev;
}	Display;

typedef	struct	WindowStru
{
	char			*name;
	unsigned int	refnum;		/* the unique reference number,
					 * assigned by IRCII */
	int	server;			/* server index */
	int	top;			/* The top line of the window, screen
					 * coordinates */
	int	bottom;			/* The botton line of the window, screen
					 * coordinates */
	int	cursor;			/* The cursor position in the window, window
					 * relative coordinates */
	int	line_cnt;		/* counter of number of lines displayed in
					 * window */
	int	absolute_size;
	int	scroll;			/* true, window scrolls... false window wraps */
	int	old_size;		/* if new_size != display_size, resize_display */
	int	visible;		/* true, window ise, window is drawn... false window is hidden */
	int	update;			/* window needs updating flag */
	unsigned miscflags;		/* Miscellaneous flags. */
	int	beep_always;		/* should this window beep when hidden */
	unsigned long notify_level;
	int	window_level;		/* The LEVEL of the window, determines what
					 * messages go to it */
	int	skip;
	
	char	*prompt;		/* A prompt string, usually set by EXEC'd process */

	/* Real status stuff */
	int	double_status;		/* number of status lines */
	char	*status_line[3];	/* The status lines string current display */

	Display *top_of_scrollback,	/* Start of the scrollback buffer */
		*top_of_display,	/* Where the viewport starts */
		*display_ip,		/* Where next line goes in rite() */
		*scrollback_point;	/* Where t_o_d was at start of sb */
	int	display_buffer_size;	/* How big the scrollback buffer is */
	int	display_buffer_max;	/* How big its supposed to be */
	int	display_size;		/* How big the window is - status */

	int	lines_scrolled_back;	/* Where window is relatively */

	int	hold_mode;		/* True if we want to hold stuff */
	int	holding_something;	/* True if we ARE holding something */
	int	held_displayed;		/* lines displayed since last hold */
	int	lines_displayed;	/* Lines held since last unhold */
	int	lines_held;		/* Lines currently being held */
	int	last_lines_held;	/* Last time we updated "lines held" */
	int	distance_from_display;

	char	*current_channel;	/* Window's current channel */
	char	*waiting_channel;
	char	*bind_channel;
	char	*query_nick;		/* User being QUERY'ied in this window */
	char	*query_host;
	char	*query_cmd;
		
	NickList *nicks;		/* List of nicks that will go to window */


	/* lastlog stuff */
	Lastlog	*lastlog_head;		/* pointer to top of lastlog list */
	Lastlog	*lastlog_tail;		/* pointer to bottom of lastlog list */
	unsigned long lastlog_level;	/* The LASTLOG_LEVEL, determines what
					 * messages go to lastlog */
	int	lastlog_size;		/* number of messages in lastlog */
	int	lastlog_max;		/* Max number of msgs in lastlog */
	
	char	*logfile;		/* window's logfile name */
	/* window log stuff */
	int	log;			/* true, file logging for window is on */
	FILE	*log_fp;		/* file pointer for the log file */

	int	window_display;		/* should we display to this window */

	struct	ScreenStru	*screen;
	struct	WindowStru	*next;	/* pointer to next entry in window list (null
					 * is end) */
	struct	WindowStru	*prev;	/* pointer to previous entry in window list
					 * (null is end) */
}	Window;

/*
 * WindowStack: The structure for the POP, PUSH, and STACK functions. A
 * simple linked list with window refnums as the data 
 */
typedef	struct	window_stack_stru
{
	unsigned int	refnum;
	struct	window_stack_stru	*next;
}	WindowStack;

typedef	struct
{
	int	top;
	int	bottom;
	int	position;
}	ShrinkInfo;

typedef struct PromptStru
{
	char	*prompt;
	char	*data;
	int	type;
	void	(*func)(char *, char *);
	struct	PromptStru	*next;
}	WaitPrompt;


typedef	struct	ScreenStru
{
	int	screennum;
	Window	*current_window;
	unsigned int	last_window_refnum;	/* reference number of the
						 * window that was last
						 * the current_window */
	Window	*window_list;			/* List of all visible
						 * windows */
	Window	*window_list_end;		/* end of visible window
						 * list */
	Window	*cursor_window;			/* Last window to have
						 * something written to it */
	int	visible_windows;		/* total number of windows */
	WindowStack	*window_stack;		/* the windows here */

	struct	ScreenStru *prev;		/* These are the Screen list */
	struct	ScreenStru *next;		/* pointers */


	FILE	*fpin;				/* These are the file pointers */
	int	fdin;				/* and descriptions for the */
	FILE	*fpout;				/* screen's input/output */
	int	fdout;

	char	input_buffer[INPUT_BUFFER_SIZE+2];	/* the input buffer */
	int	buffer_pos;			/* and the positions for the */
	int	buffer_min_pos;			/* screen */

	char	saved_input_buffer[INPUT_BUFFER_SIZE+2];
	int	saved_buffer_pos;
	int	saved_min_buffer_pos;

	WaitPrompt	*promptlist;



	int	meta_hit[10];
	int	quote_hit;			/* true if a key bound to
						 * QUOTE_CHARACTER has been
						 * hit. */
	int	digraph_hit;			/* A digraph key has been hit */
	unsigned char	digraph_first;


	char	*tty_name;
	int	co;
	int	li;

	int	alive;
}	Screen;

/* BanList: structure for the list of bans on a channel */
typedef struct ban_stru
{
	struct	ban_stru	*next;  /* pointer to next ban entry */
	char	*ban;			/* the ban */
	char	*setby;			/* person who set the ban */
	int	sent_unban;		/* flag if sent unban or not */
	time_t	sent_unban_time;	/* sent unban's time */
	time_t	time;			/* time ban was set */
	int	count;
}	BanList;


typedef struct chan_flags_stru {

	unsigned int got_modes : 1;
	unsigned int got_who : 1;
	unsigned int got_bans : 1;

} chan_flags;

/* ChannelList: structure for the list of channels you are current on */
typedef	struct	channel_stru
{
	struct	channel_stru	*next;	/* pointer to next channel entry */
	char	*channel;		/* channel name */
	int	server;			/* server index for this channel */
	u_long	mode;			/* Current mode settings for channel */
	u_long	i_mode;			/* channel mode for cached string */
	char	*s_mode;		/* cached string version of modes */
	char	*topic;
		
	int	limit;			/* max users for the channel */
	char	*key;			/* key for this channel */
	char	chop;			/* true if you are chop */
	char	voice;			/* true if you are voice */
	char	bound;			/* true if channel is bound */
	char	connected;		/* true if this channel is actually connected */

	Window	*window;		/* the window that the channel is "on" */

	HashEntry	NickListTable[NICKLIST_HASHSIZE];

	chan_flags	flags;


	int	tog_limit;
	int	do_scan;		/* flag for checking auto stuff */
	struct timeval	channel_create;		/* time for channel creation */
	struct timeval	join_time;		/* time of last join */


	int	stats_ops;		/* total ops I have seen in channel */
	int	stats_dops;		/* total dops I have seen in channel */
	int	stats_bans;		/* total bans I have seen in channel */
	int	stats_unbans;		/* total unbans I have seen in channel */

	int	stats_sops;		/* total server ops I have seen in channel */
	int	stats_sdops;		/* total server dops I have seen in channel */
	int	stats_sbans;		/* total server bans I have seen in channel */
	int	stats_sunbans;		/* total server unbans I have seen in channel */

	int	stats_topics;		/* total topics I have seen in channel */
	int	stats_kicks;		/* total kicks I have seen in channel */
	int	stats_pubs;		/* total pubs I have seen in channel */
	int	stats_parts;		/* total parts I have seen in channel */
	int	stats_signoffs;		/* total signoffs I have seen in channel */
	int	stats_joins;		/* total joins I have seen in channel */

	int	msglog_on;
	FILE	*msglog_fp;
	char	*logfile;
	
		
	int	totalnicks;		/* total number of users in channel */
	int	maxnicks;		/* max number of users I have seen */
	time_t	maxnickstime;		/* time of max users */

	int	totalbans;		/* total numbers of bans on channel */


	BanList	*bans;			/* pointer to list of bans on channel */
	int	maxbans;		/* max number of bans I have seen */
	time_t	maxbanstime;		/* time of max bans */
	struct {
		char 	*op;
		int	type;
	} cmode[4];

	char	*mode_buf;
	int	mode_len;	

}	ChannelList;

typedef	struct	list_stru
{
	struct	list_stru	*next;
	char	*name;
}	List;

typedef struct	flood_stru
{
	struct flood_stru	*next;
	char   name[31];
	char	*channel;
	int	type;
	char	flood;
	unsigned long	cnt;
	time_t	start;
}	Flooding;

/* a structure for the timer list */
typedef struct	timerlist_stru
{
	char	ref[REFNUM_MAX + 1];
	unsigned long refno;
	int	in_on_who;
	time_t	time;
	int	(*callback)(void *);
	char	*command;
	char	*subargs;
	int	events;
	time_t	interval;
	struct	timerlist_stru *next;
}	TimerList;

extern TimerList *PendingTimers;
typedef struct nicktab_stru
{
	struct nicktab_stru *next;
	char *nick;
	char *type;
} NickTab;

typedef struct clonelist_stru
{
	struct clonelist_stru *next;
	char *number;
	char *server;
	int  port;
	int  socket_num;
	int  warn;
} CloneList;

typedef struct IgnoreStru
{
	struct IgnoreStru *next;
	char *nick;
	long type;
	long dont;
	long high;
	long cgrep;
	int num;
	char *pre_msg_high;
	char *pre_nick_high;
	char *post_high;
	struct IgnoreStru *looking;
	struct IgnoreStru *except;
} Ignore;

/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
	char	*name;			/* what the user types */
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
	void	(*func)(Window *, char *, int);		/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariable;



#endif /* __struct_h_ */
