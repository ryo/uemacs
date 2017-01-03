/*
 * $Id: char.c,v 1.10 2009/02/27 17:18:43 ryo Exp $
 *
 * char.c: Character handling functions for MicroEMACS 3.10
 * (C)opyright 1988 by Daniel Lawrence
 *
 * ALL THE CODE HERE IS FOR VARIOUS FORMS OF ASCII
 * AND WILL HAVE TO BE MODIFIED FOR EBCDIC
 */

#include <stdio.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"

/* change *cp to an upper case character */
void
uppercase(char *cp)
{
	*cp = toupper(*(unsigned char *)cp);
}

/* change *cp to an lower case character */
void
lowercase(char *cp)
{
	*cp = tolower(*(unsigned char *)cp);
}

/* return the upper case equivalant of a character */
char
upperc(char ch)
{
	return toupper((unsigned int)ch);
}

/* return the lower case equivalant of a character */
char
lowerc(char ch)
{
	return tolower((unsigned int)ch);
}

