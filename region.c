/*
----------------------------------------
	REGION.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/region.c,v 1.3 1992/01/05 02:09:28 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static int casereg(int, int, int);

/*
========================================
	region の行数を得る
========================================
*/

int reglines(int df, int dn)
{
	int n;
	LINE *lp;
	REGION region;

	if (getregion(&region, df, dn) != TRUE)
		return 0;

	lp = region.r_linep;
	region.r_size += region.r_offset;
	for (n = 0; region.r_size > 0; n++, lp = lforw(lp))
		region.r_size -= llength(lp) + 1;
	curwp->w_dotp = region.r_linep;
	curwp->w_doto = region.r_offset;

	return n;
}

/*
----------------------------------------
	region 削除
----------------------------------------
*/

int killregion(int f, int n)
{
	int status;
	REGION region;

	status = getregion(&region, f, n);
	if (status != TRUE)
		return status;
	curwp->w_dotp = region.r_linep;
	curwp->w_doto = region.r_offset;
	return ldelete(region.r_size, TRUE);
}

/*
----------------------------------------
	region コピー
----------------------------------------
*/

int copyregion(int f, int n)
{
	int loffs, status;
	LINE *lp;
	REGION region;

	status = getregion(&region, f, n);
	if (status != TRUE)
		return status;

	lp = region.r_linep;
	loffs = region.r_offset;
	while (region.r_size--) {
		if (loffs == llength(lp)) {
			status = kinsert(I_NEWLINE);
			if (status != TRUE)
				return status;
			lp = lforw(lp);
			loffs = 0;
		} else {
			status = kinsert(lgetc(lp, loffs));
			if (status != TRUE)
				return status;
			loffs++;
		}
	}
	mlwrite(KTEX70);

	return TRUE;
}

/*
----------------------------------------
	region 小文字化
----------------------------------------
*/

int lowerregion(int f, int n)
{
	return casereg(0, f, n);
}

/*
----------------------------------------
	region 大文字化
----------------------------------------
*/

int upperregion(int f, int n)
{
	return casereg(1, f, n);
}

/*
========================================
	case-region 本体
========================================
*/

static int casereg(int flag, int f, int n)
{
	int loffs, status;
	LINE *lp;
	REGION region;

	status = getregion(&region, f, n);
	if (status != TRUE)
		return status;
	lchange(WFHARD);

	lp = region.r_linep;
	loffs = region.r_offset;
	while (region.r_size--) {
		if (loffs == llength(lp)) {
			lp = lforw(lp);
			loffs = 0;
		} else {
			int c;

			c = lgetc(lp, loffs);
			if (iskanji(c)) {
				loffs += 2;
				region.r_size--;
			} else {
				lputc(lp, loffs, flag ? toupper(c) : tolower(c));
				loffs++;
			}
		}
	}
	return TRUE;
}

/*
----------------------------------------
	縮小
----------------------------------------
*/

int narrow(int f, int n)
{
	int status;
	BUFFER *bp;
	REGION creg;
	WINDOW *cwp = curwp;

	bp = cwp->w_bufp;
	if (bp->b_flag & BFNAROW) {
		mlwrite(KTEX71);
		return FALSE;
	}
	status = getregion(&creg, f, n);
	if (status != TRUE)
		return status;

	cwp->w_dotp = creg.r_linep;
	cwp->w_doto = 0;
	creg.r_size += creg.r_offset;

	if (creg.r_size <= cwp->w_dotp->l_used) {
		mlwrite(KTEX72);
		return FALSE;
	}
	if (lforw(bp->b_linep) != creg.r_linep) {
		bp->b_topline = bp->b_linep->l_fp;
		creg.r_linep->l_bp->l_fp = 0;
		bp->b_linep->l_fp = creg.r_linep;
		creg.r_linep->l_bp = bp->b_linep;
	}
	ankforwchar(TRUE, creg.r_size);
	cwp->w_doto = 0;

	if (bp->b_linep != cwp->w_dotp) {
		bp->b_botline = cwp->w_dotp;
		bp->b_botline->l_bp->l_fp = bp->b_linep;
		bp->b_linep->l_bp->l_fp = 0;
		bp->b_linep->l_bp = bp->b_botline->l_bp;
	} {
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp) {
				int i;

				wp->w_linep = creg.r_linep;
				wp->w_dotp = creg.r_linep;
				wp->w_doto = 0;
				for (i = 0; i < NMARKS; i++) {
					wp->w_markp[i] = creg.r_linep;
					wp->w_marko[i] = 0;
				}
				wp->w_flag |= (WFHARD | WFMODE);
			}
		}
	}

	bp->b_flag |= BFNAROW;
	mlwrite(KTEX73);

	return TRUE;
}

/*
----------------------------------------
	復帰
----------------------------------------
*/

int widen(int f, int n)
{
	WINDOW	*wp;
	BUFFER	*bp;
	int		bottom;

	wp = curwp;
	bp = wp->w_bufp;
	if ((bp->b_flag & BFNAROW) == 0) {
		mlwrite(KTEX74);
		return FALSE;
	}
	if (wp->w_dotp == bp->b_linep) {
		backline(FALSE, 1);
		bottom = 1;
	} else
		bottom = 0;

	if (bp->b_topline) {
		LINE *lp = bp->b_topline;

		while (lp->l_fp)
			lp = lforw(lp);
		lp->l_fp = bp->b_linep->l_fp;
		lp->l_fp->l_bp = lp;
		bp->b_linep->l_fp = bp->b_topline;
		bp->b_topline->l_bp = bp->b_linep;
		bp->b_topline = 0;
	}
	if (bp->b_botline) {
		LINE *lp = bp->b_botline;

		while (lp->l_fp)
			lp = lforw(lp);
		lp->l_fp = bp->b_linep;
		bp->b_linep->l_bp->l_fp = bp->b_botline;
		bp->b_botline->l_bp = bp->b_linep->l_bp;
		bp->b_linep->l_bp = lp;
		bp->b_botline = 0;
	}
	if (bottom) {
		forwline(FALSE, 1);
		gotobol(FALSE, 1);
	}
	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp)
				wp->w_flag |= (WFHARD | WFMODE);
		}
	}

	bp->b_flag &= ~BFNAROW;
	mlwrite(KTEX75);

	return TRUE;
}

/*
========================================
	region 処理本体
========================================
*/

int getregion(REGION *rp, int f, int mno)
{
	WINDOW *wp = curwp;

	mno = (f == FALSE) ? 0 : (mno % NMARKS);

	if (wp->w_markp[mno] == 0) {
		mlwrite(KTEX76);
		return FALSE;
	}
	if (wp->w_dotp == wp->w_markp[mno]) {
		rp->r_linep = wp->w_dotp;
		if (wp->w_doto < wp->w_marko[mno]) {
			rp->r_offset = wp->w_doto;
			rp->r_size = wp->w_marko[mno] - wp->w_doto;
		} else {
			rp->r_offset = wp->w_marko[mno];
			rp->r_size = wp->w_doto - wp->w_marko[mno];
		}
		return TRUE;
	} {
		int fsize, bsize;
		LINE *blp, *flp;
		BUFFER *bp = curbp;

		blp = flp = wp->w_dotp;
		bsize = wp->w_doto;
		fsize = llength(flp) - bsize + 1;

		while (flp != bp->b_linep || lback(blp) != bp->b_linep) {
			if (flp != bp->b_linep) {
				flp = lforw(flp);
				if (flp == wp->w_markp[mno]) {
					rp->r_linep = wp->w_dotp;
					rp->r_offset = wp->w_doto;
					rp->r_size = fsize + wp->w_marko[mno];
					return TRUE;
				}
				fsize += llength(flp) + 1;
			}
			if (lback(blp) != bp->b_linep) {
				blp = lback(blp);
				bsize += llength(blp) + 1;
				if (blp == wp->w_markp[mno]) {
					rp->r_linep = blp;
					rp->r_offset = wp->w_marko[mno];
					rp->r_size = bsize - wp->w_marko[mno];
					return TRUE;
				}
			}
		}
	}

	mlwrite(KTEX77);

	return FALSE;
}

/*
========================================
	region を得る
========================================
*/

char *getreg(void)
{
	REGION region;
	static char value[NSTRING];

	if (curwp->w_markp[0] == NULL || getregion(&region, FALSE, 0) != TRUE)
		return (char *) errorm;

	{
		int loffs;
		char *sp;
		LINE *linep;

		linep = region.r_linep;
		loffs = region.r_offset;
		if (region.r_size >= NSTRING)
			region.r_size = NSTRING - 1;
		for (sp = value; region.r_size--; sp++) {
			if (loffs == llength(linep)) {
				*sp = I_NEWLINE;
				linep = lforw(linep);
				loffs = 0;
			} else {
				*sp = lgetc(linep, loffs);
				loffs++;
			}
		}
		*sp = 0;
	}

	return value;
}

/*
----------------------------------------
	append-next-kill
----------------------------------------
*/

int apendnext(int f, int n)
{
	thisflag |= CFKILL;
	mlwrite(KTEX247);
	return TRUE;
}
