/*
 * vars.h: header for vars.c
 *
 * Generated from vars.h.proto automatically by the Makefile
 *
 * @(#)$Id$
 */

#ifndef __vars_h_
#define __vars_h_

/* indexes for the irc_variable array */

enum VAR_TYPES {
#include "vars_gen.h"
    NUMBER_OF_VARIABLES
};

#define USERINFO_VAR USER_INFORMATION_VAR
#define USER_INFO_VAR USER_INFORMATION_VAR
#define CLIENTINFO_VAR CLIENT_INFORMATION_VAR
#define MSGLOGFILE_VAR MSGLOG_FILE_VAR

#define get_bool_var get_int_var
#define get_char_var get_int_var

char *get_string_var(enum VAR_TYPES);
int get_int_var(enum VAR_TYPES);
void set_string_var(enum VAR_TYPES, const char *);
void set_int_var(enum VAR_TYPES, unsigned int);
void init_variables(void);
int do_boolean(char *, int *);
void set_var_value(int, char *);
void save_variables(FILE * fp, int do_all);
char *make_string_var(char *);
int charset_size(void);
void clear_sets(void);
void set_highlight_char(Window *, char *, int);

extern const char *var_settings[];

	/* in keys.c */
void clear_bindings(void);

/* var_settings indexes  also used in display.c for highlights */
#define OFF 0
#define ON 1
#define TOGGLE 2

/* the types of IrcVariables */
#define BOOL_TYPE_VAR	0
#define CHAR_TYPE_VAR	1
#define INT_TYPE_VAR	2
#define STR_TYPE_VAR	3
#define SET_TYPE_VAR	4

#define	VF_NODAEMON	0x0001
#define VF_EXPAND_PATH	0x0002

#define VIF_CHANGED	0x01
#define VIF_GLOBAL	0x02
#define VIF_PENDING	0x08

#endif				/* __vars_h_ */
