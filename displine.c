/*
----------------------------------------
	DISPLINE.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/displine.c,v 1.4 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static void mloutw(int);
static void mlputf(int);
static void mlputi(int, int);
static void mlputs(char *);

/*
========================================
	使用変数の定義
========================================
*/

static int ml_col = 0;
static int ml_kanji = 0;
static int ml_modified = 1;
static char *ml_line, *ml_attr, *ml_type;

/*
========================================
	コマンド行初期化
========================================
*/

void mlinit(void)
{
	int maxcol;

	maxcol = term.t_mcol;

	ml_line = (char *) malloc(maxcol + 1);
	if (ml_line == 0)
		meexit(1);
	memset(ml_line, ' ', maxcol);
	ml_line[maxcol] = 0;
	ml_attr = (char *) malloc(maxcol);
	if (ml_attr == 0)
		meexit(1);
	memset(ml_attr, cmdcol, maxcol);
	ml_type = (char *) malloc(maxcol);
	if (ml_type == 0)
		meexit(1);
	memset(ml_type, CT_ANK, maxcol);
}

/*
========================================
	コマンド行アップデート
========================================
*/

void mlupdate(void)
{
	H68curoff();
	if (ml_modified == TRUE) {
		ttcol = -1;
		H_PRINT3(0, term.t_nrow, "", "", ml_line, ml_attr, "", "", diszen);
		ml_modified = FALSE;
	}
	ttcol = -1;
	movecursor(term.t_nrow, (ml_col < term.t_ncol) ? ml_col : term.t_ncol - 1);
	H68curon();
}

/*
========================================
	コマンド行カーソル移動
========================================
*/

void mlmovecursor(int x)
{
	ml_col = x;
	movecursor(term.t_nrow, (ml_col < term.t_ncol) ? ml_col : term.t_ncol - 1);
}

/*
========================================
	コマンド行カーソル移動
========================================
*/

void mlmove(int x)
{
	ml_col = x;
}

/*
========================================
	コマンド行カーソル位置
========================================
*/

int mlgetcursor(void)
{
	return ml_col;
}

/*
========================================
	コマンド行消去
========================================
*/

void mlclear(void)
{
	int maxcol = term.t_ncol;

	ml_kanji = 0;
	mlmove(0);
	if (discmd == FALSE)
		return;

	memset(ml_line, ' ',    maxcol);
	memset(ml_attr, cmdcol, maxcol);
	memset(ml_type, CT_ANK, maxcol);
	ml_modified = TRUE;
}

/*
========================================
	コマンド行消去＆表示
========================================
*/

void mlerase(void)
{
	mlclear();
	if (discmd != FALSE) {
		mlupdate();
		mpresf = FALSE;
	}
}

/*
========================================
	コマンド行カーソル以降消去
========================================
*/

void mldeltoeol(void)
{
	int		ml_color = cmdcol;
	int		col = ml_col;
	int		maxcol = term.t_ncol;

	if (col > term.t_ncol)
		return;
	if (ml_type[col] == CT_KJ2) {
		ml_line[col - 1] = ' ';
		ml_attr[col - 1] = ml_color;
		ml_type[col - 1] = CT_ANK;
	}
	memset(ml_line + col, ' ',      maxcol - col);
	memset(ml_attr + col, ml_color, maxcol - col);
	memset(ml_type + col, CT_ANK,   maxcol - col);
	ml_modified = TRUE;
}

/*
========================================
	コマンド行一文字出力
========================================
*/

void mlout(int c)
{
	int	ml_color = cmdcol;
	int col = ml_col;

	enable_int();

	if (c == '\b') {
		if (col > 0)
			col--;
	} else if (c == '\f') {
		col++;
	} else if (c < 0x20) {
		mlout('^');
		mlout(c + '@');
	} else if (ml_kanji || c > 0xff) {
		if (ml_kanji) {
			c = (ml_kanji << 8) + (c & CHARMASK);
			ml_kanji = 0;
		}
		if (col + 2 > term.t_ncol) {
			if (col + 1 == term.t_ncol) {
				if (ml_type[col] == CT_KJ2) {
					ml_line[col - 1] = ' ';
					ml_attr[col - 1] = ml_color;
					ml_type[col - 1] = CT_ANK;
				}
				ml_line[col] = ' ';
				ml_attr[col] = ml_color;
				ml_type[col] = CT_ANK;
			}
			col += 2;
			goto end_mlout;
		}
		switch (ml_type[col]) {
		case CT_KJ2:
			ml_line[col - 1] = ' ';
			ml_attr[col - 1] = ml_color;
			ml_type[col - 1] = CT_ANK;
		case CT_ANK:
			if (ml_type[col + 1] == CT_KJ1) {
				ml_line[col + 1] = ' ';
				ml_attr[col + 1] = ml_color;
				ml_type[col + 1] = CT_ANK;
				ml_line[col + 2] = ' ';
				ml_attr[col + 2] = ml_color;
				ml_type[col + 2] = CT_ANK;
			}
		}
		if (c == 0x8140)
			ml_color = cmdzencol;
		ml_line[col  ] = (c >> 8) & CHARMASK;
		ml_attr[col  ] = ml_color;
		ml_type[col++] = CT_KJ1;
		ml_line[col  ] = c & CHARMASK;
		ml_attr[col  ] = ml_color;
		ml_type[col++] = CT_KJ2;
	} else {
		if (iskanji(c)) {
			ml_kanji = c;
		} else {
			if (col < term.t_ncol) {
				switch (ml_type[col]) {
				case CT_KJ2:
					ml_line[col - 1] = ' ';
					ml_attr[col - 1] = ml_color;
					ml_type[col - 1] = CT_ANK;
					break;
				case CT_KJ1:
					ml_line[col + 1] = ' ';
					ml_attr[col + 1] = ml_color;
					ml_type[col + 1] = CT_ANK;
				}
				ml_line[col] = c;
				ml_attr[col] = ml_color;
				ml_type[col] = CT_ANK;
			}
			col++;
		}
	}

end_mlout:
	ml_col = col;
	ml_modified = TRUE;
	disable_int();
}

/*
========================================
	コマンド行一文字出力(Buf)
========================================
*/

static void mloutw(int c)
{
	disable_int();

	mlout(c);
	if (c > 0xff) {
		*lastptr++ = c >> 8;
		*lastptr++ = c & CHARMASK;
	} else
		*lastptr++ = c;

	enable_int();
}

/*
========================================
	コマンド行文字列出力
========================================
*/

void mloutstring(char *p)
{
	while (*p)
		mlout(*p++);
}

/*
========================================
	コマンド行出力
========================================
*/

void mlwrite(char *fmt,...)
{
	mlmovecursor(0);
	if (discmd == FALSE)
		return;

	mlclear();
	lastptr = lastmesg;

	{
		int c, waitflag = 0;
		char *ap = (char *) &fmt;

		ADJUST(ap, char *);
		while (c = *fmt++) {
			if (iskanji(c))
				mloutw((c << 8) | (*fmt++ & CHARMASK));
			else if (c != '%')
				mloutw(c);
			else {
				c = *fmt++;
				switch (c) {
				case 'D':
				case 'd':
					mlputi(*(int *) ap, 10);
					ADJUST(ap, int);
					break;
				case 'o':
					mlputi(*(int *) ap, 8);
					ADJUST(ap, int);
					break;
				case 'x':
					mlputi(*(int *) ap, 16);
					ADJUST(ap, int);
					break;
				case 's':
					mlputs(*(char **) ap);
					ADJUST(ap, char *);
					break;
				case 'f':
					mlputf(*(int *) ap);
					ADJUST(ap, int);
					break;
				case 'w':
					waitflag = 1;
					break;
				default:
					mloutw(c);
				}
			}
		}

		if (waitflag) {
			int starttime;

			starttime = ONTIME() - 8640000;
			while ((ONTIME() - starttime) % 8640000 < 200);
		}
	}

	mlupdate();
	mpresf = TRUE;
	*lastptr = 0;
}

/*
========================================
	コマンド行強制出力
========================================
*/

void mlforce(char *s)
{
	int oldcmd;

	oldcmd = discmd;
	discmd = TRUE;
	mlwrite(s);
	discmd = oldcmd;
}

/*
========================================
	コマンド行一行出力
========================================
*/

static void mlputs(char *s)
{
	int c;

	while (c = *s++) {
		if (iskanji(c))
			c = (c << 8) | (*s++ & CHARMASK);
		mloutw(c);
	}
}

/*
========================================
	整数出力
========================================
*/

static void mlputi(int i, int r)
{
	int q;
	static char hexdigits[] = "0123456789ABCDEF";

	if (i < 0) {
		i = -i;
		mloutw('-');
	}
	q = i / r;
	if (q != 0)
		mlputi(q, r);
	mloutw(hexdigits[i % r]);
}

/*
========================================
	実数出力
========================================
*/

static void mlputf(int s)
{
	int i, f;

	i = s / 100;
	f = s % 100;
	mlputi(i, 10);
	mloutw('.');
	mloutw((f / 10) + '0');
	mloutw((f % 10) + '0');
}
