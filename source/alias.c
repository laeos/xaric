#ident "$Id$"
/*
 * alias.c Handles command aliases for irc.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990, 1995 Michael Sandroff and others 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "irc.h"
#include "alias.h"
#include "alist.h"
#include "dcc.h"
#include "commands.h"
#include "history.h"
#include "hook.h"
#include "input.h"
#include "ircaux.h"
#include "names.h"
#include "notify.h"
#include "numbers.h"
#include "output.h"
#include "parse.h"
#include "screen.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "ircterm.h"
#include "server.h"
#include "struct.h"
#include "list.h"
#include "keys.h"
#include "misc.h"
#include "newio.h"
#include "fset.h"
#include "timer.h"
#include <sys/stat.h>

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'
#define LEFT_BRACKET '['
#define RIGHT_BRACKET ']'
#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define DOUBLE_QUOTE '"'

extern int start_time;

/* alias_illegals: characters that are illegal in alias names */
char alias_illegals[] = " #+-*/\\()={}[]<>!@$%^~`,?;:|'\"";

static char *get_variable_with_args (char *str, char *args, int *args_flag);


/**************************** INTERMEDIATE INTERFACE *********************/
/* We define Alias here to keep it encapsulated */
/*
 * This is the description of an alias entry
 * This is an ``array_item'' structure
 */
typedef struct AliasItemStru
{
	char *name;		/* name of alias */
	char *stuff;		/* what the alias is */
	int global;		/* set if loaded from `global' */
}
Alias;

/*
 * This is the description for a list of aliases
 * This is an ``array_set'' structure
 */
#define ALIAS_CACHE_SIZE 30

typedef struct AliasStru
{
	Alias **list;
	int max;
	int max_alloc;
	Alias *cache[ALIAS_CACHE_SIZE];
}
AliasSet;

static AliasSet var_alias =
{NULL, 0, 0,
 {NULL}};

static Alias *find_var_alias (char *name);



static char *
get_variable_with_args (char *str, char *args, int *args_flag)
{
	Alias *alias = NULL;
	char *ret = NULL;
	char *name = NULL;

	name = remove_brackets (str, args, args_flag);

	if ((strlen (str) == 1) && (ret = built_in_alias (*str)))
	{
		new_free (&name);
		return ret;
	}
	else if ((ret = make_string_var (str)))
		;
	else if ((ret = get_format_byname (str)))
		;
	else
		ret = getenv (str);

	new_free (&name);
	return m_strdup (ret);
}


#include "expr.c"
