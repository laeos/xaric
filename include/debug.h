/*
 * debug.h -- the runtime debug settings.  Can also be done on command line.
 */

#ifndef __X_DEBUG_H__
#define __X_DEBUG_H__

extern 	unsigned long x_debug;
extern	void xdebugcmd(char *, char *, char *, char *);

#define DEBUG_LOCAL_VARS	1 << 0
#define DEBUG_ALIAS		1 << 1
#define DEBUG_CTCPS		1 << 2
#define DEBUG_DCC_SEARCH	1 << 3
#define DEBUG_OUTBOUND		1 << 4
#define DEBUG_INBOUND		1 << 5
#define DEBUG_DCC_XMIT		1 << 6
#define DEBUG_WAITS		1 << 7
#define DEBUG_MEMORY		1 << 8
#define DEBUG_SERVER_CONNECT	1 << 9
#define DEBUG_CRASH		1 << 10
#define DEBUG_ALL		(unsigned long)0xffffffff
#endif
