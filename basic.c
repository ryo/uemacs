/*
 * $Id: basic.c,v 1.14 2017/01/02 18:14:48 ryo Exp $
 *
 * The routines in this file move the cursor around on the screen. They
 * compute a new value for the cursor, then adjust ".". The display code
 * always updates the cursor location, so only moves between lines, or
 * functions that adjust the top line in the window and invalidate the
 * framing, are hard.
 */
#include <stdio.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "kanji.h"

short int eolflag;

/*
 * Move the cursor to the
 * beginning of the current line.
 * Trivial.
 */
int
gotobol(int f, int n)
{
	eolflag = 0;

	curwp->w_doto = 0;
	return TRUE;
}

/*
 * Move the cursor backwards by "n" characters. If "n" is less than zero call
 * "forwchar" to actually do the move. Otherwise compute the new cursor
 * location. Error if you try and move out of the buffer. Set the flag if the
 * line pointer for dot changes.
 */
int
backchar(int f, int n)
{
	LINE *lp;

	eolflag = 0;

	if (n < 0)
		return forwchar(f, -n);
	while (n--) {
		if (curwp->w_doto == 0) {
			if ((lp = lback(curwp->w_dotp)) == curbp->b_linep)
				return FALSE;
			curwp->w_dotp = lp;
			curwp->w_doto = llength(lp);
			curwp->w_flag |= WFMOVE;
		} else {
			curwp->w_doto--;
			if (nthctype(curwp->w_dotp->l_text, curwp->w_doto) == CT_KJ2)
				curwp->w_doto--;
		}
	}
	return TRUE;
}

/*
 * Move the cursor to the end of the current line. Trivial. No errors.
 */
int
gotoeol(int f, int n)
{
	eolflag = 1;

	curwp->w_doto = llength(curwp->w_dotp);
	return TRUE;
}

/*
 * Move the cursor forwards by "n" characters. If "n" is less than zero call
 * "backchar" to actually do the move. Otherwise compute the new cursor
 * location, and move ".". Error if you try and move off the end of the
 * buffer. Set the flag if the line pointer for dot changes.
 */
int
forwchar(int f, int n)
{
	eolflag = 0;

	if (n < 0)
		return backchar(f, -n);
	while (n--) {
		if (curwp->w_doto == llength(curwp->w_dotp)) {
			if (curwp->w_dotp == curbp->b_linep)
				return FALSE;
			curwp->w_dotp = lforw(curwp->w_dotp);
			curwp->w_doto = 0;
			curwp->w_flag |= WFMOVE;
		} else {
			curwp->w_doto++;
			if ((nthctype(curwp->w_dotp->l_text, curwp->w_doto) == CT_KJ2) &&
			   (curwp->w_doto < llength(curwp->w_dotp)))
				curwp->w_doto++;
		}
	}
	return TRUE;
}

int
gotoline(int f, int n)
{
	/*
	 * move to a particular line. argument (n)
	 * must be a positive integer for this to
	 * actually do anything
	 */

	int status;		/* status return */
	char arg[NSTRING];	/* buffer to hold argument */

	eolflag = 0;

	/* get an argument if one doesnt exist */
	if (f == FALSE) {
		if ((status = mlreply(TEXT7, arg, NSTRING)) != TRUE) {
			/* "Line to GOTO: " */
			mlwrite(TEXT8);
			/* "[Aborted]" */
			return status;
		}
		n = asc_int(arg);
	}
	if (n < 1)		/* if a bogus argument...then leave */
		return FALSE;

	/* first, we go to the start of the buffer */
	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	return forwline(f, n - 1);
}

/*
 * Goto the beginning of the buffer. Massive adjustment of dot. This is
 * considered to be hard motion; it really isn't if the original value of dot
 * is the same as the new value of dot. Normally bound to "M-<".
 */
int
gotobob(int f, int n)
{
	eolflag = 0;

	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
 * Move to the end of the buffer. Dot is always put at the end of the file
 * (ZJ). The standard screen code does most of the hard parts of update.
 * Bound to "M->".
 */
int
gotoeob(int f, int n)
{
	eolflag = 0;

	curwp->w_dotp = curbp->b_linep;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
 * Move forward by full lines. If the number of lines to move is less than
 * zero, call the backward line function to actually do it. The last command
 * controls how the goal column is set. Bound to "C-N". No errors are
 * possible.
 */
int
forwline(int f, int n)
{
	LINE *dlp;

	if (n < 0)
		return backline(f, -n);

	/* if we are on the last line as we start....fail the command */
	if (curwp->w_dotp == curbp->b_linep)
		return FALSE;

	/*
	 * if the last command was not note a line move, reset the goal
	 * column
	 */
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);

	/* flag this command as a line move */
	thisflag |= CFCPCN;

	/* and move the point down */
	dlp = curwp->w_dotp;
	while (n-- && dlp != curbp->b_linep)
		dlp = lforw(dlp);

	/* reseting the current position */
	curwp->w_dotp = dlp;
	curwp->w_doto = getgoal(dlp);
	curwp->w_flag |= WFMOVE;

	if (eolflag)
		gotoeol(FALSE, 0);

	return TRUE;
}

/*
 * This function is like "forwline", but goes backwards. The scheme is exactly
 * the same. Check for arguments that are less than zero and call your
 * alternate. Figure out the new line and call "movedot" to perform the
 * motion. No errors are possible. Bound to "C-P".
 */
int
backline(int f, int n)
{
	LINE *dlp;

	if (n < 0)
		return forwline(f, -n);


	/* if we are on the last line as we start....fail the command */
	if (lback(curwp->w_dotp) == curbp->b_linep)
		return FALSE;

	/*
	 * if the last command was not note a line move, reset the goal
	 * column
	 */
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);

	/* flag this command as a line move */
	thisflag |= CFCPCN;

	/* and move the point up */
	dlp = curwp->w_dotp;
	while (n-- && lback(dlp) != curbp->b_linep)
		dlp = lback(dlp);

	/* reseting the current position */
	curwp->w_dotp = dlp;
	curwp->w_doto = getgoal(dlp);
	curwp->w_flag |= WFMOVE;

	if (eolflag)
		gotoeol(FALSE, 0);

	return TRUE;
}

#if WORDPRO
/*
 * go back to the beginning of the current paragraph here
 * we look for a <NL><NL> or <NL><TAB> or <NL><SPACE> combination
 * to delimit the beginning of a paragraph
 */
int
gotobop(int f, int n)
{
	int suc;		/* success of last backchar */

	eolflag = 0;

	if (n < 0)		/* the other way... */
		return gotoeop(f, -n);

	while (n-- > 0) {	/* for each one asked for */

		/* first scan back until we are in a word */
		suc = backchar(FALSE, 1);
		while (!inword() && suc)
			suc = backchar(FALSE, 1);
		curwp->w_doto = 0;	/* and go to the B-O-Line */

		/*
		 * and scan back until we hit a <NL><NL> or <NL><TAB> or a
		 * <NL><SPACE>
		 */
		while (lback(curwp->w_dotp) != curbp->b_linep)
			if (llength(curwp->w_dotp) != 0 &&
			    lgetc(curwp->w_dotp, curwp->w_doto) != TAB &&
			    lgetc(curwp->w_dotp, curwp->w_doto) != ' ')
				curwp->w_dotp = lback(curwp->w_dotp);
			else
				break;

		/* and then forward until we are in a word */
		suc = forwchar(FALSE, 1);
		while (suc && !inword())
			suc = forwchar(FALSE, 1);
	}
	curwp->w_flag |= WFMOVE;/* force screen update */
	return TRUE;
}

/*
 * go forword to the end of the current paragraph here
 * we look for a <NL><NL> or <NL><TAB> or <NL><SPACE> combination
 * to delimit the beginning of a paragraph
 */
int
gotoeop(int f, int n)
{
	int suc;		/* success of last backchar */

	eolflag = 0;

	if (n < 0)		/* the other way... */
		return gotobop(f, -n);

	while (n-- > 0) {	/* for each one asked for */

		/* first scan forward until we are in a word */
		suc = forwchar(FALSE, 1);
		while (!inword() && suc)
			suc = forwchar(FALSE, 1);
		curwp->w_doto = 0;	/* and go to the B-O-Line */
		if (suc)	/* of next line if not at EOF */
			curwp->w_dotp = lforw(curwp->w_dotp);

		/*
		 * and scan forword until we hit a <NL><NL> or <NL><TAB> or a
		 * <NL><SPACE>
		 */
		while (curwp->w_dotp != curbp->b_linep) {
			if (llength(curwp->w_dotp) != 0 &&
			    lgetc(curwp->w_dotp, curwp->w_doto) != TAB &&
			    lgetc(curwp->w_dotp, curwp->w_doto) != ' ')
				curwp->w_dotp = lforw(curwp->w_dotp);
			else
				break;
		}

		/* and then backward until we are in a word */
		suc = backchar(FALSE, 1);
		while (suc && !inword()) {
			suc = backchar(FALSE, 1);
		}
		curwp->w_doto = llength(curwp->w_dotp);	/* and to the EOL */
	}
	curwp->w_flag |= WFMOVE;/* force screen update */
	return TRUE;
}
#endif

/*
 * This routine, given a pointer to a LINE, and the current cursor goal
 * column, return the best choice for the offset. The offset is returned.
 * Used by "C-N" and "C-P".
 */
int
getgoal(LINE *dlp)
{
	int c;
	int col;
	int newcol;
	int dbo;

	col = 0;
	dbo = 0;
	while (dbo != llength(dlp)) {
		c = lgetc(dlp, dbo);
		newcol = col;
		if (c == '\t')
			newcol += -(newcol % curbp->b_tabs) + (curbp->b_tabs - 1);
		else
			if (c < 0x20 || c == 0x7F)
				++newcol;
		++newcol;
		if (newcol > curgoal)
			break;
		col = newcol;
		dbo++;
	}
	if (nthctype(dlp->l_text, dbo) == CT_KJ2)
		dbo--;

	return dbo;
}

/*
 * Scroll forward by a specified number of lines, or by a full page if no
 * argument. Bound to "C-V". The "2" in the arithmetic on the window size is
 * the overlap; this value is the default overlap value in ITS EMACS. Because
 * this zaps the top line in the display window, we have to do a hard update.
 */
int
forwpage(int f, int n)
{
	LINE *lp;

	if (f == FALSE) {
		n = curwp->w_ntrows - 2;	/* Default scroll.      */
		if (n <= 0)	/* Forget the overlap   */
			n = 1;	/* if tiny window.      */
	} else
		if (n < 0)
			return backpage(f, -n);
#if CVMVAS
		else		/* Convert from pages   */
			n *= curwp->w_ntrows;	/* to lines.            */
#endif
	lp = curwp->w_linep;
	while (n-- && lp != curbp->b_linep)
		lp = lforw(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;

	if (eolflag)
		gotoeol(FALSE, 0);

	return TRUE;
}

/*
 * This command is like "forwpage", but it goes backwards. The "2", like
 * above, is the overlap between the two windows. The value is from the ITS
 * EMACS manual. Bound to "M-V". We do a hard update for exactly the same
 * reason.
 */
int
backpage(int f, int n)
{
	LINE *lp;

	if (f == FALSE) {
		n = curwp->w_ntrows - 2;	/* Default scroll.      */
		if (n <= 0)	/* Don't blow up if the */
			n = 1;	/* window is tiny.      */
	} else
		if (n < 0)
			return forwpage(f, -n);
#if CVMVAS
		else		/* Convert from pages   */
			n *= curwp->w_ntrows;	/* to lines.            */
#endif
	lp = curwp->w_linep;
	while (n-- && lback(lp) != curbp->b_linep)
		lp = lback(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;

	if (eolflag)
		gotoeol(FALSE, 0);

	return TRUE;
}

/*
 * Set the mark in the current window to the value of "." in the window. No
 * errors are possible. Bound to "M-.".
 */
int
setmark(int f, int n)
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

	curwp->w_markp[n] = curwp->w_dotp;
	curwp->w_marko[n] = curwp->w_doto;

	mlwrite(TEXT9, n);
	/* "[Mark %d set]" */
	return TRUE;
}

/*
 * Remove the mark in the current window.
 * Bound to ^X <space>
 */
int
remmark(int f, int n)
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

	curwp->w_markp[n] = NULL;
	curwp->w_marko[n] = 0;
	mlwrite(TEXT10, n);
	/* "[Mark %d removed]" */
	return TRUE;
}

/*
 * Swap the values of "." and "mark" in the current window. This is pretty
 * easy, bacause all of the hard work gets done by the standard routine
 * that moves the mark about. The only possible error is "no mark". Bound to
 * "C-X C-X".
 */
int
swapmark(int f, int n)
{
	LINE *odotp;
	int odoto;

	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

	if (curwp->w_markp[n] == NULL) {
		mlwrite(TEXT11, n);
		/* "No mark %d in this window" */
		return FALSE;
	}
	odotp = curwp->w_dotp;
	odoto = curwp->w_doto;
	curwp->w_dotp = curwp->w_markp[n];
	curwp->w_doto = curwp->w_marko[n];
	curwp->w_markp[n] = odotp;
	curwp->w_marko[n] = odoto;
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

/*
 * Goto a mark in the current window. This is pretty easy, bacause all of
 * the hard work gets done by the standard routine that moves the mark
 * about. The only possible error is "no mark". Bound to "M-^G".
 */
int
gotomark(int f, int n)
{
	/* make sure it is in range */
	if (f == FALSE)
		n = 0;
	n %= NMARKS;

	if (curwp->w_markp[n] == NULL) {
		mlwrite(TEXT11, n);
		/* "No mark %d in this window" */
		return FALSE;
	}
	curwp->w_dotp = curwp->w_markp[n];
	curwp->w_doto = curwp->w_marko[n];
	curwp->w_flag |= WFMOVE;
	return TRUE;
}
