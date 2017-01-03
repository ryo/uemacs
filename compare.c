/*
----------------------------------------
	COMPARE.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <process.h>
#include <errno.h>

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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/compare.c,v 1.2 1991/09/01 02:47:26 SALT Exp $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static void _nextwind(void);
static void _prevwind(void);
static int getcurchar(WINDOW *);

/*
----------------------------------------
	次のウィンドウ
----------------------------------------
*/

static void _nextwind(void)
{
	WINDOW *wp;

	wp = curwp->w_wndp ? : wheadp;
	curwp = wp;
	curbp = wp->w_bufp;
}

/*
----------------------------------------
	前のウィンドウ
----------------------------------------
*/

static void _prevwind(void)
{
	WINDOW *wp1, *wp2;

	wp1 = wheadp;
	wp2 = curwp;
	if (wp1 == wp2)
		wp2 = 0;

	while (wp1->w_wndp != wp2)
		wp1 = wp1->w_wndp;
	curwp = wp1;
	curbp = wp1->w_bufp;
}

/*
========================================
	１文字取り出し
========================================
*/

static inline int getcurchar(WINDOW *wp)
{
	if (wp->w_doto == llength(wp->w_dotp))
		return I_NEWLINE;
	else {
		int ch;

		ch = lgetc(wp->w_dotp, wp->w_doto);
		if (iskanji(ch))
			ch = (ch << 8) | lgetc(wp->w_dotp, wp->w_doto + 1);

		return ch;
	}
}

/*
----------------------------------------
	compare-line	SHUNA
----------------------------------------
*/

int comparel(int f, int n)
{
	WINDOW *wp1, *wp2;

	wp1 = curwp;
	_nextwind();
	wp2 = curwp;
	_prevwind();
	if (wp1 == wp2) {
		mlwrite(KTEX207);
		return FALSE;
	}
	wp1->w_doto = wp2->w_doto = 0;

	while (n > 0) {
		int ch1, ch2;

		ch1 = getcurchar(wp1);
		ch2 = getcurchar(wp2);

		if (ch1 != ch2) {
			update(TRUE);
			mlwrite(KTEX262);
			H68beep();
			return FALSE;
		}

		if (ch1 == I_NEWLINE)
			n--;

		if (forwchar(FALSE, 1) == FALSE) {
			int stat;

			_nextwind();
			stat = forwchar(FALSE, 1);
			if (stat != FALSE)
				backchar(FALSE, 1);
			_prevwind();
			if (stat == FALSE)
				return TRUE;
			return n ? FALSE : TRUE;
		}
		if (ch1 == I_NEWLINE)
			update(TRUE);

		_nextwind();
		if (forwchar(FALSE, 1) == FALSE)
			return n ? FALSE : TRUE;
		if (ch2 == I_NEWLINE)
			update(TRUE);
		_prevwind();
	}
	return TRUE;
}

/*
========================================
	compare-buffer	SHUNA
========================================
*/

int compareb(int f, int n)
{
	WINDOW *wp1, *wp2;

	wp1 = curwp;
	_nextwind();
	wp2 = curwp;
	_prevwind();
	if (wp1 == wp2) {
		mlwrite(KTEX207);
		return FALSE;
	}
	if (wp1->w_bufp == wp2->w_bufp) {
		mlwrite(KTEX263);
		return FALSE;
	}
	wp1->w_dotp = lforw(wp1->w_bufp->b_linep);
	wp2->w_dotp = lforw(wp2->w_bufp->b_linep);
	wp1->w_doto = wp2->w_doto = 0;

	while (1) {
		int ch1, ch2;

		ch1 = getcurchar(wp1);
		ch2 = getcurchar(wp2);

		if (ch1 != ch2) {
			update(TRUE);
			mlwrite(KTEX262);
			H68beep();
			return FALSE;
		}
		if (forwchar(FALSE, 1) == FALSE) {
			int stat;

			_nextwind();
			stat = forwchar(FALSE, 1);
			if (stat != FALSE)
				backchar(FALSE, 1);
			_prevwind();
			if (stat == FALSE) {
				update(TRUE);
				mlwrite(KTEX264);
				return TRUE;
			}
			return FALSE;
		}
		_nextwind();

		if (forwchar(FALSE, 1) == FALSE) {
			update(TRUE);
			mlwrite(KTEX262);
			H68beep();
			return FALSE;
		}
		_prevwind();
	}
}
