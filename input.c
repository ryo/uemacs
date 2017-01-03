/*
----------------------------------------
	INPUT.C: MicroEMACS 3.10
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
#include "fepctrl.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/input.c,v 1.6 1992/01/26 04:53:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static char *gtdfname(char *, int);

/*
========================================
	拡張コードをもとに戻す
========================================
*/

int ectoc(int c)
{
	if (c & CTRL)
		c &= ~(CTRL | 0x40);
	if (c & SPEC)
		c &= CHARMASK;
	return c;
}

/*
========================================
	コマンド名を得る
========================================
*/

char *getname(char *prompt)
{
	char *sp;

	sp = complete(prompt, 0, 0, CMP_COMMAND, NSTRING);
	return sp;
}

/*
========================================
	コマンド名を得る 2
========================================
*/

NBIND *getname2(char *prompt)
{
	char *sp;

	sp = complete(prompt, 0, 0, CMP_COMMAND, NSTRING);
	return sp ? com_in_word_set(sp) : 0;
}

/*
========================================
	バッファ名を得る
========================================
*/

BUFFER *getcbuf(char *prompt, char *defval, int createflag)
{
	char *sp;

	sp = complete(prompt, defval, 0, CMP_BUFFER, NBUFN);
	return sp ? bfind(sp, createflag, 0) : 0;
}

/*
--------------------------------------
	カレントディレクトリ変更
--------------------------------------
*/

int cd(char *path)
{
	if (path[1] == ':')
		CHGDRV(upperc(*path) - 'A');
	return chdir(path);
}

/*
========================================
	Y/N をたずねる
========================================
*/

int mlyesno(char *prompt)
{
	int c;
	char buf[NPAT];

	strcpy(buf, prompt);
	strcat(buf, KTEX162);
	mlwrite(buf);
	fep_off();
	c = getkey();
	fep_on();
	if (c == abortc) {
		ctrlg(FALSE, 0);
		return ABORT;
	}
	return (c == 'y' || c == 'Y') ? TRUE : FALSE;
}

/*
--------------------------------------
	カレントディレクトリ取得
--------------------------------------
*/

char *getwd(char *buff)
{
	buff[0] = CURDRV() + 'A';
	buff[1] = ':';
	buff[2] = slash;

	getcwd(buff + 3, NLINE - 4);

	return cv_bslash_slash(add_slash_if(buff));
}

/*
========================================
	ファイル名を得る
========================================
*/

char *gtfilename(char *prompt)
{
	return gtdfname(prompt, CMP_FILENAME);
}

/*
========================================
	ディレクトリ名を得る
========================================
*/

char *gtdirname(char *prompt)
{
	return gtdfname(prompt, CMP_DIRNAME);
}

/*
========================================
	ファイル、ディレクトリ名を得る
========================================
*/

char *gtdfname(char *prompt, int type)
{
	char *sp;
	char cwd[NLINE];
	static char fwd[NLINE];
	struct NAMESTBUF namebuf;

	if (type == CMP_FILENAME) {
		if (cbufdir && *curbp->b_fname) {
			NAMESTS(curbp->b_fname, &namebuf);
			fwd[0] = namebuf.drive + 'A';
			fwd[1] = ':';
			strcpy(fwd + 2, (char *) namebuf.path);
		} else
			strcpy(fwd, current_dir);
	} else
		getwd(fwd);
	cv_bslash_slash(add_slash_if(fwd));

	getwd(cwd);
	cd(fwd);
	sp = complete(prompt, fwd, 0, type, NFILEN);
	cd(cwd);

	if (type == CMP_DIRNAME)
		return sp;

	return sp ? (strcmp(fwd, sp) ? sp : 0) : 0;
}

/*
========================================
	名前の補完
========================================
*/

char *complete(char *prompt, char *defval, char *str, int type, int maxlen)
{
	LEDIT	edit;
	int		quotef;
	int		inputf;
	int		his_first;
	int		quick;
	int		cpos = 0;
	static char buf[NSTRING];

	if (clexec)
		return (macarg(buf) == TRUE) ? buf : 0;

	if (kbdmode != PLAY) {
		TXcurof();
		TXcuron();
		fep_force_off();
	}

	switch (type) {
	case CMP_BUFFER:
		his_enable(bhiscmpbufp);
		break;
	case CMP_C:
		his_enable(bhiscmpcp);
		break;
	case CMP_COMMAND:
		his_enable(bhiscmpcomp);
		break;
	case CMP_FILENAME:
		his_enable(bhiscmpfnamep);
		break;
	case CMP_GENERAL:
		his_enable(bhiscmpgenp);
		break;
	case CMP_LATEX:
		his_enable(bhiscmplatexp);
		break;
	case CMP_MACRO:
		his_enable(bhiscmpmacp);
		break;
	case CMP_MODE:
		his_enable(bhiscmpmodep);
		break;
	case CMP_VARIABLE:
		his_enable(bhiscmpvarp);
		break;
	case CMP_DIRNAME:
		his_enable(bhiscmpdnamep);
	}

	if (prompt && kbdmode != PLAY) {
		if (type == CMP_COMMAND || type == CMP_MACRO)
			mlwrite("%s", prompt);
		else if (defval)
			mlwrite("%s[%s]: ", prompt, defval);
		else
			mlwrite("%s: ", prompt);
	}
	{
		int		home = mlgetcursor();

		*buf = 0;
		if (str) {
			int c;

			while (c = *str++)
				buf[cpos++] = c;
			buf[cpos] = 0;
			comp_general(buf, &cpos, type);
		}

		inp_init(&edit, buf, cpos, maxlen, home, TRUE, kbdmode != PLAY);
	}

	his_first = TRUE;
	quotef = 0;
	while (1) {
		int c;

		cpos = edit.pos;
		if (kbdmode != PLAY)
			inp_update();

		c = getkey();
		inputf = TRUE;

		if (iskanji(c & CHARMASK)) {
			quotef = 0;
			c = ((c & CHARMASK) << 8) + (getkey() & CHARMASK);
			inp_inputchar(c);
			continue;
		}
		if (quotef == FALSE) {
			if (c == (CTRL | 'M')) {
				if (kbdmode != PLAY) {
					cwin_close();
					his_regist(&edit);
					inp_close();
				}
				return (defval && cpos == 0) ? defval : buf;
			} else if (c == abortc || c == sterm) {
				cwin_close();
				inp_close();
				ctrlg(FALSE, 0);
				return 0;
			} else if (c == quotec1 || c == quotec2) {
				quotef = TRUE;
			} else {
				switch (c) {
				case ' ':
				case CTRL | 'I':
					if (type == CMP_FILENAME || type == CMP_DIRNAME)
						cv_bslash_slash(buf);
					cpos = strlen(buf);
					quick = comp_general(buf, &cpos, type);
					edit.pos = cpos;
					edit.modified = TRUE;
					if (quick == TRUE) {
						if (kbdmode != PLAY) {
							inp_update();
							cwin_close();
							his_regist(&edit);
							inp_close();
						}
						return buf;
					}
					goto over;
				case SPEC | '#':
				case CTRL | 'W':
					his_backward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | '!':
				case CTRL | 'E':
					his_forward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | SHFT | '#':
					his_sc_backward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | SHFT | '!':
					his_sc_forward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case CTRL | ']':
					if (kbdmode != PLAY)
						his_regist(&edit);
					goto over;
				case SPEC | ')':
#ifdef NOEDBIND
				case CTRL | 'F':
#endif
				case CTRL | 'D':
					inp_forwchar();
					goto over;
				case SPEC | '(':
#ifdef NOEDBIND
				case CTRL | 'B':
#endif
				case CTRL | 'S':
					inp_backchar();
					goto over;
				case SPEC | '%':
					inp_forwdel();
					goto over;
				case CTRL | 'H':
					inp_backdel();
					goto over;
#ifndef NOEDBIND
				case CTRL | 'B':
					inp_topbot();
					goto over;
#endif
				case CTRL | 'O':
				case SPEC | '$':
					inp_toggleover();
					goto over;
				case CTRL | 'U':
				case SPEC | '=':
				case SPEC | '+':
					inp_clear();
					goto over;
				case CTRL | 'K':
					inp_killtoeol();
					goto over;
				case SPEC | '-':
				case CTRL | 'N':
					cwin_down();
					goto over;
				case SPEC | '&':
				case CTRL | 'P':
					cwin_up();
					goto over;
				case CTRL | 'C':
				case CTRL | 'V':
					cwin_rollup();
					goto over;
				case CTRL | 'R':
				case CTRL | 'Z':
					cwin_rolldown();
					goto over;
				default:
					break;
over:
				his_first = TRUE;
over2:
				inputf = FALSE;
				}
			}
		}
		if (inputf == TRUE) {
			if ((c & ~CTRL) < 0x100) {
				if (quotef == TRUE) {
					c = getkey();
					if ((c & ~CTRL) >= 0x100)
						c = 0;
					c = ectoc(c);
				} else {
					c = ectoc(c);
					if (c < ' ' && c != '\r' && c != '\n' && c != '\t')
						c = 0;
					if (c == '\r')
						c = I_NEWLINE;
				}
				if (c)
					inp_inputchar(c);
			} else {
				char	keyword[32];

				cmdstr(c, keyword);
				his_shortcut(&edit, keyword);
			}
		}
		quotef = FALSE;
	}
}

/*
========================================
	キー入力
========================================
*/

int tgetc(void)
{
	int c;

	if (kbdmode == PLAY) {
		if (kbdptr < kbdend)
			return ((int) *kbdptr++);
		if (--kbdrep < 1) {
			kbdmode = STOP;
			update(FALSE);
		} else {
			kbdptr = &kbdm[0];
			return ((int) *kbdptr++);
		}
	}
	c = TTgetc();
	lastkey = c;
	if (kbdmode == RECORD) {
		*kbdptr++ = c;
		kbdend = kbdptr;
		if (kbdptr == &kbdm[NKBDM - 1]) {
			kbdmode = STOP;
			H68beep();
		}
	}
	return c;
}

/*
========================================
	キー入力 2
========================================
*/

int getkey(void)
{
	int c;

	if (isearch_last_key) {
		c = isearch_last_key;
		isearch_last_key = 0;
		return c;
	}

	c = tgetc();
	if (c <= 0) {
		int cc;

		cc = tgetc();
		switch (c) {
		case 0:
			if (cc & (MOUS >> 8)) {
				xpos = tgetc();
				ypos = tgetc();
			}
			goto lastgo;
		case -2:
			cc = (cc << 8) | tgetc();
		case -1:
			cc = (cc << 8) | tgetc();
		lastgo:
			cc = (cc << 8) | tgetc();
		}
		c = cc;
	}
	if ((c & CHARMASK) <= 0x1f)
		c = CTRL | (c + '@');

	return c;
}

/*
========================================
	コマンド入力処理
========================================
*/

int getcmd(void)
{
	int c;
	KEYTAB *key;

	c = getkey();
	key = getbind(change_key(c));

	if (key) {
		if (key->k_ptr.fp == meta)
			c = META | getkey();
		else if (key->k_ptr.fp == cex)
			c = (c << 16) | getkey();
		c = change_key(c);
	} else {
		if ((c & ~CHARMASK) && (c & (CTRL | MOUS)) == 0)
			c = change_key(c);
	}

	return c;
}

/*
========================================
	キー入力修正
========================================
*/

int change_key(int c)
{
	if (CAPS_sense()) {
		if ((c & (CTRL | MOUS)) == 0)
			c = isupper(c & CHARMASK) ? lowerc(c) : upperc(c);
	}
	if (ignmetacase) {
		if ((c & (CTRL | MOUS)) == 0)
			c = lowerc(c);
	}

	return c;
}

/*
========================================
	文字列入力
========================================
*/

int getstring(char *prompt, char *buf, int nbuf, int eolchar)
{
	LEDIT	edit;
	int		quotef;
	int		inputf;
	int		his_first;

	if (discmd && kbdmode != PLAY)
		mlwrite(prompt);
	else
		mlmovecursor(0);

	if (kbdmode != PLAY) {
		TXcurof();
		TXcuron();
		fep_force_off();
	}

	*buf = 0;
	inp_init(&edit, buf, 0, nbuf, mlgetcursor(), TRUE, kbdmode != PLAY);

	his_first = TRUE;
	quotef = 0;
	while (1) {
		int c;

		if (kbdmode != PLAY)
			inp_update();

		c = getkey();
		inputf = TRUE;

		if (iskanji(c & CHARMASK)) {
			quotef = 0;
			c = ((c & CHARMASK) << 8) + (getkey() & CHARMASK);
			inp_inputchar(c);
			continue;
		}
		if (quotef == FALSE) {
			if (c == eolchar) {
				if (kbdmode != PLAY) {
					his_regist(&edit);
					inp_close();
					mlerase();
				}
				return *buf ? TRUE : FALSE;
			}
			if (c == abortc || c == sterm) {
				ctrlg(FALSE, 0);
				return ABORT;
			} else if (c == quotec1 || c == quotec2) {
				quotef = TRUE;
			} else {
				switch (c) {
				case SPEC | '#':
				case CTRL | 'W':
					his_backward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | '!':
				case CTRL | 'E':
					his_forward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | SHFT | '#':
					his_sc_backward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case SPEC | SHFT | '!':
					his_sc_forward(&edit, his_first);
					his_first = FALSE;
					goto over2;
				case CTRL | ']':
					if (kbdmode != PLAY)
						his_regist(&edit);
					goto over;
				case SPEC | ')':
				case CTRL | 'D':
					inp_forwchar();
					goto over;
				case SPEC | '(':
				case CTRL | 'S':
					inp_backchar();
					goto over;
				case SPEC | '%':
					inp_forwdel();
					goto over;
				case CTRL | 'H':
					inp_backdel();
					goto over;
				case CTRL | 'B':
					inp_topbot();
					goto over;
				case CTRL | 'O':
				case SPEC | '$':
					inp_toggleover();
					goto over;
				case CTRL | 'U':
				case SPEC | '=':
				case SPEC | '+':
					inp_clear();
					goto over;
				case CTRL | 'K':
					inp_killtoeol();
					goto over;
				default:
					break;
over:
				his_first = TRUE;
over2:
				inputf = FALSE;
				}
			}
		}
		if (inputf == TRUE) {
			if ((c & ~CTRL) < 0x100) {
				if (quotef == TRUE) {
					c = getkey();
					if ((c & ~CTRL) >= 0x100)
						c = 0;
					c = ectoc(c);
				} else {
					c = ectoc(c);
					if (c < ' ' && c != '\r' && c != '\n' && c != '\t')
						c = 0;
					if (c == '\r')
						c = I_NEWLINE;
				}
				if (c)
					inp_inputchar(c);
			} else {
				char	keyword[32];

				cmdstr(c, keyword);
				his_shortcut(&edit, keyword);
			}
		}
		quotef = FALSE;
	}
}

/*
========================================
	文字出力1
========================================
*/

void outchar(int c)
{
	if (disinp && kbdmode != PLAY)
		mlout(c);
}

/*
========================================
	文字列出力1
========================================
*/

void outstring(char *p)
{
	if (disinp && kbdmode != PLAY) {
		while (*p)
			mlout(*p++);
	}
}

/*
========================================
	文字列アップデート1
========================================
*/

void outupdate(void)
{
	if (disinp && kbdmode != PLAY)
		mlupdate();
}

/*
========================================
	文字出力2
========================================
*/

void ochar(int c)
{
	if (discmd)
		mlout(c);
}

/*
========================================
	文字列出力2
========================================
*/

void ostring(char *p)
{
	if (discmd) {
		while (*p)
			mlout(*p++);
	}
}

/*
========================================
	文字列出力2
========================================
*/

void oupdate(void)
{
	if (discmd)
		mlupdate();
}
