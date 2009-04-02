/*
 * hook.h.proto: header for hook.c
 * 
 * Generated from hook.h.proto automatically by the Makefile
 *
 * @(#)$Id$
 */

#ifndef __hook_h_
# define __hook_h_

/* Hook: The structure of the entries of the hook functions lists */
typedef struct hook_stru {
    struct hook_stru *next;	/* pointer to next element in list */
    char *nick;			/* The Nickname */
    int not;			/* If true, this entry should be ignored when matched, otherwise it is a normal entry */
    int noisy;			/* flag indicating how much output should be given */
    int server;			/* the server in which this hook applies. (-1 if none). If bit 0x1000 is set, then no other hooks are
				 * tried in the given server if all the server specific ones fail */
    int sernum;			/* The serial number for this hook. This is used for hooks which will be concurrent with others of the 
				 * same pattern. The default is 0, which means, of course, no special behaviour. If any 1 hook
				 * suppresses the * default output, output will be suppressed. */
    char *stuff;		/* The this that gets done */
    int global;			/* set if loaded from `global' */
    int flexible;
} Hook;

/* HookFunc: A little structure to keep track of the various hook functions */
typedef struct {
    const char *name;		/* name of the function */
    Hook *list;			/* pointer to head of the list for this function */
    int params;			/* number of parameters expected */
    int mark;
    unsigned flags;
} HookFunc;

/*
 * NumericList: a special list type to dynamically handle numeric hook
 * requests 
 */
typedef struct numericlist_stru {
    struct numericlist_stru *next;
    char *name;
    Hook *list;
} NumericList;

enum HOOK_TYPES {
    ACTION_LIST,
    AR_PUBLIC_LIST,
    AR_PUBLIC_OTHER_LIST,
    AR_REPLY_LIST,
    BANS_LIST,
    BANS_HEADER_LIST,
    CHANNEL_NICK_LIST,
    CHANNEL_SIGNOFF_LIST,
    CHANNEL_STATS_LIST,
    CHANNEL_SWITCH_LIST,
    CHANNEL_SYNCH_LIST,
    CLONE_READ_LIST,
    CONNECT_LIST,
    CTCP_LIST,
    CTCP_REPLY_LIST,
    DCC_CHAT_LIST,
    DCC_CONNECT_LIST,
    DCC_ERROR_LIST,
    DCC_HEADER_LIST,
    DCC_LOST_LIST,
    DCC_POST_LIST,
    DCC_RAW_LIST,
    DCC_REQUEST_LIST,
    DCC_STAT_LIST,
    DCC_STATF_LIST,
    DCC_STATF1_LIST,
    DESYNC_MESSAGE_LIST,
    DISCONNECT_LIST,
    ENCRYPTED_NOTICE_LIST,
    ENCRYPTED_PRIVMSG_LIST,
    EXEC_LIST,
    EXEC_ERRORS_LIST,
    EXEC_EXIT_LIST,
    EXEC_PROMPT_LIST,
    EXIT_LIST,
    HELP_LIST,
    HELPSUBJECT_LIST,
    HELPTOPIC_LIST,
    HOOK_LIST,
    IDLE_LIST,
    INPUT_LIST,
    INVITE_LIST,
    JOIN_LIST,
    JOIN_ME_LIST,
    KICK_LIST,
    LEAVE_LIST,
    LIST_LIST,
    LLOOK_ADDED_LIST,
    LLOOK_JOIN_LIST,
    LLOOK_SPLIT_LIST,
    MODE_LIST,
    MODE_STRIPPED_LIST,
    MSG_LIST,
    MSG_GROUP_LIST,
    MSGLOG_LIST,
    NAMES_LIST,
    NICKNAME_LIST,
    NOTE_LIST,
    NOTICE_LIST,
    NOTIFY_SIGNOFF_LIST,
    NOTIFY_SIGNOFF_UH_LIST,
    NOTIFY_SIGNON_LIST,
    NOTIFY_SIGNON_UH_LIST,
    NSLOOKUP_LIST,
    ODD_SERVER_STUFF_LIST,
    PUBLIC_LIST,
    PUBLIC_MSG_LIST,
    PUBLIC_NOTICE_LIST,
    PUBLIC_OTHER_LIST,
    RAW_IRC_LIST,
    SAVEFILE_LIST,
    SAVEFILEPOST_LIST,
    SAVEFILEPRE_LIST,
    SEND_ACTION_LIST,
    SEND_DCC_CHAT_LIST,
    SEND_MSG_LIST,
    SEND_NOTICE_LIST,
    SEND_PUBLIC_LIST,
    SEND_TO_SERVER_LIST,
    SERVER_NOTICE_FAKES_LIST,
    SERVER_NOTICE_FAKES_MYCHANNEL_LIST,
    SERVER_NOTICE_FOREIGN_KILL_LIST,
    SERVER_NOTICE_KILL_LIST,
    SERVER_NOTICE_LIST,
    SERVER_NOTICE_LOCAL_KILL_LIST,
    SERVER_NOTICE_SERVER_KILL_LIST,
    SET_LIST,
    SHOWIDLE_HEADER_LIST,
    SHOWIDLE_LIST,
    SIGNOFF_LIST,
    SILENCE_LIST,
    STAT_LIST,
    STAT_HEADER_LIST,
    STATUS_UPDATE_LIST,
    TIMER_LIST,
    TOPIC_LIST,
    USAGE_LIST,
    USERS_LIST,
    USERS_HEADER_LIST,
    USERS_SERVER_LIST,
    USERS_SERVER_HEADER_LIST,
    WALL_LIST,
    WALLOP_LIST,
    WHO_LIST,
    WHOLEFT_LIST,
    WHOLEFT_HEADER_LIST,
    WIDELIST_LIST,
    WINDOW_LIST,
    WINDOW_KILL_LIST,
    YELL_LIST
};

#define NUMBER_OF_LISTS WINDOW_KILL_LIST + 1

int do_hook(int, const char *, ...) __A(2);
void oncmd(char *, char *, char *, char *);
void shook(char *, char *, char *, char *);
void save_hooks(FILE *, int);
void remove_hook(int, char *, int, int, int);
void show_hook(Hook *, char *);
void flush_on_hooks(void);

extern char *hook_info;
extern NumericList *numeric_list;

extern int in_on_who;

#endif				/* __hook_h_ */
