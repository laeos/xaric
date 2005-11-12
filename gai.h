#ifndef	gai_h__
#define	gai_h__

#include <netdb.h>

/* Make up for things missing things in <netdb.h> */

struct search {
	const char	*host;	/* hostname or address string */
	int		family;	/* AF_xxx */
};

#ifndef HAVE_ADDRINFO_STRUCT

struct addrinfo {
  int		ai_flags;			/* AI_PASSIVE, AI_CANONNAME */
  int		ai_family;			/* PF_xxx */
  int		ai_socktype;		/* SOCK_xxx */
  int		ai_protocol;		/* IPPROTO_xxx for IPv4 and IPv6 */
  size_t	ai_addrlen;			/* length of ai_addr */
  char		*ai_canonname;		/* canonical name for host */
  struct sockaddr	*ai_addr;	/* binary address */
  struct addrinfo	*ai_next;	/* next structure in linked list */
};

#endif /* HAVE_ADDRINFO_STRUCT */

/* chances are if one is missing they all are. right? */

/* these are for getaddrinfo */
#ifndef AI_PASSIVE 
#  define AI_PASSIVE	 1	/* socket is intended for bind() + listen() */
#  define AI_CANONNAME	 2	/* return canonical name */
#endif

/* getnameinfo() */
#ifndef NI_MAXHOST
#  define NI_MAXHOST	  1025	/* max hostname returned */
#endif
#ifndef NI_MAXSERV
#  define NI_MAXSERV	    32	/* max service name returned */
#endif
#ifndef NI_NOFQDN
#  define NI_NOFQDN	     1	/* do not return FQDN */
#endif
#ifndef NI_NUMERICHOST
#  define NI_NUMERICHOST     2	/* return numeric form of hostname */
#endif
#ifndef NI_NAMEREQD
#  define NI_NAMEREQD	     4	/* return error if hostname not found */
#endif
#ifndef NI_NUMERICSERV
#  define NI_NUMERICSERV     8	/* return numeric form of service name */
#endif
#ifndef NI_DGRAM
#  define NI_DGRAM	    16	/* datagram service for getservbyname() */
#endif

/* error returns */
#ifndef EAI_ADDRFAMILY 
#  define EAI_ADDRFAMILY	 1	/* address family for host not supported */
#  define EAI_AGAIN		 2	/* temporary failure in name resolution */
#  define EAI_BADFLAGS		 3	/* invalid value for ai_flags */
#  define EAI_FAIL		 4	/* non-recoverable failure in name resolution */
#  define EAI_FAMILY		 5	/* ai_family not supported */
#  define EAI_MEMORY		 6	/* memory allocation failure */
#  define EAI_NODATA		 7	/* no address associated with host */
#  define EAI_NONAME		 8	/* host nor service provided, or not known */
#  define EAI_SERVICE		 9	/* service not supported for ai_socktype */
#  define EAI_SOCKTYPE		10	/* ai_socktype not supported */
#  define EAI_SYSTEM		11	/* system error returned in errno */
#endif 


#ifndef HAVE_GETADDRINFO
/* function prototypes -- internal functions */
int		ga_aistruct(struct addrinfo ***, const struct addrinfo *, const void *, int);
struct addrinfo	*ga_clone(struct addrinfo *);
int		ga_echeck(const char *, const char *, int, int, int, int);
int		ga_nsearch(const char *, const struct addrinfo *, struct search *);
int		ga_port(struct addrinfo *, int , int);
int		ga_serv(struct addrinfo *, const struct addrinfo *, const char *);
int		ga_unix(const char *, struct addrinfo *, struct addrinfo **);
int		gn_ipv46(char *, size_t, char *, size_t, void *, size_t, int, int, int);


#endif

#ifndef HAVE_GETNAMEINFO
int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                    char *host, size_t hostlen,
                       char *serv, size_t servlen, int flags);

#endif

#ifndef HAVE_GAI_STRERROR
const char * gai_strerror(int err);
#endif


#ifndef HAVE_FREEADDRINFO
#endif

#endif	/* gai_h__ */
