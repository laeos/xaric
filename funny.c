#ident "@(#)funny.c 1.8"
/*
 * funny.c: Well, I put some stuff here and called it funny.  So sue me. 
 *
 * written by michael sandrof
 *
 * copyright(c) 1990 
 *
 * see the copyright file, or do a help ircii copyright 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"

#include "ircaux.h"
#include "hook.h"
#include "vars.h"
#include "funny.h"
#include "names.h"
#include "server.h"
#include "lastlog.h"
#include "ircterm.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "status.h"
#include "misc.h"
#include "screen.h"
#include "fset.h"

static char *match_str = NULL;

static int funny_min;
static int funny_max;
static int funny_flags;

void funny_match(char *stuff)
{
    malloc_strcpy(&match_str, stuff);
}

void set_funny_flags(int min, int max, int flags)
{
    funny_min = min;
    funny_max = max;
    funny_flags = flags;
}

struct WideListInfoStru {
    char *channel;
    int users;
};

typedef struct WideListInfoStru WideList;

static WideList **wide_list = NULL;
static int wl_size = 0;
static int wl_elements = 0;

static int funny_widelist_users(WideList **, WideList **);
static int funny_widelist_names(WideList **, WideList **);

static int funny_widelist_users(WideList ** left, WideList ** right)
{
    if ((**left).users > (**right).users)
	return -1;
    else if ((**right).users > (**left).users)
	return 1;
    else
	return strcasecmp((**left).channel, (**right).channel);
}

static int funny_widelist_names(WideList ** left, WideList ** right)
{
    int comp;

    if (!(comp = strcasecmp((**left).channel, (**right).channel)))
	return comp;
    else if ((**left).users > (**right).users)
	return -1;
    else if ((**right).users > (**left).users)
	return 1;
    else
	return 0;
}

void funny_print_widelist(void)
{
    int i;
    char buffer1[BIG_BUFFER_SIZE];
    char buffer2[BIG_BUFFER_SIZE];
    char *ptr;

    if (!wide_list)
	return;

    if (funny_flags & FUNNY_NAME)
	qsort((void *) wide_list, wl_elements, sizeof(WideList *), (int (*)(const void *, const void *)) funny_widelist_names);
    else if (funny_flags & FUNNY_USERS)
	qsort((void *) wide_list, wl_elements, sizeof(WideList *), (int (*)(const void *, const void *)) funny_widelist_users);

    *buffer1 = '\0';
    for (i = 1; i < wl_elements; i++) {
	sprintf(buffer2, "%s(%d) ", wide_list[i]->channel, wide_list[i]->users);
	ptr = strchr(buffer1, '\0');
	if (strlen(buffer1) + strlen(buffer2) > term_cols - 5) {
	    if (do_hook(WIDELIST_LIST, "%s", buffer1))
		put_it("%s", convert_output_format(get_fset_var(FORMAT_WIDELIST_FSET), "%s %s", update_clock(GET_TIME), buffer1));
	    *buffer1 = 0;
	    strcat(buffer1, buffer2);
	} else
	    strcpy(ptr, buffer2);
    }
    if (*buffer1 && do_hook(WIDELIST_LIST, "%s", buffer1))
	put_it("%s", convert_output_format(get_fset_var(FORMAT_WIDELIST_FSET), "%s %s", update_clock(GET_TIME), buffer1));
    for (i = 0; i < wl_elements; i++) {
	new_free(&wide_list[i]->channel);
	new_free((char **) &wide_list[i]);
    }
    new_free(&wide_list);
    wl_elements = wl_size = 0;
}

void funny_list(char *from, char **ArgList)
{
    char *channel, *user_cnt, *line;
    WideList **new_list;
    int cnt;
    static char format[30];
    static int last_width = -1;

    if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR)) {
	if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
	    snprintf(format, 25, "%%s %%-%u.%us %%-5s %%s", (unsigned char) last_width, (unsigned char) last_width);
	else
	    snprintf(format, 25, "%%s %%s %%-5s %%s");
    }
    channel = ArgList[0];
    user_cnt = ArgList[1];
    line = PasteArgs(ArgList, 2);
    if (funny_flags & FUNNY_TOPIC && !(line && *line))
	return;
    cnt = my_atol(user_cnt);
    if (funny_min && (cnt < funny_min))
	return;
    if (funny_max && (cnt > funny_max))
	return;
    if ((funny_flags & FUNNY_PRIVATE) && (*channel != '*'))
	return;
    if ((funny_flags & FUNNY_PUBLIC) && ((*channel == '*') || (*channel == '@')))
	return;
    if (match_str) {
	if (wild_match(match_str, channel) == 0)
	    return;
    }
    if (funny_flags & FUNNY_WIDE) {
	if (wl_elements >= wl_size) {
	    new_list = new_malloc(sizeof(WideList *) * (wl_size + 50));
	    memset(new_list, 0, sizeof(WideList *) * (wl_size + 50));
	    if (wl_size)
		memcpy(new_list, wide_list, sizeof(WideList *) * wl_size);
	    wl_size += 50;
	    new_free(&wide_list);
	    wide_list = new_list;
	}
	wide_list[wl_elements] = new_malloc(sizeof(WideList));
	wide_list[wl_elements]->channel = NULL;
	wide_list[wl_elements]->users = cnt;
	malloc_strcpy(&wide_list[wl_elements]->channel, (*channel != '*') ? channel : "Prv");
	wl_elements++;
	return;
    }
    if (do_hook(current_numeric, "%s %s %s %s", from, channel, user_cnt,
		line) && do_hook(LIST_LIST, "%s %s %s", channel, user_cnt, line)) {
	if (channel && user_cnt)
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_LIST_FSET), "%s %s %s %s", update_clock(GET_TIME),
					 *channel == '*' ? "Prv" : channel, user_cnt, line));
    }
}

void print_funny_names(char *line)
{
    char *t;
    int count = 0;
    char *buffer = NULL;
    char special = '\0';

    if (*line) {
	t = next_arg(line, &line);
	if (get_fset_var(FORMAT_NAMES_BANNER_FSET))
	    malloc_strcpy(&buffer, get_fset_var(FORMAT_NAMES_BANNER_FSET));
	do {
	    if (*t == '@' || *t == '+' || *t == '~' || *t == '-') {
		special = *t;
		if (special == '+')
		    malloc_strcat(&buffer, convert_output_format(get_fset_var(FORMAT_NAMES_VOICECOLOR_FSET), "%c %s", special, ++t));
		else
		    malloc_strcat(&buffer, convert_output_format(get_fset_var(FORMAT_NAMES_OPCOLOR_FSET), "%c %s", special, ++t));
	    } else
		malloc_strcat(&buffer, convert_output_format(get_fset_var(FORMAT_NAMES_NICKCOLOR_FSET), "$ %s", t));
	    malloc_strcat(&buffer, " ");
	    if (count++ == 4) {
		put_it("%s", buffer);
		if (get_fset_var(FORMAT_NAMES_BANNER_FSET))
		    malloc_strcpy(&buffer, get_fset_var(FORMAT_NAMES_BANNER_FSET));
		else
		    new_free(&buffer);
		count = 0;
	    }
	}
	while ((t = next_arg(line, &line)));

	if (buffer)
	    put_it("%s", buffer);
	new_free(&buffer);
    }
}

void funny_namreply(char *from, char **Args)
{
    char *type;
    char *channel;
    static char format[40];
    static int last_width = -1;
    int cnt;
    char *ptr;
    char *line;
    char *t;
    int user_count = 0;

    PasteArgs(Args, 2);
    type = Args[0];
    channel = Args[1];
    line = Args[2];

    if (in_join_list(channel, from_server)) {
	char *lincopy, *p;
	char *nick;

	message_from(channel, LOG_CRAP);

	p = lincopy = m_strdup(line);

	for (t = line; *t; t++) {
	    if (*t == ' ')
		user_count++;
	}
	if (user_count == 0 && strlen(line))
	    user_count = 1;

	if (do_hook(current_numeric, "%s %s %s %s", from, type, channel, line)
	    && do_hook(NAMES_LIST, "%s %s", channel, line)
	    && get_int_var(SHOW_CHANNEL_NAMES_VAR)) {
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_NAMES_FSET), "%s %s %d", update_clock(GET_TIME), channel, user_count));
	    print_funny_names(line);
	}

	while ((nick = next_arg(lincopy, &lincopy)) != NULL)
	    switch (*nick) {
	    case '%':		/* unreal half-op */
	    case '@':
		add_to_channel(channel, nick + 1, from_server, 1, 0, NULL, Args[3], Args[5]);
		break;
	    case '+':
		add_to_channel(channel, nick + 1, from_server, 0, 1, NULL, Args[3], Args[5]);
		break;

	    default:
		add_to_channel(channel, nick, from_server, 0, 0, NULL, Args[3], Args[5]);
	    }
	new_free(&p);
	message_from(NULL, LOG_CURRENT);
	return;
    }
    if (last_width != get_int_var(CHANNEL_NAME_WIDTH_VAR)) {
	if ((last_width = get_int_var(CHANNEL_NAME_WIDTH_VAR)) != 0)
	    sprintf(format, "%%s: %%-%u.%us %%s", (unsigned char) last_width, (unsigned char) last_width);
	else
	    strcpy(format, "%s: %s\t%s");
    }
    ptr = line;
    for (cnt = -1; ptr; cnt++) {
	if ((ptr = strchr(ptr, ' ')) != NULL)
	    ptr++;
    }
    if (funny_min && (cnt < funny_min))
	return;
    else if (funny_max && (cnt > funny_max))
	return;
    if ((funny_flags & FUNNY_PRIVATE) && (*type == '='))
	return;
    if ((funny_flags & FUNNY_PUBLIC) && ((*type == '*') || (*type == '@')))
	return;
    if (type && channel) {
	if (match_str) {
	    if (wild_match(match_str, channel) == 0)
		return;
	}
	if (do_hook(current_numeric, "%s %s %s %s", from, type, channel, line) && do_hook(NAMES_LIST, "%s %s", channel, line)) {
	    message_from(channel, LOG_CRAP);
	    if (get_fset_var(FORMAT_NAMES_FSET)) {
		put_it("%s", convert_output_format(get_fset_var(FORMAT_NAMES_FSET), "%s %s %d", update_clock(GET_TIME), channel, cnt));
		print_funny_names(line);
	    } else {
		switch (*type) {
		case '=':
		    if (last_width && (strlen(channel) > last_width)) {
			channel[last_width - 1] = '>';
			channel[last_width] = (char) 0;
		    }
		    put_it(format, "Pub", channel, line);
		    break;
		case '*':
		    put_it(format, "Prv", channel, line);
		    break;
		case '@':
		    put_it(format, "Sec", channel, line);
		    break;
		}
	    }
	    message_from(NULL, LOG_CRAP);
	}
    }
}

void funny_mode(char *from, char **ArgList)
{
    char *mode, *channel;
    struct channel *chan = NULL;

    if (!ArgList[0])
	return;
    channel = ArgList[0];
    mode = ArgList[1];
    PasteArgs(ArgList, 1);
    if ((channel && in_join_list(channel, from_server)) || get_chan_from_join_list(from_server)) {
	if (!channel)
	    channel = get_chan_from_join_list(from_server);
	update_channel_mode(from, channel, from_server, mode, chan);
	update_all_status(curr_scr_win, NULL, 0);
	got_info(channel, from_server, GOTMODE);
    } else {
	if (channel) {
	    message_from(channel, LOG_CRAP);
	    if (do_hook(current_numeric, "%s %s %s", from, channel, mode))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_MODE_CHANNEL_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from,
					     *FromUserHost ? FromUserHost : "ÿ", channel, mode));
	} else {
	    if (do_hook(current_numeric, "%s %s", from, mode))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_MODE_CHANNEL_FSET), "%s %s %s %s", update_clock(GET_TIME), from,
					     *FromUserHost ? FromUserHost : "ÿ", mode));
	}
    }
}

void update_user_mode(char *modes)
{
    int onoff = 1;

    for (; *modes; modes++) {
	if (*modes == '-')
	    onoff = 0;
	else if (*modes == '+')
	    onoff = 1;

	else if ((*modes >= 'a' && *modes <= 'z')
		 || (*modes >= 'A' && *modes <= 'Z')) {
	    char c = tolower(*modes);
	    size_t idx = (size_t) (strchr(umodes, c) - umodes);

	    set_server_flag(from_server, USER_MODE << idx, onoff);

	    if (c == 'o' || c == 'O')
		set_server_operator(from_server, onoff);
	}
    }
}

extern void reinstate_user_modes(void)
{
    char *modes = get_umode(from_server);

    if (modes && *modes)
	send_to_server(SERVER(from_server), "MODE %s +%s", get_server_nickname(from_server), modes);
}
