#ident "@(#)ircaux.c 1.15"
/*
 * ircaux.c: some extra routines... not specific to irc... that I needed 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990, 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "log.h"
#include "misc.h"
#include "vars.h"
#include "screen.h"
#include "util.h"

#include <pwd.h>

#include <sys/stat.h>

#include "ircaux.h"
#include "output.h"
#include "ircterm.h"
#include "expr.h"
#include "xaric_version.h"
#include "xmalloc.h"


/*
 * malloc_strcpy:  Mallocs enough space for src to be copied in to where
 * ptr points to.
 *
 * Never call this with ptr pointinng to an uninitialised string, as the
 * call to xfree() might crash the client... - phone, jan, 1993.
 */
char *
malloc_strcpy (char **ptr, const char *src)
{
	if (!src)
		return xfree (ptr), NULL;
	if (ptr && *ptr)
	{
		if (*ptr == src)
			return *ptr;
		if (xalloc_size (*ptr) > strlen (src))
			return strcpy (*ptr, src);
		xfree (ptr);
	}
	*ptr = xmalloc (strlen (src) + 1);
	return strcpy (*ptr, src);
	return *ptr;
}

/* malloc_strcat: Yeah, right */
char *
malloc_strcat (char **ptr, const char *src)
{
	size_t msize;

	if (*ptr)
	{
		if (!src)
			return *ptr;
		msize = strlen (*ptr) + strlen (src) + 1;

		*ptr = xrealloc(*ptr, msize);
		return strcat (*ptr, src);
	}
	return (*ptr = m_strdup (src));
}

char *
malloc_str2cpy (char **ptr, const char *src1, const char *src2)
{
	if (!src1 && !src2)
		return xfree (ptr), NULL;

	if (*ptr)
	{
		if (xalloc_size (*ptr) > strlen (src1) + strlen (src2))
			return strcat (strcpy (*ptr, src1), src2);

		xfree (ptr);
	}

	*ptr = xmalloc (strlen (src1) + strlen (src2) + 1);
	return strcat (strcpy (*ptr, src1), src2);
}

char *
m_3dup (const char *str1, const char *str2, const char *str3)
{
	size_t msize = strlen (str1) + strlen (str2) + strlen (str3) + 1;
	return strcat (strcat (strcpy ((char *) xmalloc (msize), str1), str2), str3);
}

char *
m_opendup (const char *str1,...)
{
	va_list args;
	int size;
	char *this_arg = NULL;
	char *retval = NULL;

	size = strlen (str1);
	va_start (args, str1);
	while ((this_arg = va_arg (args, char *)))
		  size += strlen (this_arg);

	retval = (char *) xmalloc (size + 1);

	strcpy (retval, str1);
	va_start (args, str1);
	while ((this_arg = va_arg (args, char *)))
		  strcat (retval, this_arg);

	va_end (args);
	return retval;
}

char *
m_strdup (const char *str)
{
	char *ptr;

	if (!str)
		str = empty_string;
	ptr = (char *) xmalloc (strlen (str) + 1);
	return strcpy (ptr, str);
}

char *
m_s3cat (char **one, const char *maybe, const char *definitely)
{
	if (*one && **one)
		return m_3cat (one, maybe, definitely);
	return *one = m_strdup (definitely);
}

char *
m_s3cat_s (char **one, const char *maybe, const char *ifthere)
{
	if (ifthere && *ifthere)
		return m_3cat (one, maybe, ifthere);
	return *one;
}

char *
m_3cat (char **one, const char *two, const char *three)
{
	int len = 0;
	char *str;

	if (*one)
		len = strlen (*one);
	if (two)
		len += strlen (two);
	if (three)
		len += strlen (three);
	len += 1;

	str = (char *) xmalloc (len);
	if (*one)
		strcpy (str, *one);
	if (two)
		strcat (str, two);
	if (three)
		strcat (str, three);

	xfree (one);
	return ((*one = str));
}

char *
malloc_sprintf (char **to, const char *pattern,...)
{
	char booya[BIG_BUFFER_SIZE * 10 + 1];
	*booya = 0;

	if (pattern)
	{
		va_list args;
		va_start (args, pattern);
		vsnprintf (booya, BIG_BUFFER_SIZE * 10, pattern, args);
		va_end (args);
	}
	malloc_strcpy (to, booya);
	return *to;
}

/* same thing, different variation */
char *
m_sprintf (const char *pattern,...)
{
	char booya[BIG_BUFFER_SIZE * 10 + 1];
	*booya = 0;

	if (pattern)
	{
		va_list args;
		va_start (args, pattern);
		vsnprintf (booya, BIG_BUFFER_SIZE * 10, pattern, args);
		va_end (args);
	}
	return m_strdup (booya);
}

/* case insensitive string searching */
char *
stristr (char *source, char *search)
{
	int x = 0;

	if (!source || !*source || !search || !*search || strlen (source) < strlen (search))
		return NULL;

	while (*source)
	{
		if (source[x] && toupper (source[x]) == toupper (search[x]))
			x++;
		else if (search[x])
			source++, x = 0;
		else
			return source;
	}
	return NULL;
}

/* 
 * word_count:  Efficient way to find out how many words are in
 * a given string.  Relies on isspace() not being broken.
 */
extern int 
word_count (char *str)
{
	int cocs = 0;
	int isv = 1;
	register char *foo = str;

	if (!foo)
		return 0;

	while (*foo)
	{
		if (*foo == '"' && isv)
		{
			while (*(foo + 1) && *++foo != '"')
				;
			isv = 0;
			cocs++;
		}
		if (!my_isspace (*foo) != !isv)
		{
			isv = my_isspace (*foo);
			cocs++;
		}
		foo++;
	}
	return (cocs + 1) / 2;
}

char *
next_arg (char *str, char **new_ptr)
{
	char *ptr;

	/* added by Sheik (kilau@prairie.nodak.edu) -- sanity */
	if (!str || !*str)
		return NULL;

	if ((ptr = sindex (str, "^ ")) != NULL)
	{
		if ((str = sindex (ptr, " ")) != NULL)
			*str++ = (char) 0;
		else
			str = empty_string;
	}
	else
		str = empty_string;
	if (new_ptr)
		*new_ptr = str;
	return ptr;
}

extern char *
remove_trailing_spaces (char *foo)
{
	char *end;
	if (!*foo)
		return foo;

	end = foo + strlen (foo) - 1;
	while (my_isspace (*end))
		end--;
	end[1] = 0;
	return foo;
}

/*
 * yanks off the last word from 'src'
 * kinda the opposite of next_arg
 */
char *
last_arg (char **src)
{
	char *ptr;

	if (!src || !*src)
		return NULL;

	remove_trailing_spaces (*src);
	ptr = *src + strlen (*src) - 1;

	if (*ptr == '"')
	{
		for (ptr--;; ptr--)
		{
			if (*ptr == '"')
			{
				if (ptr == *src)
					break;
				if (ptr[-1] == ' ')
				{
					ptr--;
					break;
				}
			}
			if (ptr == *src)
				break;
		}
	}
	else
	{
		for (;; ptr--)
		{
			if (*ptr == ' ')
				break;
			if (ptr == *src)
				break;
		}
	}

	if (ptr == *src)
	{
		ptr = *src;
		*src = empty_string;
	}
	else
	{
		*ptr++ = 0;
		remove_trailing_spaces (*src);
	}
	return ptr;

}

char *
new_next_arg (char *str, char **new_ptr)
{
	char *ptr, *start;

	if (!str || !*str)
		return NULL;

	if ((ptr = sindex (str, "^ \t")) != NULL)
	{
		if (*ptr == '"')
		{
			start = ++ptr;
			while ((str = sindex (ptr, "\"\\")) != NULL)
			{
				switch (*str)
				{
				case '"':
					*str++ = '\0';
					if (*str == ' ')
						str++;
					if (new_ptr)
						*new_ptr = str;
					return (start);
				case '\\':
					if (*(str + 1) == '"')
						strcpy (str, str + 1);
					ptr = str + 1;
				}
			}
			str = empty_string;
		}
		else
		{
			if ((str = sindex (ptr, " \t")) != NULL)
				*str++ = '\0';
			else
				str = empty_string;
		}
	}
	else
		str = empty_string;
	if (new_ptr)
		*new_ptr = str;
	return ptr;
}


char *
new_new_next_arg (char *str, char **new_ptr, char *type)
{
	char *ptr, *start;

	if (!str || !*str)
		return NULL;

	if ((ptr = sindex (str, "^ \t")) != NULL)
	{
		if ((*ptr == '"') || (*ptr == '\''))
		{
			char blah[3];
			blah[0] = *ptr;
			blah[1] = '\\';
			blah[2] = '\0';

			*type = *ptr;
			start = ++ptr;
			while ((str = sindex (ptr, blah)) != NULL)
			{
				switch (*str)
				{
				case '\'':
				case '"':
					*str++ = '\0';
					if (*str == ' ')
						str++;
					if (new_ptr)
						*new_ptr = str;
					return (start);
				case '\\':
					if (str[1] == *type)
						strcpy (str, str + 1);
					ptr = str + 1;
				}
			}
			str = empty_string;
		}
		else
		{
			*type = '\"';
			if ((str = sindex (ptr, " \t")) != NULL)
				*str++ = '\0';
			else
				str = empty_string;
		}
	}
	else
		str = empty_string;
	if (new_ptr)
		*new_ptr = str;
	return ptr;
}

unsigned char stricmp_table[] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 94, 95,
	96, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 126, 127,

	128, 129, 130, 131, 132, 133, 134, 135,
	136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151,
	152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183,
	184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199,
	200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215,
	216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231,
	232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247,
	248, 249, 250, 251, 252, 253, 254, 255
};

/* my_stricmp: case insensitive version of strcmp */
int 
my_stricmp (register const unsigned char *str1, register const unsigned char *str2)
{
	while (*str1 && *str2 && (stricmp_table[(unsigned short) *str1] == stricmp_table[(unsigned short) *str2]))
		str1++, str2++;

	return (*str1 - *str2);
#if 0
	int xor;

	for (; *str1 || *str2; str1++, str2++)
	{
		if (!*str1 || !*str2)
			return (*str1 - *str2);
		if (isalpha (*str1) && isalpha (*str2))
		{
			xor = *str1 ^ *str2;
			if (xor != 32 && xor != 0)
				return (*str1 - *str2);
		}
		else
		{
			if (*str1 != *str2)
				return (*str1 - *str2);
		}
	}
	return 0;
#endif
}

/* my_strnicmp: case insensitive version of strncmp */
int 
my_strnicmp (register const unsigned char *str1, register const unsigned char *str2, register int n)
{
	while (n && *str1 && *str2 && (stricmp_table[(unsigned short) *str1] == stricmp_table[(unsigned short) *str2]))
		str1++, str2++, n--;

	return (n) ? (*str1 - *str2) : 0;
}


/* chop -- chops off the last character. capiche? */
char *
chop (char *stuff, int nchar)
{
	*(stuff + strlen (stuff) - nchar) = 0;
	return stuff;
}

/*
 * strext: Makes a copy of the string delmited by two char pointers and
 * returns it in malloced memory.  Useful when you dont want to munge up
 * the original string with a null.  end must be one place beyond where
 * you want to copy, ie, its the first character you dont want to copy.
 */
char *
strext (char *start, char *end)
{
	char *ptr, *retval;

	ptr = retval = (char *) xmalloc (end - start + 1);
	while (start < end)
		*ptr++ = *start++;
	*ptr = 0;
	return retval;
}


/*
 * strmcpy: Well, it's like this, strncpy doesn't append a trailing null if
 * strlen(str) == maxlen.  strmcpy always makes sure there is a trailing null 
 */
char *
strmcpy (char *dest, const char *src, int maxlen)
{
	strncpy (dest, src, maxlen);
	dest[maxlen] = '\0';
	return dest;
}

/*
 * strmcat: like strcat, but truncs the dest string to maxlen (thus the dest
 * should be able to handle maxlen+1 (for the null)) 
 */
char *
strmcat (char *dest, const char *src, int maxlen)
{
	int srclen, len;

	len = strlen (dest);
	srclen = strlen (src);
	if ((len + srclen) > maxlen)
		strncat (dest, src, maxlen - len);
	else
		strcat (dest, src);

	return dest;
}

/*
 * strmcat_ue: like strcat, but truncs the dest string to maxlen (thus the dest
 * should be able to handle maxlen + 1 (for the null)). Also unescapes
 * backslashes.
 */
char *
strmcat_ue (char *dest, const char *src, int maxlen)
{
	int dstlen;

	dstlen = strlen (dest);
	dest += dstlen;
	maxlen -= dstlen;
	while (*src && maxlen > 0)
	{
		if (*src == '\\')
		{
			if (strchr ("npr0", src[1]))
				*dest++ = '\020';
			else if (*(src + 1))
				*dest++ = *++src;
			else
				*dest++ = '\\';
		}
		else
			*dest++ = *src;
		src++;
	}
	*dest = '\0';
	return dest;
}

/*
 * m_strcat_ues: Given two strings, concatenate the 2nd string to
 * the end of the first one, but if the "unescape" argument is 1, do
 * unescaping (like in strmcat_ue).
 * (Malloc_STRCAT_UnEscape Special, in case you were wondering. ;-))
 *
 * This uses a cheating, "not-as-efficient-as-possible" algorithm,
 * but it works with negligible cpu lossage.
 */
char *
m_strcat_ues (char **dest, char *src, int unescape)
{
	int total_length;
	char *ptr, *ptr2;
	int z;

	if (!unescape)
	{
		malloc_strcat (dest, src);
		return *dest;
	}

	z = total_length = (*dest) ? strlen (*dest) : 0;
	total_length += strlen (src);

	*dest = xrealloc(*dest, total_length + 2);

	if (z == 0)
		**dest = 0;

	ptr2 = *dest + z;
	for (ptr = src; *ptr; ptr++)
	{
		if (*ptr == '\\')
		{
			switch (*++ptr)
			{
			case 'n':
			case 'p':
			case 'r':
			case '0':
				*ptr2++ = '\020';
				break;
			case (char) 0:
				*ptr2++ = '\\';
				goto end_strcat_ues;
				break;
			default:
				*ptr2++ = *ptr;
			}
		}
		else
			*ptr2++ = *ptr;
	}
      end_strcat_ues:
	*ptr2 = '\0';

	return *dest;
}


/*
 * scanstr: looks for an occurrence of str in source.  If not found, returns
 * 0.  If it is found, returns the position in source (1 being the first
 * position).  Not the best way to handle this, but what the hell 
 */
extern int 
scanstr (char *str, char *source)
{
	int i, max, len;

	len = strlen (str);
	max = strlen (source) - len;
	for (i = 0; i <= max; i++, source++)
	{
		if (!my_strnicmp (source, str, len))
			return (i + 1);
	}
	return (0);
}


/*
 * sindex: much like index(), but it looks for a match of any character in
 * the group, and returns that position.  If the first character is a ^, then
 * this will match the first occurence not in that group.
 */
char *
sindex (char *string, char *group)
{
	char *ptr;

	if (!string || !group)
		return (char *) NULL;
	if (*group == '^')
	{
		group++;
		for (; *string; string++)
		{
			for (ptr = group; *ptr; ptr++)
			{
				if (*ptr == *string)
					break;
			}
			if (*ptr == '\0')
				return string;
		}
	}
	else
	{
		for (; *string; string++)
		{
			for (ptr = group; *ptr; ptr++)
			{
				if (*ptr == *string)
					return string;
			}
		}
	}
	return (char *) NULL;
}

/*
 * rsindex: much like rindex(), but it looks for a match of any character in
 * the group, and returns that position.  If the first character is a ^, then
 * this will match the first occurence not in that group.
 */
char *
rsindex (char *string, char *start, char *group)
{
	char *ptr;
	char *retval = NULL;

	if (string && start && group && start <= string)
	{
		if (*group == '^')
		{
			group++;
			for (ptr = start; ptr <= string; ptr++)
			{
				if (!strchr (group, *ptr))
					retval = ptr;
			}
		}
		else
		{
			for (ptr = start; ptr <= string; ptr++)
			{
				if (strchr (group, *ptr))
					retval = ptr;
			}
		}
	}
	return retval;
}



/* rfgets: exactly like fgets, cept it works backwards through a file!  */
char *
rfgets (char *buffer, int size, FILE * file)
{
	char *ptr;
	off_t pos;

	if (fseek (file, -2L, SEEK_CUR))
		return NULL;
	do
	{
		switch (fgetc (file))
		{
		case EOF:
			return NULL;
		case '\n':
			pos = ftell (file);
			ptr = fgets (buffer, size, file);
			fseek (file, pos, 0);
			return ptr;
		}
	}
	while (fseek (file, -2L, SEEK_CUR) == 0);
	rewind (file);
	pos = 0L;
	ptr = fgets (buffer, size, file);
	fseek (file, pos, 0);
	return ptr;
}

/*
 * path_search: given a file called name, this will search each element of
 * the given path to locate the file.  If found in an element of path, the
 * full path name of the file is returned in a static string.  If not, null
 * is returned.  Path is a colon separated list of directories 
 */
char *
path_search (char *name, char *path)
{
	static char buffer[BIG_BUFFER_SIZE + 1];
	char *ptr, *free_path = NULL;

	/* A "relative" path is valid if the file exists */
	/* A "relative" path is searched in the path if the
	   filename doesnt really exist from where we are */
	if (strchr (name, '/'))
		if (!access (name, F_OK))
			return name;

	/* an absolute path is always checked, never searched */
	if (name[0] == '/')
		return (access (name, F_OK) ? (char *) 0 : name);

	/* This is cheating. >;-) */
	free_path = path = m_strdup (path);
	while (path)
	{
		if ((ptr = strchr (path, ':')) != NULL)
			*(ptr++) = '\0';
		strcpy (buffer, empty_string);
		if (path[0] == '~')
		{
			strmcat (buffer, my_path, BIG_BUFFER_SIZE);
			path++;
		}
		strmcat (buffer, path, BIG_BUFFER_SIZE);
		strmcat (buffer, "/", BIG_BUFFER_SIZE);
		strmcat (buffer, name, BIG_BUFFER_SIZE);

		if (access (buffer, F_OK) == 0)
			break;
		path = ptr;
	}
	xfree (&free_path);
	return (path != NULL) ? buffer : NULL;
}

/*
 * double_quote: Given a str of text, this will quote any character in the
 * set stuff with the QUOTE_CHAR. It returns a malloced quoted, null
 * terminated string 
 */
char *
double_quote (const char *str, const char *stuff, char *buffer)
{
	char c;
	int pos;

	*buffer = 0;
	if (!stuff)
		return buffer;
	for (pos = 0; (c = *str); str++)
	{
		if (strchr (stuff, c))
		{
			if (c == '$')
				buffer[pos++] = '$';
			else
				buffer[pos++] = '\\';
		}
		buffer[pos++] = c;
	}
	buffer[pos] = '\0';
	return buffer;
}

void
ircpanic (char *format,...)
{
	char buffer[10 * BIG_BUFFER_SIZE + 1];
	static int recursion = 0;

	if (recursion++)
		abort ();

	term_reset();

	if (format)
	{
		va_list arglist;
		va_start (arglist, format);
		vsnprintf (buffer, BIG_BUFFER_SIZE, format, arglist);
		va_end (arglist);
	}

	printf ("\n\n");

	printf ("An unrecoverable logic error has occured.\n");
	printf ("Please email laeos@ptw.com with the following message\n");

	printf ("\n");
	printf ("Panic: [%s] %s\n", XARIC_VersionStr, buffer);

	exit(0);

	irc_exit ("Xaric panic... Could it possibly be a bug?  Nahhhh...", NULL);
}

/* Not very complicated, but very handy function to have */
int 
end_strcmp (const char *one, const char *two, int bytes)
{
	if (bytes < strlen (one))
		return (strcmp (one + strlen (one) - (size_t) bytes, two));
	else
		return -1;
}

/* beep_em: Not hard to figure this one out */
void 
beep_em (int beeps)
{
	int cnt, i;

	for (cnt = beeps, i = 0; i < cnt; i++)
		term_beep ();
}



static FILE *
open_compression (char *executable, char *filename)
{
	FILE *file_pointer;
	int pipes[2];

	pipes[0] = -1;
	pipes[1] = -1;

	if (pipe (pipes) == -1)
	{
		bitchsay ("Cannot start decompression: %s\n", strerror (errno));
		if (pipes[0] != -1)
		{
			close (pipes[0]);
			close (pipes[1]);
		}
		return NULL;
	}

	switch (fork ())
	{
	case -1:
		{
			bitchsay ("Cannot start decompression: %s\n", strerror (errno));
			return NULL;
		}
	case 0:
		{
			dup2 (pipes[1], 1);
			close (pipes[0]);
			close (2);	/* we dont want to see errors */
			setuid (getuid ());
			setgid (getgid ());
#ifdef ZARGS
			execl (executable, executable, "-c", ZARGS, filename, NULL);
#else
			execl (executable, executable, "-c", filename, NULL);
#endif
			_exit (0);
		}
	default:
		{
			close (pipes[1]);
			if ((file_pointer = fdopen (pipes[0], "r")) == NULL)
			{
				bitchsay ("Cannot start decompression: %s\n", strerror (errno));
				return NULL;
			}
			break;
		}
	}
	return file_pointer;
}

/* Front end to fopen() that will open ANY file, compressed or not, and
 * is relatively smart about looking for the possibilities, and even
 * searches a path for you! ;-)
 */
FILE *
uzfopen (char **filename, char *path)
{
	static int setup = 0;
	int ok_to_decompress = 0;
	char *filename_path;
	char filename_trying[256];
	char *filename_blah;
	static char path_to_gunzip[513];
	static char path_to_uncompress[513];
	FILE *doh = NULL;

	if (setup == 0)
	{
		char *gzip = path_search ("gunzip", getenv ("PATH"));
		char *compress = path_search ("uncompress", getenv ("PATH"));
		if (gzip)
			strcpy (path_to_gunzip, path_search ("gunzip", getenv ("PATH")));
		if (compress)
			strcpy (path_to_uncompress, path_search ("uncompress", getenv ("PATH")));
		setup = 1;
	}

	/* It is allowed to pass to this function either a true filename
	   with the compression extention, or to pass it the base name of
	   the filename, and this will look to see if there is a compressed
	   file that matches the base name */

	/* Start with what we were given as an initial guess */
	/* kev asked me to call expand_twiddle here */
	filename_blah = expand_twiddle (*filename);

	strcpy (filename_trying, filename_blah ? filename_blah : *filename);

	xfree (&filename_blah);

	/* Look to see if the passed filename is a full compressed filename */
	if ((!end_strcmp (filename_trying, ".gz", 3)) ||
	    (!end_strcmp (filename_trying, ".z", 2)))
	{
		if (*path_to_gunzip)
		{
			ok_to_decompress = 1;
			filename_path = path_search (filename_trying, path);
		}
		else
		{
			bitchsay ("Cannot open file %s because gunzip was not found", filename_trying);
			xfree (filename);
			return NULL;
		}
	}
	else if (!end_strcmp (filename_trying, ".Z", 2))
	{
		if (*path_to_gunzip || *path_to_uncompress)
		{
			ok_to_decompress = 1;
			filename_path = path_search (filename_trying, path);
		}
		else
		{
			bitchsay ("Cannot open file %s becuase uncompress was not found", filename_trying);
			xfree (filename);
			return NULL;
		}
	}

	/* Right now it doesnt look like the file is a full compressed fn */
	else
	{
		struct stat file_info;

		/* Trivially, see if the file we were passed exists */
		filename_path = path_search (filename_trying, path);

		/* Nope. it doesnt exist. */
		if (!filename_path)
		{
			/* Is there a "filename.gz"? */
			strcpy (filename_trying, *filename);
			strcat (filename_trying, ".gz");
			filename_path = path_search (filename_trying, path);

			/* Nope. no "filename.gz" */
			if (!filename_path)
			{
				/* Is there a "filename.Z"? */
				strcpy (filename_trying, *filename);
				strcat (filename_trying, ".Z");
				filename_path = path_search (filename_trying, path);

				/* Nope. no "filename.Z" */
				if (!filename_path)
				{
					/* Is there a "filename.z"? */
					strcpy (filename_trying, *filename);
					strcat (filename_trying, ".z");
					filename_path = path_search (filename_trying, path);

					/* Nope.  No more guesses? then punt */
					if (!filename_path)
					{
						yell ("File not found: %s", *filename);
						xfree (filename);
						return NULL;
					}
					/* Yep. there's a "filename.z" */
					else
						ok_to_decompress = 2;
				}
				/* Yep. there's a "filename.Z" */
				else
					ok_to_decompress = 1;
			}
			/* Yep. There's a "filename.gz" */
			else
				ok_to_decompress = 2;
		}
		/* Imagine that! the file exists as-is (no decompression) */
		else
			ok_to_decompress = 0;

		stat (filename_path, &file_info);
		if (file_info.st_mode & S_IFDIR)
		{
			bitchsay ("%s is a directory", filename_trying);
			xfree (filename);
			return NULL;
		}
		if (file_info.st_mode & 0111)
		{
			bitchsay ("Cannot open %s -- executable file", filename_trying);
			xfree (filename);
			return NULL;
		}
	}

	malloc_strcpy (filename, filename_path);

	/* at this point, we should have a filename in the variable
	   filename_trying, and it should exist.  If ok_to_decompress
	   is one, then we can gunzip the file if guzip is available,
	   else we uncompress the file */
	if (ok_to_decompress)
	{
		if (*path_to_gunzip)
			return open_compression (path_to_gunzip, filename_path);
		else if ((ok_to_decompress == 1) && *path_to_uncompress)
			return open_compression (path_to_uncompress, filename_path);

		bitchsay ("Cannot open compressed file %s becuase no uncompressor was found", filename_trying);
		xfree (filename);
		return NULL;
	}

	/* Its not a compressed file... Try to open it regular-like. */
	if ((doh = fopen (filename_path, "r")) != NULL);
	return doh;

	/* nope.. we just cant seem to open this file... */
	bitchsay ("Cannot open file %s: %s", filename_path, strerror (errno));
	xfree (filename);
	return NULL;
}


/*
   From: carlson@Xylogics.COM (James Carlson)
   Newsgroups: comp.terminals
   Subject: Re: Need to skip VT100 codes, any help?
   Date: 1 Dec 1994 15:38:48 GMT
 */

/* This function returns 1 if the character passed is NOT printable, it
 * returns 0 if the character IS printable.  It doesnt actually do anything
 * with the character, though.
 */
int 
vt100_decode (register unsigned char chr)
{
	static enum
	{
		Normal, Escape, SCS, CSI, DCS, DCSData, DCSEscape
	}
	vtstate = Normal;

	if (chr == 0x1B)	/* ASCII ESC */
	{
		if (vtstate == DCSData || vtstate == DCSEscape)
			vtstate = DCSEscape;
		else
		{
			vtstate = Escape;
			return 1;
		}
	}
	else if (chr == 0x18 || chr == 0x1A)	/* ASCII CAN & SUB */
		vtstate = Normal;

	else if (chr == 0xE || chr == 0xF)	/* ASCII SO & SI */
		;

	/* C0 codes are dispatched without changing machine state!  Oh, my! */
	else if (chr < 0x20)
		return 0;

	switch (vtstate)
	{
	case Normal:
		return 0;
		break;
	case Escape:
		switch (chr)
		{
		case '[':
			vtstate = CSI;
			break;
		case 'P':
			vtstate = DCS;
			break;
		case '(':
		case ')':
			vtstate = SCS;
			break;
		default:
			vtstate = Normal;
		}
		return 1;
		break;
	case SCS:
		vtstate = Normal;
		break;
	case CSI:
		if (isalpha (chr))
			vtstate = Normal;
		break;
	case DCS:
		if (chr >= 0x40 && chr <= 0x7E)
			vtstate = DCSData;
		break;
	case DCSData:
		break;
	case DCSEscape:
		vtstate = Normal;
		break;
	}
	return 1;
}

/* Gets the time in second/usecond if you can,  second/0 if you cant. */
struct timeval 
get_time (struct timeval *timer)
{
	static struct timeval timer2;
#ifdef HAVE_GETTIMEOFDAY
	if (timer)
	{
		gettimeofday (timer, NULL);
		return *timer;
	}
	gettimeofday (&timer2, NULL);
	return timer2;
#else
	time_t time2 = time (NULL);

	if (timer)
	{
		timer.tv_sec = time2;
		timer.tv_usec = 0;
		return *timer;
	}
	timer2.tv_sec = time2;
	timer2.tv_usec = 0;
	return timer2;
#endif
}

/* 
 * calculates the time elapsed between 'one' and 'two' where they were
 * gotten probably with a call to get_time.  'one' should be the older
 * timer and 'two' should be the most recent timer.
 */
double 
time_diff (struct timeval one, struct timeval two)
{
	struct timeval td;

	td.tv_sec = two.tv_sec - one.tv_sec;
	td.tv_usec = two.tv_usec - one.tv_usec;

	return (double) td.tv_sec + ((double) td.tv_usec / 1000000.0);
}

int 
time_to_next_minute (void)
{
	time_t now = time (NULL);
	static int which = 0;

	if (which == 1)
		return 60 - now % 60;
	else
	{
		struct tm *now_tm = gmtime (&now);

		if (!which)
		{
			if (now_tm->tm_sec == now % 60)
				which = 1;
			else
				which = 2;
		}
		return 60 - now_tm->tm_sec;
	}
}

char *
plural (int number)
{
	return (number != 1) ? "s" : empty_string;
}

char *
my_ctime (time_t when)
{
	return chop (ctime (&when), 1);
}

/*
 * Formats "src" into "dest" using the given length.  If "length" is
 * negative, then the string is right-justified.  If "length" is
 * zero, nothing happens.  Sure, i cheat, but its cheaper then doing
 * two sprintf's.
 */
char *
strformat (char *dest, char *src, int length, char pad_char)
{
	char *ptr1 = dest, *ptr2 = src;
	int tmplen = length;
	int abslen;
	char padc;

	abslen = (length >= 0 ? length : -length);
	if (pad_char)
		padc = pad_char;
	else
		padc = (char) get_int_var (PAD_CHAR_VAR);
	if (!padc)
		padc = ' ';

	/* Cheat by spacing out 'dest' */
	for (tmplen = abslen - 1; tmplen >= 0; tmplen--)
		dest[tmplen] = padc;
	dest[abslen] = 0;

	/* Then cheat further by deciding where the string should go. */
	if (length > 0)		/* left justified */
	{
		while ((length-- > 0) && *ptr2)
			*ptr1++ = *ptr2++;
	}
	else if (length < 0)	/* right justified */
	{
		length = -length;
		ptr1 = dest;
		ptr2 = src;
		if (strlen (src) < length)
			ptr1 += length - strlen (src);
		while ((length-- > 0) && *ptr2)
			*ptr1++ = *ptr2++;
	}
	return dest;
}


/* MatchingBracket returns the next unescaped bracket of the given type */
extern char *
MatchingBracket (char *string, char left, char right)
{
	int bracket_count = 1;

	while (*string && bracket_count)
	{
		if (*string == left)
			bracket_count++;
		else if (*string == right)
		{
			if (!--bracket_count)
				return string;
		}
		else if (*string == '\\' && string[1])
			string++;
		string++;
	}
	return NULL;
}

/*
 * parse_number: returns the next number found in a string and moves the
 * string pointer beyond that point     in the string.  Here's some examples: 
 *
 * "123harhar"  returns 123 and str as "harhar" 
 *
 * while: 
 *
 * "hoohar"     returns -1  and str as "hoohar" 
 */
extern int 
parse_number (char **str)
{
	long ret;
	char *ptr = *str;	/* sigh */

	ret = strtol (ptr, str, 10);
	if (*str == ptr)
		ret = -1;

	return (int) ret;
}

extern int 
splitw (char *str, char ***to)
{
	int numwords = word_count (str);
	int counter;

	*to = (char **) xmalloc (sizeof (char *) * numwords);
	for (counter = 0; counter < numwords; counter++)
		(*to)[counter] = new_next_arg (str, &str);

	return numwords;
}

extern char *
unsplitw (char **str, int howmany)
{
	char *retval = NULL;

	while (howmany)
	{
		m_s3cat (&retval, " ", *str);
		str++, howmany--;
	}

	return retval;
}

char *
m_2dup (const char *str1, const char *str2)
{
	size_t msize = strlen (str1) + strlen (str2) + 1;
	return strcat (strcpy ((char *) xmalloc (msize), str1), str2);
}

char *
on_off (int var)
{
	if (var)
		return ("On");
	return ("Off");
}

extern char *
strfill (char c, int num)
{
	static char buffer[BIG_BUFFER_SIZE + 1];
	int i = 0;
	if (num > BIG_BUFFER_SIZE)
		num = BIG_BUFFER_SIZE;
	for (i = 0; i < num; i++)
		buffer[i] = c;
	buffer[num] = '\0';
	return buffer;
}

extern int 
empty (const char *str)
{
	while (str && *str && *str == ' ')
		str++;

	if (str && *str)
		return 0;

	return 1;
}

/* makes foo[one][two] look like tmp.one.two -- got it? */
char *
remove_brackets (char *name, char *args, int *arg_flag)
{
	char *ptr, *right, *result1, *rptr, *retval = NULL;

	/* XXXX - ugh. */
	rptr = m_strdup (name);

	while ((ptr = strchr (rptr, '[')))
	{
		*ptr++ = 0;
		right = ptr;
		if ((ptr = MatchingBracket (right, '[', ']')))
			*ptr++ = 0;

		if (args)
			result1 = expand_alias (right, args, arg_flag, NULL);
		else
			result1 = right;

		retval = m_3dup (rptr, ".", result1);
		if (ptr)
			malloc_strcat (&retval, ptr);

		if (args)
			xfree (&result1);
		if (rptr)
			xfree (&rptr);
		rptr = retval;
	}
	return upper (rptr);
}

long 
my_atol (char *str)
{
	if (str)
		return (long) strtol (str, NULL, 0);
	else
		return 0L;
}

char *
m_dupchar (int i)
{
	char c = (char) i;	/* blah */
	char *ret = (char *) xmalloc (2);

	ret[0] = c;
	ret[1] = 0;
	return ret;
}

char *
strmopencat (char *dest, int maxlen,...)
{
	va_list args;
	int size;
	char *this_arg = NULL;
	int this_len;

	size = strlen (dest);
	va_start (args, maxlen);

	while (size < maxlen)
	{
		if (!(this_arg = va_arg (args, char *)))
			  break;

		if (size + ((this_len = strlen (this_arg))) > maxlen)
			strncat (dest, this_arg, maxlen - size);
		else
			strcat (dest, this_arg);

		size += this_len;
	}

	va_end (args);
	return dest;
}

char *
next_in_comma_list (char *str, char **after)
{
	*after = str;

	while (*after && **after && **after != ',')
		(*after)++;

	if (*after && **after == ',')
	{
		**after = 0;
		(*after)++;
	}

	return str;
}

/* 
 * Checks if ansi string only sets colors/attributes
 * ^[[m won't work anymore !!!!
 */
void 
FixColorAnsi (unsigned char *str)
{
	register unsigned char *tmpstr;
	register unsigned char *tmpstr1 = NULL;
	int what = 0;
	int numbers = 0;
	int val = 0;

	tmpstr = str;
	while (*tmpstr)
	{
		if ((*tmpstr >= '0' && *tmpstr <= '9'))
		{
			numbers = 1;
			val = val * 10 + (*tmpstr - '0');
		}
		else if (*tmpstr == ';')
			numbers = 1, val = 0;
		else if (!(*tmpstr == 'm' || *tmpstr == 'C'))
			numbers = val = 0;
		if (*tmpstr == 0x1B)
		{
			if (what && tmpstr1)
				*tmpstr1 += 64;
			what = 1;
			tmpstr1 = tmpstr;
		}
		else if (*tmpstr == 0x18 || *tmpstr == 0x0E)
			*tmpstr += 64;
		if (what && numbers && (val > 130))
		{
			what = numbers = val = 0;
			*tmpstr1 += 64;
		}
		if (what && *tmpstr == 'm')
		{
			if (!numbers || (val == 12))
			{
				*tmpstr1 += 64;
				tmpstr1 = tmpstr;
			}
			what = 0;
			numbers = val = 0;
		}
		else if (what && *tmpstr == 'C')
		{
			if (!numbers)
			{
				*tmpstr1 += 64;
				tmpstr1 = tmpstr;
			}
			what = 0;
			numbers = val = 0;
		}
		else if (what && *tmpstr == 'J')
		{
			val = numbers = 0;
			*tmpstr1 += 64;
			tmpstr1 = tmpstr;
			what = 0;
		}
		else if (what && *tmpstr == '(')
			what = 2;
		else if (what == 2 && (*tmpstr == 'U' || *tmpstr == 'B'))
			what = 0;
		tmpstr++;
	}
	if (what && tmpstr1 && *tmpstr1)
		*tmpstr1 += 64;
}

/* how many ansi escape chars do we have in string?
   if len -1 count full string, otherwise, count len chars */

int 
count_ansi (register const char *str, const int len)
{
	register int count = 0;
	register int c = len;
/*      FixColorAnsi(str); */
	for (; *str && c; str++, c--)
		if (vt100_decode (*str))
			count++;
	return count;
}
