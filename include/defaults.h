/* 'new' config.h:
 *	A configuration file designed to make best use of the abilities
 *	of ircII, and trying to make things more intuitively understandable.
 *
 * Done by Carl v. Loesch <lynx@dm.unirm1.it>
 * Based on the 'classic' config.h by Michael Sandrof.
 * Copyright(c) 1991 - See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Warning!  You will most likely have to make changes to your .ircrc file to
 * use this version of IRCII!  Please read the INSTALL and New2.2 files
 * supplied with the distribution for details!
 *
 * @(#)$Id$
 */

#ifndef __config_h_
#define __config_h_

#include "build.h"  /* for path to help files */

/*
 * Set your favorite default server list here.  This list should be a
 * whitespace separated hostname:portnum:password list (with portnums and
 * passwords optional).  This IS NOT an optional definition. Please set this
 * to your nearest servers.  However if you use a seperate 'ircII.servers'
 * file and the ircII can find it, this setting is overridden.
 */

#ifndef DEFAULT_SERVER
#define DEFAULT_SERVER	    "ircd.grateful.org:6667"
#endif


/*
 * Define this to be the number of seconds that you want the client
 * to block on a server-connection.  I found it annoying that it was
 * hard coded to 15 seconds, as that was never long enough to make
 * a connection.  Change it back if you want.
 *
 * This is ONLY for /server and 'server-like' connections, not for DCC.
 */
#define CONNECT_TIMEOUT 20


/*
 * Set the following to 1 if you wish for IRCII not to disturb the tty's flow
 * control characters as the default.  Normally, these are ^Q and ^S.  You
 * may have to rebind them in IRCII.  Set it to 0 for IRCII to take over the
 * tty's flow control.
 */
#define USE_FLOW_CONTROL 1

/*
 * Uncomment the following if the gecos field of your /etc/passwd has other
 * information in it that you don't want as the user name (such as office
 * locations, phone numbers, etc).  The default delimiter is a comma, change
 * it if you need to. If commented out, the entire gecos field is used. 
 */
#define GECOS_DELIMITER ','


/* Thanks to Allanon a very useful feature, when this is defined, ircII will
 * be able to read help files in compressed format (it will recognize the .Z)
 * If you undefine this you will spare some code, though, so better only
 * set if you are sure you are going to keep your help-files compressed.
 */
#define ZCAT "/bin/zcat"

/* Define ZSUFFIX in case we are using ZCAT */
#ifdef ZCAT
# define ZSUFFIX ".gz"
#endif

/* And here is the port number for default client connections.  */
#define IRC_PORT 6667

/*
 * Uncomment the following to make ircII read a list of irc servers from
 * the ircII.servers file in the ircII library. This file should be
 * whitespace separated hostname:portnum:password (with the portnum and
 * password being optional). This server list will supercede the
 * DEFAULT_SERVER. 
*/
#define SERVERS_FILE "xaric.servers"

/*
 * Normally BitchX uses only the IBMPC (cp437) charset.
 * Define LATIN1, if you want to see the standard Latin1 characters
 * (i.e. Ä Ö Ü ä ö ü ß <-> "A "O "U "a "o "u \ss ).
 *
 * You will still be able to see ansi graphics, but there will be some
 * smaller problems (i.e. after a PageUp).
 *
 * If you use xterm there is no easy way to use both fonts at the same
 * time.  You have to decide if you use the standard (latin1) fonts or
 * vga.pcf (cp437).
 *
 * Is here there any solution to use both fonts nethertheless ?
 */

#undef LATIN1


/* If you define REVERSE_WHITE_BLACK, then the format codes for black and
 * white color are reversed. (%W, %w is bold black and black, %K, %k is bold
 * white and white). This way the default format-strings are readable on
 * a display with white background and black foreground.
 */ 
#undef REVERSE_WHITE_BLACK


/* On some channels mass modes can be confusing and in some case 
 * spectacular so in the interest of keeping sanity, Jordy added this 
 * mode compressor to the client. It reduces the duplicate modes that 
 * might occur on a channel.. it's explained in names.c much better.
 */
#define COMPRESS_MODES


/* The default help file to use */ 
#define DEFAULT_HELP_PATH XARIC_DATA_PATH "/help"

/*
 * Below are the IRCII variable defaults.  For boolean variables, use 1 for
 * ON and 0 for OFF.  You may set string variable to NULL if you wish them to
 * have no value.  None of these are optional.  You may *not* comment out or
 * remove them.  They are default values for variables and are required for
 * proper compilation.
 */

#define DEFAULT_SIGNOFF_REASON "Jesus Saves! (And Esposito scores on the rebound!)"
#define DEFAULT_BANTYPE 2
#define DEFAULT_SHOW_CTCP_IDLE 1
#define DEFAULT_PING_TYPE 1
#define DEFAULT_MSGLOG 1
#define DEFAULT_SHOW_TOOMANY 1
#define DEFAULT_MSGLOGFILE ".irc.away"
#define DEFAULT_PUBLOGSTR ""
#define DEFAULT_AUTO_NSLOOKUP 0
#define DEFAULT_SHOW_SERVER_KILLS 1
#define DEFAULT_SHOW_UNAUTHS 0
#define DEFAULT_SHOW_FAKES 1
#define DEFAULT_LONG_MSG 1
#define DEFAULT_FLOOD_KICK 0
#define DEFAULT_FLOOD_PROTECTION 0
#define DEFAULT_CTCP_FLOOD_PROTECTION 0
#define DEFAULT_CHECK_BEEP_USERS 1
#define DEFAULT_FAKEIGNORE 1
#define DEFAULT_SHOW_SERVER_CRAP 0

#define DEFAULT_KICK_OPS 0

#define DEFAULT_LLOOK_DELAY 120

#define DEFAULT_ALWAYS_SPLIT_BIGGEST 1
#define DEFAULT_AUTO_UNMARK_AWAY 0
#define DEFAULT_AUTO_WHOWAS 0
#define DEFAULT_BEEP 1
#define DEFAULT_BEEP_MAX 3
#define DEFAULT_BEEP_ON_MSG "MSGS,DCC"
#define DEFAULT_BEEP_WHEN_AWAY 0
#define DEFAULT_BOLD_VIDEO 1
#define DEFAULT_CHANNEL_NAME_WIDTH 10
#define DEFAULT_CLOCK 1
#define DEFAULT_CLOCK_24HOUR 0
#define DEFAULT_CLOCK_ALARM NULL
#define DEFAULT_CMDCHARS "/"
#define DEFAULT_COMMAND_MODE 0
#define DEFAULT_CONTINUED_LINE "          "
#define DEFAULT_DCC_BLOCK_SIZE 2048
#define DEFAULT_DISPLAY 1
#define DEFAULT_EIGHT_BIT_CHARACTERS 1
#define DEFAULT_ENCRYPT_PROGRAM NULL
#define DEFAULT_EXEC_PROTECTION 1
#define DEFAULT_FLOOD_AFTER 4
#define DEFAULT_FLOOD_RATE 5
#define DEFAULT_FLOOD_USERS 10
#define DEFAULT_FLOOD_WARNING 0
#define DEFAULT_FLOOD_TIME 10
#define DEFAULT_FULL_STATUS_LINE 1
#define DEFAULT_HIDE_PRIVATE_CHANNELS 0
#define DEFAULT_HIGHLIGHT_CHAR "INVERSE"
#define DEFAULT_HISTORY 100
#define DEFAULT_HOLD_MODE 0
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_INDENT 1
#define DEFAULT_INPUT_ALIASES 0
#define DEFAULT_INPUT_PROTECTION 0
#define DEFAULT_INSERT_MODE 1
#define DEFAULT_INVERSE_VIDEO 1
#define DEFAULT_LASTLOG 10000
#define DEFAULT_LASTLOG_LEVEL "ALL"
#define DEFAULT_MSGLOG_LEVEL "MSGS NOTICES SEND_MSG"

#define DEFAULT_LOG 0
#define DEFAULT_LOGFILE "IrcLog"
#define DEFAULT_MAX_RECURSIONS 10
#define DEFAULT_NO_CTCP_FLOOD 1
#define DEFAULT_NOTIFY_LEVEL "ALL DCC"
#define DEFAULT_NOTIFY_ON_TERMINATION 0
#define DEFAULT_SCROLL 1
#define DEFAULT_SCROLL_LINES 1
#define DEFAULT_SEND_IGNORE_MSG 0
#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_SHELL_FLAGS "-c"
#define DEFAULT_SHELL_LIMIT 0
#define DEFAULT_SHOW_CHANNEL_NAMES 1
#define DEFAULT_SHOW_END_OF_MSGS 0
#define DEFAULT_SHOW_NUMERICS 0
#define DEFAULT_SHOW_STATUS_ALL 0
#define DEFAULT_SHOW_WHO_HOPCOUNT 0

#define DEFAULT_SUPPRESS_SERVER_MOTD 1
#define DEFAULT_TAB 1
#define DEFAULT_TAB_MAX 8
#define DEFAULT_UNDERLINE_VIDEO 1
#define DEFAULT_USERINFO ""
#define DEFAULT_USER_WALLOPS 0
#define DEFAULT_VERBOSE_CTCP 1
#define DEFAULT_WARN_OF_IGNORES 0

#define DEFAULT_DISPLAY_ANSI 1

#define DEFAULT_DCC_DLDIR "~"
#define DEFAULT_DCC_GET_LIMIT 0
#define DEFAULT_DCC_SEND_LIMIT 5
#define DEFAULT_DCC_QUEUE_LIMIT 10
#define DEFAULT_DCC_LIMIT 10
#define DEFAULT_LLOOK 0


#define DEFAULT_AUTO_REJOIN 0
#define DEFAULT_DEOPFLOOD 1
#define DEFAULT_DEOPFLOOD_TIME 30
#define DEFAULT_DEOP_ON_DEOPFLOOD 3
#define DEFAULT_DEOP_ON_KICKFLOOD 3
#define DEFAULT_KICK_IF_BANNED 0
#define DEFAULT_JOINFLOOD 0
#define DEFAULT_JOINFLOOD_TIME 50
#define DEFAULT_KICKFLOOD 0
#define DEFAULT_KICKFLOOD_TIME 30
#define DEFAULT_KICK_ON_DEOPFLOOD 4
#define DEFAULT_KICK_ON_JOINFLOOD 5
#define DEFAULT_KICK_ON_KICKFLOOD 4
#define DEFAULT_KICK_ON_NICKFLOOD 3
#define DEFAULT_KICK_ON_PUBFLOOD 40
#define DEFAULT_NICKFLOOD 0
#define DEFAULT_NICKFLOOD_TIME 30
#define DEFAULT_PUBFLOOD 0
#define DEFAULT_PUBFLOOD_TIME 8
#define DEFAULT_PAD_CHAR ' '
#define DEFAULT_CONNECT_TIMEOUT 30
#define DEFAULT_STATUS_NO_REPEAT 0
#define DEFAULT_DISPATCH_UNKNOWN_COMMANDS 0
#define DEFAULT_SHOW_NUMERICS_STR "***"

#define DEFAULT_USERMODE "+i"  /* change this to the default usermode */
#define DEFAULT_OPERMODE "swfck"

/* File in home directory to contain specific stuff */
#define IRCRC_NAME ".ircrc"

/* File in home directory that has your list of servers */
#define IRCSERVERS_NAME ".ircservers"

#define EXEC_COMMAND

				 
#define HARD_REFRESH


  /*
   * Experimental_match_hack is defined if you want the backslash to be
   * considered a real, literal character in a pattern matching unless it
   * followed by a special wildcard character (eg, *, %, or ?).
   * If you undef this, the long standing behavior will be the default.
   * This feature may become the default or may go away based on how
   * all this works out.
   */
  #define EXPERIMENTAL_MATCH_HACK
    

#endif /* __config_h_ */
