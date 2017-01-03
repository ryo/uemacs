/*
----------------------------------------
	RANDOM.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/random.c,v 1.4 1992/01/04 13:11:24 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int adjustmode(int, int);
static int countbyte(int);
static int entab(int);
static int detab(int);

/*
========================================
	バイト数計算
========================================
*/

static int countbyte(int n)
{
	int co, cnt;
	LINE *cp;
	WINDOW *wp = curwp;

	cp = wp->w_dotp;
	co = wp->w_doto;

	for (cnt = 0; n; n--, cnt++) {
		if (iskanji(lgetc2(wp->w_dotp, wp->w_doto)))
			cnt++;
		forwchar(FALSE, 1);
	}

	wp->w_dotp = cp;
	wp->w_doto = co;

	return cnt;
}

/*
----------------------------------------
	bell
----------------------------------------
*/

int bell(int f, int n)
{
	H68beep();
	return TRUE;
}

/*
----------------------------------------
	fillcolumn 設定
----------------------------------------
*/

int setfillcol(int f, int n)
{
	if (n == 1)
		n = getccol(FALSE) ? : 1;
	if (n < 2)
		n = 2;
	fillcol = n;
	mlwrite(KTEX59, n);

	return TRUE;
}

/*
----------------------------------------
	カーソル位置表示
----------------------------------------
*/

int showcpos(int f, int n)
{
	int predchars = 0;
	int predlines = 0;
	int numchars, numlines, curchar;
	LINE *lp;
	WINDOW *wp = curwp;

	numchars = curchar = numlines = 0;

	for (lp = lforw(curbp->b_linep); lp != curbp->b_linep; lp = lforw(lp)) {
		if (lp == wp->w_dotp) {
			predlines = numlines;
			predchars = numchars + wp->w_doto;

			curchar = lgetc2(lp, wp->w_doto);
			if (iskanji(curchar))
				curchar = ((curchar << 8) | lgetc(lp, wp->w_doto + 1));
		}
		numlines++;
		numchars += llength(lp) + 1;
	}

	if (wp->w_dotp == curbp->b_linep) {
		predlines = numlines;
		predchars = numchars;
	}

	{
		int col, ecol;

		col = getccol(FALSE);

		{
			int savepos;

			savepos = wp->w_doto;
			wp->w_doto = llength(wp->w_dotp);
			ecol = getccol(FALSE);
			wp->w_doto = savepos;
		}

		{
			int ratio;

			ratio = (numchars > 0) ? (predchars * 100 / numchars) : 0;
			mlwrite(KTEX60
				,predlines + 1
				,numlines + 1
				,col
				,ecol
				,predchars
				,numchars
				,ratio
				,curchar);
		}
	}

	return TRUE;
}

/*
========================================
	現在行の番号を得る
========================================
*/

int getcline(void)
{
	int n;
	LINE *lp;

	for (n = 0, lp = lforw(curbp->b_linep); lp != curbp->b_linep; lp = lforw(lp), n++) {
		if (lp == curwp->w_dotp)
			break;
	}
	return n + 1;
}

/*
========================================
	現在桁を得る
========================================
*/

int getccol(int blank)
{
	int i, col;
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;

	for (col = 0, i = 0; i < wp->w_doto; i++) {
		int ch;

		ch = lgetc(wp->w_dotp, i);
		if (blank) {
			if (ch == 0x81 && lgetc(wp->w_dotp, i + 1) == 0x40)
				i++, col += 2;
			else if (ch == ' ')
				col++;
			else if (ch == TAB)
				col += -(col % bp->b_tabs) + bp->b_tabs;
			else
				break;
		} else {
			if (iskanji(ch))
				col++, i++;
			else if (ch == TAB)
				col += -(col % bp->b_tabs) + bp->b_tabs - 1;
			else if (ch < 0x20 || ch == 0x7f)
				col++;

			col++;
		}
	}

	return col;
}

/*
========================================
	現在桁を設定
========================================
*/

int setccol(int pos)
{
	int i, col;
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;

	for (col = 0, i = 0; i < llength(wp->w_dotp); ++i) {
		int ch;

		if (col >= pos)
			break;

		ch = lgetc(wp->w_dotp, i);
		if (iskanji(ch))
			col++, i++;
		else if (ch == TAB)
			col += -(col % bp->b_tabs) + bp->b_tabs - 1;
		else if (ch < 0x20 || ch == 0x7f)
			col++;

		col++;
	}
	wp->w_doto = i;

	return col >= pos;
}

/*
----------------------------------------
	文字交換
----------------------------------------
*/

int twiddle(int f, int n)
{
	WINDOW *wp = curwp;
	LINE *lp;
	char buf[5], *p;

	lp = wp->w_dotp;
	if (llength(lp) < 2 || llength(lp) == 2 && iskanji(lgetc(lp, 0)))
		return FALSE;

	if (wp->w_doto != llength(lp))
		forwchar(FALSE, 1);

	{
		int i;
		int ch;

		for(p = buf, i = 0; i < 2; i++) {
			backchar(FALSE, 1);
			*p++ = ch = lgetc(lp, wp->w_doto);
			if (iskanji(ch))
				*p++ = lgetc(lp, wp->w_doto + 1);
			forwdel(FALSE, 1);
		}
		*p = 0;
	}
	linstr(buf);

	return TRUE;
}

/*
----------------------------------------
	コントロールコード入力
----------------------------------------
*/

int quote(int f, int n)
{
	int c;

	c = tgetc();
	if (c == '\n') {
		int status;

		do {
			status = lnewline();
		} while (status == TRUE && --n);
		return status;
	}
	return linsert(n, c);
}

/*
----------------------------------------
	tab サイズの設定
----------------------------------------
*/

int tab(int f, int n)
{
	int stabsize = curbp->b_stabs;

	if (n == 0 || n > 1) {
		curbp->b_stabs = n;
		return TRUE;
	}
	return (stabsize)
	    ? linsert(stabsize - (getccol(FALSE) % stabsize), ' ') : linsert(1, TAB);
}

/*
----------------------------------------
	detab-line
----------------------------------------
*/

int detabl(int f, int n)
{
	return detab(n);
}

/*
----------------------------------------
	detab-region
----------------------------------------
*/

int detabr(int f, int n)
{
	n = reglines(f, n);
	return detab(n);
}

/*
========================================
	tab > space 変換本体
========================================
*/

static int detab(int n)
{
	int inc;
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;

	for (inc = (n > 0) ? 1 : -1; n; n -= inc) {
		for (wp->w_doto = 0; wp->w_doto < llength(wp->w_dotp); forwchar(FALSE, 1)) {
			if (lgetc(wp->w_dotp, wp->w_doto) == TAB) {
				ldelete(1, FALSE);
				insspace(TRUE, bp->b_tabs - (wp->w_doto % bp->b_tabs));
			}
		}
		forwline(TRUE, inc);
	}

	wp->w_doto = 0;
	thisflag &= ~CFCPCN;
	lchange(WFEDIT);

	return TRUE;
}

/*
----------------------------------------
	entab-line
----------------------------------------
*/

int entabl(int f, int n)
{
	return entab(n);
}

/*
----------------------------------------
	entab-region
----------------------------------------
*/

int entabr(int f, int n)
{
	n = reglines(f, n);
	return entab(n);
}

/*
========================================
	space > tab 変換本体
========================================
*/

static int entab(int n)
{
	int inc;
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;

	for (inc = (n > 0) ? 1 : -1; n; n -= inc) {
		int ccol = 0, nchar = 1;
		int fspace = -1;

		wp->w_doto = 0;

		while (wp->w_doto < llength(wp->w_dotp)) {
			int ch;

			if (fspace >= 0 && nextab(fspace, bp->b_tabs) <= ccol) {
				if (nchar > 1) {
					backdel(FALSE, nchar);
					linsert(1, TAB);
				}
				fspace = -1;
				nchar = 1;
			}
			ch = lgetc(wp->w_dotp, wp->w_doto);
			if (iskanji(ch))
				ch = ((ch << 8) + lgetc(wp->w_dotp, wp->w_doto + 1));

			switch (ch) {
			case ' ':
				if (fspace == -1) {
					fspace = ccol;
					nchar = 1;
				} else
					nchar++;
				ccol++;
				break;
			case TAB:
				if (fspace == -1) {
					fspace = ccol;
					nchar = 1;
				} else
					nchar++;
				ccol = nextab(ccol, bp->b_tabs);
				break;
			case 0x8140:
				{
					char *p;

					p = wp->w_dotp->l_text + wp->w_doto;
					*p++ = ' ';
					*p = ' ';
				}
				if (fspace == -1) {
					fspace = ccol;
					nchar = 1;
				} else
					nchar++;
				ccol++;
				break;
			default:
				ccol++;
				if (ch >= 0x100)
					ccol++;
				fspace = -1;
				nchar = 1;
				break;
			}
			forwchar(FALSE, 1);
		}
		if (fspace >= 0 && nextab(fspace, bp->b_tabs) <= ccol && nchar > 1) {
			backdel(FALSE, nchar);
			linsert(1, TAB);
		}
		forwline(TRUE, inc);
	}

	wp->w_doto = 0;
	thisflag &= ~CFCPCN;
	lchange(WFEDIT);

	return TRUE;
}

/*
----------------------------------------
	trim-line
----------------------------------------
*/

int triml(int f, int n)
{
	return trim(n);
}

/*
----------------------------------------
	trim-region
----------------------------------------
*/

int trimr(int f, int n)
{
	n = reglines(f, n);
	return trim(n);
}

/*
========================================
	余分の空白を削除本体
========================================
*/

int trim(int n)
{
	int inc;

	for (inc = (n > 0) ? 1 : -1; n; n -= inc) {
		int offset, length;
		LINE *lp;

		lp = curwp->w_dotp;
		offset = curwp->w_doto;
		length = lp->l_used;

		while (length > offset) {
			int ch;

			ch = lgetc(lp, length - 1);
			if (ch == 0x40 && lgetc(lp, length - 2) == 0x81)
				length -= 2;
			else if (ch == ' ' || ch == TAB)
				length--;
			else
				break;
		}
		lp->l_used = length;
		forwline(TRUE, inc);
	}

	lchange(WFEDIT);
	thisflag &= ~CFCPCN;

	return TRUE;
}

/*
----------------------------------------
	行を開ける
----------------------------------------
*/

int openline(int f, int n)
{
	int i, status;

	i = n;
	do {
		status = lnewline();
	} while (status == TRUE && --i);
	if (status == TRUE)
		status = backchar(f, n);
	return status;
}

/*
----------------------------------------
	新たな行
----------------------------------------
*/

int newline(int f, int n)
{
	if (check_wrap()) {
		char tmp[NBUFN];

		exechook(strcat(strcpy(tmp, "2 "), wraphook));
	}

	{
		int status;

		while (n--) {
			status = lnewline();
			if (status != TRUE)
				return status;
		}
	}

	return TRUE;
}

/*
========================================
	行挿入、インデント
========================================
*/

int cinsert(void)
{
	int offset;
	char ichar[NSTRING];
	LINE *lp = curwp->w_dotp;

	offset = curwp->w_doto;
	if (offset == 0 && llength(lp) > 0)
		offset--;

	while (offset > 0) {
		int ch;

		ch = lgetc(lp, offset - 1);
		if (ch == 0x40) {
			if (offset > 1 && lgetc(lp, offset - 2) == 0x81) {
				backdel(FALSE, 1);
				offset -= 2;
			} else
				break;
		} else if (ch == ' ' || ch == TAB) {
			backdel(FALSE, 1);
			offset--;
		} else
			break;
	}

	{
		int bracef = 0;

		{
			int i, check = 0;

			for (i = 0; i < offset; i++) {
				int ch;

				ch = lgetc(lp, i);
				if (iskanji(ch))
					i++;
				else if (ch == '}' && check)
					bracef--;
				else if (ch == '{') {
					bracef++;
					check = 1;
				}
			}
		}

		if (offset >= 0) {
			int i;
			char *p, *q;

			if (offset == 0)
				lp = lback(lp);
			while (llength(lp) == 0 && lp != curbp->b_linep)
				lp = lback(lp);

			for (i = 0, p = lp->l_text, q = ichar;; i++) {
				if (i >= llength(lp) || i >= NSTRING - 1)
					break;
				if (*p == 0x81 && p[1] == 0x40) {
					*q++ = *p++;
					*q++ = *p++;
					i++;
				} else if (*p == ' ' || *p == TAB)
					*q++ = *p++;
				else
					break;
			}
			*q++ = 0;

			if (lnewline() == FALSE)
				return (FALSE);
			linstr(ichar);
		} else if (lnewline() == FALSE)
			return FALSE;
		if (bracef > 0)
			c_instab(FALSE, 1);
	}

	if (curwp->w_doto) {
		LINE *lp = curwp->w_dotp;
		char *p;
		int spclen;
		int i;
		int len;

		p = lp->l_text + curwp->w_doto;
		len = llength(lp);
		spclen = 0;
		for(i = curwp->w_doto; i < len; i++) {
			if (*p == TAB || *p == ' ' || (*p == 0x81 && p[1] == 0x40))
				spclen++;
			else	break;
			p++;
		}
		if (spclen)
			ldelete(spclen, FALSE);
	}

	return TRUE;
}

/*
========================================
	'}' 挿入
========================================
*/

int insbrace(int n, int c)
{
	WINDOW *wp = curwp;

	if (wp->w_doto != 0) {
		int i;

		for (i = wp->w_doto - 1; i >= 0; --i) {
			int ch;

			ch = lgetc(wp->w_dotp, i);
			if (ch == 0x40 && lgetc(wp->w_dotp, i - 1) == 0x81)
				i--;
			else if (ch != ' ' && ch != TAB)
				return linsert(n, c);
		}
	}
	{
		int oldoff = wp->w_doto;
		LINE *oldlp = wp->w_dotp;

		{
			int oc;

			switch (c) {
			case '}':
				oc = '{';
				break;
			case ']':
				oc = '[';
				break;
			case ')':
				oc = '(';
				break;
			default:
				return FALSE;
			}

			{
				int count;

				oldlp = wp->w_dotp;
				oldoff = wp->w_doto;
				count = 1;

				while (count > 0) {
					int ch;

					if (backchar(FALSE, 1) == FALSE)
						break;
					ch = lgetc2(wp->w_dotp, wp->w_doto);
					if (ch == c)
						count++;
					else if (ch == oc)
						count--;
				}
				if (count != 0) {
					wp->w_dotp = oldlp;
					wp->w_doto = oldoff;
					return linsert(n, c);
				}
			}
		}

		{
			int target;
			int ch;

			wp->w_doto = 0;
			while (1) {
				ch = lgetc(wp->w_dotp, wp->w_doto);
				if (ch != ' ' && ch != TAB
				    && !(ch == 0x81 && lgetc(wp->w_dotp, wp->w_doto + 1) == 0x40))
					break;
				forwchar(FALSE, 1);
			}

			target = getccol(FALSE);
			wp->w_dotp = oldlp;
			wp->w_doto = oldoff;
			while (target != getccol(FALSE)) {
				if (target < getccol(FALSE)) {
					while (getccol(FALSE) > target)
						backdel(FALSE, 1);
				} else {
					while (target - getccol(FALSE) >= curbp->b_tabs)
						linsert(1, TAB);
					linsert(target - getccol(FALSE), ' ');
				}
			}
		}
	}

	return linsert(n, c);
}

/*
========================================
	'#' 挿入
========================================
*/

int inspound(void)
{
	int i;
	WINDOW *wp = curwp;

	if (wp->w_doto == 0)
		return linsert(1, '#');

	for (i = wp->w_doto - 1; i >= 0; --i) {
		int ch;

		ch = lgetc(wp->w_dotp, i);
		if (ch == 0x40 && lgetc(wp->w_dotp, i - 1) == 0x81)
			i--;
		else if (ch != ' ' && ch != TAB)
			return linsert(1, '#');
	}

	while (getccol(FALSE) >= 1)
		backdel(FALSE, 1);

	return linsert(1, '#');
}

/*
----------------------------------------
	空白行を削除
----------------------------------------
*/

int deblank(int f, int n)
{
	int nld;
	LINE *lp1, *lp2;

	lp1 = curwp->w_dotp;
	while (llength(lp1) == 0 && (lp2 = lback(lp1), lp2 != curbp->b_linep))
		lp1 = lp2;

	lp2 = lp1;
	nld = 0;
	while ((lp2 = lforw(lp2), lp2 != curbp->b_linep) && llength(lp2) == 0)
		nld++;

	if (nld == 0)
		return TRUE;

	curwp->w_dotp = lforw(lp1);
	curwp->w_doto = 0;

	return ldelete(nld, FALSE);
}

/*
----------------------------------------
	インデント
----------------------------------------
*/

int indent(int f, int n)
{
	while (n--) {
		int i, nicol = 0;

		for (i = 0; i < llength(curwp->w_dotp); i++) {
			int ch;

			ch = lgetc(curwp->w_dotp, i);
			if (ch == 0x81 && lgetc(curwp->w_dotp, i + 1) == 0x40) {
				nicol += 2;
				i++;
			} else if (ch == TAB)
				nicol += -(nicol % curbp->b_tabs) + curbp->b_tabs;
			else if (ch == ' ')
				nicol++;
			else
				break;
		}

		if (lnewline() == FALSE
		|| ((i = nicol / curbp->b_tabs, i != 0) && linsert(i, TAB) == FALSE)
		|| ((i = nicol % curbp->b_tabs, i != 0) && linsert(i, ' ') == FALSE))
			return FALSE;
	}

	return TRUE;
}

/*
----------------------------------------
	削除
----------------------------------------
*/

int forwdel(int f, int n)
{
	if (n < 0)
		return backdel(f, -n);

	if (f == TRUE) {
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}
	return ldelete(countbyte(n), f);
}

/*
----------------------------------------
	削除 2
----------------------------------------
*/

int backdel(int f, int n)
{
	if (n < 0)
		return forwdel(f, -n);

	{
		int status;

		if (f == TRUE) {
			if ((lastflag & CFKILL) == 0)
				kdelete();
			thisflag |= CFKILL;
		}
		status = backchar(f, n);
		if (status == TRUE)
			status = ldelete(countbyte(n), f);
		return status;
	}
}

/*
----------------------------------------
	テキスト削除
----------------------------------------
*/

int killtext(int f, int n)
{
	int chunk;
	WINDOW *wp = curwp;

	if (f == FALSE) {
		chunk = llength(wp->w_dotp) - wp->w_doto;
		if (chunk == 0)
			chunk = 1;
	} else if (n == 0) {
		chunk = wp->w_doto;
		wp->w_doto = 0;
	} else if (n > 0) {
		LINE *lp;

		chunk = llength(wp->w_dotp) - wp->w_doto + 1;
		for (lp = lforw(wp->w_dotp); --n; lp = lforw(lp)) {
			if (lp == curbp->b_linep)
				return FALSE;
			chunk += llength(lp) + 1;
		}
	} else if (n < 0) {
		chunk = wp->w_doto;
		if (chunk == 0)
			n--;
		wp->w_doto = 0;
		while (++n) {
			if (backline(FALSE, 1) == FALSE)
				return FALSE;
			chunk += llength(wp->w_dotp) + 1;
		}
	} else {
		mlwrite(KTEX61);
		return FALSE;
	}
	return ldelete(chunk, TRUE);
}

/*
----------------------------------------
	モード設定
----------------------------------------
*/

int setmod(int f, int n)
{
	adjustmode(TRUE, FALSE);
	return TRUE;
}

/*
----------------------------------------
	モード解除
----------------------------------------
*/

int delmode(int f, int n)
{
	adjustmode(FALSE, FALSE);
	return TRUE;
}

/*
----------------------------------------
	グローバルモード設定
----------------------------------------
*/

int setgmode(int f, int n)
{
	adjustmode(TRUE, TRUE);
	return TRUE;
}

/*
----------------------------------------
	グローバルモード解除
----------------------------------------
*/

int delgmode(int f, int n)
{
	adjustmode(FALSE, TRUE);
	return TRUE;
}

/*
========================================
	モード設定本体
========================================
*/

static int adjustmode(int kind, int global)
{
	int i;
	char *scan, prompt[50], cbuf[NPAT];
	static int mapno[NUMMODES] =
	{0, 1, 2, 0, 0, 0, 0, 0, 0, 3, 4};

	strcpy(prompt, global ? KTEX62 : KTEX63);
	strcat(prompt, kind ? KTEX64 : KTEX65);

	scan = complete(prompt, 0, 0, CMP_MODE, NPAT - 1);
	if (scan == 0)
		return FALSE;

	strcpy(cbuf, scan);
	scan = cbuf;

	for (i = 0; i < NCOLORS; i++) {
		if (strcmpi(cbuf, (char *)cname[i]) == 0) {
			(global ? gbcolor : curbp->b_mlcolor) = i;
			curwp->w_flag |= WFCOLR;
			upmode();
			mlerase();
			return TRUE;
		}
	}

	for (i = 0; i < NUMMODES; i++) {
		if (strcmpi(cbuf, (char *)modename[i]) == 0) {
			if (kind == TRUE) {
				if (global) {
					if ((1 << i) & (MDC | MDLATEX | MDDIRED | MDSHELL))
						gmode &= ~(MDC | MDLATEX | MDDIRED | MDSHELL);
					gmode |= (1 << i);
				} else {
					if ((1 << i) & (MDC | MDLATEX | MDDIRED | MDSHELL)) {
						curbp->b_mode &= ~(MDC | MDLATEX | MDDIRED | MDSHELL);
						curbp->b_keymap = mapno[i];
					}
					curbp->b_mode |= (1 << i);
				}
			} else {
				if (global)
					gmode &= ~(1 << i);
				else {
					if ((1 << i) & (MDC | MDLATEX | MDDIRED | MDSHELL))
						curbp->b_keymap = 0;
					curbp->b_mode &= ~(1 << i);
				}
			}
			if ((1 << i) & MDOVER)
				updinsf = TRUE;

			if (global == 0)
				upmode();

			mlerase();

			return TRUE;
		}
	}

	mlwrite(KTEX66);

	return FALSE;
}

/*
----------------------------------------
	メッセージ消去
----------------------------------------
*/

int clrmes(int f, int n)
{
	mlforce("");
	return TRUE;
}

/*
----------------------------------------
	メッセージ表示
----------------------------------------
*/

int writemsg(int f, int n)
{
	int status;
	char buf[NPAT];

	his_disable();
	status = mlreply(KTEX67, buf, NPAT - 1);
	if (status != TRUE)
		return status;

	makelit(buf);
	mlforce(buf);

	return TRUE;
}

/*
----------------------------------------
	対応括弧を得る
----------------------------------------
*/

int getfence(int f, int n)
{
	int oldoff;
	LINE *oldlp;
	WINDOW *wp = curwp;

	oldlp = wp->w_dotp;
	oldoff = wp->w_doto;

	{
		int sdir, ofence;

		{
			char cc;

			cc = lgetc2(oldlp, oldoff);
			switch (cc) {
			case '(':
				ofence = ')';
				sdir = FORWARD;
				break;
			case '{':
				ofence = '}';
				sdir = FORWARD;
				break;
			case '[':
				ofence = ']';
				sdir = FORWARD;
				break;
			case ')':
				ofence = '(';
				sdir = REVERSE;
				break;
			case '}':
				ofence = '{';
				sdir = REVERSE;
				break;
			case ']':
				ofence = '[';
				sdir = REVERSE;
				break;
			default:
				H68beep();
				return FALSE;
			}

			{
				int count = 1;

				count = 1;
				((sdir == REVERSE) ? backchar : forwchar) (FALSE, 1);
				while (count > 0) {
					int ch;

					ch = lgetc2(wp->w_dotp, wp->w_doto);
					if (ch == cc)
						count++;
					if (ch == ofence)
						count--;
					if (((sdir == FORWARD) ? forwchar : backchar)(FALSE, 1) == FALSE)
						break;
				}
				if (count == 0) {
					((sdir == FORWARD) ? backchar : forwchar)(FALSE, 1);
					wp->w_flag |= WFMOVE;
					return TRUE;
				}
			}
		}
	}

	wp->w_dotp = oldlp;
	wp->w_doto = oldoff;
	H68beep();

	return FALSE;
}

/*
========================================
	ブリンクカーソル
========================================
*/

int fmatch(char cc)
{
	int oldoff;
	LINE *oldlp;
	WINDOW *wp = curwp;

	update(FALSE);
	oldlp = wp->w_dotp;
	oldoff = wp->w_doto;

	{
		int opench;

		switch (cc) {
		case ')':
			opench = '(';
			break;
		case '}':
			opench = '{';
			break;
		case ']':
			opench = '[';
			break;
		default:
			opench = '$';
			break;
		}

		{
			int count = 1;
			LINE *toplp = lback(wp->w_linep);

			backchar(FALSE, 1);
			while (count > 0) {
				LINE *lp;
				int ch;

				if (backchar(FALSE, 1) == FALSE || (lp = wp->w_dotp) == toplp)
					break;
				ch = lgetc2(lp, wp->w_doto);
				if (ch == opench)
					count--;
				else if (ch == cc)
					count++;
			}
			if (count == 0) {
				int i;

				for (i = 0; i < term.t_pause; i++)
					update(FALSE);
			}
		}
	}

	wp->w_dotp = oldlp;
	wp->w_doto = oldoff;

	return TRUE;
}

/*
----------------------------------------
	文字列挿入
----------------------------------------
*/

int istring(int f, int n)
{
	int status;
	char tstring[NPAT + 1];

	his_disable();
	status = mltreply(KTEX68, tstring, NPAT, sterm);
	if (status != TRUE)
		return status;
	if (n < 0)
		n = -n;
	while (n-- && (status = linstr(tstring)));
	return status;
}

/*
----------------------------------------
	上書
----------------------------------------
*/

int ovstring(int f, int n)
{
	int status;
	char tstring[NPAT + 1];

	his_disable();
	status = mltreply(KTEX69, tstring, NPAT, sterm);
	if (status != TRUE)
		return status;
	if (n < 0)
		n = -n;
	while (n-- && (status = lover(tstring)));
	return status;
}

/*
----------------------------------------
	キーバッファクリア
----------------------------------------
*/

int kflush(int f, int n)
{
	keydrops();
	return TRUE;
}

/*
----------------------------------------
	dup-line
----------------------------------------
*/

int dupline(int f, int n)
{
	{
		int off;
		char *text;
		LINE *lp;
		WINDOW *wp = curwp;

		lp = wp->w_dotp;
		off = wp->w_doto;

		{
			int ntext;

			ntext = llength(lp);
			text = (char *)malloc(ntext + 1);
			if (text == 0) {
				mlwrite(KTEX168);
				return FALSE;
			}
			memcpy(text, lp->l_text, ntext);
			text[ntext] = 0;
		}

		if (wp->w_dotp != curbp->b_linep)
			wp->w_dotp = lforw(wp->w_dotp);
		wp->w_doto = 0;

		{
			int stat = 1;

			while (n-- > 0) {
				stat = linstr(text);
				if (stat)
					lnewline();
				else {
					mlwrite(KTEX168);
					break;
					lnewline();
					mlwrite(KTEX168);
					break;
				}
			}

			free(text);
			wp->w_dotp = lp;
			wp->w_doto = off;
			return stat;
		}
	}
}

/*
----------------------------------------
	ファイル名の補完
----------------------------------------
*/

int file_complete(int f, int n)
{
	char	save[NSTRING];
	int		stat;

	strcpy(save, ignoreext);
	*ignoreext = 0;
	stat = name_complete(CMP_FILENAME);
	strcpy(ignoreext, save);

	return stat;
}

/*
----------------------------------------
	key-word の補完
----------------------------------------
*/

int keyword_complete (int f, int n)
{
	return name_complete (CMP_KEYWORD);
}

/*
----------------------------------------
	英単語探してバッファに展開するz
----------------------------------------
*/

int lookupword (int f, int n)
{
	char word[NPAT + 1];

	{
		int status;

		his_disable ();
		status = mlreply (KTEX271, word, NPAT - 1);
		if (status != TRUE)
			return status;
	}

	{
		char *ref;

		ref = ej_word (word);
		if (!ref)
			return FALSE;

		bdictp->b_flag &= ~BFCHG;
		if (bclear (bdictp) != TRUE) {
			free (ref);
			return FALSE;
		}
		*bdictp->b_fname = 0;

		{
			char *line, *p;
			char c;

			p = ref;
			while (*p) {
				line = p;
				while (c = *p++) {
					if (c == 0x0d || c == 0x0a) {
						p[-1] = 0;
						break;
					}
				}
				if (addline (bdictp, line) == FALSE) {
					bclear (bdictp);
					free (ref);
					return FALSE;
				}
				while (c = *p++) {
					if (c != 0x0d && c != 0x0a)
						break;
				}
				p--;
			}

			winbob (bdictp);

			free (ref);
			return TRUE;
		}
	}
}
