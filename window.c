/*
----------------------------------------
	WINDOW.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/window.c,v 1.3 1992/01/04 13:11:24 SALT Exp SALT $'\n""	even\n");

/*
----------------------------------------
	カーソル再配置
----------------------------------------
*/

int reposition(int f, int n)
{
	if (f == FALSE)
		n = 0;
	curwp->w_force = n;
	curwp->w_flag |= WFFORCE;
	return TRUE;
}

/*
----------------------------------------
	画面リフレッシ
----------------------------------------
*/

int refresh(int f, int n)
{
	if (f == FALSE)
		sgarbf = TRUE;
	else {
		curwp->w_force = 0;
		curwp->w_flag |= WFFORCE;
	}
	return TRUE;
}

/*
----------------------------------------
	次のウィンドウ
----------------------------------------
*/

int nextwind(int f, int n)
{
	WINDOW *wp;

	if (f == TRUE) {
		int nwindows;

		for (nwindows = 1, wp = wheadp; wp->w_wndp; wp = wp->w_wndp, nwindows++);
		if (n < 0)
			n = nwindows + n + 1;
		if (n > 0 && n <= nwindows)
			for (wp = wheadp; --n; wp = wp->w_wndp);
		else {
			mlwrite(KTEX203);
			return FALSE;
		}
	} else
		wp = curwp->w_wndp ? : wheadp;

	curwp = wp;
	curbp = wp->w_bufp;
	updinsf = TRUE;
	upmode();
	exechook(windhook);

	return TRUE;
}

/*
----------------------------------------
	前のウィンドウ
----------------------------------------
*/

int prevwind(int f, int n)
{
	WINDOW *wp1, *wp2;

	if (f == TRUE)
		return nextwind(f, -n);

	wp1 = wheadp;
	wp2 = curwp;
	if (wp1 == wp2)
		wp2 = 0;

	while (wp1->w_wndp != wp2)
		wp1 = wp1->w_wndp;
	curwp = wp1;
	curbp = wp1->w_bufp;
	updinsf = TRUE;
	upmode();
	exechook(windhook);

	return TRUE;
}

/*
----------------------------------------
	スクロールダウン
----------------------------------------
*/

int mvdnwind(int f, int n)
{
	return mvupwind(f, -n);
}

/*
----------------------------------------
	スクロールアップ
----------------------------------------
*/

int mvupwind(int f, int n)
{
	int i;
	LINE *lp;
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;

	lp = wp->w_linep;
	if (n < 0) {
		while (n++ && lp != bp->b_linep)
			lp = lforw(lp);
	} else {
		while (n-- && lback(lp) != bp->b_linep)
			lp = lback(lp);
	}

	wp->w_linep = lp;
	wp->w_flag |= WFHARD;

	for (i = 0; i < wp->w_ntrows; ++i, lp = lforw(lp)) {
		if (lp == wp->w_dotp)
			return TRUE;
		if (lp == bp->b_linep)
			break;
	}

	lp = wp->w_linep;
	i = wp->w_ntrows / 2;
	while (i-- && lp != bp->b_linep)
		lp = lforw(lp);

	wp->w_dotp = lp;
	wp->w_doto = 0;

	return TRUE;
}

/*
----------------------------------------
	ウィンドウを一つに
----------------------------------------
*/

int onlywind(int f, int n)
{
	WINDOW *cwp = curwp;

	while (wheadp != cwp) {
		WINDOW *wp;

		wp = wheadp;
		wheadp = wp->w_wndp;
		--wp->w_bufp->b_nwnd;
		copyinf_win_to_buf(wp, wp->w_bufp);
		free(wp);
	}
	while (cwp->w_wndp) {
		WINDOW *wp;

		wp = cwp->w_wndp;
		cwp->w_wndp = wp->w_wndp;
		--wp->w_bufp->b_nwnd;
		copyinf_win_to_buf(wp, wp->w_bufp);
		free(wp);
	}

	{
		int i;
		LINE *lp;

		lp = cwp->w_linep;
		i = cwp->w_toprow;
		while (i != 0 && lback(lp) != curbp->b_linep) {
			i--;
			lp = lback(lp);
		}
		cwp->w_toprow = 0;
		cwp->w_ntrows = term.t_nrow - 1;
		cwp->w_linep = lp;
		cwp->w_flag |= WFMODE | WFHARD;
	}

	return TRUE;
}

/*
----------------------------------------
	ウィンドウ削除
----------------------------------------
*/

int delwind(int f, int n)
{
	WINDOW *lwp;

	{
		WINDOW *wp;

		wp = wheadp;
		if (wp->w_wndp == 0) {
			mlwrite(KTEX204);
			return FALSE;
		}
		for (lwp = 0; wp; lwp = wp, wp = wp->w_wndp) {
			if (wp == curwp)
				break;
		}
	}

	{
		WINDOW *wp = wheadp, *cwp = curwp;

		if (cwp->w_toprow == 0) {
			int target;

			target = cwp->w_ntrows + 1;
			for (; wp; wp = wp->w_wndp) {
				if (wp->w_toprow == target)
					break;
			}
			if (wp == 0)
				return FALSE;
			wp->w_toprow = 0;
			wp->w_ntrows += target;
		} else {
			int target;

			target = cwp->w_toprow - 1;
			for (; wp; wp = wp->w_wndp) {
				if (wp->w_toprow + wp->w_ntrows == target)
					break;
			}
			if (wp == 0)
				return FALSE;
			wp->w_ntrows += curwp->w_ntrows + 1;
		}

		--cwp->w_bufp->b_nwnd;
		copyinf_win_to_buf(cwp, cwp->w_bufp);
		(lwp ? lwp->w_wndp : wheadp) = cwp->w_wndp;
		free(cwp);
		curwp = wp;
		wp->w_flag |= WFHARD;
		curbp = wp->w_bufp;
		updinsf = TRUE;
	}

	upmode();
	exechook(windhook);

	return TRUE;
}

/*
----------------------------------------
	ウィンドウ分割
----------------------------------------
*/

int splitwind(int f, int n)
{
	WINDOW *wp, *cwp = curwp;

	if (cwp->w_ntrows < (modeflag ? 3 : 2)) {
		mlwrite(KTEX205, cwp->w_ntrows);
		return FALSE;
	}
	wp = (WINDOW *) malloc(sizeof(WINDOW));
	if (wp == 0) {
		mlwrite(KTEX99);
		return FALSE;
	}

	curbp->b_nwnd++;
	*wp = *cwp;
	wp->w_flag = 0;
	wp->w_force = 0;

	{
		int ntru, ntrl, ntrd;
		LINE *lp;

		ntru = (cwp->w_ntrows - 1) >> 1;
		ntrl = (cwp->w_ntrows - 1) - ntru;
		ntrd = 0;
		for (lp = cwp->w_linep; lp != cwp->w_dotp; ntrd++, lp = lforw(lp));
		lp = cwp->w_linep;
		if ((f == FALSE && ntrd <= ntru) || (f == TRUE && n == 1)) {
			if (ntrd == ntru)
				lp = lforw(lp);
			cwp->w_ntrows = ntru;
			wp->w_wndp = cwp->w_wndp;
			cwp->w_wndp = wp;
			wp->w_toprow = cwp->w_toprow + ntru + 1;
			wp->w_ntrows = ntrl;
		} else {
			WINDOW *wp1, *wp2;

			for (wp1 = 0, wp2 = wheadp; wp2 != cwp; wp1 = wp2, wp2 = wp2->w_wndp);
			(wp1 ? wp1->w_wndp : wheadp) = wp;

			wp->w_wndp = cwp;
			wp->w_toprow = cwp->w_toprow;
			wp->w_ntrows = ntru++;
			cwp->w_toprow += ntru;
			cwp->w_ntrows = ntrl;
			while (ntru--)
				lp = lforw(lp);
		}
		cwp->w_linep = lp;
		wp->w_linep = lp;
	}

	cwp->w_flag |= WFMODE | WFHARD;
	wp->w_flag |= WFMODE | WFHARD;
	exechook(windhook);

	return TRUE;
}

/*
----------------------------------------
	ウィンドウ拡大
----------------------------------------
*/

int enlargewind(int f, int n)
{
	if (n < 0)
		return shrinkwind(f, -n);

	{
		WINDOW *adjwp, *wp = curwp;

		if (wheadp->w_wndp == 0) {
			mlwrite(KTEX206);
			return FALSE;
		}
		adjwp = wp->w_wndp;
		if (adjwp == 0)
			for (adjwp = wheadp; adjwp->w_wndp != wp; adjwp = adjwp->w_wndp);

		if (adjwp->w_ntrows + (modeflag ? 0 : 1) <= n) {
			mlwrite(KTEX207);
			return FALSE;
		}
		if (wp->w_wndp == adjwp) {
			int i;
			LINE *lp;

			lp = adjwp->w_linep;
			for (i = 0; i < n && lp != adjwp->w_bufp->b_linep; i++, lp = lforw(lp));
			adjwp->w_linep = lp;
			adjwp->w_toprow += n;
		} else {
			int i;
			LINE *lp;

			lp = wp->w_linep;
			for (i = 0; i < n && lback(lp) != curbp->b_linep; i++, lp = lback(lp));
			wp->w_linep = lp;
			wp->w_toprow -= n;
		}

		wp->w_ntrows += n;
		adjwp->w_ntrows -= n;
		wp->w_flag |= WFMODE | WFHARD;
		adjwp->w_flag |= WFMODE | WFHARD;
	}

	return TRUE;
}

/*
----------------------------------------
	ウィンドウ縮小
----------------------------------------
*/

int shrinkwind(int f, int n)
{
	if (n < 0)
		return enlargewind(f, -n);

	if (wheadp->w_wndp == 0) {
		mlwrite(KTEX206);
		return (FALSE);
	}

	{
		WINDOW *adjwp, *wp = curwp;

		adjwp = wp->w_wndp;
		if (adjwp == 0)
			for (adjwp = wheadp; adjwp->w_wndp != wp; adjwp = adjwp->w_wndp);

		if (wp->w_ntrows + (modeflag ? 0 : 1) <= n) {
			mlwrite(KTEX207);
			return FALSE;
		}
		if (wp->w_wndp == adjwp) {
			int i;
			LINE *lp;

			lp = adjwp->w_linep;
			for (i = 0; i < n && lback(lp) != adjwp->w_bufp->b_linep; i++, lp = lback(lp));
			adjwp->w_linep = lp;
			adjwp->w_toprow -= n;
		} else {
			int i;
			LINE *lp;

			lp = wp->w_linep;
			for (i = 0; i < n && lp != curbp->b_linep; i++, lp = lforw(lp));
			wp->w_linep = lp;
			wp->w_toprow += n;
		}
		wp->w_ntrows -= n;
		adjwp->w_ntrows += n;
		wp->w_flag |= WFMODE | WFHARD;
		adjwp->w_flag |= WFMODE | WFHARD;
	}

	return TRUE;
}

/*
----------------------------------------
	サイズ変更
----------------------------------------
*/

int resize(int f, int n)
{
	int clines;

	if (f == FALSE)
		return TRUE;

	clines = curwp->w_ntrows;
	if (clines == n)
		return TRUE;

	return enlargewind(TRUE, n - clines);
}

/*
----------------------------------------
	ウィンドウ pop-up
----------------------------------------
*/

WINDOW *wpopup(void)
{
	WINDOW *wp;

	if (wheadp->w_wndp == 0 && splitwind(FALSE, 0) == FALSE)
		return 0;
	for (wp = wheadp; wp && wp == curwp; wp = wp->w_wndp);
	return wp;
}

/*
----------------------------------------
	他のウィンドウアップ
----------------------------------------
*/

int nextup(int f, int n)
{
	nextwind(FALSE, 1);
	backpage(f, n);
	prevwind(FALSE, 1);

	return TRUE;
}

/*
----------------------------------------
	他のウィンドウダウン
----------------------------------------
*/

int nextdown(int f, int n)
{
	nextwind(FALSE, 1);
	forwpage(f, n);
	prevwind(FALSE, 1);

	return TRUE;
}

/*
----------------------------------------
	ウィンドウ記録
----------------------------------------
*/

int savewnd(int f, int n)
{
	swindow = curwp;
	return TRUE;
}

/*
----------------------------------------
	ウィンドウ復帰
----------------------------------------
*/

int restwnd(int f, int n)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp == swindow) {
			curwp = wp;
			curbp = wp->w_bufp;
			updinsf = TRUE;
			upmode();
			return TRUE;
		}
	}
	mlwrite(KTEX208);

	return FALSE;
}

/*
----------------------------------------
	リサイズ
----------------------------------------
*/

int newsize(int f, int n)
{
	int mode, max, fkey;
	int mline;

	mode = CRTMOD(-1);
	if (density >= 0) {
		fkey = (tinf[mode].funckey && fkmode) ? (1 << density) : 0;
		max = tinf[mode].vsize << density;
	} else {
		fkey = (tinf2[mode].funckey && fkmode) ? 1 : 0;
		max = tinf2[mode].vsize;
	}
	mline = modeflag ? 2 : 1;

	if (f == FALSE)
		n = max - fkey;

	if (n < 6 || n > max - fkey) {
		mlwrite(KTEX209);
		return FALSE;
	}

	if (term.t_nrow == n - 1)
		return TRUE;
	else if (term.t_nrow < n) {
		WINDOW *wp;

		for (wp = wheadp; wp->w_wndp; wp = wp->w_wndp);
		wp->w_ntrows = n - wp->w_toprow - mline;
		wp->w_flag |= WFHARD | WFMODE;
	} else {
		WINDOW *nextwp, *lastwp;

		nextwp = wheadp;
		lastwp = 0;

		while (nextwp) {
			WINDOW *wp;

			wp = nextwp;
			nextwp = wp->w_wndp;
			if (wp->w_toprow > n - mline) {
				--wp->w_bufp->b_nwnd;
				copyinf_win_to_buf(wp, wp->w_bufp);
				if (wp == curwp) {
					curwp = wheadp;
					curbp = curwp->w_bufp;
					updinsf = TRUE;
				}
				if (lastwp)
					lastwp->w_wndp = 0;
				free(wp);
				wp = 0;
			} else {
				if (wp->w_toprow + wp->w_ntrows - 1 >= n - mline) {
					wp->w_ntrows = n - wp->w_toprow - mline;
					wp->w_flag |= WFHARD | WFMODE;
				}
			}
			lastwp = wp;
		}
	}

	term.t_nrow = n - 1;
	sgarbf = TRUE;

	return TRUE;
}

/*
----------------------------------------
	リサイズ
----------------------------------------
*/

int newwidth(int f, int n)
{
	int max;

	max = tinf2[CRTMOD(-1)].hsize;

	if (f == FALSE)
		n = max;

	if (n < 10 || n > max) {
		mlwrite(KTEX210);
		return FALSE;
	}

	if (term.t_ncol == n - 1)
		return TRUE;

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp)
			wp->w_flag |= WFHARD | WFMOVE | WFMODE;
	}

	term.t_ncol = n - 1;
	term.t_margin = n / 10;
	term.t_scrsiz = n - term.t_margin * 2;
	sgarbf = TRUE;

	return TRUE;
}

/*
----------------------------------------
	ウィンドウの先頭行を得る
----------------------------------------
*/

int getwpos(void)
{
	int sline;
	LINE *lp;

	for (lp = curwp->w_linep, sline = 1; lp != curwp->w_dotp; sline++, lp = lforw(lp));
	return sline;
}

/*
----------------------------------------
	copyinf_win_to_buf
----------------------------------------
*/

void copyinf_win_to_buf(WINDOW *wp, BUFFER *bp)
{
	int i;

	bp->b_dotp = wp->w_dotp;
	bp->b_doto = wp->w_doto;
	bp->b_jumpno = wp->w_jumpno;
	bp->b_markno = wp->w_markno;
	{
		LINE **b_markp, **w_markp;
		short *b_marko, *w_marko;

		b_markp = bp->b_markp;
		w_markp = wp->w_markp;
		b_marko = bp->b_marko;
		w_marko = wp->w_marko;

		for (i = 0; i < NMARKS; i++) {
			b_markp[i] = w_markp[i];
			b_marko[i] = w_marko[i];
		}
	}
	bp->b_fcol = wp->w_fcol;
}

/*
----------------------------------------
	copyinf_buf_to_win
----------------------------------------
*/

void copyinf_buf_to_win(BUFFER *bp, WINDOW *wp)
{
	int i;

	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	wp->w_jumpno = bp->b_jumpno;
	wp->w_markno = bp->b_markno;
	{
		LINE **b_markp, **w_markp;
		short *b_marko, *w_marko;

		b_markp = bp->b_markp;
		w_markp = wp->w_markp;
		b_marko = bp->b_marko;
		w_marko = wp->w_marko;

		for (i = 0; i < NMARKS; i++) {
			w_markp[i] = b_markp[i];
			w_marko[i] = b_marko[i];
		}
	}
	wp->w_fcol = bp->b_fcol;
}
