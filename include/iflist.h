#ifndef iflist_h__
#define iflist_h__
/*
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
 * @(#)iflist.h 1.1
 *
 */

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#define	IFI_NAME	16		/* same as IFNAMSIZ in <net/if.h> */

#ifndef NI_MAXHOST
# define NI_MAXHOST	1024		/* hostname from getnameinfo */
#endif


struct iflist {
	char ifi_name[IFI_NAME];	/* interface name, null terminated */
	struct sockaddr *ifi_addr;	/* primary address */
	int ifi_len;			/* length of above address */
	char ifi_host[NI_MAXHOST];	/* hostname */
	struct iflist *ifi_next;	/* next of these structures */
};

/* callback function used below */
typedef void (*iface_cb)(void *data, struct iflist *list);

int ifaces(iface_cb callback, void *data);
const struct iflist * iface_find (const char *host);
void iflist_update (void);

#endif	/* iflist_h__ */
