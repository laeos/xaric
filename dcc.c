/*
 * dcc.c: Things dealing client to client connections. 
 *
 * Written By Troy Rollo <troy@cbme.unsw.oz.au> 
 *
 * Copyright(c) 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * Heavily modified Colten Edwards 1996-97
 *
 * needs to be rewritten
 * it really sucks.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>

#include "ctcp.h"
#include "dcc.h"
#include "hook.h"
#include "ircaux.h"
#include "lastlog.h"
#include "newio.h"
#include "output.h"
#include "parse.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "whois.h"
#include "window.h"
#include "ircterm.h"
#include "hook.h"
#include "misc.h"
#include "screen.h"
#include "hash.h"
#include "fset.h"
#include "tcommand.h"

#include <float.h>

static off_t filesize = 0;

DCC_list *ClientList = NULL;

static char DCC_current_transfer_buffer[BIG_BUFFER_SIZE / 4];
static void dcc_add_deadclient(register DCC_list *);
static void dcc_close(const char *, char *);
static void dcc_getfile(const char *, char *);
int dcc_open(DCC_list *);
static void dcc_really_erase(void);
static void dcc_rename(const char *, char *);
static void dcc_send_raw(const char *, char *);
static void add_to_dcc_buffer(DCC_list * Client, char *buf);
static void output_reject_ctcp(char *, char *);
static void process_incoming_chat(register DCC_list *);
static void process_incoming_listen(register DCC_list *);
static void process_incoming_raw(register DCC_list *);
static void process_outgoing_file(register DCC_list *, int);
static void process_incoming_file(register DCC_list *);
static void DCC_close_filesend(DCC_list *, char *);
static void update_transfer_buffer(const char *format, ...);

static void dcc_got_connected(DCC_list *);

static void dcc_set_paths(const char *, char *);
static void dcc_set_quiet(const char *, char *);
static void dcc_show_active(const char *, char *);
static void dcc_help1(const char *, char *);

static unsigned char byteordertest(void);
static void dcc_reject_notify(char *, char *, const char *);
static int get_to_from(const char *);

static char *strip_path(char *);
static void dcc_tog_rename(const char *, char *);
static char *check_paths(char *);
static void dcc_overwrite_toggle(const char *, char *);

typedef void (*dcc_function) (const char *, char *);
typedef struct {
    const char *name;
    dcc_function function;
} DCC_commands;

static const DCC_commands dcc_commands[] = {
    {"ACTIVE", dcc_show_active},
    {"AUTO_RENAME", dcc_tog_rename},
    {"CHAT", dcc_chat},
    {"LIST", dcc_list},
    {"GLIST", dcc_glist},

    {"SEND", dcc_filesend},
    {"RESEND", dcc_resend},

    {"GET", dcc_getfile},
    {"REGET", dcc_regetfile},

    {"CLOSE", dcc_close},
    {"RENAME", dcc_rename},

    {"RAW", dcc_send_raw},
    {"QUIET", dcc_set_quiet},
    {"PATHS", dcc_set_paths},
    {"OVERWRITE", dcc_overwrite_toggle},

    {"?", dcc_help1},
    {"HELP", dcc_help1},
    {NULL, (dcc_function) NULL}
};

#define BAR_LENGTH 50
static int dcc_count = 1;
int dcc_active_count = 0;
static int doing_multi = 0;
static int dcc_quiet = 0;
static int dcc_paths = 0;
static int dcc_overwrite_var = 0;

unsigned int send_count_stat = 0;
unsigned int get_count_stat = 0;

static char *last_chat_req = NULL;

static const char *dcc_types[] = {
    "<none>",
    "CHAT",
    "SEND",
    "GET",
    "RAW_LISTEN",
    "RAW",
    "RESEND",
    "REGET",
    NULL
};

static struct deadlist {
    DCC_list *it;
    struct deadlist *next;
} *deadlist = NULL;

static int dccBlockSize(void)
{
    register int BlockSize;

    BlockSize = get_int_var(DCC_BLOCK_SIZE_VAR);
    if (BlockSize > MAX_DCC_BLOCK_SIZE) {
	BlockSize = MAX_DCC_BLOCK_SIZE;
	set_int_var(DCC_BLOCK_SIZE_VAR, MAX_DCC_BLOCK_SIZE);
    } else if (BlockSize < 16) {
	BlockSize = 16;
	set_int_var(DCC_BLOCK_SIZE_VAR, 16);
    }
    return (BlockSize);
}

/*
 * dcc_searchlist searches through the dcc_list and finds the client
 * with the the flag described in type set.
 */
static DCC_list *dcc_searchlist(const char *name, const char *user, int type, int flag, char *othername, char *userhost, int active)
{
    DCC_list **Client = NULL, *NewClient = NULL;
    int dcc_num = 0;

    if (user && *user == '#' && my_atol(user + 1)) {
	/* we got a number so find that instead */
	const char *p;

	p = user;
	p++;
	dcc_num = my_atol(p);
	if (dcc_num > 0) {
	    for (Client = (&ClientList); *Client; Client = (&(**Client).next))
		if (dcc_num && ((**Client).dccnum == dcc_num))
		    return *Client;
	}
    } else {
	for (Client = (&ClientList); *Client; Client = (&(**Client).next)) {
	    if ((**Client).flags & DCC_DELETE)
		continue;	/* ignore deleted clients */
	    /* 
	     * The following things have to be true:
	     * -> The types have to be the same
	     * -> One of the following things is true:
	     *      -> `name' is NULL or `name' is the same as the 
	     *         entry's description
	     *          *OR*
	     *      -> `othername' is the same as the entry's othername
	     * -> `user' is the same as the entry's user-perr
	     * -> One of the following is true:
	     *      -> `active' is 1 and the entry is active
	     *      -> `active' is 0 and the entry is not active
	     *      -> `active' is -1 (dont care)
	     */
	    if ((((**Client).flags & DCC_TYPES) == type) &&
		((!name || ((**Client).description && !my_stricmp(name, (**Client).description))) ||
		 (othername && (**Client).othername && !my_stricmp(othername, (**Client).othername))) &&
		(user && !my_stricmp(user, (**Client).user)) &&
		((active == -1) ||
		 ((active == 0) && !((**Client).flags & DCC_ACTIVE)) || ((active == 1) && ((**Client).flags & DCC_ACTIVE))
		)
		)
		return *Client;
	}
    }
    if (!flag)
	return NULL;
    *Client = NewClient = new_malloc(sizeof(DCC_list));
    NewClient->flags = type;
    NewClient->read = NewClient->write = NewClient->file = -1;
    NewClient->filesize = filesize;
    malloc_strcpy(&NewClient->description, name);
    malloc_strcpy(&NewClient->user, user);
    malloc_strcpy(&NewClient->othername, othername);
    malloc_strcpy(&NewClient->userhost, userhost);
    NewClient->packets_total = filesize ? (filesize / dccBlockSize() + 1) : 0;
    get_time(&NewClient->lasttime);
    if (dcc_count == 0)
	dcc_count = 1;
    NewClient->dccnum = dcc_count++;
    return NewClient;
}

/*
 * Added by Chaos: Is used in edit.c for checking redirect.
 */
int dcc_active(char *user)
{
    return (dcc_searchlist("chat", user, DCC_CHAT, 0, NULL, NULL, 1)) ? 1 : 0;
}

int dcc_activeraw(char *user)
{
    return (dcc_searchlist( /* "RAW" */ NULL, user, DCC_RAW, 0, NULL, NULL, 1)) ? 1 : 0;
}

static void dcc_add_deadclient(register DCC_list * client)
{
    struct deadlist *new;

    new = new_malloc(sizeof(struct deadlist));
    new->next = deadlist;
    new->it = client;
    deadlist = new;
}

/*
 * dcc_erase searches for the given entry in the dcc_list and
 * removes it
 */
int dcc_erase(DCC_list * Element)
{
    register DCC_list **Client;
    int erase_one = 0;

    dcc_count = 1;

    for (Client = &ClientList; *Client; Client = &(**Client).next) {
	if (*Client == Element) {
	    *Client = Element->next;
	    if (Element->write > -1) {
		close(Element->write);
	    }
	    if (Element->read > -1) {
		close(Element->read);
	    }
	    if (Element->file > -1)
		close(Element->file);

	    new_free(&Element->othername);
	    new_free(&Element->description);
	    new_free(&Element->userhost);
	    new_free(&Element->user);
	    new_free(&Element->buffer);
	    new_free(&Element->cksum);
	    if (Element->dcc_handler)
		(Element->dcc_handler) (NULL, NULL);
	    new_free(&Element);
	    erase_one++;
	    break;
	}
    }
    for (Client = &ClientList; *Client; Client = &(**Client).next)
	(*Client)->dccnum = dcc_count++;
    if (erase_one) {
	*DCC_current_transfer_buffer = 0;
    }
    return erase_one;
}

static void dcc_really_erase(void)
{
    register struct deadlist *dies;

    for (; (dies = deadlist) != NULL;) {
	deadlist = deadlist->next;
	dcc_erase(dies->it);
    }
}

/*
 * Set the descriptor set to show all fds in Client connections to
 * be checked for data.
 */
void set_dcc_bits(fd_set * rd, fd_set * wd)
{
    register DCC_list *Client;
    unsigned int flag;

    for (Client = ClientList; Client != NULL; Client = Client->next) {

	flag = Client->flags & DCC_TYPES;
	if (Client->write != -1 && ((Client->flags & DCC_CNCT_PEND) || (flag == DCC_FILEOFFER) || (flag == DCC_RESENDOFFER)))
	    FD_SET(Client->write, wd);
	if (Client->read != -1)
	    FD_SET(Client->read, rd);
    }
    if (!ClientList && dcc_active_count)
	dcc_active_count = 0;	/* A HACK */
}

static void dcc_got_connected(DCC_list * client)
{
    struct sockaddr_in remaddr = { 0 };
    socklen_t rl = sizeof(remaddr);

    if (getpeername(client->read, (struct sockaddr *) &remaddr, &rl) != -1) {

	if (client->flags & DCC_OFFER) {
	    client->flags &= ~DCC_OFFER;

	    if ((client->flags & DCC_TYPES) != DCC_RAW) {

		if (do_hook(DCC_CONNECT_LIST, "%s %s %s %d", client->user,
			    dcc_types[client->flags & DCC_TYPES], inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port))) {
		    if (!dcc_quiet)
			put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
							   "%s %s %s %s %s %d", update_clock(GET_TIME),
							   dcc_types[client->flags & DCC_TYPES], client->user,
							   client->userhost ? client->userhost : "u@h", inet_ntoa(remaddr.sin_addr),
							   (int) ntohs(remaddr.sin_port)));
		}

	    }
	}
	get_time(&client->starttime);
	client->flags &= ~DCC_CNCT_PEND;
	if (((client->flags & DCC_TYPES) == DCC_REGETFILE)) {
	    /* send a packet to the sender with transfer resume instructions */
	    put_it("%s",
		   convert_output_format("$G %RDCC %YTelling uplink we want to start at%n: $0", "%l",
					 client->transfer_orders.byteoffset));
	    send(client->read, (const char *) &client->transfer_orders, sizeof(struct transfer_struct), 0);
	}
	if (0 /* !get_int_var(DCC_FAST_VAR) */ ) {
	    set_blocking(client->read);
	    if (client->read != client->write)
		set_blocking(client->write);
	}
    }
}

/*
 * Check all DCCs for data, and if they have any, perform whatever
 * actions are required.
 */
void dcc_check(fd_set * Readables, fd_set * Writables)
{
    register DCC_list **Client;
    struct timeval timeout;
    int previous_server;
    int lastlog_level;
    register int flags;

    previous_server = from_server;
    from_server = (-1);
    timeout.tv_sec = timeout.tv_usec = 0;
    message_from(NULL, LOG_DCC);
    lastlog_level = set_lastlog_msg_level(LOG_DCC);

    for (Client = (&ClientList); *Client != NULL;) {
	flags = (*Client)->flags;

	if (flags & DCC_CNCT_PEND)
	    dcc_got_connected(*Client);

	if (!(flags & DCC_DELETE) && (*Client)->read != -1) {
	    if (FD_ISSET((*Client)->read, Readables)) {
		FD_CLR((*Client)->read, Readables);
		switch (flags & DCC_TYPES) {
		case DCC_CHAT:
		    from_server = previous_server;
		    process_incoming_chat(*Client);
		    from_server = -1;
		    break;
		case DCC_RAW_LISTEN:
		    process_incoming_listen(*Client);
		    break;
		case DCC_RAW:
		    process_incoming_raw(*Client);
		    break;
		case DCC_FILEOFFER:
		case DCC_RESENDOFFER:
		    process_outgoing_file(*Client, 1);
		    break;
		case DCC_FILEREAD:
		case DCC_REGETFILE:
		    process_incoming_file(*Client);
		    break;
		}
	    }
	}
	if (flags & DCC_DELETE)
	    dcc_add_deadclient(*Client);
	Client = (&(**Client).next);
    }

    message_from(NULL, LOG_CRAP);
    (void) set_lastlog_msg_level(lastlog_level);
    dcc_really_erase();
    from_server = previous_server;
}

/*
 * Process a DCC command from the user.
 */
void process_dcc(char *args)
{
    char *command;
    int i;
    int lastlog_level;

    if (!(command = next_arg(args, &args)))
	return;
    for (i = 0; dcc_commands[i].name != NULL; i++) {
	if (!my_stricmp(dcc_commands[i].name, command)) {
	    message_from(NULL, LOG_DCC);
	    lastlog_level = set_lastlog_msg_level(LOG_DCC);
	    dcc_commands[i].function(dcc_commands[i].name, args);
	    message_from(NULL, LOG_CRAP);
	    (void) set_lastlog_msg_level(lastlog_level);
	    return;
	}
    }
    put_it("%s", convert_output_format("$G Unknown %RDCC%n command: $0", "%s", command));
}

int dcc_open(DCC_list * Client)
{
    char *user;
    const char *type;
    struct in_addr myip;
    int old_server;
    struct sockaddr *saddr;
    socklen_t slen;

    user = Client->user;
    old_server = from_server;

    if (from_server == -1)
	from_server = get_window_server(0);

    myip.s_addr = 0;
    if (sa_addr_a2s(server_list[from_server].lcl_addr, &saddr, &slen) == SA_OK) {
	if (saddr->sa_family == AF_INET) {
	    myip.s_addr = ((struct sockaddr_in *) saddr)->sin_addr.s_addr;
	    free(saddr);
	} else {
	    yell("hello sorry dcc works only with IPv4 kthx");
	    free(saddr);
	    return 0;
	}
    } else {
	yell("something bad happened, I couldn't get local address");
	return 0;
    }
    type = dcc_types[Client->flags & DCC_TYPES];

    if (Client->flags & DCC_OFFER) {
	message_from(NULL, LOG_DCC);
	Client->flags |= DCC_CNCT_PEND;
	/* BLAH! */
	Client->remport = ntohs(Client->remport);
	if ((Client->write = connect_by_number(inet_ntoa(Client->remote), &Client->remport, SERVICE_CLIENT, PROTOCOL_TCP, 1)) < 0) {
	    put_it("%s",
		   convert_output_format("$G %RDCC%n Unable to create connection: $0-", "%s",
					 errno ? strerror(errno) : "Unknown Host"));
	    message_from(NULL, LOG_CRAP);
	    dcc_erase(Client);
	    from_server = old_server;
	    return 0;
	}
	/* BLAH! */
	Client->remport = htons(Client->remport);
	Client->read = Client->write;
	Client->flags |= DCC_ACTIVE;
	message_from(NULL, LOG_CRAP);
	get_time(&Client->starttime);
	Client->bytes_read = Client->bytes_sent = 0L;
	from_server = old_server;
	return 1;
    } else {
	unsigned short portnum = 0;

	Client->flags |= DCC_WAIT | DCC_CNCT_PEND;
	message_from(NULL, LOG_DCC);
	if (Client->remport)
	    portnum = Client->remport;
	if ((Client->read = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1)) < 0) {
	    put_it("%s",
		   convert_output_format("$G %RDCC%n Unable to create connection: $0-", "%s",
					 errno ? strerror(errno) : "Unknown Host"));
	    message_from(NULL, LOG_CRAP);
	    dcc_erase(Client);
	    from_server = old_server;
	    return 0;
	}
	if (get_to_from(type) != -1)
	    dcc_active_count++;
	if (Client->flags & DCC_TWOCLIENTS) {
	    /* patch to NOT send pathname accross */
	    char *nopath;

	    if (((Client->flags & DCC_FILEOFFER) == DCC_FILEOFFER) && (nopath = strrchr(Client->description, '/')))
		nopath++;
	    else
		nopath = Client->description;

	    if (Client->filesize)
		send_ctcp(CTCP_PRIVMSG, user, CTCP_DCC,
			  "%s %s %lu %u %lu", type, nopath, (u_long) ntohl(myip.s_addr), (u_short) portnum, Client->filesize);
	    else
		send_ctcp(CTCP_PRIVMSG, user, CTCP_DCC, "%s %s %lu %u", type, nopath, (u_long) ntohl(myip.s_addr), (u_short) portnum);
	    message_from(NULL, LOG_DCC);
	    if (!doing_multi && !dcc_quiet)
		put_it("%s", convert_output_format(get_fset_var(FORMAT_SEND_DCC_CHAT_FSET),
						   "%s %s %s", update_clock(GET_TIME), type, user));
	    message_from(NULL, LOG_CRAP);
	}
	Client->starttime.tv_sec = Client->starttime.tv_usec = 0;
	from_server = old_server;
	return 2;
    }
    message_from(NULL, LOG_CRAP);
}
void add_userhost_to_dcc(WhoisStuff * stuff, char *nick, char *args)
{
    DCC_list *Client;

    Client = dcc_searchlist("chat", nick, DCC_CHAT, 0, NULL, NULL, -1);
    if (!stuff || !nick || !stuff->nick || !stuff->user || !stuff->host || !strcmp(stuff->user, "<UNKNOWN>"))
	return;
    if (Client)
	Client->userhost = m_sprintf("%s@%s", stuff->user, stuff->host);
    return;
}

void dcc_chat(const char *command, char *args)
{
    char *user;
    char *port = NULL;
    char *equal_user = NULL;
    DCC_list *Client;

    if ((user = next_arg(args, &args)) == NULL) {
	put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC chat", NULL, NULL));
	return;
    }
    while (args && *args) {
	char *argument = next_arg(args, &args);

	if (argument && strlen(argument) >= 2) {
	    if (argument[0] == '-' || argument[1] == 'p')
		port = next_arg(args, &args);
	}
    }

    Client = dcc_searchlist("chat", user, DCC_CHAT, 1, NULL, NULL, -1);

    if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT)) {
	put_it("%s", convert_output_format("$G %RDCC%n A previous DCC chat to $0 exists", "%s", user));
	return;
    }
    if (port)
	Client->remport = atol(port);
    Client->flags |= DCC_TWOCLIENTS;

    equal_user = m_sprintf("=%s", user);
    addtabkey(equal_user, "msg");
    new_free(&equal_user);

    dcc_open(Client);
    add_to_userhost_queue(user, add_userhost_to_dcc, "%d %s", Client->read, user);
}

char *dcc_raw_listen(int port)
{
    DCC_list *Client;
    char *PortName;
    struct sockaddr_in locaddr;
    char *RetName = NULL;
    socklen_t size;
    int lastlog_level;

    lastlog_level = set_lastlog_msg_level(LOG_DCC);
    if (port && port < 1025) {
	put_it("%s", convert_output_format("$G %RDCC%n Cannot bind to a privileged port", NULL, NULL));
	(void) set_lastlog_msg_level(lastlog_level);
	return NULL;
    }
    PortName = ltoa(port);
    Client = dcc_searchlist("raw_listen", PortName, DCC_RAW_LISTEN, 1, NULL, NULL, -1);
    if (Client->flags & DCC_ACTIVE) {
	put_it("%s", convert_output_format("$G %RDCC%n A previous Raw Listen on $0 exists", "%s", PortName));
	set_lastlog_msg_level(lastlog_level);
	return RetName;
    }
    memset((char *) &locaddr, 0, sizeof(locaddr));
    locaddr.sin_family = AF_INET;
    locaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    locaddr.sin_port = htons(port);
    if (0 > (Client->read = socket(AF_INET, SOCK_STREAM, 0))) {
	dcc_erase(Client);
	put_it("%s", convert_output_format("$G %RDCC%n socket() failed: $0-", "%s", strerror(errno)));
	(void) set_lastlog_msg_level(lastlog_level);
	return RetName;
    }
    set_socket_options(Client->read);
    if (bind(Client->read, (struct sockaddr *) &locaddr, sizeof(locaddr)) == -1) {
	dcc_erase(Client);
	put_it("%s", convert_output_format("$G %RDCC%n Could not bind port: $0-", "%s", strerror(errno)));
	(void) set_lastlog_msg_level(lastlog_level);
	return RetName;
    }
    listen(Client->read, 4);
    size = sizeof(locaddr);
    get_time(&Client->starttime);
    getsockname(Client->read, (struct sockaddr *) &locaddr, &size);
    Client->write = ntohs(locaddr.sin_port);
    Client->flags |= DCC_ACTIVE;
    malloc_strcpy(&Client->user, ltoa(Client->write));
    malloc_strcpy(&RetName, Client->user);
    (void) set_lastlog_msg_level(lastlog_level);
    return RetName;
}

char *dcc_raw_connect(char *host, u_short port)
{
    DCC_list *Client;
    char *PortName;
    struct in_addr address;
    struct hostent *hp;
    int lastlog_level;

    lastlog_level = set_lastlog_msg_level(LOG_DCC);
    if ((address.s_addr = inet_addr(host)) == -1) {
	hp = gethostbyname(host);
	if (!hp) {
	    put_it("%s", convert_output_format("$G %RDCC%n Unknown host: $0-", "%s", host));
	    set_lastlog_msg_level(lastlog_level);
	    return m_strdup(empty_str);
	}
	memcpy(&address, hp->h_addr, sizeof(address));
    }
    Client = dcc_searchlist(host, ltoa(port), DCC_RAW, 1, NULL, NULL, -1);
    if (Client->flags & DCC_ACTIVE) {
	put_it("%s", convert_output_format("$G %RDCC%n A previous DCC raw to $0 on $1 exists", "%s %d", host, port));
	set_lastlog_msg_level(lastlog_level);
	return m_strdup(empty_str);
    }
    /* Sorry. The missing 'htons' call here broke $connect() */
    Client->remport = htons(port);
    memcpy((char *) &Client->remote, (char *) &address, sizeof(address));
    Client->flags = DCC_OFFER | DCC_RAW;
    if (!dcc_open(Client))
	return m_strdup(empty_str);
    PortName = ltoa(Client->read);
    Client->user = m_strdup(PortName);
    if (do_hook(DCC_RAW_LIST, "%s %s E %d", PortName, host, port))
	if (do_hook(DCC_CONNECT_LIST, "%s RAW %s %d", PortName, host, port))
	    put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
					       "%s %s %s %s %s %d", update_clock(GET_TIME), "RAW", host,
					       Client->userhost ? Client->userhost : "u@h", PortName, port));
    (void) set_lastlog_msg_level(lastlog_level);
    return m_strdup(ltoa(Client->read));
}

void real_dcc_filesend(char *filename, char *real_file, char *user, int type, int portnum)
{
    DCC_list *Client;
    struct stat stat_buf;
    char *nick = NULL;

    stat(filename, &stat_buf);
#ifdef S_IFDIR
    if (stat_buf.st_mode & S_IFDIR) {
	put_it("%s", convert_output_format("$G %RDCC%n Cannot send a directory", NULL, NULL));
	return;
    }
#endif
    filesize = stat_buf.st_size;
    while ((nick = next_in_comma_list(user, &user))) {
	if (!nick || !*nick)
	    break;
	Client = dcc_searchlist(filename, nick, type, 1, real_file, NULL, -1);
	filesize = 0;
	if (!(Client->flags & DCC_ACTIVE) && !(Client->flags & DCC_WAIT)) {
	    Client->flags |= DCC_TWOCLIENTS;
	    Client->remport = portnum;
	    dcc_open(Client);
	} else
	    put_it("%s", convert_output_format("$G %RDCC%n A previous DCC send:$0 to $1 exists", "%s %s", filename, nick));
    }
}

void dcc_resend(const char *command, char *args)
{
    char *user = NULL;
    char *filename = NULL, *fullname = NULL;
    char *FileBuf = NULL;
    int portnum = 0;

    if (!(user = next_arg(args, &args)) || !(filename = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname and a filename for DCC resend", NULL, NULL));
	return;
    }
    if (*filename == '/') {
	malloc_strcpy(&FileBuf, filename);
    } else if (*filename == '~') {
	if (!(fullname = expand_twiddle(filename))) {
	    put_it("%s", convert_output_format("$G %RDCC%n Unable to expand: $0", "%s", filename));
	    return;
	}
	malloc_strcpy(&FileBuf, fullname);
	new_free(&fullname);
    } else {
	char current_dir[BIG_BUFFER_SIZE + 1];

	getcwd(current_dir, sizeof(current_dir) - strlen(filename) - 4);
	malloc_sprintf(&FileBuf, "%s/%s", current_dir, filename);
    }

    if (access(FileBuf, R_OK)) {
	put_it("%s", convert_output_format("$G %RDCC%n Cannot access: $0", "%s", FileBuf));
	new_free(&FileBuf);
	return;
    }

    while (args && *args) {
	char *argument = next_arg(args, &args);

	if (argument && strlen(argument) >= 2) {
	    if (*argument == '-' || *(argument + 1) == 'p') {
		char *v = next_arg(args, &args);

		portnum = (v) ? my_atol(v) : 0;
	    }
	}
    }

    real_dcc_filesend(FileBuf, filename, user, DCC_RESENDOFFER, portnum);
    new_free(&FileBuf);
}

void dcc_filesend(const char *command, char *args)
{
    char *user = NULL;
    char *filename = NULL, *fullname = NULL;
    char FileBuf[BIG_BUFFER_SIZE + 1];
    int portnum = 0;

    if (!(user = next_arg(args, &args)) || !(filename = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname and a filename for DCC send", NULL, NULL));
	return;
    }

    if (*filename == '/')
	strcpy(FileBuf, filename);

    else if (*filename == '~') {
	if (0 == (fullname = expand_twiddle(filename))) {
	    put_it("%s", convert_output_format("$G %RDCC%n Unable to expand: $0", "%s", filename));
	    return;
	}
	strcpy(FileBuf, fullname);
	new_free(&fullname);
    } else {
	getcwd(FileBuf, sizeof(FileBuf));
	strcat(FileBuf, "/");
	strcat(FileBuf, filename);
    }

    while (args && *args) {
	char *argument = next_arg(args, &args);

	if (argument && strlen(argument) >= 2) {
	    if (*argument == '-' && *(argument + 1) == 'p') {
		char *v = next_arg(args, &args);

		portnum = (v) ? my_atol(v) : 0;
	    }
	}
    }

    real_dcc_filesend(FileBuf, filename, user, DCC_FILEOFFER, portnum);
}

void multiget(char *usern, char *filen)
{
    DCC_list *dccList;
    char *newbuf = NULL;
    char *expand = NULL;
    char *q = NULL;

    doing_multi = 1;
    if ((q = strchr(filen, ' ')))
	*q = 0;
    for (dccList = ClientList; dccList; dccList = dccList->next) {
	if ((match(dccList->description, filen) || match(filen, dccList->description)) &&
	    !my_stricmp(usern, dccList->user) && (dccList->flags & DCC_TYPES) == DCC_FILEREAD) {
	    if (get_string_var(DCC_DLDIR_VAR)) {
		expand = expand_twiddle(get_string_var(DCC_DLDIR_VAR));
		malloc_sprintf(&newbuf, "%s %s/%s", usern, expand, dccList->description);
	    } else
		malloc_sprintf(&newbuf, "%s %s", usern, dccList->description);
	    if (!dcc_quiet)
		put_it("%s", convert_output_format("$G %RDCC%n Attempting DCC get: $0", "%s", dccList->description));
	    dcc_getfile(NULL, newbuf);
	    new_free(&expand);
	    new_free(&newbuf);
	}
    }
    doing_multi = 0;
}

static void dcc_getfile(const char *command, char *args)
{
    char *user;
    char *filename = NULL;
    char *tmp = NULL;
    DCC_list *Client;
    char *fullname = NULL;
    char *argument = NULL;

    if (0 == (user = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC get", NULL, NULL));
	return;
    }
    if (args && *args) {
	if (args && strlen(args) >= 2) {
	    if (*args != '-' && *(args + 1) != 'e')
		filename = next_arg(args, &args);
	    else if (args && *args == '-' && *(args + 1) == 'e') {
		argument = next_arg(args, &args);
	    }
	}
    }

    if (0 == (Client = dcc_searchlist(filename, user, DCC_FILEREAD, 0, NULL, NULL, 0))) {
	if (filename)
	    put_it("%s", convert_output_format("$G %RDCC%n No file ($0) offered in SEND mode by $1", "%s %s", filename, user));
	else
	    put_it("%s", convert_output_format("$G %RDCC%n No file offered in SEND mode by $0", "%s", user));
	return;
    }
    if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT)) {
	put_it("%s", convert_output_format("$G %RDCC%n A previous DCC get:$0 to $0 exists", "%s %s", filename, user));
	return;
    }
    Client->flags |= DCC_TWOCLIENTS;
    Client->bytes_sent = Client->bytes_read = 0L;

    if (!dcc_open(Client))
	return;
    if (get_string_var(DCC_DLDIR_VAR))
	malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), Client->description);
    else
	malloc_sprintf(&tmp, "%s", Client->description);
    if (!(fullname = expand_twiddle(tmp)))
	malloc_strcpy(&fullname, tmp);

    if (-1 == (Client->file = open(fullname, O_WRONLY | O_TRUNC | O_CREAT, 0644))) {
	put_it("%s",
	       convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", Client->description,
				     errno ? strerror(errno) : "Unknown"));
	if (dcc_active_count)
	    dcc_active_count--;
	Client->flags |= DCC_DELETE;
    }
    new_free(&fullname);
    new_free(&tmp);
}

void dcc_regetfile(const char *command, char *args)
{
    char *user;
    char *tmp = NULL;
    char *filename;
    DCC_list *Client;
    char *fullname = NULL;
    struct stat buf;

    if (0 == (user = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n You must supply a nickname for DCC reget", NULL, NULL));
	return;
    }
    filename = next_arg(args, &args);

    if (!(Client = dcc_searchlist(filename, user, DCC_REGETFILE, 0, NULL, NULL, 0))) {
	if (filename)
	    put_it("%s", convert_output_format("$G %RDCC%n No file ($0) offered in RESEND mode by $1", "%s %s", filename, user));
	else
	    put_it("%s", convert_output_format("$G %RDCC%n No file offered in RESEND mode by $0", "%s", user));
	return;
    }
    if ((Client->flags & DCC_ACTIVE) || (Client->flags & DCC_WAIT)) {
	put_it("%s", convert_output_format("$G %RDCC%n A previous DCC reget:$0 to $0 exists", "%s %s", filename, user));
	return;
    }
    Client->flags |= DCC_TWOCLIENTS;
    Client->bytes_sent = Client->bytes_read = 0L;

    if (get_string_var(DCC_DLDIR_VAR))
	malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), Client->description);
    else
	malloc_sprintf(&tmp, "%s", Client->description);
    if (!(fullname = expand_twiddle(tmp)))
	malloc_strcpy(&fullname, tmp);

    if (!dcc_open(Client))
	return;
    if (-1 == (Client->file = open(fullname, O_WRONLY | O_CREAT, 0644))) {
	put_it("%s",
	       convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", Client->description,
				     errno ? strerror(errno) : "Unknown"));
	Client->flags |= DCC_DELETE;
	if (dcc_active_count)
	    dcc_active_count--;
	return;
    }

    /* seek to the end of the file about to be resumed */
    lseek(Client->file, 0, SEEK_END);

    /* get the size of our file to be resumed */
    fstat(Client->file, &buf);

    Client->transfer_orders.packet_id = DCC_PACKETID;
    Client->transfer_orders.byteoffset = buf.st_size;
    Client->transfer_orders.byteorder = byteordertest();

    new_free(&fullname);
    new_free(&tmp);
}

void register_dcc_offer(char *user, char *type, char *description, char *address, char *port, char *size, char *extra, char *userhost)
{
    DCC_list *Client;
    int CType;
    char *c = NULL;
    u_long TempLong;
    unsigned TempInt;
    long packets = 0;

    if ((c = strrchr(description, '/')))
	description = c + 1;
    if ('.' == *description)
	*description = '_';

    TempInt = (unsigned) strtoul(port, NULL, 10);

    if (TempInt < 1024) {
	put_it("%s", convert_output_format("$G $RDCC%n Priveleged port attempt [$0]", "%d", TempInt));
	message_from(NULL, LOG_CRAP);
	return;
    }

    message_from(NULL, LOG_DCC);
    if (size && *size) {
	filesize = my_atol(size);
	packets = filesize / dccBlockSize() + 1;
    } else
	packets = filesize = 0;

    if (!my_stricmp(type, "CHAT"))
	CType = DCC_CHAT;
    else if (!my_stricmp(type, "SEND"))
	CType = DCC_FILEREAD;
    else if (!my_stricmp(type, "RESEND"))
	CType = DCC_REGETFILE;
    else {
	put_it("%s", convert_output_format("$G %RDCC%n Unknown DCC $0 ($1) recieved from $2", "%s %s %s", type, description, user));
	message_from(NULL, LOG_CRAP);
	return;
    }

    Client = dcc_searchlist(description, user, CType, 1, NULL, NULL, -1);
    filesize = 0;

    if (extra && *extra)
	Client->cksum = m_strdup(extra);

    if (Client->flags & DCC_WAIT) {
	if (DCC_CHAT == CType) {
	    Client = dcc_searchlist(description, user, CType, 1, NULL, NULL, -1);
	} else {

	    put_it("%s", convert_output_format("$G %RDCC%n $0 collision for $1:$2", "%s %s %s", type, user, description));
	    send_ctcp(CTCP_NOTICE, user, CTCP_DCC, "DCC %s collision occured while connecting to %s (%s)", type, nickname,
		      description);
	    message_from(NULL, LOG_CRAP);
	    return;
	}
    }
    if (Client->flags & DCC_ACTIVE) {
	put_it("%s",
	       convert_output_format("$G %RDCC%n Recieved DCC $0 request from $1 while previous session active", "%s %s", type, user));
	message_from(NULL, LOG_CRAP);
	return;
    }
    Client->flags |= DCC_OFFER;

    TempLong = strtoul(address, NULL, 10);
    Client->remote.s_addr = htonl(TempLong);
    Client->remport = htons(TempInt);

    if (userhost)
	Client->userhost = m_strdup(userhost);

    /* This right here compares the hostname from the userhost stamped on the incoming privmsg to the address stamped in the
     * handshake. We do not automatically reject any discrepencies, but warn the user instead to be cautious. */
    {
	char tmpbuf[128];
	char *fromhost;
	u_32int_t compare, compare2;
	u_32int_t compare3, compare4;
	struct hostent *hostent_fromhost;

	strncpy(tmpbuf, FromUserHost, 127);
	fromhost = strchr(tmpbuf, '@');
	fromhost++;
	alarm(1);		/* dont block too long... */
	hostent_fromhost = gethostbyname(fromhost);
	alarm(0);
	if (!hostent_fromhost) {
	    yell("Incoming handshake has an address [%s] that could not be figured out!", fromhost);
	    yell("Please use caution in deciding whether to accept it or not");
	} else {
	    compare = *((u_32int_t *) hostent_fromhost->h_addr_list[0]);
	    compare2 = inet_addr(fromhost);
	    compare3 = htonl(0x00000000);
	    compare4 = htonl(0xffffffff);
	    if (((compare != Client->remote.s_addr) && (compare2 != Client->remote.s_addr))) {
		say("WARNING: Fake dcc handshake detected! [%x]", Client->remote.s_addr);
		say("Unless you know where this dcc request is coming from");
		say("It is recommended you ignore it!");
	    }
	    if (compare3 == Client->remote.s_addr || compare4 == Client->remote.s_addr) {
		yell("WARNING: Fake dcc handshake detected! [%x]", Client->remote.s_addr);
		Client->flags = DCC_DELETE;
		message_from(NULL, LOG_CRAP);
		return;
	    }
	}
    }

    if ((u_long) 0 == TempLong || 0 == Client->remport) {
	yell("DCC handshake from %s ignored becuase it had an null port or address", user);
	Client->flags = DCC_DELETE;
	message_from(NULL, LOG_CRAP);
	return;
    } else if (do_hook(DCC_REQUEST_LIST, "%s %s %s %d", user, type, description, Client->filesize)) {
	if (!dcc_quiet) {

	    char buf[40];

	    sprintf(buf, "%2.4g", _GMKv(Client->filesize));
	    if (Client->filesize)
		put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_REQUEST_FSET),
						   "%s %s %s %s %s %s %d %s %s",
						   update_clock(GET_TIME), type, description, user, FromUserHost,
						   inet_ntoa(Client->remote), ntohs(Client->remport), _GMKs(Client->filesize), buf));
	    else
		put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_REQUEST_FSET),
						   "%s %s %s %s %s %s %d", update_clock(GET_TIME), type, description,
						   user, FromUserHost, inet_ntoa(Client->remote), ntohs(Client->remport)));

	}
	if (CType == DCC_CHAT) {
	    bitchsay("Type /chat to answer or /nochat to close");
	    malloc_sprintf(&last_chat_req, "%s", user);
	}
    }

    message_from(NULL, LOG_CRAP);
    return;
}

static void process_incoming_chat(DCC_list * Client)
{
    struct sockaddr_in remaddr;
    char tmp[MAX_DCC_BLOCK_SIZE + 1];
    char *s = NULL, *bufptr = NULL;
    char *buf = NULL;
    u_32int_t bytesread;
    int old_timeout;
    int len = 0;

    if (Client->flags & DCC_WAIT) {
	socklen_t sra = sizeof(struct sockaddr_in);

	Client->write = accept(Client->read, (struct sockaddr *) &remaddr, &sra);
	close(Client->read);
	Client->read = -1;
	if ((Client->read = Client->write) <= 0) {
	    put_it("%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));
	    Client->flags |= DCC_DELETE;
	    return;
	}
	Client->flags &= ~DCC_WAIT;
	Client->flags |= DCC_ACTIVE;
	if (do_hook(DCC_CONNECT_LIST, "%s CHAT %s %d", Client->user, inet_ntoa(remaddr.sin_addr)), ntohs(remaddr.sin_port))
	    put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
					       "%s %s %s %s %s %d", update_clock(GET_TIME),
					       "CHAT",
					       Client->user, Client->userhost ? Client->userhost : "u@h",
					       inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port)));
	get_time(&Client->starttime);
	return;
    }
    old_timeout = dgets_timeout(1);
    s = Client->buffer;
    bufptr = tmp;
    if (s && *s) {
	len = strlen(s);
	if (len > (MAX_DCC_BLOCK_SIZE / 2) - 1) {
	    put_it("%s", convert_output_format("$G %RDCC buffer overrun. Data lost", NULL, NULL));
	    new_free(&(Client->buffer));
	} else {
	    strncpy(tmp, s, len);
	    bufptr += len;
	}
    }
    bytesread = dgets(bufptr, dccBlockSize() - len, Client->read, NULL);
    (void) dgets_timeout(old_timeout);
    switch (bytesread) {
    case -1:
	add_to_dcc_buffer(Client, tmp);
	return;
    case 0:
	{
	    char *real_tmp = ((dgets_errno == -1) ? "Remote End Closed Connection" : strerror(dgets_errno));

	    if (do_hook(DCC_LOST_LIST, "%s CHAT %s", Client->user, real_tmp))
		put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET),
						   "%s %s %s %s", update_clock(GET_TIME), "CHAT", Client->user, real_tmp));
	    Client->flags |= DCC_DELETE;
	    return;
	}
    default:
	{
	    char userhost[BIG_BUFFER_SIZE + 1];
	    char equal_nickname[NICKNAME_LEN + 4];
	    int tmplen = strlen(tmp) - 1;

	    tmp[tmplen] = '\0';

	    if (tmp[tmplen - 1] == '\r') {
		tmplen--;
		tmp[tmplen] = '\0';
	    }

	    new_free(&Client->buffer);

	    Client->bytes_read += bytesread;
	    message_from(Client->user, LOG_DCC);

	    malloc_strcpy(&buf, tmp);

	    {
		{
		    if (!Client->userhost) {
			strcpy(userhost, "Unknown@");
			strcat(userhost, inet_ntoa(remaddr.sin_addr));
			FromUserHost = userhost;
		    } else
			FromUserHost = Client->userhost;

		    strncpy(equal_nickname, "=", NICKNAME_LEN);
		    strncat(equal_nickname, Client->user, NICKNAME_LEN);

		    if (tmp[0] == CTCP_DELIM_CHAR && tmp[tmplen - 1] == CTCP_DELIM_CHAR)
			do_ctcp(equal_nickname, get_server_nickname(from_server), stripansicodes(tmp));
		    else if (do_hook(DCC_CHAT_LIST, "%s %s", Client->user, tmp)) {
			addtabkey(equal_nickname, "msg");
			put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CHAT_FSET),
							   "%s %s %s %s", update_clock(GET_TIME), Client->user,
							   Client->userhost ? Client->userhost : "u@h", tmp));
		    }
		    FromUserHost = empty_str;
		}
	    }
	}
    }
    message_from(NULL, LOG_CRAP);
    new_free(&buf);
}

static void process_incoming_listen(DCC_list * Client)
{
    struct sockaddr_in remaddr;
    socklen_t sra;
    char FdName[10];
    DCC_list *NewClient;
    int new_socket;
    struct hostent *hp;

#if defined(__linux__) || defined(__sgi)
    const char *Name;
#else
    char *Name;
#endif

    sra = sizeof(struct sockaddr_in);
    new_socket = accept(Client->read, (struct sockaddr *) &remaddr, &sra);
    if (new_socket < 0) {
	put_it("%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));
	return;
    }
    if (0 != (hp = gethostbyaddr((char *) &remaddr.sin_addr, sizeof(remaddr.sin_addr), remaddr.sin_family)))
	Name = hp->h_name;
    else
	Name = inet_ntoa(remaddr.sin_addr);
    sprintf(FdName, "%d", new_socket);
    NewClient = dcc_searchlist((char *) Name, FdName, DCC_RAW, 1, NULL, NULL, 0);
    get_time(&NewClient->starttime);
    NewClient->read = NewClient->write = new_socket;
    NewClient->remote = remaddr.sin_addr;
    NewClient->remport = remaddr.sin_port;
    NewClient->flags |= DCC_ACTIVE;
    NewClient->bytes_read = NewClient->bytes_sent = 0L;
    if (do_hook(DCC_RAW_LIST, "%s %s N %d", NewClient->user, NewClient->description, Client->write))
	if (do_hook(DCC_CONNECT_LIST, "%s RAW %s %d", NewClient->user, NewClient->description, Client->write))
	    put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
					       "%s %s %s %s %s %d", update_clock(GET_TIME),
					       "RAW", NewClient->user,
					       Client->userhost ? Client->userhost : "u@h", NewClient->description, Client->write));
}

static void process_incoming_raw(DCC_list * Client)
{
    char tmp[MAX_DCC_BLOCK_SIZE + 1];
    char *s, *bufptr;
    u_32int_t bytesread;
    int old_timeout;
    int len = 0;

    s = Client->buffer;
    bufptr = tmp;
    if (s && *s) {
	len = strlen(s);
	if (len > MAX_DCC_BLOCK_SIZE - 1) {
	    put_it("%s", convert_output_format("$G %RDCC raw buffer overrun. Data lost", NULL, NULL));
	    new_free(&Client->buffer);
	} else {
	    strncpy(tmp, s, len);
	    bufptr += len;
	}
    }
    old_timeout = dgets_timeout(1);
    switch (bytesread = dgets(bufptr, dccBlockSize() - len, Client->read, NULL)) {
    case -1:
	{
	    add_to_dcc_buffer(Client, tmp);
	    return;
	}
    case 0:
	{
	    if (do_hook(DCC_RAW_LIST, "%s %s C", Client->user, Client->description))
		if (do_hook(DCC_LOST_LIST, "%s RAW %s", Client->user, Client->description))
		    put_it("%s",
			   convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), "RAW",
						 Client->user, Client->description));
	    Client->flags |= DCC_DELETE;
	    (void) dgets_timeout(old_timeout);
	    return;
	}
    default:
	{
	    new_free(&Client->buffer);
	    tmp[strlen(tmp) - 1] = '\0';
	    Client->bytes_read += bytesread;
	    if (Client->dcc_handler)
		(Client->dcc_handler) (Client, tmp);
	    else {
		if (do_hook(DCC_RAW_LIST, "%s %s D %s", Client->user, Client->description, tmp))
		    say("Raw data on %s from %s: %s", Client->user, Client->description, tmp);
	    }
	    dgets_timeout(old_timeout);
	    return;
	}
    }
}

/* Now that ive mucked this function up, i should go back and fix it. */
static void process_outgoing_file(DCC_list * Client, int readwaiting)
{
    struct sockaddr_in remaddr;
    socklen_t sra;
    char tmp[MAX_DCC_BLOCK_SIZE + 1];
    u_32int_t bytesrecvd = 0;
    int bytesread = 0;
    const char *type;
    unsigned char packet[BIG_BUFFER_SIZE / 8];
    struct transfer_struct *received;

    type = dcc_types[Client->flags & DCC_TYPES];
    if (Client->flags & DCC_WAIT) {
	sra = sizeof(struct sockaddr_in);
	Client->write = accept(Client->read, (struct sockaddr *) &remaddr, &sra);
	if ((Client->flags & DCC_RESENDOFFER) == DCC_RESENDOFFER) {
	    recv(Client->write, packet, sizeof(struct transfer_struct), 0);
	    received = (struct transfer_struct *) packet;
	    if (byteordertest() != received->byteorder) {
		/* the packet sender orders bytes differently than us, reverse what they sent to get the right value */
		Client->transfer_orders.packet_id = ((received->packet_id & 0x00ff) << 8) | ((received->packet_id & 0xff00) >> 8);

		Client->transfer_orders.byteoffset =
		    ((received->byteoffset & 0xff000000) >> 24) |
		    ((received->byteoffset & 0x00ff0000) >> 8) |
		    ((received->byteoffset & 0x0000ff00) << 8) | ((received->byteoffset & 0x000000ff) << 24);
	    } else
		memcpy(&Client->transfer_orders, packet, sizeof(struct transfer_struct));

	    if (Client->transfer_orders.packet_id != DCC_PACKETID)
		put_it("%s", convert_output_format("$G %RDCC%n reget packet is invalid!!", NULL, NULL));
	    else
		put_it("%s", convert_output_format("$G %RDCC%n reget starting at $0", "%d", Client->transfer_orders.byteoffset));
	}

	close(Client->read);

	if ((Client->read = Client->write) < 0) {
	    put_it("%s", convert_output_format("$G %RDCC error: accept() failed. punt!!", NULL, NULL));

	    Client->flags |= DCC_DELETE;
	    if (get_to_from(type) != -1 && dcc_active_count)
		dcc_active_count--;
	    return;
	}
	set_non_blocking(Client->write);
	Client->flags &= ~DCC_WAIT;
	Client->flags &= ~DCC_CNCT_PEND;
	Client->flags |= DCC_ACTIVE;
	Client->eof = 0;
	get_time(&Client->starttime);
	if ((Client->file = open(Client->description, O_RDONLY)) == -1) {
	    put_it("%s",
		   convert_output_format("$G %RDCC%n Unable to open $0: $1-", "%s %s", Client->description,
					 errno ? strerror(errno) : "Unknown Host"));
	    if (get_to_from(type) != -1 && dcc_active_count)
		dcc_active_count--;
	    close(Client->read);
	    Client->read = Client->write = (-1);
	    Client->flags |= DCC_DELETE;
	    return;
	}
	if ((Client->flags & DCC_RESENDOFFER) == DCC_RESENDOFFER) {
	    lseek(Client->file, Client->transfer_orders.byteoffset, SEEK_SET);
	    errno = 0;
	    if (do_hook(DCC_CONNECT_LIST, "%s %s %s %d", Client->user, "SEND", inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port)))
		if (!dcc_quiet)
		    put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
						       "%s %s %s %s %s %d %d", update_clock(GET_TIME), "RESEND",
						       Client->user, Client->userhost ? Client->userhost : "u@h",
						       inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port),
						       Client->transfer_orders.byteoffset));
	} else {
	    if (do_hook(DCC_CONNECT_LIST, "%s %s %s %d %s %d", Client->user, "SEND",
			inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port), Client->description, Client->filesize))
		if (!dcc_quiet)
		    put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_CONNECT_FSET),
						       "%s %s %s %s %s %d %d", update_clock(GET_TIME), "SEND",
						       Client->user, Client->userhost ? Client->userhost : "u@h",
						       inet_ntoa(remaddr.sin_addr), ntohs(remaddr.sin_port),
						       Client->transfer_orders.byteoffset));
	    if (Client->transfer_orders.byteoffset)
		lseek(Client->file, Client->transfer_orders.byteoffset, SEEK_SET);
	}
    } else if (readwaiting) {
	if (recv(Client->read, (char *) &bytesrecvd, sizeof(u_32int_t), 0) < sizeof(u_32int_t)) {
	    if (do_hook(DCC_LOST_LIST, "%s SEND %s CONNECTION LOST", Client->user, Client->description))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), "SEND",
					     Client->user, Client->description));
	    if (get_to_from(type) != -1 && dcc_active_count)
		dcc_active_count--;
	    Client->flags |= DCC_DELETE;
	    return;
	} else if (ntohl(bytesrecvd) != Client->bytes_sent)
	    return;
/* CDE do we just return here? */
    }
    if ((bytesread = read(Client->file, tmp, dccBlockSize())) > 0) {
	send(Client->write, tmp, bytesread, 0);
	Client->packets_transfer++;
	Client->bytes_sent += bytesread;
	update_transfer_buffer("");
	if (!(Client->packets_transfer % 20))
	    update_all_status(curr_scr_win, NULL, 0);
    } else
	DCC_close_filesend(Client, "SEND");
}

static void process_incoming_file(DCC_list * Client)
{
    char tmp[MAX_DCC_BLOCK_SIZE + 1];
    u_32int_t bytestemp;
    int bytesread;
    const char *type;

    type = dcc_types[Client->flags & DCC_TYPES];

    if ((bytesread = read(Client->read, tmp, MAX_DCC_BLOCK_SIZE)) <= 0) {
	if (Client->bytes_read + Client->transfer_orders.byteoffset < Client->filesize)
	    put_it("%s", convert_output_format("$G %RDCC get to $0 lost: Remote peer closed connection", "%s", Client->user));
	DCC_close_filesend(Client, "GET");
    } else {
	write(Client->file, tmp, bytesread);

	Client->bytes_read += bytesread;
	bytestemp = htonl(Client->bytes_read);
	send(Client->write, (char *) &bytestemp, sizeof(u_32int_t), 0);
	Client->packets_transfer++;

/* TAKE THIS OUT IF IT CAUSES PROBLEMS */
	if (Client->filesize) {
	    if (Client->bytes_read > Client->filesize) {
		put_it("%s", convert_output_format("$G %RDCC%n Warning: incoming file is larger than the handshake said", NULL, NULL));
		put_it("%s", convert_output_format("$G %RDCC%n Warning: GET: closing connection", NULL, NULL));
		if (dcc_active_count)
		    dcc_active_count--;
		Client->flags |= DCC_DELETE;
		update_all_status(curr_scr_win, NULL, 0);
		return;
	    } else if (Client->bytes_read == Client->filesize) {
		DCC_close_filesend(Client, "GET");
		return;
	    }
	}
	update_transfer_buffer("");
	if (!(Client->packets_transfer % 20))
	    update_all_status(curr_scr_win, NULL, 0);
    }
}

/* flag == 1 means show it.  flag == 0 used by redirect (and /ctcp) */

static void dcc_message_transmit(const char *user, char *text, const char *text_display, int type, int flag, const char *cmd, int check_host)
{
    DCC_list *Client;
    char tmp[MAX_DCC_BLOCK_SIZE + 1];
    int lastlog_level;
    int len = 0;
    char *host = NULL;
    char thing = 0;

    *tmp = 0;
    switch (type) {
    case DCC_CHAT:
	thing = '=';
	host = "chat";
	break;
    case DCC_RAW:
	if (check_host) {
	    if (!(host = next_arg(text, &text))) {
		put_it("%s", convert_output_format("$G %RDCC%n No host specified for DCC RAW", NULL, NULL));
		return;
	    }
	}
	break;
    }
    if (!(Client = dcc_searchlist(host, user, type, 0, NULL, NULL, 1)) || !(Client->flags & DCC_ACTIVE)) {
	put_it("%s",
	       convert_output_format("$G %RDCC No active $0:$1 connection for $2", "%s %s %s", dcc_types[type], host ? host : "(null)",
				     user));
	return;
    }
    lastlog_level = set_lastlog_msg_level(LOG_DCC);
    message_from(Client->user, LOG_DCC);

#if 0
    /* 
     * Check for CTCPs... whee.
     */
    if (cmd && *text == CTCP_DELIM_CHAR) {
	if (!strcmp(cmd, "PRIVMSG"))
	    strmcpy(tmp, "CTCP_MESSAGE ", dccBlockSize());
	else
	    strmcpy(tmp, "CTCP_REPLY ", dccBlockSize());
    }
#endif
    strmcat(tmp, text, dccBlockSize() - 3);
    strmcat(tmp, "\n", dccBlockSize() - 2);

    len = strlen(tmp);
    write(Client->write, tmp, len);
    Client->bytes_sent += len;

    if (flag && type != DCC_RAW) {
	if (!my_strnicmp(tmp, ".chat", strlen(tmp)))
	    Client->in_dcc_chat = 1;
	if (do_hook(SEND_DCC_CHAT_LIST, "%s %s", Client->user, text_display ? text_display : text))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_SEND_DCC_CHAT_FSET), "%c %s %s", thing, Client->user,
					 text_display ? text_display : text));
    }
    set_lastlog_msg_level(lastlog_level);
    message_from(NULL, LOG_CRAP);
    return;
}

void dcc_chat_transmit(const char *user, char *text, const char *orig, const char *type)
{
    dcc_message_transmit(user, text, orig, DCC_CHAT, 1, type, 0);
}

void dcc_raw_transmit(const char *user, char *text, const char *type)
{
    dcc_message_transmit(user, text, NULL, DCC_RAW, 0, type, 0);
}

void dcc_chat_crash_transmit(char *user, char *text)
{
    char buffer[20000];
    DCC_list *Client;
    char *host = "chat";

    memset(buffer, ' ', 20000 - 10);
    buffer[20000 - 9] = '\n';
    buffer[20000 - 8] = '\0';
    if (!(Client = dcc_searchlist(host, user, DCC_CHAT, 0, NULL, NULL, 1)) || !(Client->flags & DCC_ACTIVE)) {
	put_it("%s",
	       convert_output_format("$G %RDCC%n No active DCC $0:$1 connection for $2", "%s %s %s", dcc_types[DCC_CHAT],
				     host ? host : "(null)", user));
	return;
    }
    send(Client->write, buffer, strlen(buffer), 0);
    Client->bytes_sent += strlen(buffer);
    return;
}

static void dcc_send_raw(const char *command, char *args)
{
    char *name;

    if (!(name = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n No name specified for DCC raw", NULL, NULL));
	return;
    }
    dcc_message_transmit(name, args, NULL, DCC_RAW, 1, NULL, 1);
}

/*
 * dcc_time: Given a time value, it returns a string that is in the
 * format of "hours:minutes:seconds month day year" .  Used by 
 * dcc_list() to show the start time.
 */
char *dcc_time(time_t time)
{
    struct tm *btime;
    char *buf = NULL;
    static const char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    btime = localtime(&time);
    if (time)
	malloc_sprintf(&buf, "%-2.2d:%-2.2d:%-2.2d %s %-2.2d %d", btime->tm_hour,
		       btime->tm_min, btime->tm_sec, months[btime->tm_mon], btime->tm_mday, btime->tm_year + 1900);
    return buf;
}

static char *get_dcc_type(unsigned long flag)
{
    static char ret[20];

    strcpy(ret, dcc_types[flag & DCC_TYPES]);
    return ret;
}

void dcc_list(const char *command, char *args)
{
    DCC_list *Client;
    static char *format = "%-5.5s%-3.3s %-9.9s %-8.8s %-20.20s %-8.8s %-8.8s %s";
    unsigned flags;
    int count = 0;
    char *filename = NULL;

    for (Client = ClientList; Client != NULL; Client = Client->next) {
	char completed[9];
	char size[9];
	char *stime = NULL;

	if (Client->filesize) {
	    sprintf(completed, "%ld%%",
		    (unsigned long) (Client->bytes_sent ? Client->bytes_sent : Client->bytes_read) * 100 / Client->filesize);
	    sprintf(size, "%ld", (unsigned long) Client->filesize);
	} else {
	    sprintf(completed, "%ldK", (unsigned long) (Client->bytes_sent ? Client->bytes_sent : Client->bytes_read) / 1024);
	    strcpy(size, empty_str);
	}
	stime = Client->starttime.tv_sec ? dcc_time(Client->starttime.tv_sec) : "";
	flags = Client->flags;

	if (!dcc_paths)
	    filename = strrchr(Client->description, '/');

	if (!filename)
	    filename = Client->description;

	if (!count)
	    put_it(format, "Type", " ", "Nick", "Status", "Start time", "Size", "Complete", "Arguments");
	put_it(format,
	       get_dcc_type(flags & DCC_TYPES),
	       Client->user,
	       flags & DCC_DELETE ? "Closed" :
	       flags & DCC_ACTIVE ? "Active" :
	       flags & DCC_WAIT ? "Waiting" : flags & DCC_OFFER ? "Offered" : "Unknown", stime, size, completed, filename);

	if (stime && *stime)
	    new_free(&stime);
	count++;
    }
}

void dcc_glist(const char *command, char *args)
{
    DCC_list *Client = ClientList;

    static const char *dformat = "#$[2]0 $[6]1%Y$2%n $[11]3 $[25]4 $[7]5 $6";
    static const char *d1format = "#$[2]0 $[6]1%Y$2%n $[11]3 $4 $[11]5 $[10]6 $[7]7 $8-";
    static const char *c1format = "#$[2]0 $[6]1%Y$2%n $[11]3 $4 $[-4]5 $[-4]6 $[-3]7 $[-3]8  $[7]9 $10";

    unsigned flags;
    unsigned count = 0;
    int size = 0;
    double barsize = 0.0, perc = 0.0;
    int barlen = BAR_LENGTH;
    char spec[BIG_BUFFER_SIZE + 1];
    const char *type;

    barlen = BAR_LENGTH;
    barsize = 0.0;
    memset(spec, 0, sizeof(spec) - 1);

    if (ClientList && do_hook(DCC_HEADER_LIST, "%s %s %s %s %s %s %s", "Dnum", "Type", "Nick", "Status", "K/s", "File")) {
	put_it("%s",
	       convert_output_format
	       ("%G#  %W|%n %GT%gype  %W|%n %GN%gick      %W|%n %GP%gercent %GC%gomplete        %W|%n %GK%g/s   %W|%n %GF%gile", NULL,
		NULL));
	put_it("%s",
	       convert_output_format("%W---------------------------------------------------------------------------", NULL, NULL));

    }
    for (Client = ClientList; Client != NULL; Client = Client->next) {
	flags = Client->flags & DCC_TYPES;
	if ((flags == DCC_FILEOFFER || flags == DCC_FILEREAD || flags == DCC_RESENDOFFER || flags == DCC_REGETFILE))
	    if ((Client->flags & DCC_ACTIVE))
		continue;
	type = get_dcc_type(flags & DCC_TYPES);
	if (do_hook(DCC_STAT_LIST, "%d %s %s %s %s %s",
		    Client->dccnum, type,
		    Client->user,
		    Client->flags & DCC_OFFER ? "Offer" :
		    Client->flags & DCC_DELETE ? "Close" :
		    Client->flags & DCC_ACTIVE ? "Active" :
		    Client->flags & DCC_WAIT ? "Wait" :
		    Client->flags & DCC_CNCT_PEND ? "Connect" : "Unknown", "N/A", Client->description)) {
	    time_t u_time = (time_t) time_diff(Client->starttime, get_time(NULL));

	    if ((((flags & DCC_TYPES) == DCC_CHAT) || ((flags & DCC_TYPES) == DCC_RAW))) {
		put_it("%s", convert_output_format(c1format, "%d %s %s %s %s %s %s",
						   Client->dccnum,
						   type,
						   Client->user,
						   "Active", convert_time(u_time), "N/A", check_paths(Client->description)));
	    } else {
		put_it("%s", convert_output_format(dformat, "%d %s %s %s %s %s",
						   Client->dccnum,
						   type,
						   Client->user,
						   Client->flags & DCC_OFFER ? "Offer" :
						   Client->flags & DCC_DELETE ? "Close" :
						   Client->flags & DCC_ACTIVE ? "Active" :
						   Client->flags & DCC_WAIT ? "Wait" :
						   Client->flags & DCC_CNCT_PEND ? "Connect" :
						   "Unknown", "N/A", check_paths(Client->description)));
	    }
	}

	count++;
    }
    for (Client = ClientList; Client != NULL; Client = Client->next) {
	char kilobytes[100];
	time_t xtime = time(NULL) - Client->starttime.tv_sec;
	double bytes = Client->bytes_read + Client->bytes_sent + Client->transfer_orders.byteoffset, sent;
	char stats[80];
	int seconds = 0, minutes = 0;
	int iperc = 0;
	static const char *dcc_offer[12] = {
	    "%K-.........%n",	/* 0 */
	    "%K-.........%n",	/* 10 */
	    "%K-=........%n",	/* 20 */
	    "%K-=*.......%n",	/* 30 */
	    "%K-=*%1%K=%0%K......%n",	/* 40 */
	    "%K-=*%1%K=-%0%K.....%n",	/* 50 */
	    "%K-=*%1%K=-.%0%K....%n",	/* 60 */
	    "%K-=*%1%K=-. %0%K...%n",	/* 70 */
	    "%K-=*%1%K=-. %R.%0%K..%n",	/* 80 */
	    "%K-=*%1%K=-. %R.-%0%K.%n",	/* 90 */
	    "%K-=*%1%K=-. %R.-=%n",	/* 100 */
	    ""
	};
	char *bar_end;

	*stats = 0;
	*kilobytes = 0;
	perc = 0.0;
	sent = bytes - Client->transfer_orders.byteoffset;
	flags = Client->flags & DCC_TYPES;

	if ((Client->flags & DCC_WAIT || Client->flags & DCC_OFFER) || ((flags & DCC_FILEOFFER) == 0) || ((flags & DCC_FILEREAD) == 0)
	    || ((flags & DCC_RESENDOFFER) == 0) || ((flags & DCC_REGETFILE) == 0))
	    continue;

	sent /= (double) 1024.0;
	if (xtime <= 0)
	    xtime = 1;
	sprintf(kilobytes, "%2.4g", sent / (double) xtime);

	if ((bytes >= 0) && (Client->flags & DCC_ACTIVE)) {
	    if (bytes && Client->filesize >= bytes) {
		perc = (100.0 * ((double) bytes) / (double) (Client->filesize));
		if (perc > 100.0)
		    perc = 100.0;
		else if (perc < 0.0)
		    perc = 0.0;
		seconds = (int) (((Client->filesize - bytes) / (bytes / xtime)) + 0.5);
		minutes = seconds / 60;
		seconds = seconds - (minutes * 60);
		if (minutes > 999) {
		    minutes = 999;
		    seconds = 59;
		}
		if (seconds < 0)
		    seconds = 0;
	    } else
		seconds = minutes = perc = 0;

	    iperc = ((int) perc) / 10;
	    barsize = ((double) (Client->filesize)) / (double) barlen;

	    size = (int) ((double) bytes / (double) barsize);

	    if (Client->filesize == 0)
		size = barlen;
	    sprintf(stats, "%4.1f", perc);
	    sprintf(spec, "%s %s%s %02d:%02d", dcc_offer[iperc], stats, "%%", minutes, seconds);
	    sprintf(spec, "%s", convert_output_format(spec, NULL, NULL));
	}
	type = get_dcc_type(flags & DCC_TYPES);
	if (do_hook(DCC_STATF_LIST, "%d %s %s %s %s %s",
		    Client->dccnum, type,
		    Client->user,
		    Client->flags & DCC_OFFER ? "Offer" :
		    Client->flags & DCC_DELETE ? "Close" :
		    Client->flags & DCC_ACTIVE ? "Active" :
		    Client->flags & DCC_WAIT ? "Wait" :
		    Client->flags & DCC_CNCT_PEND ? "Connect" : "Unknown", kilobytes, check_paths(Client->description))) {
	    const char *s;

	    if (get_int_var(DISPLAY_ANSI_VAR)) {
		if (!get_int_var(DCC_BAR_TYPE_VAR))
		    s = d1format;
		else
		    s = dformat;
	    } else
		s = dformat;
	    put_it("%s", convert_output_format(s, "%d %s %s %s %s %s",
					       Client->dccnum, type,
					       Client->user,
					       Client->flags & DCC_OFFER ? "Offer" :
					       Client->flags & DCC_DELETE ? "Close" :
					       Client->
					       flags & DCC_ACTIVE ? ((get_int_var(DISPLAY_ANSI_VAR) && !get_int_var(DCC_BAR_TYPE_VAR))
								     ? spec : "Active") : Client->flags & DCC_WAIT ? "Wait" : Client->
					       flags & DCC_CNCT_PEND ? "Connect" : "?????", kilobytes,
					       strip_path(Client->description)));
	}

	if (do_hook(DCC_STATF1_LIST, "%s %ld %ld %d %d",
		    stats, (unsigned long) bytes, (unsigned long) Client->filesize,
		    minutes, seconds) && (!get_int_var(DISPLAY_ANSI_VAR) || get_int_var(DCC_BAR_TYPE_VAR))) {
	    sprintf(stats, "%4.1f%% (%ld of %ld bytes)", perc, (unsigned long) bytes, (unsigned long) Client->filesize);
	    strcpy(spec, "\002[\026");
	    sprintf(spec + 3, "%*s", size + 1, " ");
	    bar_end = spec + (strlen(spec));
	    sprintf(bar_end, "%*s", barlen - size + 1, " ");
	    strcat(spec, "\026\002]\002 ETA %02d:%02d");
	    memcpy((spec + (((BAR_LENGTH + 2) / 2) - (strlen(stats) / 2))), stats, strlen(stats));
	    if (size < barlen) {
		memmove(bar_end + 1, bar_end, strlen(bar_end));
		*bar_end = '\002';
	    }
	    put_it(spec, minutes, seconds);
	}
	count++;
    }
    if (ClientList && count)
	do_hook(DCC_POST_LIST, "%s %s %s %s %s %s %s", "DCCnum", "Type", "Nick", "Status", "K/s", "File");
    if (count == 0)
	put_it("%s", convert_output_format("$G %RDCC%n Nothing on DCC list.", NULL, NULL));
}

static char DCC_reject_type[12];
static char DCC_reject_description[40];

static void output_reject_ctcp(char *notused, char *nicklist)
{
    if (nicklist && *nicklist && *DCC_reject_description)
	send_ctcp(CTCP_NOTICE, nicklist, CTCP_DCC, "REJECT %s %s", DCC_reject_type, DCC_reject_description);
    strcpy(DCC_reject_description, empty_str);
    strcpy(DCC_reject_type, empty_str);
}

/* added by Patch */
void dcc_close_client_num(unsigned int closenum)
{
    DCC_list *Client, *next;
    unsigned flags;
    const char *type;
    int to_from_idx;

    for (Client = ClientList; Client != NULL; Client = next) {
	flags = Client->flags;
	type = get_dcc_type(flags & DCC_TYPES);
	/* dcc_types[flags & DCC_TYPES]; */
	to_from_idx = get_to_from(type);
	next = Client->next;

	if (Client->dccnum == closenum) {
	    if (flags & DCC_DELETE)
		return;
	    if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
		if (to_from_idx != -1 && dcc_active_count)
		    dcc_active_count--;
		close(Client->read);
		if (Client->file)
		    close(Client->file);
	    }

	    if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, type, Client->description ? Client->description : "<any>"))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), type,
					     Client->user, Client->description));
	    dcc_reject_notify(Client->description, Client->user, type);
	    dcc_erase(Client);
	    update_transfer_buffer("");
	    update_all_status(curr_scr_win, NULL, 0);
	    return;
	}
    }

    put_it("%s", convert_output_format("%RDCC%n CLOSE number $0 does not exist", "%d", closenum));
}

/* added by Patch */
void dcc_close_all(void)
{
    DCC_list *Client, *next;
    unsigned flags;
    const char *type;
    int to_from_idx;

    for (Client = ClientList; Client != NULL; Client = next) {
	flags = Client->flags;
	type = get_dcc_type(flags & DCC_TYPES);
	/* dcc_types[flags & DCC_TYPES]; */
	to_from_idx = get_to_from(type);
	next = Client->next;

	if (flags & DCC_DELETE)
	    continue;
	if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
	    if (to_from_idx != -1 && dcc_active_count)
		dcc_active_count--;
	}

	if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, type, Client->description ? Client->description : "<any>"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), type,
					 Client->user, Client->description));
	dcc_reject_notify(Client->description, Client->user, type);
	dcc_erase(Client);
	update_transfer_buffer("");
	update_all_status(curr_scr_win, NULL, 0);
    }
}

/* added by Patch */
void dcc_close_type_all(char *typestr)
{
    DCC_list *Client, *next;
    unsigned flags;
    const char *type;
    int to_from_idx;

    to_from_idx = get_to_from(typestr);

    for (Client = ClientList; Client != NULL; Client = next) {
	flags = Client->flags;
	type = get_dcc_type(flags & DCC_TYPES);
	/* dcc_types[flags & DCC_TYPES]; */
	next = Client->next;

	if (my_stricmp(type, typestr) == 0) {
	    if (flags & DCC_DELETE)
		return;
	    if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
		if (to_from_idx != -1 && dcc_active_count)
		    dcc_active_count--;
		close(Client->read);
		if (Client->file)
		    close(Client->file);
	    }
	    if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, type, Client->description ? Client->description : "<any>"))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), type,
					     Client->user, Client->description));
	    dcc_reject_notify(Client->description, Client->user, type);
	    dcc_erase(Client);
	    update_transfer_buffer("");
	    update_all_status(curr_scr_win, NULL, 0);
	}
    }
}

/* added by Patch */
void dcc_close_nick_all(char *nickstr)
{
    DCC_list *Client, *next;
    unsigned flags;
    char *Type;
    int to_from_idx;

    for (Client = ClientList; Client != NULL; Client = next) {
	flags = Client->flags;
	Type = get_dcc_type(flags & DCC_TYPES);
	/* dcc_types[flags & DCC_TYPES]; */
	to_from_idx = get_to_from(Type);
	next = Client->next;

	if (my_stricmp(Client->user, nickstr) == 0) {
	    if (flags & DCC_DELETE)
		return;
	    if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
		if (to_from_idx != -1 && dcc_active_count)
		    dcc_active_count--;
		close(Client->read);
		if (Client->file)
		    close(Client->file);
	    }

	    if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, Type, Client->description ? Client->description : "<any>"))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), Type,
					     Client->user, Client->description));
	    dcc_reject_notify(Client->description, Client->user, Type);
	    dcc_erase(Client);
	    update_transfer_buffer("");
	    update_all_status(curr_scr_win, NULL, 0);
	}
    }
}

/* added by Patch */
void dcc_close_type_nick_all(char *typestr, char *nickstr)
{
    DCC_list *Client, *next;
    unsigned flags;
    char *Type;
    int to_from_idx;

    to_from_idx = get_to_from(typestr);

    for (Client = ClientList; Client != NULL; Client = next) {
	flags = Client->flags;
	Type = get_dcc_type(flags & DCC_TYPES);
	/* dcc_types[flags & DCC_TYPES]; */
	next = Client->next;

	if (my_stricmp(Type, typestr) == 0 && my_stricmp(Client->user, nickstr) == 0) {
	    if (flags & DCC_DELETE)
		return;
	    if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
		if (to_from_idx != -1 && dcc_active_count)
		    dcc_active_count--;
		close(Client->read);
		if (Client->file)
		    close(Client->file);
	    }
	    if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, Type, Client->description ? Client->description : "<any>"))
		put_it("%s",
		       convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), Type,
					     Client->user, Client->description));
	    dcc_reject_notify(Client->description, Client->user, Type);
	    dcc_erase(Client);
	    update_transfer_buffer("");
	    update_all_status(curr_scr_win, NULL, 0);
	}
    }
}

/* added by Patch */
void dcc_close_filename(char *filename, char *user, char *Type, int CType)
{
    DCC_list *Client;
    unsigned flags;
    int to_from_idx;

    to_from_idx = get_to_from(Type);
    if ((Client = dcc_searchlist(filename, user, CType, 0, filename, NULL, -1))) {
	flags = Client->flags;
	if (flags & DCC_DELETE)
	    return;
	if ((flags & DCC_WAIT) || (flags & DCC_ACTIVE)) {
	    if (to_from_idx != -1 && dcc_active_count)
		dcc_active_count--;
	}

	if (do_hook(DCC_LOST_LIST, "%s %s %s", Client->user, Type, Client->description ? Client->description : "<any>"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), Type,
					 Client->user, Client->description));
	dcc_reject_notify(Client->description, Client->user, Type);
	dcc_erase(Client);
	update_transfer_buffer("");
	update_all_status(curr_scr_win, NULL, 0);
    } else
	put_it("%s",
	       convert_output_format("$G %RDCC%n No DCC $0:$1 to $2 found", "%s %s %s", Type, filename ? filename : "(null)", user));
}

/* completely rewritten by Patch */
static void dcc_close(const char *command, char *args)
{
    char *Type;
    char *user;
    char *description;
    int CType;
    unsigned int closenum;

    Type = next_arg(args, &args);
    user = next_arg(args, &args);
    description = next_arg(args, &args);

/*****************************************************************************
 DCC CLOSE #all or #number
*****************************************************************************/

    if (!Type) {
	put_it("%s", convert_output_format("$G %RDCC%n You must specify a type|dcc_num for DCC close", NULL, NULL));
	return;
    }

    if ((Type[0] == '#' && !user && *(Type + 1) && my_atol(Type + 1)) || !my_stricmp(Type, "-all")) {
	if (!my_stricmp(Type, "-all")) {
	    dcc_close_all();
	    dcc_active_count = 0;
	    update_all_status(curr_scr_win, NULL, 0);
	    return;
	}

	closenum = atol(Type + 1);

	if (closenum == 0) {
	    put_it("%s", convert_output_format("$G %RDCC%n close invalid number", NULL, NULL));
	    return;
	}

	dcc_close_client_num(closenum);
	update_all_status(curr_scr_win, NULL, 0);
	return;
    }

/*****************************************************************************
 DCC CLOSE SEND|GET|RESEND|REGET|nick #all
*****************************************************************************/

    if (!user) {
	put_it("%s", convert_output_format("$G %RDCC%n specify a type and a nick for DCC close", NULL, NULL));
	return;
    }

    for (CType = 0; dcc_types[CType] != NULL; CType++)
	if (!my_stricmp(Type, dcc_types[CType]))
	    break;

    if (user[0] == '#' && !description && (my_atol(user + 1) || my_stricmp(user + 1, "all") == 0)) {
	if (my_stricmp(user + 1, "all") == 0) {
	    if (!dcc_types[CType])
		dcc_close_nick_all(Type);
	    else
		dcc_close_type_all(Type);
	    update_all_status(curr_scr_win, NULL, 0);
	    return;
	}
	put_it("%s", convert_output_format("$G %RDCC%n close invalid number", NULL, NULL));
	return;
    }

/*****************************************************************************
 DCC CLOSE SEND|GET|RESEND|REGET nick #all|filename
*****************************************************************************/

    if (description && *description == '-') {
	if (!my_stricmp(description, "-all"))
	    dcc_close_type_nick_all(Type, user);
	else
	    put_it("%s", convert_output_format("$G %RDCC%n CLOSE invalid description", NULL, NULL));
	return;
    }

    if (dcc_types[CType])
	dcc_close_filename(description, user, Type, CType);
    else
	put_it("%s", convert_output_format("$G %RDCC%n Unknown type [$0]", "%s", dcc_types[CType]));
}

void dcc_reject_notify(char *description, char *user, const char *type)
{
    strcpy(DCC_reject_description, description ? description : "(null)");
    if (!my_stricmp(type, "SEND"))
	strcpy(DCC_reject_type, "GET");
    else if (!my_stricmp(type, "GET"))
	strcpy(DCC_reject_type, "SEND");
    else if (!my_stricmp(type, "RESEND"))
	strcpy(DCC_reject_type, "REGET");
    else if (!my_stricmp(type, "REGET"))
	strcpy(DCC_reject_type, "RESEND");
    else
	strcpy(DCC_reject_type, type);
    if (*user == '=')
	user++;
    add_ison_to_whois(user, output_reject_ctcp);
}

void dcc_reject(char *from, char *type, char *args)
{
    DCC_list *Client;
    char *description;
    int CType;

    upper(type);
    for (CType = 0; dcc_types[CType] != NULL; CType++)
	if (!strcmp(type, dcc_types[CType]))
	    break;

    if (!dcc_types[CType])
	return;

    description = next_arg(args, &args);

    if ((Client = dcc_searchlist(NULL, from, CType, 0, description, NULL, -1))) {
	if (Client->flags & DCC_DELETE)
	    return;
	if (do_hook(DCC_LOST_LIST, "%s %s %s REJECTED", from, type, description ? description : "<any>"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DCC_ERROR_FSET), "%s %s %s %s", update_clock(GET_TIME), type,
					 Client->user, Client->description));
	update_transfer_buffer("");
	Client->flags |= DCC_DELETE;
	update_all_status(curr_scr_win, NULL, 0);
    }
}

static void dcc_rename(const char *command, char *args)
{
    DCC_list *Client;
    char *user;
    char *description;
    char *newdesc;
    char *temp;

    if (!(user = next_arg(args, &args)) || !(temp = next_arg(args, &args))) {
	put_it("%s", convert_output_format("$G %RDCC%n You must specify a nick and a new filename", NULL, NULL));
	return;
    }
    if ((newdesc = next_arg(args, &args)) != NULL)
	description = temp;
    else {
	newdesc = temp;
	description = NULL;
    }

    if ((Client = dcc_searchlist(description, user, DCC_FILEREAD, 0, NULL, NULL, 0))) {
	/* Is this needed now? */
	if (!(Client->flags & DCC_OFFER)) {
	    put_it("%s", convert_output_format("$G %RDCC Too late to rename that file", NULL, NULL));
	    return;
	}
	new_free(&(Client->description));
	malloc_strcpy(&(Client->description), newdesc);
	put_it("%s",
	       convert_output_format("$G %RDCC File $0 from $1 rename to $2", "%s %s %s", description ? description : "(null)", user,
				     newdesc));
    } else
	put_it("%s",
	       convert_output_format("$G %RDCC No file $0 from $1 found, or it's too late to rename", "%s %s",
				     description ? description : "(null)", user));
}

/*
 * close_all_dcc:  We call this when we create a new process so that
 * we don't leave any fd's lying around, that won't close when we
 * want them to..
 */
void close_all_dcc(void)
{
    DCC_list *Client, *last;

    for (Client = ClientList; Client; Client = last) {
	last = Client->next;
	dcc_erase(Client);
    }
    dcc_active_count = 0;
}

static void add_to_dcc_buffer(DCC_list * Client, char *buf)
{
    if (buf && *buf) {
	if (Client->buffer)
	    malloc_strcat(&Client->buffer, buf);
	else
	    malloc_strcpy(&Client->buffer, buf);
    }
}

/* Looks for the dcc transfer that is "current" (last recieved data)
 * and returns information for it
 */
char *DCC_get_current_transfer(void)
{
    return DCC_current_transfer_buffer;
}

void cmd_chat(struct command *cmd, char *args)
{
    int no_chat = 0;

    if (*cmd->name == 'N')
	no_chat = 1;
    if (args && *args) {
	char *tmp = NULL;

	if (no_chat)
	    malloc_sprintf(&tmp, "CLOSE CHAT %s", args);
	else
	    malloc_sprintf(&tmp, "CHAT %s", args);
	process_dcc(tmp);
	new_free(&tmp);
    } else if (last_chat_req) {
	DCC_list *chat_req = NULL;

	if ((chat_req = dcc_searchlist("chat", last_chat_req, DCC_CHAT, 0, NULL, NULL, no_chat ? -1 : 0))) {
	    if (no_chat) {
		dcc_close_filename(NULL, last_chat_req, "CHAT", DCC_CHAT);
		new_free(&last_chat_req);
	    } else {
		char nick[BIG_BUFFER_SIZE];

		dcc_open(chat_req);
		sprintf(nick, "=%s", chat_req->user);
		addtabkey(nick, "msg");
	    }
	} else {
	    bitchsay("Error occurred");
	    new_free(&last_chat_req);
	}
    } else
	userage(cmd->name, cmd->qhelp);
}

static void DCC_close_filesend(DCC_list * Client, char *type)
{
    char lame_ultrix[30];	/* should be plenty */
    char lame_ultrix2[30];
    char lame_ultrix3[30];
    char buffer[200];
    const char *tofrom = NULL;
    time_t xtime;
    double xfer;

    xtime = time_diff(Client->starttime, get_time(NULL));
    xfer = (double) (Client->bytes_sent ? Client->bytes_sent : Client->bytes_read);

    if (xfer <= 0)
	xfer = 1;
    if (xtime <= 0)
	xtime = 1;
    sprintf(lame_ultrix, "%2.4g", (xfer / 1024.0 / xtime));
    /* Cant pass %g to put_it (lame ultrix/dgux), fix suggested by sheik. */
    sprintf(lame_ultrix2, "%2.4g%s", _GMKv(xfer), _GMKs(xfer));
    sprintf(lame_ultrix3, "%2.4g", (double) xtime);
    sprintf(buffer, "%%s %s %%s %%s TRANSFER COMPLETE", type);

    switch (get_to_from(dcc_types[Client->flags & DCC_TYPES])) {
    case 0:
    case 1:
	tofrom = "to";
	break;
    case 2:
    case 3:
	tofrom = "from";
	break;
    default:
	tofrom = "to";
	break;
    }
    if (do_hook(DCC_LOST_LIST, buffer, Client->user, check_paths(Client->description), lame_ultrix))
	put_it("%s", convert_output_format(get_fset_var(FORMAT_DCC_LOST_FSET),
					   "%s %s %s %s %s %s %s %s %s", update_clock(GET_TIME), type,
					   check_paths(Client->description), lame_ultrix2, tofrom, Client->user,
					   lame_ultrix3, lame_ultrix, "Kb"));

    if (get_to_from(dcc_types[Client->flags & DCC_TYPES]) != -1 && dcc_active_count)
	dcc_active_count--;
    close(Client->read);
    if (Client->read != Client->write)
	close(Client->write);
    close(Client->file);
    Client->file = Client->read = Client->write = -1;
    Client->flags |= DCC_DELETE;
    *DCC_current_transfer_buffer = 0;
    update_transfer_buffer("");
    update_all_status(curr_scr_win, NULL, 0);
}

char transfer_buffer[BIG_BUFFER_SIZE];

static void update_transfer_buffer(const char *format, ...)
{
    register DCC_list *Client = ClientList;
    unsigned count = 0;
    double perc = 0.0;
    char temp_str[60];

    errno = 0;
    *transfer_buffer = 0;
    for (Client = ClientList; Client; Client = Client->next) {
	double bytes;
	unsigned long flags = Client->flags & DCC_TYPES;

	if ((flags == DCC_RAW) || (flags == DCC_RAW_LISTEN) || (flags == DCC_CHAT))
	    continue;
	if ((Client->flags & DCC_WAIT) || (Client->flags == DCC_DELETE))
	    continue;
	bytes = Client->bytes_read + Client->bytes_sent + Client->transfer_orders.byteoffset;
	if (bytes >= 0) {
	    if (Client->filesize >= bytes) {
		perc = (100.0 * ((double) bytes) / (double) (Client->filesize));
		if (perc > 100.0)
		    perc = 100.0;
		else if (perc < 0.0)
		    perc = 0.0;

	    }
	    sprintf(temp_str, "%d%%,", (int) perc);
	    strcat(transfer_buffer, temp_str);
	}
	if (count++ > 9)
	    break;
    }
    if (count) {
	chop(transfer_buffer, 1);
	if (get_fset_var(FORMAT_DCC_FSET)) {
	    sprintf(DCC_current_transfer_buffer, "%s", convert_output_format(get_fset_var(FORMAT_DCC_FSET), "%s", transfer_buffer));
	    chop(DCC_current_transfer_buffer, 4);
	} else
	    sprintf(DCC_current_transfer_buffer, "[%s]", transfer_buffer);
    } else
	*DCC_current_transfer_buffer = 0;
}

static int get_to_from(const char *type)
{
    if (!my_stricmp(type, "SEND"))
	return 0;
    else if (!my_stricmp(type, "RESEND"))
	return 1;
    else if (!my_stricmp(type, "GET"))
	return 2;
    else if (!my_stricmp(type, "REGET"))
	return 3;
    else
	return -1;
}

static void dcc_help1(const char *command, char *args)
{
    put_it("%s", convert_output_format("$G %RDCC%n help -", NULL, NULL));
    put_it("%s", convert_output_format("   Active    Stats    List     GList   ?", NULL, NULL));
    put_it("%s", convert_output_format("   Resend/Reget    Send/Get    Chat    Raw   Close  Rename", NULL, NULL));
    put_it("%s", convert_output_format("   Auto   Quiet   Paths   Quiet   Overwrite  Auto_Rename", NULL, NULL));
}

static void dcc_show_active(const char *command, char *args)
{
    put_it("%s", convert_output_format("$G %RDCC%n  DCC Active = \002$0\002", "%d", dcc_active_count));
}

static void dcc_set_quiet(const char *command, char *args)
{
    dcc_quiet ^= 1;
    put_it("%s", convert_output_format("$G %RDCC%n  DCC Quiet = \002$0\002", "%s", on_off(dcc_quiet)));
}

/* returns the string without the path (if present) */
static char *strip_path(char *str)
{
    char *ptr;

    ptr = strrchr(str, '/');
    if (ptr == NULL)
	return str;
    else
	return ptr + 1;
}

/* returns the string without the path (if present) */
static char *check_paths(char *str)
{
    if (dcc_paths == 0)
	return strip_path(str);
    else
	return str;
}

static void dcc_set_paths(const char *command, char *args)
{
    dcc_paths ^= 1;

    put_it("%s", convert_output_format("$G %RDCC%n  DCC paths is now \002$0\002", "%s", on_off(dcc_paths)));
}

static void dcc_tog_rename(const char *command, char *args)
{
    int arename = get_int_var(DCC_AUTORENAME_VAR);

    arename ^= 1;
    set_int_var(DCC_AUTORENAME_VAR, arename);
    put_it("%s",
	   convert_output_format("$G %RDCC%n  DCC auto rename is now \002$0\002", "%s", on_off(get_int_var(DCC_AUTORENAME_VAR))));
}

static void dcc_overwrite_toggle(const char *command, char *args)
{
    dcc_overwrite_var ^= 1;

    put_it("%s", convert_output_format("  DCC overwrite is now \002$0\002", "%s", on_off(dcc_overwrite_var)));
}

unsigned char byteordertest(void)
{
    unsigned short test = DCC_PACKETID;

    if (*((unsigned char *) &test) == ((DCC_PACKETID & 0xff00) >> 8))
	return 0;

    if (*((unsigned char *) &test) == (DCC_PACKETID & 0x00ff))
	return 1;
    return 0;
}

void dcc_check_idle(void)
{
    register DCC_list *Client = NULL, *tmpClient;

    time_t dcc_idle_time = 100;
    static char *last_notify = NULL;

    if (dcc_idle_time) {
	int this_idle_time = dcc_idle_time;
	int erase_it;

	for (Client = ClientList; Client;) {
	    time_t client_idle = time(NULL) - Client->lasttime.tv_sec;

	    if (client_idle <= 0)
		client_idle = 1;
	    erase_it = 0;
	    tmpClient = Client->next;
	    switch (Client->flags & DCC_TYPES) {
	    case DCC_FILEOFFER:
	    case DCC_FILEREAD:
	    case DCC_RESENDOFFER:
	    case DCC_REGETFILE:
		this_idle_time = dcc_idle_time * 3;
		break;
	    default:
		this_idle_time = dcc_idle_time;
		break;
	    }
	    if ((client_idle > this_idle_time) && !(Client->flags & DCC_ACTIVE)) {
		put_it("%s",
		       convert_output_format("$G %RDCC%n Auto-closing idle dcc $0 to $1", "%s %s",
					     dcc_types[Client->flags & DCC_TYPES], Client->user));
		if (!last_notify || !match(Client->user, last_notify)) {
		    send_to_server(SERVER(from_server), "NOTICE %s :Dcc %s Auto Closed", Client->user,
				   dcc_types[Client->flags & DCC_TYPES]);
		    malloc_strcpy(&last_notify, Client->user);
		}
		if ((get_to_from(dcc_types[Client->flags & DCC_TYPES]) != -1))
		    if (dcc_active_count)
			dcc_active_count--;
		erase_it = 1;
	    }
	    if (erase_it)
		dcc_erase(Client);
	    Client = tmpClient;
	}
    }
}

int check_dcc_list(char *name)
{
    register DCC_list *Client;
    int do_it = 0;

    for (Client = ClientList; Client; Client = Client->next) {
	if (Client->user && !my_stricmp(name, Client->user) && !(Client->flags & DCC_ACTIVE)) {
	    Client->flags |= DCC_DELETE;
	    do_it++;
	    if (get_to_from(dcc_types[Client->flags & DCC_TYPES]) != -1 && dcc_active_count)
		dcc_active_count--;
	}
    }
    return do_it;
}
