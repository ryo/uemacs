/*
----------------------------------------
	EXEC.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "edir.h"
#include "elang.h"
#include "ekanji.h"
#include "ecall.h"
#include "fepctrl.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/exec.c,v 1.6 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int debug(BUFFER *, char *);
static int docmd(char *);
static int doproc(int, int, char *, int *);

/*
========================================
	ブロック解放
========================================
*/

static void freewhile(WHBLOCK *wp)
{
	WHBLOCK *w_next;

	for (; wp; wp = w_next) {
		w_next = wp->w_next;
		free(wp);
	}
}

/*
========================================
	引数取得
========================================
*/

int macarg(char *t)
{
	int status, sv = clexec;

	clexec = TRUE;
	status = nextarg("", t, NSTRING, ctoec('\r'));
	clexec = sv;
	return status;
}

/*
----------------------------------------
	コマンド実行
----------------------------------------
*/

int namedcmd(int f, int n)
{
	char *buf;
	char buffer[NSTRING];

	if (clexec == TRUE) {
		char *sp;

		execstr = token(execstr, buffer, NPAT);
		sp = getval(buffer);
		strcpy(buffer, sp ? : "");
		if (strcmp(buffer, (char *) errorm) == 0)
			return FALSE;
		buf = buffer;
	} else
		buf = fixnull(getname(": "));

	{
		NBIND *nb;
		int status;
		int save;

		save = clexec;
		clexec = FALSE;
		nb = com_in_word_set(buf);
		if (nb == 0) {
			int ecode;

			status = doproc(f, n, buf, &ecode);
			switch (ecode) {
			case -1:
				mlwrite(KTEX16);
				break;
			case -2:
				mlwrite(KTEX130);
				break;
			}
		} else
			status = execnbind(nb, f, n);
		clexec = save;
		return status;
	}
}

/*
----------------------------------------
	コマンド実行 2
----------------------------------------
*/

int execcmd(int f, int n)
{
	int status;
	char cmdstr[NSTRING];

	his_enable(bhiscmdp);
	status = mlreply(": ", cmdstr, NSTRING);
	if (status != TRUE)
		return status;
	execlevel = 0;
	return docmd(cmdstr);
}

/*
----------------------------------------
	コマンド実行 3
----------------------------------------
*/

int exechook(char *cmdstr)
{
	int status;

	execlevel = 0;
	executinghook = TRUE;
	status = *cmdstr ? docmd(cmdstr) : TRUE;
	executinghook = FALSE;

	return status;
}

/*
========================================
	コマンド実行本体
========================================
*/

static int docmd(char *cline)
{
	char tkn[NSTRING];
	char bufn[NBUFN + 2];

	if (execlevel)
		return (TRUE);

	{
		int f, n, status;
		char *oldestr = execstr;

		execstr = cline;
		f = FALSE;
		n = 1;
		lastflag = thisflag;
		thisflag = 0;
		status = macarg(tkn);
		if (status != TRUE) {
			execstr = oldestr;
			return status;
		}
		if (gettyp(tkn) != TKCMD) {
			char *s;

			f = TRUE;
			s = getval(tkn);
			strcpy(tkn, s ? : "");
			n = asc_int(tkn);
			status = macarg(tkn);
			if (status != TRUE) {
				execstr = oldestr;
				return status;
			}
		}

		{
			NBIND *nb;

			nb = com_in_word_set(tkn);
			if (nb == 0) {
				BUFFER *bp;

				sprintf(bufn, "[%s]", tkn);
				bp = bfind(bufn, FALSE, 0);
				if (bp == 0 || bp->b_active == FALSE) {
					int ecode;

					status = loadexec(tkn, &ecode);
					if (ecode < 0)
						mlwrite(KTEX116);
					if (status != TRUE)
						return status;
					bp = bfind(bufn, FALSE, 0);
					if (bp == 0) {
						mlwrite(KTEX16);
						execstr = oldestr;
						return FALSE;
					}
				}
				if (bp->b_active == FALSE) {
					mlwrite(KTEX130);
					return FALSE;
				}

				{
					int savearg, oldcle = clexec;

					clexec = TRUE;
					savearg = cmdarg;
					for (cmdarg = n; cmdarg > 0; cmdarg--) {
						status = dobuf(bp);
						if (status != TRUE)
							break;
					}
					cmdarg = savearg;
					cmdstatus = status;
					clexec = oldcle;
				}

				execstr = oldestr;
				return status;
			}

			{
				int oldcle = clexec;

				clexec = TRUE;
				status = execnbind(nb, f, n);
				cmdstatus = status;
				clexec = oldcle;
			}
		}
		execstr = oldestr;
		return status;
	}
}

/*
========================================
	トークン切り出し
========================================
*/

char *token(char *src, char *tok, int size)
{
	while (1) {
		if (*src == ' ' || *src == TAB)
			src++;
		else if (*src == 0x81 && src[1] == 0x40)
			src += 2;
		else
			break;
	}

	{
		int quotef = 0;

		while (*src) {
			if (*src == '~') {
				char c;

				if (*++src == 0)
					break;
				switch (*src) {
				case 'r':
					c = 13;
					break;
				case 'n':
					c = 10;
					break;
				case 'l':
					c = 10;
					break;
				case 't':
					c = 9;
					break;
				case 'b':
					c = 8;
					break;
				case 'f':
					c = 12;
					break;
				default:
					c = *src;
					break;
				}
				src++;
				if (--size > 0)
					*tok++ = c;
			} else {
				if (quotef) {
					if (*src == '"')
						break;
				} else {
					if (*src == ' ' || *src == TAB)
						break;
					else if (*src == 0x81 && src[1] == 0x40) {
						src++;
						break;
					}
				}
				if (*src == '"')
					quotef = TRUE;
				if (iskanji(*src)) {
					size -= 2;
					if (size > 0) {
						*tok++ = *src++;
						*tok++ = *src++;
					}
				} else {
					if (--size > 0)
						*tok++ = *src++;
				}
			}
		}
	}

	if (*src)
		src++;
	*tok = 0;
	return src;
}

/*
========================================
	次の引数
========================================
*/

int nextarg(char *prompt, char *buffer, int size, int terminator)
{
	char *sp;

	if (clexec == FALSE)
		return getstring(prompt, buffer, size, terminator);
	execstr = token(execstr, buffer, size);
	sp = getval(buffer);
	if (sp == 0)
		return FALSE;
	strcpy(buffer, sp);
	return TRUE;
}

/*
----------------------------------------
	procedure 設定
----------------------------------------
*/

int storeproc(int f, int n)
{
	int status;
	char bname[NBUFN];

	his_disable();
	status = mlreply(KTEX114, bname + 1, NBUFN - 2);
	if (status != TRUE)
		return status;
	*bname = '[';
	strcat(bname, "]");

	{
		BUFFER *bp;

		bp = bfind(bname, TRUE, BFINVS);
		if (bp == 0) {
			mlwrite(KTEX113);
			return FALSE;
		}
		bp->b_active = TRUE;
		bclear(bp);
		mstore = TRUE;
		bstore = bp;
	}

	return TRUE;
}

/*
----------------------------------------
	procedure 実行
----------------------------------------
*/

int execproc(int f, int n)
{
	char *name;
	int status, ecode;

	name = complete(KTEX115, 0, 0, CMP_MACRO, NBUFN);
	if (name == 0)
		return FALSE;

	status = doproc(f, n, name, &ecode);
	switch (ecode) {
	case -1:
		mlwrite(KTEX116);
		break;
	case -2:
		mlwrite(KTEX130);
		break;
	}

	return status;
}

int doproc(int f, int n, char *name, int *ecode)
{
	char bufn[NBUFN + 2];

	*ecode = 0;
	sprintf(bufn, "[%s]", name);

	{
		BUFFER *bp;

		bp = bfind(bufn, FALSE, 0);
		if (bp == 0 || bp->b_active == FALSE) {
			int result;

			result = loadexec(name, ecode);
			if (result != TRUE)
				return result;
			bp = bfind(bufn, FALSE, 0);
			if (bp == 0) {
				*ecode = -1;
				return FALSE;
			}
		}
		if (bp->b_active == FALSE) {
			*ecode = -2;
			return FALSE;
		}
		{
			int savearg = cmdarg;

			for (cmdarg = n; cmdarg > 0; cmdarg--) {
				int status;

				status = dobuf(bp);
				if (status != TRUE)
					return status;
			}
			cmdarg = savearg;
		}
	}

	return TRUE;
}

/*
----------------------------------------
	バッファ実行
----------------------------------------
*/

int execbuf(int f, int n)
{
	BUFFER *bp;

	bp = getcbuf(KTEX117, curbp->b_bname, FALSE);
	if (bp == 0) {
		mlwrite(KTEX118);
		return FALSE;
	}

	{
		int savearg = cmdarg;

		for (cmdarg = n; cmdarg > 0; cmdarg--) {
			int status;

			status = dobuf(bp);
			if (status != TRUE)
				return status;
		}
		cmdarg = savearg;
	}

	return TRUE;
}

/*
========================================
	バッファ実行本体
========================================
*/

int dobuf(BUFFER *bp)
{
	int ifdone;
	char *eline, tkn[NSTRING], out[NSTRING];
	WHBLOCK *whlist, *scanner;

	ifdone = execlevel = 0;
	syseval = TRUE;
	whlist = scanner = 0;

	{
		int ln;
		LINE *lp, *hlp;

		hlp = bp->b_linep;
		lp = hlp->l_fp;
		for (ln = 1; lp != hlp; ln++, lp = lforw(lp)) {
			{
				int i;

				for (eline = lp->l_text, i = llength(lp); i > 0; i--, eline++) {
					int c;

					if (iskanji(c = *eline))
						i--, c = (c << 8) + *++eline;
					if (c != ' ' && c != TAB && c != '(' && c != ')' && c != 0x8140)
						break;
				}
				if (i <= 0)
					continue;
			}

			if (*eline == '!') {
				if (eline[1] == 'w' && eline[2] == 'h') {
					WHBLOCK *whtemp;

					whtemp = (WHBLOCK *) malloc(sizeof(WHBLOCK));
					if (whtemp == 0) {
					      noram:
						mlforce(KTEX119);
					      failexit:
						freewhile(scanner);
						freewhile(whlist);
						return FALSE;
					}
					whtemp->w_begin = lp;
					whtemp->w_type = BTWHILE;
					whtemp->w_next = scanner;
					scanner = whtemp;
				} else if (eline[1] == 'b' && eline[2] == 'r') {
					WHBLOCK *whtemp;

					if (scanner == 0) {
						sprintf(out, KTEX120, bp->b_bname, ln);
						makelit(out);
						mlforce(out);
						goto failexit;
					}
					whtemp = (WHBLOCK *) malloc(sizeof(WHBLOCK));
					if (whtemp == 0)
						goto noram;
					whtemp->w_begin = lp;
					whtemp->w_type = BTBREAK;
					whtemp->w_next = scanner;
					scanner = whtemp;
				} else if (strncmp(&eline[1], "endw", 4) == 0) {
					WHBLOCK *whtemp;

					if (scanner == 0) {
						sprintf(out, KTEX121, bp->b_bname, ln);
						makelit(out);
						mlforce(out);
						goto failexit;
					}
					do {
						scanner->w_end = lp;
						whtemp = whlist;
						whlist = scanner;
						scanner = scanner->w_next;
						whlist->w_next = whtemp;
					}
					while (whlist->w_type == BTBREAK);
				}
			}
		}
		if (scanner) {
			sprintf(out, KTEX122, bp->b_bname, ln);
			makelit(out);
			mlforce(out);
			goto failexit;
		}
	}

	{
		int ln;
		char *einit;
		LINE *lp, *hlp;

		thisflag = lastflag;
		hlp = bp->b_linep;
		lp = hlp->l_fp;
		for (ln = 1; lp != hlp && eexitflag == FALSE; ln++, lp = lforw(lp)) {
			if (forceabort()) {
				mlforce(KTEX234);
				freewhile(whlist);
				return FALSE;
			}

			{
				int linlen = llength(lp);

				einit = eline = (char *) malloc(linlen + 1);
				if (eline == 0) {
					sprintf(out, KTEX123, bp->b_bname, ln);
					makelit(out);
					mlforce(out);
					freewhile(whlist);
					return FALSE;
				}
				strncpy(eline, lp->l_text, linlen);

				{
					int i, quote = 0;

					for (i = 0; i < linlen; i++) {
						int c = eline[i];

						if (iskanji(c)) {
							if (eline[i] == 0x81 && eline[i + 1] == 0x40) {
								eline[i] = ' ';
								eline[i + 1] = ' ';
							}
							i++;
						} else {
							if (c == '"')
								quote = quote ? FALSE : TRUE;
							else if (c == TAB)
								eline[i] = ' ';
							else if (c == '(' || c == ')') {
								if (quote == TRUE)
									continue;
								c = eline[i - 1];
								if (c == '^' || c == '-'
								    || (c == 'N' && eline[i - 2] == 'F')
								    || (c == 'X' && eline[i - 2] == '^'))
									continue;
								eline[i] = ' ';
							}
						}
					}
					eline[i] = 0;
				}

				while (*eline == ' ')
					eline++;
				if (*eline == ';' || *eline == 0)
					goto onward;
			}

			if (macbug && !mstore && (execlevel == 0)) {
				if (debug(bp, eline) == FALSE) {
					mlforce(KTEX54);
					freewhile(whlist);
					return FALSE;
				}
			}

			{
				int dirnum = -1;

				if (*eline == '!') {
					int i;
					UDIRS *fdir;

					for (i = 0; islower(eline[i + 1]); i++)
						tkn[i] = eline[i + 1];
					tkn[i] = 0;
					fdir = dirs_in_word_set(tkn, i);
					if (fdir == 0) {
						sprintf(out, KTEX124, bp->b_bname, ln);
						makelit(out);
						mlforce(out);
						freewhile(whlist);
						return FALSE;
					}
					dirnum = fdir->d_num;
					if (dirnum == DENDM) {
						mstore = FALSE;
						bstore = 0;
						goto onward;
					}
				}
				if (mstore) {
					int linlen;
					LINE *mp;

					linlen = strlen(eline);
					mp = lalloc(linlen);
					if (mp == 0) {
						sprintf(out, KTEX125, bp->b_bname, ln);
						makelit(out);
						mlforce(out);
						return FALSE;
					}
					{
						LINE *b_linep = bstore->b_linep;

						memcpy(mp->l_text, eline, linlen);

						b_linep->l_bp->l_fp = mp;
						mp->l_bp = b_linep->l_bp;
						b_linep->l_bp = mp;
						mp->l_fp = b_linep;
					}
					goto onward;
				}

				{
					int force = 0;

					if (*eline == '*')
						goto onward;
					if (dirnum != -1) {
						while (*eline && *eline != ' ')
							eline++;
						execstr = eline;
						switch (dirnum) {
						case DIF:
							if (execlevel == 0) {
								ifdone = 0;
								if (macarg(tkn) != TRUE)
									goto eexec;
								if (stol(tkn) == FALSE)
									execlevel++;
							} else
								execlevel++;
							goto onward;
						case DWHILE:
							if (execlevel == 0) {
								if (macarg(tkn) != TRUE)
									goto eexec;
								if (stol(tkn) == TRUE)
									goto onward;
							}
						case DBREAK:
							{
								WHBLOCK *whtemp;

								if (dirnum == DBREAK && execlevel)
									goto onward;
								for (whtemp = whlist; whtemp; whtemp = whtemp->w_next) {
									if (whtemp->w_begin == lp)
										break;
								}
								if (whtemp == 0) {
									sprintf(out, KTEX126, bp->b_bname, ln);
									makelit(out);
									mlforce(out);
									freewhile(whlist);
									return FALSE;
								}
								lp = whtemp->w_end;
							}
							goto onward;
						case DELSE:
							if (execlevel == 1 && ifdone == 0)
								execlevel--;
							else if (execlevel == 0) {
								execlevel++;
								ifdone++;
							}
							goto onward;
						case DELSIF:
							if (execlevel == 1 && ifdone == 0) {
								if (macarg(tkn) != TRUE)
									goto eexec;
								if (stol(tkn) == TRUE)
									execlevel--;
							} else if (execlevel == 0) {
								execlevel++;
								ifdone++;
							}
							goto onward;
						case DENDIF:
							if (execlevel)
								execlevel--;
							else
								ifdone = 0;
							goto onward;
						case DGOTO:
							if (execlevel == 0) {
								int linlen;
								LINE *glp;

								eline = token(eline, golabel, NPAT);
								linlen = strlen(golabel);
								for (glp = lforw(hlp); glp != hlp; glp = lforw(glp)) {
									if (*glp->l_text == '*'
									    && strncmp(glp->l_text + 1, golabel, linlen) == 0) {
										lp = glp;
										goto onward;
									}
								}
								sprintf(out, KTEX127, bp->b_bname, ln);
								makelit(out);
								mlforce(out);
								freewhile(whlist);
								return FALSE;
							}
							goto onward;
						case DRETURN:
							if (execlevel == 0) {
								if (macarg(tkn) != TRUE)
									goto eexec;
								if (*tkn == 0)
									goto eexec;
								if (stol(tkn) == FALSE)
									syseval = FALSE;
								goto eexec;
							}
							goto onward;
						case DENDWHILE:
							if (execlevel)
								execlevel--;
							else {
								WHBLOCK *whtemp;

								for (whtemp = whlist; whtemp; whtemp = whtemp->w_next) {
									if (whtemp->w_type == BTWHILE && whtemp->w_end == lp)
										break;
								}
								if (whtemp == 0) {
									sprintf(out, KTEX126, bp->b_bname, ln);
									makelit(out);
									mlforce(out);
									freewhile(whlist);
									return FALSE;
								}
								lp = lback(whtemp->w_begin);
							}
							goto onward;
						case DFORCE:
							force = TRUE;
						}
					}

					{
						int status;

						status = docmd(eline);
						if (force)
							status = TRUE;
						if (status != TRUE) {
							WINDOW *wp;

							for (wp = wheadp; wp; wp = wp->w_wndp) {
								if (wp->w_bufp == bp) {
									wp->w_dotp = lp;
									wp->w_doto = 0;
									wp->w_flag |= WFHARD;
								}
							}
							bp->b_dotp = lp;
							bp->b_doto = 0;
							free(einit);
							freewhile(whlist);

							execlevel = 0;
							sprintf(out, KTEX228, bp->b_bname, ln, eline);
							makelit(out);
							mlforce(out);
							getkey();

							return status;
						}
					}
				}
			}

		      onward:

			free(einit);
		}
	}

eexec:

	execlevel = 0;
	freewhile(whlist);
	return TRUE;
}

/*
========================================
	デバッグトレーサー
========================================
*/

static int debug(BUFFER *bp, char *eline)
{
	char temp[NSTRING];
	static char track[NSTRING];

	his_enable(bhisdebugp);

dbuild:

	strcpy(outline, "<<<");
	if (*track) {
		int oldstatus = cmdstatus;

		docmd(track);
		cmdstatus = oldstatus;
		sprintf(outline, "<<<[=%s]%s:%s>>>", gtusr("track"), bp->b_bname, eline);
	} else
		sprintf(outline, "<<<%s:%s>>>", bp->b_bname, eline);
	makelit(outline);

dinput:

	outline[term.t_ncol - (nthctype(outline, term.t_ncol) == CT_KJ2 ? 2 : 1)] = 0;
	mlforce(outline);

	{
		int c;
		KEYTAB *key;

		if (kbdmode != PLAY)
			fep_off();
		c = getkey();
		if (kbdmode != PLAY)
			fep_on();
		key = getbind(c);
		if (key && key->k_ptr.fp == meta)
			macbug = FALSE;
		else if (c == abortc)
			return FALSE;
		else {
			switch (c) {
			case '?':
				strcpy(outline, KTEX128);
				goto dinput;
			case 'c':
				{
					int oldcmd = discmd, oldinp = disinp, oldclexec = clexec;

					clexec = FALSE;
					discmd = TRUE;
					disinp = TRUE;
					execcmd(FALSE, 1);
					clexec = oldclexec;
					discmd = oldcmd;
					disinp = oldinp;
				}
				goto dbuild;
			case 'x':
				{
					int oldcmd = discmd, oldinp = disinp, oldclexec = clexec;
					int oldstatus = cmdstatus;

					clexec = FALSE;
					discmd = TRUE;
					disinp = TRUE;
					namedcmd(FALSE, 1);
					clexec = oldclexec;
					cmdstatus = oldstatus;
					discmd = oldcmd;
					disinp = oldinp;
				}
				goto dbuild;
			case 'e':
				strcpy(temp, "set %track ");
				{
					int oldcmd = discmd, oldinp = disinp;

					discmd = TRUE;
					disinp = TRUE;
					getstring("Exp: ", temp + 11, NSTRING, ctoec('\r'));
					discmd = oldcmd;
					disinp = oldinp;
				}
				{
					int oldstatus = cmdstatus;

					docmd(temp);
					cmdstatus = oldstatus;
				}
				sprintf(temp, " = [%s]", gtusr("track"));
				makelit(temp);
				mlforce(temp);
				getkey();
				goto dinput;
			case 't':
				{
					int oldcmd = discmd, oldinp = disinp;

					discmd = TRUE;
					disinp = TRUE;
					getstring("Exp: ", temp, NSTRING, ctoec('\r'));
					discmd = oldcmd;
					disinp = oldinp;
				}
				strcpy(track, "set %track ");
				strcat(track, temp);
				goto dbuild;
			case ' ':
				break;
			default:
				H68beep();
				goto dbuild;
			}
		}
	}

	return TRUE;
}

/*
========================================
	% を %% に拡張
========================================
*/

void makelit(char *s)
{
	char c, *sp, *ep;

	for (sp = s; c = *sp; sp++) {
		if (iskanji(c))
			sp++;
		else if (c == '%') {
			for (ep = sp + strlen(sp); ep >= sp; ep--)
				ep[1] = ep[0];
			sp++;
		}
	}
}

/*
----------------------------------------
	ファイル実行
----------------------------------------
*/

int execfile(int f, int n)
{
	char *fspec;

	{
		char *sp;

		sp = singleexpwild(gtfilename(KTEX129));
		if (sp == 0)
			return FALSE;
		fspec = flook(sp, TRUE);
		if (fspec == 0) {
			if (clexec == FALSE)
				mlwrite(KTEX214, sp);
			return FALSE;
		}
	}

	{
		int savearg = cmdarg;

		for (cmdarg = n; cmdarg > 0; cmdarg--) {
			int status;

			status = dofile(fspec);
			if (status != TRUE)
				return status;
		}
		cmdarg = savearg;
	}

	return TRUE;
}

/*
========================================
	ファイル実行本体
========================================
*/

int dofile(char *fname)
{
	BUFFER *bp;
	char bname[NBUFN];

	makename(bname, fname);
	unqname(bname);
	bp = bfind(bname, TRUE, 0);
	if (bp == 0)
		return FALSE;

	{
		int status;
		BUFFER *cb = curbp;

		curbp = bp;
		status = readin(FALSE, 0, fname);
		bp->b_mode = MDVIEW;
		bp->b_flag |= BFINVS;
		curbp = cb;
		if (status == TRUE)
			status = dobuf(bp);
		if (bp->b_nwnd == 0)
			zotbuf(bp);
		return status;
	}
}
