/*
 * $Id: search.c,v 1.25 2017/01/02 18:14:48 ryo Exp $
 *
 * The functions in this file implement commands that search in the forward
 * and backward directions.
 *
 * (History comments formerly here have been moved to history.c)
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"

static int patlenadd;
static int deltaf[HICHAR], deltab[HICHAR];
static int lastchfjump, lastchbjump;
static short int gr_closure;

/*
 * forwsearch -- Search forward.  Get a search string from the user,
 * and search for the string.
 * If found, reset the "." to be just after the match string,
 * and (perhaps) repaint the display.
 */
int
forwsearch(int f, int n)
{
	int status;

	/*
	 * If n is negative, search backwards. Otherwise proceed by asking
	 * for the search string.
	 */
	if (n < 0)
		return backsearch(f, -n);

	/*
	 * Ask the user for the text of a pattern.  If the response is TRUE
	 * (responses other than FALSE are possible), search for the pattern
	 * for up to n times, as long as the pattern is there to be found.
	 */
	if ((status = readpattern(TEXT78, &pat[0], TRUE, FALSE)) == TRUE)
		status = forwhunt(f, n);
	/* "Search" */

	return status;
}

/*
 * forwhunt -- Search forward for a previously acquired search string.
 * If found, reset the "." to be just after the match string,
 * and (perhaps) repaint the display.
 */
int
forwhunt(int f, int n)
{
	int spoint = PTEND;
	int status;

	if (n < 0)		/* search backwards */
		return backhunt(f, -n);

	/*
	 * Make sure a pattern exists, or that we didn't switch into MAGIC
	 * mode after we entered the pattern.
	 */
	if (pat[0] == '\0') {
		mlwrite(TEXT80);
		/* "No pattern set" */
		return FALSE;
	}
#if MAGIC
	if ((curwp->w_bufp->b_mode & MDMAGIC) != 0 &&
	    mcpat[0].mc_type == MCNIL) {
		if (!mcstr())
			return FALSE;
	}
#endif


#if MAGIC
	if ((magical && curwp->w_bufp->b_mode & MDMAGIC) != 0)
		status = mcscanner(FORWARD, spoint, n);
	else
#endif
		status = scanner(FORWARD, spoint, n);

	/*
	 * Complain if not there.
	 */
	if (status != TRUE)
		mlwrite(TEXT79);
	/* "Not found" */

	return status;
}

/*
 * backsearch -- Reverse search.  Get a search string from the user, and search,
 * starting at "." and proceeding toward the front of the buffer.
 * If found "." is left pointing at the first character of the pattern
 * (the last character that was matched).
 */
int
backsearch(int f, int n)
{
	int status;

	/*
	 * If n is negative, search forwards. Otherwise proceed by asking for
	 * the search string.
	 */
	if (n < 0)
		return forwsearch(f, -n);

	/*
	 * Ask the user for the text of a pattern.  If the response is TRUE
	 * (responses other than FALSE are possible), search for the pattern
	 * for up to n times, as long as the pattern is there to be found.
	 */
	if ((status = readpattern(TEXT81, &pat[0], TRUE, FALSE)) == TRUE)
		status = backhunt(f, n);
	/* "Reverse search" */

	return status;
}

/*
 * backhunt -- Reverse search for a previously acquired search string,
 * starting at "." and proceeding toward the front of the buffer.
 * If found "." is left pointing at the first character of the pattern
 * (the last character that was matched).
 */
int
backhunt(int f, int n)
{
	int spoint = PTBEG;
	int status;

	if (n < 0)
		return forwhunt(f, -n);

	/*
	 * Make sure a pattern exists, or that we didn't switch into MAGIC
	 * mode after we entered the pattern.
	 */
	if (tap[0] == '\0') {
		mlwrite(TEXT80);
		/* "No pattern set" */
		return FALSE;
	}
#if MAGIC
	if ((curwp->w_bufp->b_mode & MDMAGIC) != 0 &&
	    tapcm[0].mc_type == MCNIL) {
		if (!mcstr())
			return FALSE;
	}
#endif

#if MAGIC
	if ((magical && curwp->w_bufp->b_mode & MDMAGIC) != 0)
		status = mcscanner(REVERSE, spoint, n);
	else
#endif
		status = scanner(REVERSE, spoint, n);

	/*
	 * Complain if not there.
	 */
	if (status != TRUE)
		mlwrite(TEXT79);
	/* "Not found" */

	return status;
}

#if MAGIC
/*
 * mcscanner -- Search for a meta-pattern in either direction.
 * If found, reset the "." to be at the start or just after the match string,a
 * and (perhaps) repaint the display.
 *
 * direct: which way to go.
 * beg_or_end: put point at beginning or end of pattern.
 * repeats: search repetitions.
 */
int
mcscanner(int direct, int beg_or_end, int repeats)
{
	MC *mcpatrn;		/* pointer into pattern */
	LINE *curline;		/* current line during scan */
	int curoff;		/* position within current line */

	/*
	 * If we are going in reverse, then the 'end' is actually the
	 * beginning of the pattern.  Toggle it.
	 */
	beg_or_end ^= direct;

	/*
	 * Another directional problem: if we are searching forwards, use the
	 * pattern in mcpat[].  Otherwise, use the reversed pattern in
	 * tapcm[].
	 */
	mcpatrn = (direct == FORWARD) ? &mcpat[0] : &tapcm[0];

	/*
	 * Save the old matchlen length, in case it is
	 * very different (closure) from the old length.
	 * This is important for query-replace undo
	 * command.
	 */
	mlenold = matchlen;

	/*
	 * Setup local scan pointers to global ".".
	 */
	curline = curwp->w_dotp;
	curoff = curwp->w_doto;

	/*
	 * Scan each character until we hit the head link record.
	 */
	while (!boundry(curline, curoff, direct)) {
		/*
		 * Save the current position in case we need to restore it on
		 * a match, and initialize matchlen to zero in case we are
		 * doing a search for replacement.
		 */
		matchline = curline;
		matchoff = curoff;
		matchlen = 0;

		if (amatch(mcpatrn, direct, &curline, &curoff)) {
			/*
			 * A SUCCESSFULL MATCH!!! reset the global "."
			 * pointers.
			 */
			if (beg_or_end == PTEND) {	/* at end of string */
				curwp->w_dotp = curline;
				curwp->w_doto = curoff;
			} else {/* at beginning of string */
				curwp->w_dotp = matchline;
				curwp->w_doto = matchoff;
			}

			curwp->w_flag |= WFMOVE;	/* flag that we have
							 * moved */
			savematch();

			/*
			 * Continue scanning if we haven't found
			 * the nth match.
			 */
			if (--repeats <= 0)
				return TRUE;
		}
		/*
		 * Advance the cursor.
		 */
		nextch(&curline, &curoff, direct);
	}

	return FALSE;		/* We could not find a match. */
}

/*
 * amatch -- Search for a meta-pattern in either direction.  Based on the
 *	recursive routine amatch() (for "anchored match") in
 *	Kernighan & Plauger's "Software Tools".
 *
 * mcptr: string to scan for
 * direct: which way to go.
 * pcwline: current line during scan
 * pcwoff: position within current line
 */
int
amatch(MC *mcptr, int direct, LINE **pcwline, int *pcwoff)
{
	int c;		/* character at current position */
	LINE *curline;		/* current line during scan */
	int curoff;		/* position within current line */
	int nchars;

	/*
	 * Set up local scan pointers to ".", and get the current character.
	 * Then loop around the pattern pointer until success or failure.
	 */
	curline = *pcwline;
	curoff = *pcwoff;

	/*
	 * The beginning-of-line and end-of-line metacharacters do not
	 * compare against characters, they compare against positions. BOL is
	 * guaranteed to be at the start of the pattern for forward searches,
	 * and at the end of the pattern for reverse searches.  The reverse
	 * is true for EOL. So, for a start, we check for them on entry.
	 */
	if (mcptr->mc_type == BOL) {
		if (curoff != 0)
			return FALSE;
		mcptr++;
	}
	if (mcptr->mc_type == EOL) {
		if (curoff != llength(curline))
			return FALSE;
		mcptr++;
	}
	while (mcptr->mc_type != MCNIL) {
		c = nextch(&curline, &curoff, direct);

		if (mcptr->mc_type & CLOSURE) {
			/*
			 * Try to match as many characters as possible
			 * against the current meta-character.  It used to be
			 * that a newline never matched a closure.  This
			 * restriction has been dropped.
			 */
			nchars = 0;
			while (mceq(c, mcptr)) {
				c = nextch(&curline, &curoff, direct);
				nchars++;
			}

			/*
			 * We are now at the character that made us fail.
			 * Try to match the rest of the pattern. Shrink the
			 * closure by one for each failure. Since closure
			 * matches *zero* or more occurences of a pattern, a
			 * match may start even if the previous loop matched
			 * no characters.
			 */
			mcptr++;

			for (;;) {
				c = nextch(&curline, &curoff, direct ^ REVERSE);

				if (amatch(mcptr, direct, &curline, &curoff)) {
					matchlen += nchars;
					goto success;
				}
				if (nchars-- == 0)
					return FALSE;
			}
		} else {	/* Not closure. */
			/*
			 * The only way we'd get a BOL metacharacter at this
			 * point is at the end of the reversed pattern. The
			 * only way we'd get an EOL metacharacter here is at
			 * the end of a regular pattern. So if we have one or
			 * the other, and are at the appropriate position, we
			 * are guaranteed success (since the next pattern
			 * character has to be MCNIL). Before we report
			 * success, however, we back up by one character, so
			 * as to leave the cursor in the correct position.
			 * For example, a search for "X$" will leave the
			 * cursor at the end of the line, while a search for
			 * "X<NL>" will leave the cursor at the beginning of
			 * the next line.  This follows the notion that the
			 * meta-character '$' (and likewise '^') matches
			 * positions, not characters.
			 */
			if (mcptr->mc_type == BOL) {
				if (curoff == llength(curline)) {
					nextch(&curline, &curoff,
					       direct ^ REVERSE);
					goto success;
				} else {
					return FALSE;
				}
			}

			if (mcptr->mc_type == EOL) {
				if (curoff == 0) {
					nextch(&curline, &curoff,
					       direct ^ REVERSE);
					goto success;
				} else {
					return FALSE;
				}
			}

			/*
			 * Neither BOL nor EOL, so go through the
			 * meta-character equal function.
			 */
			if (!mceq(c, mcptr))
				return FALSE;
		}

		/*
		 * Increment the length counter and advance the pattern
		 * pointer.
		 */
		matchlen++;
		mcptr++;
	}			/* End of mcptr loop. */

	/*
	 * A SUCCESSFULL MATCH!!! Reset the "." pointers.
	 */
success:
	*pcwline = curline;
	*pcwoff = curoff;

	return TRUE;
}
#endif

/*
 * scanner -- Search for a pattern in either direction.  If found,
 * reset the "." to be at the start or just after the match string,
 * and (perhaps) repaint the display.
 * Fast version using simplified version of Boyer and Moore
 * Software-Practice and Experience, vol 10, 501-506 (1980)
 *
 * direct: which way to go.
 * beg_or_end: put point at beginning or end of pattern.
 * repeats: search repetitions.
 */
int
scanner(int direct, int beg_or_end, int repeats)
{
	int c;		/* character at current position */
	char *patptr;	/* pointer into pattern */
	char *patrn;		/* string to scan for */
	LINE *curline;		/* current line during scan */
	int curoff;		/* position within current line */
	LINE *scanline;		/* current line during scanning */
	int scanoff;		/* position in scanned line */
	int jump;		/* next offset */

	/*
	 * If we are going in reverse, then the 'end' is actually the
	 * beginning of the pattern.  Toggle it.
	 */
	beg_or_end ^= direct;

	/*
	 * Another directional problem: if we are searching forwards, use the
	 * pattern in pat[].  Otherwise, use the reversed pattern in tap[].
	 */
	patrn = (direct == FORWARD) ? &pat[0] : &tap[0];

	/*
	 * Set up local pointers to global ".".
	 */
	curline = curwp->w_dotp;
	curoff = curwp->w_doto;

	/*
	 * Scan each character until we hit the head link record. Get the
	 * character resolving newlines, offset by the pattern length, i.e.
	 * the last character of the potential match.
	 */
	jump = patlenadd;

	while (!fbound(jump, &curline, &curoff, direct)) {
		/*
		 * Set up the scanning pointers, and save the current
		 * position in case we match the search string at this point.
		 */
		scanline = matchline = curline;
		scanoff = matchoff = curoff;

		patptr = patrn;

		/*
		 * Scan through the pattern for a match.
		 */
		while ((c = *patptr++ & 255) != '\0')
			if (!eq(c, nextch(&scanline, &scanoff, direct))) {
				jump = (direct == FORWARD)
					? lastchfjump
					: lastchbjump;
				goto fail;
			}
		/*
		 * A SUCCESSFULL MATCH!!! reset the global "." pointers
		 */
		if (beg_or_end == PTEND) {	/* at end of string */
			curwp->w_dotp = scanline;
			curwp->w_doto = scanoff;
		} else {	/* at beginning of string */
			curwp->w_dotp = matchline;
			curwp->w_doto = matchoff;
		}

		curwp->w_flag |= WFMOVE;	/* Flag that we have moved. */
		savematch();

		/*
		 * Continue scanning if we haven't found
		 * the nth match.
		 */
		if (--repeats <= 0)
			return TRUE;
		else {
			curline = scanline;
			curoff = scanoff;
		}

fail:		;		/* continue to search */
	}

	return FALSE;		/* We could not find a match */
}

/*
 * fbound -- Return information depending on whether we have hit a boundry
 *	(and may therefore search no further) or if a trailing character
 *	of the search string has been found.  See boundry() for search
 *	restrictions.
 */
int
fbound(int jump, LINE **pcurline, int *pcuroff, int dir)
{
	int spare, curoff;
	LINE *curline;

	curline = *pcurline;
	curoff = *pcuroff;

	if (dir == FORWARD) {
		while (jump != 0) {
			curoff += jump;
			spare = curoff - llength(curline);

			if (curline == curbp->b_linep)
				return TRUE;	/* hit end of buffer */

			while (spare > 0) {
				curline = lforw(curline);	/* skip to next line */
				curoff = spare - 1;
				spare = curoff - llength(curline);
				if (curline == curbp->b_linep)
					return TRUE;	/* hit end of buffer */
			}

			if (spare == 0)
				jump = deltaf[(int) '\r'];
			else
				jump = deltaf[(int) lgetc(curline, curoff)];
		}

		/*
		 * The last character matches, so back up to start of
		 * possible match.
		 */
		curoff -= patlenadd;

		while (curoff < 0) {
			curline = lback(curline);	/* skip back a line */
			curoff += llength(curline) + 1;
		}

	} else {		/* Reverse. */
		jump++;		/* allow for offset in reverse */
		while (jump != 0) {
			curoff -= jump;
			while (curoff < 0) {
				curline = lback(curline);	/* skip back a line */
				curoff += llength(curline) + 1;
				if (curline == curbp->b_linep)
					return TRUE;	/* hit end of buffer */
			}

			if (curoff == llength(curline))
				jump = deltab[(int) '\r'];
			else
				jump = deltab[(int) lgetc(curline, curoff)];
		}

		/*
		 * The last character matches, so back up to start of
		 * possible match.
		 */
		curoff += matchlen;
		spare = curoff - llength(curline);
		while (spare > 0) {
			curline = lforw(curline);	/* skip back a line */
			curoff = spare - 1;
			spare = curoff - llength(curline);
		}
	}

	*pcurline = curline;
	*pcuroff = curoff;
	return FALSE;
}

/*
 * setjtable -- Settting up search delta forward and delta backward
 *	tables.  The reverse search string and string lengths are
 *	set here, for table initialization and for substitution
 *	purposes.  The default for any character to jump is the
 *	pattern length.
 */
int
setjtable(void)
{
	int i;

	rvstrcpy(tap, pat);
	patlenadd = (mlenold = matchlen = strlen(pat)) - 1;

	for (i = 0; i < HICHAR; i++) {
		deltaf[i] = matchlen;
		deltab[i] = matchlen;
	}

	/*
	 * Now put in the characters contained in the pattern, duplicating
	 * the CASE
	 */
	for (i = 0; i < patlenadd; i++) {
#if 0
		/*
		 * Debugging & tracing information.
		 */
		mlwrite(TEXT82, pat[i] & 255, patlenadd - i);
		/* "Considering %d with jump %d" */
		tgetc();
		if (isletter(pat[i] & 255)) {
			mlwrite(TEXT83, CHCASE(pat[i] & 255));
			/* "Its other case is %d" */
			tgetc();
		}
#endif
		if (isletter(pat[i] & 255))
			deltaf[CHCASE(pat[i] & 255)]
				= patlenadd - i;
		deltaf[pat[i] & 255] = patlenadd - i;

		if (isletter(tap[i] & 255))
			deltab[CHCASE(tap[i] & 255)]
				= patlenadd - i;
		deltab[tap[i] & 255] = patlenadd - i;
	}

	/*
	 * The last character will have the pattern length unless there are
	 * duplicates of it.  Get the number to jump from the arrays delta,
	 * and overwrite with zeroes in delta duplicating the CASE.
	 */
	lastchfjump = patlenadd + deltaf[pat[patlenadd] & 255];
	lastchbjump = patlenadd + deltab[pat[0] & 255];

	if (isletter(pat[patlenadd] & 255))
		deltaf[CHCASE(pat[patlenadd] & 255)] = 0;
	deltaf[pat[patlenadd] & 255] = 0;

	if (isletter(pat[0] & 255))
		deltab[CHCASE(pat[0] & 255)] = 0;
	deltab[pat[0] & 255] = 0;

	return TRUE;
}

/*
 * eq -- Compare two characters.  The "bc" comes from the buffer, "pc"
 *	from the pattern.  If we are not in EXACT mode, fold out the case.
 */
int
eq(int bc, int pc)
{
	bc = bc & 255;
	pc = pc & 255;
	if ((curwp->w_bufp->b_mode & MDEXACT) == 0) {
		if (islower(bc))
			bc = CHCASE(bc);

		if (islower(pc))
			pc = CHCASE(pc);
	}
	return bc == pc;
}

/*
 * readpattern -- Read a pattern.  Stash it in apat.  If it is the
 *	search string (which means that the global variable pat[]
 *	has been passed in), create the reverse pattern and the magic
 *	pattern, assuming we are in MAGIC mode (and #defined that way).
 *
 *	Apat is not updated if the user types in an empty line.  If
 *	the user typed an empty line, and there is no old pattern, it is
 *	an error.  Display the old pattern, in the style of Jeff Lomicka.
 *	There is some do-it-yourself control expansion.  Change to using
 *	<META> to delimit the end-of-pattern to allow <NL>s in the search
 *	string.
 */
/*
 * Except that you CAN change the search terminator, but not the prompt Also
 * need force_magic parameter to make reg-exp-search work - Fixed 8/9/90 Paul
 * Amaranth
 */

int
readpattern(char *prompt, char apat[], int srch, int force_magic)
{
	int status;
	char tpat[NPAT + 20];

	strcpy(tpat, prompt);	/* copy prompt to output string */
	strcat(tpat, " [");	/* build new prompt string */
	expandp(&apat[0], &tpat[strlen(tpat)], NPAT / 2);	/* add old pattern */
	/* Make sure we use the right prompt PGA */
	if (sterm == ctoec('\r'))
		strcat(tpat, "]<NL>: ");
	else
		strcat(tpat, "]<META>: ");

	/*
	 * Read a pattern.  Either we get one, or we just get the META
	 * charater, and use the previous pattern. Then, if it's the search
	 * string, make a reversed pattern. *Then*, make the meta-pattern, if
	 * we are defined that way.
	 */
	/* if ((status = mltreply(tpat, tpat, NPAT, sterm)) == TRUE) */
	if (srch)
		status = mltreply(tpat, tpat, NPAT, sterm);
	else {
		strcpy(tpat, apat);

		/* if we are interactive, go get it! */
		if (clexec == FALSE) {
			status = (getstring(prompt, tpat, NPAT, sterm));
		} else {
			char *sp;

			/* grab token and advance past */
			execstr = token(execstr, tpat, NPAT);

			/* evaluate it */
			if ((sp = getval(tpat)) == NULL) {
				status = FALSE;
			} else {
				strcpy(tpat, sp);
				status = TRUE;
			}	/* end of else */
		}		/* end of else */
	}			/* end of else */

	if (status == TRUE) {
		strcpy(apat, tpat);

		/*
		 * If we are doing the search string, set the delta tables.
		 */
		if (srch)
			setjtable();

	} else
		if (status == FALSE && apat[0] != 0)	/* Old one */
			status = TRUE;

#if MAGIC
	/*
	 * Only make the meta-pattern if in magic mode, since the pattern in
	 * question might have an invalid meta combination.
	 */
	if (status == TRUE) {
		if ((!force_magic && curwp->w_bufp->b_mode & MDMAGIC) == 0) {
			mcclear();
			rmcclear();
		} else {
			status = srch ? mcstr() : rmcstr();
		}
	}
#endif
	return status;
}

/*
 * savematch -- We found the pattern?  Let's save it away.
 */
int
savematch(void)
{
	char *ptr;	/* pointer to last match string */
	int j;
	LINE *curline;		/* line of last match */
	int curoff;		/* offset "      "    */

	/*
	 * Free any existing match string, then attempt to allocate a new
	 * one.
	 */
	if (patmatch != NULL)
		FREE(patmatch);

	ptr = patmatch = (void*)MALLOC(matchlen + 1);

	if (ptr != NULL) {
		curoff = matchoff;
		curline = matchline;

		for (j = 0; j < matchlen; j++)
			*ptr++ = nextch(&curline, &curoff, FORWARD);

		*ptr = '\0';
	}
	return TRUE;
}

/*
 * rvstrcpy -- Reverse string copy.
 */
int
rvstrcpy(char *rvstr, char *str)
{
	int i;

	str += (i = strlen(str));

	while (i-- > 0)
		*rvstr++ = *--str;

	*rvstr = '\0';

	return TRUE;
}

/*
 * sreplace -- Search and replace.
 */
int
sreplace(int f, int n)
{
	return replaces(FALSE, f, n);
}

/*
 * qreplace -- search and replace with query.
 */
int
qreplace(int f, int n)
{
	return replaces(TRUE, f, n);
}

/*
 * replaces -- Search for a string and replace it with another string.
 * Query might be enabled (according to kind).
 *
 * kind: Query enabled flag
 * f: default flag
 * n: # of repetitions wanted
 */
int
replaces(int kind, int f, int n)
{
	int status;	/* success flag on pattern inputs */
	int rlength;	/* length of replacement string */
	int numsub;	/* number of substitutions */
	int nummatch;	/* number of found matches */
	int nlflag;		/* last char of search string a <NL>? */
	int nlrepl;		/* was a replace done on the last line? */
	char c;			/* input char for query */
	char tpat[NPAT];	/* temporary to hold search pattern */
	LINE *origline;		/* original "." position */
	int origoff;		/* and offset (for . query option) */
	LINE *lastline;		/* position of last replace and */
	int lastoff;		/* offset (for 'u' query option) */

	(void)&lastline;	/* stupid gcc */
	(void)&lastoff;		/* stupid gcc */

	if (!checkmodify())
		return FALSE;

	/*
	 * Check for negative repetitions.
	 */
	if (f && n < 0)
		return FALSE;

	/*
	 * Ask the user for the text of a pattern.
	 */
	if ((status = readpattern(
	  (kind == FALSE ? TEXT84 : TEXT85), &pat[0], TRUE, FALSE)) != TRUE)
		/* "Replace" */
		/* "Query replace" */
		return status;

	/*
	 * Ask for the replacement string.
	 */
	if ((status = readpattern(TEXT86, &rpat[0], TRUE, FALSE)) == ABORT)
		/* "with" */
		return status;

	/*
	 * Find the length of the replacement string.
	 */
	rlength = strlen(&rpat[0]);

	/*
	 * Set up flags so we can make sure not to do a recursive replace on
	 * the last line.
	 */
	nlflag = (pat[matchlen - 1] == '\r');
	nlrepl = FALSE;

	if (kind) {
		/*
		 * Build query replace question string.
		 */
		strcpy(tpat, TEXT87);
		/* "Replace '" */
		expandp(&pat[0], &tpat[strlen(tpat)], NPAT / 3);
		strcat(tpat, TEXT88);
		/* "' with '" */
		expandp(&rpat[0], &tpat[strlen(tpat)], NPAT / 3);
		strcat(tpat, "'? ");

		/*
		 * Initialize last replaced pointers.
		 */
		lastline = NULL;
		lastoff = 0;
	}
	/*
	 * Save original . position, init the number of matches and
	 * substitutions, and scan through the file.
	 */
	origline = curwp->w_dotp;
	origoff = curwp->w_doto;
	numsub = 0;
	nummatch = 0;

	while ((f == FALSE || n > nummatch) &&
	       (nlflag == FALSE || nlrepl == FALSE)) {
		/*
		 * Search for the pattern. If we search with a regular
		 * expression, matchlen is reset to the true length of the
		 * matched string.
		 */
#if MAGIC
		if ((magical && curwp->w_bufp->b_mode & MDMAGIC) != 0) {
			if (!mcscanner(FORWARD, PTBEG, 1))
				break;
		} else
#endif
			if (!scanner(FORWARD, PTBEG, 1))
				break;	/* all done */

		++nummatch;	/* Increment # of matches */

		/*
		 * Check if we are on the last line.
		 */
		nlrepl = (lforw(curwp->w_dotp) == curwp->w_bufp->b_linep);

		/*
		 * Check for query.
		 */
		if (kind) {
			/*
			 * Get the query.
			 */
	pprompt:	mlwrite(&tpat[0], &pat[0], &rpat[0]);
	qprompt:
			update(TRUE);	/* show the proposed place to change */
			c = tgetc();	/* and input */
			mlerase();	/* and clear it */

			/*
			 * And respond appropriately.
			 */
			switch (c) {
			case 'y':	/* yes, substitute */
			case 'Y':
			case ' ':
				break;

			case 'n':	/* no, onward */
			case 'N':
				forwchar(FALSE, 1);
				continue;

			case '!':	/* yes/stop asking */
				kind = FALSE;
				break;

			case 'u':	/* undo last and re-prompt */
			case 'U':
				/*
				 * Restore old position.
				 */
				if (lastline == NULL) {
					/*
					 * There is nothing to undo.
					 */
					TTbeep();
					goto pprompt;
				}
				curwp->w_dotp = lastline;
				curwp->w_doto = lastoff;
				lastline = NULL;
				lastoff = 0;

				/*
				 * Delete the new string.
				 */
				backchar(FALSE, rlength);
				status = delins(rlength, patmatch, FALSE);
				if (status != TRUE)
					return status;

				/*
				 * Record one less substitution, backup, save
				 * our place, and reprompt.
				 */
				--numsub;
				backchar(FALSE, mlenold);
				matchline = curwp->w_dotp;
				matchoff = curwp->w_doto;
				goto pprompt;

			case '.':	/* abort! and return */
				/* restore old position */
				curwp->w_dotp = origline;
				curwp->w_doto = origoff;
				curwp->w_flag |= WFMOVE;

			case BELL:	/* abort! and stay */
				mlwrite(TEXT89);
				/* "Aborted!" */
				return FALSE;

			default:	/* bitch and beep */
				TTbeep();

			case '?':	/* help me */
				mlwrite(TEXT90);
				/*
				 * "(Y)es, (N)o, (!)Do rest, (U)ndo last,
				 * (^G)Abort, (.)Abort back, (?)Help: "
				 */
				goto qprompt;

			}	/* end of switch */
		}		/* end of "if kind" */
		/* if this is the point origin, flag so we a can reset it */
		if (curwp->w_dotp == origline) {
			origline = NULL;
			lastline = curwp->w_dotp->l_bp;
		}
		/*
		 * Delete the sucker, and insert its
		 * replacement.
		 */
		status = delins(matchlen, &rpat[0], TRUE);
		if (origline == NULL) {
			origline = lastline->l_fp;
			origoff = 0;
		}
		if (status != TRUE)
			return status;

		/*
		 * Save our position, since we may undo this. If we are not
		 * querying, check to make sure that we didn't replace an
		 * empty string (possible in MAGIC mode), because we'll
		 * infinite loop.
		 */
		if (kind) {
			lastline = curwp->w_dotp;
			lastoff = curwp->w_doto;
		} else
			if (matchlen == 0) {
				mlwrite(TEXT91);
				/* "Empty string replaced, stopping." */
				return FALSE;
			}
		numsub++;	/* increment # of substitutions */
	}

	/*
	 * And report the results.
	 */
	mlwrite(TEXT92, numsub);
	/* "%d substitutions" */

	update(TRUE);
	return TRUE;
}

/*
 * delins -- Delete a specified length from the current point
 *	then either insert the string directly, or make use of
 *	replacement meta-array.
 */
int
delins(int dlength, char *instr, int use_rmc)
{
	int status;
#if MAGIC
	RMC *rmcptr;
#endif

	/*
	 * Zap what we gotta, and insert its replacement.
	 */
	if ((status = ldelete((long) dlength, FALSE)) != TRUE)
		mlwrite(TEXT93);
	/* "%%ERROR while deleting" */
	else
#if MAGIC
		if ((rmagical && use_rmc) &&
		    (curwp->w_bufp->b_mode & MDMAGIC) != 0) {
			rmcptr = &rmcpat[0];
			while (rmcptr->mc_type != MCNIL && status == TRUE) {
				if (rmcptr->mc_type == LITCHAR)
					status = linstr(rmcptr->rstr);
				else
					status = linstr(patmatch);
				rmcptr++;
			}
		} else
#endif
			status = linstr(instr);

	return status;
}

/*
 * expandp -- Expand control key sequences for output.
 *  srcstr: string to expand
 *  deststr:  destination of expanded string
 *  maxlength: maximum chars in destination
 */
int
expandp(char *srcstr, char *deststr, int maxlength)
{
	unsigned char c;	/* current char to translate */
				/* Scan through the string. */
	while ((c = *srcstr++) != 0) {
		if (c == '\r') {/* it's a newline */
			*deststr++ = '<';
			*deststr++ = 'N';
			*deststr++ = 'L';
			*deststr++ = '>';
			maxlength -= 4;
			goto skip_expandp;
		}

		if (c == '\n') {/* it's a linefeed */
			*deststr++ = '<';
			*deststr++ = 'L';
			*deststr++ = 'F';
			*deststr++ = '>';
			maxlength -= 4;
			goto skip_expandp;
		}

		if (c == '\e') {/* it's a ESC */
			*deststr++ = '<';
			*deststr++ = 'M';
			*deststr++ = 'E';
			*deststr++ = 'T';
			*deststr++ = 'A';
			*deststr++ = '>';
			maxlength -= 6;
			goto skip_expandp;
		}

		if (c == '\t') {/* it's a tab */
			*deststr++ = '<';
			*deststr++ = 'T';
			*deststr++ = 'A';
			*deststr++ = 'B';
			*deststr++ = '>';
			maxlength -= 5;
			goto skip_expandp;
		}

		if (c < 0x20 || c == 0x7f) {	/* control character */
			*deststr++ = '^';
			*deststr++ = c ^ 0x40;
			maxlength -= 2;
			goto skip_expandp;
		}

#if 0
		if (c == '%') {
			*deststr++ = '%';
			*deststr++ = '%';
			maxlength -= 2;
			goto skip_expandp;
		}
#endif

		/* any other character */
		*deststr++ = c;
		maxlength--;
skip_expandp:


		/* check for maxlength */
		if (maxlength < 4) {
			*deststr++ = '$';
			*deststr = '\0';
			return FALSE;
		}
	}
	*deststr = '\0';
	return TRUE;
}

/*
 * boundry -- Return information depending on whether we may search no
 *	further.  Beginning of file and end of file are the obvious
 *	cases, but we may want to add further optional boundry restrictions
 *	in future, a' la VMS EDT.  At the moment, just return TRUE or
 *	FALSE depending on if a boundry is hit (ouch).
 */
int
boundry(LINE *curline, int curoff, int dir)
{
	int border;

	if (dir == FORWARD) {
		border = (curoff == llength(curline)) &&
			(lforw(curline) == curbp->b_linep);
	} else {
		border = (curoff == 0) &&
			(lback(curline) == curbp->b_linep);
	}
	return border;
}

/*
 * nextch -- retrieve the next/previous character in the buffer,
 *	and advance/retreat the point.
 *	The order in which this is done is significant, and depends
 *	upon the direction of the search.  Forward searches look at
 *	the current character and move, reverse searches move and
 *	look at the character.
 */
int
nextch(LINE **pcurline, int *pcuroff, int dir)
{
	LINE *curline;
	int curoff;
	int c;

	curline = *pcurline;
	curoff = *pcuroff;

	if (dir == FORWARD) {
		if (curoff == llength(curline)) {	/* if at EOL */
			curline = lforw(curline);	/* skip to next line */
			curoff = 0;
			c = '\r';	/* and return a <NL> */
		} else
			c = lgetc(curline, curoff++);	/* get the char */
	} else {		/* Reverse. */
		if (curoff == 0) {
			curline = lback(curline);
			curoff = llength(curline);
			c = '\r';
		} else
			c = lgetc(curline, --curoff);

	}
	*pcurline = curline;
	*pcuroff = curoff;

	return c;
}

#if MAGIC
/*
 * mcstr -- Set up the 'magic' array.  The closure symbol is taken as
 *	a literal character when (1) it is the first character in the
 *	pattern, and (2) when preceded by a symbol that does not allow
 *	closure, such as beginning or end of line symbol, or another
 *	closure symbol.
 *
 *	Coding comment (jmg):  yes, i know i have gotos that are, strictly
 *	speaking, unnecessary.  But right now we are so cramped for
 *	code space that i will grab what i can in order to remain
 *	within the 64K limit.  C compilers actually do very little
 *	in the way of optimizing - they expect you to do that.
 */
int
mcstr(void)
{
	MC *mcptr, *rtpcm;
	char *patptr;
	int pchr;
	int status = TRUE;
	int does_closure = FALSE;
	int mj = 0;
	int group_level = 0;

	/*
	 * If we had metacharacters in the MC array previously, free up any
	 * bitmaps that may have been allocated, and reset magical.
	 */
	if (magical)
		mcclear();

	mcptr = &mcpat[0];
	patptr = &pat[0];

	while ((pchr = *patptr) && status) {
		switch (pchr) {
		case MC_CCL:
			status = cclmake(&patptr, mcptr);
			magical = TRUE;
			does_closure = TRUE;
			break;

		case MC_BOL:
			/*
			 * If the BOL character isn't the
			 * first in the pattern, we assume
			 * it's a literal instead.
			 */
			if (mj != 0)
				goto litcase;

			mcptr->mc_type = BOL;
			magical = TRUE;
			break;

		case MC_EOL:
			/*
			 * If the EOL character isn't the
			 * last in the pattern, we assume
			 * it's a literal instead.
			 */
			if (*(patptr + 1) != '\0')
				goto litcase;

			mcptr->mc_type = EOL;
			magical = TRUE;
			break;

		case MC_ANY:
			mcptr->mc_type = ANY;
			magical = TRUE;
			does_closure = TRUE;
			break;

		case MC_CLOSURE:
			/*
			 * Does the closure symbol mean closure here?
			 * If so, back up to the previous element
			 * and indicate it is enclosed.  A bit is
			 * set in gr_closure in case we enclose a
			 * group.
			 */
			if (does_closure == FALSE)
				goto litcase;
			mj--;
			mcptr--;
			mcptr->mc_type |= CLOSURE;
			if (mcptr->mc_type & GRPEND)
				gr_closure |= BIT(group_level + 1);

			magical = TRUE;
			does_closure = FALSE;
			break;
		case MC_ESC:
			/*
			 * Note: no break between MC_ESC case
			 * and the default.
			 */
			if (*(patptr + 1) != '\0') {
				pchr = *++patptr;
				magical = TRUE;
			}
		default:
	litcase:	mcptr->mc_type = LITCHAR;
			mcptr->u.lchar = pchr;
			does_closure = TRUE;
			break;
		}		/* End of switch. */
		mcptr++;
		patptr++;
		mj++;
	}			/* End of while. */

	/*
	 * Close off the meta-string, then set up the reverse array,
	 * if the status is good.
	 * Please note the structure assignment - your compiler may
	 * not like that.
	 *
	 * The beginning and end of groups are reversed by flipping
	 * the bits, if they are set.
	 *
	 * If the status is not good, nil out the meta-pattern.
	 * The only way the status would be bad is from the cclmake()
	 * routine, and the bitmap for that member is guarenteed to be
	 * freed.  So we stomp a MCNIL value there, and call mcclear()
	 * to free any other bitmaps.
	 */
	mcptr->mc_type = MCNIL;

	if (status) {
		rtpcm = &tapcm[0];
		while (--mj >= 0) {
			*rtpcm = *--mcptr;
			if (rtpcm->mc_type & (GRPBEG | GRPEND))
				rtpcm->mc_type ^= (GRPBEG | GRPEND);
			rtpcm++;
		}
		rtpcm->mc_type = MCNIL;
	} else {
		(--mcptr)->mc_type = MCNIL;
		mcclear();
	}

	return status;
}

/*
 * rmcstr -- Set up the replacement 'magic' array.  Note that if there
 *	are no meta-characters encountered in the replacement string,
 *	the array is never actually created - we will just use the
 *	character array rpat[] as the replacement string.
 */
int
rmcstr(void)
{
	RMC *rmcptr;
	char *patptr;
	int status = TRUE;
	int mj;

	patptr = &rpat[0];
	rmcptr = &rmcpat[0];
	mj = 0;
	rmagical = FALSE;

	while (*patptr && status == TRUE) {
		switch (*patptr) {
		case MC_DITTO:

			/*
			 * If there were non-magical characters in the string
			 * before reaching this character, plunk it in the
			 * replacement array before processing the current
			 * meta-character.
			 */
			if (mj != 0) {
				rmcptr->mc_type = LITCHAR;
				if ((rmcptr->rstr = (void*)MALLOC(mj + 1)) == NULL) {
					mlwrite(TEXT94);
					/* "%%Out of memory" */
					status = FALSE;
					break;
				}
				bytecopy(rmcptr->rstr, patptr - mj, mj);
				rmcptr++;
				mj = 0;
			}
			rmcptr->mc_type = DITTO;
			rmcptr++;
			rmagical = TRUE;
			break;

		case MC_ESC:
			rmcptr->mc_type = LITCHAR;

			/*
			 * We malloc mj plus two here, instead of one,
			 * because we have to count the current character.
			 */
			if ((rmcptr->rstr = (void*)MALLOC(mj + 2)) == NULL) {
				mlwrite(TEXT94);
				/* "%%Out of memory" */
				status = FALSE;
				break;
			}
			bytecopy(rmcptr->rstr, patptr - mj, mj + 1);

			/*
			 * If MC_ESC is not the last character in the string,
			 * find out what it is escaping, and overwrite the
			 * last character with it.
			 */
			if (*(patptr + 1) != '\0')
				*((rmcptr->rstr) + mj) = *++patptr;

			rmcptr++;
			mj = 0;
			rmagical = TRUE;
			break;

		default:
			mj++;
		}
		patptr++;
	}

	if (rmagical && mj > 0) {
		rmcptr->mc_type = LITCHAR;
		if ((rmcptr->rstr = (void*)MALLOC(mj + 1)) == NULL) {
			mlwrite(TEXT94);
			/* "%%Out of memory" */
			status = FALSE;
		}
		bytecopy(rmcptr->rstr, patptr - mj, mj);
		rmcptr++;
	}
	rmcptr->mc_type = MCNIL;

	return TRUE;
}

/*
 * mcclear -- Free up any CCL bitmaps, and MCNIL the MC search arrays.
 */
int
mcclear(void)
{
	MC *mcptr;

	mcptr = &mcpat[0];

	while (mcptr->mc_type != MCNIL) {
		if ((mcptr->mc_type & MASKCL) == CCL ||
		    (mcptr->mc_type & MASKCL) == NCCL)
			FREE(mcptr->u.cclmap);
		mcptr++;
	}
	mcpat[0].mc_type = tapcm[0].mc_type = MCNIL;
	magical = FALSE;
	gr_closure = 0;

	return TRUE;
}

/*
 * rmcclear -- Free up any strings, and MCNIL the RMC array.
 */
int
rmcclear(void)
{
	RMC *rmcptr;

	rmcptr = &rmcpat[0];

	while (rmcptr->mc_type != MCNIL) {
		if (rmcptr->mc_type == LITCHAR)
			FREE(rmcptr->rstr);
		rmcptr++;
	}

	rmcpat[0].mc_type = MCNIL;

	return TRUE;
}

/*
 * mceq -- meta-character equality with a character.  In Kernighan & Plauger's
 *	Software Tools, this is the function omatch(), but i felt there
 *	were too many functions with the 'match' name already.
 */
int
mceq(int bc, MC *mt)
{
	int result;

	bc = bc & 255;
	switch (mt->mc_type & MASKCL) {
	case LITCHAR:
		result = eq(bc, (mt->u.lchar) & 255);
		break;

	case ANY:
		result = (bc != '\r');
		break;

	case CCL:
		if (!(result = biteq(bc, mt->u.cclmap))) {
			if ((curwp->w_bufp->b_mode & MDEXACT) == 0 &&
			    (isletter(bc))) {
				result = biteq(CHCASE(bc), mt->u.cclmap);
			}
		}
		break;

	case NCCL:
		result = !biteq(bc, mt->u.cclmap);
		if ((curwp->w_bufp->b_mode & MDEXACT) == 0 &&
		    (isletter(bc))) {
			result &= !biteq(CHCASE(bc), mt->u.cclmap);
		}
		break;

	default:
		mlwrite(TEXT95, mt->mc_type);
		/* "%%mceq: what is %d?" */
		result = FALSE;
		break;

	}			/* End of switch. */

	return result;
}

/*
 * cclmake -- create the bitmap for the character class.
 *	ppatptr is left pointing to the end-of-character-class character,
 *	so that a loop may automatically increment with safety.
 */
int
cclmake(char **ppatptr, MC *mcptr)
{
	BITMAP bmap;
	char *patptr;
	int pchr, ochr;

	if ((bmap = clearbits()) == NULL) {
		mlwrite(TEXT94);
		/* "%%Out of memory" */
		return FALSE;
	}
	mcptr->u.cclmap = bmap;
	patptr = *ppatptr;

	/*
	 * Test the initial character(s) in ccl for
	 * special cases - negate ccl, or an end ccl
	 * character as a first character.  Anything
	 * else gets set in the bitmap.
	 */
	if (*++patptr == MC_NCCL) {
		patptr++;
		mcptr->mc_type = NCCL;
	} else
		mcptr->mc_type = CCL;

	if ((ochr = *patptr) == MC_ECCL) {
		mlwrite(TEXT96);
		/* "%%No characters in character class" */
		FREE(bmap);
		return FALSE;
	} else {
		if (ochr == MC_ESC)
			ochr = *++patptr;

		setbit(ochr, bmap);
		patptr++;
	}

	while (ochr != '\0' && (pchr = *patptr) != MC_ECCL) {
		switch (pchr) {
			/*
			 * Range character loses its meaning if it is the
			 * last character in the class.
			 */
		case MC_RCCL:
			if (*(patptr + 1) == MC_ECCL)
				setbit(pchr, bmap);
			else {
				pchr = *++patptr;
				while (++ochr <= pchr)
					setbit(ochr, bmap);
			}
			break;

			/*
			 * Note: no break between case MC_ESC and the
			 * default.
			 */
		case MC_ESC:
			pchr = *++patptr;
		default:
			setbit(pchr, bmap);
			break;
		}
		patptr++;
		ochr = pchr;
	}

	*ppatptr = patptr;

	if (ochr == '\0') {
		mlwrite(TEXT97);
		/* "%%Character class not ended" */
		FREE(bmap);
		return FALSE;
	}
	return TRUE;
}

/*
 * biteq -- is the character in the bitmap?
 */
int
biteq(int bc, BITMAP cclmap)
{
	if ((bc & 255) >= HICHAR)
		return FALSE;

	return (*(cclmap + (bc >> 3)) & BIT(bc & 7)) ? TRUE : FALSE;
}

/*
 * clearbits -- Allocate and zero out a CCL bitmap.
 */
BITMAP
clearbits(void)
{
	BITMAP cclstart, cclmap;
	int j;

	if ((cclmap = cclstart = (BITMAP)MALLOC(HIBYTE)) != NULL)
		for (j = 0; j < (HIBYTE); j++)
			*cclmap++ = 0;

	return cclstart;
}

/*
 * setbit -- Set a bit (ON only) in the bitmap.
 */
int
setbit(int bc, BITMAP cclmap)
{
	if (bc < HICHAR)
		*(cclmap + (bc >> 3)) |= BIT(bc & 7);

	return TRUE;
}
#endif
