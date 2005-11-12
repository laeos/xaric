
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include "gai.h"


/*
 * Basic error checking of flags, family, socket type, and protocol.
 */

int
ga_echeck(const char *hostname, const char *servname,
		  int flags, int family, int socktype, int protocol)
{
	if (flags & ~(AI_PASSIVE | AI_CANONNAME))
		return(EAI_BADFLAGS);	/* unknown flag bits */

	if (hostname == NULL || hostname[0] == '\0') {
		if (servname == NULL || servname[0] == '\0')
			return(EAI_NONAME);	/* host or service must be specified */
	}

	switch(family) {
		case AF_UNSPEC:
			break;
#ifdef	IPV4
		case AF_INET:
			if (socktype != 0 &&
				(socktype != SOCK_STREAM &&
				 socktype != SOCK_DGRAM &&
				 socktype != SOCK_RAW))
				return(EAI_SOCKTYPE);	/* invalid socket type */
			break;
#endif
#ifdef	IPV6
		case AF_INET6:
			if (socktype != 0 &&
				(socktype != SOCK_STREAM &&
				 socktype != SOCK_DGRAM &&
				 socktype != SOCK_RAW))
				return(EAI_SOCKTYPE);	/* invalid socket type */
			break;
#endif
		case AF_LOCAL:
			if (socktype != 0 &&
				(socktype != SOCK_STREAM &&
				 socktype != SOCK_DGRAM))
				return(EAI_SOCKTYPE);	/* invalid socket type */
			break;
		default:
			return(EAI_FAMILY);		/* unknown protocol family */
	}
	return(0);
}
