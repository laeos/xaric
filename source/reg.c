#ident "@(#)reg.c 1.7"
/*
 * The original was spagetti. I have replaced Michael's code with some of
 * my own which is a thousand times more readable and can also handle '%',
 * which substitutes anything except a space. This should enable people
 * to position things better based on argument. I have also added '?', which
 * substitutes to any single character. And of course it still handles '*'.
 * this should be more efficient than the previous version too.
 *
 * Thus this whole file becomes:
 *
 * Written By Troy Rollo
 *
 * Copyright(c) 1992
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "irc.h"
#include "ircaux.h"
#include "output.h"


#ifdef OLD_MATCH

static int total_explicit;

/*
 * The following #define is here because we *know* its behaviour.
 * The behaviour of toupper tends to be undefined when it's given
 * a non lower case letter.
 * All the systems supported by IRCII should be ASCII
 */
#define	mkupper(c)	(((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : c)

int 
match (register char *pattern, register char *string)
{
	char type = 0;

	while (*string && *pattern && *pattern != '*' && *pattern != '%')
	{
		if (*pattern == '\\' && pattern[1])
		{
			if (!*++pattern || !(mkupper (*pattern) == mkupper (*string)))
				return 0;
			else
				pattern++, string++, total_explicit++;
		}


		if (*pattern == '?')
			pattern++, string++;
		else if (mkupper (*pattern) == mkupper (*string))
			pattern++, string++, total_explicit++;
		else
			break;
	}
	if (*pattern == '*' || *pattern == '%')
	{
		type = (*pattern++);
		while (*string)
		{
			if (match (pattern, string))
				return 1;
			else if (type == '*' || *string != ' ')
				string++;
			else
				break;
		}
	}

	/* Slurp up any trailing *'s or %'s... */
	if (!*string && (type == '*' || type == '%'))
		while (*pattern && (*pattern == '*' || *pattern == '%'))
			pattern++;

	if (!*string && !*pattern)
		return 1;

	return 0;
}

/*
 * This version of wild_match returns 1 + the  count  of characters
 * explicitly matched if a match occurs. That way we can look for
 * the best match in a list
 */
/* \\[ and \\] handling done by Jeremy Nelson
 * EPIC will not use the new pattern matcher currently used by 
 * ircii because i am not convinced that it is 1) better * and 
 * 2) i think the \\[ \\] stuff is important.
 */
int 
wild_match (register char *pattern, register char *str)
{
	register char *ptr;
	char *ptr2 = pattern;
	int nest = 0;
	char my_buff[2048];
	char *arg;
	int best_total = 0;

	total_explicit = 0;

	/* Is there a \[ in the pattern to be expanded? */
	/* This stuff here just reduces the \[ \] set into a series of
	 * one-simpler patterns and then recurses */
	if ((ptr2 = strstr (pattern, "\\[")))
	{
		/* we will have to null this out, but not until weve used it */
		char *placeholder = ptr2;
		ptr = ptr2;

		/* yes. whats the character after it? (first time
		   through is a trivial case) */
		do
		{
			switch (ptr[1])
			{
				/* step over it and add to nest */
			case '[':
				ptr2 = ptr + 2;
				nest++;
				break;
				/* step over it and remove nest */
			case ']':
				ptr2 = ptr + 2;
				nest--;
				break;
			default:
				if (*ptr == '\\')
					ptr2++;
			}
		}
		/* 
		 * Repeat while there are more backslashes to look at and
		 * we have are still in nested \[ \] sets
		 */
		while ((nest) && (ptr = index (ptr2, '\\')));

		/* right now, we know ptr points to a \] or to null */
		/* remember that && short circuits and that ptr will 
		   not be set to null if (nest) is zero... */
		if (ptr)
		{
			/* null out and step over the original \[ */
			*placeholder = '\0';
			placeholder += 2;

			/* null out and step over the matching \] */
			*ptr = '\0';
			ptr += 2;

			/* 
			 * grab words ("" sets or space words) one at a time
			 * and attempt to match all of them.  The best value
			 * matched is the one used.
			 */
			while ((arg = new_next_arg (placeholder, &placeholder)))
			{
				int tmpval;
				strcpy (my_buff, pattern);
				strcat (my_buff, arg);
				strcat (my_buff, ptr);

				/* the total_explicit we return is whichever
				 * pattern has the highest total_explicit */
				if ((tmpval = wild_match (my_buff, str)))
				{
					if (tmpval > best_total)
						best_total = tmpval;
				}
			}
			return best_total;	/* end of expansion section */
		}
		/* Possibly an unmatched \[ \] set */
		else
		{
			total_explicit = 0;
			if (match (pattern, str))
				return total_explicit + 1;
			else
				return 0;
		}
	}
	/* trivial case (no expansion) when weve expanded all the way out */
	else if (match (pattern, str))
		return total_explicit + 1;
	else
		return 0;
}

#else /* NEW MATCH */

/*
 * Written By Douglas A. Lewis <dalewis@cs.Buffalo.EDU>
 *
 * This file is in the public domain.
 */

#include "irc.h"

#include "ircaux.h"

static int _wild_match (u_char *, u_char *);

#define RETURN_FALSE -1
#define RETURN_TRUE count

u_char lower_tab[256] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 91, 92, 93, 94, 95,
 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 145, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

#undef tolower			/* don't want previous version. */
#define tolower(x) lower_tab[x]

/*
 * So I don't make the same mistake twice: We don't need to check for '\\'
 * in the input (string to check) because it doesn't have a special meaning
 * in that context.
 *
 * We don't need to check for '\\' when we come accross a * or % .. The rest
 * of the routine will do that for us.
 */

static int
_wild_match (mask, string)
     u_char *mask, *string;
{
	u_char *m = mask, *n = string, *ma = NULL, *na = NULL, *mp = NULL,
	 *np = NULL;
	int just = 0, pcount = 0, acount = 0, count = 0;

	for (;;)
	{
		if (*m == '*')
		{
			ma = ++m;
			na = n;
			just = 1;
			mp = NULL;
			acount = count;
		}
		else if (*m == '%')
		{
			mp = ++m;
			np = n;
			pcount = count;
		}
		else if (*m == '?')
		{
			m++;
			if (!*n++)
				return RETURN_FALSE;
		}
		else
		{
			if (*m == '\\')
			{
				m++;
				/* Quoting "nothing" is a bad thing */
				if (!*m)
					return RETURN_FALSE;
			}
			if (!*m)
			{
				/*
				 * If we are out of both strings or we just
				 * saw a wildcard, then we can say we have a
				 * match
				 */
				if (!*n)
					return RETURN_TRUE;
				if (just)
					return RETURN_TRUE;
				just = 0;
				goto not_matched;
			}
			/*
			 * We could check for *n == NULL at this point, but
			 * since it's more common to have a character there,
			 * check to see if they match first (m and n) and
			 * then if they don't match, THEN we can check for
			 * the NULL of n
			 */
			just = 0;
			if (tolower (*m) == tolower (*n))
			{
				m++;
				if (*n == ' ')
					mp = NULL;
				count++;
				n++;
			}
			else
			{

			      not_matched:

				/*
				 * If there are no more characters in the
				 * string, but we still need to find another
				 * character (*m != NULL), then it will be
				 * impossible to match it
				 */
				if (!*n)
					return RETURN_FALSE;
				if (mp)
				{
					m = mp;
					if (*np == ' ')
					{
						mp = NULL;
						goto check_percent;
					}
					n = ++np;
					count = pcount;
				}
				else
				      check_percent:

				if (ma)
				{
					m = ma;
					n = ++na;
					count = acount;
				}
				else
					return RETURN_FALSE;
			}
		}
	}
}

int 
match (char *pattern, char *string)
{
/* -1 on false >= 0 on true */
	return ((_wild_match (pattern, string) >= 0) ? 1 : 0);
}

int
wild_match (pattern, str)
     char *pattern, *str;
{
	/* assuming a -1 return of false */
	return _wild_match ((u_char *) pattern, (u_char *) str) + 1;
}

#endif /* NEW MATCH */
