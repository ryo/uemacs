/*
 * $Id: random.c,v 1.45 2017/01/02 18:14:48 ryo Exp $
 *
 * This file contains the command processing functions for a number of random
 * commands. There is no functional grouping here, for sure.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "kanji.h"

/*
 * Set fill column to n.
 */
int
setfillcol(int f, int n)
{
	if (n <= 1)
		n = getccol(FALSE);
	if (n <= 0)
		n = 80;

	fillcol = n;
	mlwrite(TEXT59, n);
	/* "[Fill column is %d]" */
	return TRUE;
}

/*
 * Display the current position of the cursor, in origin 1 X-Y coordinates,
 * the character that is under the cursor (in hex), and the fraction of the
 * text that is before the cursor. The displayed column is not the current
 * column, but the column that would be used on an infinite width display.
 * Normally this is bound to "C-X =".
 */
int
showcpos(int f, int n)
{
	LINE *lp;		/* current line */
	long numchars;		/* # of chars in file */
	int numlines;		/* # of lines in file */
	long predchars;		/* # chars preceding point */
	int predlines;		/* # lines preceding point */
	int curchar;		/* character under cursor */
	int ratio;
	int col;
	int savepos;		/* temp save for current offset */
	int ecol;		/* column pos/end of current line */
	int multibyte;

	(void)&predchars;	/* stupid gcc */
	(void)&predlines;	/* stupid gcc */

	/* starting at the beginning of the buffer */
	lp = lforw(curbp->b_linep);
	curchar = '\n';

	/* start counting chars and lines */
	numchars = 0;
	numlines = 0;
	while (lp != curbp->b_linep) {
		/* if we are on the current line, record it */
		if (lp == curwp->w_dotp) {
			predlines = numlines;
			predchars = numchars + curwp->w_doto;
			if ((curwp->w_doto) == llength(lp)) {
				curchar = '\n';
				multibyte = 0;
			} else {
				switch (nthctype(lp->l_text, curwp->w_doto)) {
				case CT_KJ1:
				case CT_KJ2:
					curchar = lgetc(lp, curwp->w_doto) * 256 +
					          lgetc(lp, curwp->w_doto+1);
					multibyte = 1;
					break;
				default:
					curchar = lgetc(lp, curwp->w_doto);
					if (chkana(curchar))
						multibyte = 1;
					else
						multibyte = 0;
					break;
				}
			}
		}
		/* on to the next line */
		++numlines;
		numchars += llength(lp) + 1;
		lp = lforw(lp);
	}

	/* if at end of file, record it */
	if (curwp->w_dotp == curbp->b_linep) {
		predlines = numlines;
		predchars = numchars;
	}
	/* Get real column and end-of-line column. */
	col = getccol(FALSE);
	savepos = curwp->w_doto;
	curwp->w_doto = llength(curwp->w_dotp);
	ecol = getccol(FALSE);
	curwp->w_doto = savepos;

	ratio = 0;		/* Ratio before dot. */
	if (numchars != 0)
		ratio = (100L * predchars) / numchars;

	/* summarize and report the info */
	if (multibyte) {
		mlwrite(TEXT60a,
		/* "Line %d/%d Col %d/%d Char %D/%D (%d%%) code = S:0x%04x", E:0x%04x", J:0x%04x" */
			predlines + 1, numlines + 1, col, ecol,
			predchars, numchars, ratio,
			curchar,
			sjis2euc(curchar),
			sjis2euc(curchar) & 0x7f7f);
	} else {
		mlwrite(TEXT60,
		/* "Line %d/%d Col %d/%d Char %D/%D (%d%%) code = 0x%02x" */
			predlines + 1, numlines + 1, col, ecol,
			predchars, numchars, ratio, curchar);
	}
	return TRUE;
}

int
getcline(void)
{				/* get the current line number */
	LINE *lp;		/* current line */
	int numlines;		/* # of lines before point */

	/* starting at the beginning of the buffer */
	lp = lforw(curbp->b_linep);

	/* start counting lines */
	numlines = 0;
	while (lp != curbp->b_linep) {
		/* if we are on the current line, record it */
		if (lp == curwp->w_dotp)
			break;
		++numlines;
		lp = lforw(lp);
	}

	/* and return the resulting count */
	return numlines + 1;
}

/*
 * Return current column.  Stop at first non-blank given TRUE argument.
 */
int
getccol(int bflg)
{
	int c, i, col;
	col = 0;
	for (i = 0; i < curwp->w_doto; ++i) {
		c = lgetc(curwp->w_dotp, i);
		if (c != ' ' && c != '\t' && bflg)
			break;
		if (c == '\t')
			col += -(col % curwp->w_bufp->b_tabs) + (curwp->w_bufp->b_tabs - 1);
		else
			if (c < 0x20 || c == 0x7F)
				++col;
		++col;
	}
	return col;
}

/*
 * Set current column.
 */
int
setccol(int pos)	/* position to set cursor */
{
	int c;			/* character being scanned */
	int i;			/* index into current line */
	int col;		/* current cursor column   */
	int llen;		/* length of line in bytes */

	col = 0;
	llen = llength(curwp->w_dotp);

	/* scan the line until we are at or past the target column */
	for (i = 0; i < llen; ++i) {
		/* upon reaching the target, drop out */
		if (col >= pos)
			break;

		/* advance one character */
		c = lgetc(curwp->w_dotp, i);
		if (c == '\t')
			col += -(col % curwp->w_bufp->b_tabs) + (curwp->w_bufp->b_tabs - 1);
		else
			if (c < 0x20 || c == 0x7F)
				++col;
		++col;
	}

	/* set us at the new position */
	curwp->w_doto = i;

	/* and tell weather we made it */
	return col >= pos;
}

/*
 * Twiddle the two characters on either side of dot. If dot is at the end of
 * the line twiddle the two characters before it. Return with an error if dot
 * is at the beginning of line; it seems to be a bit pointless to make this
 * work. This fixes up a very common typo with a single stroke. Normally bound
 * to "C-T". This always works within a line, so "WFEDIT" is good enough.
 */
int
twiddle(int f, int n)
{
	LINE *dotp;
	int doto;
	int doto_l,doto_r;
	unsigned char ch;

	if (!checkmodify())
		return FALSE;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;

	if (doto == llength(dotp) && --doto < 0)
		return FALSE;

	if ((nthctype(dotp->l_text, doto) == CT_KJ2) && (--doto < 0))
		return FALSE;
	doto_r = doto;

	if (--doto < 0)
		return FALSE;

	if ((nthctype(dotp->l_text, doto) == CT_KJ2) && (--doto < 0))
		return FALSE;
	doto_l = doto;


	if (nthctype(dotp->l_text, doto_l) == CT_KJ1) {
		if (nthctype(dotp->l_text, doto_r) == CT_KJ1) {
			/* MB:MB -> MB:MB */
			ch = lgetc(dotp, doto_l);
			lputc(dotp, doto_l, lgetc(dotp,doto_r));
			lputc(dotp, doto_r, ch);

			ch = lgetc(dotp, doto_l+1);
			lputc(dotp, doto_l+1, lgetc(dotp,doto_r+1));
			lputc(dotp, doto_r+1, ch);
		} else {
			/* MB:A -> A:MB */

			ch = lgetc(dotp, doto_r);
			lputc(dotp, doto_r, lgetc(dotp,doto_l+1));
			lputc(dotp, doto_l+1, lgetc(dotp,doto_l));
			lputc(dotp, doto_l, ch);
			curwp->w_doto--;
		}
	} else {
		if (nthctype(dotp->l_text, doto_r) == CT_KJ1) {
			/* A:MB -> MB:A */

			ch = lgetc(dotp, doto_l);
			lputc(dotp, doto_l, lgetc(dotp,doto_r));
			lputc(dotp, doto_r, lgetc(dotp,doto_r+1));
			lputc(dotp, doto_r+1, ch);
			curwp->w_doto++;
		} else {
			/* A:A -> A:A */

			ch = lgetc(dotp, doto_l);
			lputc(dotp, doto_l, lgetc(dotp,doto_r));
			lputc(dotp, doto_r, ch);
		}
	}

	lchange(WFEDIT);
	return TRUE;
}

/*
 * Quote the next character, and insert it into the buffer. All the characters
 * are taken literally, including the newline, which does not then have
 * its line splitting meaning. The character is always read, even if it is
 * inserted 0 times, for regularity. Bound to "C-Q"
 */

int
quote(int f, int n)
{
	int c;

	if (!checkmodify())
		return FALSE;

	c = tgetc();
	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;
	return linsert(n, c);
}

/*
 * Set tab size if given non-default argument (n <> 1).  Otherwise, insert a
 * tab into file.  If given argument, n, of zero, change to hard tabs.
 * If n > 1, simulate tab stop every n-characters using spaces. This has to be
 * done in this slightly funny way because the tab (in ASCII) has been turned
 * into "C-I" (in 10 bit code) already. Bound to "C-I".
 */
int
tab(int f, int n)
{
	if (n < 0)
		return FALSE;
	if (n == 0 || n > 1) {
		stabsize = n;
		return TRUE;
	}
	if (!stabsize)
		return linsert(1, '\t');
	return linsert(stabsize - (getccol(FALSE) % stabsize), ' ');
}

#if AEDIT
/* change tabs to spaces */
int
detab(int f, int n)
{
	int inc;		/* increment to next line [sgn(n)] */

	if (!checkmodify())
		return FALSE;

	if (f == FALSE)
		n = reglines();

	/* loop thru detabbing n lines */
	inc = ((n > 0) ? 1 : -1);
	while (n) {
		curwp->w_doto = 0;	/* start at the beginning */

		/* detab the entire current line */
		while (curwp->w_doto < llength(curwp->w_dotp)) {
			/* if we have a tab */
			if (lgetc(curwp->w_dotp, curwp->w_doto) == '\t') {
				ldelete(1L, FALSE);
				/* insspace(TRUE, 8 - (curwp->w_doto & 7)); */
				insspace(TRUE, curwp->w_bufp->b_tabs - (curwp->w_doto % curwp->w_bufp->b_tabs));
			}
			forwchar(FALSE, 1);
		}

		/* advance/or back to the next line */
		forwline(TRUE, inc);
		n -= inc;
	}
	curwp->w_doto = 0;	/* to the begining of the line */
	thisflag &= ~CFCPCN;	/* flag that this resets the goal column */
	lchange(WFEDIT);	/* yes, we have made at least an edit */
	return TRUE;
}

/* change spaces to tabs where posible */
int
entab(int f, int n)
{
	int inc;		/* increment to next line [sgn(n)] */
	int ccol;		/* current cursor column */
	char cchar;		/* current character */
	int nspace;

	if (!checkmodify())
		return FALSE;

	if (f == FALSE)
		n = reglines();

	/* loop thru entabbing n lines */
	inc = ((n > 0) ? 1 : -1);
	while (n) {
		/* detab the entire current line */
		curwp->w_doto = 0;	/* start at the beginning */
		while (curwp->w_doto < llength(curwp->w_dotp)) {
			/* if we have a tab */
			if (lgetc(curwp->w_dotp, curwp->w_doto) == '\t') {
				ldelete(1L, FALSE);
				/* insspace(TRUE, 8 - (curwp->w_doto & 7)); */
				insspace(TRUE, curwp->w_bufp->b_tabs - (curwp->w_doto % curwp->w_bufp->b_tabs));
			}
			forwchar(FALSE, 1);
		}

		/* now, entab the resulting spaced line */
		curwp->w_doto = 0;	/* start at the beginning */

		/* entab the entire current line */
		ccol = 0;	/* current column */
		nspace = 0;	/* continuous number of space */
		while (curwp->w_doto < llength(curwp->w_dotp)) {
			/* get the current character */
			cchar = lgetc(curwp->w_dotp, curwp->w_doto);
			switch (cchar) {
			case '\t':	/* a tab...count em up */
				ccol = nextab(ccol, curwp->w_bufp->b_tabs);
				nspace = 0;
				break;
			case ' ':	/* a space...compress? */
				ccol++;
				nspace++;
				break;
			default:	/* any other char...just count */
				ccol++;
				nspace = 0;
				break;
			}
			forwchar(FALSE, 1);

			if (ccol == ((ccol + curwp->w_bufp->b_tabs - 1) /
			    curwp->w_bufp->b_tabs * curwp->w_bufp->b_tabs)) {

				if (nspace > 1) {
					backchar(TRUE, nspace);
					ldelete(nspace, FALSE);
					linsert(1, '\t');
				}
				nspace = 0;
			}
		}

		/* 2nd, continuous <SPC>[<SPC>...] + <TAB> replace <TAB> + <TAB> */
		curwp->w_doto = 0;	/* start at the beginning */
		ccol = 0;	/* current column */
		nspace = 0;	/* continuous number of space */
		while (curwp->w_doto < llength(curwp->w_dotp)) {
			cchar = lgetc(curwp->w_dotp, curwp->w_doto);
			switch (cchar) {
			case '\t':
				if (nspace > 0) {
					backchar(TRUE, nspace);
					ldelete(nspace, FALSE);
					linsert(1, '\t');
				}
				ccol = nextab(ccol, curwp->w_bufp->b_tabs);
				nspace = 0;
				break;
			case ' ':
				ccol++;
				nspace++;
				break;
			default:
				ccol++;
				nspace = 0;
				break;
			}
			forwchar(FALSE, 1);
		}


		/* advance/or back to the next line */
		forwline(TRUE, inc);
		n -= inc;
	}
	curwp->w_doto = 0;	/* to the begining of the line */
	thisflag &= ~CFCPCN;	/* flag that this resets the goal column */
	lchange(WFEDIT);	/* yes, we have made at least an edit */
	return TRUE;
}

/*
 * trim: trim trailing whitespace from the point to eol with no
 * arguments, it trims the current region
 */
int
trim(int f, int n)
{
	LINE *lp;	/* current line pointer */
	int offset;	/* original line offset position */
	int length;	/* current length */
	int inc;	/* increment to next line [sgn(n)] */

	if (!checkmodify())
		return FALSE;

	if (f == FALSE)
		n = reglines();

	/* loop thru trimming n lines */
	inc = ((n > 0) ? 1 : -1);
	while (n) {
		lp = curwp->w_dotp;	/* find current line text */
		offset = curwp->w_doto;	/* save original offset */
		length = lp->l_used;	/* find current length */

		/* trim the current line */
		while (length > offset) {
			if (lgetc(lp, length - 1) != ' ' &&
			    lgetc(lp, length - 1) != '\t')
				break;
			length--;
		}
		lp->l_used = length;

		/* advance/or back to the next line */
		forwline(TRUE, inc);
		n -= inc;
	}
	lchange(WFEDIT);
	thisflag &= ~CFCPCN;	/* flag that this resets the goal column */
	return TRUE;
}
#endif

/*
 * Open up some blank space. The basic plan is to insert a bunch of newlines,
 * and then back up over them. Everything is done by the subcommand
 * procerssors. They even handle the looping. Normally this is bound to "C-O".
 */
int
openline(int f, int n)
{
	int i;
	int s;

	if (!checkmodify())
		return FALSE;

	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;
	i = n;			/* Insert newlines.     */
	do {
		s = lnewline();
	} while (s == TRUE && --i);
	if (s == TRUE)		/* Then back up overtop */
		s = backchar(f, n);	/* of them all.         */
	return s;
}

/*
 * Insert a newline. Bound to "C-M". If we are in CMODE, do automatic
 * indentation as specified.
 */
int
newline(int f, int n)
{
	int s;

	if (!checkmodify())
		return FALSE;

	if (n < 0)
		return FALSE;

	/* if we are in C mode and this is a default <NL> */
	if (n == 1 && (curbp->b_mode & MDCMOD) &&
	    curwp->w_dotp != curbp->b_linep)
		return cinsert();

	/*
	 * If a newline was typed, fill column is defined, the argument is non-
	 * negative, wrap mode is enabled, and we are now past fill column,
	 * and we are not read-only, perform word wrap.
	 */
	if ((curwp->w_bufp->b_mode & MDWRAP) && fillcol > 0 &&
	    getccol(FALSE) > fillcol &&
	    (curwp->w_bufp->b_mode & MDVIEW) == FALSE)
		exechook(wraphook);

	/* insert some lines */
	while (n--) {
		if ((s = lnewline()) != TRUE)
			return s;
	}
	return TRUE;
}

/* insert a newline and indentation for C */
int
cinsert(void)
{
	char *cptr;		/* string pointer into text to copy */
	int i;			/* index into line to copy indent from */
	int llen;		/* length of line to copy indent from */
	int bracef;		/* was there a brace at the end of line? */
	LINE *lp;		/* current line pointer */
	int offset;
	char ichar[NSTRING];	/* buffer to hold indent of last line */

	/* trim the whitespace before the point */
	lp = curwp->w_dotp;
	offset = curwp->w_doto;
	while ((offset > 0) &&
	       ((lgetc(lp, offset - 1) == ' ') || (lgetc(lp, offset - 1) == '\t'))) {
		backdel(FALSE, 1);
		offset--;
	}

	/* check for a brace */
	bracef = (offset > 0 && lgetc(lp, offset - 1) == '{');

	/* put in the newline */
	if (lnewline() == FALSE)
		return FALSE;

	/* if the new line is not blank... don't indent it! */
	lp = curwp->w_dotp;
	if (lp->l_used != 0)
		return TRUE;

	/* hunt for the last non-blank line to get indentation from */
	while (lp->l_used == 0 && lp != curbp->b_linep)
		lp = lp->l_bp;

	/* check for a brace again */
	if (!bracef)
		bracef = (llength(lp) > 0 && lgetc(lp, llength(lp) - 1) == '{');

	/* grab a pointer to text to copy indentation from */
	cptr = (char *)&(lp->l_text[0]);
	llen = lp->l_used;

	/* save the indent of the last non blank line */
	i = 0;
#if 1
	while ((i < llen) && (cptr[i] == ' ' || cptr[i] == '\t')
#else
	while ((i < llen) && (cptr[i] == '\t')
#endif
	       && (i < NSTRING - 1)) {
		ichar[i] = cptr[i];
		++i;
	}
	ichar[i] = 0;		/* terminate it */

	/* insert this saved indentation */
	linstr(ichar);

	/* and one more tab for a brace */
	if (bracef)
		tab(FALSE, 1);

	return TRUE;
}


int
insbrace(int n, int c)
{
	WINDOW *wp = curwp;

	if (wp->w_doto != 0) {
		int i;
		for (i = wp->w_doto - 1; i >= 0; --i) {
			int ch;

			ch = lgetc(wp->w_dotp, i);
			if (ch == 0x40 && lgetc(wp->w_dotp, i - 1) == 0x81)
				i--;
			else if (ch != ' ' && ch != TAB)
				return linsert(n, c);
		}
	}
	{
		int oldoff = wp->w_doto;
		LINE *oldlp = wp->w_dotp;

		{
			int oc;

			switch (c) {
			case '}':
				oc = '{';
				break;
			case ']':
				oc = '[';
				break;
			case ')':
				oc = '(';
				break;
			default:
				return FALSE;
			}

			{
				int count;

				oldlp = wp->w_dotp;
				oldoff = wp->w_doto;
				count = 1;

				while (count > 0) {
					int ch;

					if (backchar(FALSE, 1) == FALSE)
						break;
					ch = lgetc2(wp->w_dotp, wp->w_doto);
					if (ch == c)
						count++;
					else if (ch == oc)
						count--;
				}
				if (count != 0) {
					wp->w_dotp = oldlp;
					wp->w_doto = oldoff;
					return linsert(n, c);
				}
			}
		}

		{
			int target;
			int ch;

			wp->w_doto = 0;
			while (1) {
				ch = lgetc(wp->w_dotp, wp->w_doto);
#if 0
				if (ch != ' ' && ch != TAB)
#else
				if (ch != TAB)
#endif
					break;
				forwchar(FALSE, 1);
			}

			target = getccol(FALSE);
			wp->w_dotp = oldlp;
			wp->w_doto = oldoff;

			while (target != getccol(FALSE)) {
				if (target < getccol(FALSE)) {
					while (getccol(FALSE) > target)
						backdel(FALSE, 1);
				} else {
					while (target - getccol(FALSE) >= curwp->w_bufp->b_tabs &&
					       target != getccol(FALSE)) {
						linsert(1, TAB);
					}
					linsert(target - getccol(FALSE), ' ');
				}
			}
		}
	}

	return linsert(n, c);
}


/* insert a # into the text here...we are in CMODE */
int
inspound(void)
{
	int ch;			/* last character before input */
	int i;

	/* if we are at the beginning of the line, no go */
	if (curwp->w_doto == 0)
		return linsert(1, '#');

	/* scan to see if all space before this is white space */
	for (i = curwp->w_doto - 1; i >= 0; --i) {
		ch = lgetc(curwp->w_dotp, i);
		if (ch != ' ' && ch != '\t')
			return linsert(1, '#');
	}

	/* delete back first */
	while (getccol(FALSE) >= 1)
		backdel(FALSE, 1);

	/* and insert the required pound */
	return linsert(1, '#');
}

/*
 * Delete blank lines around dot. What this command does depends if dot is
 * sitting on a blank line. If dot is sitting on a blank line, this command
 * deletes all the blank lines above and below the current line. If it is
 * sitting on a non blank line then it deletes all of the blank lines after
 * the line. Normally this command is bound to "C-X C-O". Any argument is
 * ignored.
 */
int
deblank(int f, int n)
{
	LINE *lp1;
	LINE *lp2;
	long nld;

	if (!checkmodify())
		return FALSE;

	lp1 = curwp->w_dotp;
	while (llength(lp1) == 0 && (lp2 = lback(lp1)) != curbp->b_linep)
		lp1 = lp2;
	lp2 = lp1;
	nld = 0;
	while ((lp2 = lforw(lp2)) != curbp->b_linep && llength(lp2) == 0)
		++nld;
	if (nld == 0)
		return TRUE;
	curwp->w_dotp = lforw(lp1);
	curwp->w_doto = 0;
	return ldelete(nld, FALSE);
}

/*
 * Insert a newline, then enough tabs and spaces to duplicate the indentation
 * of the previous line. Tabs are every tabsize characters. Quite simple.
 * Figure out the indentation of the current line. Insert a newline by calling
 * the standard routine. Insert the indentation by inserting the right number
 * of tabs and spaces. Return TRUE if all ok. Return FALSE if one of the
 * subcomands failed. Normally bound to "C-J".
 */
int
indent(int f, int n)
{
	int nicol;
	int c;
	int i;

	if (!checkmodify())
		return FALSE;

	if (n < 0)
		return FALSE;
	while (n--) {
		nicol = 0;
		for (i = 0; i < llength(curwp->w_dotp); ++i) {
			c = lgetc(curwp->w_dotp, i);
			if (c != ' ' && c != '\t')
				break;
			if (c == '\t')
				nicol += -(nicol % curwp->w_bufp->b_tabs) + (curwp->w_bufp->b_tabs - 1);
			++nicol;
		}
		if (lnewline() == FALSE
		|| ((i = nicol / curwp->w_bufp->b_tabs) != 0 && linsert(i, '\t') == FALSE)
		|| ((i = nicol % curwp->w_bufp->b_tabs) != 0 && linsert(i, ' ') == FALSE))
			return FALSE;
	}
	return TRUE;
}

/*
 * Delete forward. This is real easy, because the basic delete routine does
 * all of the work. Watches for negative arguments, and does the right thing.
 * If any argument is present, it kills rather than deletes, to prevent loss
 * of text if typed with a big argument. Normally bound to "C-D".
 */
int
forwdel(int f, int n)
{
	if (!checkmodify())
		return FALSE;

	if (n < 0)
		return backdel(f, -n);
	if (f != FALSE) {	/* Really a kill. */
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}
	return ldelete((long) n, f);
}

/*
 * Delete backwards. This is quite easy too, because it's all done with other
 * functions. Just move the cursor back, and delete forwards. Like delete
 * forward, this actually does a kill if presented with an argument. Bound to
 * both "RUBOUT" and "C-H".
 */
int
backdel(int f, int n)
{
	int s;

	if (!checkmodify())
		return FALSE;

	if (n < 0)
		return forwdel(f, -n);
	if (f != FALSE) {	/* Really a kill. */
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}
	if ((s = backchar(f, n)) == TRUE)
		s = ldelete((long) n, f);
	return s;
}

/*
 * Kill text. If called without an argument, it kills from dot to the end of
 * the line, unless it is at the end of the line, when it kills the newline.
 * If called with an argument of 0, it kills from the start of the line to dot.
 * If called with a positive argument, it kills from dot forward over that
 * number of newlines. If called with a negative argument it kills backwards
 * that number of newlines. Normally bound to "C-K".
 */
int
killtext(int f, int n)
{
	LINE *nextp;
	long chunk;

	if (!checkmodify())
		return FALSE;

	if ((lastflag & CFKILL) == 0)	/* Clear kill buffer if */
		kdelete();	/* last wasn't a kill.  */
	thisflag |= CFKILL;
	if (f == FALSE) {
		chunk = llength(curwp->w_dotp) - curwp->w_doto;
		if (chunk == 0)
			chunk = 1;
	} else
		if (n == 0) {
			chunk = curwp->w_doto;
			curwp->w_doto = 0;
		} else
			if (n > 0) {
				chunk = llength(curwp->w_dotp) - curwp->w_doto + 1;
				nextp = lforw(curwp->w_dotp);
				while (--n) {
					if (nextp == curbp->b_linep)
						return FALSE;
					chunk += llength(nextp) + 1;
					nextp = lforw(nextp);
				}
			} else {
				mlwrite(TEXT61);
				/* "%%Negative argumet to kill is illegal" */
				return FALSE;
			}
	return ldelete(chunk, TRUE);
}

/* prompt and set an editor mode */
int
setmod(int f, int n)
{
	adjustmode(TRUE, FALSE);
	return TRUE;
}

/* prompt and delete an editor mode */
int
delmode(int f, int n)
{
	adjustmode(FALSE, FALSE);
	return TRUE;
}

/* prompt and set a global editor mode */
int
setgmode(int f, int n)
{
	adjustmode(TRUE, TRUE);
	return TRUE;
}

/* prompt and delete a global editor mode */
int
delgmode(int f, int n)
{
	adjustmode(FALSE, TRUE);
	return TRUE;
}

/*
 * change the editor mode status
 *
 *  kind: true = set, false = delete
 *  global: true = global flag, false = current buffer flag
 */
int
adjustmode(int kind, int global)
{
	char *scan;		/* scanning pointer to convert prompt */
	int i;			/* loop index */
	char prompt[50];	/* string to prompt user with */
	char crbuf[NPAT];	/* buffer to recieve mode name into */

	/* build the proper prompt string */
	if (global)
		strcpy(prompt, TEXT62);
	/* "Global mode to " */
	else
		strcpy(prompt, TEXT63);
	/* "Mode to " */

	if (kind == TRUE) {
		/* "add: " */
		strcat(prompt, TEXT64);
	} else {
		/* "delete: " */
		strcat(prompt, TEXT65);
	}

	scan = complete(prompt, 0, CMP_MODE, NPAT - 1);
	if (scan == 0)
		return FALSE;

	strcpy(crbuf, scan);
	scan = crbuf;
	while (*scan)
		uppercase(scan++);

	for (i = 0; i < NUMMODES; i++) {
		if ((strcasecmp(crbuf,modename[i]) == 0) ||
		    ((crbuf[1] == 0) && (crbuf[0] == modecode[i]))) {
			/* finding a match, we process it */
			if (kind == TRUE)
				if (global)
					gmode |= (1 << i);
				else
					curbp->b_mode |= (1 << i);
			else
				if (global)
					gmode &= ~(1 << i);
				else
					curbp->b_mode &= ~(1 << i);

			if ((1 << i) == MDEMPHASIS) {
				upwind();
			}

			/* display new mode line */
			if (global == 0)
				upmode();

			mlerase();	/* erase the junk */
			return TRUE;
		}
	}

	mlwrite(TEXT66);
	/* "No such mode!" */
	return FALSE;
}

/*
 * This function simply clears the message line, mainly for macro usage
 */
int
clrmes(int f, int n)
{
	mlforce("");
	return TRUE;
}

/*
 * This function writes a string on the message line mainly for macro usage
 */

int
writemsg(int f, int n)
{
	int status;
	char buf[NPAT];		/* buffer to recieve message into */

	if ((status = mlreply(TEXT67, buf, NPAT - 1)) != TRUE)
		/* "Message to write: " */
		return status;

	/* expand all '%' to "%%" so mlwrite won't expect arguments */
	makelit(buf);

	/* write the message out */
	mlforce(buf);
	return TRUE;
}

#if CFENCE
/* the cursor is moved to a matching fence	 */

#undef FENCE_BEEP

int
getfence(int f, int n)
{
	LINE *oldlp;		/* original line pointer */
	int oldoff;		/* and offset */
	int sdir;		/* direction of search (1/-1) */
	int count;		/* current fence level count */
	char ch;		/* fence type to match against */
	char ofence;		/* open fence */
	char c;			/* current character in scan */

	/* save the original cursor position */
	oldlp = curwp->w_dotp;
	oldoff = curwp->w_doto;

	/* get the current character */
	if (oldoff == llength(oldlp))
		ch = '\r';
	else
		ch = lgetc(oldlp, oldoff);

	/* setup proper matching fence */
	switch (ch) {
	case '(':
		ofence = ')';
		sdir = FORWARD;
		break;
	case '{':
		ofence = '}';
		sdir = FORWARD;
		break;
	case '[':
		ofence = ']';
		sdir = FORWARD;
		break;
	case ')':
		ofence = '(';
		sdir = REVERSE;
		break;
	case '}':
		ofence = '{';
		sdir = REVERSE;
		break;
	case ']':
		ofence = '[';
		sdir = REVERSE;
		break;
	default:
#ifdef FENCE_BEEP
		TTbeep();
#endif
		return FALSE;
	}

	/* set up for scan */
	count = 1;
	if (sdir == REVERSE)
		backchar(FALSE, 1);
	else
		forwchar(FALSE, 1);

	/* scan until we find it, or reach the end of file */
	while (count > 0) {
		if (curwp->w_doto == llength(curwp->w_dotp))
			c = '\r';
		else
			c = lgetc(curwp->w_dotp, curwp->w_doto);
		if (c == ch)
			++count;
		if (c == ofence)
			--count;
		if (sdir == FORWARD)
			forwchar(FALSE, 1);
		else
			backchar(FALSE, 1);
		if (boundry(curwp->w_dotp, curwp->w_doto, sdir))
			break;
	}

	/* if count is zero, we have a match, move the sucker */
	if (count == 0) {
		if (sdir == FORWARD)
			backchar(FALSE, 1);
		else
			forwchar(FALSE, 1);
		curwp->w_flag |= WFMOVE;
		return TRUE;
	}
	/* restore the current position */
	curwp->w_dotp = oldlp;
	curwp->w_doto = oldoff;
#ifdef FENCE_BEEP
	TTbeep();
#endif
	return FALSE;
}
#endif

/*
 * Close fences are matched against their partners, and if on screen the
 * cursor briefly lights there
 */

int
fmatch(char ch)	/* fence type to match against */
{
	LINE *oldlp;		/* original line pointer */
	int oldoff;		/* and offset */
	LINE *toplp;		/* top line in current window */
	int count;		/* current fence level count */
	char opench;		/* open fence */
	char c;			/* current character in scan */

	/* first get the display update out there */
	update(FALSE);

	/* save the original cursor position */
	oldlp = curwp->w_dotp;
	oldoff = curwp->w_doto;

	/* setup proper open fence for passed close fence */
	if (ch == ')')
		opench = '(';
	else
		if (ch == '}')
			opench = '{';
		else
			opench = '[';

	/* find the top line and set up for scan */
	toplp = curwp->w_linep->l_bp;
	count = 1;
	backchar(FALSE, 2);

	/* scan back until we find it, or reach past the top of the window */
	while (count > 0 && curwp->w_dotp != toplp) {
		if (curwp->w_doto == llength(curwp->w_dotp))
			c = '\r';
		else
			c = lgetc(curwp->w_dotp, curwp->w_doto);
		if (c == ch)
			++count;
		if (c == opench)
			--count;
		backchar(FALSE, 1);
		if (curwp->w_dotp == curwp->w_bufp->b_linep->l_fp &&
		    curwp->w_doto == 0)
			break;
	}

	/* if count is zero, we have a match, display the sucker */
	/*
	 * there is a real machine dependant timing problem here we have yet
	 * to solve.........
	 */
	if (count == 0) {
		fd_set readfds;
		struct timeval timeout;

		forwchar(FALSE, 1);

		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		timeout.tv_sec = term.t_pause / 1000;
		timeout.tv_usec = (term.t_pause % 1000) * 1000;

		update(FALSE);

		if ((kbdmode != PLAY) && (tthaveinput() == 0))
			select(1, &readfds, 0, 0, &timeout);
	}
	/* restore the current position */
	curwp->w_dotp = oldlp;
	curwp->w_doto = oldoff;
	return TRUE;
}

/* ask for and insert a string into the current buffer at the current point */
int
istring(int f, int n)
{
	int status;		/* status return code */
	char tstring[NPAT + 1];	/* string to add */

	/* ask for string to insert */
	status = mltreply(TEXT68, tstring, NPAT, sterm);
	/* "String to insert<META>: " */
	if (status != TRUE)
		return status;

	if (f == FALSE)
		n = 1;

	if (n < 0)
		n = -n;

	/* insert it */
	while (n-- && (status = linstr(tstring)));
	return status;
}

/* ask for and overwite a string into the current buffer at the current point */
int
ovstring(int f, int n)
{
	int status;		/* status return code */
	char tstring[NPAT + 1];	/* string to add */

	/* ask for string to insert */
	status = mltreply(TEXT69, tstring, NPAT, sterm);
	/* "String to overwrite<META>: " */
	if (status != TRUE)
		return status;

	if (f == FALSE)
		n = 1;

	if (n < 0)
		n = -n;

	/* insert it */
	while (n-- && (status = lover(tstring)));
	return status;
}


int
bell(int f, int n)
{
	TTbeep();
	return TRUE;
}

void
chomp(char *str)
{
	if (!*str)
		return;

	if (!str[1]) {
		if (str[0] == '\n')
			str[0] = '\0';
		return;
	}

	while (*str++)
		;

	if (str[-2] == '\n')
		str[-2] = '\0';
}

