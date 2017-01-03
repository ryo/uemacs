/*
----------------------------------------
	FILEIO.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/fileio.c,v 1.5 1991/10/26 01:37:50 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static int handle, breakflag;
static int handle_mode, buf_left;
static char *buf_ptr, turbobuf[TURBOSIZE];

/*
========================================
	使用関数の定義
========================================
*/

static int write_turbobuf (void);
static int read_turbobuf (void);
static int search_lf (char *text, int size);

/*
========================================
	読み込みオープン
========================================
*/

int ffropen(char *fn)
{
	breakflag = BREAKCK(-1);
	BREAKCK(0);
	handle_mode = READ_MODE;
	buf_ptr = turbobuf;
	buf_left = 0;

	handle = OPEN(fn, 0x000);
	if (handle < 0) {
		BREAKCK(breakflag);
		return FIOFNF;
	}
	return FIOSUC;
}

/*
========================================
	書き込みオープン
========================================
*/

int ffwopen(char *fn, int attr)
{
	breakflag = BREAKCK(-1);
	BREAKCK(0);
	handle_mode = READ_MODE;
	buf_ptr = turbobuf;
	buf_left = sizeof (turbobuf);

	handle_mode = WRITE_MODE;
	handle = CREATE(fn, attr);
	if (handle < 0) {
		BREAKCK(breakflag);
		mlwrite(KTEX155);
		return FIOERR;
	}
	return FIOSUC;
}

/*
========================================
	ファイルクローズ
========================================
*/

int ffclose(void)
{
	BREAKCK(breakflag);
	if (handle_mode == WRITE_MODE && buf_left) {
		int result;

		result = write_turbobuf ();
		if (result != FIOSUC) {
			CLOSE(handle);
			return FIOERR;
		}
	}
	if (CLOSE(handle) != 0) {
		mlwrite(KTEX156);
		return FIOERR;
	}
	return FIOSUC;
}

/*
========================================
	バッファをファイルに書き出す
========================================
*/

static int write_turbobuf (void)
{
	int wsize, actual;

	wsize = buf_ptr - turbobuf;
	actual = WRITE (handle, turbobuf, wsize);

	if (actual != wsize) {
		mlwrite (KTEX157);
		return FIOERR;
	}

	return FIOSUC;
}

/*
========================================
	ファイルからバッファに読み込む
========================================
*/

static int read_turbobuf (void)
{
  int rsize;

  rsize = READ (handle, turbobuf, TURBOSIZE);
  if (rsize < 0)
    rsize = 0;

  return rsize;
}

/*
========================================
	LF の位置を探す
========================================
*/

static int search_lf (char *text, int size)
{
  char *p;

  for (p = text; size > 0; size--) {
    if (*p++ == 0x0a)
      break;
  }

  return p - text;
}

/*
========================================
	ファイル一行出力
========================================
*/

int ffwriteline (char *src, int lsize)
{
	int left, total, n;
	char *dst;

	dst = buf_ptr;
	left = buf_left;

	total = 0;
	while (lsize > total) {
		n = lsize - total;
		if (n > left)
			n = left;
		memcpy (dst, src + total, n);
		dst += n;
		left -= n;
		total += n;

		if (left == 0) {
			int result;

			buf_ptr = dst;
			result = write_turbobuf ();
			if (result != FIOSUC)
				return result;
			dst = turbobuf;
			left = sizeof (turbobuf);
		}
	}

	if (left < 16) {
		int result;

		buf_ptr = dst;
		result = write_turbobuf ();
		if (result != FIOSUC)
			return result;
		dst = turbobuf;
		left = sizeof (turbobuf);
	}

	if (lsize >= 0) {
		if (!unix_newline) {
			*dst++ = 0x0d;
			left--;
		}
		*dst++ = 0x0a;
	} else
		*dst++ = 0x1a;
	left--;

	buf_ptr = dst;
	buf_left = left;

	return FIOSUC;
}

/*
========================================
	ファイル一行入力
========================================
*/

int ffreadline (LINE **line, int *nbytes)
{
	LINE *lp;
	int left, lsize;
	char *src, *text;

	src = buf_ptr;
	left = buf_left;
	if (left == 0) {
		src = turbobuf;
		left = read_turbobuf ();
		if (left == 0)
			return FIOEOF;
	}

	*nbytes = lsize = search_lf (src, left);
	lp = lalloc (lsize);
	if (lp == NULL)
		return FIOMEM;
	text = lp->l_text;
	memcpy (text, src, lsize);
	src += lsize;
	left -= lsize;

	if (left == 0) {
		src = turbobuf;
		left = read_turbobuf ();
		while (left && text[lsize - 1] != 0x0a) {
			int lsize_add;

			lsize_add = search_lf (src, left);
			lp = lrealloc (lp, lsize + lsize_add);
			if (lp == NULL)
				return FIOMEM;
			text = lp->l_text;
			memcpy (text + lsize, src, lsize_add);
			lsize += lsize_add;
			src += lsize_add;
			left -= lsize_add;

			if (left == 0) {
				src = turbobuf;
				left = read_turbobuf ();
			}
		}
	}

	{
		char c;

		c = text[lsize - 1];
		if (c == 0x1a) {
			free (lp);
			return FIOEOF;
		}
		if (c == 0x0a) {
			if (lsize >= 2) {
				if (text[lsize - 2] == 0x0d)
					lsize--;
			}
			lsize--;
		}
	}

	*line = lp;
	lp->l_used = lsize;

	buf_ptr = src;
	buf_left = left;

	return FIOSUC;
}

/*
========================================
	ファイル一行入力(その２)
========================================
*/

int ffgetline (int *length)
{
	int left, lsize;
	char *src;

	src = buf_ptr;
	left = buf_left;
	if (left == 0) {
		left = read_turbobuf ();
		if (left == 0)
			return FIOEOF;
	}

	lsize = search_lf (src, left);
	fline = (fline) ? realloc (fline, lsize) : malloc (lsize);
	if (fline == NULL)
		return FIOMEM;
	memcpy (fline, src, lsize);
	src += lsize;
	left -= lsize;

	if (left == 0) {
		src = turbobuf;
		left = read_turbobuf ();
		while (left && fline[lsize - 1] != 0x0a) {
			int lsize_add;

			lsize_add = search_lf (src, left);
			fline = realloc (fline, lsize + lsize_add);
			if (fline == NULL)
				return FIOMEM;
			memcpy (fline + lsize, src, lsize_add);
			lsize += lsize_add;
			src += lsize_add;
			left -= lsize_add;

			if (left == 0) {
				src = turbobuf;
				left = read_turbobuf ();
			}
		}
	}

	{
		char c;

		c = fline[lsize - 1];
		if (c == 0x1a)
			return FIOEOF;
		if (c == 0x0a) {
			if (lsize >= 2) {
				if (fline[lsize - 2] == 0x0d)
					lsize--;
			}
			lsize--;
		}
	}
	fline[lsize] = 0;

	*length = lsize;

	buf_ptr = src;
	buf_left = left;

	return FIOSUC;
}

/*
========================================
	ファイルの有無を調べる
========================================
*/

int fexist(char *fname)
{
	int handle;

	handle = OPEN(fname, 0x0000);
	if (handle < 0)
		return FALSE;
	CLOSE(handle);
	return TRUE;
}

/*
========================================
	ファイルのサイズを調べる
========================================
*/

int ffsize(void)
{
	int old_seekptr, file_size;

	old_seekptr = SEEK(handle, 0, 0);
	file_size = SEEK(handle, 0, 2);
	SEEK(handle, old_seekptr, 0);
	return file_size;
}
