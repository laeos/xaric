
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/nameser.h>	/* needed for <resolv.h> */
#include <arpa/inet.h>		/* inet_pton */
#include <resolv.h>		/* res_init, _res */
#include <string.h>
#include <ctype.h>

#include "gai.h"

int getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hintsp, struct addrinfo **result)
{
    int rc, error, nsearch;
    char **ap, *canon;
    struct hostent *hptr;
    struct search search[3], *sptr;
    struct addrinfo hints, *aihead, **aipnext;

    /* 
     * If we encounter an error we want to free() any dynamic memory
     * that we've allocated.  This is our hack to simplify the code.
     */
#define	error(e) { error = (e); goto bad; }

    aihead = NULL;		/* initialize automatic variables */
    aipnext = &aihead;
    canon = NULL;

    if (hintsp == NULL) {
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
    } else
	hints = *hintsp;	/* struct copy */

    /* first some basic error checking */
    if ((rc = ga_echeck(hostname, servname, hints.ai_flags, hints.ai_family, hints.ai_socktype, hints.ai_protocol)) != 0)
	error(rc);

    /* remainder of function for IPv4/IPv6 */
    nsearch = ga_nsearch(hostname, &hints, &search[0]);
    for (sptr = &search[0]; sptr < &search[nsearch]; sptr++) {
#ifdef	IPV4
	/* check for an IPv4 dotted-decimal string */
	if (isdigit(sptr->host[0])) {
	    struct in_addr inaddr;

	    if (inet_pton(AF_INET, sptr->host, &inaddr) == 1) {
		if (hints.ai_family != AF_UNSPEC && hints.ai_family != AF_INET)
		    error(EAI_ADDRFAMILY);
		if (sptr->family != AF_INET)
		    continue;	/* ignore */
		rc = ga_aistruct(&aipnext, &hints, &inaddr, AF_INET);
		if (rc != 0)
		    error(rc);
		continue;
	    }
	}
#endif

#ifdef	IPV6
	/* check for an IPv6 hex string */
	if ((isxdigit(sptr->host[0]) || sptr->host[0] == ':') && (strchr(sptr->host, ':') != NULL)) {
	    struct in6_addr in6addr;

	    if (inet_pton(AF_INET6, sptr->host, &in6addr) == 1) {
		if (hints.ai_family != AF_UNSPEC && hints.ai_family != AF_INET6)
		    error(EAI_ADDRFAMILY);
		if (sptr->family != AF_INET6)
		    continue;	/* ignore */
		rc = ga_aistruct(&aipnext, &hints, &in6addr, AF_INET6);
		if (rc != 0)
		    error(rc);
		continue;
	    }
	}
#endif
	/* remainder of for() to look up hostname */
	if ((_res.options & RES_INIT) == 0)
	    res_init();		/* need this to set _res.options */

	if (nsearch == 2) {
#ifdef	IPV6
	    _res.options &= ~RES_USE_INET6;
#endif
	    hptr = gethostbyname2(sptr->host, sptr->family);
	} else {
#ifdef  IPV6
	    if (sptr->family == AF_INET6)
		_res.options |= RES_USE_INET6;
	    else
		_res.options &= ~RES_USE_INET6;
#endif
	    hptr = gethostbyname(sptr->host);
	}
	if (hptr == NULL) {
	    if (nsearch == 2)
		continue;	/* failure OK if multiple searches */

	    switch (h_errno) {
	    case HOST_NOT_FOUND:
		error(EAI_NONAME);
	    case TRY_AGAIN:
		error(EAI_AGAIN);
	    case NO_RECOVERY:
		error(EAI_FAIL);
	    case NO_DATA:
		error(EAI_NODATA);
	    default:
		error(EAI_NONAME);
	    }
	}

	/* check for address family mismatch if one specified */
	if (hints.ai_family != AF_UNSPEC && hints.ai_family != hptr->h_addrtype)
	    error(EAI_ADDRFAMILY);

	/* save canonical name first time */
	if (hostname != NULL && hostname[0] != '\0' && (hints.ai_flags & AI_CANONNAME) && canon == NULL) {
	    if ((canon = strdup(hptr->h_name)) == NULL)
		error(EAI_MEMORY);
	}

	/* create one addrinfo{} for each returned address */
	for (ap = hptr->h_addr_list; *ap != NULL; ap++) {
	    rc = ga_aistruct(&aipnext, &hints, *ap, hptr->h_addrtype);
	    if (rc != 0)
		error(rc);
	}
    }
    if (aihead == NULL)
	error(EAI_NONAME);	/* nothing found */

    /* return canonical name */
    if (hostname != NULL && hostname[0] != '\0' && hints.ai_flags & AI_CANONNAME) {
	if (canon != NULL)
	    aihead->ai_canonname = canon;	/* strdup'ed earlier */
	else {
	    if ((aihead->ai_canonname = strdup(search[0].host)) == NULL)
		error(EAI_MEMORY);
	}
    }

    /* now process the service name */
    if (servname != NULL && servname[0] != '\0') {
	if ((rc = ga_serv(aihead, &hints, servname)) != 0)
	    error(rc);
    }

    *result = aihead;		/* pointer to first structure in linked list */
    return (0);

  bad:
    freeaddrinfo(aihead);	/* free any alloc'ed memory */
    return (error);
}
