/*
----------------------------------------
	ISEARCH.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/isearch.c,v 1.4 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static int (*saved_get_char) (void);
static int eaten_char = -1;
static int cmd_offset, cmd_reexecute = -1;
static int cmd_buff[CMDBUFLEN];
static int roll;

/*
========================================
	使用関数の定義
========================================
*/

static int checknext(int, char *, int);
static int echochar(int, int);
static int get_char(void);
static int isearch(int, int);
static int match_pat(char *);
static int promptpattern(char *);
static int scanmore(char *, int);

/*
----------------------------------------
	逆方向 isearch
----------------------------------------
*/

int risearch(int f, int n)
{
	int curoff;
	LINE *curline;
	WINDOW *wp = curwp;

	curline = wp->w_dotp;
	curoff = wp->w_doto;
	backchar(TRUE, 1);
	if (isearch(f, -n) == FALSE) {
		wp->w_dotp = curline;
		wp->w_doto = curoff;
		wp->w_flag |= WFMOVE;
		update(FALSE);
		mlwrite(KTEX164);
	} else
		mlerase();

	return TRUE;
}

/*
----------------------------------------
	順方向 isearch
----------------------------------------
*/

int fisearch(int f, int n)
{
	int curoff;
	LINE *curline;
	WINDOW *wp = curwp;

	curline = wp->w_dotp;
	curoff = wp->w_doto;
	if (isearch(f, n) == FALSE) {
		wp->w_dotp = curline;
		wp->w_doto = curoff;
		wp->w_flag |= WFMOVE;
		update(FALSE);
		mlwrite(KTEX164);
	} else
		mlerase();

	return TRUE;
}

/*
========================================
	isearch 本体
========================================
*/

int isearch(int f, int n)
{
	WINDOW *wp = curwp;
	LINE *curline;
	int status, col, cpos, expc, kan;
	int c, curoff, init_direction;
	char pat_save[NPAT];

	cmd_reexecute = -1;
	cmd_offset = 0;
	cmd_buff[0] = 0;
	strncpy(pat_save, pat, NPAT);

	curline = wp->w_dotp;
	curoff = wp->w_doto;
	init_direction = n;
	roll = 0;

start_over:

	col = promptpattern(KTEX165);
	cpos = 0;
	status = TRUE;
	expc = get_char();
	c = ectoc(expc);

	if (c == IS_FORWARD1 || c == IS_FORWARD2 || c == IS_REVERSE1 || c == IS_REVERSE2) {
		for (cpos = 0; pat[cpos]; cpos++)
			col = echochar((int) pat[cpos], col);
		if (c == IS_REVERSE1 || c == IS_REVERSE2) {
			n = -1;
			backchar(TRUE, 1);
		} else
			n = 1;
		status = scanmore(pat, n);
		expc = get_char();
		c = ectoc(expc);
	}
	while (1) {
		if (expc == sterm)
			return TRUE;

		kan = 0;
		switch (expc) {
		case CTRL | 'G':
			return FALSE;
		case CTRL | 'R':
			n = -1;
			goto same;
		case CTRL | 'S':
			n = 1;
		  same:
			status = scanmore(pat, n);
			goto next;
		case CTRL | 'Q':
		case CTRL | 'V':
			expc = get_char();
			c = ectoc(expc);
			break;
		case CTRL | 'M':
			c = I_NEWLINE;
			break;
		case CTRL | 'I':
		case CTRL | 'J':
			break;
		case CTRL | 'H':
#if 0
		case SPEC | '(':
		case CTRL | 'B':
		case 0x7f:
#endif
		  cancel_input:
			if (cmd_offset > 0) {
				cmd_offset -= 2;
				if (cmd_offset > 0) {
					c = ectoc(cmd_buff[cmd_offset - 1]);
					if (iskanji(c)) {
						cmd_offset--;
						if (cmd_offset > 0) {
							c = ectoc(cmd_buff[cmd_offset - 1]);
							if (c == IS_QUOTE || c == IS_68QUOTE)
								cmd_offset--;
						}
					} else if (c == IS_QUOTE || c == IS_68QUOTE)
						cmd_offset--;
				}
				cmd_buff[cmd_offset] = 0;
				wp->w_dotp = curline;
				wp->w_doto = curoff;
				wp->w_flag |= WFMOVE;
				n = init_direction;
				roll = 0;
				strncpy(pat, pat_save, NPAT);
				cmd_reexecute = 0;
			}
			goto start_over;
#if 0
		case SPEC | ')':
		case CTRL | 'F':
			if (n > 0) {
				LINE *lp;
				int cc;

				lp = wp->w_dotp;
				if (llength(lp) != wp->w_doto) {
					c = lgetc(lp, wp->w_doto);
					if (iskanji(c)) {
						if (cpos + 2 >= NPAT - 1) {
							mlwrite(KTEX166);
							if (cmd_reexecute == -1)
								H68beep();
							goto next;
						}
						pat[cpos++] = c;
						col = echochar(c, col);
						cc = lgetc(lp, wp->w_doto + 1);
						pat[cpos++] = cc;
						col = echochar(cc, col);
						pat[cpos] = 0;
						c = (c << 8) + cc;
						goto do_scan;
					}
				} else {
					if (lp == curbp->b_linep) {
						if (cmd_reexecute == -1)
							H68beep();
						goto next;
					}
					c = I_NEWLINE;
				}
				break;
			} else {
				H68beep();
				goto next;
			}
#endif
		default:
			if (expc & 0xff00) {
				isearch_last_key = expc;
				return TRUE;
			}
			kan = iskanji(c);
			break;
		}

		if (cpos + (kan ? 2 : 1) >= NPAT - 1) {
			mlwrite(KTEX166);
			if (cmd_reexecute == -1)
				H68beep();
			goto next;
		}
		pat[cpos++] = c;
		pat[cpos] = 0;
		col = echochar(c, col);
		if (kan) {
			int cc;

			cc = ectoc(get_char());
			pat[cpos++] = cc;
			pat[cpos] = 0;
			col = echochar(cc, col);
			c = (c << 8) + cc;
		}

#if 0
	do_scan:
#endif

   		if (!status) {
			if (cmd_reexecute == -1)
				H68beep();
		} else if (!(status = checknext(c, pat, n)))
			status = scanmore(pat, n);

	next:
		expc = get_char();
		c = ectoc(expc);
	}
}

/*
========================================
	次の一文字チェック
========================================
*/

static int checknext(int chr, char *patrn, int dir)
{
	int curoff;
	LINE *curline;
	WINDOW *wp = curwp;

	curline = wp->w_dotp;
	curoff = wp->w_doto;

	if (dir > 0) {
		int buffchar, status;

		if (curoff == llength(curline)) {
			curline = lforw(curline);
			if (curline == curbp->b_linep)
				return FALSE;
			curoff = 0;
			buffchar = I_NEWLINE;
		} else
			buffchar = lgetc(curline, curoff++);

		if (chr & 0xff00) {
			if (curoff == llength(curline))
				return FALSE;
			buffchar = (buffchar << 8) + lgetc(curline, curoff++);
			status = buffchar == chr;
		} else
			status = eq(buffchar, chr);

		if (status) {
			wp->w_dotp = curline;
			wp->w_doto = curoff;
			wp->w_flag |= WFMOVE;
		}
		return status;
	}
	return match_pat(patrn);
}

/*
========================================
	次のマッチングを探す
========================================
*/

static int scanmore(char *patrn, int dir)
{
	int status;

	setjtable(patrn);
	status = (dir < 0)
	    ? scanner(tap, REVERSE, PTBEG) : scanner(patrn, FORWARD, PTEND);
	if (status == FALSE) {
		if (roll != dir) {
			if (cmd_reexecute == -1)
				H68beep();
			roll = dir;
		} else {
			int off;
			LINE *lp;
			WINDOW *wp = curwp;

			lp = wp->w_dotp;
			off = wp->w_doto;

			if (dir < 0) {
				wp->w_dotp = curbp->b_linep;
				wp->w_doto = llength(wp->w_dotp);
				status = scanner(tap, REVERSE, PTBEG);
			} else {
				wp->w_dotp = lforw(curbp->b_linep);
				wp->w_doto = 0;
				status = scanner(patrn, FORWARD, PTEND);
			}
			if (status == FALSE) {
				wp->w_dotp = lp;
				wp->w_doto = off;
			}
		}
	} else	roll = 0;
	return status;
}

/*
========================================
	リバース用マッチングサブ
========================================
*/

static int match_pat(char *patrn)
{
	LINE *curline;
	int i, len, curoff;

	curline = curwp->w_dotp;
	curoff = curwp->w_doto;

	len = strlen(patrn);
	for (i = 0; i < len; i++) {
		int buffchar;

		if (curoff == llength(curline)) {
			curline = lforw(curline);
			curoff = 0;
			if (curline == curbp->b_linep)
				return FALSE;
			buffchar = I_NEWLINE;
		} else
			buffchar = lgetc(curline, curoff++);

		if (iskanji(buffchar) && curoff != llength(curline)) {
			if (buffchar != patrn[i++])
				return FALSE;
			if (lgetc(curline, curoff++) != patrn[i])
				return FALSE;
		} else {
			if (!eq(buffchar, patrn[i]))
				return FALSE;
		}
	}

	return TRUE;
}

/*
========================================
	文字列表示
========================================
*/

static int promptpattern(char *prompt)
{
	char tpat[NPAT + 20], epat[NPAT + 20];

	expandp(pat, epat, NPAT >> 1);
	sprintf(tpat, "%s [%s]%s: "
		,prompt
		,epat
		,termstr());
	if (!clexec)
		mlwrite(tpat);

	return strlen(tpat);
}

/*
========================================
	文字表示
========================================
*/

static int echochar(int c, int col)
{
	static int lastchar, lastcharflag = 0;

	if (lastcharflag) {
		lastcharflag = 0;
		c = ((lastchar << 8) | (c & CHARMASK)) & WORDMASK;
		outchar(c);
		outupdate();
		col += 2;
		return col;
	} else if (iskanji(c)) {
		lastcharflag = 1;
		lastchar = c;
		return col;
	}
	if (c < ' ' || c == 0x7f) {
		switch (c) {
		case I_NEWLINE:
			outstring("<NL>");
			col += 3;
			break;
		case TAB:
			outstring("<TAB>");
			col += 4;
			break;
		case 0x7f:
			outstring("^?");
			col++;
			break;
		default:
			outchar('^');
			outchar(c + 0x40);
			col++;
		}
	} else
		outchar(c);

	outupdate();

	return ++col;
}

/*
========================================
	isearch 用 get_char
========================================
*/

static int get_char(void)
{
	int c;

	if (cmd_reexecute >= 0) {
		c = cmd_buff[cmd_reexecute++];
		if (c)
			return c;
	}
	cmd_reexecute = -1;
	update(FALSE);
	if (cmd_offset >= CMDBUFLEN - 1) {
		mlwrite(KTEX167);
		return sterm;
	}
	c = getkey();
	cmd_buff[cmd_offset++] = c;
	cmd_buff[cmd_offset] = 0;
	return c;
}

/*
========================================
	get_char を元に戻す
========================================
*/

int uneat(void)
{
	int c;

	term.t_getchar = saved_get_char;
	c = eaten_char;
	eaten_char = -1;
	return c;
}

/*
========================================
	get_char を奪う
========================================
*/

int reeat(int c)
{
	if (eaten_char == -1) {
		eaten_char = c;
		saved_get_char = term.t_getchar;
		term.t_getchar = uneat;
		return TRUE;
	}
	return FALSE;
}
