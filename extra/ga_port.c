
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "gai.h"


/*
 * Go through all the addrinfo structures, checking for a match of the
 * socket type and filling in the socket type, and then the port number
 * in the corresponding socket address structures.
 */

/* include ga_port */
int
ga_port(struct addrinfo *aihead, int port, int socktype)
		/* port must be in network byte order */
{
	int				nfound = 0;
	struct addrinfo	*ai;

	for (ai = aihead; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_socktype != socktype)
			continue;		/* ignore if mismatch on socket type */

		ai->ai_socktype = socktype;

		switch (ai->ai_family) {
#ifdef	IPV4
			case AF_INET:
				((struct sockaddr_in *) ai->ai_addr)->sin_port = port;
				nfound++;
				break;
#endif
#ifdef	IPV6
			case AF_INET6:
				((struct sockaddr_in6 *) ai->ai_addr)->sin6_port = port;
				nfound++;
				break;
#endif
		}
	}
	return(nfound);
}
/* end ga_port */
