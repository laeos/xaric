#ifndef _FORMAT_H_
#define _FORMAT_H_

/* All the default formats are here! */

#define DEFAULT_FMT_381_FSET "%K>%n>%W> You are now a %GIRC%n whore"
#define DEFAULT_FMT_391_FSET "$G [$1] Channel is full"
#define DEFAULT_FMT_443_FSET "$G [$1] Channel is full"

#define DEFAULT_FMT_471_FSET "$G [$1] Channel is full"
#define DEFAULT_FMT_473_FSET "$G [$1] Invite only channel"
#define DEFAULT_FMT_474_FSET "$G [$1] Banned from channel"
#define DEFAULT_FMT_475_FSET "$G [$1] Bad channel key"
#define DEFAULT_FMT_476_FSET "$G [$1] You are not opped"

#define DEFAULT_FMT_ACTION_FSET "%K.%n.%W. %W$1 %n$4-"
#define DEFAULT_FMT_ACTION_OTHER_FSET "%K.%n.%W. %n>%c$1 %n$3-"

#define DEFAULT_FMT_ALIAS_FSET "Alias $[20.]0 $1-"
#define DEFAULT_FMT_ASSIGN_FSET "Assign $[20.]0 $1-"
#define DEFAULT_FMT_AWAY_FSET "is away: $1-"	
#define DEFAULT_FMT_BACK_FSET "is back from the dead. Gone $1 hrs $2 min $3 secs"
#define DEFAULT_FMT_BANS_HEADER_FSET "#  Channel    SetBy        Sec  Ban"
#define DEFAULT_FMT_BANS_FSET "$[2]0 $[10]1 $[10]3 $[-5]numdiff($time() $4)  $2"

#define DEFAULT_FMT_BWALL_FSET "[%GXaric-Wall%n/%W$1:$2%n] $4-"

#define DEFAULT_FMT_CHANNEL_SIGNOFF_FSET "$G %nSignOff %W$1%n: $3 %K(%n$4-%K)"
#define DEFAULT_FMT_CONNECT_FSET "$G Connecting to server $1/%c$2%n"


#define DEFAULT_FMT_CTCP_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEFAULT_FMT_CTCP_FUNC_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from $3"
#define DEFAULT_FMT_CTCP_FUNC_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested $4- from you"
#define DEFAULT_FMT_CTCP_UNKNOWN_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from $3"
#define DEFAULT_FMT_CTCP_UNKNOWN_USER_FSET "%K>%n>%W> %G$1 %K[%g$2%K]%g requested unknown ctcp $4- from %g$3"
#define DEFAULT_FMT_CTCP_REPLY_FSET "$G %nCTCP %W$3 %nreply from %n$1: $4-"


#define DEFAULT_FMT_DCC_CONNECT_FSET "$G %RDCC%n $1 %nconnection with %W$2%K[%c$4, port $5%K]%n established"
#define DEFAULT_FMT_DCC_ERROR_FSET "$G %RDCC%n lost %w$1%w %rto $2 %K[%w$3-%K]"
#define DEFAULT_FMT_DCC_LOST_FSET "$G %RDCC%n %W$1%n:%g$2%n %K[%C$3%K]%n $4 $5 completed in $6 secs %K(%W$7 $8/sec%K)"
#define DEFAULT_FMT_DCC_REQUEST_FSET "$G %RDCC%n $1 %K(%n$2%K)%n request from %W$3%K[%c$4 [$5:$6]%K]%n $8 $7"
#define DEFAULT_FMT_DESYNC_FSET "$G $1 is desynced from $2 at $0"
#define DEFAULT_FMT_DISCONNECT_FSET "$G Use %G/Server%n to connect to a server"
#define DEFAULT_FMT_ENCRYPTED_NOTICE_FSET "%K-%Y$1%K(%p$2%K)-%n $3-"
#define DEFAULT_FMT_ENCRYPTED_PRIVMSG_FSET "%K[%Y$1%K(%p$2%K)]%n $3-"
#define DEFAULT_FMT_FLOOD_FSET "%Y$1%n flood detected from %G$2%K(%g$3%K)%n on %K[%G$4%K]"
#define DEFAULT_FMT_HOOK_FSET "$0-"
#define DEFAULT_FMT_INVITE_FSET "%K>%n>%W> $1 Invites You to $2-"
#define DEFAULT_FMT_INVITE_USER_FSET "%K>%n>%W> Inviting $1 to $2-"
#define DEFAULT_FMT_JOIN_FSET "$G %C$1 %K[%c$2%K]%n has joined $3"
#define DEFAULT_FMT_KICK_FSET "$G %n$3 was kicked off $2 by %c$1 %K(%n$4-%K)"
#define DEFAULT_FMT_KICK_USER_FSET "%K>%n>%W> %WYou%n have been kicked off %c$2%n by %c$1 %K(%n$4-%K)"
#define DEFAULT_FMT_KILL_FSET "%K>%n>%W> %RYou have been killed by $1 for $2-"
#define DEFAULT_FMT_LEAVE_FSET "$G $1 %K[%w$2%K]%n has left $3 %K[%W$4-%K]"
#define DEFAULT_FMT_LINKS_FSET "%K|%n$[24]0%K| |%n$[24]1%K| |%n$[3]2%K| |%n$[13]3%K|"
#define DEFAULT_FMT_LIST_FSET "$[12]1 $[-5]2   $[40]3-"

#define DEFAULT_FMT_MSGLOG_FSET "[$[8]0] [$1] - $2-"

#define DEFAULT_FMT_MODE_FSET "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"
#define DEFAULT_FMT_SMODE_FSET "$G %RServerMode%K/%c$3 %K[%W$4-%K]%n by %W$1"
#define DEFAULT_FMT_MODE_CHANNEL_FSET "$G %nmode%K/%c$3 %K[%W$4-%K]%n by %W$1"


/* zer0-ized */
#define DEFAULT_FMT_MSG_FSET           "%Y[%B$1%Y]%n $3-"
#define DEFAULT_FMT_SEND_MSG_FSET      "%Y>%R$1%Y<%n $3-"

#define DEFAULT_FMT_SEND_NOTICE_FSET   "%K-> *%R$1%K* %n$3-"
#define DEFAULT_FMT_NOTICE_FSET        "%K-%P$1%K(%p$2%K)-%n $3-"
#define DEFAULT_FMT_IGNORE_NOTICE_FSET "%K-%P$2%K(%p$3%K)-%n $4-"

#define DEFAULT_FMT_SEND_DCC_CHAT_FSET "%Y|%R$1%Y| %n$2-"
#define DEFAULT_FMT_DCC_CHAT_FSET      "%Y=%B$1%Y= %n$3-"



#define DEFAULT_FMT_OPER_FSET "%C$1 %K[%c$2%K]%n is now %Wan%w %GIRC%n whore"

#define DEFAULT_FMT_IGNORE_INVITE_FSET "%K>%n>%W> You have been invited to $1-"
#define DEFAULT_FMT_IGNORE_MSG_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FMT_IGNORE_MSG_AWAY_FSET "%K[%P$1%P$2%K(%p$3%K)]%n $4-"
#define DEFAULT_FMT_IGNORE_WALL_FSET "%K%P$1%n $2-"
#define DEFAULT_FMT_MSG_GROUP_FSET "%K-%P$1%K:%p$2%K-%n $3-"

#define DEFAULT_FMT_NAMES_FSET "$G%C $2 %Gusers on %C$1%G at %K[%g$0%K]%c $3"
#define DEFAULT_FMT_NAMES_NICKCOLOR_FSET "%K[%B $[10]1%K]"
#define DEFAULT_FMT_NAMES_NONOP_FSET "$G %K[%GNonChanOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEFAULT_FMT_NAMES_VOICECOLOR_FSET "%K[%Mv%B$[10]1%K]"
#define DEFAULT_FMT_NAMES_OP_FSET "$G %K[%GChanOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEFAULT_FMT_NAMES_IRCOP_FSET "$G %K[%GIrcOps%K(%g$1%K:%g$2%K)]%c $3"
#define DEFAULT_FMT_NAMES_VOICE_FSET "$G %K[%MVoiceUsers%K(%m$1%K:%m$2%K)]%c $3"
#define DEFAULT_FMT_NAMES_OPCOLOR_FSET "%K[%C$0%n%B$[10]1%K]"

#define DEFAULT_FMT_NETADD_FSET "$G %nAdded: %W$1 $2"
#define DEFAULT_FMT_NETJOIN_FSET "$G %nNetjoined: %W$1 $2"
#define DEFAULT_FMT_NETSPLIT_FSET "$G %nNetSplit detected: %W$1%n split from %W$2 %K[%c$0%K]"
#define DEFAULT_FMT_NICKNAME_FSET "$G %W$1 %nis now known as %c$3"
#define DEFAULT_FMT_NICKNAME_OTHER_FSET "$G %W$1 %nis now known as %c$4"
#define DEFAULT_FMT_NICKNAME_USER_FSET "%K*%n*%W* %WYou%K(%n$1%K)%n are now known as %c$3"
#define DEFAULT_FMT_NONICK_FSET "%W$1%K:%n $3-"


#define DEFAULT_FMT_NOTE_FSET "($0) ($1) ($2) ($3) ($4) ($5-)"



#define DEFAULT_FMT_NOTIFY_SIGNOFF_FSET "$G %GSignoff%n by %r$[10]1%n at $0"
#define DEFAULT_FMT_NOTIFY_SIGNOFF_UH_FSET "$G %GSignoff%n by %r$1%K!%r$2%K@%r$3%n at $0"
#define DEFAULT_FMT_NOTIFY_SIGNON_UH_FSET "$G %GSignon%n by %R$1%K!%R$2%K@%R$3%n at $0"
#define DEFAULT_FMT_NOTIFY_SIGNON_FSET "$G %GSignon%n by %r$[-10]1%n at $0"
#define DEFAULT_FMT_PUBLIC_FSET "%b<%n$1%b>%n $3-"
#define DEFAULT_FMT_PUBLIC_AR_FSET "%b<%C$1%b>%n $3-"
#define DEFAULT_FMT_PUBLIC_MSG_FSET "%b(%n$1%K/%n$3%b)%n $4-"
#define DEFAULT_FMT_PUBLIC_MSG_AR_FSET "%b(%Y$1%K/%Y$3%b)%n $4-"
#define DEFAULT_FMT_PUBLIC_NOTICE_FSET "%K-%P$1%K:%p$3%K-%n $4-"
#define DEFAULT_FMT_PUBLIC_NOTICE_AR_FSET "%K-%G$1%K:%g$3%K-%n $4-"
#define DEFAULT_FMT_PUBLIC_OTHER_FSET "%b<%n$1%K:%n$2%b>%n $3-"
#define DEFAULT_FMT_PUBLIC_OTHER_AR_FSET "%b<%Y$1%K:%n$2%b>%n $3-"
#define DEFAULT_FMT_SEND_ACTION_FSET "%K.%n.%W. %W$1 %n$3-"
#define DEFAULT_FMT_SEND_ACTION_OTHER_FSET "%K.%n.%W. %n-> %W$1%n/%c$2 %n$3-"
#define DEFAULT_FMT_SEND_AWAY_FSET "[Away ($strftime($1 %a %b %d %I:%M%p))] [Current ($strftime($0 %a %b %d %I:%M%p))]"
#define DEFAULT_FMT_SEND_CTCP_FSET "%K[%rctcp%K(%R$1%K)] %n$2"
#define DEFAULT_FMT_SEND_PUBLIC_FSET "%B<%w$2%B>%n $3-"
#define DEFAULT_FMT_SEND_PUBLIC_OTHER_FSET "%p<%n$2%K:%n$1%p>%n $3-"
#define DEFAULT_FMT_SERVER_FSET "$G%n $1: $2-"
#define DEFAULT_FMT_SERVER_MSG1_FSET "$G%n $1: $2-"
#define DEFAULT_FMT_SERVER_MSG1_FROM_FSET "$G%n $1: $2-"
#define DEFAULT_FMT_SERVER_MSG2_FSET "$G%n $1-"
#define DEFAULT_FMT_SERVER_MSG2_FROM_FSET "$G%n $1-"

#define DEFAULT_FMT_SERVER_NOTICE_FSET "%G!%g$1%G%n $2-"

#define DEFAULT_FMT_SET_FSET "%g$[-30.]0 %w$1-"
#define DEFAULT_FMT_CSET_FSET "%r$[-14]1 %R$[-20.]0 %w$[-5]2-"
#define DEFAULT_FMT_SET_NOVALUE_FSET "%g$[-30.]0 has no value"
#define DEFAULT_FMT_SIGNOFF_FSET "$G %nSignOff: %W$1 %K(%n$3-%K)"


#define DEFAULT_FMT_SILENCE_FSET "$G %RWe are $1 silencing $2 at $0"

#define DEFAULT_FMT_TRACE_OPER_FSET "%R$1%n %K[%n$3%K]"
#define DEFAULT_FMT_TRACE_SERVER_FSET "%R$1%n $2 $3 $4 %K[%n$5%K]%n $6-"
#define DEFAULT_FMT_TRACE_USER_FSET "%R$1%n %K[%n$3%K]"

#define DEFAULT_FMT_TIMER_FSET "$G $[-5]0 $[-10]1 $[-6]2 $3-"
#define DEFAULT_FMT_TOPIC_FSET "$G Topic for %c$1%K:%n $2-"
#define DEFAULT_FMT_TOPIC_CHANGE_FSET "$G %W$1 %nhas changed the topic on channel $2 to%K:%n $3-"
#define DEFAULT_FMT_TOPIC_SETBY_FSET "$G %ntopic set by %c$2%K"
#define DEFAULT_FMT_TOPIC_UNSET_FSET "$G %ntopic unset by $1 on $2"

#define DEFAULT_FMT_USAGE_FSET "$G Usage: /$0  $1-"
#define DEFAULT_FMT_USERMODE_FSET "$G %nMode change %K[%W$4-%K]%n for user %c$3"

#define DEFAULT_FMT_USERS_FSET "%K[%n$[10]3%K] %K[%n%C$6%B$[10]4%K] %K[%n$[37]5%K] %K[%n$[-3]0%b:%n$1%b:%n$2%K]"
#define DEFAULT_FMT_USERS_USER_FSET "%K[%n$[10]3%K] %K[%n%C$6%B$[10]4%K] %K[%n%B$[37]5%K] %K[%n$[-3]0%b:%n$1%b:%n$2%K]"
#define DEFAULT_FMT_USERS_HEADER_FSET "%K[ %WC%nhannel  %K] [ %WN%wickname  %K] [%n %Wu%wser@host                           %K] [%n %Wl%wevel %K]"
#define DEFAULT_FMT_VERSION_FSET "\002Xaric v$0\002 ($1) running $2 $3"


#define DEFAULT_FMT_WALL_FSET "%G!%g$1:$2%G!%n $3-"
#define DEFAULT_FMT_WALL_AR_FSET "%G!%g$1:$2%G!%n $3-"


#define DEFAULT_FMT_WALLOP_FSET "%G!%g$1$2%G!%n $3-"
#define DEFAULT_FMT_WHO_FSET "%Y$[10]0 %W$[10]1%w %c$[3]2 %w$3%R@%w$4 ($6-)"
#define DEFAULT_FMT_WHOIS_AWAY_FSET "%G| %Wa%nway     : $0 - $1-"
#define DEFAULT_FMT_WHOIS_CHANNELS_FSET "%G| %Wc%nhannels : $0-"
#define DEFAULT_FMT_WHOIS_HEADER_FSET "%G+-----------------------------------------------"
#define DEFAULT_FMT_WHOIS_IDLE_FSET "%G| %Wi%ndle     : $0 hours $1 mins $2 secs (signon: $stime($3))"
#define DEFAULT_FMT_WHOIS_SIGNON_FSET "%K %Ws%nignon   : $0-"
#define DEFAULT_FMT_WHOIS_NAME_FSET "%G| %Wi%nrcname  : $0-"
#define DEFAULT_FMT_WHOIS_NICK_FSET "%G| %W$0 %K(%n$1@$2%K) %K(%W$3-%K)"
#define DEFAULT_FMT_WHOIS_OPER_FSET "%G| %Wo%nperator : $0 $1-"
#define DEFAULT_FMT_WHOIS_SERVER_FSET "%G| %Ws%nerver   : $0 ($1-)"
#define DEFAULT_FMT_WHOLEFT_HEADER_FSET "%G+------ %WWho %G---------------------- %WChannel%G--- %wServer %G------------- %wSeconds"
#define DEFAULT_FMT_WHOLEFT_USER_FSET "%G|%n $[-10]0!$[20]1 $[10]2 $[20]4 $3"
#define DEFAULT_FMT_WHOWAS_HEADER_FSET "%G-----------------------------------------------"
#define DEFAULT_FMT_WHOWAS_NICK_FSET "%G| %W$0%n was %K(%n$1@$2%K)"
#define DEFAULT_FMT_WIDELIST_FSET "$1-"
#define DEFAULT_FMT_WINDOW_SET_FSET "$0-"

#define DEFAULT_FMT_NICK_MSG_FSET "$0 $1 $2-"

#define DEFAULT_FMT_NICK_COMP_FSET "$0: $1-"
#define DEFAULT_FMT_NICK_AUTO_FSET "$0:$1-"

#define DEFAULT_FMT_STATUS_FSET "%4%W$0-"
#define DEFAULT_FMT_STATUS2_FSET "%4%W$0-"
#define DEFAULT_FMT_NOTIFY_OFF_FSET "$[10]0 $[35]1 $[-6]2 $[10]3 $4"
#define DEFAULT_FMT_NOTIFY_ON_FSET "$[10]0 $[35]1 $[-6]2 $[10]3 $4"


#define DEFAULT_KICK_REASON "Xaric: There can be only one"

#define DEFAULT_INPUT_PROMPT "%K[%n$C%K] "

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
#define DEFAULT_STATUS_NOTIFY " [0;44;36m[[37mActivity: [1;37m%F[0;44;36m]"
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



#endif /* _FORMAT_H_ */
