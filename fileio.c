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

static int handle, breakflag, eofflag;
static int handle_mode, buf_count;
static char *buf_ptr, turbobuf[TURBOSIZE + 32];

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
	buf_count = 0;
	buf_ptr = turbobuf;

	handle = OPEN(fn, 0x000);
	if (handle < 0)
		return FIOFNF;
	eofflag = FALSE;
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
	buf_count = 0;
	buf_ptr = turbobuf;

	handle_mode = WRITE_MODE;
	handle = CREATE(fn, attr);
	if (handle < 0) {
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
	if (handle_mode == WRITE_MODE && buf_count > 0) {
		if (WRITE(handle, turbobuf, buf_count) != buf_count) {
			CLOSE(handle);
			mlwrite(KTEX157);
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
	ファイル一行出力
========================================
*/

int ffputline(char *buf, int nbuf)
{
	int i, count;
	char *src, *dst;

	count = buf_count;
	src = buf;
	dst = buf_ptr;

	for (i = 0; i < nbuf; i++) {
		*dst++ = *src++;
		if (++count >= TURBOSIZE) {
			if (WRITE(handle, turbobuf, count) != count) {
				mlwrite(KTEX157);
				return FIOERR;
			}
			count = 0;
			dst = turbobuf;
		}
	}

	if (nbuf >= 0) {
		if (!unix_newline) {
			*dst++ = '\r';
			count++;
		}
		*dst++ = '\n';
		count++;
	} else {
		*dst++ = '\x1a';
		count++;
	}
	if (count >= TURBOSIZE) {
		if (WRITE(handle, turbobuf, count) != count) {
			mlwrite(KTEX157);
			return FIOERR;
		}
		count = 0;
		dst = turbobuf;
	}
	buf_count = count;
	buf_ptr = dst;
	return FIOSUC;
}

/*
========================================
	ファイル一行入力
========================================
*/

int ffgetline(int *nbytes, int *truenbytes)
{
	int	n;
	int i, c, count;
	char *src, *dst;

	if (eofflag)
		return FIOEOF;

	if (fline == 0 || flen != NSTRING) {
		if (fline)
			free(fline);

		flen = NSTRING;
		fline = (char *) malloc(flen);
		if (fline == 0)
			return FIOMEM;
	}
	count = buf_count;
	src = buf_ptr;
	dst = fline;
	i = n = 0;

	while (1) {
		if (--count < 0) {
			src = turbobuf;
			count = READ(handle, src, TURBOSIZE);
			if (count <= 0) {
				if (count == 0) {
					c = EOF;
					break;
				} else {
					mlwrite(KTEX158);
					return FIOERR;
				}
			}
			count--;
		}
		c = *src++;
		n++;
		if (c == '\n' || (c == 0x1a && i == 0))
			break;
		*dst++ = c;

		if (++i == flen) {
			char *tmp;

			tmp = realloc(fline, flen + NSTRING);
			if (tmp == NULL)
				return FIOMEM;

			flen += NSTRING;
			fline = tmp;
			dst = fline + i;
		}
	}

	if (c == EOF) {
		if (i == 0) {
			buf_count = count;
			buf_ptr = src;
			*nbytes = 0;
			return FIOEOF;
		}
		eofflag = TRUE;
	}
	if (dst[-1] == '\r') {
		dst--;
		i--;
	}
	*dst = 0;

	buf_count = count;
	buf_ptr = src;
	*nbytes = i;
	*truenbytes = n;

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
