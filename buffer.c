/*
 * $Id: buffer.c,v 1.29 2017/01/02 15:17:50 ryo Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "keyword.h"

int
usebuffer(int f, int n)
{
	BUFFER *bp;

	bp = getdefb();
	bp = getcbuf(TEXT24, bp ? bp->b_bname : "*scratch*", TRUE);
	if (!bp)
		return ABORT;

	if (f == TRUE)
		bp->b_flag |= BFINVS;

	return swbuffer(bp);
}

int
nextbuffer(int f, int n)
{
	BUFFER *bp;
	int status;

	if (f == FALSE)
		n = 1;
	if (n < 1)
		return FALSE;

	while (n-- > 0) {
		bp = getdefb();
		if (bp == NULL)
			return FALSE;

		status = swbuffer(bp);
		if (status != TRUE)
			return status;
	}
	return TRUE;
}

int
swbuffer(BUFFER *bp)
{
	WINDOW *wp;
	int cmark;
	int do_postreadhook = 0;

	/* let a user macro get hold of things...if he wants */
	exechook(exbhook);

	if (--curbp->b_nwnd == 0) {
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		for (cmark = 0; cmark < NMARKS; cmark++) {
			curbp->b_markp[cmark] = curwp->w_markp[cmark];
			curbp->b_marko[cmark] = curwp->w_marko[cmark];
		}
		curbp->b_fcol = curwp->w_fcol;
	}
	curbp = bp;
	if (curbp->b_active != TRUE) {
		/* read it in and activate it */
		readin(curbp->b_fname, TRUE);
		do_postreadhook = 1;
		curbp->b_dotp = lforw(curbp->b_linep);
		curbp->b_doto = 0;
		curbp->b_active = TRUE;
	}
	curwp->w_bufp = bp;
	curwp->w_linep = bp->b_linep;
	curwp->w_flag |= WFMODE | WFFORCE | WFHARD;
	if (bp->b_nwnd++ == 0) {/* First use. */
		curwp->w_dotp = bp->b_dotp;
		curwp->w_doto = bp->b_doto;
		for (cmark = 0; cmark < NMARKS; cmark++) {
			curwp->w_markp[cmark] = bp->b_markp[cmark];
			curwp->w_marko[cmark] = bp->b_marko[cmark];
		}
		curwp->w_fcol = bp->b_fcol;
	} else {
		wp = wheadp;
		while (wp != NULL) {
			if (wp != curwp && wp->w_bufp == bp) {
				curwp->w_dotp = wp->w_dotp;
				curwp->w_doto = wp->w_doto;
				for (cmark = 0; cmark < NMARKS; cmark++) {
					curwp->w_markp[cmark] = wp->w_markp[cmark];
					curwp->w_marko[cmark] = wp->w_marko[cmark];
				}
				curwp->w_fcol = wp->w_fcol;
				break;
			}
			wp = wp->w_wndp;
		}
	}

	if (do_postreadhook)
		exechook(postreadhook);

	/* let a user macro get hold of things...if he wants */
	exechook(bufhook);

	return TRUE;
}

int
killbuffer(int f, int n)
{
	BUFFER *bp;

	bp = getdefb();
	bp = getcbuf(TEXT26, bp ? bp->b_bname : "*scratch*", TRUE);
	/* "Kill buffer" */
	if (bp == NULL)
		return ABORT;

	return zotbuf(bp);
}

/* get the default buffer for a use or kill */
BUFFER *
getdefb(void)
{
	BUFFER *bp;

	/* Find the next buffer, which will be the default */
	bp = curbp->b_bufp;

	/* cycle through the buffers to find an eligable one */
	while (bp == NULL || bp->b_flag & BFINVS) {
		if (bp == NULL)
			bp = bheadp;
		else
			bp = bp->b_bufp;

		/* don't get caught in an infinite loop! */
		if (bp == curbp) {
			bp = NULL;
			break;
		}
	}
	return bp;
}


/* kill the buffer pointed to by bp */
int
zotbuf(BUFFER *bp)
{
	BUFFER *bp1;
	BUFFER *bp2;
	int s;

	if (bp->b_nwnd != 0) {	/* Error if on screen.	 */
		mlwrite(TEXT28);
		/* "Buffer is being displayed" */
		return FALSE;
	}
	if ((s = bclear(bp)) != TRUE)	/* Blow text away.	 */
		return s;
	FREE((char *) bp->b_linep);	/* Release header line. */
	bp1 = NULL;		/* Find the header.	 */
	bp2 = bheadp;
	while (bp2 != bp) {
		bp1 = bp2;
		bp2 = bp2->b_bufp;
	}
	bp2 = bp2->b_bufp;	/* Next one in chain.	 */
	if (bp1 == NULL)	/* Unlink it.		 */
		bheadp = bp2;
	else
		bp1->b_bufp = bp2;

	hash_destroy_root(bp->b_hashroot);

	FREE((char *) bp);	/* Release buffer block */
	return TRUE;
}


/* Rename the current buffer	 */
int
namebuffer(int f, int n)
{
	BUFFER *bp;
	char bufn[NBUFN];

	/* prompt for and get the new buffer name */
ask:
	if (mlreply(TEXT29, bufn, NBUFN) != TRUE)
		/* "Change buffer name to: " */
		return FALSE;

	/* and check for duplicates */
	bp = bheadp;
	while (bp != NULL) {
		if (bp != curbp) {
			/* if the names the same */
			if (strcmp(bufn, bp->b_bname) == 0)
				goto ask;	/* try again */
		}
		bp = bp->b_bufp;	/* onward */
	}

	strncpy(curbp->b_bname, bufn, sizeof(bufn));	/* copy buffer name to structure */
	curwp->w_flag |= WFMODE;	/* make mode line replot */
	mlerase();
	return TRUE;
}

/*
 * List all of the active buffers.  First update the special buffer that holds the list.
 * Next make sure at least 1 window is displaying the buffer list,
 * splitting the screen if this is what it takes.
 * Lastly, repaint all of the windows that are displaying the list.
 * Bound to "C-X C-B".
 * A numeric argument forces it to list invisable buffers as well.
 */
int
listbuffers(int f, int n)
{
	WINDOW *wp;
	BUFFER *bp;
	int s;
	int cmark;	/* current mark */

	if ((s = makelist(f)) != TRUE)
		return s;
	if (blistp->b_nwnd == 0) {	/* Not on screen yet.	 */
		if ((wp = wpopup()) == NULL)
			return FALSE;
		bp = wp->w_bufp;
		if (--bp->b_nwnd == 0) {
			bp->b_dotp = wp->w_dotp;
			bp->b_doto = wp->w_doto;
			for (cmark = 0; cmark < NMARKS; cmark++) {
				bp->b_markp[cmark] = wp->w_markp[cmark];
				bp->b_marko[cmark] = wp->w_marko[cmark];
			}
			bp->b_fcol = wp->w_fcol;
		}
		wp->w_bufp = blistp;
		++blistp->b_nwnd;
	}
	wp = wheadp;
	while (wp != NULL) {
		if (wp->w_bufp == blistp) {
			wp->w_linep = lforw(blistp->b_linep);
			wp->w_dotp = lforw(blistp->b_linep);
			wp->w_doto = 0;
			for (cmark = 0; cmark < NMARKS; cmark++) {
				wp->w_markp[cmark] = NULL;
				wp->w_marko[cmark] = 0;
			}
			wp->w_flag |= WFMODE | WFHARD;
		}
		wp = wp->w_wndp;
	}
	return TRUE;
}


/*
 * This routine rebuilds the text in the special secret buffer that holds the buffer list.
 * It is called  by the list buffers command.
 * Return TRUE if everything works.
 * Return FALSE if there is an error (if there is no memory).
 * Iflag indecates weather to list hidden buffers.
 */
int
makelist(int iflag)
{
	char *cp1;
	BUFFER *bp;
	LINE *lp;
	int s;
	int i;
	unsigned int nbytes;	/* # of bytes in current buffer */
	char line[NSTRING];

	blistp = bfind("[List]", TRUE, BFINVS); /* Buffer list buffer    */

	blistp->b_flag &= ~BFCHG;	/* Don't complain!	 */
	if ((s = bclear(blistp)) != TRUE)	/* Blow old text away	 */
		return s;
	strcpy(blistp->b_fname, "");

	if (addline(TEXT30) == FALSE ||  /* "ACT   Modes           Size Buffer               File" */
	    addline(TEXT30a) == FALSE)   /* "--- ----------- ---------- -------------------- ----" */
		return FALSE;
	bp = bheadp;		/* For all buffers	 */

	/* build line to report global mode settings */
	cp1 = &line[0];
	*cp1++ = ' ';
	*cp1++ = ' ';
	*cp1++ = ' ';
	*cp1++ = ' ';

	/* output the mode codes */
	for (i = 0; i < NUMMODES; i++)
		if (gmode & (1 << i))
			*cp1++ = modecode[i];
		else
			*cp1++ = '.';
	strcpy(cp1, TEXT31);
	/* "            Global Modes" */
	if (addline(line) == FALSE)
		return FALSE;


	while (bp != NULL) {
		/* skip invisable buffers if iflag is false */
		if (((bp->b_flag & BFINVS) != 0) && (iflag != TRUE)) {
			bp = bp->b_bufp;
			continue;
		}
		cp1 = &line[0];	/* Start at left edge */

		/* output status of ACTIVE flag (has the file been read in? */
		if (bp->b_active == TRUE)	/* "@" if activated       */
			*cp1++ = '@';
		else
			*cp1++ = ' ';


		/* output status of changed flag */
		if ((bp->b_flag & BFCHG) != 0)	/* "*" if changed	 */
			*cp1++ = '*';
		else
			*cp1++ = ' ';


		/* report if the file is truncated */
		if ((bp->b_flag & BFTRUNC) != 0)
			*cp1++ = '#';
		else
			*cp1++ = ' ';

		*cp1++ = ' ';	/* space */


		/* output the mode codes */
		for (i = 0; i < NUMMODES; i++) {
			if (bp->b_mode & (1 << i))
				*cp1++ = modecode[i];
			else
				*cp1++ = '.';
		}
		*cp1++ = ' ';	/* Gap. 		 */
		nbytes = 0L;	/* Count bytes in buf.	 */
		lp = lforw(bp->b_linep);


		while (lp != bp->b_linep) {
			nbytes += (long) llength(lp) + 1L;
			lp = lforw(lp);
		}


		sprintf(cp1, "%10u %-20s %s",
			nbytes,
			bp->b_bname,
			bp->b_fname
			);

		if (addline(line) == FALSE)
			return FALSE;

		bp = bp->b_bufp;
	}
	return TRUE;		/* All done */
}

/*
 * The argument "text" points to a string.
 * Append this line to the buffer list buffer.
 * Handcraft the EOL on the end.
 * Return TRUE if it worked and FALSE if you ran out of room.
 */
int
addline(char *text)
{
	LINE *lp;
	int i;
	int ntext;

	ntext = strlen(text);
	if ((lp = lalloc(ntext)) == NULL)
		return FALSE;

	for (i = 0; i < ntext; ++i)
		lputc(lp, i, text[i]);
	blistp->b_linep->l_bp->l_fp = lp;	/* Hook onto the end	 */
	lp->l_bp = blistp->b_linep->l_bp;
	blistp->b_linep->l_bp = lp;
	lp->l_fp = blistp->b_linep;
	if (blistp->b_dotp == blistp->b_linep)	/* If "." is at the end */
		blistp->b_dotp = lp;	/* move it to new line	 */

	return TRUE;
}

/*
 * Look through the list of buffers.
 * Return TRUE if there are any changed buffers.
 * Buffers that hold magic internal stuff are not considered;
 * who cares if the list of buffer names is hacked.
 * Return FALSE if no buffers have been changed.
 */
int
anycb(void)
{
	BUFFER *bp;

	bp = bheadp;
	while (bp != NULL) {
		if ((bp->b_flag & BFINVS) == 0 && (bp->b_flag & BFCHG) != 0)
			return TRUE;
		bp = bp->b_bufp;
	}
	return FALSE;
}

/*
 * Find a buffer, by name. Return a pointer to the BUFFER structure associated with it.
 * If the buffer is not found and the "cflag" is TRUE, create it.
 * The "bflag" is the settings for the flags in in buffer.
 */
BUFFER *
bfind(char *bname, int cflag, int bflag)
{
	BUFFER *bp;
	BUFFER *sb;		/* buffer to insert after */
	LINE *lp;
	int cmark;		/* current mark */

	bp = bheadp;
	while (bp != NULL) {
		if (strcmp(bname, bp->b_bname) == 0)
			return bp;
		bp = bp->b_bufp;
	}
	if (cflag != FALSE) {
		if ((bp = (BUFFER *)MALLOC(sizeof(BUFFER))) == NULL)
			return NULL;
		if ((lp = lalloc(0)) == NULL) {
			FREE((char *) bp);
			return NULL;
		}
		/* find the place in the list to insert this buffer */
		if (bheadp == NULL || strcmp(bheadp->b_bname, bname) > 0) {
			/* insert at the beginning */
			bp->b_bufp = bheadp;
			bheadp = bp;
		} else {
			sb = bheadp;
			while (sb->b_bufp != NULL) {
				if (strcmp(sb->b_bufp->b_bname, bname) > 0)
					break;
				sb = sb->b_bufp;
			}

			/* and insert it */
			bp->b_bufp = sb->b_bufp;
			sb->b_bufp = bp;
		}

		/* and set up the other buffer fields */
		bp->b_topline = NULL;
		bp->b_botline = NULL;
		bp->b_active = TRUE;
		bp->b_dotp = lp;
		bp->b_doto = 0;
		for (cmark = 0; cmark < NMARKS; cmark++) {
			bp->b_markp[cmark] = NULL;
			bp->b_marko[cmark] = 0;
		}
		bp->b_hashroot = NULL;
		bp->b_fcol = 0;
		bp->b_flag = bflag;
		bp->b_mode = gmode;
		bp->b_eofreturn = TRUE;
		bp->b_bom = 0;
		bp->b_nwnd = 0;
		bp->b_linep = lp;
		bp->b_tabs = tabsize;
		bp->b_mlcol = mlcol;
		strcpy(bp->b_mlform, mlform);
		strcpy(bp->b_fname, "");
		strcpy(bp->b_bname, bname);
		bp->b_mtime = 0;
		bp->b_kanjicode = 0;
#if CRYPT
		bp->b_key[0] = 0;
#endif
		lp->l_fp = lp;
		lp->l_bp = lp;
	}
	return bp;
}

/*
 * This routine blows away all of the text in a buffer.
 * If the buffer is marked as changed then we ask if it is ok to blow it away;
 * this is to save the user the grief of losing text.
 * The window chain is nearly always wrong if this gets called;
 * the caller must arrange for the updates that are required.
 * Return TRUE if everything looks good.
 */
int
bclear(BUFFER *bp)
{
	LINE *lp;
	int s;
	int cmark;		/* current mark */

	if ((bp->b_flag & BFINVS) == 0	/* Not scratch buffer.	 */
	    && (bp->b_flag & BFCHG) != 0	/* Something changed	 */
	    && (s = mlyesno(TEXT32)) != TRUE)
		/* "Discard changes" */
		return s;
	bp->b_flag &= ~BFCHG;	/* Not changed		 */
	while ((lp = lforw(bp->b_linep)) != bp->b_linep)
		lfree(lp);
	bp->b_dotp = bp->b_linep;	/* Fix "."		 */
	bp->b_doto = 0;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		bp->b_markp[cmark] = NULL;	/* Invalidate "mark"    */
		bp->b_marko[cmark] = 0;
	}
	bp->b_fcol = 0;
	return TRUE;
}

/* unmark the current buffers change flag */
int
unmark(int f, int n)
{
	/* unmark the buffer */
	curbp->b_flag &= ~BFCHG;

	/* unmark all windows as well */
	upmode();

	return TRUE;
}
