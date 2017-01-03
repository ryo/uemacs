/*
----------------------------------------
	DISPLAY.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ehprint.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/display.c,v 1.10 1992/01/05 02:05:54 SALT Exp SALT $'\n""	even\n");

/*
========================================
	マクロの定義
========================================
*/

#define	TAB_CHAR	0x09
#define	TAB_CHAR2	0x0a

#define	TAB_COLOR	0x01
#define	NUM_COLOR	lnumcol
#define	SEP_COLOR	lsepcol
#define	TEXT_COLOR	0x03
#define	EM_COLOR	0x83
#define	EX_COLOR	0x82
#define	CR_COLOR	crcol
#define	DSEL_COLOR	0x01

/*
========================================
	使用関数の定義
========================================
*/

static int check_high(WINDOW *);
static int getlinenum(WINDOW *);
static int mlformat(WINDOW *, char *, char *);
static void modeline(WINDOW *);
static int reframe(WINDOW *);
static void scrollback(WINDOW *);
static void scrollforw(WINDOW *);
static void setnum(char *, int);
static void updall(WINDOW *);
static int updateline(int, VIDEO *);
static void upddex(void);
static void updext(void);
static void updgar(void);
static void updone(WINDOW *, LINE *, int);
static void updpos(void);
static void updupd(int);
static inline void vteeol(void);
static void vtmove(short, short);
static void vtputc(char, short);
static void vtputs(char *, short, short);

/*
========================================
	使用変数の定義
========================================
*/

static short scrollflag = 0;
static short ckanji = ANK;
static char num_attr[8], *eol = "\x0d";
static char *no_attr, *sel_attr;

/*
========================================
	仮想カーソル移動
========================================
*/

static inline void vtmove(short row, short col)
{
	vtrow = row;
	vtcol = col;
}

/*
========================================
	モード行強制リフレッシュ
========================================
*/

void upmode(void)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp)
		wp->w_flag |= WFMODE;
}

/*
========================================
	ウィンドウ強制リフレッシ
========================================
*/

void upwind(void)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp)
		wp->w_flag |= WFHARD | WFMODE;
}

/*
========================================
	カーソル移動
========================================
*/

void movecursor(int row, int col)
{
	if (row != ttrow || col != ttcol) {
		ttrow = row;
		ttcol = col;
		H68move(col, row);
	}
}

/*
========================================
	行末まで消去
========================================
*/

static inline void vteeol(void)
{
	int col = vtcol;

	if (col < 0)
		col = 0;

	vscreen[vtrow]->v_text[col] = 0;
}

/*
========================================
	一文字出力
========================================
*/

static void vtputc(char c, short tabs)
{
	short tnc, vtco;
	char *vtext;

	vtco = vtcol;
	vtext = vscreen[vtrow]->v_text;
	tnc = term.t_ncol - leftmargin + 1;

	if (c == TAB) {
		short tlen, dis, tab_top;

		dis = distab;
		tab_top = 1;
		tlen = tabs - ((vtco + taboff) % tabs);
		ckanji = ANK;

		for (; tlen > 0; tlen--) {
			if (vtco >= tnc) {
				if (vtco == tnc) {
					vtext[tnc - 1] = '$';
					vtext[tnc] = 0;
				}
				vtco++;
				break;
			}
			if (vtco >= 0) {
				vtext[vtco] = dis ? (TAB_CHAR + tab_top) : ' ';
				tab_top = 0;
			}
			vtco++;
		}
	} else if (vtco >= tnc) {
		if (vtco == tnc) {
			if (nthctype(vtext, tnc) == CT_KJ2)
				vtext[tnc - 2] = ' ';
			vtext[tnc - 1] = '$';
			vtext[tnc] = 0;
		}
		vtco++;
	} else if (c < 0x20 || c == 0x7f) {
		ckanji = ANK;
		vtcol = vtco;
		vtputc('^', tabs);
		vtputc(c ^ 0x40, tabs);
		return;
	} else {
		if (vtco > 0)
			vtext[vtco] = c;
		else if (vtco < 0)
			ckanji = (ckanji == KANJI1) ? KANJI2 : (iskanji(c) ? KANJI1 : ANK);
		else
			vtext[0] = (ckanji == KANJI1) ? ' ' : c;

		vtco++;
	}
	vtcol = vtco;
}

/*
========================================
	一行出力
========================================
*/

static void vtputs(char *s, short l, short tabs)
{
	short tnc, vtco;
	char *vtext;

	vscreen[vtrow]->v_crflag = 1;

	vtco = vtcol;
	ckanji = ANK;
	vtext = vscreen[vtrow]->v_text;
	tnc = term.t_ncol - leftmargin + 1;

	while (l--) {
		int c;

		c = *s++;

		if (c == TAB) {
			short tlen, dis, tab_top;

			dis = distab;
			tab_top = 1;
			tlen = tabs - ((vtco + taboff) % tabs);
			ckanji = ANK;

			for (; tlen > 0; tlen--) {
				if (vtco >= tnc) {
					if (vtco == tnc) {
						vtext[tnc - 1] = '$';
						vtext[tnc] = 0;
						vscreen[vtrow]->v_crflag = 0;
					}
					vtco++;
					break;
				}
				if (vtco >= 0) {
					vtext[vtco] = dis ? (TAB_CHAR + tab_top) : ' ';
					tab_top = 0;
				}
				vtco++;
			}
		} else if (vtco >= tnc) {
			if (vtco == tnc) {
				if (nthctype(vtext, tnc) == CT_KJ2)
					vtext[tnc - 2] = ' ';
				vtext[tnc - 1] = '$';
				vtext[tnc] = 0;
				vscreen[vtrow]->v_crflag = 0;
			}
			vtco++;
			break;
		} else if (c < 0x20 || c == 0x7f) {
			ckanji = ANK;
			vtcol = vtco;
			vtputc('^', tabs);
			vtputc(c ^ 0x40, tabs);
			vtco = vtcol;
		} else {
			if (vtco > 0)
				vtext[vtco] = c;
			else if (vtco < 0)
				ckanji = (ckanji == KANJI1) ? KANJI2 : (iskanji(c) ? KANJI1 : ANK);
			else
				vtext[0] = (ckanji == KANJI1) ? ' ' : c;

			vtco++;
		}
	}
	if (vtco < 0)
		vscreen[vtrow]->v_crflag = 0;

	vtcol = vtco;
}

/*
========================================
	画面表示初期化
========================================
*/

void vtinit(void)
{
	int maxcol, maxrow;

	H68open();

	maxcol = term.t_mcol + 1;
	maxrow = term.t_mrow;

	vscreen = (VIDEO **)malloc(maxrow * sizeof(VIDEO *));
	if (vscreen == 0)
		meexit(1);

	no_attr = (char *)malloc(maxcol);
	if (no_attr == 0)
		meexit(1);
	memset(no_attr, TEXT_COLOR, maxcol);
	sel_attr = (char *)malloc(maxcol);
	if (sel_attr == 0)
		meexit(1);
	memset(sel_attr, DSEL_COLOR, maxcol);
	memset(num_attr, NUM_COLOR, 8);
	num_attr[5] = SEP_COLOR;

	{
		int i;

		for (i = 0; i < maxrow; ++i) {
			VIDEO *vp;

			vp = (VIDEO *)malloc(sizeof(VIDEO) + maxcol);
			if (vp == 0)
				meexit(1);

			vp->v_flag = 0;
			vp->v_linestr[5] = 0x01;
			vp->v_linestr[6] = 0;
			vp->v_rbcolor = 13;
			vscreen[i] = vp;
		}
	}
}

/*
========================================
	画面終了
========================================
*/

void vttidy(void)
{
	mlerase();
	mlmovecursor(0);
	H68close();
}

/*
----------------------------------------
	画面アップデート
----------------------------------------
*/

int upscreen(int f, int n)
{
	update(TRUE);
	return TRUE;
}

/*
========================================
	画面アップデート本体
========================================
*/

int update(int force)
{
	LINE *editline;
	short editcol;

	if (sgarbf != FALSE)
		updgar();

	if (scrollflag) {
		scrollflag = FALSE;
		force = TRUE;
	}
	if (force == FALSE && (typahead() || kbdmode == PLAY))
		return TRUE;

	H68curoff();

	editline = curwp->w_dotp;
	editcol = curwp->w_doto;

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_flag) {
				reframe(wp);

				if (scrollflag)
					force = TRUE;

				if ((wp->w_flag & ~WFMODE) == WFEDIT)
					updone(wp, editline, editcol);
				else if (wp->w_flag & ~WFMOVE)
					updall(wp);
				if (wp->w_flag & WFMODE)
					modeline(wp);

				wp->w_flag = 0;
				wp->w_force = 0;
			}
		}
	}

	updpos();
	upddex();
	updupd(force);
	sccurcol = curcol - lbound + leftmargin;
	sccurline = currow;
	movecursor(sccurline, sccurcol);
	if (updinsf) {
		adj_ins_mode();
		updinsf = FALSE;
		if (*inshook) {
			int save;
			char tmp[NBUFN];

			strcpy(tmp, (curbp->b_mode & MDOVER) ? "1 " : "2 ");
			strcat(tmp, inshook);
			save = macbug;
			macbug = FALSE;
			exechook(tmp);
			macbug = save;
		}
	}

	H68curon();

	return TRUE;
}

/*
========================================
	現在のウィンドウのリフレーム
========================================
*/

static int reframe(WINDOW *wp)
{
	short nlines;
	LINE *bottom = wp->w_bufp->b_linep;

	nlines = wp->w_ntrows + ((modeflag == FALSE) ? 1 : 0);
	if (!(wp->w_flag & WFFORCE)) {
		LINE *lp = wp->w_linep;

		if (sscroll
		    && (wp->w_dotp == lback(lp))
		    && (!(wp->w_flag & (WFEDIT | WFHARD)))
		    && (wp->w_dotp != bottom)
		    && (wp->w_dotp != lback(bottom))
		    && (wp->w_ntrows > 1)) {
			keydrops();
			scrollback(wp);
			wp->w_linep = wp->w_dotp;
			wp->w_flag |= WFEDIT;
			wp->w_flag &= ~(WFFORCE | WFHARD | WFMOVE);
			scrollflag = TRUE;
			return TRUE;
		}

		{
			short i;

			for (i = 0; i < nlines; i++, lp = lforw(lp)) {
				if (lp == wp->w_dotp)
					return TRUE;
				if (lp == bottom)
					break;
			}
		}

		if (sscroll
		    && (wp->w_dotp == lp)
		    && (!(wp->w_flag & (WFEDIT | WFHARD)))
		    && (wp->w_ntrows > 1)) {
			keydrops();
			scrollforw(wp);
			wp->w_linep = lforw(wp->w_linep);
			wp->w_flag |= WFEDIT;
			wp->w_flag &= ~(WFFORCE | WFHARD | WFMOVE);
			scrollflag = TRUE;
			return TRUE;
		}
	}

	{
		short i = wp->w_force;

		if (i > 0) {
			if (--i >= nlines)
				i = nlines - 1;
		} else if (i < 0) {
			i += nlines;
			if (i < 0)
				i = 0;
		} else
			i = nlines / 2;

		{
			LINE *lp;

			for (lp = wp->w_dotp; i != 0 && lback(lp) != bottom; i--, lp = lback(lp));
			wp->w_linep = lp;
		}
	}

	wp->w_flag |= WFHARD;
	wp->w_flag &= ~WFFORCE;
	return TRUE;
}

/*
========================================
	順方向スクロール
========================================
*/

static void scrollforw(WINDOW *wp)
{
	short toprow, height;

	toprow = wp->w_toprow;
	height = wp->w_ntrows - (modeflag == TRUE ? 1 : 0);

	{
		short i, tcol = sizeof(VIDEO);
		VIDEO **vp1 = &vscreen[toprow];
		VIDEO **vp2 = &vscreen[toprow + 1];

		for (i = 0; i < height; i++) {
			LDIRL(vp2[i], vp1[i], tcol);
			vp1[i]->v_flag &= ~VFCHG;
		}
	}

	{
		int pos, size;
		int src, dst;
		int nras;

		nras = (density >= 0) ? (4 >> density) : 3;
		src = (toprow + 1) * nras;
		dst = src - nras;
		if (sscroll_slow) {
			if (density >= 0) {
				pos = ((src - nras / 2) << 8) + dst;
				size = height * nras + nras / 2;
				TXRASCOPY(pos, size, 0x0003);
				TXRASCOPY(pos, size, 0x0003);
			} else {
				pos = ((src - 1) << 8) + dst;
				size = height * nras + 1;
				TXRASCOPY(pos, size, 0x0003);
				pos = ((src - 2) << 8) + dst;
				size = height * nras + 2;
				TXRASCOPY(pos, size, 0x0003);
			}
		} else {
			pos = (src << 8) + dst;
			size = height * nras;
			TXRASCOPY(pos, size, 0x0003);
		}
	}
}

/*
========================================
	逆方向スクロール
========================================
*/

static void scrollback(WINDOW *wp)
{
	short toprow, height;

	height = wp->w_ntrows - (modeflag == TRUE ? 1 : 0);
	toprow = wp->w_toprow;

	{
		short i, tcol = sizeof(VIDEO);
		VIDEO **vp1 = &vscreen[toprow];
		VIDEO **vp2 = &vscreen[toprow + 1];

		for (i = height - 1; i >= 0; i--) {
			LDIRL(vp1[i], vp2[i], tcol);
			vp2[i]->v_flag &= ~VFCHG;
		}
	}
	toprow += height;

	{
		int pos, size;
		int src, dst;
		int nras;

		nras = (density >= 0) ? (4 >> density) : 3;
		src = toprow * nras - 1;
		dst = src + nras;
		if (sscroll_slow) {
			if (density >= 0) {
				pos = ((src + nras / 2) << 8) + dst;
				size = height * nras + nras / 2;
				TXRASCOPY(pos, size, 0x8003);
				TXRASCOPY(pos, size, 0x8003);
			} else {
				pos = ((src + 1) << 8) + dst;
				size = height * nras + 1;
				TXRASCOPY(pos, size, 0x8003);
				pos = ((src + 2) << 8) + dst;
				size = height * nras + 2;
				TXRASCOPY(pos, size, 0x8003);
			}
		} else {
			pos = (src << 8) + dst;
			size = height * nras;
			TXRASCOPY(pos, size, 0x8003);
		}
	}
}

/*
========================================
	現在行をアップデート
========================================
*/

static void updone(WINDOW *wp, LINE *editedline, int editedcol)
{
	LINE *lp;
	int i;
	short sline, maxrow;

	lp = wp->w_linep;
	sline = wp->w_toprow;
	maxrow = wp->w_ntrows + ((modeflag == FALSE) ? 1 : 0);

	{
		LINE *eob = wp->w_bufp->b_linep;
		short found = 0;

		for (i = 0; i < maxrow; i++, sline++, lp = lforw(lp)) {
			if (lp == editedline) {
				found = 1;
				break;
			}
			if (lp == eob)
				break;
		}
		if (!found)
			return;
	}

	{
		VIDEO **vpp, *vp;

		vpp = &vscreen[sline];
		vp = *vpp;
		if (leftmargin) {
			int lc;

			if (i)
				lc = vpp[-1]->v_linecnt + 1;
			else {
				if (maxrow == 1 || wp->w_bufp->b_linep == lback(editedline)) {
					LINE *lp;

					lc = 0;
					for(lp = wp->w_bufp->b_linep; lp != editedline; lp = lforw(lp))
						lc++;
				} else
					lc = vpp[1]->v_linecnt - 1;
			}
			setnum(vp->v_linestr, vp->v_linecnt = lc);
		}
		vp->v_flag |= VFCHG;
		vp->v_flag &= ~VFREQ;
		vp->v_high = check_high(wp);

		taboff = wp->w_fcol;
		vtmove(sline, -taboff);

		if (lp != wp->w_bufp->b_linep) {
			vtputs(lp->l_text, llength(lp), wp->w_bufp->b_tabs);
			vp->v_rbcolor = wp->w_bufp->b_mlcolor;
			vteeol();
		} else
			*(vscreen[vtrow]->v_text) = 0x01;
	}

	taboff = 0;
}

/*
========================================
	ウィンドウをアップデート
========================================
*/

static void updall(WINDOW *wp)
{
	LINE *lp, *bottom;
	int sline, nlines, linenc, high;

	memset(num_attr, NUM_COLOR, 8);
	num_attr[5] = SEP_COLOR;

	lp = wp->w_linep;
	taboff = wp->w_fcol;
	nlines = wp->w_ntrows + ((modeflag == FALSE) ? 1 : 0);
	bottom = wp->w_bufp->b_linep;
	linenc = leftmargin ? getlinenum(wp) : 0;
	high = check_high(wp);

	for (sline = wp->w_toprow; sline < wp->w_toprow + nlines; sline++) {
		{
			VIDEO *vp;

			vp = vscreen[sline];

			if (leftmargin) {
				setnum(vp->v_linestr, linenc);
				vp->v_linecnt = linenc++;
			}
			vp->v_flag |= VFCHG;
			vp->v_flag &= ~VFREQ;
			vp->v_high = high;
		}

		vtmove(sline, -taboff);

		if (lp != bottom) {
			vtputs(lp->l_text, llength(lp), wp->w_bufp->b_tabs);
			lp = lforw(lp);
			vteeol();
		} else
			vscreen[vtrow]->v_text[0] = 0x01;
	}

	taboff = 0;
}

/*
========================================
	カーソル位置をアップデート
========================================
*/

static void updpos(void)
{
	short i, curr, curc;
	WINDOW *wp;

	wp = curwp;

	{
		LINE *lp;

		curr = wp->w_toprow;
		for (lp = wp->w_linep; lp != wp->w_dotp; curr++, lp = lforw(lp));
	}

	{
		short tabs = wp->w_bufp->b_tabs;
		char *p = wp->w_dotp->l_text;

		for (curc = 0, i = 0; i < wp->w_doto; i++) {
			char c;

			c = *p++;

			if (c == TAB)
				curc += tabs - (curc % tabs);
			else if (c < 0x20 || c == 0x7f)
				curc += 2;
			else {
				if (iskanji(c)) {
					if (c != 0x80 && c < 0xf0)
						curc++;
					i++;
					p++;
				}
				curc++;
			}
		}
	}

	curc -= wp->w_fcol;

	while (curc < 0) {
		if (wp->w_fcol >= hjump) {
			curc += hjump;
			wp->w_fcol -= hjump;
		} else {
			curc += wp->w_fcol;
			wp->w_fcol = 0;
		}
		wp->w_flag |= WFHARD | WFMODE;
	}

	if (hscroll) {
		while (curc >= term.t_ncol - leftmargin - 1) {
			curc -= hjump;
			wp->w_fcol += hjump;
			wp->w_flag |= WFHARD | WFMODE;
		}
	} else {
		if (curc >= term.t_ncol - leftmargin - 1) {
			vscreen[curr]->v_flag |= (VFEXT | VFCHG);
			curcol = curc;
			currow = curr;
			updext();
			curc = curcol;
			curr = currow;
		} else
			lbound = 0;
	}

	curcol = curc;
	currow = curr;

	if (wp->w_flag & WFHARD)
		updall(wp);
	if (wp->w_flag & WFMODE)
		modeline(wp);
	wp->w_flag = 0;
}

/*
========================================
	拡張行のアップデート
========================================
*/

static void upddex(void)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		int i, nl;
		LINE *lp;

		nl = wp->w_ntrows + ((modeflag == FALSE) ? 1 : 0);
		lp = wp->w_linep;
		for (i = wp->w_toprow; i < wp->w_toprow + nl; i++, lp = lforw(lp)) {
			VIDEO *vp = vscreen[i];

			if (vp->v_flag & VFEXT) {
				if (wp != curwp
				    || lp != wp->w_dotp
				    || curcol < term.t_ncol - leftmargin - 1) {
					taboff = wp->w_fcol;
					vtmove(i, -taboff);
					vtputs(lp->l_text, llength(lp), wp->w_bufp->b_tabs);
					vteeol();
					taboff = 0;
					vp->v_flag &= ~VFEXT;
					vp->v_flag |= VFCHG;
				}
			}
		}
	}
}

/*
========================================
	ガベージ処理
========================================
*/

static void updgar(void)
{
	sgarbf = FALSE;
	mpresf = FALSE;

	H68clear();
	upwind();
	mlerase();
}

/*
========================================
	仮想画面から実画面へ
========================================
*/

static void updupd(int force)
{
	short i;

	for (i = 0; i < term.t_nrow; i++) {
		VIDEO *vp;

		vp = vscreen[i];
		if (vp->v_flag & VFCHG)
			updateline(i, vp);
	}

	H68move(curcol - lbound + leftmargin, currow);
}

/*
========================================
	拡張行（拡張後）処理
========================================
*/

static void updext(void)
{
	WINDOW *wp = curwp;

	{
		short rcur;

		rcur = ((curcol - (term.t_ncol - leftmargin)) % term.t_scrsiz) + term.t_margin;
		lbound = curcol - rcur + 1;
	}

	taboff = lbound + wp->w_fcol;
	vtmove(currow, -taboff);

	{
		short j;
		LINE *lp;

		ckanji = ANK;

		for (lp = curwp->w_dotp, j = 0; j < llength(lp); j++)
			vtputc(lgetc(lp, j), curwp->w_bufp->b_tabs);
		vteeol();
		if (nthctype(lp->l_text, taboff + 1) == CT_KJ1)
			vscreen[currow]->v_text[1] = ' ';
	}

	taboff = 0;

	vscreen[currow]->v_text[0] = '$';
}

/*
========================================
	一行表示
========================================
*/

static int updateline(int row, VIDEO *vp)
{
	char v_attr[512];
	char cr_attr[4], work[512];

	if (vp->v_flag & VFREQ) {
		short len;
		char *p, *q;

		memset(v_attr, vp->v_rbcolor, term.t_ncol + 1);
		p = vp->v_text;
		q = work;
		while (*q++ = *p++);
		len = term.t_ncol - (p - vp->v_text - 1);
		if (len > 0)
			memset(q - 1, ' ', len);
		work[term.t_ncol + 1] = 0;
		if (nthctype(work, term.t_ncol + 1) == CT_KJ1)
			work[term.t_ncol] = ' ';
		vp->v_bcolor = vp->v_rbcolor;

		H_PRINT3(0, row, "", num_attr, work, v_attr, "", no_attr, diszen);
	} else {
		if (*vp->v_text == 0x01) {
			H_ERA(row);
		} else {
			char *v;
			char m;
			int v_high = vp->v_high;

			if (v_high == 1 || v_high == 3) {
				int emc;
				char ch, *p;

				m = (v_high == 1) ? 1 : 2;
				emc = colvis ? EX_COLOR : EM_COLOR;

				for (v = v_attr, p = vp->v_text; ch = *p;) {
					if (!iskeyword(ch, m)) {
						if (diszen) {
							char ch;

							while ((ch = *p++) && !iskeyword(ch, m)) {
								if (ch == 0x81 && *p == 0x40)
									*v++ = TAB_COLOR;
								else
									*v++ = (ch == TAB_CHAR || ch == TAB_CHAR2)
									    ? TAB_COLOR : TEXT_COLOR;
								if (iskanji(ch)) {
									v++;
									p++;
								}
							}
						} else {
							char	ch;

							while ((ch = *p++) && !iskeyword(ch, m)) {
								*v++ = (ch == TAB_CHAR || ch == TAB_CHAR2)
								    ? TAB_COLOR : TEXT_COLOR;
								if (iskanji(ch)) {
									v++;
									p++;
								}
							}
						}
						p--;
					} else {
						int len;
						char ch, cd;
						char *mp;

						for (mp = p; ch = *++p, iskeyword(ch, m);)
							;
						len = p - mp;
						*p = 0;
						if (v_high == 1)
							cd = c_in_word_set(mp, len) ? emc : TEXT_COLOR;
						else
							cd = keyword_in_word_set(mp, len) ? emc : TEXT_COLOR;
						for (; len--; *v++ = cd)
							;
						*p = ch;
					}
				}
				v = v_attr;
			} else if (vp->v_high == 2 && *vp->v_text == '*')
				v = sel_attr;
			else
				v = no_attr;

			if ((distab || diszen) && v != v_attr) {
				char ch, *p, *q;

				if (distab && diszen) {
					for (p = v_attr, q = vp->v_text; ch = *q++;) {
						if (ch == TAB_CHAR || ch == TAB_CHAR2 || ch == 0x81 && *q == 0x40)
							*p++ = TAB_COLOR;
						else {
							*p++ = *v++;
							if (iskanji(ch)) {
								v++;
								p++;
								q++;
							}
						}
					}
				} else if (distab) {
					for (p = v_attr, q = vp->v_text; ch = *q++;) {
						if (ch == TAB_CHAR || ch == TAB_CHAR2)
							*p++ = TAB_COLOR;
						else {
							*p++ = *v++;
							if (iskanji(ch)) {
								v++;
								p++;
								q++;
							}
						}
					}
				} else {
					/* diszen == TRUE */
					for (p = v_attr, q = vp->v_text; ch = *q++;) {
						if (ch == 0x81 && *q == 0x40)
							*p++ = TAB_COLOR;
						else
							*p++ = *v++;
						if (iskanji(ch)) {
							v++;
							p++;
							q++;
						}
					}
				}
				v = v_attr;
			}
			*cr_attr = (dispcr && vp->v_high != 2) ? CR_COLOR : 0;

			{
				char *eolmark = eol;

				if (!vscreen[row]->v_crflag)
					eolmark = "";
				if (leftmargin)
					H_PRINT3(0, row, vp->v_linestr, num_attr, vp->v_text, v, eolmark, cr_attr, diszen);
				else
					H_PRINT3(0, row, "", num_attr, vp->v_text, v, eolmark, cr_attr, diszen);
			}
		}
	}

	vp->v_flag &= ~VFCHG;

	return TRUE;
}

/*
========================================
	モード行アップデート
========================================
*/

static void modeline(WINDOW *wp)
{
	char tline[NLINE];

	if (modeflag == FALSE)
		return;

	ckanji = ANK;

	{
		short n, oldleft = leftmargin;

		leftmargin = 0;
		n = wp->w_toprow + wp->w_ntrows;
		vscreen[n]->v_flag |= VFCHG | VFREQ | VFCOL;
		vscreen[n]->v_rbcolor = wp->w_bufp->b_mlcolor;
		vtmove(n, 0);
		n = mlformat(wp, tline, wp->w_bufp->b_mlform);

		{
			char *cp;

			for (cp = tline; n > 0; n--)
				vtputc(*cp++, wp->w_bufp->b_tabs);
			vteeol();
		}

		leftmargin = oldleft;
	}
}

/*
----------------------------------------
	モード行フォーマット
----------------------------------------
*/

static int mlformat(WINDOW *wp, char *line, char *format)
{
	char c, lchar;
	char *linep, var[NSTRING];
	BUFFER *bp;
	static const char *week_name = "日月火水木金土";

	lchar = (wp == curwp) ? '=' : '-';
	bp = wp->w_bufp;
	for (linep = line; c = *format; format++) {
		if (iskanji(c)) {
			*linep++ = c;
			*linep++ = *++format;
			continue;
		}
		switch (c) {
		case '?':
			switch (c = *++format) {
			case '#':
				*linep++ = (bp->b_flag & BFTRUNC) ? '#' : lchar;
				break;
			case '*':
				*linep++ = (bp->b_flag & BFCHG) ? '*' : lchar;
				break;
			case 'n':
			case 'N':
				if (bp->b_flag & BFNAROW) {
					*linep++ = '<';
					*linep++ = '>';
				} else {
					*linep++ = lchar;
					*linep++ = lchar;
				}
				break;
			case 'p':
			case 'P':
				strcpy(linep, PROGNAME);
				linep += strlen(linep);
				break;
			case 'v':
			case 'V':
				strcpy(linep, VERSION);
				linep += strlen(linep);
				break;
			case 'l':
				if (wp->w_fcol > 0) {
					*linep++ = '[';
					*linep++ = '<';
					strcpy(linep, int_asc(wp->w_fcol));
					linep += strlen(linep);
					*linep++ = ']';
				}
				break;
			case 'L':
				if (wp->w_fcol > 0) {
					strcpy(linep, int_asc(wp->w_fcol));
					linep += strlen(linep);
				}
				break;
			case 'r':
			case 'R':
				strcpy(linep, int_asc(malloc_rating));
				linep += strlen(linep);
				break;
			case 'm':
			case 'M':
				{
					int i, mod, firstm = 1;

					for (i = 0, mod = 1; i < NUMMODES; i++, mod <<= 1) {
						if (wp->w_bufp->b_mode & mod) {
							if (!firstm)
								*linep++ = ' ';
							else
								firstm = 0;
							if (c == 'M')
								*linep++ = modecode[i];
							else {
								strcpy(linep, (char *)modename[i]);
								linep += strlen(linep);
							}
						}
					}
				}
				break;
			case 'b':
			case 'B':
				strcpy(linep, bp->b_bname);
				linep += strlen(linep);
				break;
			case 'f':
				if (*bp->b_fname) {
					*linep++ = ' ';
					strcpy(linep, (english ? TEXT34 : KTEX34));
					strcat(linep, bp->b_fname);
					linep += strlen(linep);
					*linep++ = ' ';
				}
				break;
			case 'F':
				if (*bp->b_fname) {
					strcpy(linep, bp->b_fname);
					linep += strlen(linep);
				}
				break;
			case '=':
				*linep++ = lchar;
				break;
			case '?':
				*linep++ = '?';
				break;
			case 't':
			case 'T':
				{
					time_t now;
					struct tm *tmp;

					time(&now);
					tmp = localtime(&now);
					if (c == 't')
						sprintf(linep, "%d:%02d%s"
							,(tmp->tm_hour > 12 ? tmp->tm_hour - 12 : tmp->tm_hour)
							,(tmp->tm_min)
							,(tmp->tm_hour > 11 ? "pm" : "am"));
					else
						sprintf(linep, "%d:%02d", tmp->tm_hour, tmp->tm_min);
				}
				linep += strlen(linep);
				break;
			case 'd':
			case 'D':
				{
					char *varp;
					time_t now;
					struct tm *tmp;

					time(&now);
					tmp = localtime(&now);
					varp = asctime(tmp);
					switch (c = *++format) {
					case 'm':
						sprintf(linep, "%d", tmp->tm_mon + 1);
						break;
					case 'M':
						strncpy(linep, varp + 4, 3);
						linep[3] = 0;
						break;
					case 'd':
					case 'D':
						sprintf(linep, "%d", tmp->tm_mday);
						break;
					case 'w':
						strncpy(linep, varp, 3);
						linep[3] = 0;
						break;
					case 'W':
						strncpy(linep, (char *)week_name + (tmp->tm_wday << 1), 2);
						linep[2] = 0;
						break;
					case 'y':
					case 'Y':
						sprintf(linep, "%d", tmp->tm_year + (c == 'y' ? 0 : 1900));
						break;
					}
				}
				linep += strlen(linep);
				break;
			default:
				*linep++ = c;
				break;
			}
			break;
		case '$':
			if (format[1] == '$') {
				*linep++ = '$';
				format++;
			} else {
				char *varp;

				for (varp = var; (c = *++format) != '$' && c != 0;)
					*varp++ = c;
				if (c == 0)
					goto blockout;
				*varp = 0;
				varp = gtenv(var);
				if (varp) {
					strcpy(linep, varp);
					linep += strlen(linep);
				}
			}
			break;
		case '%':
			if (format[1] == '%') {
				*linep++ = '%';
				format++;
			} else {
				char *varp;

				for (varp = var; (c = *++format) != '%' && c != 0;)
					*varp++ = c;
				if (c == 0)
					goto blockout;
				*varp = 0;
				varp = gtusr(var);
				if (varp) {
					strcpy(linep, varp);
					linep += strlen(linep);
				}
			}
			break;
		default:
			*linep++ = c;
			break;
		}
	}

blockout:

	{
		short i;

		*linep = 0;
		for (i = strlen(line); i <= term.t_ncol; i++)
			*linep++ = lchar;
		*linep = 0;

		return i;
	}
}

/*
========================================
	行番号を得る
========================================
*/

static int getlinenum(WINDOW *wp)
{
	int i;
	LINE *lp, *cp;

	cp = wp->w_linep;
	for (i = 0, lp = wp->w_bufp->b_linep; lp != cp; lp = lforw(lp), i++);
	return i;
}

/*
----------------------------------------
	行番号表示トグル
----------------------------------------
*/

int swlinenum(int f, int n)
{
	if (disnum) {
		leftmargin = 0;
		disnum = FALSE;
	} else {
		leftmargin = 6;
		disnum = TRUE;
	}
	upwind();
	return TRUE;
}

/*
----------------------------------------
	行番号表示
----------------------------------------
*/

void setnum(char *s, int num)
{
	short i;

	for (i = 4; i >= 0; --i) {
		if (num > 0) {
			s[i] = '0' + (num % 10);
			num /= 10;
		} else
			s[i] = ' ';
	}
}

/*
========================================
	強調チェック
========================================
*/

static int check_high(WINDOW *wp)
{
	int bmode = wp->w_bufp->b_mode;
	int emp_text = wp->w_bufp->b_emp_text;

	if (emp_text)
		return 3;
	else if ((bmode & MDC) && !(bmode & MDWRAP) && chigh)
		return 1;
	else if (bmode & MDDIRED)
		return 2;

	return 0;
}

/*
----------------------------------------
	バージョン表示
----------------------------------------
*/

int veremacs(int f, int n)
{
	mlwrite(KTEX265, PROGNAME, ORGVER, VERSION, RELEASE, MODIFIER);
	return TRUE;
}
