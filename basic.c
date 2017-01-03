/*
----------------------------------------
	BASIC.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/basic.c,v 1.4 1992/01/04 13:11:20 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static int gotomultimark(int);

/*
----------------------------------------
	行の先頭、終わりに移動
----------------------------------------
*/

inline int gotobeol(int f, int n)
{
	if (curwp->w_doto)
		curwp->w_doto = 0;
	else
		curwp->w_doto = llength(curwp->w_dotp);
	return TRUE;
}

/*
----------------------------------------
	行の先頭に移動
----------------------------------------
*/

inline int gotobol(int f, int n)
{
	curwp->w_doto = 0;
	return TRUE;
}

/*
----------------------------------------
	行の終わりに移動
----------------------------------------
*/

inline int gotoeol(int f, int n)
{
	curwp->w_doto = llength(curwp->w_dotp);
	return TRUE;
}

/*
----------------------------------------
	バッファの先頭に移動
----------------------------------------
*/

inline int gotobob(int f, int n)
{
	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
----------------------------------------
	バッファの終端に移動
----------------------------------------
*/

inline int gotoeob(int f, int n)
{
	curwp->w_dotp = curbp->b_linep;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/*
----------------------------------------
	Ｎ文字戻る
----------------------------------------
*/

int backchar(int f, int n)
{
	WINDOW *wp;

	if (n < 0)
		return forwchar(f, -n);
	wp = curwp;
	while (n--) {
		if (wp->w_doto == 0) {
			LINE *lp;

			lp = lback(wp->w_dotp);
			if (lp == curbp->b_linep)
				return FALSE;
			wp->w_dotp = lp;
			wp->w_doto = llength(lp);
			wp->w_flag |= WFMOVE;
		} else {
			wp->w_doto--;
			if (nthctype(wp->w_dotp->l_text, wp->w_doto + 1) == CT_KJ2)
				wp->w_doto--;
		}
	}
	return TRUE;
}

/*
----------------------------------------
	Ｎ文字進む
----------------------------------------
*/

int forwchar(int f, int n)
{
	WINDOW *wp;

	if (n < 0)
		return (backchar(f, -n));
	wp = curwp;
	while (n--) {
		if (wp->w_doto == llength(wp->w_dotp)) {
			if (wp->w_dotp == curbp->b_linep)
				return FALSE;
			wp->w_dotp = lforw(wp->w_dotp);
			wp->w_doto = 0;
			wp->w_flag |= WFMOVE;
		} else {
			wp->w_doto++;
			if (nthctype(wp->w_dotp->l_text, wp->w_doto + 1) == CT_KJ2
			    && wp->w_doto < llength(wp->w_dotp))
				wp->w_doto++;
		}
	}
	return TRUE;
}

/*
----------------------------------------
	指定行へジャンプ
----------------------------------------
*/

int gotoline(int f, int n)
{
	if (f == FALSE) {
		int status;
		char arg[NSTRING];

		his_disable();
		status = mlreply(KTEX7, arg, NSTRING);
		if (status != TRUE) {
			mlwrite(KTEX8);
			return status;
		}
		n = asc_int(arg);
	}
	if (n < 1)
		return FALSE;

	{
		LINE *lp, *bottom;

		bottom = curbp->b_linep;
		for (lp = lforw(bottom); --n && (lp != bottom); lp = lforw(lp));
		if (n) {
			mlwrite(KTEX102);
			return FALSE;
		}
		curwp->w_dotp = lp;
		curwp->w_doto = 0;
		curwp->w_flag |= WFMOVE;
	}

	return TRUE;
}

/*
----------------------------------------
	Ｎ行進む
----------------------------------------
*/

int forwline(int f, int n)
{
	LINE *lp, *bottom;
	WINDOW *wp;

	if (n < 0)
		return backline(f, -n);
	thisflag |= CFCPCN;
	wp = curwp;
	bottom = curbp->b_linep;
	if (wp->w_dotp == bottom)
		return FALSE;
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);
	lp = wp->w_dotp;
	while (n-- && lp != bottom)
		lp = lforw(lp);
	wp->w_dotp = lp;
	wp->w_doto = getgoal(lp, curbp->b_tabs);
	wp->w_flag |= WFMOVE;
	return TRUE;
}

/*
----------------------------------------
	Ｎ行戻る
----------------------------------------
*/

int backline(int f, int n)
{
	LINE *lp, *bottom;
	WINDOW *wp;

	if (n < 0)
		return forwline(f, -n);
	thisflag |= CFCPCN;
	wp = curwp;
	bottom = curbp->b_linep;
	if (lback(wp->w_dotp) == bottom)
		return FALSE;
	if ((lastflag & CFCPCN) == 0)
		curgoal = getccol(FALSE);
	lp = wp->w_dotp;
	while (n-- && lback(lp) != bottom)
		lp = lback(lp);
	wp->w_dotp = lp;
	wp->w_doto = getgoal(lp, curbp->b_tabs);
	wp->w_flag |= WFMOVE;
	return TRUE;
}

/*
----------------------------------------
	段落の先頭に移動
----------------------------------------
*/

int gotobop(int f, int n)
{
	WINDOW *wp;
	LINE *bottom, *lp;

	if (n < 0)
		return gotoeop(f, -n);
	wp = curwp;
	lp = wp->w_dotp;
	bottom = curbp->b_linep;
	while (n--) {
		int suc;

		suc = backchar(FALSE, 1);
		if (suc != TRUE)
			return suc;
		while (!inword() && suc)
			suc = backchar(FALSE, 1);
		wp->w_doto = 0;
		while (lback(wp->w_dotp) != bottom) {
			int c;

			c = lgetc(wp->w_dotp, wp->w_doto);
			if (iskanji(c))
				c = (c << 8) | lgetc(wp->w_dotp, wp->w_doto + 1);
			if (llength(wp->w_dotp)
			  && (wrapexpand || (c != TAB && c != ' ' && c != 0x8140)))
				wp->w_dotp = lback(wp->w_dotp);
			else
				break;
		}
		suc = forwchar(FALSE, 1);
		while (suc && wp->w_dotp != lp && !inword())
			suc = forwchar(FALSE, 1);
		wp->w_doto = 0;
	}
	wp->w_flag |= WFMOVE;
	return TRUE;
}

/*
----------------------------------------
	段落の終わりに移動
----------------------------------------
*/

int gotoeop(int f, int n)
{
	WINDOW *wp;
	LINE *bottom, *lp;

	if (n < 0)
		return gotobop(f, -n);
	wp = curwp;
	lp = wp->w_dotp;
	bottom = curbp->b_linep;
	while (n--) {
		int suc;

		suc = TRUE;
		while (!inword() && suc)
			suc = forwchar(FALSE, 1);
		wp->w_doto = 0;
		if (suc)
			wp->w_dotp = lforw(wp->w_dotp);
		while (wp->w_dotp != bottom) {
			int c;

			c = lgetc(wp->w_dotp, wp->w_doto);
			if (iskanji(c))
				c = (c << 8) | lgetc(wp->w_dotp, wp->w_doto + 1);
			if (llength(wp->w_dotp)
			  && (wrapexpand || (c != TAB && c != ' ' && c != 0x8140)))
				wp->w_dotp = lforw(wp->w_dotp);
			else
				break;
		}
		suc = backchar(FALSE, 1);
		while (suc && wp->w_dotp != lp && !inword())
			suc = backchar(FALSE, 1);
		wp->w_doto = llength(wp->w_dotp);
	}
	wp->w_flag |= WFMOVE;
	return TRUE;
}

/*
========================================
	カーソル目標桁を得る
========================================
*/

int getgoal(LINE *dlp, int tab)
{
	int col, dbo;

	col = dbo = 0;
	while (dbo != llength(dlp)) {
		int c, newcol;

		newcol = col;
		c = lgetc(dlp, dbo);
		if (c == TAB)
			newcol += -(newcol % tab) + (tab - 1);
		else if (c < 0x20 || c == 0x7f)
			newcol++;
		newcol++;
		if (newcol > curgoal)
			break;
		col = newcol;
		dbo++;
	}
	if (nthctype(dlp->l_text, dbo + 1) == CT_KJ2)
		dbo--;
	return dbo;
}

/*
----------------------------------------
	Ｎページ進む
----------------------------------------
*/

int forwpage(int f, int n)
{
	LINE *baselp, *lp, *bottom;
	WINDOW *wp;

	wp = curwp;
	if (f == FALSE) {
		n = wp->w_ntrows - 2;
		if (n <= 0)
			n = 1;
	} else if (n < 0)
		return backpage(f, -n);
	else
		n *= wp->w_ntrows;

	bottom = curbp->b_linep;

	if (edpage) {
		if ((lastflag & CFCPCN) == 0)
			curgoal = getccol(FALSE);
		baselp = wp->w_linep;
		lp = wp->w_dotp;
		while (n-- && lp != bottom) {
			lp = lforw(lp);
			if (lp != bottom)
				baselp = lforw(baselp);
		}
		if (lp != bottom)
			wp->w_linep = baselp;
		wp->w_doto = getgoal(lp, curbp->b_tabs);
		thisflag |= CFCPCN;
	} else {
		lp = wp->w_linep;
		while (n-- && lp != bottom)
			lp = lforw(lp);
		if (lp != bottom)
			wp->w_linep = lp;
		wp->w_doto = 0;
	}

	wp->w_dotp = lp;
	wp->w_flag |= WFHARD;
	keydrops();
	return TRUE;
}

/*
----------------------------------------
	Ｎページ戻る
----------------------------------------
*/

int backpage(int f, int n)
{
	LINE *baselp, *lp, *bottom;
	WINDOW *wp;

	wp = curwp;
	if (f == FALSE) {
		n = wp->w_ntrows - 2;
		if (n <= 0)
			n = 1;
	} else if (n < 0)
		return forwpage(f, -n);
	else
		n *= wp->w_ntrows;

	bottom = curbp->b_linep;

	if (edpage) {
		if ((lastflag & CFCPCN) == 0)
			curgoal = getccol(FALSE);
		baselp = wp->w_linep;
		lp = wp->w_dotp;
		while (n-- && lback(lp) != bottom) {
			if (lback(lp) != bottom)
				baselp = lback(baselp);
			lp = lback(lp);
		}
		wp->w_linep = baselp;
		wp->w_doto = getgoal(lp, curbp->b_tabs);
		thisflag |= CFCPCN;
	} else {
		lp = wp->w_linep;
		while (n-- && lback(lp) != bottom)
			lp = lback(lp);
		wp->w_linep = lp;
		wp->w_doto = 0;
	}

	wp->w_dotp = lp;
	wp->w_flag |= WFHARD;
	keydrops();
	return TRUE;
}

/*
----------------------------------------
	Ｎ番のマークを設定
----------------------------------------
*/

int setmark(int f, int n)
{
	WINDOW *wp;
	BUFFER *bp;
	LINE *dotp;
	int doto;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	n = (f == TRUE) ? (n % NMARKS) : 0;
	bp = curbp;
	bp->b_markp[n] = dotp;
	bp->b_marko[n] = doto;
	for(wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			wp->w_markp[n] = dotp;
			wp->w_marko[n] = doto;
		}
	}
	mlwrite(KTEX9, n);
	return TRUE;
}

/*
----------------------------------------
	Ｎ番のマーク除去
----------------------------------------
*/

int remmark(int f, int n)
{
	WINDOW *wp;
	BUFFER *bp;

	n = (f == TRUE) ? (n % NMARKS) : 0;
	bp = curbp;
	bp->b_markp[n] = 0;
	bp->b_marko[n] = 0;
	for(wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			wp->w_markp[n] = 0;
			wp->w_marko[n] = 0;
		}
	}
	mlwrite(KTEX10, n);
	return TRUE;
}

/*
----------------------------------------
	マーク位置の交換
----------------------------------------
*/

int swapmark(int f, int n)
{
	WINDOW *wp;
	BUFFER *bp;
	LINE *odotp;
	int odoto;

	n = (f == TRUE) ? (n % NMARKS) : 0;
	wp = curwp;
	if (wp->w_markp[n] == 0) {
		mlwrite(KTEX11, n);
		return FALSE;
	}
	bp = curbp;
	odotp = wp->w_dotp;
	odoto = wp->w_doto;
	wp->w_dotp = wp->w_markp[n];
	wp->w_doto = wp->w_marko[n];
	wp->w_flag |= WFMOVE;
	bp->b_markp[n] = odotp;
	bp->b_marko[n] = odoto;
	for(wp = wheadp; wp; wp = wp->w_wndp) {
		if (wp->w_bufp == bp) {
			wp->w_markp[n] = odotp;
			wp->w_marko[n] = odoto;
		}
	}
	return TRUE;
}

/*
----------------------------------------
	Ｎ番のマークへジャンプ
----------------------------------------
*/

int gotomark(int f, int n)
{
	WINDOW *wp;

	n = (f == TRUE) ? (n % NMARKS) : 0;
	wp = curwp;
	if (wp->w_markp[n] == 0) {
		mlwrite(KTEX11, n);
		return FALSE;
	}
	wp->w_dotp = wp->w_markp[n];
	wp->w_doto = wp->w_marko[n];
	wp->w_flag |= WFMOVE;
	return TRUE;
}

/*
----------------------------------------
	行削除
----------------------------------------
*/

int killline(int f, int n)
{
	int chunk = 0;
	LINE *nextp;
	WINDOW *wp = curwp;

	if (n > 0) {
		chunk = llength(wp->w_dotp) + 1;
		wp->w_doto = 0;
		nextp = lforw(wp->w_dotp);
		while (--n) {
			if (nextp == curbp->b_linep)
				return FALSE;
			chunk += llength(nextp) + 1;
			nextp = lforw(nextp);
		}
	} else if (n < 0) {
		wp->w_doto = 0;
		while (n++) {
			if (backline(FALSE, 1) == FALSE)
				return FALSE;
			chunk += llength(wp->w_dotp) + 1;
		}
	}
	return ldelete(chunk, TRUE);
}

/*
----------------------------------------
	行コピー
----------------------------------------
*/

int copyline(int f, int n)
{
	int doto;
	LINE *dotp;
	WINDOW *wp = curwp;

	doto = wp->w_doto;
	dotp = wp->w_dotp;
	if (n > 0) {
		wp->w_doto = 0;
		while (n--) {
			int i;

			for (i = 0; i < llength(wp->w_dotp); i++) {
				if (kinsert(lgetc(wp->w_dotp, i)) == FALSE)
					return FALSE;
			}
			if (kinsert(I_NEWLINE) == FALSE)
				return FALSE;
			if (forwline(FALSE, 1) == FALSE)
				return FALSE;
		}
	} else if (n < 0) {
		wp->w_doto = 0;
		if (backline(FALSE, -n) == FALSE)
			return FALSE;
		while (n++) {
			int i;

			for (i = 0; i < llength(wp->w_dotp); i++) {
				if (kinsert(lgetc(wp->w_dotp, i)) == FALSE)
					return FALSE;
			}
			if (kinsert(I_NEWLINE) == FALSE)
				return FALSE;
			if (forwline(FALSE, 1) == FALSE)
				return FALSE;
		}
	}
	wp->w_doto = doto;
	wp->w_dotp = dotp;
	mlwrite(KTEX70);
	return TRUE;
}

/*
----------------------------------------
	キャラクタまで削除
----------------------------------------
*/

int zaptochar(int f, int n)
{
	int c, chunk;
	char *sp;
	char buf[NSTRING];

	if (clexec == FALSE) {
		mlwrite(KTEX229);
		c = tgetc();
		if (c == ectoc(abortc)) {
			ctrlg(FALSE, 1);
			return ABORT;
		}
		if (iskanji(c))
			c = ((c << 8) + (tgetc() & CHARMASK)) & WORDMASK;
		ochar(c);
		oupdate();
	} else {
		execstr = token(execstr, buf, NSTRING);
		sp = getval(buf);
		if (sp == 0)
			return FALSE;
		strcpy(buf, sp);
		if (iskanji(c = *buf))
			c = ((c << 8) + (buf[1] & CHARMASK)) & WORDMASK;
	}

	{
		WINDOW *wp;

		chunk = 0;
		wp = curwp;
		if (n > 0) {
			int doto;
			LINE *dotp;

			doto = wp->w_doto;
			dotp = wp->w_dotp;
			while (n--) {
				int fsf;

				fsf = 1;
				while (1) {
					int s;

					s = lgetc2(wp->w_dotp, wp->w_doto);
					if (iskanji(s))
						s = (s << 8) + lgetc(wp->w_dotp, wp->w_doto + 1);
					if (!fsf && s == c)
						break;
					chunk++;
					if (forwchar(FALSE, 1) == FALSE)
						break;
					fsf = 0;
				}
			}
			wp->w_doto = doto;
			wp->w_dotp = dotp;
		}
	}

	return forwdel(TRUE, chunk);
}

/*
----------------------------------------
	Ｎ文字進む(narrow 用)
----------------------------------------
*/

int ankforwchar(int f, int n)
{
	WINDOW *wp;

	wp = curwp;
	while (n--) {
		if (wp->w_doto == llength(wp->w_dotp)) {
			if (wp->w_dotp == curbp->b_linep)
				return FALSE;
			wp->w_dotp = lforw(wp->w_dotp);
			wp->w_doto = 0;
			wp->w_flag |= WFMOVE;
		} else
			wp->w_doto++;
	}
	return TRUE;
}

/*
----------------------------------------
	連続してマークを設定
----------------------------------------
*/

int setmultimark(int f, int n)
{
	int multimark = 0, markmax;

	markmax = (mulmax < 0) ? (NMARKS - (-mulmax) % NMARKS) : (mulmax % NMARKS);
	if (markmax < 1)
		markmax = 1;

	{
		WINDOW *wp;
		BUFFER *bp;
		LINE *dotp;
		int doto;

		bp = curbp;
		dotp = curwp->w_dotp;
		doto = curwp->w_doto;

		multimark = bp->b_markno;
		if (multimark) {
			if (bp->b_markp[multimark] != dotp || bp->b_marko[multimark] != doto) {
				if (++multimark > markmax)
					multimark = 1;
				bp->b_markp[multimark] = dotp;
				bp->b_marko[multimark] = doto;
			}
		} else {
			multimark++;
			bp->b_markp[multimark] = dotp;
			bp->b_marko[multimark] = doto;
		}
		bp->b_jumpno = multimark;
		bp->b_markno = multimark;
		bp->b_markp[0] = dotp;
		bp->b_marko[0] = doto;

		for(wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				multimark = wp->w_markno;
				if (multimark) {
					if (wp->w_markp[multimark] != dotp || wp->w_marko[multimark] != doto) {
						if (++multimark > markmax)
							multimark = 1;
						wp->w_markp[multimark] = dotp;
						wp->w_marko[multimark] = doto;
					}
				} else {
					multimark++;
					wp->w_markp[multimark] = dotp;
					wp->w_marko[multimark] = doto;
				}
				wp->w_jumpno = multimark;
				wp->w_markno = multimark;
				wp->w_markp[0] = dotp;
				wp->w_marko[0] = doto;
			}
		}
		mlwrite(KTEX9, multimark);
	}

	return TRUE;
}

/*
========================================
	連続するマークへジャンプ本体
========================================
*/

static int gotomultimark(int dir)
{
	int multimark, markmax;

	markmax = (mulmax < 0) ? (NMARKS - (-mulmax) % NMARKS) : (mulmax % NMARKS);
	if (markmax < 1)
		markmax = 1;

	{
		int jumpno;
		WINDOW *wp = curwp;

		jumpno = wp->w_jumpno;
		multimark = wp->w_markno;

		if (!multimark) {
			mlwrite(KTEX76);
			return FALSE;
		}
		if (wp->w_markp[multimark]
		    && (wp->w_markp[jumpno] != wp->w_dotp
			|| wp->w_marko[jumpno] != wp->w_doto))
			jumpno = multimark;
		else {
			int i;

			if (dir > 0) {
				for (i = 1; i <= markmax; i++) {
					if (--jumpno == 0)
						jumpno = markmax;
					if (wp->w_markp[jumpno])
						break;
				}
			} else {
				for (i = 1; i <= markmax; i++) {
					if (++jumpno > markmax)
						jumpno = 1;
					if (wp->w_markp[jumpno])
						break;
				}
			}
			if (i > markmax) {
				mlwrite(KTEX76);
				return FALSE;
			}
		}

		wp->w_dotp = wp->w_markp[jumpno];
		wp->w_doto = wp->w_marko[jumpno];
		wp->w_markp[0] = wp->w_dotp;
		wp->w_marko[0] = wp->w_doto;
		wp->w_flag |= WFMOVE;

		for(wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == curbp) {
				wp->w_jumpno = jumpno;
				wp->w_markno = multimark;
			}
		}

		mlwrite(KTEX266, jumpno, multimark);
	}

	return TRUE;
}

/*
----------------------------------------
	マークジャンプ（正順）
----------------------------------------
*/

int gotomultimarkf(int f, int n)
{
	return gotomultimark(1);
}

/*
----------------------------------------
	マークジャンプ（逆順）
----------------------------------------
*/

int gotomultimarkr(int f, int n)
{
	return gotomultimark(-1);
}
