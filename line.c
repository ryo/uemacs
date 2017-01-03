/*
----------------------------------------
	LINE.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/line.c,v 1.3 1992/01/31 12:27:44 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int ldelnewline(void);
static void swap_kbf(void);
static void syskillchk(void);

/*
========================================
	kill buffer の swap
========================================
*/

static void swap_kbf(void)
{
	int tmpkud;
	KILL *tmpkbh, *tmpkbp;

	tmpkbh = kbufh;
	tmpkbp = kbufp;
	tmpkud = kused;
	kbufh = kbufh2;
	kbufp = kbufp2;
	kused = kused2;
	kbufh2 = tmpkbh;
	kbufp2 = tmpkbp;
	kused2 = tmpkud;
}

/*
========================================
	system-kill-buffer の入れ換え
========================================
*/

static void syskillchk(void)
{
	if (syskillflg != syskill) {
		swap_kbf();
		syskillflg = syskill;
	}
}

/*
========================================
	行確保
========================================
*/

LINE *lalloc(int used)
{
	LINE *lp;

	lp = (LINE *) malloc(sizeof(LINE) + used);
	if (lp == 0) {
		mlwrite(KTEX99);
		return 0;
	}
	lp->l_size = used;
	lp->l_used = used;
	return lp;
}

/*
========================================
	行解放
========================================
*/

void lfree(LINE *lp)
{
	LINE *next = lforw(lp);

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			int i;

			if (wp->w_linep == lp)
				wp->w_linep = next;
			if (wp->w_dotp == lp) {
				wp->w_dotp = next;
				wp->w_doto = 0;
			}
			{
				LINE **mp;

				for (mp = wp->w_markp, i = 0; i < NMARKS; i++, mp++) {
					if (*mp == lp) {
						*mp = next;
						wp->w_marko[i] = 0;
					}
				}
			}
		}
	}

	{
		BUFFER *bp;

		for (bp = bheadp; bp; bp = bp->b_bufp) {
			int i;

			if (bp->b_dotp == lp) {
				bp->b_dotp = next;
				bp->b_doto = 0;
			}
			{
				LINE **mp;

				for (mp = bp->b_markp, i = 0; i < NMARKS; i++, mp++) {
					if (*mp == lp) {
						*mp = next;
						bp->b_marko[i] = 0;
					}
				}
			}
		}
	}

	lp->l_bp->l_fp = next;
	lp->l_fp->l_bp = lback(lp);

	free(lp);
}

/*
========================================
	行変更
========================================
*/

void lchange(int flag)
{
	BUFFER *bp = curbp;

	if (!intercept_flag && (clexec || kbdmode == PLAY))
		flag = WFHARD;
	if ((bp->b_flag & BFCHG) == 0) {
		flag |= WFMODE;
		bp->b_flag |= BFCHG;
	}

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp)
				wp->w_flag |= flag;
		}
	}
}

/*
----------------------------------------
	空白をまとめる
----------------------------------------
*/

int justonespc(int f, int n)
{
	LINE *dotp;
	int start;
	int used, doto;
	char c, *line;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	line = dotp->l_text;
	used = dotp->l_used;

	c = line[doto];
	if (c == ' ' || c == '\t') {
		while (doto >= 0 && ((c = line[doto]) == ' ' || c == '\t'))
			doto--;
		doto += 2;
		start = doto;
		while (used > doto && ((c = line[doto]) == ' ' || c == '\t'))
			doto++;
		line[start - 1] = ' ';
		curwp->w_doto = start;
		ldelete(doto - start , 0);
	}
	lchange(WFEDIT);

	return TRUE;
}

/*
----------------------------------------
	空白挿入
----------------------------------------
*/

int insspace(int f, int n)
{
	linsert(n, ' ');
	backchar(f, n);
	return TRUE;
}

/*
========================================
	文字列挿入
========================================
*/

int linstr(char *instr)
{
	int status = TRUE;

	if (instr) {
		int c;

		while ((c = *instr++) && status == TRUE) {
			status = (c == I_NEWLINE) ? lnewline() : linsert(1, c);
			if (status != TRUE) {
				mlwrite(KTEX168);
				break;
			}
		}
	}
	return status;
}

/*
========================================
	同文字を複数挿入
========================================
*/

int linsert(int n, char c)
{
	WINDOW *wp = curwp;

	if (c == ' ' && n >= 0 && check_wrap())
		exechook(wraphook);

	{
		LINE *lp1 = wp->w_dotp;
		LINE *lp2;
		int doto = wp->w_doto;

		if (lp1 == curbp->b_linep) {
			LINE *lp3;

			if (wp->w_doto > 0) {
				mlwrite(KTEX170);
				return FALSE;
			}
			lp2 = lalloc(BSIZE(n));
			if (lp2 == 0)
				return FALSE;
			lp2->l_used = n;
			lp3 = lp1->l_bp;
			lp3->l_fp = lp2;
			lp2->l_fp = lp1;
			lp1->l_bp = lp2;
			lp2->l_bp = lp3;

			{
				int i;
				char *p;

				for (p = lp2->l_text, i = 0; i < n; i++)
					*p++ = c;
			}
			lchange(WFEDIT);
		} else {
			if (lp1->l_used + n > lp1->l_size) {
				char *cp1, *cp2;

				lp2 = lalloc(BSIZE(lp1->l_used + n));
				if (lp2 == 0)
					return FALSE;
				lp2->l_used = lp1->l_used + n;
				cp1 = lp1->l_text;
				cp2 = lp2->l_text;
				while (cp1 != lp1->l_text + doto)
					*cp2++ = *cp1++;
				cp2 += n;
				while (cp1 != lp1->l_text + lp1->l_used)
					*cp2++ = *cp1++;
				lp1->l_bp->l_fp = lp2;
				lp2->l_fp = lp1->l_fp;
				lp1->l_fp->l_bp = lp2;
				lp2->l_bp = lp1->l_bp;
				free(lp1);
			} else {
				char *cp1, *cp2;

				lp2 = lp1;
				lp2->l_used += n;
				cp2 = lp1->l_text + lp1->l_used;
				cp1 = cp2 - n;
				while (cp1 != lp1->l_text + doto)
					*--cp2 = *--cp1;
			}

			{
				int i;
				char *p;

				for (p = lp2->l_text + doto, i = 0; i < n; i++)
					*p++ = c;
			}
			lchange(WFEDIT);
		}

		{
			BUFFER *bp;
			int i;

			bp = curbp;

			if (bp->b_dotp == lp1) {
				bp->b_dotp = lp2;
				if (bp->b_doto >= doto)
					bp->b_doto += n;
			}
			for (i = 0; i < NMARKS; i++) {
				if (bp->b_markp[i] == lp1) {
					bp->b_markp[i] = lp2;
					if (bp->b_marko[i] > doto)
						bp->b_marko[i] += n;
				}
			}
		}
		for (wp = wheadp; wp; wp = wp->w_wndp) {
			int i;

			if (wp->w_linep == lp1)
				wp->w_linep = lp2;
			if (wp->w_dotp == lp1) {
				wp->w_dotp = lp2;
				if (wp == curwp || wp->w_doto >= doto)
					wp->w_doto += n;
			}
			for (i = 0; i < NMARKS; i++) {
				if (wp->w_markp[i] == lp1) {
					wp->w_markp[i] = lp2;
					if (wp->w_marko[i] > doto)
						wp->w_marko[i] += n;
				}
			}
		}
	}

	return TRUE;
}

/*
========================================
	上書
========================================
*/

int lowrite(char c)
{
	WINDOW *wp = curwp;

	if (wp->w_doto < wp->w_dotp->l_used
	    && (lgetc(wp->w_dotp, wp->w_doto) != TAB
		|| ((wp->w_doto) % wp->w_bufp->b_tabs == wp->w_bufp->b_tabs - 1)))
		ldelete(1, FALSE);

	return linsert(1, c);
}

/*
========================================
	文字列上書
========================================
*/

int lover(char *ostr)
{
	int status = TRUE;

	if (ostr) {
		int c;

		while ((c = *ostr++) && status == TRUE) {
			status = (c == I_NEWLINE) ? lnewline() : lowrite(c);
			if (status != TRUE) {
				mlwrite(KTEX172);
				break;
			}
		}
	}
	return status;
}

/*
========================================
	空白行挿入
========================================
*/

int lnewline(void)
{
	WINDOW *wp = curwp;

	lchange(WFHARD);

	{
		int doto = wp->w_doto;
		LINE *lp2, *lp1 = wp->w_dotp;

		lp2 = lalloc(doto);
		if (lp2 == 0)
			return FALSE;

		{
			char *cp1, *cp2;

			cp1 = lp1->l_text;
			cp2 = lp2->l_text;
			while (cp1 != lp1->l_text + doto)
				*cp2++ = *cp1++;
			cp2 = lp1->l_text;
			while (cp1 != lp1->l_text + llength(lp1))
				*cp2++ = *cp1++;
		}

		lp1->l_used -= doto;
		lp2->l_bp = lp1->l_bp;
		lp1->l_bp = lp2;
		lp2->l_bp->l_fp = lp2;
		lp2->l_fp = lp1;

		{
			BUFFER *bp;
			int i;

			bp = curbp;

			if (bp->b_dotp == lp1) {
				if (bp->b_doto < doto)
					bp->b_dotp = lp2;
				else
					bp->b_doto -= doto;
			}
			for (i = 0; i < NMARKS; i++) {
				if (bp->b_markp[i] == lp1) {
					if (bp->b_marko[i] < doto)
						bp->b_markp[i] = lp2;
					else
						bp->b_marko[i] -= doto;
				}
			}
		}
		for (wp = wheadp; wp; wp = wp->w_wndp) {
			int i;

			if (wp->w_linep == lp1)
				wp->w_linep = lp2;
			if (wp->w_dotp == lp1) {
				if (wp->w_doto < doto)
					wp->w_dotp = lp2;
				else
					wp->w_doto -= doto;
			}
			for (i = 0; i < NMARKS; i++) {
				if (wp->w_markp[i] == lp1) {
					if (wp->w_marko[i] < doto)
						wp->w_markp[i] = lp2;
					else
						wp->w_marko[i] -= doto;
				}
			}
		}
	}

	return TRUE;
}

/*
========================================
	文字削除
========================================
*/

int ldelete(int n, int kflag)
{
	while (n) {
		LINE *dotp;
		int doto, chunk;

		dotp = curwp->w_dotp;
		doto = curwp->w_doto;
		if (dotp == curbp->b_linep)
			return FALSE;

		chunk = dotp->l_used - doto;
		if (chunk > n)
			chunk = n;
		if (chunk == 0) {
			lchange(WFHARD);
			if (ldelnewline() == FALSE
			    || (kflag != FALSE && kinsert(I_NEWLINE) == FALSE))
				return FALSE;
			n--;
			continue;
		}
		lchange(WFEDIT);

		{
			char *cp1, *cp2;

			cp1 = dotp->l_text + doto;
			cp2 = cp1 + chunk;
			if (kflag != FALSE) {
				while (cp1 != cp2) {
					if (kinsert(*cp1++) == FALSE)
						return FALSE;
				}
				cp1 = dotp->l_text + doto;
			}
			while (cp2 != dotp->l_text + dotp->l_used)
				*cp1++ = *cp2++;
			dotp->l_used -= chunk;
		}

		{
			BUFFER *bp;
			int i;

			bp = curbp;

			if (bp->b_dotp == dotp && bp->b_doto >= doto) {
				bp->b_doto -= chunk;
				if (bp->b_doto < doto)
					bp->b_doto = doto;
			}
			for (i = 0; i < NMARKS; i++) {
				if (bp->b_markp[i] == dotp && bp->b_marko[i] >= doto) {
					bp->b_marko[i] -= chunk;
					if (bp->b_marko[i] < doto)
						bp->b_marko[i] = doto;
				}
			}
		}
		{
			WINDOW *wp;

			for (wp = wheadp; wp; wp = wp->w_wndp) {
				int i;

				if (wp->w_dotp == dotp && wp->w_doto >= doto) {
					wp->w_doto -= chunk;
					if (wp->w_doto < doto)
						wp->w_doto = doto;
				}
				for (i = 0; i < NMARKS; i++) {
					if (wp->w_markp[i] == dotp && wp->w_marko[i] >= doto) {
						wp->w_marko[i] -= chunk;
						if (wp->w_marko[i] < doto)
							wp->w_marko[i] = doto;
					}
				}
			}
		}

		n -= chunk;
	}

	return TRUE;
}

/*
========================================
	行の文字列を得る
========================================
*/

char *getctext(void)
{
	int size;
	char *sp, *dp;
	static char rline[NSTRING];

	size = curwp->w_dotp->l_used;
	if (size >= NSTRING)
		size = NSTRING - 1;

	sp = curwp->w_dotp->l_text;
	dp = rline;
	while (size--)
		*dp++ = *sp++;
	*dp = 0;
	return rline;
}

/*
========================================
	文字列を新行に挿入
========================================
*/

int putctext(char *iline)
{
	int status;

	curwp->w_doto = 0;
	if (llength(curwp->w_dotp)) {
		int old_nokill = nokill;

		nokill = TRUE;
		status = killtext(FALSE, 1);
		nokill = old_nokill;
		if (status != TRUE)
			return status;
	}
	return linstr(iline);
}

/*
========================================
	新行削除
========================================
*/

static int ldelnewline(void)
{
	LINE *lp1, *lp2;

	lp1 = curwp->w_dotp;
	lp2 = lp1->l_fp;
	if (lp2 == curbp->b_linep) {
		if (lp1->l_used == 0)
			lfree(lp1);
		return TRUE;
	}
	if (lp2->l_used <= lp1->l_size - lp1->l_used) {
		{
			char *cp1, *cp2;

			cp1 = lp1->l_text + lp1->l_used;
			cp2 = lp2->l_text;
			while (cp2 != lp2->l_text + lp2->l_used)
				*cp1++ = *cp2++;
		}

		{
			BUFFER *bp;
			int i;

			bp = curbp;

			if (bp->b_dotp == lp2) {
				bp->b_dotp = lp1;
				bp->b_doto += lp1->l_used;
			}
			for (i = 0; i < NMARKS; i++) {
				if (bp->b_markp[i] == lp2) {
					bp->b_markp[i] = lp1;
					bp->b_marko[i] += lp1->l_used;
				}
			}
		}
		{
			WINDOW *wp;

			for (wp = wheadp; wp; wp = wp->w_wndp) {
				int i;

				if (wp->w_linep == lp2)
					wp->w_linep = lp1;
				if (wp->w_dotp == lp2) {
					wp->w_dotp = lp1;
					wp->w_doto += lp1->l_used;
				}
				for (i = 0; i < NMARKS; i++) {
					if (wp->w_markp[i] == lp2) {
						wp->w_markp[i] = lp1;
						wp->w_marko[i] += lp1->l_used;
					}
				}
			}
		}

		lp1->l_used += lp2->l_used;
		lp1->l_fp = lp2->l_fp;
		lp2->l_fp->l_bp = lp1;
		free(lp2);
		return TRUE;
	} else {
		LINE *lp3;

		lp3 = lalloc(lp1->l_used + lp2->l_used);
		if (lp3 == 0)
			return FALSE;

		{
			char *cp1, *cp2;

			cp1 = lp1->l_text;
			cp2 = lp3->l_text;
			while (cp1 != lp1->l_text + lp1->l_used)
				*cp2++ = *cp1++;
			cp1 = lp2->l_text;
			while (cp1 != lp2->l_text + lp2->l_used)
				*cp2++ = *cp1++;
			lp1->l_bp->l_fp = lp3;
			lp3->l_fp = lp2->l_fp;
			lp2->l_fp->l_bp = lp3;
			lp3->l_bp = lp1->l_bp;
		}

		{
			BUFFER *bp;
			int i;

			bp = curbp;

			if (bp->b_dotp == lp1)
				bp->b_dotp = lp3;
			else if (bp->b_dotp == lp2) {
				bp->b_dotp = lp3;
				bp->b_doto += lp1->l_used;
			}
			for (i = 0; i < NMARKS; i++) {
				if (bp->b_markp[i] == lp1)
					bp->b_markp[i] = lp3;
				else if (bp->b_markp[i] == lp2) {
					bp->b_markp[i] = lp3;
					bp->b_marko[i] += lp1->l_used;
				}
			}
		}
		{
			WINDOW *wp;

			for (wp = wheadp; wp; wp = wp->w_wndp) {
				int i;

				if (wp->w_linep == lp1 || wp->w_linep == lp2)
					wp->w_linep = lp3;
				if (wp->w_dotp == lp1)
					wp->w_dotp = lp3;
				else if (wp->w_dotp == lp2) {
					wp->w_dotp = lp3;
					wp->w_doto += lp1->l_used;
				}
				for (i = 0; i < NMARKS; i++) {
					if (wp->w_markp[i] == lp1)
						wp->w_markp[i] = lp3;
					else if (wp->w_markp[i] == lp2) {
						wp->w_markp[i] = lp3;
						wp->w_marko[i] += lp1->l_used;
					}
				}
			}
		}
	}

	free(lp1);
	free(lp2);

	return TRUE;
}

/*
========================================
	キルバッファ削除
========================================
*/

void kdelete(void)
{
	syskillchk();

	if (kbufh && nokill == FALSE) {
		KILL *kfp = kbufh;

		while (kfp) {
			KILL *kp;

			kp = kfp->d_next;
			free(kfp);
			kfp = kp;
		}
		kbufh = kbufp = 0;
		kused = KBLOCK;
	}
}

/*
----------------------------------------
	キルバッファの削除（コマンド）
----------------------------------------
*/

int delkbuf(int f, int n)
{
	kdelete();
	return TRUE;
}

/*
========================================
	キルバッファ挿入
========================================
*/

int kinsert(char c)
{
	syskillchk();

	if (nokill == FALSE) {
		if (kused >= KBLOCK) {
			KILL *nchunk;

			nchunk = (KILL *) malloc(sizeof(KILL));
			if (nchunk == 0)
				return FALSE;
			if (kbufh == 0)
				kbufh = nchunk;
			if (kbufp)
				kbufp->d_next = nchunk;
			kbufp = nchunk;
			kbufp->d_next = 0;
			kused = 0;
		}
		kbufp->d_chunk[kused++] = c;
	}
	return TRUE;
}

/*
----------------------------------------
	ペースト
----------------------------------------
*/

int yank(int f, int n)
{
	char ybuf[2048];

	syskillchk();

	if (kbufh == 0)
		return TRUE;

	while (n--) {
		int crs = 0;
		char *sp, *ecp, *scp = 0, *tcp = 0;
		KILL *kp;

		for (kp = kbufh; kp; kp = kp->d_next) {
			int i = kp->d_next ? KBLOCK : kused;

			for (sp = kp->d_chunk; i--; sp++) {
				if (*sp == I_NEWLINE) {
					if (crs == 0)
						scp = sp;
					crs++;
					tcp = sp;
				}
			}
		}

		ecp = tcp;
		kp = kbufh;

		if (crs == 0 || scp == ecp) {
			for (; kp; kp = kp->d_next) {
				int i = kp->d_next ? KBLOCK : kused;

				for (sp = kp->d_chunk; i--; sp++) {
					int c;

					c = *sp;
					if (c == I_NEWLINE) {
						if (lnewline() == FALSE)
							return FALSE;
					} else {
						if (linsert(1, c) == FALSE)
							return FALSE;
					}
				}
			}
		} else {
			int i, j;

			for (;; kp = kp->d_next) {
				i = kp->d_next ? KBLOCK : kused;
				for (sp = kp->d_chunk; i--; sp++) {
					if (sp == scp) {
						if (lnewline() == FALSE)
							return FALSE;
						j = 0;
						goto next_step;
					} else {
						if (linsert(1, *sp) == FALSE)
							return FALSE;
					}
				}
			}
			for (;; kp = kp->d_next) {
				i = kp->d_next ? KBLOCK : kused;
				for (sp = kp->d_chunk; i--; sp++) {
					int c;

					c = *sp;
					if (c == I_NEWLINE) {
						ybuf[j] = 0;
						if (putline(ybuf) == FALSE)
							return FALSE;
						j = 0;
						if (sp == ecp)
							goto last_step;
					} else
						ybuf[j++] = c;

				      next_step:;
				}
			}
			for (; kp; kp = kp->d_next) {
				i = kp->d_next ? KBLOCK : kused;
				for (sp = kp->d_chunk; i--; sp++) {
					if (linsert(1, *sp) == FALSE)
						return FALSE;

				      last_step:;
				}
			}
		}
	}

	return TRUE;
}

/*
========================================
	高速行追加
========================================
*/

int putline(char *text)
{
	int ntext;
	LINE *lpc;

	ntext = strlen(text);
	lpc = lalloc(BSIZE(ntext));
	if (lpc == 0)
		return FALSE;
	lpc->l_used = ntext;

	{
		int i;
		char *cp1, *cp2;

		cp1 = lpc->l_text;
		cp2 = text;
		for (i = 0; i < ntext; ++i)
			*cp1++ = *cp2++;
	}

	{
		LINE *lpf, *lpb;

		lpf = curwp->w_dotp;
		lpb = lpf->l_bp;
		lpb->l_fp = lpc;
		lpc->l_bp = lpb;
		lpf->l_bp = lpc;
		lpc->l_fp = lpf;
	}

	return TRUE;
}
