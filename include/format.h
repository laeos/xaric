#ifndef format_h__
#define format_h__

/*
 * format.h - default formats for xaric
 * Copyright (C) 1998, 1999, 2000 Rex Feany <laeos@laeos.net>
 * 
 * This file is a part of xaric, an irc client.
 * You can find xaric at <http://www.laeos.net/projects/xaric/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * %W%
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* All the default formats are here! */

#define DEF_FORMAT_381 "%K>%n>%W> You are now a %GIRC%n whore"
#define DEF_FORMAT_391 "$G [$1] Channel is full"
#define DEF_FORMAT_443 "$G [$1] Channel is full"

#define DEF_FORMAT_471 "$G [$1] Channel is full"
#define DEF_FORMAT_473 "$G [$1] Invite only channel"
#define DEF_FORMAT_474 "$G [$1] Banned from channel"
#define DEF_FORMAT_475 "$G [$1] Bad channel key"
#define DEF_FORMAT_476 "$G [$1] You are not opped"

#define DEF_FORMAT_ACTION "%K.%n.%W. %W$1 %n$4-"
#define DEF_FORMAT_ACTION_OTHER "%K.%n.%W. %n>%c$1 %n$3-"

#define DEF_FORMAT_ALIAS "Alias $[20.]0 $1-"
#define DEF_FORMAT_ASSIGN "Assign $[20.]0 $1-"
#define DEF_FORMAT_AWAY "is away: $1-"	
#define DEF_FORMAT_BACK "is back from the dead. Gone $1 hrs $2 min $3 secs"
#define DEF_FORMAT_BANS_HEADER "#  Channel    SetBy        Sec  Ban"
#define DEF_FORMAT_BANS "$[2]0 $[10]1 $[10]3 $[-5]numdiff($time() $4)  $2"

#define DEF_FORMAT_BWALL "[%GXaric-Wall%n/%W$1:$2%n] $4-"

#define DEF_FORMAT_CHANNEL_SIGNOFF "$G %nSignOff %W$1%n: $3 %K(%n$4-%K)"
#define DEF_FORMAT_CONNECT "$G Connecting to server $1/%c$2%n"


#define DEF_FORMAT_CTCP "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEF_FORMAT_CTCP_FUNC "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEF_FORMAT_CTCP_FUNC_USER "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from you"
#define DEF_FORMAT_CTCP_UNKNOWN "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from $3"
#define DEF_FORMAT_CTCP_UNKNOWN_USER "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from %g$3"
#define DEF_FORMAT_CTCP_REPLY "$G %nCTCP %W$3 %nreply from %n$1: $4-"


#define DEF_FORMAT_DCC	NULL
#define DEF_FORMAT_DCC_CONNECT "$G %RDCC%n $1 %nconnection with %W$2%K[%c$4, port $5%K]%n established"
#define DEF_FORMAT_DCC_ERROR "$G %RDCC%n lost %w$1%w %rto $2 %K[%w$3-%K]"
#define DEF_FORMAT_DCC_LOST "$G %RDCC%n %W$1%n:%g$2%n %K[%C$3%K]%n $4 $5 completed in $6 secs %K(%W$7 $8/sec%K)"
#define DEF_FORMAT_DCC_REQUEST "$G %RDCC%n $1 %K(%n$2%K)%n request from %W$3%K[%c$4 [$5:$6]%K]%n $8 $7"

#define DEF_FORMAT_DESYNC "$G $1 is desynced from $2 at $0"
#define DEF_FORMAT_DISCONNECT "$G Use %G/Server%n to connect to a server"
#define DEF_FORMAT_ENCRYPTED_NOTICE "%K-%Y$1%K(%p$2%K)-%n $3-"
#define DEF_FORMAT_ENCRYPTED_PRIVMSG "%K[%Y$1%K(%p$2%K)]%n $3-"
#define DEF_FORMAT_FLOOD "%Y$1%n flood detected from %G$2%K(%g$3%K)%n on %K[%G$4%K]"
#define DEF_FORMAT_HOOK "$0-"
#define DEF_FORMAT_INVITE "%K>%n>%W> $1 Invites You to $2-"
#define DEF_FORMAT_INVITE_USER "%K>%n>%W> Inviting $1 to $2-"
#define DEF_FORMAT_JOIN "$G %C$1 %K[%c$2%K]%n has joined $3"
#define DEF_FORMAT_KICK "$G %n$3 was kicked off $2 by %c$1 %K(%n$4-%K)"
#define DEF_FORMAT_KICK_USER "%K>%n>%W> %WYou%n have been kicked off %c$2%n by %c$1 %K(%n$4-%K)"
#define DEF_FORMAT_KILL "%K>%n>%W> %RYou have been killed by $1 for $2-"
#define DEF_FORMAT_LEAVE "$G $1 %K[%w$2%K]%n has left $3 %K[%W$4-%K]"
#define DEF_FORMAT_LINKS "%K|%n$[24]0%K| |%n$[24]1%K| |%n$[3]2%K| |%n$[13]3%K|"
#define DEF_FORMAT_LIST "$[12]1 $[-5]2   $[40]3-"

#define DEF_FORMAT_MSGLOG "[$[8]0] [$1] - $2-"

#define DEF_FORMAT_MODE "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"
#define DEF_FORMAT_SMODE "$G %RServerMode%K/%c$3 %K[%W$4-%K]%n by %W$1"
#define DEF_FORMAT_MODE_CHANNEL "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"


/* zer0-ized */
#define DEF_FORMAT_MSG           "%Y[%B$1%Y]%n $3-"
#define DEF_FORMAT_SEND_MSG      "%Y>%R$1%Y<%n $3-"

#define DEF_FORMAT_SEND_NOTICE   "%K-> *%R$1%K* %n$3-"
#define DEF_FORMAT_NOTICE        "%K-%P$1%K(%p$2%K)-%n $3-"
#define DEF_FORMAT_IGNORE_NOTICE "%K-%P$2%K(%p$3%K)-%n $4-"

#define DEF_FORMAT_SEND_DCC_CHAT "%Y|%R$1%Y| %n$2-"
#define DEF_FORMAT_DCC_CHAT      "%Y=%B$1%Y= %n$3-"


#define DEF_FORMAT_SCANDIR_LIST_HEADER	"%g***%n File matches:"
#define DEF_FORMAT_SCANDIR_LIST_LINE	"%g>>>%n $0-"
#define DEF_FORMAT_SCANDIR_LIST_FOOTER	NULL

#define DEF_FORMAT_OPER "%C$1 %K[%c$2%K]%n is now %Wan%w %GIRC%n whore"

#define DEF_FORMAT_IGNORE_INVITE "%K>%n>%W> You have been invited to $1-"
#define DEF_FORMAT_IGNORE_MSG "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEF_FORMAT_IGNORE_MSG_AWAY "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEF_FORMAT_IGNORE_WALL "%K%P$1%n $2-"
#define DEF_FORMAT_MSG_GROUP "%K-%P$1%K:%p$2%K-%n $3-"

#define DEF_FORMAT_NAMES "$G%C $2 %Gusers on %C$1%G at %K[%g$0%K]%c $3"
#define DEF_FORMAT_NAMES_NICKCOLOR "%K[%B $[10]1%K]"
#define DEF_FORMAT_NAMES_NONOP "$G %K[%GNonChanOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEF_FORMAT_NAMES_VOICECOLOR "%K[%Mv%B$[10]1%K]"
#define DEF_FORMAT_NAMES_OP "$G %K[%GChanOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEF_FORMAT_NAMES_IRCOP "$G %K[%GIrcOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEF_FORMAT_NAMES_VOICE "$G %K[%MVoiceUsers%K(%m$1%K:%m$2%K)]%c $3"
#define DEF_FORMAT_NAMES_OPCOLOR "%K[%C$0%n%B$[10]1%K]"
#define DEF_FORMAT_NAMES_BANNER NULL
#define DEF_FORMAT_NAMES_FOOTER NULL


#define DEF_FORMAT_NETADD "$G %nAdded: %W$1 $2"
#define DEF_FORMAT_NETJOIN "$G %nNetjoined: %W$1 $2"
#define DEF_FORMAT_NETSPLIT "$G %nNetSplit detected: %W$1%n split from %W$2 %K[%c$0%K]"
#define DEF_FORMAT_NETSPLIT_HEADER NULL
#define DEF_FORMAT_NICKNAME "$G %W$1 %nis now known as %c$3"
#define DEF_FORMAT_NICKNAME_OTHER "$G %W$1 %nis now known as %c$4"
#define DEF_FORMAT_NICKNAME_USER "%K*%n*%W* %WYou%K(%n$1%K)%n are now known as %c$3"
#define DEF_FORMAT_NONICK "%W$1%K:%n $3-"


#define DEF_FORMAT_NOTE "($0) ($1) ($2) ($3) ($4) ($5-)"



#define DEF_FORMAT_NOTIFY_SIGNOFF "$G %GSignoff%n by %r$[10]1%n at $0"
#define DEF_FORMAT_NOTIFY_SIGNOFF_UH "$G %GSignoff%n by %r$1%K!%r$2%K@%r$3%n at $0"
#define DEF_FORMAT_NOTIFY_SIGNON_UH "$G %GSignon%n by %R$1%K!%R$2%K@%R$3%n at $0"
#define DEF_FORMAT_NOTIFY_SIGNON "$G %GSignon%n by %r$[-10]1%n at $0"
#define DEF_FORMAT_PUBLIC "%b<%n$1%b>%n $3-"
#define DEF_FORMAT_PUBLIC_AR "%b<%C$1%b>%n $3-"
#define DEF_FORMAT_PUBLIC_MSG "%b(%n$1%K/%n$3%b)%n $4-"
#define DEF_FORMAT_PUBLIC_MSG_AR "%b(%Y$1%K/%Y$3%b)%n $4-"
#define DEF_FORMAT_PUBLIC_NOTICE "%K-%P$1%K:%p$3%K-%n $4-"
#define DEF_FORMAT_PUBLIC_NOTICE_AR "%K-%G$1%K:%g$3%K-%n $4-"
#define DEF_FORMAT_PUBLIC_OTHER "%b<%n$1%K:%n$2%b>%n $3-"
#define DEF_FORMAT_PUBLIC_OTHER_AR "%b<%Y$1%K:%n$2%b>%n $3-"
#define DEF_FORMAT_SEND_ACTION "%K.%n.%W. %W$1 %n$3-"
#define DEF_FORMAT_SEND_ACTION_OTHER "%K.%n.%W. %n-> %W$1%n/%c$2 %n$3-"
#define DEF_FORMAT_SEND_AWAY "[Away ($strftime($1 %a %b %d %I:%M%p))] [Current ($strftime($0 %a %b %d %I:%M%p))]"
#define DEF_FORMAT_SEND_CTCP "%K[%rctcp%K(%R$1%K)] %n$2"
#define DEF_FORMAT_SEND_PUBLIC "%B<%w$2%B>%n $3-"
#define DEF_FORMAT_SEND_PUBLIC_OTHER "%p<%n$2%K:%n$1%p>%n $3-"
#define DEF_FORMAT_SERVER "$G%n $1: $2-"
#define DEF_FORMAT_SERVER_MSG1 "$G%n $1: $2-"
#define DEF_FORMAT_SERVER_MSG1_FROM "$G%n $1: $2-"
#define DEF_FORMAT_SERVER_MSG2 "$G%n $1-"
#define DEF_FORMAT_SERVER_MSG2_FROM "$G%n $1-"

#define DEF_FORMAT_SERVER_NOTICE "%G!%g$1%G%n $2-"

#define DEF_FORMAT_SET "%g$[-30.]0 %w$1-"
#define DEF_FORMAT_CSET "%r$[-14]1 %R$[-20.]0 %w$[-5]2-"
#define DEF_FORMAT_SET_NOVALUE "%g$[-30.]0 has no value"
#define DEF_FORMAT_SIGNOFF "$G %nSignOff: %W$1 %K(%n$3-%K)"


#define DEF_FORMAT_SILENCE "$G %RWe are $1 silencing $2 at $0"

#define DEF_FORMAT_TRACE_OPER "%R$1%n %K[%n$3%K]"
#define DEF_FORMAT_TRACE_SERVER "%R$1%n $2 $3 $4 %K[%n$5%K]%n $6-"
#define DEF_FORMAT_TRACE_USER "%R$1%n %K[%n$3%K]"

#define DEF_FORMAT_TIMER "$G $[-5]0 $[-10]1 $[-6]2 $3-"
#define DEF_FORMAT_TOPIC "$G Topic for %c$1%K:%n $2-"
#define DEF_FORMAT_TOPIC_CHANGE "$G %W$1 %nhas changed the topic on channel $2 to%K:%n $3-"
#define DEF_FORMAT_TOPIC_CHANGE_HEADER NULL
#define DEF_FORMAT_TOPIC_SETBY "$G %ntopic set by %c$2%K"
#define DEF_FORMAT_TOPIC_UNSET "$G %ntopic unset by $1 on $2"

#define DEF_FORMAT_USAGE "$G Usage: /$0  $1-"
#define DEF_FORMAT_USERMODE "$G %nMode change %K[%W$4-%K]%n for user %c$3"

#define DEF_FORMAT_USERS "%K[%n$[10]3%K] %K[%n%C$6%B$[10]4%K] %K[%n$[37]5%K] %K[%n$[-3]0%b:%n$1%b:%n$2%K]"
#define DEF_FORMAT_USERS_USER "%K[%n$[10]3%K] %K[%n%C$6%B$[10]4%K] %K[%n%B$[37]5%K] %K[%n$[-3]0%b:%n$1%b:%n$2%K]"
#define DEF_FORMAT_USERS_HEADER "%K[ %WC%nhannel  %K] [ %WN%wickname  %K] [%n %Wu%wser@host                           %K] [%n %Wl%wevel %K]"
#define DEF_FORMAT_VERSION "\002Xaric v$0\002 running $1 $2"


#define DEF_FORMAT_WALL "%G!%g$1:$2%G!%n $3-"
#define DEF_FORMAT_WALL_AR "%G!%g$1:$2%G!%n $3-"


#define DEF_FORMAT_WALLOP "%G!%g$1$2%G!%n $3-"
#define DEF_FORMAT_WHO "%Y$[10]0 %W$[10]1%w %c$[3]2 %w$3%R@%w$4 ($6-)"
#define DEF_FORMAT_WHOIS_AWAY "%G| %Wa%nway     : $0 - $1-"
#define DEF_FORMAT_WHOIS_CHANNELS "%G| %Wc%nhannels : $0-"
#define DEF_FORMAT_WHOIS_HEADER "%G+-----------------------------------------------"
#define DEF_FORMAT_WHOIS_IDLE "%G| %Wi%ndle     : $0 hours $1 mins $2 secs (signon: $stime($3))"
#define DEF_FORMAT_WHOIS_SIGNON "%K %Ws%nignon   : $0-"
#define DEF_FORMAT_WHOIS_NAME "%G| %Wi%nrcname  : $0-"
#define DEF_FORMAT_WHOIS_NICK "%G| %W$0 %K(%n$1@$2%K) %K(%W$3-%K)"
#define DEF_FORMAT_WHOIS_OPER "%G| %Wo%nperator : $0 $1-"
#define DEF_FORMAT_WHOIS_SERVER "%G| %Ws%nerver   : $0 ($1-)"
#define DEF_FORMAT_WHOIS_FOOTER NULL
#define DEF_FORMAT_WHOLEFT_HEADER "%G+------ %WWho %G---------------------- %WChannel%G--- %wServer %G------------- %wSeconds"
#define DEF_FORMAT_WHOLEFT_USER "%G|%n $[-10]0!$[20]1 $[10]2 $[20]4 $3"
#define DEF_FORMAT_WHOLEFT_FOOTER NULL
#define DEF_FORMAT_WHOWAS_HEADER "%G-----------------------------------------------"
#define DEF_FORMAT_WHOWAS_NICK "%G| %W$0%n was %K(%n$1@$2%K)"
#define DEF_FORMAT_WIDELIST "$1-"
#define DEF_FORMAT_WINDOW_SET "$0-"

#define DEF_FORMAT_NICK_MSG "$0 $1 $2-"

#define DEF_FORMAT_NICK_COMP "$0: $1-"
#define DEF_FORMAT_NICK_AUTO "$0:$1-"

#define DEF_FORMAT_STATUS "%4%W$0-"
#define DEF_FORMAT_STATUS2 "%4%W$0-"
#define DEF_FORMAT_NOTIFY_OFF "$[10]0 $[35]1 $[-6]2 $[10]3 $4"
#define DEF_FORMAT_NOTIFY_ON "$[10]0 $[35]1 $[-6]2 $[10]3 $4"


/* Hmm these im not sure where they belong */
#define DEFAULT_KICK_REASON "Xaric: There can be only one"
#define DEFAULT_INPUT_PROMPT "%K[%n$C%K] "

/* Status formats are used in var.h, not in fset.h */
#define DEFAULT_STATUS_AWAY " [0;44;36m([1;37maway[0;44;36m)[0;44;37m"
#define DEFAULT_STATUS_CHANNEL " [0;44;37m%C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_CLOCK " %T"
#define DEFAULT_STATUS_FORMAT "[%R]%T %=%*%@%N%L%#%S%H%B%Q%A%C%+%I%O%M%F %W "
#define DEFAULT_STATUS_FORMAT2 "%^ %> %P"
#define DEFAULT_STATUS_HOLD " -- more --"
#define DEFAULT_STATUS_HOLD_LINES " [0;44;36m([1;37m%B[0;44m)[0;44;37m"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_LAG " [0;44;36m[[0;44;37mLag[1;37m %L[0;44;36m]"
#define DEFAULT_STATUS_MODE " [1;37m([0;44;36m+[0;44;37m%+[1;37m)"
#define DEFAULT_STATUS_MSGCOUNT " Received [0;44;36m[[1;37m%^[0;44;36m] msg's since away"
#define DEFAULT_STATUS_NOTIFY " [0;44;36m[[37mAct: [1;37m%F[0;44;36m]"
#define DEFAULT_STATUS_OPER "[1;31m*[0;44;37m"
#define DEFAULT_STATUS_VOICE "[1;32m+[0;44;37m"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_QUERY " [0;44;36m[[37mQuery: [1;37m%Q[0;44;36m]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_TOPIC "%-"
#define DEFAULT_STATUS_UMODE " [1;37m([0;44;36m+[37m%#[1;37m)"
#define DEFAULT_STATUS_DCCCOUNT  "[DCC  gets/%& sends/%&]"
#define DEFAULT_STATUS_OPER_KILLS "[0;44;36m[[37mnk[36m/[1;37m%K [0;44mok[36m/[1;37m%K[0;44;36m]"
#define DEFAULT_STATUS_WINDOW "[1;33m^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"



#endif /* format_h__ */
