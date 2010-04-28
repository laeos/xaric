/*
 * words.c -- right now it just holds the stuff i wrote to replace
 * that beastie arg_number().  Eventually, i may move all of the
 * word functions out of ircaux and into here.  Now wouldnt that
 * be a beastie of a patch! Beastie! Beastie!
 *
 * Oh yea.  This file is beastierighted (C) 1994 by the beastie author.
 * Right now the only author is Jeremy "Beastie" Nelson.  See the
 * beastieright file for beastie info.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc.h"
#include "ircaux.h"

/* Move to an absolute word number from start */
/* First word is always numbered zero. */
static char *move_to_abs_word(char *start, char **mark, int word)
{
    char *pointer = start;
    int counter = word;

    /* This fixes a bug that counted leading spaces as a word, when theyre really not a word.... (found by Genesis K.) The stock
     * client strips leading spaces on both the cases $0 and $-0.  I personally think this is not the best choice, but im not going to 
     * stick my foot in this one... im just going to go with what the stock client does... */
    while (pointer && *pointer && my_isspace(*pointer))
	pointer++;

    for (; counter > 0 && *pointer; counter--) {
	while (*pointer && !my_isspace(*pointer))
	    pointer++;
	while (*pointer && my_isspace(*pointer))
	    pointer++;
    }

    if (mark)
	*mark = pointer;
    return pointer;
}

/* Move a relative number of words from the present mark */
static char *move_word_rel(char *start, char **mark, int word)
{
    char *pointer = *mark;
    int counter = word;
    char *end = start + strlen(start);

    if (end == start)		/* null string, return it */
	return start;

    /* 
     * XXXX - this is utterly pointless at best, and
     * totaly wrong at worst.
     */

    if (counter > 0) {
	for (; counter > 0 && pointer; counter--) {
	    while (*pointer && !my_isspace(*pointer))
		pointer++;
	    while (*pointer && my_isspace(*pointer))
		pointer++;
	}
    } else if (counter == 0)
	pointer = *mark;
    else {
	for (; counter < 0 && pointer > start; counter++) {
	    while (pointer >= start && my_isspace(*pointer))
		pointer--;
	    while (pointer >= start && !my_isspace(*pointer))
		pointer--;
	}
	pointer++;		/* bump up to the word we just passed */
    }

    if (mark)
	*mark = pointer;
    return pointer;
}

/*
 * extract2 is the word extractor that is used when its important to us
 * that 'firstword' get special treatment if it is negative (specifically,
 * that it refer to the "firstword"th word from the END).  This is used
 * basically by the ${n}{-m} expandos and by function_rightw(). 
 *
 * Note that because of a lot of flak, if you do an expando that is
 * a "range" of words, unless you #define STRIP_EXTRANEOUS_SPACES,
 * the "n"th word will be backed up to the first character after the
 * first space after the "n-1"th word.  That apparantly is what everyone
 * wants, so thats whatll be the default.  Those of us who may not like
 * that behavior or are at ambivelent can just #define it.
 */
char *extract_words(char *start, int firstword, int lastword)
{
    /* If firstword or lastword is negative, then we take those values from the end of the string */
    char *mark;
    char *mark2;
    char *booya = NULL;

    /* If firstword is EOS, then the user wants the last word */
    if (firstword == EOS) {
	mark = start + strlen(start);
	mark = move_word_rel(start, &mark, -1);
	return m_strdup(mark);
    }

    /* SOS is used when the user does $-n, all leading spaces are retained */
    else if (firstword == SOS)
	mark = start;

    /* If the firstword is positive, move to that word */
    else if (firstword >= 0) {
	move_to_abs_word(start, &mark, firstword);
	if (!*mark)
	    return m_strdup(empty_str);
    }
    /* Otherwise, move to the firstwords from the end */
    else {
	mark = start + strlen(start);
	move_word_rel(start, &mark, firstword);
    }

    /* 
     * When we find the last word, we need to move to the 
     * END of the word, so that word 3 to 3, would include
     * all of word 3, so we sindex to the space after the word
     */
    if (lastword == EOS)
	mark2 = mark + strlen(mark);

    else {
	if (lastword >= 0)
	    move_to_abs_word(start, &mark2, lastword + 1);
	else {
	    mark2 = start + strlen(start);
	    move_word_rel(start, &mark2, lastword);
	}

	while (mark2 > start && my_isspace(mark2[-1]))
	    mark2--;
    }

    /* 
     * If the end is before the string, then there is nothing
     * to extract (this is perfectly legal, btw)
     */
    if (mark2 < mark)
	booya = m_strdup(empty_str);

    else {
	/* Otherwise, copy off the string we just isolated */
	char tmp;

	tmp = *mark2;
	*mark2 = '\0';
	booya = m_strdup(mark);
	*mark2 = tmp;
    }

    return booya;
}
