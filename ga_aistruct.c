

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "gai.h"


/*
 * Create and fill in an addrinfo{}.
 */

int
ga_aistruct(struct addrinfo ***paipnext, const struct addrinfo *hintsp,
			const void *addr, int family)
{
	struct addrinfo	*ai;

	if ( (ai = calloc(1, sizeof(struct addrinfo))) == NULL)
		return(EAI_MEMORY);
	ai->ai_next = NULL;
	ai->ai_canonname = NULL;
	**paipnext = ai;
	*paipnext = &ai->ai_next;

	ai->ai_socktype = hintsp->ai_socktype;
	ai->ai_protocol = hintsp->ai_protocol;
	
	switch ((ai->ai_family = family)) {
#ifdef	IPV4
		case AF_INET: {
			struct sockaddr_in	*sinptr;

				/* 4allocate sockaddr_in{} and fill in all but port */
			if ( (sinptr = calloc(1, sizeof(struct sockaddr_in))) == NULL)
				return(EAI_MEMORY);
#ifdef	HAVE_SOCKADDR_SA_LEN
			sinptr->sin_len = sizeof(struct sockaddr_in);
#endif
			sinptr->sin_family = AF_INET;
			memcpy(&sinptr->sin_addr, addr, sizeof(struct in_addr));
			ai->ai_addr = (struct sockaddr *) sinptr;
			ai->ai_addrlen = sizeof(struct sockaddr_in);
			break;
		}
#endif	/* IPV4 */
#ifdef	IPv6
		case AF_INET6: {
			struct sockaddr_in6	*sin6ptr;

				/* 4allocate sockaddr_in6{} and fill in all but port */
			if ( (sin6ptr = calloc(1, sizeof(struct sockaddr_in6))) == NULL)
				return(EAI_MEMORY);
#ifdef	HAVE_SOCKADDR_SA_LEN
			sin6ptr->sin6_len = sizeof(struct sockaddr_in6);
#endif
			sin6ptr->sin6_family = AF_INET6;
			memcpy(&sin6ptr->sin6_addr, addr, sizeof(struct in6_addr));
			ai->ai_addr = (struct sockaddr *) sin6ptr;
			ai->ai_addrlen = sizeof(struct sockaddr_in6);
			break;
		}
#endif	/* IPV6 */
	}
	return(0);
}
