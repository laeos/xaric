#ident "@(#)network.c 1.5"
/*
 * network.c -- handles stuff dealing with connecting and name resolving
 *
 * Written by Jeremy Nelson in 1995
 * See the COPYRIGHT file or do /help ircii copyright
 */
#define SET_SOURCE_SOCKET

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "ircaux.h"
#include "vars.h"

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

extern char hostname[NAME_LEN + 1];

/*
 * connect_by_number:  Wheeeee. Yet another monster function i get to fix
 * for the sake of it being inadequate for extension.
 *
 * we now take four arguments:
 *
 *      - hostname - name of the host (pathname) to connect to (if applicable)
 *      - portnum - port number to connect to or listen on (0 if you dont care)
 *      - service -     0 - set up a listening socket
 *                      1 - set up a connecting socket
 *      - protocol -    0 - use the TCP protocol
 *                      1 - use the UDP protocol
 *
 *
 * Returns:
 *      Non-negative number -- new file descriptor ready for use
 *      -1 -- could not open a new file descriptor or 
 *              an illegal value for the protocol was specified
 *      -2 -- call to bind() failed
 *      -3 -- call to listen() failed.
 *      -4 -- call to connect() failed
 *      -5 -- call to getsockname() failed
 *      -6 -- the name of the host could not be resolved
 *      -7 -- illegal or unsupported request
 *
 *
 * Credit: I couldnt have put this together without the help of BSD4.4-lite
 * User Supplimentary Document #20 (Inter-process Communications tutorial)
 */
int 
connect_by_number (char *hostn, unsigned short *portnum, int service, int protocol, int nonblocking)
{
	int fd = -1;
	int is_unix = (hostn && *hostn == '/');
	int sock_type, proto_type;

	sock_type = (is_unix) ? AF_UNIX : AF_INET;
	proto_type = (protocol == PROTOCOL_TCP) ? SOCK_STREAM : SOCK_DGRAM;

	if ((fd = socket (sock_type, proto_type, 0)) < 0)
		return -1;

	set_socket_options (fd);

	/* Unix domain server */
#ifdef HAVE_SYS_UN_H
	if (is_unix)
	{
		struct sockaddr_un name;

		memset (&name, 0, sizeof (struct sockaddr_un));
		name.sun_family = AF_UNIX;
		strcpy (name.sun_path, hostn);
#ifdef HAVE_SUN_LEN
#ifdef SUN_LEN
		name.sun_len = SUN_LEN (&name);
#else
		name.sun_len = strlen (hostn) + 1;
#endif
#endif

		if (is_unix && (service == SERVICE_SERVER))
		{
			if (bind (fd, (struct sockaddr *) &name, strlen (name.sun_path) + 2))
				return close (fd), -2;

			if (protocol == PROTOCOL_TCP)
				if (listen (fd, 4) < 0)
					return close (fd), -3;
		}

		/* Unix domain client */
		else if (service == SERVICE_CLIENT)
		{
			alarm (get_int_var (CONNECT_TIMEOUT_VAR));
			if (connect (fd, (struct sockaddr *) &name, strlen (name.sun_path) + 2) < 0)
			{
				alarm (0);
				return close (fd), -4;
			}
			alarm (0);
		}
	}
	else
#endif

		/* Inet domain server */
	if (!is_unix && (service == SERVICE_SERVER))
	{
		int length;
		struct sockaddr_in name;

		memset (&name, 0, sizeof (struct sockaddr_in));
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = htonl (INADDR_ANY);
		name.sin_port = htons (*portnum);

		if (bind (fd, (struct sockaddr *) &name, sizeof (name)))
			return close (fd), -2;

		length = sizeof (name);
		if (getsockname (fd, (struct sockaddr *) &name, &length))
			return close (fd), -5;

		*portnum = ntohs (name.sin_port);

		if (protocol == PROTOCOL_TCP)
			if (listen (fd, 4) < 0)
				return close (fd), -3;
		if (nonblocking && set_non_blocking (fd) < 0)
			return close (fd), -4;
	}

	/* Inet domain client */
	else if (!is_unix && (service == SERVICE_CLIENT))
	{
		struct sockaddr_in server;
		struct hostent *hp;
		struct sockaddr_in localaddr;
		/*
		 * Doing this bind is bad news unless you are sure that
		 * the hostname is valid.  This is not true for me at home,
		 * since i dynamic-ip it.
		 */
		if (LocalHostName)
		{
			memset (&localaddr, 0, sizeof (struct sockaddr_in));
			localaddr.sin_family = AF_INET;
			localaddr.sin_addr = LocalHostAddr;
			localaddr.sin_port = 0;
			if (bind (fd, (struct sockaddr *) &localaddr, sizeof (localaddr)))
				return close (fd), -2;
		}

		memset (&server, 0, sizeof (struct sockaddr_in));
		if (!(hp = resolv (hostn)))
			return close (fd), -6;
		memcpy (&(server.sin_addr), hp->h_addr, hp->h_length);
		server.sin_family = AF_INET;
		server.sin_port = htons (*portnum);

		if (nonblocking && set_non_blocking (fd) < 0)
			return close (fd), -4;
		alarm (get_int_var (CONNECT_TIMEOUT_VAR));
		if (connect (fd, (struct sockaddr *) &server, sizeof (server)) < 0)
		{
			alarm (0);
			if (errno != EINPROGRESS && !nonblocking)
				return close (fd), -4;
		}
		alarm (0);
	}

	/* error */
	else
		return close (fd), -7;

	return fd;
}


extern struct hostent *
resolv (const char *stuff)
{
	struct hostent *hep;

	if ((hep = lookup_host (stuff)) == NULL)
		hep = lookup_ip (stuff);

	return hep;
}

extern struct hostent *
lookup_host (const char *host)
{
	struct hostent *hep;

	alarm (1);
	hep = gethostbyname (host);
	alarm (0);
	return hep;
}

extern char *
host_to_ip (const char *host)
{
	struct hostent *hep = lookup_host (host);
	static char ip[256];

	return (hep ? sprintf (ip, "%u.%u.%u.%u", hep->h_addr[0] & 0xff,
			       hep->h_addr[1] & 0xff,
			       hep->h_addr[2] & 0xff,
			       hep->h_addr[3] & 0xff),
		ip : empty_string);
}

extern struct hostent *
lookup_ip (const char *ip)
{
	int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char foo[4];
	struct hostent *hep;

	sscanf (ip, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
	foo[0] = b1;
	foo[1] = b2;
	foo[2] = b3;
	foo[3] = b4;

	alarm (1);
	hep = gethostbyaddr (foo, 4, AF_INET);
	alarm (0);

	return hep;
}

extern char *
ip_to_host (const char *ip)
{
	struct hostent *hep = lookup_ip (ip);
	static char host[128];

	return (hep ? strcpy (host, hep->h_name) : empty_string);
}

extern char *
one_to_another (const char *what)
{

	if (!isdigit (what[strlen (what) - 1]))
		return host_to_ip (what);
	else
		return ip_to_host (what);
}


int 
set_non_blocking (int fd)
{
	int res, nonb = 0;

#if defined(O_NONBLOCK)
	nonb |= O_NONBLOCK;
#else
#if defined(O_NDELAY)
	nonb |= O_NDELAY;
#else
	res = 1;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#endif
#endif
#if defined(O_NONBLOCK) || defined(O_NDELAY)
	if ((res = fcntl (fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl (fd, F_SETFL, res | nonb) == -1)
		return -1;
#endif
	return 0;
}

int 
set_blocking (int fd)
{
	int res, nonb = 0;

#if defined(O_NONBLOCK)
	nonb |= O_NONBLOCK;
#else
#if defined(O_NDELAY)
	nonb |= O_NDELAY;
#else
	res = 0;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#endif
#endif
#if defined(O_NONBLOCK) || defined(O_NDELAY)
	if ((res = fcntl (fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl (fd, F_SETFL, res & ~nonb) == -1)
		return -1;
#endif
	return 0;
}

