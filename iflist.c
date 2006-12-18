#ident "@(#)iflist.c 1.5"
/*
 * iflist.c - figure out interface information
 * Copyright (C) 2000 Rex Feany <laeos@laeos.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "irc.h"
#include "ircaux.h"
#include "iflist.h"
#include "threads.h"
#include "gai.h"

/* List of my interfaces */
static struct iflist *iflist;

/* struct for passing information between threads */
struct thr_comm {
    iface_cb thr_ifcb;
    void *thr_data;
};

/* These were written/stolen from W Richard Steven's "Unix Network Programming" */
static struct iflist *iflist_get(void)
{
    struct iflist *ifi, *ifihead, **ifipnext;
    int sockfd, len, lastlen;
    char *ptr, *buf;
    struct ifconf ifc;
    struct ifreq *ifr, ifrcopy;

    errno = 0;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	return NULL;
    }

    lastlen = 0;
    len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
    for (;;) {
	buf = new_malloc(len);
	ifc.ifc_len = len;
	ifc.ifc_buf = buf;
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
	    if (errno != EINVAL || lastlen != 0)
		return NULL;
	} else {
	    if (ifc.ifc_len == lastlen)
		break;		/* success, len has not changed */
	    lastlen = ifc.ifc_len;
	}
	len += 10 * sizeof(struct ifreq);	/* increment */
	new_free(&buf);
    }
    ifihead = NULL;
    ifipnext = &ifihead;

    for (ptr = buf; ptr < buf + ifc.ifc_len;) {
	ifr = (struct ifreq *) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
	len = MAX(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
	switch (ifr->ifr_addr.sa_family) {
#ifdef	IPV6
	case AF_INET6:
	    len = sizeof(struct sockaddr_in6);
	    break;
#endif
	case AF_INET:
	default:
	    len = sizeof(struct sockaddr);
	    break;
	}
#endif				/* HAVE_SOCKADDR_SA_LEN */
	ptr += sizeof(ifr->ifr_name) + len;	/* for next one in buffer */

#ifdef	AF_LINK
	/* we don't care about link level things. just ip */
	if (AF_LINK == ifr->ifr_addr.sa_family)
	    continue;
#endif
	ifrcopy = *ifr;

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy) < 0) {
	    /* just ignore this error, and this interface ... */
	    continue;
	}

	if ((ifrcopy.ifr_flags & IFF_UP) == 0)
	    continue;		/* ignore if interface not up */
	if (ifrcopy.ifr_flags & IFF_LOOPBACK)
	    continue;		/* ignore if ... */

	ifi = new_malloc(sizeof(struct iflist));
	*ifipnext = ifi;	/* prev points to this new one */
	ifipnext = &ifi->ifi_next;	/* pointer to next one goes here */

	memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);;
	ifi->ifi_name[IFI_NAME - 1] = '\0';
	getnameinfo(&ifr->ifr_addr, len, ifi->ifi_host, NI_MAXHOST, NULL, 0, NI_NUMERICSERV);

	ifi->ifi_len = len;
	ifi->ifi_addr = new_malloc(len);
	memcpy(ifi->ifi_addr, &ifr->ifr_addr, len);
    }
    new_free(&buf);
    return (ifihead);		/* pointer to first structure in linked list */
}

/* free list of interfaces */
static void iflist_free(struct iflist *ifihead)
{
    struct iflist *ifi, *ifinext;

    for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
	if (ifi->ifi_addr != NULL)
	    new_free(&ifi->ifi_addr);
	ifinext = ifi->ifi_next;	/* can't fetch ifi_next after free() */
	new_free(&ifi);		/* the ifi_info{} itself */
    }
}

/* get list of interfaces */
static void *ifaces_r(void *data)
{
    struct thr_comm *th = (struct thr_comm *) data;

    /* this could take a long time (usually not) */
    iflist_update();

    th->thr_ifcb(th->thr_data, iflist);
    new_free(&th);

    THR_EXIT();
}

/**
 * ifaces - fetch a list of interfaces/hostnames
 * @callback: called with the list of interfaces
 * @data: app-specific data also passed to callback
 *
 * A callback function is used here because we may
 * block while looking up hostnames.
 **/
int ifaces(iface_cb callback, void *data)
{
    struct thr_comm *c = new_malloc(sizeof(struct thr_comm));

    assert(callback);

    c->thr_ifcb = callback;
    c->thr_data = data;

    return (int)THR_CREATE(ifaces_r, c);
}

/**
 * iface_find - find an iface struct given a host
 * @host: hostname to look for
 *
 * Each "interface" has hostnames associated with it,
 * we search for the interface structure that is 
 * associated with this hostname. Mainly used to
 * verify that a hostname is actually valid on this machine.
 **/
const struct iflist *iface_find(const char *host)
{
    struct iflist *l;
    struct iflist *m = NULL;
    int hlen = strlen(host);
    int len;

    assert(host);

    len = strlen(host);
    for (l = iflist; l; l = l->ifi_next)
	if (end_strcmp(l->ifi_host, host, len) == 0) {

	    /* best match */
	    if (strlen(l->ifi_host) == hlen)
		return l;

	    /* else take first match */
	    if (m == NULL)
		m = l;
	}
    return m;
}

/**
 * iflist_update - fetch and update the interface list
 *
 * We store a list of interfaces, which we get from the 
 * operating system. It may change over time, this
 * updates that list. Normally this doesn't need to
 * be called.
 **/
const struct iflist *iflist_update(void)
{
    struct iflist *ifl = iflist_get();
    struct iflist *ift = iflist;

    /* we don't lock, but what are the chances..? */
    iflist = ifl;

    if (ift)
	iflist_free(ift);

    return iflist;
}
