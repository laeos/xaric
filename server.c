/*
 * server.c: Things dealing with server connections, etc. 
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
#include "parse.h"
#include "build.h"

#include <stdarg.h>

#include "server.h"
#include "ircaux.h"
#include "input.h"
#include "whois.h"
#include "lastlog.h"
#include "exec.h"
#include "window.h"
#include "output.h"
#include "names.h"
#include "hook.h"
#include "struct.h"
#include "vars.h"
#include "screen.h"
#include "notify.h"
#include "misc.h"
#include "status.h"
#include "fset.h"

/* for debug */
#define MODULE_ID XD_COMM

static char *set_umode(int du_index);
static void login_to_server(int, int);

const char *umodes = "abcdefghijklmnopqrstuvwxyz";

/* server_list: the list of servers that the user can connect to,etc */
struct server *server_list = NULL;

/* number_of_servers: in the server list */
int number_of_servers = 0;

extern WhoisQueue *WQ_head;
extern WhoisQueue *WQ_tail;

int primary_server = -1;
int from_server = -1;
int never_connected = 1;	/* true until first connection is made */
int connected_to_server = 0;	/* true when connection is confirmed */
char *connect_next_nick;
int parsing_server_index = -1;
int last_server = -1;

extern int dgets_errno;
extern int doing_who;
extern int in_on_who;

/*
 * close_server: Given an index into the server list, this closes the
 * connection to the corresponding server.  It does no checking on the
 * validity of the index.  It also first sends a "QUIT" to the server being
 * closed 
 */
void close_server(int cs_index, char *message)
{
    int i, min, max;

    if (cs_index == -1) {
	min = 0;
	max = number_of_servers;
    } else {
	min = cs_index;
	max = cs_index + 1;
    }
    for (i = min; i < max; i++) {
	if (i == primary_server) {
	    int old_server = from_server;

	    /* make sure no leftover whois's are out there */
	    from_server = i;
	    clean_whois_queue();
	    from_server = old_server;
	    if (waiting_out > waiting_in)
		waiting_out = waiting_in = 0;
	    set_waiting_channel(i);
	} else {
	    int old_server = from_server;

	    from_server = -1;
	    clear_channel_list(i);
	    from_server = old_server;
	}
	if (split_watch) {
	    clear_link(&server_last);
	    clear_link(&tmplink);
	}
	server_list[i].operator = 0;
	server_list[i].connected = 0;
	server_list[i].pos = 0;
	server_list[i].close_serv = -1;
	new_free(&server_list[i].away);

	if (server_list[i].sock) {
	    if (message && *message) {
		sa_writef(server_list[i].sock, "QUIT :%s\n", message);
	    }
	    sa_shutdown(server_list[i].sock, "rw");
	    sa_destroy(server_list[i].sock);
	    server_list[i].sock = NULL;
	}
    }
}

/*
 * set_server_bits: Sets the proper bits in the fd_set structure according to
 * which servers in the server list have currently active read descriptors.  
 */
void set_server_bits(fd_set * rd, fd_set * wr)
{
    int i;

    for (i = 0; i < number_of_servers; i++) {
	if (server_list[i].sock) {
	    int fd;

	    sa_getfd(server_list[i].sock, &fd);
	    FD_SET(fd, rd);
	    if (!(server_list[i].flags & (LOGGED_IN | CLOSE_PENDING)))
		FD_SET(fd, wr);
	}
    }
}

int in_timed_server = 0;

int timed_server(void *args)
{
    char *p = (char *) args;
    static int retry = 0;

    if (!connected_to_server) {
	get_connected(atol(p));
	bitchsay("Servers exhausted. Restarting. [%d]", ++retry);
    }
    new_free(&p);
    in_timed_server = 0;
    return 0;
}

static int is_connected_p(int i)
{
    if (server_list[i].sock) {
	int des;
	struct sockaddr_in sa;
	size_t sa_len = sizeof(struct sockaddr_in);

	sa_getfd(server_list[i].sock, &des);
	if (getpeername(des, (struct sockaddr *) &sa, &sa_len) != -1)
	    return 1;
    }
    return 0;
}

static int read_data(int i, time_t * last_timeout)
{
    static int times = 0;
    char *buf = server_list[i].line;
    int pos = server_list[i].pos;
    size_t done = 0;
    sa_rc_t ret = sa_readln(server_list[i].sock, buf + pos, SERVER_BUF_LEN - pos, &done);

    last_server = from_server = i;
    switch (ret) {
    case SA_OK:
	*last_timeout = 0;
	parsing_server_index = i;
	server_list[i].last_msg = time(NULL);
	parse_server(buf);
	server_list[i].pos = 0;
	parsing_server_index = -1;
	message_from(NULL, LOG_CRAP);
	return 1;

    case SA_ERR_TMT:
	server_list[from_server].pos = pos;
	return 0;

    default:
	{
	    int old_serv = server_list[i].close_serv;

	    close_server(i, empty_str);
	    *last_timeout = 0;
	    say("Connection closed from %s: %s", server_list[i].name, sa_error(ret));
	    server_list[i].connected = 0;
	    sa_destroy(server_list[i].sock);
	    server_list[i].sock = NULL;

	    if (!(server_list[i].flags & LOGGED_IN)) {
		if (old_serv == i)	/* a hack? you bet */
		    goto a_hack;
		if (old_serv != -1 && (server_list[old_serv].flags & CLOSE_PENDING)) {
		    say("Connection to server %s resumed...", server_list[old_serv].name);
		    server_list[i].close_serv = -1;
		    server_list[old_serv].flags &= ~CLOSE_PENDING;
		    server_list[old_serv].flags |= LOGGED_IN;
		    server_list[old_serv].connected = 1;
		    get_connected(old_serv);
		    return 0;
		}
	    }
	  a_hack:

	    if (i == primary_server) {
		if (server_list[i].eof) {
		    put_it("%s",
			   convert_output_format(get_fset_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME),
						 "Unable to connect to a server"));
		    if (++i >= number_of_servers) {
			clean_whois_queue();
			times = 0;
			i = 0;
		    }

		    get_connected(i);
		    return 0;
		} else {
		    if (times++ > 3) {
			clean_whois_queue();
			if (do_hook(DISCONNECT_LIST, "No Connection"))
			    put_it("%s",
				   convert_output_format(get_fset_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME),
							 "No connection"));
			times = 0;
		    }
		    get_connected(i);
		    return 0;
		}
	    } else if (server_list[i].eof) {
		say("Connection to server %s lost.", server_list[i].name);
		close_server(i, empty_str);
		clean_whois_queue();
		window_check_servers();
	    } else if (connect_to_server_by_refnum(i, -1)) {
		say("Connection to server %s lost.", server_list[i].name);
		close_server(i, empty_str);
		clean_whois_queue();
		window_check_servers();
	    }
	    server_list[i].eof = 1;
	    return 0;
	}
    }
    from_server = primary_server;
    return 0;
}

/*
 * do_server: check the given fd_set against the currently open servers in
 * the server list.  If one have information available to be read, it is read
 * and and parsed appropriately.  If an EOF is detected from an open server,
 * one of two things occurs. 1) If the server was the primary server,
 * get_connected() is called to maintain the connection status of the user.
 * 2) If the server wasn't a primary server, connect_to_server() is called to
 * try to keep that connection alive. 
 */
void do_server(fd_set * rd, fd_set * wr)
{
    int des, i;
    static time_t last_timeout = 0;

    for (i = 0; i < number_of_servers; i++) {
	if (server_list[i].sock == NULL)
	    continue;
	sa_getfd(server_list[i].sock, &des);
	if (FD_ISSET(des, wr) && !(server_list[i].flags & LOGGED_IN)) {
	    if (is_connected_p(i))
		login_to_server(i, -1);
	}
	if (FD_ISSET(des, rd)) {
	    while (read_data(i, &last_timeout)) ;
	}
	if (errno == ENETUNREACH || errno == EHOSTUNREACH) {
	    if (last_timeout == 0)
		last_timeout = time(NULL);
	    else if (time(NULL) - last_timeout > 600) {
		close_server(i, empty_str);
		get_connected(i);
	    }
	}

    }
}

/*
 * find_in_server_list: given a server name, this tries to match it against
 * names in the server list, returning the index into the list if found, or
 * -1 if not found 
 */
extern int find_in_server_list(char *server, int port)
{
    int i, len, len2;

    len = strlen(server);
    for (i = 0; i < number_of_servers; i++) {
	if (port && server_list[i].port && port != server_list[i].port && port != -1)
	    continue;

	len2 = strlen(server_list[i].name);
	if (len2 > len) {
	    if (!my_strnicmp(server, server_list[i].name, len))
		return (i);
	} else {
	    if (!my_strnicmp(server, server_list[i].name, len2))
		return (i);
	}

	if (server_list[i].itsname) {
	    len2 = strlen(server_list[i].itsname);
	    if (len2 > len) {
		if (!my_strnicmp(server, server_list[i].itsname, len))
		    return (i);
	    } else {
		if (!my_strnicmp(server, server_list[i].name, len2))
		    return (i);
	    }
	}
    }
    return (-1);
}

/*
 * parse_server_index:  given a string, this checks if it's a number, and if
 * so checks it validity as a server index.  Otherwise -1 is returned 
 */
int parse_server_index(char *str)
{
    int i;

    if (is_number(str)) {
	i = my_atol(str);
	if ((i >= 0) && (i < number_of_servers))
	    return (i);
    }
    return (-1);
}

/*
 * This replaces ``get_server_index''.
 */
int find_server_refnum(char *server, char **rest)
{
    int refnum;
    int port = irc_port;
    char *cport = NULL, *password = NULL, *nick = NULL;

    /* 
     * First of all, check for an existing server refnum
     */
    if ((refnum = parse_server_index(server)) != -1)
	return refnum;

    /* 
     * Next check to see if its a "server:port:password:nick"
     */
    else if (index(server, ':'))
	parse_server_info(server, &cport, &password, &nick);

    /* 
     * Next check to see if its "server port password nick"
     */
    else if (rest && *rest) {
	cport = next_arg(*rest, rest);
	password = next_arg(*rest, rest);
	nick = next_arg(*rest, rest);
    }

    if (cport && *cport)
	port = my_atol(cport);

    /* 
     * Add to the server list (this will update the port
     * and password fields).
     */
    add_to_server_list(server, port, password, nick, 1);
    return from_server;
}

/*
 * add_to_server_list: adds the given server to the server_list.  If the
 * server is already in the server list it is not re-added... however, if the
 * overwrite flag is true, the port and passwords are updated to the values
 * passes.  If the server is not on the list, it is added to the end. In
 * either case, the server is made the current server. 
 */
void add_to_server_list(char *server, int port, char *password, char *nick, int overwrite)
{
    if ((from_server = find_in_server_list(server, port)) == -1) {
	from_server = number_of_servers++;
	RESIZE(server_list, struct server, number_of_servers + 1);

	server_list[from_server].name = m_strdup(server);
	server_list[from_server].sock = NULL;
	server_list[from_server].lag = -1;
	server_list[from_server].motd = 1;
	server_list[from_server].port = port;
	server_list[from_server].pos = 0;

	if (password && *password)
	    malloc_strcpy(&(server_list[from_server].password), password);

	if (nick && *nick)
	    malloc_strcpy(&(server_list[from_server].d_nickname), nick);
	else if (!server_list[from_server].d_nickname)
	    malloc_strcpy(&(server_list[from_server].d_nickname), nickname);

	make_notify_list(from_server);
	set_umode(from_server);
    } else {
	if (overwrite) {
	    server_list[from_server].port = port;
	    if (password || !server_list[from_server].password) {
		if (password && *password)
		    malloc_strcpy(&(server_list[from_server].password), password);
		else
		    new_free(&(server_list[from_server].password));
	    }
	    if (nick || !server_list[from_server].d_nickname) {
		if (nick && *nick)
		    malloc_strcpy(&(server_list[from_server].d_nickname), nick);
		else
		    new_free(&(server_list[from_server].d_nickname));
	    }
	}
	if (strlen(server) > strlen(server_list[from_server].name))
	    malloc_strcpy(&(server_list[from_server].name), server);
    }
}

void remove_from_server_list(int i)
{
    int old_server = from_server;
    Window *tmp;

    if (i < 0 || i >= number_of_servers)
	return;

    say("Deleting server [%d]", i);

    from_server = i;
    clean_whois_queue();
    from_server = old_server;

    new_free(&server_list[i].name);
    new_free(&server_list[i].itsname);
    new_free(&server_list[i].password);
    new_free(&server_list[i].away);
    new_free(&server_list[i].version_string);
    new_free(&server_list[i].nickname);
    new_free(&server_list[i].s_nickname);
    new_free(&server_list[i].d_nickname);

    new_free(&server_list[i].whois_stuff.nick);
    new_free(&server_list[i].whois_stuff.user);
    new_free(&server_list[i].whois_stuff.host);
    new_free(&server_list[i].whois_stuff.channel);
    new_free(&server_list[i].whois_stuff.channels);
    new_free(&server_list[i].whois_stuff.name);
    new_free(&server_list[i].whois_stuff.server);
    new_free(&server_list[i].whois_stuff.server_stuff);

    /* 
     * this should save a coredump.  If number_of_servers drops
     * down to zero, then trying to do a realloc ends up being
     * a free, and accessing that is a no-no.
     */
    if (number_of_servers == 1) {
	say("Sorry, the server list is empty and I just dont know what to do.");
	irc_exit("Sorry, the server list is empty and I just dont know what to do.", NULL);
    }

    memmove(&server_list[i], &server_list[i + 1], (number_of_servers - i) * sizeof(struct server));
    number_of_servers--;
    RESIZE(server_list, struct server, number_of_servers);

    /* update all he structs with server in them */
    channel_server_delete(i);
    exec_server_delete(i);
    if (i < primary_server)
	--primary_server;
    if (i < from_server)
	--from_server;
    tmp = NULL;
    while (traverse_all_windows(&tmp))
	if (tmp->server > i)
	    tmp->server--;
}

/*
 * parse_server_info:  This parses a single string of the form
 * "server:portnum:password:nickname".  It the points port to the portnum
 * portion and password to the password portion.  This chews up the original
 * string, so * upon return, name will only point the the name.  If portnum
 * or password are missing or empty,  their respective returned value will
 * point to null. 
 */
void parse_server_info(char *name, char **port, char **password, char **nick)
{
    char *ptr;

    *port = *password = *nick = NULL;
    if ((ptr = (char *) index(name, ':')) != NULL) {
	*(ptr++) = (char) 0;
	if (strlen(ptr) == 0)
	    *port = NULL;
	else {
	    *port = ptr;
	    if ((ptr = (char *) index(ptr, ':')) != NULL) {
		*(ptr++) = (char) 0;
		if (strlen(ptr) == 0)
		    *password = '\0';
		else {
		    *password = ptr;
		    if ((ptr = (char *) index(ptr, ':'))
			!= NULL) {
			*(ptr++) = '\0';
			if (!strlen(ptr))
			    *nick = NULL;
			else
			    *nick = ptr;
		    }
		}
	    }
	}
    }
}

/*
 * build_server_list: given a whitespace separated list of server names this
 * builds a list of those servers using add_to_server_list().  Since
 * add_to_server_list() is used to added each server specification, this can
 * be called many many times to add more servers to the server list.  Each
 * element in the server list case have one of the following forms: 
 *
 * servername 
 *
 * servername:port 
 *
 * servername:port:password 
 *
 * servername::password 
 *
 * Note also that this routine mucks around with the server string passed to it,
 * so make sure this is ok 
 */
void build_server_list(char *servers)
{
    char *host, *rest, *password = NULL, *port = NULL, *nick = NULL;
    int port_num;

    if (servers == NULL)
	return;

    while (servers) {
	if ((rest = (char *) index(servers, '\n')) != NULL)
	    *rest++ = '\0';
	while ((host = next_arg(servers, &servers)) != NULL) {
	    parse_server_info(host, &port, &password, &nick);
	    if (port && *port) {
		port_num = my_atol(port);
		if (!port_num)
		    port_num = irc_port;
	    } else
		port_num = irc_port;

	    add_to_server_list(host, port_num, password, nick, 0);
	}
	servers = rest;
    }
}

#if 0
/*
 * connect_to_server_direct: handles the tcp connection to a server.  If
 * successful, the user is disconnected from any previously connected server,
 * the new server is added to the server list, and the user is registered on
 * the new server.  If connection to the server is not successful,  the
 * reason for failure is displayed and the previous server connection is
 * resumed uniterrupted. 
 *
 * This version of connect_to_server() connects directly to a server 
 */
static int connect_to_server_direct(char *server_name, int port)
{
    int new_des;
    struct sockaddr_in localaddr;
    size_t address_len;
    unsigned short this_sucks;

    oper_command = 0;
    errno = 0;
    memset(&localaddr, 0, sizeof(localaddr));
    this_sucks = (unsigned short) port;

    new_des = connect_by_number(server_name, &this_sucks, SERVICE_CLIENT, PROTOCOL_TCP, 0);
    port = this_sucks;
    XDEBUG(19, "Descriptor for [%s:%d]: %d", server_name, port, new_des);

    if (new_des < 0) {
	say("Unable to connect to port %d of server %s: %s", port, server_name, errno ? strerror(errno) : "unknown host");
	if ((from_server != -1) && (server_list[from_server].read != -1))
	    say("Connection to server %s resumed...", server_list[from_server].name);
	return (-1);
    }

    if (*server_name != '/') {
	address_len = sizeof(struct sockaddr_in);
	getsockname(new_des, (struct sockaddr *) &localaddr, &address_len);
    }

    update_all_status(curr_scr_win, NULL, 0);
    add_to_server_list(server_name, port, NULL, NULL, 1);
    if (port) {
	server_list[from_server].read = new_des;
	server_list[from_server].write = new_des;
    } else
	server_list[from_server].read = new_des;

    server_list[from_server].local_addr.s_addr = localaddr.sin_addr.s_addr;
    server_list[from_server].operator = 0;
    return (0);
}
#endif

static void login_to_server(int refnum, int c_server)
{
    sa_buffer(server_list[refnum].sock, SA_BUFFER_READ, 1024);
    sa_timeout(server_list[refnum].sock, SA_TIMEOUT_READ, 0, 400);

    if ((c_server != -1) && (c_server != refnum)) {
	server_list[c_server].flags &= ~CLOSE_PENDING;
	close_server(c_server, "changing servers");
    }

    if (!server_list[refnum].d_nickname)
	malloc_strcpy(&(server_list[refnum].d_nickname), nickname);

    if (server_list[refnum].password)
	send_to_server("PASS %s", server_list[refnum].password);
    server_list[refnum].flags |= LOGGED_IN;
    register_server(refnum, server_list[refnum].d_nickname);

    server_list[refnum].last_msg = time(NULL);
    in_on_who = 0;
    doing_who = 0;
    server_list[refnum].connected = 1;
    *server_list[refnum].umode = 0;
    server_list[refnum].operator = 0;
    set_umode(refnum);
}

int connect_to_server_by_refnum(int refnum, int c_server)
{
    char *sname;
    int sport;

    if (refnum == -1) {
	say("Connecting to refnum -1.  That makes no sense.");
	return -1;
    }

    sname = server_list[refnum].name;
    if ((sport = server_list[refnum].port) == -1)
	sport = irc_port;

    if (server_list[refnum].sock == NULL) {
	char buffer[BIG_BUFFER_SIZE];
	sa_rc_t ret;

	snprintf(buffer, BIG_BUFFER_SIZE, "inet://%s:%d", sname, sport);
	buffer[BIG_BUFFER_SIZE - 1] = '\0';

	from_server = refnum;
	say("Connecting to port %d of server %s", sport, sname);

	if (!server_list[refnum].rem_addr)
	    sa_addr_create(&server_list[refnum].rem_addr);

	if ((ret = sa_addr_u2a(server_list[refnum].rem_addr, buffer)) != SA_OK) {
	    say("Couldn't figure out address %s: %s", buffer, sa_error(ret));
	    return -1;
	}
	sa_create(&server_list[refnum].sock);
	if ((ret = sa_connect(server_list[refnum].sock, server_list[refnum].rem_addr)) != SA_OK) {
	    say("Couldn't connect to %s: %s", buffer, sa_error(ret));
	    sa_destroy(server_list[refnum].sock);
	    server_list[refnum].sock = NULL;
	    return -1;
	}
	server_list[refnum].flags &= ~LOGGED_IN;
	if (is_connected_p(refnum))
	    login_to_server(refnum, c_server);
    } else {
	say("Connected to port %d of server %s", sport, sname);
	from_server = refnum;
    }
    message_from(NULL, LOG_CRAP);
    update_all_status(curr_scr_win, NULL, 0);
    return 0;
}

/*
 * get_connected: This function connects the primary server for IRCII.  It
 * attempts to connect to the given server.  If this isn't possible, it
 * traverses the server list trying to keep the user connected at all cost.  
 */
void get_connected(int server)
{
    int s, ret = -1;

    if (server_list) {
	if (server == number_of_servers)
	    server = 0;
	else if (server < 0)
	    server = number_of_servers - 1;

	s = server;
	if (connect_to_server_by_refnum(server, primary_server)) {
	    while (server_list[server].sock == NULL) {
		server++;
		if (server == number_of_servers)
		    server = 0;
		if (server == s) {
		    clean_whois_queue();
		    break;
		}
		from_server = server;
		ret = connect_to_server_by_refnum(server, primary_server);
	    }
	    if (!ret)
		from_server = server;
	    else
		from_server = -1;
	}
	change_server_channels(primary_server, from_server);
	set_window_server(-1, from_server, 1);
    } else {
	clean_whois_queue();
	if (do_hook(DISCONNECT_LIST, "No Server List"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME),
					 "You are not connected to a server. Use /SERVER to connect."));
    }
}

/*
 * read_server_file: reads hostname:portnum:password server information from
 * a file and adds this stuff to the server list.  See build_server_list()/ 
 */
int read_server_file(char *servers_file)
{
    FILE *fp;
    int some = 0;
    char *file_path = NULL;
    char buffer[BIG_BUFFER_SIZE + 1];
    int old_window_display = window_display;

    window_display = 0;
#ifdef SERVERS_FILE
    malloc_sprintf(&file_path, "%s/%s", XARIC_DATA_PATH, SERVERS_FILE);
    if ((fp = fopen(file_path, "r"))) {
	while (fgets(buffer, BIG_BUFFER_SIZE, fp)) {
	    chop(buffer, 1);
	    build_server_list(buffer);
	}
	fclose(fp);
    } else
	some = 1;
    new_free(&file_path);
#endif
    malloc_sprintf(&file_path, "~/%s", servers_file);
    if ((fp = uzfopen(&file_path, "."))) {
	while (fgets(buffer, BIG_BUFFER_SIZE, fp)) {
	    chop(buffer, 1);
	    build_server_list(buffer);
	}
	fclose(fp);
	some = 0;
    }
    new_free(&file_path);
    window_display = old_window_display;
    return (some ? 0 : 1);
}

/* display_server_list: just guess what this does */
void display_server_list(void)
{
    int i;

    if (server_list) {
	if (from_server != -1)
	    say("Current server: %s %d", server_list[from_server].name, server_list[from_server].port);
	else
	    say("Current server: <None>");

	if (primary_server != -1)
	    say("Primary server: %s %d", server_list[primary_server].name, server_list[primary_server].port);
	else
	    say("Primary server: <None>");

	say("Server list:");
	for (i = 0; i < number_of_servers; i++) {
	    if (!server_list[i].nickname) {
		if (server_list[i].sock == NULL)
		    say("\t%d) %s %d", i, server_list[i].name, server_list[i].port);
		else
		    say("\t%d) %s %d", i, server_list[i].name, server_list[i].port);
	    } else {
		if (server_list[i].sock == NULL)
		    say("\t%d) %s %d (was %s)", i, server_list[i].name, server_list[i].port, server_list[i].nickname);
		else
		    say("\t%d) %s %d (%s)", i, server_list[i].name, server_list[i].port, server_list[i].nickname);
	    }
	}
    } else
	say("The server list is empty");
}

#if 0
void MarkAllAway(char *command, char *message, char *subargs)
{
    int old_server;

    old_server = from_server;
    for (from_server = 0; from_server < number_of_servers; from_server++) {
	if (server_list[from_server].connected)
	    send_to_server("%s :%s", command, message);
    }
    from_server = old_server;
}
#endif

/*
 * set_server_password: this sets the password for the server with the given
 * index.  If password is null, the password for the given server is returned 
 */
char *set_server_password(int ssp_index, char *password)
{

    if (server_list) {
	if (password)
	    malloc_strcpy(&(server_list[ssp_index].password), password);
	return (server_list[ssp_index].password);
    } else
	return (NULL);
}

/* server_list_size: returns the number of servers in the server list */
int server_list_size(void)
{
    return (number_of_servers);
}

/*
 * flush_server: eats all output from server, until there is at least a
 * second delay between bits of servers crap... useful to abort a /links. 
 */
void flush_server(void)
{
    fd_set rd;
    struct timeval timeout;
    int flushing = 1;
    int des;
    int old_timeout;
    char buffer[BIG_BUFFER_SIZE + 1];

    if (server_list[from_server].sock == NULL)
	return;

    sa_getfd(server_list[from_server].sock, &des);
    timeout.tv_usec = 0;
    timeout.tv_sec = 1;
    old_timeout = dgets_timeout(1);
    while (flushing) {
	FD_ZERO(&rd);
	FD_SET(des, &rd);
	switch (new_select(&rd, NULL, &timeout)) {
	case -1:
	case 0:
	    flushing = 0;
	    break;
	default:
	    if (FD_ISSET(des, &rd)) {
		if (!dgets(buffer, BIG_BUFFER_SIZE, des, NULL))
		    flushing = 0;
	    }
	    break;
	}
    }
    /* make sure we've read a full line from server */
    FD_ZERO(&rd);
    FD_SET(des, &rd);
    if (new_select(&rd, NULL, &timeout) > 0)
	dgets(buffer, BIG_BUFFER_SIZE, des, NULL);
    (void) dgets_timeout(old_timeout);
}

static char *set_umode(int du_index)
{
    char *c = server_list[du_index].umode;
    int flags = server_list[du_index].flags;
    int i;

    for (i = 0; umodes[i]; i++) {
	if (umodes[i] == 'o')
	    continue;
	if (flags & USER_MODE << i)
	    *c++ = umodes[i];
    }
    if (get_server_operator(du_index))
	*c++ = 'O';
    *c = 0;
    return server_list[du_index].umode;
}

char *get_umode(int gu_index)
{
    if (gu_index == -1)
	gu_index = primary_server;
    else if (gu_index >= number_of_servers)
	return empty_str;

    return server_list[gu_index].umode;
}

/*
 * Encapsulates everything we need to change our AWAY status.
 * This improves greatly on having everyone peek into that member.
 * Also, we can deal centrally with someone changing their AWAY
 * message for a server when we're not connected to that server
 * (when we do connect, then we send out the AWAY command.)
 * All this saves a lot of headaches and crashes.
 */
void set_server_away(int ssa_index, char *message)
{
    int old_from_server = from_server;

    from_server = ssa_index;
    if (ssa_index == -1)
	say("You are not connected to a server.");

    else if (message && *message) {
	if (!server_list[ssa_index].away)
	    away_set++;
	if (server_list[ssa_index].away != message)
	    malloc_strcpy(&server_list[ssa_index].away, message);

	if (!server_list[ssa_index].awaytime)
	    server_list[ssa_index].awaytime = time(NULL);

	if (get_int_var(MSGLOG_VAR))
	    log_toggle(1, NULL);
	if (!server_list[ssa_index].connected) {
	    from_server = old_from_server;
	    return;
	}
	if (get_fset_var(FORMAT_AWAY_FSET) && get_int_var(SEND_AWAY_MSG_VAR)) {
	    struct channel *chan;

	    for (chan = server_list[ssa_index].chan_list; chan; chan = chan->next) {
		send_to_server("PRIVMSG %s :ACTION %s", chan->channel,
			       stripansicodes(convert_output_format
					      (get_fset_var(FORMAT_AWAY_FSET), "%s %s", update_clock(GET_TIME), message)));
	    }
	}
	if (get_fset_var(FORMAT_AWAY_FSET)) {
	    char buffer[BIG_BUFFER_SIZE + 1];

	    send_to_server("%s :%s", "AWAY",
			   stripansicodes(convert_output_format
					  (get_fset_var(FORMAT_AWAY_FSET), "%s %s", update_clock(GET_TIME), message)));
	    strncpy(buffer, convert_output_format("%Kð %W$N%n is away: ", NULL, NULL), BIG_BUFFER_SIZE);
	    strncat(buffer,
		    convert_output_format(get_fset_var(FORMAT_AWAY_FSET), "%s %s", update_clock(GET_TIME), message), BIG_BUFFER_SIZE);
	    put_it("%s", buffer);
	} else
	    send_to_server("%s :%s", "AWAY", stripansicodes(convert_output_format(message, NULL)));
    } else {
	if (server_list[ssa_index].away)
	    away_set--;
	server_list[ssa_index].awaytime = 0;
	new_free(&server_list[ssa_index].away);
	if (server_list[ssa_index].connected)
	    send_to_server("AWAY :");
    }
    from_server = old_from_server;
}

char *get_server_away(int gsa_index)
{
    if (gsa_index == -1)
	return NULL;
    return server_list[gsa_index].away;
}

void set_server_flag(int ssf_index, int flag, int value)
{
    if (ssf_index == -1)
	ssf_index = primary_server;
    else if (ssf_index >= number_of_servers)
	return;

    if (value)
	server_list[ssf_index].flags |= flag;
    else
	server_list[ssf_index].flags &= ~flag;

    set_umode(ssf_index);
}

int get_server_flag(int gsf_index, int value)
{
    if (gsf_index == -1)
	gsf_index = primary_server;
    else if (gsf_index >= number_of_servers)
	return 0;

    return server_list[gsf_index].flags & value;
}

/*
 * set_server_version: Sets the server version for the given server type.  A
 * zero version means pre 2.6, a one version means 2.6 aso. (look server.h
 * for typedef)
 */
void set_server_version(int ssv_index, int version)
{
    if (ssv_index == -1)
	ssv_index = primary_server;
    else if (ssv_index >= number_of_servers)
	return;
    server_list[ssv_index].version = version;
}

/*
 * get_server_version: returns the server version value for the given server
 * index 
 */
int get_server_version(int gsv_index)
{
    if (gsv_index == -1)
	gsv_index = primary_server;
    else if (gsv_index >= number_of_servers)
	return 0;
    if (gsv_index == -1)
	return 0;
    return (server_list[gsv_index].version);
}

/* get_server_name: returns the name for the given server index */
char *get_server_name(int gsn_index)
{
    if (gsn_index == -1)
	gsn_index = primary_server;
    if (gsn_index == -1 || gsn_index >= number_of_servers)
	return empty_str;

    return (server_list[gsn_index].name);
}

/* get_server_itsname: returns the server's idea of its name */
char *get_server_itsname(int gsi_index)
{
    if (gsi_index == -1)
	gsi_index = primary_server;
    else if (gsi_index >= number_of_servers)
	return empty_str;

    /* better check gsi_index for -1 here CDE */
    if (gsi_index == -1)
	return empty_str;

    if (server_list[gsi_index].itsname)
	return server_list[gsi_index].itsname;
    else
	return server_list[gsi_index].name;
}

void set_server_itsname(int ssi_index, char *name)
{
    if (ssi_index == -1)
	ssi_index = primary_server;
    else if (ssi_index >= number_of_servers)
	return;

    malloc_strcpy(&server_list[ssi_index].itsname, name);
}

/*
 * is_server_open: Returns true if the given server index represents a server
 * with a live connection, returns false otherwise 
 */
int is_server_open(int iso_index)
{
    if (iso_index < 0 || iso_index >= number_of_servers)
	return (0);
    return !!server_list[iso_index].sock;
}

/*
 * is_server_connected: returns true if the given server is connected.  This
 * means that both the tcp connection is open and the user is properly
 * registered 
 */
int is_server_connected(int isc_index)
{
    if (isc_index >= 0 && isc_index < number_of_servers)
	return (server_list[isc_index].connected);
    return 0;
}

/* get_server_port: Returns the connection port for the given server index */
int get_server_port(int gsp_index)
{
    if (gsp_index == -1)
	gsp_index = primary_server;
    else if (gsp_index >= number_of_servers)
	return 0;

    return (server_list[gsp_index].port);
}

/*
 * get_server_nickname: returns the current nickname for the given server
 * index 
 */
char *get_server_nickname(int gsn_index)
{
    if (gsn_index >= number_of_servers)
	return empty_str;
    else if (gsn_index != -1 && server_list[gsn_index].nickname)
	return (server_list[gsn_index].nickname);
    else
	return "<Not-registered-yet>";
}

/* get_server_qhead - get the head of the whois queue */
WhoisQueue *get_server_qhead(int gsq_index)
{
    if (gsq_index == -1)
	return WQ_head;
    else if (gsq_index >= number_of_servers)
	return NULL;

    return server_list[gsq_index].WQ_head;
}

/* get_server_whois_stuff */
WhoisStuff *get_server_whois_stuff(int gsw_index)
{
    if (gsw_index == -1)
	gsw_index = primary_server;
    else if (gsw_index >= number_of_servers)
	return NULL;

    return &server_list[gsw_index].whois_stuff;
}

/* get_server_qtail - get the tail of the whois queue */
WhoisQueue *get_server_qtail(int gsq_index)
{
    if (gsq_index == -1)
	return WQ_tail;
    else if (gsq_index < number_of_servers)
	return server_list[gsq_index].WQ_tail;

    return NULL;
}

/* set_server_qhead - set the head of the whois queue */
void set_server_qhead(int ssq_index, WhoisQueue * value)
{
    if (ssq_index == -1)
	WQ_head = value;
    else if (ssq_index < number_of_servers)
	server_list[ssq_index].WQ_head = value;

    return;
}

/* set_server_qtail - set the tail of the whois queue */
void set_server_qtail(int ssq_index, WhoisQueue * value)
{
    if (ssq_index == -1)
	WQ_tail = value;
    else if (ssq_index < number_of_servers)
	server_list[ssq_index].WQ_tail = value;

    return;
}

/* 
 * set_server2_8 - set the server as a 2.8 server 
 * This is used if we get a 001 numeric so that we dont bite on
 * the "kludge" that ircd has for older clients
 */
void set_server2_8(int ss2_index, int value)
{
    if (ss2_index < number_of_servers)
	server_list[ss2_index].server2_8 = value;
    return;
}

/* get_server2_8 - get the server as a 2.8 server */
int get_server2_8(int gs2_index)
{
    if (gs2_index == -1)
	gs2_index = primary_server;
    else if (gs2_index >= number_of_servers)
	return 0;
    return (server_list[gs2_index].server2_8);
}

/*
 * get_server_operator: returns true if the user has op privs on the server,
 * false otherwise 
 */
int get_server_operator(int gso_index)
{
    if ((gso_index < 0) || (gso_index >= number_of_servers))
	return 0;
    return (server_list[gso_index].operator);
}

/*
 * set_server_operator: If flag is non-zero, marks the user as having op
 * privs on the given server.  
 */
void set_server_operator(int sso_index, int flag)
{
    if (sso_index < 0 || sso_index >= number_of_servers)
	return;
    server_list[sso_index].operator = flag;
    oper_command = 0;
    set_umode(sso_index);
}

/*
 * set_server_nickname: sets the nickname for the given server to nickname.
 * This nickname is then used for all future connections to that server
 * (unless changed with NICK while connected to the server 
 */
void set_server_nickname(int ssn_index, char *nick)
{
    if (ssn_index != -1 && ssn_index < number_of_servers) {
	malloc_strcpy(&(server_list[ssn_index].nickname), nick);
	if (ssn_index == primary_server)
	    strmcpy(nickname, nick, NICKNAME_LEN);
    }
    update_all_status(curr_scr_win, NULL, 0);
}

void register_server(int ssn_index, char *nick)
{
    int old_from_server = from_server;

    send_to_server("USER %s %s %s :%s", username,
		   (send_umode && *send_umode) ? send_umode :
		   (LocalHostName ? LocalHostName : hostname), username, *realname ? realname : space_str);
    change_server_nickname(ssn_index, nick);
    from_server = old_from_server;
}

void set_server_motd(int ssm_index, int flag)
{
    if (ssm_index != -1 && ssm_index < number_of_servers)
	server_list[ssm_index].motd = flag;
}

int get_server_lag(int gso_index)
{
    if ((gso_index < 0 || gso_index >= number_of_servers))
	return 0;
    return (server_list[gso_index].lag);
}

int get_server_motd(int gsm_index)
{
    if (gsm_index != -1 && gsm_index < number_of_servers)
	return (server_list[gsm_index].motd);
    return (0);
}

void server_is_connected(int sic_index, int value)
{
    if (sic_index < 0 || sic_index >= number_of_servers)
	return;

    server_list[sic_index].connected = value;
    if (value)
	server_list[sic_index].eof = 0;
}

/* send_to_server: sends the given info the the server */
void send_to_server(const char *format, ...)
{
    char buffer[BIG_BUFFER_SIZE + 1];	/* make this buffer *much* bigger than needed */
    char *buf = buffer;
    int len;
    int server;

    if ((server = from_server) == -1)
	server = primary_server;

    if (server != -1 && server_list[server].sock && format && (server_list[server].flags & LOGGED_IN)) {
	va_list args;

	va_start(args, format);
	vsnprintf(buf, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	server_list[server].sent = 1;
	len = strlen(buffer);
	if (len > (IRCD_BUFFER_SIZE - 2) || len == -1)
	    buffer[IRCD_BUFFER_SIZE - 2] = (char) 0;
	strmcat(buffer, "\r\n", IRCD_BUFFER_SIZE);
	XDEBUG(5, "[%d] -> [%s]", des, buffer);
	sa_write(server_list[server].sock, buffer, strlen(buffer), NULL);
	memset(buffer, 0, 80);
    } else
	/* if (from_server == -1) */
    {
	if (do_hook(DISCONNECT_LIST, "No Connection"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME),
					 "You are not connected to a server. Use /SERVER to connect."));
    }
}

/* send_to_server: sends the given info the the server */
void my_send_to_server(int server, const char *format, ...)
{
    char buffer[BIG_BUFFER_SIZE + 1];	/* make this buffer *much* bigger than needed */
    char *buf = buffer;
    int len;

    if (server == -1)
	server = primary_server;

    if (server != -1 && server_list[server].sock && format && (server_list[server].flags & LOGGED_IN)) {
	va_list args;

	va_start(args, format);
	vsnprintf(buf, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	server_list[server].sent = 1;
	len = strlen(buffer);
	if (len > (IRCD_BUFFER_SIZE - 2) || len == -1)
	    buffer[IRCD_BUFFER_SIZE - 2] = (char) 0;
	strmcat(buffer, "\r\n", IRCD_BUFFER_SIZE);
	sa_write(server_list[server].sock, buffer, strlen(buffer), NULL);
	memset(buffer, 0, 80);
    } else {
	if (do_hook(DISCONNECT_LIST, "No Connection"))
	    put_it("%s",
		   convert_output_format(get_fset_var(FORMAT_DISCONNECT_FSET), "%s %s", update_clock(GET_TIME),
					 "You are not connected to a server. Use /SERVER to connect."));
    }
}

/*
 * close_all_server: Used when creating new screens to close all the open
 * server connections in the child process...
 */
extern void close_all_server(void)
{
    int i;

    for (i = 0; i < number_of_servers; i++) {
	if (server_list[i].sock)
	    sa_destroy(server_list[i].sock);
    }
}

extern char *create_server_list(void)
{
    int i;
    char *value = NULL;
    char buffer[BIG_BUFFER_SIZE / 4 + 1];

    *buffer = '\0';
    for (i = 0; i < number_of_servers; i++) {
	if (server_list[i].sock) {
	    if (server_list[i].itsname) {
		strcat(buffer, server_list[i].itsname);
		strcat(buffer, space_str);
	    } else
		yell("Warning: server_list[%d].itsname is null and it shouldnt be", i);
	}
    }
    malloc_strcpy(&value, buffer);

    return value;
}

/*
 * This is the function to attempt to make a nickname change.  You
 * cannot send the NICK command directly to the server: you must call
 * this function.  This function makes sure that the neccesary variables
 * are set so that if the NICK command fails, a sane action can be taken.
 *
 * If ``nick'' is NULL, then this function just tells the server what
 * we're trying to change our nickname to.  If we're not trying to change
 * our nickname, then this function does nothing.
 */
void change_server_nickname(int ssn_index, char *nick)
{
    int old_from_server = from_server;

    from_server = ssn_index;

    if (nick) {
	if ((nick = check_nickname(nick)) != NULL) {
	    if (from_server != -1) {
		malloc_strcpy(&server_list[from_server].d_nickname, nick);
		malloc_strcpy(&server_list[from_server].s_nickname, nick);
	    } else {
		int i = 0;

		strncpy(nickname, nick, NICKNAME_LEN);
		for (i = 0; i < number_of_servers; i++) {
		    malloc_strcpy(&server_list[i].nickname, nickname);
		    new_free(&server_list[i].d_nickname);
		    new_free(&server_list[i].s_nickname);
		}
		user_changing_nickname = 0;
		return;
	    }
	} else
	    reset_nickname();
    }

    if (server_list[from_server].s_nickname)
	send_to_server("NICK %s", server_list[from_server].s_nickname);

    from_server = old_from_server;
}

void accept_server_nickname(int ssn_index, char *nick)
{
    malloc_strcpy(&server_list[ssn_index].nickname, nick);
    malloc_strcpy(&server_list[ssn_index].d_nickname, nick);
    new_free(&server_list[ssn_index].s_nickname);
    server_list[ssn_index].fudge_factor = 0;

    if (from_server == primary_server)
	strmcpy(nickname, nick, NICKNAME_LEN);

    update_all_status(curr_scr_win, NULL, 0);
    update_input(UPDATE_ALL);
}

/* 
 * This will generate up to 18 nicknames plus the 9-length(nickname)
 * that are unique but still have some semblance of the original.
 * This is intended to allow the user to get signed back on to
 * irc after a nick collision without their having to manually
 * type a new nick every time..
 * 
 * The func will try to make an intelligent guess as to when it is
 * out of guesses, and if it ever gets to that point, it will do the
 * manually-ask-you-for-a-new-nickname thing.
 */
void fudge_nickname(int servnum)
{
    char l_nickname[NICKNAME_LEN + 1];

    /* 
     * If we got here because the user did a /NICK command, and
     * the nick they chose doesnt exist, then we just dont do anything,
     * we just cancel the pending action and give up.
     */
    if (user_changing_nickname) {
	new_free(&server_list[from_server].s_nickname);
	return;
    }

    /* 
     * Ok.  So we're not doing a /NICK command, so we need to see
     * if maybe we're doing some other type of NICK change.
     */
    if (server_list[servnum].s_nickname)
	strcpy(l_nickname, server_list[servnum].s_nickname);
    else if (server_list[servnum].nickname)
	strcpy(l_nickname, server_list[servnum].nickname);
    else
	strcpy(l_nickname, nickname);

    if (server_list[servnum].fudge_factor < strlen(l_nickname))
	server_list[servnum].fudge_factor = strlen(l_nickname);
    else {
	server_list[servnum].fudge_factor++;
	if (server_list[servnum].fudge_factor == 17) {
	    /* give up... */
	    reset_nickname();
	    server_list[servnum].fudge_factor = 0;
	    return;
	}
    }

    /* 
     * Process of fudging a nickname:
     * If the nickname length is less then 9, add an underscore.
     */
    if (strlen(l_nickname) < 9)
	strcat(l_nickname, "_");

    /* 
     * The nickname is 9 characters long. roll the nickname
     */
    else {
	char tmp = l_nickname[8];

	l_nickname[8] = l_nickname[7];
	l_nickname[7] = l_nickname[6];
	l_nickname[6] = l_nickname[5];
	l_nickname[5] = l_nickname[4];
	l_nickname[4] = l_nickname[3];
	l_nickname[3] = l_nickname[2];
	l_nickname[2] = l_nickname[1];
	l_nickname[1] = l_nickname[0];
	l_nickname[0] = tmp;
    }

    change_server_nickname(servnum, l_nickname);
}

/*
 * -- Callback function
 */
static void nickname_sendline(char *data, char *nick)
{
    int new_server;

    new_server = atoi(data);
    change_server_nickname(new_server, nick);
}

/*
 * reset_nickname: when the server reports that the selected nickname is not
 * a good one, it gets reset here. 
 * -- Called by more than one place
 */
void reset_nickname(void)
{
    char server_num[10];

    kill(getpid(), SIGINT);
    say("You have specified an illegal nickname");
    say("Please enter your nickname");
    strcpy(server_num, ltoa(from_server));
    add_wait_prompt("Nickname: ", nickname_sendline, server_num, WAIT_PROMPT_LINE);
    update_all_status(curr_scr_win, NULL, 0);
}

/*
 * password_sendline: called by send_line() in get_password() to handle
 * hitting of the return key, etc 
 * -- Callback function
 */
void password_sendline(char *data, char *line)
{
    int new_server;

    new_server = atoi(line);
    set_server_password(new_server, line);
    connect_to_server_by_refnum(new_server, -1);
}

char *get_pending_nickname(int servnum)
{
    return server_list[servnum].s_nickname;
}
