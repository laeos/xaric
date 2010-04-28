/*
 * cmd_modes.c : commands that have to do with modes?!
 *
 * Parts Written By Michael Sandrof, I think :)
 * Code from original banlist.c (C) Colten Edwards
 * Portions are based on EPIC.
 * Copyright(c) 1996
 * Modified for Xaric by Rex Feany <laeos@ptw.com> 1998
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "irc.h"
#include "ircaux.h"
#include "whois.h"
#include "output.h"
#include "status.h"
#include "misc.h"
#include "tcommand.h"
#include "list.h"
#include "hook.h"
#include "ignore.h"
#include "ircaux.h"
#include "output.h"
#include "screen.h"
#include "server.h"
#include "struct.h"
#include "window.h"
#include "whois.h"
#include "whowas.h"
#include "vars.h"
#include "misc.h"
#include "hash.h"
#include "fset.h"

static char *mode_buf = NULL;
static int mode_len = 0;

static char *mode_str = NULL;
static char *user_str = NULL;
static int mode_str_len = 0;
static char plus_mode[20] = "\0";

static void add_mode_buffer(char *buffer, int len)
{
    malloc_strcat(&mode_buf, buffer);
    mode_len += len;
}

static void flush_mode(struct channel *chan)
{
    if (mode_buf)
	send_to_server(SERVER(from_server), "%s", mode_buf);
    new_free(&mode_buf);
    mode_len = 0;
}

void flush_mode_all(struct channel *chan)
{
    char buffer[BIG_BUFFER_SIZE + 1];
    int len;

    if (mode_str && user_str) {
	len = sprintf(buffer, "MODE %s %s%s %s\r\n", chan->channel, plus_mode, mode_str, user_str);
	add_mode_buffer(buffer, len);
	mode_str_len = 0;
	new_free(&mode_str);
	new_free(&user_str);
	memset(plus_mode, 0, sizeof(plus_mode));
	len = 0;
    }
    flush_mode(chan);
}

static void add_mode(struct channel *chan, const char *mode, int plus, const char *nick, const char *reason, int max_modes)
{
    char buffer[BIG_BUFFER_SIZE + 1];
    int len;

    /* 
     * KICK $C nick :reason
     * MODE $C +/-o nick
     * MODE $C +/-b userhost
     */

    if (mode_len >= (IRCD_BUFFER_SIZE - 100)) {
	flush_mode(chan);
    }

    if (reason) {
	len = sprintf(buffer, "KICK %s %s :%s\r\n", chan->channel, nick, reason);
	add_mode_buffer(buffer, len);
    } else {
	mode_str_len++;
	malloc_strcat(&mode_str, plus ? "+" : "-");
	malloc_strcat(&mode_str, mode);
	malloc_strcat(&user_str, nick);
	malloc_strcat(&user_str, " ");
	if (mode_str_len >= max_modes) {
	    len = sprintf(buffer, "MODE %s %s %s\r\n", chan->channel, mode_str, user_str);
	    add_mode_buffer(buffer, len);
	    new_free(&mode_str);
	    new_free(&user_str);
	    memset(plus_mode, 0, sizeof(plus_mode));
	    mode_str_len = len = 0;
	}
    }
}

static char *screw(char *user)
{
    char *p;

    for (p = user; p && *p;) {
	switch (*p) {
	case '.':
	case '*':
	case '@':
	case '!':
	    p += 1;
	    break;
	default:
	    *p = '?';
	    if (*(p + 1) && *(p + 2))
		p += 2;
	    else
		p++;
	}
    }
    return user;
}

static char *ban_it(char *nick, char *user, char *host)
{
    static char banstr[BIG_BUFFER_SIZE + 1];
    char *tmpstr = NULL;
    char *t = user;
    char *t1 = user;

    *banstr = 0;
    while (strlen(t1) > 9)
	t1++;
    switch (get_int_var(BANTYPE_VAR)) {
    case 2:			/* Better */
	sprintf(banstr, "*!*%s@%s", t1, cluster(host));
	break;
    case 3:			/* Host */
	sprintf(banstr, "*!*@%s", host);
	break;
    case 4:			/* Domain */
	sprintf(banstr, "*!*@*%s", strrchr(host, '.'));
	break;
    case 5:			/* User */
	sprintf(banstr, "*!%s@%s", t, cluster(host));
	break;
    case 6:			/* Screw */
	malloc_sprintf(&tmpstr, "*!*%s@%s", t1, host);
	strcpy(banstr, screw(tmpstr));
	new_free(&tmpstr);
	break;
    case 1:			/* Normal */
    default:
	sprintf(banstr, "%s!*%s@%s", nick, t1, host);
	break;
    }
    return banstr;
}

static void userhost_unban(WhoisStuff * stuff, char *nick1, char *args)
{
    char *tmp;
    struct channel *chan;
    BanList *bans;

    char *host = NULL;
    struct whowas_list *whowas = NULL;

    int count = 0;
    int old_server = from_server;

    if (!stuff || !stuff->nick || !nick1 || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick1)) {
	if (nick1 && (whowas = check_whowas_nick_buffer(nick1, args, 0))) {
	    malloc_sprintf(&host, "%s!%s", whowas->nicklist->nick, whowas->nicklist->host);
	    bitchsay("Using WhoWas info for unban of %s ", nick1);
	} else if (nick1) {
	    bitchsay("No match for the unban of %s on %s", nick1, args);
	    return;
	}
	if (!nick1)
	    return;
    } else {
	tmp = clear_server_flags(stuff->user);
	malloc_sprintf(&host, "%s!%s@%s", stuff->nick, tmp, stuff->host);
    }

    if (!(chan = prepare_command(&from_server, NULL, NEED_OP))) {
	new_free(&host);
	return;
    }

    for (bans = chan->bans; bans; bans = bans->next) {
	if (!bans->sent_unban && (match(bans->ban, host) || match(host, bans->ban))) {
	    add_mode(chan, "b", 0, bans->ban, NULL, get_int_var(NUM_BANMODES_VAR));
	    bans->sent_unban++;
	    count++;
	}
    }

    flush_mode_all(chan);
    if (!count)
	bitchsay("No match for Unban of %s on %s", nick1, args);
    new_free(&host);
    from_server = old_server;
}

static void userhost_ban(WhoisStuff * stuff, char *nick1, char *args)
{
    char *temp;
    char *channel;

    char *ob = "-o+b";
    char *b = "+b";

    char *host = NULL, *nick = NULL, *user = NULL;
    struct whowas_list *whowas = NULL;

    channel = next_arg(args, &args);
    temp = next_arg(args, &args);

    /* nasty */
    if (!stuff || !stuff->nick || !nick1 || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick1)) {
	if (nick1 && channel && (whowas = check_whowas_nick_buffer(nick1, channel, 0))) {
	    nick = whowas->nicklist->nick;
	    user = m_strdup(clear_server_flags(whowas->nicklist->host));

	    if (strcmp(user, "<UNKNOWN>") == 0) {
		bitchsay("No match for the ban of %s", nick1);
		new_free(&user);
		return;
	    }
	    host = strchr(user, '@');
	    *host++ = 0;
	    bitchsay("Using WhoWas info for ban of %s ", nick1);
	} else if (nick1) {
	    bitchsay("No match for the ban of %s", nick1);
	    return;
	}
    } else {
	nick = stuff->nick;
	user = m_strdup(clear_server_flags(stuff->user));
	host = stuff->host;
    }

    if (!(my_stricmp(nick, get_server_nickname(from_server)))) {
	bitchsay("Try to kick yourself again!!");
	new_free(&user);
	return;
    }

    send_to_server(SERVER(from_server), "MODE %s %s %s", channel, stuff->channel ? ob : b, ban_it(nick, user, host));
    send_to_server(SERVER(from_server), "KICK %s %s :%s", channel, nick, args ? args : get_reason(nick));

    new_free(&user);
}

void cmd_unban(struct command *cmd, char *args)
{
    char *to, *spec, *host;
    struct channel *chan;
    BanList *bans;
    int count = 0;
    int server = from_server;

    to = spec = host = NULL;

    if (!(to = new_next_arg(args, &args)))
	to = NULL;
    if (to && !is_channel(to)) {
	spec = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&server, to, NEED_OP)))
	return;

    if (!spec && !(spec = next_arg(args, &args))) {
	spec = "*";
	host = "*@*";
    }

    if (spec && *spec == '#') {
	count = atoi(spec);
    }
    if (!strchr(spec, '*')) {
	add_to_userhost_queue(spec, userhost_unban, "%s", chan->channel);
	return;
    }

    if (count) {
	int tmp = 1;

	for (bans = chan->bans; bans; bans = bans->next) {
	    if (tmp == count) {
		if (bans->sent_unban == 0) {
		    send_to_server(SERVER(from_server), "MODE %s -b %s", chan->channel, bans->ban);
		    bans->sent_unban++;
		    tmp = 0;
		    break;
		}
	    } else
		tmp++;
	}
	if (tmp != 0)
	    count = 0;
    } else {
	char *banstring = NULL;
	int num = 0;

	count = 0;
	for (bans = chan->bans; bans; bans = bans->next) {
	    if (match(bans->ban, spec) || match(spec, bans->ban)) {
		if (bans->sent_unban == 0) {
		    malloc_strcat(&banstring, bans->ban);
		    malloc_strcat(&banstring, " ");
		    bans->sent_unban++;
		    count++;
		    num++;
		}
	    }
	    if (count && (count % get_int_var(NUM_BANMODES_VAR) == 0)) {
		send_to_server(SERVER(from_server), "MODE %s -%s %s", chan->channel, strfill('b', num), banstring);
		new_free(&banstring);
		num = 0;
	    }
	}
	if (banstring && num)
	    send_to_server(SERVER(from_server), "MODE %s -%s %s", chan->channel, strfill('b', num), banstring);
	new_free(&banstring);
    }
    if (!count)
	bitchsay("No ban matching %s found", spec);

}

void cmd_kick(struct command *cmd, char *args)
{
    char *to = NULL, *spec = NULL, *reason = NULL;
    struct channel *chan;
    int server = from_server;

    if (!(to = next_arg(args, &args)))
	to = NULL;

    if (to && !is_channel(to)) {
	spec = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&server, (to && !strcmp(to, "*")) ? NULL : to, NEED_OP)))
	return;

    if (!spec && !(spec = next_arg(args, &args))) {
	userage(cmd->name, cmd->qhelp);
	return;
    }
    if (args && *args)
	reason = args;
    else
	reason = get_reason(spec);

    send_to_server(SERVER(from_server), "KICK %s %s :%s", chan->channel, spec, reason);
}

void cmd_kickban(struct command *cmd, char *args)
{
    char *to = NULL, *spec = NULL, *rest = NULL;

    struct channel *chan;
    struct nick_list *nicks;
    int count = 0;
    int server = from_server;

    if (!(to = next_arg(args, &args)))
	to = NULL;

    if (to && !is_channel(to)) {
	spec = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&server, to, NEED_OP)))
	return;
    if (!spec && !(spec = next_arg(args, &args))) {
	userage(cmd->name, cmd->qhelp);
	return;
    }
    rest = args;
    if (rest && !*rest)
	rest = NULL;

    for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks)) {
	if (my_stricmp(spec, nicks->nick) == 0) {
	    char *user, *host;
	    char *t = NULL;

	    /* hrm */
	    if (strcmp(nicks->host, "<UNKNOWN>") == 0)
		break;

	    malloc_strcpy(&t, nicks->host);
	    user = clear_server_flags(t);
	    host = strchr(user, '@');
	    *host++ = 0;

	    send_to_server(SERVER(from_server), "MODE %s -o+b %s %s", chan->channel, nicks->nick, ban_it(nicks->nick, user, host));
	    send_to_server(SERVER(from_server), "KICK %s %s :%s", chan->channel, nicks->nick, rest ? rest : get_reason(nicks->nick));
	    count++;
	    new_free(&t);
	}

    }
    if (count == 0)
	add_to_userhost_queue(spec, userhost_ban, "%s %s %s", chan->channel, spec,
			      rest ? rest : get_reason(nicks ? nicks->nick : NULL));
}

void cmd_ban(struct command *cmd, char *args)
{
    char *to = NULL, *spec = NULL, *rest = NULL;
    struct channel *chan;
    struct nick_list *nicks;
    int server = from_server;
    int found = 0;

    if (!(to = next_arg(args, &args)))
	to = NULL;

    if (to && !is_channel(to)) {
	spec = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&server, to, NEED_OP)))
	return;

    if (!spec && !(spec = new_next_arg(args, &args))) {
	userage(cmd->name, cmd->qhelp);
	return;
    }
    rest = args;

    if (rest && !*rest)
	rest = NULL;

    for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks)) {
	if (!my_stricmp(spec, nicks->nick) && strcmp(nicks->host, "<UNKNOWN>")) {
	    char *t = NULL, *host, *user;

	    malloc_strcpy(&t, nicks->host);
	    user = clear_server_flags(t);
	    host = strchr(user, '@');

	    if (host) {
		*host++ = 0;
		send_to_server(SERVER(from_server), "MODE %s -o+b %s %s", chan->channel, nicks->nick, ban_it(nicks->nick, user, host));
	    } else
		send_to_server(SERVER(from_server), "MODE %s -o+b %s %s", chan->channel, nicks->nick, ban_it(nicks->nick, user, "*"));

	    new_free(&t);
	    found++;

	}
    }
    if (!chan && !found) {
	bitchsay("No match for the ban of %s", spec);
    }
    if (!found) {
	if (strchr(spec, '!') && strchr(spec, '@'))
	    send_to_server(SERVER(from_server), "MODE %s +b %s", chan->channel, spec);
	else
	    add_to_userhost_queue(spec, userhost_ban, "%s %s", chan->channel, spec);
    }
}

void cmd_banstat(struct command *cmd, char *args)
{
    char *channel = NULL, *tmp = NULL, *check = NULL;
    struct channel *chan;
    BanList *tmpc;
    int count = 1;
    int server;

    if (args && *args) {
	tmp = next_arg(args, &args);
	if (*tmp == '#' && is_channel(tmp))
	    malloc_strcpy(&channel, tmp);
	else
	    malloc_strcpy(&check, tmp);
	if (args && *args && channel) {
	    tmp = next_arg(args, &args);
	    malloc_strcpy(&check, tmp);
	}
    }

    if ((chan = prepare_command(&server, channel, NO_OP))) {
	if (!chan->bans) {
	    bitchsay("No bans on %s", chan->channel);
	    return;
	}
	if ((do_hook(BANS_HEADER_LIST, "%s %s %s %s %s", "#", "Channel", "Ban", "SetBy", "Seconds")))
	    put_it("%s", convert_output_format(get_fset_var(FORMAT_BANS_HEADER_FSET), NULL));
	for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++) {
	    if (check && (!match(check, tmpc->ban) || !match(tmpc->ban, check)))
		continue;
	    if (do_hook
		(BANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban,
		 tmpc->setby ? tmpc->setby : get_server_name(from_server), (unsigned long) tmpc->time))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_BANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban,
					     tmpc->setby ? tmpc->setby : get_server_name(from_server), tmpc->time));
	}
	new_free(&check);
	new_free(&channel);
    } else if (channel)
	send_to_server(SERVER(from_server), "MODE %s b", channel);
}

static void remove_bans(char *stuff, char *line)
{
    struct channel *chan;
    int count = 1;
    BanList *tmpc, *next;
    char *banstring = NULL;
    int num = 0;
    int server = from_server;

    if (stuff && (chan = prepare_command(&server, stuff, NEED_OP))) {

	if (!chan->bans) {
	    bitchsay("No bans on %s", stuff);
	    return;
	}

	for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++)
	    if (!tmpc->sent_unban && (matchmcommand(line, count))) {
		malloc_strcat(&banstring, tmpc->ban);
		malloc_strcat(&banstring, " ");
		num++;
		tmpc->sent_unban++;
		if (num % get_int_var(NUM_BANMODES_VAR) == 0) {
		    send_to_server(SERVER(from_server), "MODE %s -%s %s", stuff, strfill('b', num), banstring);
		    new_free(&banstring);
		    num = 0;
		}
	    }
	if (banstring && num)
	    send_to_server(SERVER(from_server), "MODE %s -%s %s", stuff, strfill('b', num), banstring);
	for (tmpc = chan->bans; tmpc; tmpc = next) {
	    next = tmpc->next;
	    if (tmpc->sent_unban) {
		if ((tmpc = (BanList *) remove_from_list((struct list **) &chan->bans, tmpc->ban))) {
		    new_free(&tmpc->ban);
		    new_free(&tmpc->setby);
		    new_free(&tmpc);
		    tmpc = NULL;
		}
	    }
	}
	new_free(&banstring);
    }
}

void cmd_tban(struct command *cmd, char *args)
{
    struct channel *chan;
    int count = 1;
    BanList *tmpc;
    int server;

    if ((chan = prepare_command(&server, NULL, NEED_OP))) {

	if (!chan->bans) {
	    bitchsay("No bans on %s", chan->channel);
	    return;
	}
	if ((do_hook(BANS_HEADER_LIST, "%s %s %s %s %s", "#", "Channel", "Ban", "SetBy", "Seconds")))
	    put_it("%s", convert_output_format(get_fset_var(FORMAT_BANS_HEADER_FSET), NULL));
	for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++)
	    if (do_hook
		(BANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban,
		 tmpc->setby ? tmpc->setby : get_server_name(from_server), (unsigned long) tmpc->time))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_BANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban,
					     tmpc->setby ? tmpc->setby : get_server_name(from_server), tmpc->time));
	add_wait_prompt("Which ban to delete (-2, 2-5, ...) ? ", remove_bans, chan->channel, WAIT_PROMPT_LINE);
    }
}

static void set_default_bantype(char value, const char *helparg)
{
    int bant;

    switch (toupper(value)) {
    case 'B':
	bant = 2;
	break;
    case 'H':
	bant = 3;
	break;
    case 'D':
	bant = 4;
	break;
    case 'S':
	bant = 6;
	break;
    case 'U':
	bant = 5;
	break;
    case 'N':
	bant = 1;
	break;
    default:
	userage("BanType", helparg);
	return;
	break;
    }
    bitchsay("BanType set to %s",
	     (bant == 1) ? "\002N\002ormal" : (bant == 2) ? "\002B\002etter" : (bant == 3) ? "\002H\002ost" : (bant ==
													       4) ? "\002D\002omain"
	     : (bant == 5) ? "\002U\002ser" : "\002S\002crew");
    set_int_var(BANTYPE_VAR, bant);
}

void cmd_bantype(struct command *cmd, char *args)
{
    if (args && *args)
	set_default_bantype(*args, cmd->qhelp);
    else
	userage(cmd->name, cmd->qhelp);
}

void cmd_deop(struct command *cmd, char *args)
{
    char *to = NULL, *temp;
    int count, max;
    struct channel *chan;
    char buffer[BIG_BUFFER_SIZE + 1];
    int isvoice = 0;
    int server = from_server;

    count = 0;
    temp = NULL;
    max = get_int_var(NUM_OPMODES_VAR);

    *buffer = 0;

    if (cmd->rname)
	isvoice = 1;

    if (!(to = next_arg(args, &args)))
	to = NULL;
    if (to && !is_channel(to)) {
	temp = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&server, to, NEED_OP)))
	return;

    if (!temp)
	temp = next_arg(args, &args);

    while (temp && *temp) {
	count++;
	add_mode(chan, isvoice ? "v" : "o", 0, temp, NULL, max);
	temp = next_arg(args, &args);
    }
    flush_mode_all(chan);
}

void cmd_deoper(struct command *cmd, char *args)
{
    send_to_server(SERVER(from_server), "MODE %s -o", get_server_nickname(from_server));
}

void cmd_op(struct command *cmd, char *args)
{
    char *to = NULL, *temp = NULL;
    struct channel *chan = NULL;
    int count, max = get_int_var(NUM_OPMODES_VAR);
    char buffer[BIG_BUFFER_SIZE + 1];
    int old_server = from_server;
    int voice = 0;

    if (cmd->rname)
	voice = 1;
    count = 0;
    *buffer = 0;

    if (to && !is_channel(to)) {
	temp = to;
	to = NULL;
    }

    if (!(chan = prepare_command(&from_server, to, NEED_OP)))
	return;

    if (!temp)
	temp = next_arg(args, &args);

    while (temp && *temp) {
	count++;
	add_mode(chan, voice ? "v" : "o", 1, temp, NULL, max);
	temp = next_arg(args, &args);
    }
    flush_mode_all(chan);
    from_server = old_server;
}

static void oper_password_received(char *data, char *line)
{
    send_to_server(SERVER(from_server), "OPER %s %s", data, line);
}

void cmd_oper(struct command *cmd, char *args)
{
    char *password;
    char *nick;

    oper_command = 1;
    if (!(nick = next_arg(args, &args)))
	nick = get_server_nickname(from_server);
    if (!(password = next_arg(args, &args))) {
	add_wait_prompt("Operator Password:", oper_password_received, nick, WAIT_PROMPT_LINE);
	return;
    }
    send_to_server(SERVER(from_server), "OPER %s %s", nick, password);
}

void cmd_umode(struct command *cmd, char *args)
{
    send_to_server(SERVER(from_server), "%s %s %s", cmd->rname, get_server_nickname(from_server), (args && *args) ? args : empty_str);
}

void cmd_unkey(struct command *cmd, char *args)
{
    char *channel = NULL;
    int server = from_server;
    struct channel *chan;

    if (args)
	channel = next_arg(args, &args);
    if (!(chan = prepare_command(&server, channel, NEED_OP)))
	return;
    if (chan->key)
	send_to_server(SERVER(server), "MODE %s -k %s", chan->channel, chan->key);
}
void cmd_kill(struct command *cmd, char *args)
{
    char *reason = NULL;

    if ((reason = strchr(args, ' ')) != NULL)
	*reason++ = '\0';
    else if ((reason = get_reason(args)))
	reason = empty_str;

    if (!strcmp(args, "*")) {
	args = get_current_channel_by_refnum(0);
	if (!args || !*args)
	    args = "*";		/* what-EVER */
    }
    if (reason && *reason)
	send_to_server(SERVER(from_server), "%s %s :%s", cmd->name, args, reason);
    else
	send_to_server(SERVER(from_server), "%s %s", cmd->name, args);
}
