
#ifndef _WhoWas_h
#define _WhoWas_h

#define whowas_userlist_max 300
#define whowas_reg_max 500
#define whowas_chan_max 20

#include "hash.h"

struct whowas_chan_list {
	struct whowas_chan_list *next;
	char *channel;
	struct channel *channellist;
	time_t time;
};

struct whowas_chan_list_head {
	struct hash_entry NickListTable[WHOWASLIST_HASHSIZE];
};

struct whowas_list {
	struct whowas_list *next;
	int	has_ops; 	/* true is user split away with opz */
	char 	*channel;	/* name of channel */
	time_t 	time;		/* time of split/leave */
	char	*server1;
	char	*server2;
	struct nick_list *nicklist;     /* pointer to nicklist */
	struct channel *channellist;		
};

struct whowas_list_head {
	unsigned long total_hits;
	unsigned long total_links;
	unsigned long total_unlinks;
	struct hash_entry NickListTable[WHOWASLIST_HASHSIZE];
};

struct whowas_list *check_whowas_buffer(char *, char *, char *, int);
struct whowas_list *check_whowas_nick_buffer(char *, char *, int);
struct whowas_list *check_whosplitin_buffer(char *, char *, char *, int);

void add_to_whowas_buffer(struct nick_list *, char *, char *, char *);
void add_to_whosplitin_buffer(struct nick_list *, char *, char *, char *);

int remove_oldest_whowas(struct whowas_list_head *, time_t, int);
void clean_whowas_list(void);

struct whowas_chan_list *check_whowas_chan_buffer(char *, int);
void add_to_whowas_chan_buffer(struct channel *);
int remove_oldest_chan_whowas(struct whowas_chan_list **, time_t, int);
void clean_whowas_chan_list(void);
void show_whowas(void);
void show_wholeft(char *);

extern struct whowas_list_head whowas_splitin_list;
extern struct whowas_list_head whowas_userlist_list;
extern struct whowas_list_head whowas_reg_list;
#endif
