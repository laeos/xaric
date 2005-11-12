
#ifndef _WhoWas_h
#define _WhoWas_h

#define whowas_userlist_max 300
#define whowas_reg_max 500
#define whowas_chan_max 20
#include "hash.h"

typedef struct _whowaschan_str {
	struct _whowaschan_str *next;
	char *channel;
	ChannelList *channellist;
	time_t time;
} WhowasChanList;

typedef struct _whowaswrapchan_str {
	HashEntry NickListTable[WHOWASLIST_HASHSIZE];
} WhowasWrapChanList;

typedef struct _whowas_str {
	struct _whowas_str *next;
	int	has_ops; 	/* true is user split away with opz */
	char 	*channel;	/* name of channel */
	time_t 	time;		/* time of split/leave */
	char	*server1;
	char	*server2;
	NickList *nicklist;     /* pointer to nicklist */
	ChannelList *channellist;		
} WhowasList;

typedef struct _whowas_wrap_str {
	unsigned long total_hits;
	unsigned long total_links;
	unsigned long total_unlinks;
	HashEntry NickListTable[WHOWASLIST_HASHSIZE];
} WhowasWrapList;

WhowasList *check_whowas_buffer(char *, char *, char *, int);
WhowasList *check_whowas_nick_buffer(char *, char *, int);
WhowasList *check_whosplitin_buffer(char *, char *, char *, int);

void add_to_whowas_buffer(NickList *, char *, char *, char *);
void add_to_whosplitin_buffer(NickList *, char *, char *, char *);

int remove_oldest_whowas(WhowasWrapList *, time_t, int);
void clean_whowas_list(void);

WhowasChanList *check_whowas_chan_buffer(char *, int);
void add_to_whowas_chan_buffer(ChannelList *);
int remove_oldest_chan_whowas(WhowasChanList **, time_t, int);
void clean_whowas_chan_list(void);
void show_whowas(void);
void show_wholeft(char *);

extern WhowasWrapList whowas_splitin_list;
extern WhowasWrapList whowas_userlist_list;
extern WhowasWrapList whowas_reg_list;
#endif
