/*
----------------------------------------
	BUFFER.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/buffer.c,v 1.5 1992/01/04 13:11:20 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static void long_asc(char *, int, int);
static int makelist1(int, int);
static int makelist2(int);

/*
----------------------------------------
	バッファ表示
----------------------------------------
*/

int usebuffer(int f, int n)
{
	BUFFER *bp;

	bp = getdefb();
	bp = getcbuf(KTEX24, bp ? bp->b_bname : "*scratch*", TRUE);
	if (!bp)
		return ABORT;
	if (bp->b_active != TRUE && *bp->b_fname == 0) {
		mlwrite(KTEX250, bp->b_bname);
		return FALSE;
	}
	if (f == TRUE)
		bp->b_flag |= BFINVS;
	return swbuffer(bp);
}

/*
----------------------------------------
	次のバッファ
----------------------------------------
*/

int nextbuffer(int f, int n)
{
	if (n < 1)
		return FALSE;
	while (n--) {
		int status;
		BUFFER *bp;

		bp = getdefb();
		if (bp == 0)
			return FALSE;
		status = swbuffer(bp);
		if (status != TRUE)
			return status;
	}
	return TRUE;
}

/*
========================================
	バッファをカレントに
========================================
*/

int swbuffer(BUFFER *bp)
{
	WINDOW *cwp;
	BUFFER *cbp;

	exechook(exbhook);
	cbp = curbp;
	cwp = curwp;

	--cbp->b_nwnd;
	copyinf_win_to_buf(cwp, cbp);

	curbp = cbp = bp;
	updinsf = TRUE;
	if (cbp->b_active != TRUE) {
		readin(FALSE, 0, cbp->b_fname);
		cbp->b_dotp = lforw(cbp->b_linep);
		cbp->b_doto = 0;
		cbp->b_active = TRUE;
	}
	cwp->w_bufp = bp;
	cwp->w_linep = bp->b_linep;
	cwp->w_flag |= WFMODE | WFFORCE | WFHARD;

	bp->b_nwnd++;
	copyinf_buf_to_win(bp, cwp);

	exechook(bufhook);

	return TRUE;
}

/*
----------------------------------------
	バッファ削除
----------------------------------------
*/

int killbuffer(int f, int n)
{
	BUFFER *bp;

	bp = getcbuf(KTEX26, curbp ? curbp->b_bname : "*scratch*", TRUE);
	return bp ? zotbuf(bp) : ABORT;
}

/*
========================================
	次のバッファを得る
========================================
*/

BUFFER *getdefb(void)
{
	BUFFER *bp;

	{
		BUFFER *last;
		int nbuf;

		for(nbuf = 0, last = NULL, bp = bheadp; bp; bp = bp->b_bufp) {
			if (!(bp->b_flag & BFINVS)) {
				last = bp;
				nbuf++;
			}
		}
		if (nbuf <= 1)
			return last;
	}

	bp = curbp;
	while (1) {
		bp = bp->b_bufp;
		if (bp == 0)
			bp = bheadp;
		if (bp != curbp && !(bp->b_flag & BFINVS))
			break;
	}
	return bp;
}

/*
========================================
	バッファ削除
========================================
*/

int zotbuf(BUFFER *bp)
{
	int nwin = 0;
	char wchg[52];

	{
		int	nbuf;

		{
			BUFFER	*bp;

			for(nbuf = 0, bp = bheadp; bp; bp = bp->b_bufp) {
				if ((bp->b_flag & BFINVS) == 0)
					nbuf++;
			}
		}
		if (nbuf <= 1) {
			if (!strcmp(bp->b_bname, "*scratch*")) {
				if (curbp == bp)
					mlwrite(KTEX28);
				return FALSE;
			}
			if (bfind("*scratch*", TRUE, 0) == NULL)
				return FALSE;
		}
	}

	if (bp->b_nwnd != 0) {
		int no;
		WINDOW *wp;

		for (nwin = 0, wp = wheadp; wp; wp = wp->w_wndp)
			wchg[nwin++] = 0;
		for (no = 0; no < nwin; no++) {
			if (curwp->w_bufp == bp) {
				if (nextbuffer(TRUE, 1) == TRUE)
					wchg[no] = 1;
				else {
					mlwrite(KTEX28);
					return FALSE;
				}
			}
			nextwind(FALSE, 0);
		}
	}
	if (bp == blistp || bp == bcompp || bp == bdiredp || bp == bdmarkp ||
		  bp == bhisexecp || bp == bhisenvp || bp == bhiscmdp || bp == bhissearchp ||
		  bp == bhisargp || bp == bhisdebugp || bp == bhiscmpbufp || bp == bhiscmpcp ||
		  bp == bhiscmpcomp || bp == bhiscmpfnamep || bp == bhiscmpgenp ||
		  bp == bhiscmplatexp || bp == bhiscmpmacp || bp == bhiscmpmodep ||
		  bp == bhiscmpvarp || bp == bhiscmpdnamep || bp == bdictp)
		return TRUE;
	if (bclear(bp) != TRUE) {
		int i;

		for (i = 0; i < nwin; i++) {
			if (wchg[i])
				swbuffer(bp);
			nextwind(FALSE, 0);
		}
		return FALSE;
	}
	free(bp->b_linep);
	if (bp->b_localvar)
	  free (bp->b_localvar);
	if (bp->b_comp_keyword)
	  free (bp->b_comp_keyword);
	if (bp->b_comp_keyword_set)
	  free (bp->b_comp_keyword_set);

	{
		BUFFER **bpn;

		for (bpn = &bheadp; *bpn != bp; bpn = &(*bpn)->b_bufp)
			;
		*bpn = bp->b_bufp;
	}
	free(bp);

	return TRUE;
}

/*
----------------------------------------
	バッファ名の設定
----------------------------------------
*/

int namebuffer(int f, int n)
{
	BUFFER *bp;
	char bufn[NBUFN];

retry_ask:

	his_disable();
	if (mlreply(KTEX29, bufn, NBUFN) != TRUE)
		return FALSE;
	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if (bp != curbp) {
			if (strcmp(bufn, bp->b_bname) == 0)
				goto retry_ask;
		}
	}
	strcpy(curbp->b_bname, bufn);
	curwp->w_flag |= WFMODE;
	mlerase();
	return TRUE;
}

/*
========================================
	リスト作成本体１
========================================
*/

static int makelist1(int iflag, int spcflag)
{
	char tmp[8];
	char line[NSTRING];
	BUFFER *bp;

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		char *cp1;

		if ((bp->b_flag & BFINVS) && iflag == FALSE)
			continue;

		cp1 = line;
		if (spcflag == TRUE) {
			*cp1++ = ' ';
			*cp1++ = ' ';
			*cp1++ = ' ';
		}
		*cp1++ = bp->b_active ? '@' : ' ';
		*cp1++ = (bp->b_flag & BFCHG) ? '*' : ' ';
		*cp1++ = (bp->b_flag & BFTRUNC) ? '#' : ' ';
		*cp1++ = ' ';

		{
			int i;

			for (i = 0; i < NUMMODES; i++)
				*cp1++ = (bp->b_mode & (1 << i)) ? modecode[i] : '.';
		}

		*cp1++ = ' ';

		{
			int nbyte;
			LINE *lp;

			for (nbyte = 0, lp = lforw(bp->b_linep); lp != bp->b_linep; lp = lforw(lp))
				nbyte += llength(lp) + 1;
			long_asc(tmp, 7, nbyte);
		}

		{
			char *cp2;

			for (cp2 = tmp; *cp2; *cp1++ = *cp2++);
			*cp1++ = ' ';
			for (cp2 = bp->b_bname; *cp2; *cp1++ = *cp2++);
			*cp1++ = ' ';
			cp2 = bp->b_fname;
			if (*cp2) {
				while (cp1 < line + 22 + NBUFN)
					*cp1++ = ' ';
				for (; *cp2; *cp1++ = *cp2++) {
					if (cp1 >= line + NSTRING - 1)
						break;
				}
			}
		}
		*cp1 = 0;
		if (addline(blistp, line) == FALSE)
			return FALSE;
	}

	winbob(blistp);
	return TRUE;
}

/*
----------------------------------------
	バッファリスト
----------------------------------------
*/

int makebuflist(int f, int n)
{
	int status;

	blistp->b_flag &= ~BFCHG;
	*blistp->b_fname = 0;
	status = bclear(blistp);
	if (status != TRUE)
		return status;
	return makelist1(f, TRUE);
}

/*
----------------------------------------
	バッファリスト
----------------------------------------
*/

int listbuffers(int f, int n)
{
	{
		int status;

		status = makelist2(f);
		if (status != TRUE)
			return status;
	}

	if (blistp->b_nwnd == 0) {
		WINDOW *wp;

		wp = wpopup();
		if (wp == 0)
			return FALSE;

		{
			WINDOW *cwp;

			cwp = curwp;
			for(; curwp != wp; nextwind(FALSE, 1));
			swbuffer(blistp);
			for(; curwp != cwp; nextwind(FALSE, 1));
		}
		winbob(blistp);
	}

	return TRUE;
}

/*
========================================
	リスト作成本体
========================================
*/

static int makelist2(int iflag)
{
	char line[NSTRING];

	blistp->b_flag &= ~BFCHG;
	*blistp->b_fname = 0;

	{
		int status;

		status = bclear(blistp);
		if (status != TRUE)
			return status;
	}

	if (addline(blistp, KTEX30) == FALSE
	    || addline(blistp, KTEX35) == FALSE)
		return FALSE;

	{
		int i;
		char *cp1;

		cp1 = line;
		*cp1++ = ' ';
		*cp1++ = ' ';
		*cp1++ = ' ';
		*cp1++ = ' ';
		for (i = 0; i < NUMMODES; i++)
			*cp1++ = (gmode & (1 << i)) ? modecode[i] : '.';
		strcpy(cp1, KTEX31);
	}

	if (addline(blistp, line) == FALSE)
		return FALSE;

	return makelist1(iflag, FALSE);
}

/*
========================================
	整数を文字列に変換
========================================
*/

static void long_asc(char *buf, int width, int num)
{
	char *p;

	for (*(p = buf + width) = 0; num >= 10; num /= 10)
		*--p = (num % 10) + '0';
	*--p = num + '0';
	while (p > buf)
		*--p = ' ';
}

/*
========================================
	文字列追加
========================================
*/

int addline(BUFFER *bp, char *text)
{
	int ntext;
	LINE *lp;

	ntext = strlen(text);
	lp = lalloc(ntext);
	if (lp == 0)
		return FALSE;

	{
		int i;
		char *p;

		for (p = lp->l_text, i = 0; i < ntext; i++)
			*p++ = *text++;
	}

	bp->b_linep->l_bp->l_fp = lp;
	lp->l_bp = bp->b_linep->l_bp;
	bp->b_linep->l_bp = lp;
	lp->l_fp = bp->b_linep;
	if (bp->b_dotp == bp->b_linep)
		bp->b_dotp = lp;

	return TRUE;
}

/*
========================================
	バッファのアップデート
========================================
*/

int anycb(void)
{
	BUFFER *bp;

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if ((bp->b_flag & BFINVS) == 0
		    && (bp->b_flag & BFCHG)
		    && *bp->b_fname != 0)
			return TRUE;
	}
	return FALSE;
}

/*
========================================
	バッファ検索
========================================
*/

BUFFER *bfind(char *bname, int cflag, int bflag)
{
	BUFFER *bp;

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if (strcmp(bname, bp->b_bname) == 0)
			return bp;
	}
	if (cflag != FALSE) {
		LINE *lp;
		int i;

		bp = (BUFFER *)malloc(sizeof(BUFFER));
		if (bp == 0)
			return 0;
		lp = lalloc(0);
		if (lp == 0) {
			free(bp);
			return 0;
		}
		if (bheadp == 0 || strcmp(bheadp->b_bname, bname) > 0) {
			bp->b_bufp = bheadp;
			bheadp = bp;
		} else {
			BUFFER *sb;

			for (sb = bheadp; sb->b_bufp; sb = sb->b_bufp) {
				if (strcmp(sb->b_bufp->b_bname, bname) > 0)
					break;
			}
			bp->b_bufp = sb->b_bufp;
			sb->b_bufp = bp;
		}

		bp->b_topline = 0;
		bp->b_botline = 0;
		bp->b_active = TRUE;
		bp->b_dotp = lp;
		bp->b_doto = 0;
		bp->b_jumpno = 0;
		bp->b_markno = 0;
		bp->b_keymap = 0;
		bp->b_tabs = tabsize;
		bp->b_stabs = 0;
		for (i = 0; i < NMARKS; i++) {
			bp->b_markp[i] = 0;
			bp->b_marko[i] = 0;
		}
		bp->b_fcol = 0;
		bp->b_flag = bflag;
		bp->b_mode = gmode;
		bp->b_nwnd = 0;
		bp->b_linep = lp;
		bp->b_emp_text = 0;
		*bp->b_fname = 0;
		strcpy(bp->b_bname, bname);
		*bp->b_key = 0;
		strcpy(bp->b_mlform, mlform);
		bp->b_mlcolor = gbcolor;
		bp->b_fep_mode = 0;
		bp->b_localvar = NULL;
		bp->b_comp_keyword = NULL;
		bp->b_comp_keyword_set = NULL;
		lp->l_fp = lp;
		lp->l_bp = lp;
	}
	return bp;
}

/*
========================================
	バッファ消去
========================================
*/

int bclear(BUFFER *bp)
{
	int numlines;

	{
		int status;

		if ((bp->b_flag & BFINVS) == 0
		    && (bp->b_flag & BFCHG)
		    && *bp->b_fname
		    && (status = mlyesno(KTEX32), status != TRUE))
			return status;
	}

	{
		LINE *lp, *nextlp, *b_linep;

		b_linep = bp->b_linep;
		bp->b_flag &= ~BFCHG;
		for (numlines = 0, lp = lforw(b_linep); lp != b_linep; lp = lforw(lp))
			numlines++;
		if (numlines > LARGE_LINES)
			mlwrite(KTEX244);

		for (lp = lforw(b_linep); lp != b_linep; lp = nextlp) {
			nextlp = lforw(lp);
			free(lp);
		}
		b_linep->l_fp = b_linep->l_bp = b_linep;
	}

	{
		int i;

		bp->b_dotp = bp->b_linep;
		bp->b_doto = 0;
		bp->b_jumpno = 0;
		bp->b_markno = 0;
		for (i = 0; i < NMARKS; i++) {
			bp->b_markp[i] = 0;
			bp->b_marko[i] = 0;
		}
		bp->b_fcol = 0;
	}

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp) {
				int i;

				wp->w_linep = bp->b_linep;
				wp->w_dotp = bp->b_dotp;
				wp->w_doto = bp->b_doto;
				wp->w_jumpno = 0;
				wp->w_markno = 0;
				for (i = 0; i < NMARKS; i++) {
					wp->w_markp[i] = 0;
					wp->w_marko[i] = 0;
				}
				wp->w_flag |= WFMODE | WFHARD;
			}
		}
	}

	if (numlines > LARGE_LINES)
		mlerase();

	return TRUE;
}

/*
----------------------------------------
	変更マーク解除
----------------------------------------
*/

int unmark(int f, int n)
{
	curbp->b_flag &= ~BFCHG;
	upmode();
	return TRUE;
}

/*
========================================
	beginning of buffer / all match window
========================================
*/

void winbob(BUFFER *bp)
{
	WINDOW *wp;

	for(wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			wp->w_dotp = lforw(bp->b_linep);
			wp->w_doto = 0;
			wp->w_flag |= WFHARD;
		}
	}
}
