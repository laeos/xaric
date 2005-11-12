

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "gai.h"


int
getnameinfo(const struct sockaddr *sa, socklen_t salen,
		    char *host, size_t hostlen,
			char *serv, size_t servlen, int flags)
{

	switch (sa->sa_family) {
#ifdef	IPV4
	case AF_INET: {
		struct sockaddr_in	*sain = (struct sockaddr_in *) sa;

		return(gn_ipv46(host, hostlen, serv, servlen,
						&sain->sin_addr, sizeof(struct in_addr),
						AF_INET, sain->sin_port, flags));
	}
#endif

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sain = (struct sockaddr_in6 *) sa;

		return(gn_ipv46(host, hostlen, serv, servlen,
						&sain->sin6_addr, sizeof(struct in6_addr),
						AF_INET6, sain->sin6_port, flags));
	}
#endif


	default:
		return(1);
	}
}
