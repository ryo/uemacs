/*
----------------------------------------
	LINEEDIT.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <io.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/lineedit.c,v 1.3 1992/01/05 02:06:36 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int getlocation(char *, int);

/*
========================================
	使用変数の定義
========================================
*/

LEDIT	*curep;

/*
========================================
	カーソルの表示位置を得る
========================================
*/

static int getlocation(char *text, int pos)
{
	char	*p;
	int		i;
	int		c;
	int		loc;

	loc = 0;
	p = text;
	for(i = pos; i > 0; i--) {
		c = *p++;
		if (iskanji(c)) {
			if (c != 0x80 && c < 0xf0)
				loc++;
			loc++;
			p++;
			i--;
		} else if (c == I_NEWLINE) {
			loc += 4;
		} else if (c < 0x20) {
			loc += 2;
		} else
			loc++;
	}
	return loc;
}

/*
========================================
	inp_init
========================================
*/

void inp_init(LEDIT *edit, char *text, int pos, int limit, int home, int insmode, int uf)
{
	{
		int		width;
		int		loc;
		int		base;

		base = 0;
		loc = getlocation(text, pos);
		width = term.t_ncol - home;
		if (width >= 6) {
			if (loc >= width - 2)
				base = -(((loc - 2) / (width - 4)) * (width - 4));
		}
		*edit = (LEDIT){text, pos, limit, insmode, home, base, uf, TRUE};
	}
	curep = edit;

	if (uf)
		inp_update();
}

/*
========================================
	inp_close
========================================
*/

void inp_close(void)
{
	updinsf = TRUE;
}

/*
========================================
	inp_update
========================================
*/

void inp_update(void)
{
	LEDIT	*ep = curep;
	int		len;
	int		pos;
	int		loc;
	int		base;
	char	*text;

	text = ep->text;
	pos  = ep->pos;
	len  = strlen(text);

	{
		int		width;

		base = 0;
		loc = getlocation(text, pos);
		width = term.t_ncol - ep->home;
		if (width >= 6) {
			if (loc >= width - 2)
				base = -(((loc - 2) / (width - 4)) * (width - 4));
		}
		if (ep->base != base) {
			ep->base = base;
			ep->modified = TRUE;
		}
	}
	if (ep->modified == TRUE) {
		char	*p = text;
		int		col = base;
		int		c;
		int		kanji;

		mlmove(ep->home);

		kanji = 0;
		while (c = *p++) {
			if (c == I_NEWLINE) {
				char	*q = "<NL>";
				int		c;

				while (c = *q++)
					inp_update2(c, &col, &kanji);
			} else if (c < ' ') {
				inp_update2('^', &col, &kanji);
				inp_update2(c + '@', &col, &kanji);
			} else
				inp_update2(c, &col, &kanji);
		}
		mldeltoeol();
		mlmovecursor(ep->home + base + loc);
		outupdate();
		ep->modified = FALSE;
	}
	if (ep->chg_insmode) {
		K_INSMOD(ep->insmode ? 0xff : 0x00);
		ep->chg_insmode = FALSE;
		if (*inshook) {
			int save;
			char tmp[NBUFN];

			strcpy(tmp, ep->insmode ? "2 " : "1 ");
			strcat(tmp, inshook);
			save = macbug;
			macbug = FALSE;
			exechook(tmp);
			macbug = save;
		}
	}
	mlmovecursor(ep->home + base + loc);
	H68curon();
}

/*
========================================
	inp_update2
========================================
*/

void inp_update2(int c, int *pos, int *kanji)
{
	int		width;
	int		_pos = *pos;
	int		_kanji = *kanji;

	width = term.t_ncol - curep->home;

	if (_kanji) {
		int		size;

		c = (_kanji << 8) + c;
		size = (_kanji != 0x80 && _kanji < 0xf0) ? 2 : 1;
		*kanji = 0;

		if (_pos == 0 && curep->base < 0 || _pos == -1 && size == 2) {
			outchar('$');
			if (_pos == 0 && size == 2)
				outchar(' ');
		} else if (_pos >= 0 && _pos < width) {
			if (_pos + size <= width)
				outchar(c);
			else
				outchar('$');
		} else if (_pos >= width) {
			if (_pos == width)
				outstring("\b$");
		}
		(*pos) += size;
	} else {
		if (iskanji(c)) {
			*kanji = c;
			return;
		}
		if (_pos == 0 && curep->base < 0)
			outchar('$');
		else if (_pos == width)
			outstring("\b$");
		else if (_pos >= 0 && _pos < width)
			outchar(c);
		(*pos)++;
	}
}

/*
========================================
	inp_inpchar
========================================
*/

void inp_inputchar(int c)
{
	LEDIT	*ep = curep;
	int		len;
	int		pos;
	int		limit;
	char	*text;

	text  = ep->text;
	pos   = ep->pos;
	limit = ep->limit;
	len   = strlen(text);

	if (ep->insmode == FALSE) {
		if (c > 0xff && !iskanji(text[pos]) && len == limit)
			return;
		inp_forwdel();
	}
	{
		int		csize;

		csize = (c > 0xff) ? 2 : 1;
		if (len + csize >= limit)
			return;

		{
			char	*p, *q;
			int		i;

			p = text + len;
			q = p + csize;
			for(i = len - pos; i >= 0; i--)
				*q-- = *p--;
		}

		if (csize == 2)
			text[pos++] = (c >> 8) & CHARMASK;
		text[pos] = c & CHARMASK;

		ep->pos += csize;
		ep->modified = TRUE;
	}
}

/*
========================================
	inp_clear
========================================
*/

void inp_clear(void)
{
	LEDIT	*ep = curep;

	ep->pos = 0;
	ep->text[0] = 0;
	ep->modified = TRUE;
}

/*
========================================
	inp_forwdel
========================================
*/

void inp_forwdel(void)
{
	LEDIT	*ep = curep;
	int		pos;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	if (text[pos] == 0)
		return;

	inp_forwchar();
	inp_backdel();
}

/*
========================================
	inp_backdel
========================================
*/

void inp_backdel(void)
{
	LEDIT	*ep = curep;
	int		pos;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	if (pos == 0)
		return;

	{
		int		csize;

		csize = (nthctype(text, pos) == CT_KJ2) ? 2 : 1;
		{
			char	*p, *q;

			p = text + pos;
			q = p - csize;
			while (*q++ = *p++);
		}
		ep->pos -= csize;
		ep->modified = TRUE;
	}
}

/*
========================================
	inp_forwchar
========================================
*/

void inp_forwchar(void)
{
	LEDIT	*ep = curep;
	int		pos;
	int		c;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	c = text[pos];
	if (c == 0)
		return;

	ep->pos += iskanji(c) ? 2 : 1;
}

/*
========================================
	inp_backchar
========================================
*/

void inp_backchar(void)
{
	LEDIT	*ep = curep;
	int		pos;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	if (pos == 0)
		return;

	ep->pos -= (nthctype(text, pos) == CT_KJ2) ? 2 : 1;
}

void inp_killtoeol(void)
{
	LEDIT	*ep = curep;
	int		pos;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	text[pos] = 0;

	ep->modified = TRUE;
}

void inp_top(void)
{
	curep->pos = 0;
}

void inp_bottom(void)
{
	curep->pos = strlen(curep->text);
}

void inp_topbot(void)
{
	LEDIT	*ep = curep;
	int		pos;
	char	*text;

	text = ep->text;
	pos  = ep->pos;

	ep->pos = pos ? 0 : strlen(text);
}

void inp_toggleover(void)
{
	LEDIT	*ep = curep;

	ep->insmode = ep->insmode ? FALSE : TRUE;
	ep->chg_insmode = TRUE;
}
