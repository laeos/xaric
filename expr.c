/*
 * alias.c Handles command aliases for irc.c 
 * expr.c -- The expression mode parser and the textual mode parser
 *
 * Copyright(c) 1990, 1995 Michael Sandroff and others 
 * Copyright 1997 EPIC Software Labs
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <assert.h>

#include "irc.h"
#include "ircaux.h"
#include "output.h"
#include "fset.h"
#include "vars.h"
#include "expr.h"
#include "history.h"
#include "input.h"

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'
#define LEFT_BRACKET '['
#define RIGHT_BRACKET ']'
#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define DOUBLE_QUOTE '"'

/* ick XXX */
char *built_in_alias(char c);

/* alias_illegals: characters that are illegal in alias names */
static char alias_illegals[] = " #+-*/\\()={}[]<>!@$%^~`,?;:|'\"";

/* ??? */
static char *alias_string = NULL;

/* Function decls */
static void TruncateAndQuote(char **, char *, int, char *, char);
static void do_alias_string(char, char *);

/* given a variable name, try to figure out its value */
static char *get_variable_with_args(char *str, char *args, int *args_flag)
{
    char *ret = NULL;
    char *name = NULL;

    assert(str);

    name = remove_brackets(str, args, args_flag);

    if ((strlen(str) == 1) && (ret = built_in_alias(*str))) {
	new_free(&name);
	return ret;
    } else if ((ret = make_string_var(str))) ;
    else if ((ret = get_format_byname(str))) ;
    else
	ret = getenv(str);

    new_free(&name);
    return m_strdup(ret);
}

/**
 * TruncateAndQuote - String width formatting and quoting for irc variables.
 * @buff:
 * @add:
 * @length:
 * @quote_em:
 * @pad_char:
 * Description:
 *   Handle formatting and quoting when [] or ^x is specified.
 *   If length is nonzero, then "add" will be truncated to "length" characters.
 *   If length is zero, nothing is done to "add". If quote_em is not NULL,
 *   then the resulting string will be quoted and appended to "buff".
 *   If quote_em is NULL, then the value of "add" is appended to "buff"
 **/
static void TruncateAndQuote(char **buff, char *add, int length, char *quote_em, char pad_char)
{
    char *l_ptr = NULL, *q_ptr = NULL;

    if (length) {
	l_ptr = malloc(abs(length) + 1);
	strformat(l_ptr, add, length, pad_char);
	add = l_ptr;
    }
    if (quote_em) {
	q_ptr = malloc(strlen(add) * 2 + 2);
	add = double_quote(add, quote_em, q_ptr);
    }
    if (buff)
	malloc_strcat(buff, add);
    if (l_ptr) free(l_ptr);
    if (q_ptr) free(q_ptr);
}

static void do_alias_string(char unused, char *not_used)
{
    malloc_strcpy(&alias_string, get_input());
}

/**************************** TEXT MODE PARSER *****************************/
/*
 * expand_alias: Expands inline variables in the given string and returns the
 * expanded string in a new string which is malloced by expand_alias(). 
 *
 * Also unescapes anything that was quoted with a backslash
 *
 * Behaviour is modified by the following:
 *      Anything between brackets (...) {...} is left unmodified.
 *      If more_text is supplied, the text is broken up at
 *              semi-colons and returned one at a time. The unprocessed
 *              portion is written back into more_text.
 *      Backslash escapes are unescaped.
 */
char *expand_alias(char *string, char *args, int *args_flag, char **more_text)
{
    char *buffer = NULL, *ptr, *stuff = (char *) 0, *free_stuff, *quote_str = (char *) 0;
    char quote_temp[2];
    char ch;
    int is_quote = 0;
    int unescape = 1;

    if (!string || !*string)
	return m_strdup(empty_str);

    if (*string == '@' && more_text) {
	unescape = 0;
	*args_flag = 1;		/* Stop the @ command from auto appending */
    }
    quote_temp[1] = 0;

    ptr = free_stuff = stuff = strdup(string);

    if (more_text)
	*more_text = NULL;

    while (ptr && *ptr) {
	if (is_quote) {
	    is_quote = 0;
	    ++ptr;
	    continue;
	}
	switch (*ptr) {
	case '$':
	    {
		/* 
		 * The test here ensures that if we are in the 
		 * expression evaluation command, we don't expand $. 
		 * In this case we are only coming here to do command 
		 * separation at ';'s.  If more_text is not defined, 
		 * and the first character is '@', we have come here 
		 * from [] in an expression.
		 */
		if (more_text && *string == '@') {
		    ptr++;
		    break;
		}
		*ptr++ = 0;
		if (!*ptr)
		    break;
		m_strcat_ues(&buffer, stuff, unescape);

		while (*ptr == '^') {
		    quote_temp[0] = *++ptr;
		    malloc_strcat(&quote_str, quote_temp);
		    ptr++;
		}
		stuff = alias_special_char(&buffer, ptr, args, quote_str, args_flag);
		if (quote_str)
		    new_free(&quote_str);
		ptr = stuff;
		break;
	    }
	case ';':
	    {
		if (!more_text) {
		    ptr++;
		    break;
		}
		*more_text = string + (ptr - free_stuff) + 1;
		*ptr = '\0';	/* To terminate the loop */
		break;
	    }
	case LEFT_PAREN:
	case LEFT_BRACE:
	    {
		ch = *ptr;
		*ptr = '\0';
		m_strcat_ues(&buffer, stuff, unescape);
		stuff = ptr;
		*args_flag = 1;
		if (!(ptr = MatchingBracket(stuff + 1, ch, (ch == LEFT_PAREN) ? RIGHT_PAREN : RIGHT_BRACE))) {
		    yell("Unmatched %c", ch);
		    ptr = stuff + strlen(stuff + 1) + 1;
		} else
		    ptr++;

		*stuff = ch;
		ch = *ptr;
		*ptr = '\0';
		malloc_strcat(&buffer, stuff);
		stuff = ptr;
		*ptr = ch;
		break;
	    }
	case '\\':
	    {
		is_quote = 1;
		ptr++;
		break;
	    }
	default:
	    ptr++;
	    break;
	}
    }
    if (stuff)
	m_strcat_ues(&buffer, stuff, unescape);

    free(free_stuff);
    return buffer;
}

/*
 * alias_special_char: Here we determine what to do with the character after
 * the $ in a line of text. The special characters are described more fully
 * in the help/ALIAS file.  But they are all handled here. Parameters are the
 * return char ** pointer to which things are placed,
 * a ptr to the string (the first character of which is the special
 * character), the args to the alias, and a character indication what
 * characters in the string should be quoted with a backslash.  It returns a
 * pointer to the character right after the converted alias.
 * 
 * The args_flag is set to 1 if any of the $n, $n-, $n-m, $-m, $*, or $() 
 * is used in the alias.  Otherwise it is left unchanged.
 */
char *alias_special_char(char **buffer, char *ptr, char *args, char *quote_em, int *args_flag)
{
    char *tmp, *tmp2, c, pad_char = 0;

    int uppr, lowr, len;

    len = 0;
    if ((c = *ptr) == LEFT_BRACKET) {
	ptr++;
	if ((tmp = (char *) strchr(ptr, RIGHT_BRACKET)) != NULL) {
	    if (!isdigit(*(tmp - 1)))
		pad_char = *(tmp - 1);
	    *(tmp++) = (char) 0;
	    len = my_atol(ptr);
	    ptr = tmp;
	    c = *ptr;
	} else {
	    say("Missing %c", RIGHT_BRACKET);
	    return (ptr);
	}
    }
    tmp = ptr + 1;
    switch (c) {
    case LEFT_PAREN:
	{
	    char *sub_buffer = NULL;

	    if ((ptr = MatchingBracket(tmp, LEFT_PAREN, RIGHT_PAREN)) || (ptr = (char *) strchr(tmp, RIGHT_PAREN)))
		*(ptr++) = (char) 0;
	    tmp = expand_alias(tmp, args, args_flag, NULL);
	    alias_special_char(&sub_buffer, tmp, args, quote_em, args_flag);
	    TruncateAndQuote(buffer, sub_buffer, len, quote_em, pad_char);
	    new_free(&sub_buffer);
	    new_free(&tmp);
	    *args_flag = 1;
	    return (ptr);
	}
    case '!':
	{
	    if ((ptr = (char *) strchr(tmp, '!')) != NULL)
		*(ptr++) = (char) 0;
	    if ((tmp = do_history(tmp, empty_str)) != NULL) {
		TruncateAndQuote(buffer, tmp, len, quote_em, pad_char);
		new_free(&tmp);
	    }
	    return (ptr);
	}
    case DOUBLE_QUOTE:
	{
	    if ((ptr = (char *) strchr(tmp, DOUBLE_QUOTE)) != NULL)
		*(ptr++) = (char) 0;
	    alias_string = (char *) 0;
	    get_line(tmp, 0, do_alias_string);
	    TruncateAndQuote(buffer, alias_string, len, quote_em, pad_char);
	    new_free(&alias_string);
	    return (ptr);
	}
    case '*':
	{
	    TruncateAndQuote(buffer, args, len, quote_em, pad_char);
	    *args_flag = 1;
	    return (ptr + 1);
	}

	/* ok, ok. so i did forget something. */
    case '#':
    case '@':
	{
	    char c2 = 0;
	    char *sub_buffer = NULL;
	    char *rest, *val;

	    if ((rest = sindex(ptr + 1, alias_illegals))) {
		c2 = *rest;
		*rest = 0;
	    }

	    if (!ptr[1])
		sub_buffer = m_strdup(args);
	    else
		alias_special_char(&sub_buffer, ptr + 1, args, quote_em, args_flag);
	    if (c == '#')
		val = m_strdup(ltoa(word_count(sub_buffer)));
	    else
		val = m_strdup(ltoa(strlen(sub_buffer)));

	    TruncateAndQuote(buffer, val, len, quote_em, pad_char);
	    new_free(&val);
	    new_free(&sub_buffer);

	    if (rest)
		*rest = c2;

	    return rest;
	}

    default:
	{
	    if (!c)
		break;
	    if (isdigit(c) || (c == '-') || c == '~') {
		*args_flag = 1;
		if (c == '~') {
		    /* double check to make sure $~ still works */
		    lowr = uppr = EOS;
		    ptr++;
		} else if (c == '-') {
		    /* special case.. leading spaces are always retained when you do $-n, even if 'n' is 0.  The stock client stripped 
		     * spaces on $-0, which is not correct. */
		    lowr = SOS;
		    ptr++;
		    uppr = parse_number(&ptr);
		    if (uppr == -1)
			return empty_str;	/* error */
		} else {
		    lowr = parse_number(&ptr);
		    if (*ptr == '-') {
			ptr++;
			uppr = parse_number(&ptr);
			if (uppr == -1)
			    uppr = EOS;
		    } else
			uppr = lowr;
		}

		/* 
		 * Protect against a crash.  There
		 * are some gross syntactic errors
		 * that can be made that will result
		 * in ''args'' being NULL here.  That
		 * will crash the client, so we have
		 * to protect against that by simply
		 * chewing the expando.
		 */
		if (!args)
		    tmp2 = m_strdup(empty_str);
		else
		    tmp2 = extract_words(args, lowr, uppr);

		TruncateAndQuote(buffer, tmp2, len, quote_em, pad_char);
		new_free(&tmp2);
		return (ptr ? ptr : empty_str);
	    } else {
		char *rest, lch = (char) 0;
		int function_call = 0;

		/* 
		 * Why use ptr+1?  Cause try to maintain backward compatability
		 * can be a pain in the butt.  Basically, we don't want any of
		 * the illegal characters in the alias, except that things like
		 * $* and $, were around first, so they must remain legal.  So
		 * we skip the first char after the $.  Does this make sense?
		 *
		 * Ok.  All together now -- "NO, IT DOESNT."
		 */
		/* special case for $ */
		if (*ptr == '$') {
		    rest = ptr + 1;
		    lch = *rest;
		    *rest = (char) 0;
		}

		else if ((rest = sindex(ptr + 1, alias_illegals)) != NULL) {
		    if (isalpha(*ptr) || *ptr == '_') {
			/* its an array reference. */
			if (*rest == LEFT_BRACKET) {
			    while (*rest == LEFT_BRACKET) {
				if ((tmp = MatchingBracket(rest + 1, LEFT_BRACKET, RIGHT_BRACKET)))
				    rest = tmp + 1;
				else
				    /* Cross your fingers and hope for the best */
				    break;
			    }
			}

			/* Its a function call */
			else if (*rest == LEFT_PAREN) {
			    char *saver;

			    function_call = 1;
			    *rest++ = 0;
			    saver = rest;
			    if (!(tmp = MatchingBracket(rest, LEFT_PAREN, RIGHT_PAREN)))
				function_call = 0;
			    else {
				*tmp++ = 0;
				rest = tmp;
			    }
			    tmp = saver;
			}
		    }
		    lch = *rest;
		    *rest = (char) 0;
		}

		tmp = get_variable_with_args(ptr, args, args_flag);

		if (tmp) {
		    TruncateAndQuote(buffer, tmp, len, quote_em, pad_char);
		    new_free(&tmp);
		}

		if (rest)
		    *rest = lch;
		return (rest);
	    }
	}
    }
    return NULL;
}

/**
 * my_next_expr  - Find the next expression delimited by brackets.
 * @args: String to find expression in.
 * @type: Bracket type to use.
 * @whine: If true, yell when there is an error.
 *
 * The type of bracket expected is passed as a parameter. 
 * Returns NULL on error.
 **/
char *my_next_expr(char **args, char type, int whine)
{
    char *ptr, *ptr2, *ptr3;

    if (!*args)
	return NULL;
    ptr2 = *args;
    if (!*ptr2)
	return 0;
    if (*ptr2 != type) {
	if (whine)
	    say("Expression syntax");
	return 0;
    }				/* { */
    ptr = MatchingBracket(ptr2 + 1, type, (type == '(') ? ')' : '}');
    if (!ptr) {
	say("Unmatched '%c'", type);
	return 0;
    }
    *ptr = '\0';

    do
	ptr2++;
    while (my_isspace(*ptr2));

    ptr3 = ptr + 1;
    while (my_isspace(*ptr3))
	ptr3++;
    *args = ptr3;
    if (*ptr2) {
	ptr--;
	while (my_isspace(*ptr))
	    *ptr-- = '\0';
    }
    return ptr2;
}

extern char *next_expr_failok(char **args, char type)
{
    return my_next_expr(args, type, 0);
}

extern char *next_expr(char **args, char type)
{
    return my_next_expr(args, type, 1);
}
