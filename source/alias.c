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

#include "irc.h"
#include "alias.h"
#include "alist.h"
#include "dcc.h"
#include "commands.h"
#include "files.h"
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


/************************ LOW LEVEL INTERFACE *************************/

/*
 * 'name' is expected to already be in canonical form (uppercase, dot notation)
 */
static
Alias *
find_var_alias (char *name)
{
	Alias *item = NULL;
	int cache;
	int loc;
	int cnt = 0;
	int i;

	if (!strcmp (name, "::"))
		name += 2;

	for (cache = 0; cache < ALIAS_CACHE_SIZE; cache++)
	{
		if (var_alias.cache[cache] &&
		    var_alias.cache[cache]->name &&
		    !strcmp (name, var_alias.cache[cache]->name))
		{
			item = var_alias.cache[cache];
			cnt = -1;
			break;
		}
	}

	if (!item)
	{
		cache = ALIAS_CACHE_SIZE - 1;
		item = (Alias *) find_array_item ((array *) & var_alias, name, &cnt, &loc);
	}

	if (cnt < 0)
	{
		for (i = cache; i > 0; i--)
			var_alias.cache[i] = var_alias.cache[i - 1];
		var_alias.cache[0] = item;
		return item;
	}

	return NULL;
}


/************************* DIRECT VARIABLE EXPANSION ************************/
/*
 * get_variable: This simply looks up the given str.  It first checks to see
 * if its a user variable and returns it if so.  If not, it checks to see if
 * it's an IRC variable and returns it if so.  If not, it checks to see if
 * its and environment variable and returns it if so.  If not, it returns
 * null.  It mallocs the returned string 
 */
char *
get_variable (char *str)
{
	int af = 0;
	return get_variable_with_args (str, NULL, &af);
}


static char *
get_variable_with_args (char *str, char *args, int *args_flag)
{
	Alias *alias = NULL;
	char *ret = NULL;
	char *name = NULL;

	name = remove_brackets (str, args, args_flag);

	if ((alias = find_var_alias (name)) != NULL)
		ret = alias->stuff;
	else if ((strlen (str) == 1) && (ret = built_in_alias (*str)))
	{
		new_free (&name);
		return ret;
	}
	else if ((ret = make_string_var (str)))
		;
	else if ((ret = make_fstring_var (str)))
		;
	else
		ret = getenv (str);

	new_free (&name);
	return m_strdup (ret);
}


#include "expr.c"
