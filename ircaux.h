/*
 * ircaux.h: header file for ircaux.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id$
 */

#ifndef _IRCAUX_H_
#define _IRCAUX_H_

#include "irc.h"
#include "irc_std.h"
#include <stdio.h>

typedef int comp_len_func(char *, char *, int);
typedef int comp_func(char *, char *);

extern void *n_malloc(size_t, char *, int);
extern void *n_realloc(void **, size_t, char *, int);

extern void *n_free(void **, char *, int);

#define new_malloc(x) n_malloc(x, __FILE__, __LINE__)
#define new_free(x) n_free((void **)(x), __FILE__, __LINE__)

/*#define new_realloc(x,y) n_realloc((x),(y),__FILE__,__LINE__)*/

#define RESIZE(x, y, z) n_realloc     ((void **)& (x), sizeof(y) * (z), __FILE__, __LINE__)

extern void beep_em(int);
extern char *check_nickname(char *);
extern char *next_arg(char *, char **);
extern char *new_next_arg(char *, char **);
extern char *new_new_next_arg(char *, char **, char *);
extern char *last_arg(char **);
extern char *expand_twiddle(char *);
extern char *upper(char *);
extern char *lower(char *);
extern char *sindex(char *, char *);
extern char *path_search(char *, char *);
extern char *double_quote(const char *, const char *, char *);
extern char *malloc_strcpy(char **, const char *);
extern char *malloc_str2cpy(char **, const char *, const char *);
extern char *malloc_strcat(char **, const char *);
extern char *m_s3cat_s(char **, const char *, const char *);
extern char *m_s3cat(char **, const char *, const char *);
extern char *m_3cat(char **, const char *, const char *);
extern char *m_e3cat(char **, const char *, const char *);
extern char *m_3dup(const char *, const char *, const char *);
extern char *m_opendup(const char *, ...);
extern char *m_strdup(const char *);
extern void wait_new_free(char **);
extern char *malloc_sprintf(char **, const char *, ...);
extern char *m_sprintf(const char *, ...);
extern int is_number(char *);
extern char *my_ctime(time_t);
extern int my_stricmp(const char *, const char *);
extern int my_strnicmp(const char *, const char *, int);
extern void really_free(int);
extern char *chop(char *, int);
extern char *strmcpy(char *, const char *, int);
extern char *strmcat(char *, const char *, int);
extern char *strmcat_ue(char *, const char *, int);
extern char *m_strcat_ues(char **, char *, int);
extern char *stristr(char *, char *);
extern char *rstristr(char *, char *);
extern FILE *uzfopen(char **, char *);
extern int end_strcmp(const char *, const char *, int);
extern void ircpanic(char *, ...);
extern int vt100_decode(register unsigned char);
extern int fw_strcmp(comp_len_func *, char *, char *);
extern int lw_strcmp(comp_func *, char *, char *);
extern int open_to(char *, int, int);
extern struct timeval get_time(struct timeval *);
extern double time_diff(struct timeval, struct timeval);
extern char *plural(int);
extern char *remove_trailing_spaces(char *);
extern char *ltoa(long);
extern char *strformat(char *, char *, int, char);
extern char *chop_word(char *);
extern int check_val(char *);
extern char *strextend(char *, char, int);
extern char *pullstr(char *, char *);
extern int empty(const char *);
extern char *safe_new_next_arg(char *, char **);
extern char *MatchingBracket(char *, char, char);
extern int word_count(char *);
extern int parse_number(char **);
extern char *remove_brackets(char *, char *, int *);
extern u_long hashpjw(char *, u_long);
extern char *m_dupchar(int);
extern char *strmccat(char *, char, int);
extern long file_size(char *);
extern int is_root(char *, char *, int);
extern size_t streq(const char *, const char *);
extern char *m_strndup(const char *, size_t);

extern char *on_off(int);
extern char *rfgets(char *, int, FILE *);
extern char *strmopencat(char *, int, ...);
extern long my_atol(char *);
extern char *s_next_arg(char **);
extern char *next_in_comma_list(char *, char **);

/* From words.c */
#define SOS -32767
#define EOS 32767
extern char *extract_words(char *, int, int);
extern int match(char *, char *);

/* Used for connect_by_number */
#define SERVICE_SERVER 0
#define SERVICE_CLIENT 1
#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1

/* Used from network.c */
extern int connect_by_number(char *, unsigned short *, int, int, int);
extern struct hostent *resolv(const char *);
extern struct hostent *lookup_host(const char *);
extern struct hostent *lookup_ip(const char *);
extern char *host_to_ip(const char *);
extern char *ip_to_host(const char *);
extern char *one_to_another(const char *);
extern char *strfill(char, int);
extern char *ov_strcpy(char *, const char *);
extern int count_ansi(register const char *, const int);
extern void FixColorAnsi(char *);

extern int set_blocking(int);
extern int set_non_blocking(int);

#define my_isspace(x) \
	((x) == 9 || (x) == 10 || (x) == 11 || (x) == 12 || (x) == 13 || (x) == 32)

#define my_isdigit(x) \
(*x >= '0' && *x <= '9') || \
((*x == '-'  || *x == '+') && (x[1] >= '0' && x[1] <= '9'))

#define	_1KB	(1024.0)
#define	_1MEG	(1024.0*1024.0)
#define	_1GIG	(1024.0*1024.0*1024.0)
#define	_1TER	(1024.0*1024.0*1024.0*1024.0)
#define	_1ETA	(1024.0*1024.0*1024.0*1024.0*1024.0)

#define	_GMKs(x)	( (x > _1ETA) ? "Eb" : ((x > _1TER) ? "Tb" : ((x > _1GIG) ? "Gb" : \
			((x > _1MEG) ? "Mb" : ((x > _1KB)? "Kb" : "bytes")))))

#define	_GMKv(x)	((x > _1ETA) ? \
			(double)(x/_1ETA) : ((x > _1TER) ? \
			(double)(x/_1TER) : ((x > _1GIG) ? \
			(double)(x/_1GIG) : ((x > _1MEG) ? \
			(double)(x/_1MEG) : ((x > _1KB) ? \
			(double)(x/_1KB): (double)x)))) )

#endif				/* _IRCAUX_H_ */
