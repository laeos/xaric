
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "gai.h"


/*
 * Set up the search[] array with the hostnames and address families
 * that we are to look up.
 */

/* include ga_nsearch1 */
int
ga_nsearch(const char *hostname, const struct addrinfo *hintsp,
		   struct search *search)
{
	int		nsearch = 0;

	if (hostname == NULL || hostname[0] == '\0') {
		if (hintsp->ai_flags & AI_PASSIVE) {
				/* 4no hostname and AI_PASSIVE: implies wildcard bind */
			switch (hintsp->ai_family) {
#ifdef	IPV4
			case AF_INET:
				search[nsearch].host = "0.0.0.0";
				search[nsearch].family = AF_INET;
				nsearch++;
				break;
#endif
#ifdef	IPV6
			case AF_INET6:
				search[nsearch].host = "0::0";
				search[nsearch].family = AF_INET6;
				nsearch++;
				break;
#endif
			case AF_UNSPEC:
#ifdef	IPV6
				search[nsearch].host = "0::0";	/* IPv6 first, then IPv4 */
				search[nsearch].family = AF_INET6;
				nsearch++;
#endif
#ifdef	IPV4
				search[nsearch].host = "0.0.0.0";
				search[nsearch].family = AF_INET;
				nsearch++;
#endif
				break;
			}
		} else {
			/* no host and not AI_PASSIVE: connect to local host */
			switch (hintsp->ai_family) {
#ifdef	IPV4
			case AF_INET:
				search[nsearch].host = "localhost";	/* 127.0.0.1 */
				search[nsearch].family = AF_INET;
				nsearch++;
				break;
#endif
#ifdef	IPV6
			case AF_INET6:
				search[nsearch].host = "0::1";
				search[nsearch].family = AF_INET6;
				nsearch++;
				break;
#endif
			case AF_UNSPEC:
#ifdef	IPV6
				search[nsearch].host = "0::1";	/* IPv6 first, then IPv4 */
				search[nsearch].family = AF_INET6;
				nsearch++;
#endif
#ifdef	IPV4
				search[nsearch].host = "localhost";
				search[nsearch].family = AF_INET;
				nsearch++;
#endif
				break;
			}
		}
	} else {	/* host is specified */
		switch (hintsp->ai_family) {
#ifdef	IPV4
		case AF_INET:
			search[nsearch].host = hostname;
			search[nsearch].family = AF_INET;
			nsearch++;
			break;
#endif
#ifdef	IPV6
		case AF_INET6:
			search[nsearch].host = hostname;
			search[nsearch].family = AF_INET6;
			nsearch++;
			break;
#endif
		case AF_UNSPEC:
#ifdef	IPV6
			search[nsearch].host = hostname;
			search[nsearch].family = AF_INET6;	/* IPv6 first */
			nsearch++;
#endif
#ifdef	IPV4
			search[nsearch].host = hostname;
			search[nsearch].family = AF_INET;	/* then IPv4 */
			nsearch++;
#endif
			break;
		}
	}
	assert(nsearch > 0);
	assert(nsearch < 3);

	return(nsearch);
}
