/*
----------------------------------------
	MOUSE.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/mouse.c,v 1.4 1992/01/04 13:11:24 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static int lastypos = HUGE;
static int lastxpos = HUGE;
static int lastmcmd = MNONE;

/*
========================================
	使用関数の定義
========================================
*/

static WINDOW *lastset(int);
static void posadd(WINDOW *, LINE *);
static void posupd(WINDOW *, WINDOW *, LINE *);
static int ismodeline(WINDOW *, int);
static LINE *mouseline(WINDOW *, int);
static int mouseoffset(WINDOW *, LINE *, int);
static WINDOW *mousewindow(int);

/*
----------------------------------------
	前回処理
----------------------------------------
*/

static WINDOW *lastset(int cmd)
{
	WINDOW *lastwp;

	if (lastmcmd != cmd || lastypos != ypos || lastxpos != xpos)
		nclicks = 0;
	nclicks++;
	lastwp = mousewindow(lastypos);
	lastypos = ypos;
	lastxpos = xpos;
	lastmcmd = cmd;

	return lastwp;
}

/*
----------------------------------------
	ポジション調整
----------------------------------------
*/

static void posadd(WINDOW *wp, LINE *lp)
{
	if (wp == curwp && lp == curwp->w_dotp)
		xpos += lbound;
	curwp = wp;
	curbp = wp->w_bufp;
}

/*
----------------------------------------
	ポジションセット
----------------------------------------
*/

static void posupd(WINDOW *wp, WINDOW *lastwp, LINE *lp)
{
	if (wp != lastwp)
		upmode();
	if (lp) {
		wp->w_dotp = lp;
		wp->w_doto = mouseoffset(wp, lp, xpos);
	}
}

/*
----------------------------------------
	カーソル位置設定
----------------------------------------
*/

int movemdown(int f, int n)
{
	WINDOW *wp, *lastwp;
	LINE *lp;

	lastwp = lastset(MMOVE);
	wp = mousewindow(ypos);
	if (wp == 0)
		return FALSE;
	lp = mouseline(wp, ypos);
	posadd(wp, lp);
	posupd(wp, lastwp, lp);

	return TRUE;
}

/*
----------------------------------------
	指定開始
----------------------------------------
*/

int movemup(int f, int n)
{
	int lastmodeline, deltay, deltax;
	WINDOW *lastwp, *wp;

	if (lastypos == ypos && lastxpos == xpos)
		return FALSE;
	if (lastypos + 1 == term.t_nrow && lastxpos + 1 == term.t_ncol) {
		newwidth(TRUE, xpos + 1);
		newsize(TRUE, ypos + 1);
		return TRUE;
	}
	lastwp = mousewindow(lastypos);
	if (lastwp == 0)
		return FALSE;
	lastmodeline = ismodeline(lastwp, lastypos);
	wp = mousewindow(ypos);
	if (wp == 0)
		return FALSE;

	deltay = lastypos - ypos;
	deltax = lastxpos - xpos;
	lastypos = ypos;
	lastxpos = xpos;

	if (lastmodeline) {
		if (deltax != 0 && (diagflag || deltay == 0)) {
			lastwp->w_fcol += deltax;
			if (lastwp->w_fcol < 0)
				lastwp->w_fcol = 0;
			lastwp->w_flag |= WFMODE | WFHARD;
			if (deltay == 0)
				return TRUE;
		}
		if (lastwp->w_wndp == 0)
			return FALSE;
		if (deltay > 0) {
			if (lastwp != wp)
				return FALSE;
			curwp = wp;
			curbp = wp->w_bufp;
			return shrinkwind(TRUE, deltay);
		}
		if (deltay < 0) {
			if (wp != lastwp->w_wndp)
				return FALSE;
			curwp = lastwp;
			curbp = lastwp->w_bufp;
			return enlargewind(TRUE, -deltay);
		}
	}
	if (ismodeline(wp, ypos) != FALSE)
		return FALSE;
	if (lastwp != wp)
		return FALSE;
	if (deltax != 0 && (diagflag || deltay == 0)) {
		wp->w_fcol += deltax;
		if (wp->w_fcol < 0)
			wp->w_fcol = 0;
		wp->w_flag |= WFMODE;
	}
	return mvdnwind(TRUE, deltay);
}

/*
----------------------------------------
	ドラッグ
----------------------------------------
*/

int mregdown(int f, int n)
{
	WINDOW *wp, *lastwp;
	LINE *lp;

	lastwp = lastset(MREG);
	wp = mousewindow(ypos);
	if (wp == 0 || ismodeline(wp, ypos))
		return FALSE;
	lp = mouseline(wp, ypos);
	posadd(wp, lp);
	posupd(wp, lastwp, lp);

	if (nclicks == 1)
		return setmark(FALSE, 0);
	else {
		kdelete();
		thisflag |= CFKILL;
		return killregion(FALSE, 0);
	}
}

/*
----------------------------------------
	ドラッグ
----------------------------------------
*/

int mregup(int f, int n)
{
	int lastmodeline;
	WINDOW *wp, *lastwp;
	LINE *lp;

	lastwp = lastset(MREG);
	lastmodeline = ismodeline(lastwp, lastypos);
	if (lastmodeline)
		return delwind(TRUE, 0);
	wp = mousewindow(ypos);
	if (wp == 0)
		return FALSE;
	lp = mouseline(wp, ypos);
	posadd(wp, lp);

	if (lp && nclicks < 3) {
		wp->w_dotp = lp;
		wp->w_doto = mouseoffset(wp, lp, xpos);
	}
	if (wp != lastwp) {
		upmode();
		return TRUE;
	}
	if (nclicks == 1) {
		kdelete();
		thisflag |= CFKILL;
		return copyregion(FALSE, 0);
	} else if (nclicks == 2)
		return yank(FALSE, 1);
	else {
		nclicks = 0;
		return TRUE;
	}
}

/*
========================================
	ウィンドウ指定
========================================
*/

static WINDOW *mousewindow(int row)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (row < wp->w_ntrows + 1)
			return wp;
		row -= wp->w_ntrows + 1;
	}
	return 0;
}

/*
========================================
	モードライン指定
========================================
*/

static LINE *mouseline(WINDOW *wp, int row)
{
	LINE *lp;

	row -= wp->w_toprow;
	if (row >= wp->w_ntrows)
		return 0;
	for (lp = wp->w_linep; row--; lp = lforw(lp)) {
		if (lp == wp->w_bufp->b_linep)
			return 0;
	}
	return lp;
}

/*
========================================
	オフセット
========================================
*/

static int mouseoffset(WINDOW *wp, LINE *lp, int col)
{
	int offset, curcol, kanjiflag;

	offset = 0;
	curcol = 0;
	col -= leftmargin;
	if (col < 0)
		col = 0;
	col += wp->w_fcol;
	kanjiflag = ANK;

	while (offset != llength(lp)) {
		int c, newcol;

		newcol = curcol;
		c = lgetc(lp, offset);
		switch (kanjiflag) {
		case ANK:
		case KANJI2:
			kanjiflag = iskanji(c) ? KANJI1 : ANK;
			break;
		case KANJI1:
			kanjiflag = KANJI2;
			break;
		}

		if (c == TAB)
			newcol += -(newcol % wp->w_bufp->b_tabs) + (wp->w_bufp->b_tabs - 1);
		else if (c < 32)
			newcol++;

		if (++newcol > col)
			break;

		curcol = newcol;
		offset++;
	}

	if (offset < llength(lp) && kanjiflag == KANJI2)
		offset--;

	return offset;
}

/*
========================================
	モードラインチェック
========================================
*/

static int ismodeline(WINDOW *wp, int row)
{
	return (wp && row == wp->w_toprow + wp->w_ntrows && modeflag) ? TRUE : FALSE;
}
