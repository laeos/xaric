
/*
 * names.c: This here is used to maintain a list of all the people currently
 * on your channel.  Seems to work 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "ircaux.h"
#include "names.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "lastlog.h"
#include "list.h"
#include "output.h"
#include "numbers.h"
#include "timer.h"
#include "input.h"
#include "hook.h"
#include "parse.h"
#include "whowas.h"
#include "misc.h"
#include "vars.h"
#include "status.h"
#include "hash.h"
#include "fset.h"

extern char *last_split_server;

static const char mode_str[] = "iklmnpsta";

static void add_to_mode_list(char *, int, char *);
static void check_mode_list_join(char *, int);
static void show_channel(struct channel *);
static void clear_channel(struct channel *);
char *recreate_mode(struct channel *);
static void clear_mode_list(int);
static int decifer_mode(char *, char *, struct channel **, unsigned long *, char *, char *, char **);
char *channel_key(char *);
void clear_bans(struct channel *);

static struct modelist {
    char *chan;
    int server;
    char *mode;
    struct modelist *next;
} *mode_list = NULL;

static struct joinlist {
    char *chan;
    int server, gotinfo, winref;
    struct timeval tv;
    struct joinlist *next;
} *join_list = NULL;

/* clear_channel: erases all entries in a nick list for the given channel */
static void clear_channel(struct channel *chan)
{
    struct nick_list *Nick, *n;

    while ((Nick = next_nicklist(chan, NULL))) {
	n = find_nicklist_in_channellist(Nick->nick, chan, REMOVE_FROM_LIST);
	add_to_whowas_buffer(n, chan->channel, NULL, NULL);
    }
    clear_nicklist_hashtable(chan);
    chan->totalnicks = 0;
}

struct channel *lookup_channel(char *channel, int server, int unlink)
{
    register struct channel *chan = NULL, *last = NULL;

    if (server == -1)
	server = primary_server;
    if (server == -1)
	return NULL;

    if (!channel || !*channel || !strcmp(channel, "*"))
	channel = get_current_channel_by_refnum(0);
    if (!channel)
	return NULL;
    chan = server_list[server].chan_list;
    if (!chan)
	return NULL;
    while (chan) {
	if (chan->server == server && !my_stricmp(chan->channel, channel)) {
	    if (unlink == CHAN_UNLINK) {
		if (last)
		    last->next = chan->next;
		else
		    server_list[server].chan_list = chan->next;
	    }
	    break;
	}
	last = chan;
	chan = chan->next;
    }
    return chan;
}

void set_waiting_channel(int i)
{
    Window *tmp = NULL;

    while (traverse_all_windows(&tmp))
	if (tmp->server == i && tmp->current_channel) {
	    if (tmp->bind_channel)
		tmp->waiting_channel = tmp->current_channel;
	    else
		new_free(&tmp->current_channel);
	    tmp->current_channel = NULL;
	}
}

/* if the user is on the given channel, it returns 1. */
int im_on_channel(char *channel)
{
    return (channel ? (lookup_channel(channel, from_server, 0) ? 1 : 0) : 0);
}

void add_userhost_to_channel(char *channel, char *nick, int server, char *userhost)
{
    struct nick_list *new;
    struct channel *chan;

    if ((chan = lookup_channel(channel, server, 0)) != NULL)
	if ((new = find_nicklist_in_channellist(nick, chan, 0)))
	    malloc_strcpy(&new->host, userhost);
}

/*
 * add_channel: adds the named channel to the channel list.  If the channel
 * is already in the list, then the channel gets cleaned, and ready for use
 * again.   The added channel becomes the current channel as well.
 */
struct channel *add_channel(char *channel, int server)
{
    struct channel *new = NULL;
    struct whowas_chan_list *whowaschan;

    if ((new = lookup_channel(channel, server, CHAN_NOUNLINK)) != NULL) {
	new->connected = 1;
	new->mode = 0;
	new->limit = 0;
	new_free(&new->s_mode);
	new->server = server;
	new->window = curr_scr_win;
	malloc_strcpy(&(new->channel), channel);
	clear_channel(new);
	clear_bans(new);
    } else {
	if (!(whowaschan = check_whowas_chan_buffer(channel, 1))) {
	    new = (struct channel *) new_malloc(sizeof(struct channel));
	    new->connected = 1;
	    get_time(&new->channel_create);
	    malloc_strcpy(&(new->channel), channel);
	} else {
	    new = whowaschan->channellist;
	    new_free(&whowaschan->channel);
	    new_free((char **) &whowaschan);
	    new_free(&(new->key));
	    new->mode = 0;
	    new_free(&new->s_mode);
	    new->limit = new->i_mode = 0;
	    clear_channel(new);
	    clear_bans(new);
	}
	new->window = get_window_by_refnum(get_win_from_join_list(channel, server));	/* curr_scr_win; */
	new->server = server;
	new->flags.got_modes = new->flags.got_who = new->flags.got_bans = 1;
	get_time(&new->join_time);
	add_to_list((struct list **) &(server_list[server].chan_list), (struct list *) new);
    }
    new->chop = 0;
    new->voice = 0;

    if (!is_current_channel(channel, server, 0)) {
	Window *tmp;
	Window *wind = NULL;

	tmp = NULL;
	while (traverse_all_windows(&tmp)) {
	    if (tmp->server != from_server)
		continue;
	    if (!wind)
		wind = tmp;
	    if (!tmp->waiting_channel && !tmp->bind_channel)
		continue;
	    if (tmp->bind_channel && !my_stricmp(tmp->bind_channel, channel) && tmp->server == from_server) {
		set_current_channel_by_refnum(tmp->refnum, channel);
		new->window = tmp;
		update_all_windows();
		return new;
	    }
	    if (tmp->waiting_channel && !my_stricmp(tmp->waiting_channel, channel) && tmp->server == from_server) {
		set_current_channel_by_refnum(tmp->refnum, channel);
		new->window = tmp;
		new_free(&tmp->waiting_channel);
		update_all_windows();
		return new;
	    }
	}
	set_current_channel_by_refnum(new->window ? new->window->refnum : 0, channel);
	new->window = curr_scr_win;
    }
    update_all_windows();
    return new;
}

/*
 * add_to_channel: adds the given nickname to the given channel.  If the
 * nickname is already on the channel, nothing happens.  If the channel is
 * not on the channel list, nothing happens (although perhaps the channel
 * should be addded to the list?  but this should never happen) 
 */
struct channel *add_to_channel(char *channel, char *nick, int server, int oper, int voice, char *userhost, char *server1, char *away)
{
    struct nick_list *new = NULL;
    struct channel *chan = NULL;
    struct whowas_list *whowas;

    int ischop = oper;

    if (!(*channel == '*') && (chan = lookup_channel(channel, server, CHAN_NOUNLINK))) {
	if (*nick == '+') {
	    nick++;
	    if (!my_stricmp(nick, get_server_nickname(server))) {
		check_mode_list_join(channel, server);
		chan->voice = 1;
	    }
	}
	if (ischop) {
	    if (!my_stricmp(nick, get_server_nickname(server))) {
		check_mode_list_join(channel, server);
		chan->chop = 1;
	    }
	}
	if (!(new = find_nicklist_in_channellist(nick, chan, 0))) {
	    if (!(whowas = check_whowas_buffer(nick, userhost ? userhost : "<UNKNOWN>", channel, 1))) {
		new = (struct nick_list *) new_malloc(sizeof(struct nick_list));

		new->idle_time = new->kicktime = new->doptime = new->nicktime = new->floodtime = new->bantime = time(NULL);

		new->joincount = 1;
		malloc_strcpy(&(new->nick), nick);
		if (userhost)
		    malloc_strcpy(&new->host, userhost);
		else
		    malloc_strcpy(&new->host, "<UNKNOWN>");
	    } else {
		new = whowas->nicklist;
		new_free(&whowas->channel);
		new_free((char **) &whowas);
		malloc_strcpy(&(new->nick), nick);

		new->idle_time = new->kicktime = new->doptime = new->nicktime = new->floodtime = new->bantime = time(NULL);

		new->sent_reop = new->sent_deop =
		    new->bancount = new->nickcount = new->dopcount = new->kickcount = new->floodcount = 0;
		new->next = NULL;
	    }
	    add_nicklist_to_channellist(new, chan);
	}
	new->chanop = ischop;
	new->voice = voice;
	if (userhost && (strcmp(new->host, "<UNKNOWN>") == 0))
	    malloc_strcpy(&(new->host), userhost);
	if (away) {
	    new->away = *away;
	    new->ircop = *(away + 1) == '*';
	}
    }
    return chan;
}

char *get_channel_key(char *channel, int server)
{
    struct channel *tmp;

    if ((tmp = lookup_channel(channel, server, 0)) != NULL)
	return (tmp->key ? tmp->key : empty_str);
    return empty_str;
}

/*
 * recreate_mode: converts the bitmap representation of a channels mode into
 * a string 
 *
 * This malloces what it returns, but nothing that calls this function
 * is expecting to have to free anything.  Therefore, this function
 * should not malloc what it returns.  (hop)
 *
 * but this leads to horrible race conditions, so we add a bit to
 * the channel structure, and cache the string value of mode, and
 * the u_long value of the cached string, so that each channel only
 * has one copy of the string.  -mrg, june '94.
 */
char *recreate_mode(struct channel *chan)
{
    int mode_pos = 0, mode;
    static char *s;
    char buffer[BIG_BUFFER_SIZE + 1];

    chan->i_mode = chan->mode;
    buffer[0] = 0;
    s = buffer;
    mode = chan->mode;
    if (!mode)
	return NULL;
    while (mode) {
	if (mode % 2)
	    *s++ = mode_str[mode_pos];
	mode /= 2;
	mode_pos++;
    }
    if (chan->key && *chan->key) {
	*s++ = ' ';
	strcpy(s, chan->key);
	s += strlen(chan->key);
    }
    if (chan->limit)
	sprintf(s, " %d", chan->limit);
    else
	*s = 0;

    malloc_strcpy(&chan->s_mode, buffer);
    return chan->s_mode;
}

#ifdef COMPRESS_MODES
/* some structs to help along the process, struct nick_list is way too much of a memory
   hog */

typedef struct _UserChanModes {
    struct _UserChanModes *next;
    char *nick;
    int o_ed;
    int v_ed;
    int deo_ed;
    int dev_ed;
    int b_ed;
    int deb_ed;
} UserChanModes;

/*
 * compress_modes: This will return a list of modes which removes duplicates
 * 
 * for instance:
 * +oooo Jordy Jordy Jordy Jordy
 *
 * would return:
 * +o Jordy
 *
 * +oo-oo Sheik Jordy Sheik Jordy
 *
 * would return:
 * -oo Sheik jordy
 *
 * also if *!*@* isn't banned on the channel
 * -b *!*@*
 *
 * will return NULL
 *
 * This is a frigging ugly function and because I'm not going to sit and
 * learn every shortcut function in irc, I don't care :)
 *
 * Written by Jordy (jordy@wserv.com)
 */
char *compress_modes(int server, char *channel, char *modes)
{
    int add = 0, isbanned = 0, isopped = 0, isvoiced = 0, mod = -1;
    char *tmp, *rest, nmodes[16], nargs[100];
    UserChanModes *ucm = NULL, *tucm = NULL;
    BanList *tbl = NULL;
    struct nick_list *tnl = NULL;
    struct channel *chan;

    /* now, modes contains the actual modes, and rest contains the arguments to those modes */

    chan = lookup_channel(channel, server, CHAN_NOUNLINK);

    if (!chan || !(modes = next_arg(modes, &rest)))
	return NULL;

    strcpy(nmodes, "");
    strcpy(nargs, "");

    for (; *modes; modes++) {
	switch (*modes) {
	case '+':
	    add = 1;
	    break;
	case '-':
	    add = 0;
	    break;
	case 'o':
	    tmp = next_arg(rest, &rest);
	    tucm = (UserChanModes *) find_in_list((struct list **) &ucm, tmp, 0);

/*                  tnl = (struct nick_list *)find_in_list((struct list **)&chan->nicks, tmp, 0); */
	    tnl = find_nicklist_in_channellist(tmp, chan, 0);
	    if (tnl && tnl->chanop)
		isopped = 1;
	    else if (tnl)
		isopped = 0;

	    if (tucm) {
		if (add && !isopped) {
		    tucm->o_ed = 1;
		    tucm->deo_ed = 0;
		} else if (!add && isopped) {
		    tucm->o_ed = 0;
		    tucm->deo_ed = 1;
		} else
		    break;
	    } else {
		tucm = (UserChanModes *) new_malloc(sizeof(UserChanModes));
		malloc_strcpy(&tucm->nick, tmp);
		if (add && !isopped) {
		    tucm->o_ed = 1;
		    tucm->deo_ed = 0;
		} else if (!add & isopped) {
		    tucm->o_ed = 0;
		    tucm->deo_ed = 1;
		} else
		    break;
		tucm->b_ed = tucm->deb_ed = 0;
		tucm->v_ed = tucm->dev_ed = 0;
		add_to_list((struct list **) &ucm, (struct list *) tucm);
	    }
	    break;
	case 'v':
	    tmp = next_arg(rest, &rest);
	    tucm = (UserChanModes *) find_in_list((struct list **) &ucm, tmp, 0);

/*                  tnl = (struct nick_list *)find_in_list((struct list **)&chan->nicks, tmp, 0); */
	    tnl = find_nicklist_in_channellist(tmp, chan, 0);
	    if (tnl && tnl->voice)
		isvoiced = 1;
	    else if (tnl)
		isvoiced = 0;

	    if (tucm) {
		if (add && !isvoiced) {
		    tucm->v_ed = 1;
		    tucm->dev_ed = 0;
		} else if (!add & isvoiced) {
		    tucm->v_ed = 0;
		    tucm->dev_ed = 1;
		} else
		    break;
	    } else {
		tucm = (UserChanModes *) new_malloc(sizeof(UserChanModes));
		malloc_strcpy(&tucm->nick, tmp);
		if (add && !isvoiced) {
		    tucm->v_ed = 1;
		    tucm->dev_ed = 0;
		} else if (!add && isvoiced) {
		    tucm->v_ed = 0;
		    tucm->dev_ed = 1;
		}
		break;
		tucm->o_ed = tucm->deo_ed = 0;
		tucm->b_ed = tucm->deb_ed = 0;
		add_to_list((struct list **) &ucm, (struct list *) tucm);
	    }
	    break;
	case 'b':
	    tmp = next_arg(rest, &rest);
	    tucm = (UserChanModes *) find_in_list((struct list **) &ucm, tmp, 0);
	    isbanned = 0;
	    for (tbl = chan->bans; tbl && !isbanned; tbl = tbl->next) {
		if (!my_stricmp(tbl->ban, tmp)) {
		    isbanned = 1;
		} else {
		    isbanned = 0;
		}
	    }

	    if (tucm) {
		if (add && !isbanned) {
		    tucm->b_ed = 1;
		    tucm->deb_ed = 0;
		} else if (!add && isbanned) {
		    tucm->b_ed = 0;
		    tucm->deb_ed = 1;
		} else
		    break;
	    } else {
		tucm = (UserChanModes *) new_malloc(sizeof(UserChanModes));
		malloc_strcpy(&tucm->nick, tmp);
		if (add && !isbanned) {
		    tucm->b_ed = 1;
		    tucm->deb_ed = 0;
		} else if (!add && isbanned) {
		    tucm->b_ed = 0;
		    tucm->deb_ed = 1;
		} else
		    break;
		tucm->o_ed = tucm->deo_ed = 0;
		tucm->v_ed = tucm->dev_ed = 0;
		add_to_list((struct list **) &ucm, (struct list *) tucm);
	    }
	    break;
	case 'l':
	    if (add) {
		tmp = next_arg(rest, &rest);

		if (mod == 1) {
		    strcat(nmodes, "l");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		} else {
		    strcat(nmodes, "+l");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "l");
		} else {
		    strcat(nmodes, "-l");
		    mod = 0;
		}
	    }
	    break;
	case 'k':
	    tmp = next_arg(rest, &rest);

	    if (add) {
		if (mod == 1) {
		    strcat(nmodes, "k");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		} else {
		    strcat(nmodes, "+k");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "k");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		} else {
		    strcat(nmodes, "-k");
		    strcat(nargs, " ");
		    strcat(nargs, tmp);
		    mod = 0;
		}
	    }
	    break;
	case 'i':
	    if (add) {
		if (mod == 1) {
		    strcat(nmodes, "i");
		} else {
		    strcat(nmodes, "+i");
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "i");
		} else {
		    strcat(nmodes, "-i");
		    mod = 0;
		}
	    }
	    break;
	case 'n':
	    if (add) {
		if (mod == 1) {
		    strcat(nmodes, "n");
		} else {
		    strcat(nmodes, "+n");
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "n");
		} else {
		    strcat(nmodes, "-n");
		    mod = 0;
		}
	    }
	    break;
	case 's':
	    if (add) {
		if (mod == 1) {
		    strcat(nmodes, "s");
		} else {
		    strcat(nmodes, "+s");
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "s");
		} else {
		    strcat(nmodes, "-s");
		    mod = 0;
		}
	    }
	    break;
	case 't':
	    if (add) {
		if (mod == 1) {
		    strcat(nmodes, "t");
		} else {
		    strcat(nmodes, "+t");
		    mod = 1;
		}
	    } else {
		if (mod == 0) {
		    strcat(nmodes, "t");
		} else {
		    strcat(nmodes, "-t");
		    mod = 0;
		}
	    }
	    break;
	}
    }

    /* modes which can be done multiple times are added here */

    for (tucm = ucm; tucm; tucm = tucm->next) {
	if (tucm->o_ed) {
	    if (mod == 1) {
		strcat(nmodes, "o");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "+o");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 1;
	    }
	} else if (tucm->deo_ed) {
	    if (mod == 0) {
		strcat(nmodes, "o");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "-o");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 0;
	    }
	}
	if (tucm->v_ed) {
	    if (mod == 1) {
		strcat(nmodes, "v");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "+v");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 1;
	    }
	} else if (tucm->dev_ed) {
	    if (mod == 0) {
		strcat(nmodes, "v");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "-v");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 0;
	    }
	}
	if (tucm->b_ed) {
	    if (mod == 1) {
		strcat(nmodes, "b");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "+b");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 1;
	    }
	} else if (tucm->deb_ed) {
	    if (mod == 0) {
		strcat(nmodes, "b");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
	    } else {
		strcat(nmodes, "-b");
		strcat(nargs, " ");
		strcat(nargs, tucm->nick);
		mod = 0;
	    }
	}
    }

    if (strlen(nmodes) || strlen(nargs)) {
	return m_sprintf("%s%s", nmodes, nargs);
    }
    return NULL;
}
#endif

/*
 * decifer_mode: This will figure out the mode string as returned by mode
 * commands and convert that mode string into a one byte bit map of modes 
 */
static int
decifer_mode(char *from, register char *mode_string, struct channel **channel, unsigned long *mode, char *chop, char *voice,
	     char **key)
{

    char *limit = 0;
    char *person;
    int add = 0;
    int limit_set = 0;
    int limit_reset = 0;
    int splitter = 0;
    char *rest, *the_key;

    struct nick_list *ThisNick = NULL;
    BanList *new;
    unsigned int value = 0;
    int its_me = 0;

    if (!(mode_string = next_arg(mode_string, &rest)))
	return -1;

    its_me = !my_stricmp(from, get_server_nickname(from_server)) ? 1 : 0;
    splitter = match("*.*.*", from);

    for (; *mode_string; mode_string++) {
	switch (*mode_string) {
	case '+':
	    add = 1;
	    value = 0;
	    break;
	case '-':
	    add = 0;
	    value = 0;
	    break;
	case 'p':
	    value = MODE_PRIVATE;
	    break;
	case 'l':
	    value = MODE_LIMIT;
	    if (add) {
		limit_set = 1;
		if (!(limit = next_arg(rest, &rest)))
		    limit = empty_str;
		else if (!strncmp(limit, zero_str, 1))
		    limit_reset = 1, limit_set = 0, add = 0;
	    } else
		limit_reset = 1;
	    break;
	case 'a':
	    value = MODE_ANON;
	    break;
	case 't':
	    value = MODE_TOPIC;
	    break;
	case 'i':
	    value = MODE_INVITE;
	    break;
	case 'n':
	    value = MODE_MSGS;
	    break;
	case 's':
	    value = MODE_SECRET;
	    break;
	case 'm':
	    value = MODE_MODERATED;
	    break;
	case 'o':
	    {
		if (!(person = next_arg(rest, &rest)))
		    break;
		if (!my_stricmp(person, get_server_nickname(from_server))) {
		    if (add && add != (*channel)->chop && !in_join_list((*channel)->channel, from_server)) {
			*chop = (*channel)->chop = add;
			flush_mode_all(*channel);
		    } else {
			if (!add && add != (*channel)->chop && !in_join_list((*channel)->channel, from_server)) {
			    register struct nick_list *tmp;

			    for (tmp = next_nicklist(*channel, NULL); tmp; tmp = next_nicklist(*channel, tmp))
				tmp->sent_reop = tmp->sent_deop = 0;
			}
		    }
		    *chop = add;
		    (*channel)->chop = add;
		}
		ThisNick = find_nicklist_in_channellist(from, *channel, 0);
		if ((ThisNick = find_nicklist_in_channellist(person, *channel, 0)))
		    ThisNick->chanop = add;
/* Sergs_ sent a patch for this */
		if (ThisNick) {
		    if (add) {
			if (ThisNick->sent_reop)
			    ThisNick->sent_reop--;
		    } else if (ThisNick->sent_deop)
			ThisNick->sent_deop--;
		}
		break;
	    }
	case 'k':
	    value = MODE_KEY;
	    the_key = next_arg(rest, &rest);
	    if (add)
		malloc_strcpy(key, the_key);
	    else
		new_free(key);
	    break;
	case 'v':
	    if ((person = next_arg(rest, &rest))) {
		if ((ThisNick = find_nicklist_in_channellist(person, *channel, 0)))
		    ThisNick->voice = add;
		if (!my_stricmp(person, get_server_nickname(from_server)))
		    (*channel)->voice = add;
	    }
	    break;
	case 'b':
	    person = next_arg(rest, &rest);
	    if (!person || !*person)
		break;
	    ThisNick = find_nicklist_in_channellist(from, *channel, 0);
	    if (add) {
		ThisNick = find_nicklist_in_channellist(person, *channel, 0);
		if (!(new = (BanList *) find_in_list((struct list **) &(*channel)->bans, person, 0)) || my_stricmp(person, new->ban)) {
		    new = (BanList *) new_malloc(sizeof(BanList));
		    malloc_strcpy(&new->ban, person);
		    add_to_list((struct list **) &(*channel)->bans, (struct list *) new);
		}
		new->sent_unban = 0;
		if (!new->setby)
		    malloc_strcpy(&new->setby, from ? from : get_server_name(from_server));
		new->time = time(NULL);
	    } else {
		if ((new = (BanList *) remove_from_list((struct list **) &(*channel)->bans, person))) {
		    new_free(&new->setby);
		    new_free(&new->ban);
		    new_free((char **) &new);
		}
	    }
	    break;
	}
	if (add)
	    *mode |= value;
	else
	    *mode &= ~value;
    }
    flush_mode_all(*channel);
    if (limit_set)
	return (atoi(limit));
    else if (limit_reset)
	return (0);
    else
	return (-1);
}

/*
 * get_channel_mode: returns the current mode string for the given channel
 */
char *get_channel_mode(char *channel, int server)
{
    struct channel *tmp;

    if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)))
	return recreate_mode(tmp);
    return empty_str;
}

/*
 * update_channel_mode: This will modify the mode for the given channel
 * according the the new mode given.  
 */
void update_channel_mode(char *from, char *channel, int server, char *mode, struct channel *tmp)
{
    int limit;

    if (tmp || (channel && (tmp = lookup_channel(channel, server, CHAN_NOUNLINK)))) {
	if ((limit = decifer_mode(from, mode, &(tmp), &(tmp->mode), &(tmp->chop), &(tmp->voice), &(tmp->key))) != -1)
	    tmp->limit = limit;
    }
}

/*
 * is_channel_mode: returns the logical AND of the given mode with the
 * channels mode.  Useful for testing a channels mode 
 */
int is_channel_mode(char *channel, int mode, int server_index)
{
    struct channel *tmp;

    if ((tmp = lookup_channel(channel, server_index, CHAN_NOUNLINK)))
	return (tmp->mode & mode);
    return 0;
}

void clear_bans(struct channel *channel)
{
    BanList *bans, *next;

    if (!channel || !channel->bans)
	return;
    for (bans = channel->bans; bans; bans = next) {
	next = bans->next;
	new_free(&bans->setby);
	new_free(&bans->ban);
	new_free((char **) &bans);
    }
    channel->bans = NULL;
}

/*
 * remove_channel: removes the named channel from the
 * server_list[server].chan_list.  If the channel is not on the
 * server_list[server].chan_list, nothing happens.  If the channel was
 * the current channel, this will select the top of the
 * server_list[server].chan_list to be the current_channel, or 0 if the
 * list is empty. 
 */

void remove_channel(char *channel, int server)
{
    struct channel *tmp;
    int old_from_server = from_server;

    if (channel) {
	from_server = server;
	from_server = old_from_server;
	if ((tmp = lookup_channel(channel, server, CHAN_UNLINK))) {
	    clear_bans(tmp);
	    clear_channel(tmp);
	    add_to_whowas_chan_buffer(tmp);
	}
	if (is_current_channel(channel, server, 1))
	    switch_channels(0, NULL);
    } else {
	struct channel *next;

	for (tmp = server_list[server].chan_list; tmp; tmp = next) {
	    next = tmp->next;
	    clear_channel(tmp);
	    clear_bans(tmp);
	    add_to_whowas_chan_buffer(tmp);
	}
	server_list[server].chan_list = NULL;
    }
    update_all_windows();
}

/*
 * remove_from_channel: removes the given nickname from the given channel. If
 * the nickname is not on the channel or the channel doesn't exist, nothing
 * happens. 
 */
void remove_from_channel(char *channel, char *nick, int server, int netsplit, char *reason)
{
    struct channel *chan;
    struct nick_list *tmp = NULL;
    char buf[BIG_BUFFER_SIZE + 1];
    char *server1 = NULL, *server2 = NULL;

    if (netsplit && reason) {
	char *p = NULL;

	strncpy(buf, reason, sizeof(buf) - 1);
	if ((p = strchr(buf, ' '))) {
	    *p++ = '\0';
	    server2 = buf;
	    server1 = p;
	}
	if (server1 && !check_split_server(server1)) {
	    add_split_server(server1, server2, 0);
	    message_from(channel, LOG_CRAP);
	    malloc_strcpy(&last_split_server, server1);
	    if (do_hook(LLOOK_SPLIT_LIST, "%s %s", server2, server1)) {
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_NETSPLIT_FSET), "%s %s %s", update_clock(GET_TIME), server1,
					     server2));
		bitchsay("\002Ctrl-F\002 to see who left \002Ctrl-E\002 to change to [%s]", server1);
	    }
	    message_from(NULL, LOG_CRAP);

	}
    }

    if (channel) {
	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
	    if ((tmp = find_nicklist_in_channellist(nick, chan, REMOVE_FROM_LIST))) {
		add_to_whowas_buffer(tmp, channel, server1, server2);
		if (netsplit && tmp->nick)
		    add_to_whosplitin_buffer(tmp, channel, server1, server2);
	    }
    } else {
	for (chan = server_list[server].chan_list; chan; chan = chan->next)
	    if ((tmp = find_nicklist_in_channellist(nick, chan, REMOVE_FROM_LIST))) {
		if (tmp->nick)
		    add_to_whowas_buffer(tmp, chan->channel, server1, server2);

		if (netsplit && tmp->nick)
		    add_to_whosplitin_buffer(tmp, chan->channel, server1, server2);
	    }
    }
}

void
handle_nickflood(char *old_nick, char *new_nick, register struct nick_list *nick, register struct channel *chan, time_t current_time,
		 int flood_time)
{
    if ((!nick->bancount) || (nick->bancount && ((current_time - nick->bantime) > 3))) {
	if (!nick->kickcount++)
	    send_to_server(SERVER(from_server), "KICK %s %s :\002Nick flood (%d nicks in %dsecs of %dsecs)\002", chan->channel, new_nick, get_int_var(KICK_ON_NICKFLOOD_VAR)	/* chan->set_kick_on_nickflood 
			    */ , flood_time, get_int_var(NICKFLOOD_TIME_VAR) /* chan->set_nickflood_time */ );
    }
}

/*
 * rename_nick: in response to a changed nickname, this looks up the given
 * nickname on all you channels and changes it the new_nick 
 */
void rename_nick(char *old_nick, char *new_nick, int server)
{
    register struct channel *chan;
    register struct nick_list *tmp;
    time_t current_time = time(NULL);
    int t = 0;

    for (chan = server_list[server].chan_list; chan; chan = chan->next) {
	if ((chan->server == server)) {
	    if ((tmp = find_nicklist_in_channellist(old_nick, chan, REMOVE_FROM_LIST))) {
		tmp->stat_nicks++;
		if (chan->chop) {
		    if (is_other_flood(chan, tmp, NICK_FLOOD, &t))
			handle_nickflood(old_nick, new_nick, tmp, chan, current_time, t);
		}
		malloc_strcpy(&tmp->nick, new_nick);
		add_nicklist_to_channellist(tmp, chan);
	    }
	}
    }
}

/*
 * is_on_channel: returns true if the given nickname is in the given channel,
 * false otherwise.  Also returns false if the given channel is not on the
 * channel list. 
 */
int is_on_channel(char *channel, int server, char *nick)
{
    struct channel *chan;

    if (nick && (chan = lookup_channel(channel, server, CHAN_NOUNLINK)) && chan->connected)
	/* if (find_nicklist_in_channellist(nick, chan, 0)) */
	return 1;
    return 0;
}

int is_chanop(char *channel, char *nick)
{
    struct channel *chan;
    struct nick_list *Nick;

    if (nick && (chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)) /* && chan->connected */ ) {
	if ((Nick = find_nicklist_in_channellist(nick, chan, 0)))
	    if (Nick->chanop)
		return 1;
    }
    return 0;
}

static void show_channel(struct channel *chan)
{
    struct nick_list *tmp;
    int buffer_len;
    char *nicks = NULL;
    char *s;
    char buffer[BIG_BUFFER_SIZE + 1];

    *buffer = 0;
    buffer_len = 0;

    s = recreate_mode(chan);
    for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp)) {
	if (buffer_len >= (BIG_BUFFER_SIZE / 2)) {
	    malloc_strcpy(&nicks, buffer);
	    say("\t%s %c%s (%s): %s", chan->channel, s ? '+' : ' ', s ? s : "<none>", get_server_name(chan->server), nicks);
	    *buffer = (char) 0;
	    buffer_len = 0;
	}
	buffer_len += strlen(tmp->nick) + 1;
	strmcat(buffer, tmp->nick, BIG_BUFFER_SIZE);
	if (tmp->host) {
	    buffer_len += strlen(tmp->host) + 1;
	    strmcat(buffer, "!", BIG_BUFFER_SIZE);
	    strmcat(buffer, tmp->host, BIG_BUFFER_SIZE);
	}
	strmcat(buffer, " ", BIG_BUFFER_SIZE);
    }
    malloc_strcpy(&nicks, buffer);
    say("\t%s %c%s (%s): %s", chan->channel, s ? '+' : ' ', s ? s : "<none>", get_server_name(chan->server), nicks);
    new_free(&nicks);
}

/* list_channels: displays your current channel and your channel list */
void list_channels(void)
{
    struct channel *tmp;
    int server, no = 1;

    if (server_list[from_server].chan_list) {
	if (get_current_channel_by_refnum(0))
	    say("Current channel %s", get_current_channel_by_refnum(0));
	else
	    say("No current channel for this window");
	if ((tmp = server_list[get_window_server(0)].chan_list)) {
	    for (; tmp; tmp = tmp->next)
		show_channel(tmp);
	    no = 0;
	}
	if (connected_to_server != 1) {
	    for (server = 0; server < number_of_servers; server++) {
		if (server == get_window_server(0))
		    continue;
		say("Other servers:");
		for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next)
		    show_channel(tmp);
		no = 0;
	    }
	}
    } else
	say("You are not on any channels");
}

void switch_channels(char key, char *ptr)
{
    struct channel *tmp;

    if (server_list[from_server].chan_list) {
	if (get_current_channel_by_refnum(0)) {
	    if ((tmp = lookup_channel(get_current_channel_by_refnum(0), from_server, CHAN_NOUNLINK))) {
		for (tmp = tmp->next; tmp; tmp = tmp->next) {
		    if ((tmp->server == from_server) && !is_current_channel(tmp->channel, from_server, 0)
			/* && !is_bound(tmp->channel, from_server) && curr_scr_win == tmp->window */
			) {
			set_current_channel_by_refnum(0, tmp->channel);
			update_all_windows();
			update_all_status(curr_scr_win, NULL, 0);
			set_input_prompt(curr_scr_win, get_string_var(INPUT_PROMPT_VAR), 0);
			update_input(UPDATE_ALL);
			do_hook(CHANNEL_SWITCH_LIST, "%s", tmp->channel);
			return;
		    }
		}
	    }
	}
	for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next) {
	    if ((tmp->server == from_server) && !is_current_channel(tmp->channel, from_server, 0) &&	/* !(is_bound(tmp->channel,
													 * from_server)) && */ curr_scr_win == tmp->window) {
		set_current_channel_by_refnum(0, tmp->channel);
		update_all_windows();
		update_all_status(curr_scr_win, NULL, 0);
		set_input_prompt(curr_scr_win, get_string_var(INPUT_PROMPT_VAR), 0);
		update_input(UPDATE_ALL);
		do_hook(CHANNEL_SWITCH_LIST, "%s", tmp->channel);
		return;
	    }
	}
    }
}

/* real_channel: returns your "real" channel (your non-multiple channel) */
char *real_channel(void)
{
    struct channel *tmp;

    if (server_list[from_server].chan_list)
	for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next)
	    if (tmp->server == from_server && *(tmp->channel) != '#')
		return (tmp->channel);
    return (NULL);
}

void change_server_channels(int old, int new)
{
    struct channel *tmp;

    if (new == old)
	return;
    if (new > -1 && server_list[new].chan_list)
	server_list[new].chan_list->server = new;
    if (new > -1 && old > -1) {
	tmp = server_list[old].chan_list;
	server_list[new].chan_list = tmp;
	server_list[old].chan_list = NULL;
    }
}

void clear_channel_list(int server)
{
    struct channel *tmp, *next;
    Window *ptr = NULL;

    while (traverse_all_windows(&ptr))
	if (ptr->server == server && ptr->current_channel)
	    new_free(&ptr->current_channel);

    for (tmp = server_list[server].chan_list; tmp; tmp = next) {
	next = tmp->next;
	clear_channel(tmp);
	add_to_whowas_chan_buffer(tmp);
    }

    server_list[server].chan_list = NULL;
    clear_mode_list(server);
    return;
}

/*
 * reconnect_all_channels: used after you get disconnected from a server, 
 * clear each channel nickname list and re-JOINs each channel in the 
 * channel_list ..  
 */
void reconnect_all_channels(int server)
{
    struct channel *tmp;
    char *mode;
    char *channels = NULL;
    char *keys = NULL;

    for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next) {
	if ((mode = recreate_mode(tmp)))
	    add_to_mode_list(tmp->channel, server, mode);
/*
   send_to_server("JOIN %s%s%s", tmp->channel, tmp->key ? " " : "", tmp->key ? tmp->key : "");
 */
	add_to_join_list(tmp->channel, server, tmp->window->refnum);
	m_s3cat(&channels, ",", tmp->channel);
	if (tmp->key)
	    m_s3cat(&keys, ",", tmp->key);
	clear_channel(tmp);
	clear_bans(tmp);
    }
    if (channels)
	send_to_server(SERVER(from_server), "JOIN %s %s", channels, keys ? keys : empty_str);

    new_free(&channels);
    new_free(&keys);

    clear_channel_list(from_server);
/*      window_check_servers(); */
    message_from(NULL, LOG_CRAP);
}

char *what_channel(char *nick, int server)
{
    struct channel *tmp;

    /* 
     * if (curr_scr_win->current_channel && is_on_channel(curr_scr_win->current_channel, curr_scr_win->server, nick)) return
     * curr_scr_win->current_channel; */
    for (tmp = server_list[from_server].chan_list; tmp; tmp = tmp->next) {
	if (find_nicklist_in_channellist(nick, tmp, 0))
	    return tmp->channel;
    }
    return NULL;
}

char *walk_channels(char *nick, int init, int server)
{
    static struct channel *tmp = NULL;

    if (init)
	tmp = server_list[server].chan_list;
    else if (tmp)
	tmp = tmp->next;
    for (; tmp; tmp = tmp->next) {
	if ((tmp->server == from_server) && (find_nicklist_in_channellist(nick, tmp, 0)))
	    return (tmp->channel);
    }
    return NULL;
}

int get_channel_oper(char *channel, int server)
{
    struct channel *chan;

    if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
	return chan->chop;
    return 1;
}

char *fetch_userhost(int server, char *nick)
{
/*
   struct channel *tmp = NULL;
   struct nick_list *user = NULL;

   for (tmp = channel_list; tmp; tmp = tmp->next)
   {
   if ((tmp->server == server) && 
   ((user = (struct nick_list *)find_in_list((struct list **)&tmp->nicks, nick, 0))))
   return user->userhost;
   }
 */
    return NULL;
}

int get_channel_voice(char *channel, int server)
{
    struct channel *chan;

    if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
	return chan->voice;
    return 1;
}

void set_channel_window(Window * window, char *channel, int server)
{
    struct channel *tmp;

    if (!channel || server < 0)
	return;
    for (tmp = server_list[server].chan_list; tmp; tmp = tmp->next) {
	if (!my_stricmp(channel, tmp->channel) && tmp->server == server) {
	    tmp->window = window;
	    return;
	}
    }
}

char *create_channel_list(Window * window)
{
    struct channel *tmp;
    char buffer[BIG_BUFFER_SIZE + 1];

    *buffer = 0;
    for (tmp = server_list[window->server].chan_list; tmp; tmp = tmp->next) {
	if (tmp->server == from_server) {
	    strmcat(buffer, tmp->channel, BIG_BUFFER_SIZE);
	    strmcat(buffer, " ", BIG_BUFFER_SIZE);
	}
    }
    return m_strdup(buffer);
}

void channel_server_delete(int server)
{
    struct channel *tmp;
    int i;

    for (i = server + 1; i < number_of_servers; i++)
	for (tmp = server_list[i].chan_list; tmp; tmp = tmp->next)
	    if (tmp->server >= server)
		tmp->server--;
}

/* remove_from_join_list: called when modes have been received or
   when access has been denied */
void remove_from_join_list(char *chan, int server)
{
    struct joinlist *tmp, *next, *prev = NULL;

    for (tmp = join_list; tmp; tmp = tmp->next) {
	next = tmp->next;
	if (!my_stricmp(chan, tmp->chan) && tmp->server == server) {
	    if (tmp == join_list)
		join_list = next;
	    else
		prev->next = next;
	    new_free(&tmp->chan);
	    new_free((char **) &tmp);
	    return;
	} else
	    prev = tmp;
    }
}

/* get_win_from_join_list: returns the window refnum associated to the channel
   we're trying to join */
int get_win_from_join_list(char *chan, int server)
{
    struct joinlist *tmp;
    int found = 0;

    for (tmp = join_list; tmp; tmp = tmp->next) {
	if (!my_stricmp(tmp->chan, chan) && tmp->server == server)
	    found = tmp->winref;
    }
    return found;
}

/* get_chan_from_join_list: returns a pointer on the name of the first channel
   entered into join_list for the specified server */
char *get_chan_from_join_list(int server)
{
    struct joinlist *tmp;
    char *found = NULL;

    for (tmp = join_list; tmp; tmp = tmp->next) {
	if (tmp->server == server)
	    found = tmp->chan;
    }
    return found;
}

/* in_join_list: returns 1 if we're attempting to join the channel */
int in_join_list(char *chan, int server)
{
    struct joinlist *tmp;

    for (tmp = join_list; tmp; tmp = tmp->next) {
	if (!my_stricmp(tmp->chan, chan) && tmp->server == server)
	    return 1;
    }
    return 0;
}

void show_channel_sync(struct joinlist *tmp, char *chan)
{
    struct timeval tv;

    get_time(&tv);
    message_from(chan, LOG_CRAP);
    if (do_hook(CHANNEL_SYNCH_LIST, "%s %1.3f", chan, time_diff(tmp->tv, tv)))
	bitchsay("Join to %s was synced in %1.3f secs!!", chan, time_diff(tmp->tv, tv));
    update_all_status(curr_scr_win, NULL, 0);
    message_from(NULL, LOG_CRAP);
}

/* got_info: increments the gotinfo field when receiving names and mode
   and removes the channel if both have been received */
int got_info(char *chan, int server, int type)
{
    struct joinlist *tmp;

    for (tmp = join_list; tmp; tmp = tmp->next)
	if (!my_stricmp(tmp->chan, chan) && tmp->server == server) {
	    if ((tmp->gotinfo |= type) == (GOTMODE | GOTBANS | GOTNAMES)) {
		if (prepare_command(&server, chan, 3))
		    show_channel_sync(tmp, chan);
		remove_from_join_list(chan, server);
		return 1;
	    }
	    return 0;
	}
    return -1;
}

/* add_to_join_list: registers a channel we're trying to join */
void add_to_join_list(char *chan, int server, int winref)
{
    struct joinlist *tmp;

    for (tmp = join_list; tmp; tmp = tmp->next) {
	if (!my_stricmp(chan, tmp->chan) && tmp->server == server) {
	    tmp->winref = winref;
	    return;
	}
    }
    tmp = (struct joinlist *) new_malloc(sizeof(struct joinlist));
    tmp->chan = NULL;
    malloc_strcpy(&tmp->chan, chan);
    tmp->server = server;
    tmp->gotinfo = 0;
    tmp->winref = winref;
    tmp->next = join_list;
    get_time(&tmp->tv);
    join_list = tmp;
}

static void add_to_mode_list(char *channel, int server, char *mode)
{
    struct modelist *mptr;

    if (!channel || !*channel || !mode || !*mode)
	return;

    mptr = (struct modelist *) new_malloc(sizeof(struct modelist));
    mptr->chan = NULL;
    malloc_strcpy(&mptr->chan, channel);
    mptr->server = server;
    mptr->mode = NULL;
    malloc_strcpy(&mptr->mode, mode);
    mptr->next = mode_list;
    mode_list = mptr;
}

static void check_mode_list_join(char *channel, int server)
{
    struct modelist *mptr;

    if (!channel)
	return;
    for (mptr = mode_list; mptr; mptr = mptr->next) {
	if (!my_stricmp(mptr->chan, channel) && mptr->server == server) {
	    int old_server = from_server;

	    from_server = server;
	    send_to_server(SERVER(from_server), "MODE %s %s", mptr->chan, mptr->mode);
	    from_server = old_server;
	    remove_from_mode_list(channel, server);
	    return;
	}
    }
}

void remove_from_mode_list(char *channel, int server)
{
    struct modelist *curr, *next, *prev = NULL;

    for (next = mode_list; next;) {
	curr = next;
	next = curr->next;
	if (!my_stricmp(curr->chan, channel) && curr->server == server) {
	    if (curr == mode_list)
		mode_list = curr->next;
	    else
		prev->next = curr->next;
	    prev = curr;
	    new_free(&curr->chan);
	    new_free(&curr->mode);
	    new_free((char **) &curr);
	} else
	    prev = curr;
    }
}

void clear_mode_list(int server)
{
    struct modelist *curr, *next, *prev = NULL;

    for (next = mode_list; next;) {
	curr = next;
	next = curr->next;
	if (curr == mode_list)
	    mode_list = curr->next;
	else
	    prev->next = curr->next;
	prev = curr;
	new_free(&curr->chan);
	new_free(&curr->mode);
	new_free((char **) &curr);
    }
}

int chan_is_connected(char *channel, int server)
{
    struct channel *cp = lookup_channel(channel, server, CHAN_NOUNLINK);

    if (!cp)
	return 0;
    return (cp->connected);
}
