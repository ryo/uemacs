/*
----------------------------------------
	WORD.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/word.c,v 1.8 1992/01/05 00:20:52 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static int oldchar, wbound, csize;

#define CASE_LOWER 0
#define CASE_UPPER 1
#define CASE_CAP   2

/*
========================================
	使用関数の定義
========================================
*/

static int backbreakok(int);
static char *findleft(char *, char *);
static void reform(char *, char *, int, int, char *, int, int, char *, int, int);
static int forw_skip(void);
static int back_skip(void);
static int forw_go(void);
static int back_go(void);
static int forw_case(int);
static int case_sub(int, int);
static int nthctype_ptr(char *, char *);

/*
========================================
	漢字コードチェック
========================================
*/

static int nthctype_ptr(char *s, char *p)
{
	char *q = --p;

	while (q >= s && iskanji(*q))
		--q;
	return ((p - q) & 01) && iskanji(*p);
}

/*
----------------------------------------
	禁足処理
----------------------------------------
*/

int wrapword(int f, int n)
{
	int c;
	WINDOW *wp = curwp;

	if (backchar(FALSE, 1) == FALSE)
		return FALSE;

	c = lgetc(wp->w_dotp, wp->w_doto);
	if (iskanji(c)) {
		c = (c << 8) + lgetc(wp->w_dotp, wp->w_doto + 1);
		if (breakmode) {
			int wf, cnt = 1;

			wf = backbreakok(c);
			if (wf == TRUE)
				forwchar(FALSE, 1);
			else {
				if (backchar(FALSE, 1) == FALSE)
					return FALSE;

				while (c = lgetc(wp->w_dotp, wp->w_doto), iskanji(c)) {
					c = (c << 8) + lgetc(wp->w_dotp, wp->w_doto + 1);
					if (backbreakok(c)) {
						forwchar(FALSE, 1);
						break;
					}
					cnt++;
					if (backchar(FALSE, 1) == FALSE)
						return FALSE;
					if (wp->w_doto == 0) {
						gotoeol(FALSE, 1);
						if (f == TRUE)
							return TRUE;
						return wrapexpand ? indent(FALSE, 1) : lnewline();
					}
				}
			}
			if (f == TRUE)
				return TRUE;
			if ((wrapexpand ? indent(FALSE, 1) : lnewline()) == FALSE)
				return FALSE;
			if (wf == FALSE) {
				while (cnt-- > 0)
					forwchar(FALSE, 1);
			}
			return TRUE;
		} else {
			forwchar(FALSE, 1);
			if (backbreakok(c) == FALSE || f == TRUE)
				return TRUE;
			return wrapexpand ? indent(FALSE, 1) : lnewline();
		}
	}
	if (getccol(FALSE) < fillcol) {
		forwchar(FALSE, 1);
		return wrapexpand ? indent(FALSE, 1) : lnewline();
	}

	{
		int cnt = 0;

		while (c = lgetc(wp->w_dotp, wp->w_doto), (c != ' ' && c != TAB && !iskanji(c))) {
			cnt++;
			if (backchar(FALSE, 1) == FALSE)
				return FALSE;
			if (wp->w_doto == 0) {
				gotoeol(FALSE, 0);
				if (f == TRUE)
					return TRUE;
				return wrapexpand ? indent(FALSE, 1) : lnewline();
			}
		}
		if (c == ' ' || c == TAB) {
			if (forwdel(FALSE, 1) == FALSE)
				return FALSE;
		} else {
			while (iskanji(c)) {
				c = (c << 8) + lgetc(wp->w_dotp, wp->w_doto + 1);
				if (backbreakok(c)) {
					forwchar(FALSE, 1);
					break;
				}
				cnt++;
				if (backchar(FALSE, 1) == FALSE)
					return FALSE;
				if (wp->w_doto == 0) {
					gotoeol(FALSE, 1);
					if (f == TRUE)
						return TRUE;
					return wrapexpand ? indent(FALSE, 1) : lnewline();
				}
				c = lgetc(wp->w_dotp, wp->w_doto);
			}
		}
		if ((wrapexpand ? indent(FALSE, 1) : lnewline()) == FALSE)
			return FALSE;
		while (cnt-- > 0) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}

	if (wp->w_fcol != 0) {
		wp->w_fcol = 0;
		wp->w_flag |= WFHARD | WFMOVE | WFMODE;
	}
	return TRUE;
}

/*
========================================
	前進 非単語部分スキップ
========================================
*/

static int forw_skip(void)
{
	while (inword() == FALSE) {
		if (forwchar(FALSE, 1) == FALSE)
			return FALSE;
		csize++;
	}
	return TRUE;
}

/*
========================================
	後進 非単語部分スキップ
========================================
*/

static int back_skip(void)
{
	while (inword() == FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return FALSE;
		csize++;
	}
	return TRUE;
}

/*
========================================
	前進 単語部分スキップ
========================================
*/

static int forw_go(void)
{
	while (inword() == TRUE && wbound == FALSE) {
		if (forwchar(FALSE, 1) == FALSE)
			return FALSE;
		csize++;
	}
	return TRUE;
}

/*
========================================
	後進 単語部分スキップ
========================================
*/

static int back_go(void)
{
	while (inword() == TRUE && wbound == FALSE) {
		if (backchar(FALSE, 1) == FALSE)
			return FALSE;
		csize++;
	}
	return TRUE;
}

/*
========================================
	前進単語書き換え
========================================
*/

static int forw_case(int flag)
{
	int cap = 0;

	while (inword() == TRUE && wbound == FALSE) {
		int c;

		c = lgetc(curwp->w_dotp, curwp->w_doto);
		switch (flag) {
		case CASE_LOWER:
			c = tolower(c);
			break;
		case CASE_UPPER:
			c = toupper(c);
			break;
		case CASE_CAP:
			c = cap ? tolower(c) : (cap = 1, toupper(c));
			break;
		}
		lputc(curwp->w_dotp, curwp->w_doto, c);
		lchange(WFEDIT);

		if (forwchar(FALSE, 1) == FALSE)
			return FALSE;
	}

	return TRUE;
}

/*
----------------------------------------
	Ｎ語戻る
----------------------------------------
*/

int backword(int f, int n)
{
	if (n < 0)
		return forwword(f, -n);

	if (backchar(FALSE, 1) == FALSE)
		return FALSE;

	inword();
	while (n--) {
		if (back_skip() == FALSE)
			return FALSE;
		if (back_go() == FALSE)
			return FALSE;
	}

	return forwchar(FALSE, 1);
}

/*
----------------------------------------
	Ｎ語進む
----------------------------------------
*/

int forwword(int f, int n)
{
	if (n < 0)
		return backword(f, -n);

	inword();
	while (n--) {
		if (forw_go() == FALSE)
			return FALSE;
		if (forw_skip() == FALSE)
			return FALSE;
	}

	return TRUE;
}

/*
----------------------------------------
	語尾まで進む
----------------------------------------
*/

int endword(int f, int n)
{
	if (n < 0)
		return backword(f, -n);

	inword();
	while (n--) {
		if (forw_skip() == FALSE)
			return FALSE;
		if (forw_go() == FALSE)
			return FALSE;
	}

	return TRUE;
}

/*
========================================
	CASE 本体
========================================
*/

static int case_sub(int n, int flag)
{
	inword();
	while (n--) {
		if (forw_skip() == FALSE)
			return FALSE;
		if (forw_case(flag) == FALSE)
			return FALSE;
	}

	return TRUE;
}

/*
----------------------------------------
	単語を大文字にする
----------------------------------------
*/

int upperword(int f, int n)
{
	return case_sub(n, CASE_UPPER);
}

/*
----------------------------------------
	単語を小文字にする
----------------------------------------
*/

int lowerword(int f, int n)
{
	return case_sub(n, CASE_LOWER);
}

/*
----------------------------------------
	単語をキャピタライズ
----------------------------------------
*/

int capword(int f, int n)
{
	return case_sub(n, CASE_CAP);
}

/*
----------------------------------------
	次のＮ語削除
----------------------------------------
*/

int delfword(int f, int n)
{
	int doto;
	LINE *dotp;
	WINDOW *wp = curwp;

	dotp = curwp->w_dotp;
	doto = curwp->w_doto;
	csize = 0;
	inword();

	if (forw_skip() == FALSE)
		return FALSE;

	if (n == 0) {
		if (forw_go() == FALSE)
			return FALSE;
	} else {
		while (n--) {
			while (wp->w_doto == llength(wp->w_dotp)) {
				if (forwchar(FALSE, 1) == FALSE)
					return FALSE;
				csize++;
			}
			if (forw_go() == FALSE)
				return FALSE;
			if (n != 0) {
				if (forw_skip() == FALSE)
					return FALSE;
			}
		}

		{
			int c;

			while (wp->w_doto == llength(wp->w_dotp)
			       || (c = lgetc(wp->w_dotp, wp->w_doto)
				   ,((c == ' ')
				     || (c == TAB)
				     || (c == 0x81 && lgetc(wp->w_dotp, wp->w_doto + 1) == 0x40)))) {
				if (forwchar(FALSE, 1) == FALSE)
					break;
				csize++;
			}
		}
	}
	wp->w_dotp = dotp;
	wp->w_doto = doto;

	return forwdel(TRUE, csize);
}

/*
----------------------------------------
	前のＮ語削除
----------------------------------------
*/

int delbword(int f, int n)
{
	if (backchar(FALSE, 1) == FALSE)
		return FALSE;

	csize = 0;
	inword();
	while (n--) {
		if (back_skip() == FALSE)
			return FALSE;
		while (inword() == TRUE && wbound == FALSE) {
			csize++;
			if (backchar(FALSE, 1) == FALSE)
				goto del_go;
		}
	}
	if (forwchar(FALSE, 1) == FALSE)
		return FALSE;

del_go:

	return forwdel(TRUE, csize);
}

/*
========================================
	単語内か判定
========================================
*/

int inword(void)
{
	char buff[3];

	if (oldchar == BREAK) {
		oldchar = KANJI;
		wbound = TRUE;
		return TRUE;
	}
	wbound = FALSE;

	{
		int c;
		WINDOW *wp = curwp;

		if (wp->w_doto == llength(wp->w_dotp))
			return FALSE;

		c = lgetc(wp->w_dotp, wp->w_doto);

		if (isalnmkana(c)) {
			if (oldchar == KANJI)
				wbound = TRUE;
			oldchar = ANK;
			return TRUE;
		} else {
			buff[0] = c;
			buff[1] = lgetc(wp->w_dotp, wp->w_doto + 1);
			buff[2] = 0;
			if (*jstrmatch(buff, (char *) breakchar)) {
				oldchar = BREAK;
				return TRUE;
			} else if (iskanji(c)) {
				if (oldchar == ANK)
					wbound = TRUE;
				oldchar = KANJI;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
----------------------------------------
	インデント付 文整形
----------------------------------------
*/

int fillpara(int f, int n)
{
	int save;
	int status;

	status = FALSE;
	save = fillcol;
	if (f == TRUE)
		fillcol = n;

	if (fillcol <= 2) {
		mlwrite(KTEX98);
		goto done;
	}
	if (curwp->w_dotp == curbp->b_linep)
		goto done;

	{
		LINE *bop, *eop;
		int isitem = FALSE;
		int psize;
		int indent_size, indent_column;
		int indent_add_size, indent_add_col;
		char *para;
		char *indent_str;
		char *indent_add_str;

		gotobol(FALSE, 1);
		gotoeop(FALSE, 1);
		eop = lforw(curwp->w_dotp);
		gotobop(FALSE, 1);
		bop = curwp->w_dotp;

		if (wrapexpand) {
			LINE *lp;
			int c, length;
			char *p;

			isitem = FALSE;

			lp = bop;
			length = llength(lp);
			indent_size = 0;
			indent_column = 0;
			for(p = lp->l_text; indent_size < length; indent_size++) {
				c = *p++;
				switch (c) {
				case TAB:
					indent_column += curbp->b_tabs - (indent_column % curbp->b_tabs);
					break;
				case ' ':
					indent_column++;
					break;
				case 0x81:
					if (length - indent_size >= 2) {
						if (*p++ == 0x40) {
							indent_column += 2;
							indent_size++;
							break;
						} else
							p--;
					}
				default:
					if (iskanji(c) && length - indent_size >= 2)
						c = (c << 8) | *p;
					isitem = jstrchr(wrapitem, c) != 0;
					goto calc_end;
				}
			}

		calc_end:

			{
				int c;
				char *p;

				indent_add_col = 0;
				indent_add_str = isitem ? wrapindentitem : wrapindenthead;

				p = indent_add_str;
				while (c = *p++) {
					switch (c) {
					case TAB:
						indent_add_col += curbp->b_tabs
						  - (indent_add_col % curbp->b_tabs);
						break;
					case ' ':
						indent_add_col++;
						break;
					case 0x81:
						if (*p++ == 0x40)
							indent_add_col += 2;
						else
							p[-2] = 0;
						break;
					default:
						*p = 0;
					}
				}
				indent_add_size = strlen(indent_add_str);
			}

			if (isitem) {
				indent_str = malloc(indent_size ? : 1);
				if (indent_str == 0) {
					mlwrite(KTEX99);
					goto done;
				}
				memcpy(indent_str, lp->l_text, indent_size);
			} else {
				indent_column -= indent_add_col;
				if (indent_column > 0) {
					int left;
					int col, i;
					char *p;

					left = 0;
					for(col = i = 0, p = lp->l_text; col < indent_column; i++) {
						switch (*p++) {
						case TAB:
							{
								int new_col;

								new_col = col + curbp->b_tabs - (col % curbp->b_tabs);
								if (new_col > indent_column) {
									left = indent_column - col;
									goto check_end;
								}
								col = new_col;
							}
							break;
						case ' ':
							col++;
							break;
						case 0x81:
							if (col + 1 == indent_column) {
								left = 1;
								goto check_end;
							}
							p++;
							i++;
							col += 2;
						}
					}

				check_end:
					indent_size = i + left;
					indent_str = malloc(indent_size ? : 1);
					if (indent_str == 0) {
						mlwrite(KTEX99);
						goto done;
					}
					memcpy(indent_str, lp->l_text, i);
					memset(indent_str + i, ' ', left);
				} else {
					indent_size = 0;
					indent_column = 0;
					indent_str = malloc(0);
					if (indent_str == 0) {
						mlwrite(KTEX99);
						goto done;
					}
				}
			}
		} else {
			indent_size = 0;
			indent_column = 0;
			indent_str = malloc(0);
			if (indent_str == 0) {
				mlwrite(KTEX99);
				goto done;
			}
			indent_add_size = 0;
			indent_add_col = 0;
			indent_add_str = indent_str;
		}

		{
			int last_ch_type = 0;
			LINE *lp;

			para = malloc(0);
			if (para == 0) {
				mlwrite(KTEX99);
				goto done;
			}

			curbp->b_dotp = bop;

			for(psize = 0, lp = bop; lp != eop; lp = lforw(lp)) {
				int c, length;
				int i;
				char *p;

				i = 0;
				p = lp->l_text;
				length = llength(lp);
				if (wrapexpand) {
					for(; i < length; i++) {
						c = *p++;
						switch (c) {
						case TAB:
						case ' ':
							break;
						case 0x81:
							if (length - i >= 2) {
								if (*p++ == 0x40) {
									i++;
									break;
								}
								p--;
							}
						default:
							p--;
							goto search_stop;
						}
					}
				search_stop:;
				}

				{
					char *begin, *end;

					begin = p;
					end = p + length - i - 1;
					if (wrapexpand) {
						while (end >= begin) {
							int c;

							c = *end;
							if (c == TAB || c == ' ')
								end--;
							else if (c == 0x40) {
								if (end - 1 >= begin) {
									if (end[-1] == 0x81)
										end -= 2;
									else
										break;
								}
							} else
								break;
						}
					}

					if (end >= begin) {
						int size;
						int add_spc;

						size = end - begin + 1;
						add_spc = 1;
						if (!psize || (last_ch_type == CT_KJ2 && iskanji(*begin)))
							add_spc = 0;
						para = realloc(para, psize + size + add_spc + 1);
						if (para == 0) {
							mlwrite(KTEX99);
							goto done;
						}
						if (add_spc)
							para[psize++] = ' ';
						memcpy(para + psize, begin, size);
						psize += size;
						last_ch_type = nthctype(begin, size);
					}
				}
			}
			para[psize] = 0;

			if (psize == 0) {
				free(para);
				status = TRUE;
				goto done;
			}
		}

		if (fillcol - (indent_column + indent_add_col) >= 2) {
			int nline;
			char *newpara;

			nline = psize / (fillcol - (indent_column + indent_add_col)) + 1;
			newpara = malloc(nline * (fillcol + 20));
			if (newpara == 0) {
				free(para);
				free(indent_str);
				mlwrite(KTEX99);
				goto done;
			}

			reform(para, newpara, curbp->b_tabs, isitem,
			  indent_str, indent_size, indent_column,
			  indent_add_str, indent_add_size, indent_add_col);
			free(para);
			free(indent_str);
			{
				LINE *lp;

				for(lp = bop; lp != eop; lp = lforw(lp))
					lfree(lp);
			}
			status = linstr(newpara);
			free(newpara);
			lnewline();
			if (curwp->w_fcol != 0) {
				curwp->w_fcol = 0;
				curwp->w_flag |= WFHARD | WFMOVE | WFMODE;
			}
		}
	}

done:
	fillcol = save;
	return status;
}

/*
========================================
	前で切れるか判断
========================================
*/

int forwbreakok(int c)
{
	char buff[3];

	buff[0] = c >> 8;
	buff[1] = c & CHARMASK;
	buff[2] = 0;
	return *jstrmatch(buff, fornonbreakchar) ? FALSE : TRUE;
}

/*
========================================
	後ろで切れるか判断
========================================
*/

static int backbreakok(int c)
{
	char buff[3];

	buff[0] = c >> 8;
	buff[1] = c & CHARMASK;
	buff[2] = 0;
	return *jstrmatch(buff, backnonbreakchar) ? FALSE : TRUE;
}

/*
-----------------------------------
	行の左端へ
----------------------------------
*/

static char *findleft(char *top, char *sp)
{
	char *lf;

	for (lf = sp; lf > top; lf--) {
		if (*lf == I_NEWLINE) {
			lf++;
			break;
		}
	}
	return lf;
}

/*
========================================
	リフォーマット
========================================
*/

static void reform(char *sp, char *newpara, int tabs, int isitem,
  char *indent_str, int indent_size, int indent_column,
  char *indent_add_str, int indent_add_size, int indent_add_col)
{
	int c1;
	int c2;
	int c3;
	int bol;
	int col;
	int kanjiflag = ANK;
	int devkanjiflag = ANK;
	int oldkanji;
	char *lastsp;
	char *lastnew;
	char *top = sp;
	char *newtop = newpara;
	char *spp;
	char *npp;
	char *lf;

	bol = TRUE;

	lastsp = sp;
	lastnew = newpara;
	col = 0;
	while (*sp) {
		oldkanji = kanjiflag;

		if (col == 0) {
			if (indent_column) {
				memcpy(newpara, indent_str, indent_size);
				newpara += indent_size;
				col = indent_column;
			}
			if (indent_add_col) {
				if (bol && !isitem || !bol && isitem) {
					memcpy(newpara, indent_add_str, indent_add_size);
					newpara += indent_add_size;
					col += indent_add_col;
				}
				bol = FALSE;
			}
		}

		if ((*sp == ' ') || (*sp == '\t')) {
			kanjiflag = SPC;
			if (*sp == '\t')
				col += -(col % tabs) + tabs;
			else
				col++;
			if (col >= fillcol) {
				*newpara++ = I_NEWLINE;
				col = 0;
			} else {
				*newpara++ = *sp;
			}
			sp++;
			lastsp = sp;
			lastnew = newpara;
		} else if (iskanji(*sp)) {
			kanjiflag = KANJI;
			if (oldkanji == ANK) {
				devkanjiflag = ANK;
				lastsp = sp;
				lastnew = newpara;
			}
			*newpara++ = *(sp + 0);
			*newpara++ = *(sp + 1);
			col += 2;
			if (col > fillcol) {
				c1 = (*(sp + 0) << 8 | *(sp + 1));
				if (nthctype_ptr(findleft(top, sp - 1), sp - 1)) {
					c2 = (*(sp - 2) << 8 | *(sp - 1));
				} else {
					c2 = sp[-1];
				}
				if (breakmode != 0) {
					if (backbreakok(c2) == FALSE) {
						spp = sp - 3;
						npp = newpara - 4;
						lf = findleft(top, sp);
						while (nthctype_ptr(lf, spp)) {
							c3 = (spp[-1] << 8) + spp[0];
							if (backbreakok(c3))
								break;
							npp -= 2;
							spp -= 2;
						}
						if (npp != newtop && npp[-1] != I_NEWLINE) {
							spp += 1;
							*npp++ = I_NEWLINE;
							newpara = npp;

							col = 0;
							if (indent_column) {
								memcpy(newpara, indent_str, indent_size);
								newpara += indent_size;
								col = indent_column;
							}
							if (indent_add_col) {
								if (bol && !isitem || !bol && isitem) {
									memcpy(newpara, indent_add_str, indent_add_size);
									newpara += indent_add_size;
									col += indent_add_col;
								}
								bol = FALSE;
							}
							while (spp <= sp) {
								*newpara++ = spp[0];
								*newpara++ = spp[1];
								col += 2;
								spp += 2;
							}
						}
					}
				}
				if (forwbreakok(c1) && backbreakok(c2)) {
					*lastnew = I_NEWLINE;
					newpara = lastnew + 1;
					sp = lastsp;
					col = 0;
					continue;
				}
			}
			sp += 2;
			lastnew = newpara;
			lastsp = sp;
		} else {
			kanjiflag = ANK;
			if (oldkanji == KANJI) {
				devkanjiflag = KANJI;
				lastsp = sp;
				lastnew = newpara;
			}
			*newpara++ = *sp++;
			++col;
			if (col > fillcol) {
				if (devkanjiflag == KANJI) {
					devkanjiflag = ANK;
					if (breakmode != 0) {
						spp = lastsp;
						npp = lastnew;
						lf = findleft(newtop, npp - 1);
						while (npp > newtop && nthctype_ptr(lf, npp - 1)) {
							c3 = (npp[-2] << 8) + npp[-1];
							if (backbreakok(c3))
								break;
							spp -= 2;
							npp -= 2;
						}
						if (npp != newtop && npp[-1] != I_NEWLINE) {
							sp = spp;
							lastnew = newpara = npp + 1;
							*npp = I_NEWLINE;
							col = 0;
						} else {
							devkanjiflag = KANJI;
						}
					} else {
						c3 = (lastsp[-2] << 8) + lastsp[-1];
						if (backbreakok(c3)) {
							sp = lastsp;
							*lastnew = I_NEWLINE;
							newpara = ++lastnew;
							col = 0;
						} else {
							devkanjiflag = KANJI;
						}
					}
				} else {
					if (lastnew > newtop && *(lastnew - 1) != I_NEWLINE) {
						*(lastnew - 1) = I_NEWLINE;
						newpara = lastnew;
						sp = lastsp;
						col = 0;
					}
				}
			}
		}
	}
	*newpara = 0;
}

/*
----------------------------------------
	段落削除
----------------------------------------
*/

int killpara(int f, int n)
{
	WINDOW *wp = curwp;

	while (n--) {
		int status;

		gotoeop(FALSE, 1);
		wp->w_markp[0] = wp->w_dotp;
		wp->w_marko[0] = wp->w_doto;
		gotobop(FALSE, 1);
		wp->w_doto = 0;

		status = killregion(FALSE, 1);
		if (status != TRUE)
			return status;

		ldelete(2, TRUE);
	}

	return TRUE;
}

/*
----------------------------------------
	単語数計算
----------------------------------------
*/

int wordcount(int f, int n)
{
	REGION region;

	{
		int status;

		status = getregion(&region, f, n);
		if (status != TRUE)
			return status;
	}

	{
		int offset, size, lastword;
		int nchars, nwords, nlines, wordflag;
		LINE *lp;

		lp = region.r_linep;
		offset = region.r_offset;
		size = region.r_size;
		lastword = wordflag = 0;
		nchars = nwords = nlines = 0;

		{
			int ch;

			while (size--) {
				if (offset == llength(lp)) {
					ch = I_NEWLINE;
					lp = lforw(lp);
					offset = 0;
					nlines++;
				} else {
					ch = lgetc(lp, offset);
					if (iskanji(ch)) {
						offset++;
						size--;
					}
					offset++;
				}

				wordflag = isalnum(ch);
				if (wordflag && !lastword)
					nwords++;
				lastword = wordflag;
				nchars++;
			}
		}

		{
			int avgch;

			avgch = (nwords > 0) ? (nchars * 100 / nwords) : 0;
			mlwrite(KTEX100, nwords, nchars, nlines + 1, avgch);
		}
	}

	return TRUE;
}

/*
----------------------------------------
	全角を半角にする
----------------------------------------
*/

int zentohanword(int f, int n)
{
	WINDOW *wp = curwp;

	inword();
	while (n--) {
		if (forw_skip() == FALSE)
			return FALSE;
		while (inword() == TRUE && wbound == FALSE) {
			int c;

			c = lgetc(wp->w_dotp, wp->w_doto);
			if (iskanji(c)) {
				int s;

				c = (c << 8) + lgetc(wp->w_dotp, wp->w_doto + 1);
				s = zentohan(c);
				if (s != c) {
					ldelete(2, FALSE);
					linsert(1, s & CHARMASK);
					if (s >= 0x100)
						linsert(1, s >> 8);
					continue;
				}
			}
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}

	return TRUE;
}

/*
----------------------------------------
	半角を全角にする
----------------------------------------
*/

int hantozenword(int f, int n)
{
	WINDOW *wp = curwp;

	inword();
	while (n--) {
		if (forw_skip() == FALSE)
			return FALSE;
		while (inword() == TRUE && wbound == FALSE) {
			int c, r;

			c = lgetc(wp->w_dotp, wp->w_doto) | ((tokana == TRUE) ? 0 : 0x10000);
			r = hantozen(c);
			if (r != c) {
				c = lgetc(wp->w_dotp, wp->w_doto + 1);
				if (c == 0xde || c == 0xdf) {
					if (c == 0xdf)
						r++;
					r++;
					ldelete(1, FALSE);
				}
				ldelete(1, FALSE);
				linsert(1, r >> 8);
				linsert(1, r & CHARMASK);
			} else if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}

	return TRUE;
}

/*
----------------------------------------
	check wordrap
----------------------------------------
*/

int check_wrap(void)
{
	return ((curwp->w_bufp->b_mode & MDWRAP)
		&& fillcol > 0
		&& getccol(FALSE) > fillcol
		&& (curwp->w_bufp->b_mode & (MDVIEW | MDDIRED)) == FALSE)
	? TRUE : FALSE;
}
