/*
 * timer.h: header for timer.c 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef _TIMER_H_
#define _TIMER_H_

/* functions that may be called by others */
extern void timercmd(char *, char *, char *, char *);
extern void ExecuteTimers(void);
extern char *add_timer(char *, long, long, int (*)(void *), char *, char *);
extern int delete_timer(char *);
extern void delete_all_timers(void);

extern time_t TimerTimeout(void);

#endif				/* _TIMER_H_ */
