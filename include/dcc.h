/*
 * dcc.h: Things dealing client to client connections. 
 *
 * Written By Troy Rollo <troy@plod.cbme.unsw.oz.au> 
 *
 * Copyright(c) 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

/*
 * this file must be included after irc.h as i needed <sys/types.h>
 * <netinet/in.h> and <apra/inet.h> and, possibly, <sys/select.h>
 */

#ifndef __dcc_h_
#define __dcc_h_

#include "whois.h"

#define DCC_CHAT	((unsigned) 0x0001)
#define DCC_FILEOFFER	((unsigned) 0x0002)
#define DCC_FILEREAD	((unsigned) 0x0003)
#define	DCC_RAW_LISTEN	((unsigned) 0x0004)
#define	DCC_RAW		((unsigned) 0x0005)
#define DCC_RESENDOFFER	((unsigned) 0x0006)
#define DCC_REGETFILE	((unsigned) 0x0007)
#define DCC_FTPOPEN	((unsigned) 0x0009)
#define DCC_FTPGET	((unsigned) 0x000a)
#define DCC_FTPSEND	((unsigned) 0x000b)
#define DCC_XMITSEND	((unsigned) 0x000c)
#define DCC_XMITRECV	((unsigned) 0x000d)
#define DCC_TYPES	((unsigned) 0x00ff)

#define DCC_WAIT	((unsigned) 0x0100)
#define DCC_ACTIVE	((unsigned) 0x0200)
#define DCC_OFFER	((unsigned) 0x0400)
#define DCC_DELETE	((unsigned) 0x0800)
#define DCC_TWOCLIENTS	((unsigned) 0x1000)

#define DCC_CNCT_PEND	((unsigned) 0x2000)

#define DCC_QUEUE	((unsigned) 0x4000)
#define DCC_TDCC	((unsigned) 0x8000)
#define DCC_STATES	((unsigned) 0xff00)

#define DCC_PACKETID  0xfeab
#define MAX_DCC_BLOCK_SIZE 8192

	void	register_dcc_offer(char *, char *, char *, char *, char *, char *, char *, char *);
	void	process_dcc(char *);
	char	*dcc_raw_connect(char *, u_short);
	char	*dcc_raw_listen(int);
	void	dcc_list(char *, char *);
	void	dcc_chat_transmit(char *, char *, char *, char *);
	void	dcc_message_transmit(char *, char *, char *, int, int, char *, int);
	void	close_all_dcc(void);
	void	dcc_check(fd_set *, fd_set *);
	int	dcc_active(char *);
	void	dcc_reject(char *, char *, char *);
	void	set_dcc_bits(fd_set *, fd_set *);
	void	dcc_sendfrom_queue(void);	
	void	dcc_check_idle(void);
	void	dcc_glist(char *, char *);
	void	dcc_chatbot(char *, char *);
DCC_list	*dcc_searchlist(char *, char *, int, int, char *, char *, int);
	void	dcc_chat_crash_transmit(char *, char *);
	int	dcc_erase(DCC_list *);
	void	dcc_chat(char *, char *);
	void	dcc_filesend(char *, char *);
	char	*dcc_time(time_t);
	void	multiget(char *, char *);
	void	multisend(char *, char*);
	void	dcc_resend(char *, char *);
	void	dcc_regetfile(char *, char *);
	int	dcc_activebot(char *);
	void	dcc_bot_transmit(char *, char *, char *);
	void	dcc_raw_transmit(char *, char *, char *);
	int	dcc_activeraw(char *);
		                	                        
	extern	DCC_list *ClientList;

	void	dcc_ftpopen(char *, char *);
	int	dcc_ftpcommand(char *, char *);
	int	check_dcc_list(char *);
	int	dcc_exempt_save(FILE *);
							
#endif /* __dcc_h_ */
