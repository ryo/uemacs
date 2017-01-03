/*
----------------------------------------
	SEARCH.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/search.c,v 1.6 1992/01/04 13:11:24 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static int delf[HICHAR], delb[HICHAR];
static int padd, lastf, lastb;

/*
========================================
	使用関数の定義
========================================
*/

static int amatch(MC *, int, LINE **, int *);
static int cclmake(char **, MC *);
static BITMAP clearbits(void);
static int delins(int, char *, int);
static int fbound(int, LINE **, int *, int);
static int mceq(char, MC *);
static int mcscanner(MC *, int, int);
static int mcstr(void);
static int nextch(LINE **, int *, int);
static int readpattern(char *, char *, int);
static void rmcclear(void);
static int rmcstr(void);
static void rvstrcpy(char *, char *);
static void savematch(int);
static void setbit(char, BITMAP);

/*
========================================
	文字列コピー（逆順）
========================================
*/

static void rvstrcpy(char *rvstr, char *str)
{
	int i;

	str += (i = strlen(str));
	while (i-- > 0)
		*rvstr++ = *--str;
	*rvstr = 0;
}

/*
========================================
	BITMAP 判定
========================================
*/

static int biteq(char bc, BITMAP cclmap)
{
	return (*(cclmap + (bc >> 3)) & BIT(bc & 7)) ? TRUE : FALSE;
}

/*
========================================
	BIT オン
========================================
*/

static void setbit(char bc, BITMAP cclmap)
{
	*(cclmap + (bc >> 3)) |= BIT(bc & 7);
}

/*
----------------------------------------
	順方向検索
----------------------------------------
*/

int forwsearch(int f, int n)
{
	int status;

	if (n < 0)
		return backsearch(f, -n);

	status = readpattern(KTEX78, pat, TRUE);
	if (status == TRUE) {
		if (!clexec && kbdmode != PLAY)
			mlwrite(KTEX224);
		do {
			status = (magical && (curwp->w_bufp->b_mode & MDMAGIC) != 0)
			  ? mcscanner(mcpat, FORWARD, PTEND)
			  : scanner(pat, FORWARD, PTEND);
		}
		while ((--n > 0) && status);
		if (!clexec && kbdmode != PLAY)
			mlwrite((status == TRUE) ? KTEX230 : KTEX79);
	}
	return status;
}

/*
----------------------------------------
	順方向次検索
----------------------------------------
*/

int forwhunt(int f, int n)
{
	int status;

	if (n < 0)
		return backhunt(f, -n);

	if (*pat == 0) {
		mlwrite(KTEX80);
		return FALSE;
	}
	if ((curwp->w_bufp->b_mode & MDMAGIC) != 0 && mcpat[0].mc_type == MCNIL) {
		if (!mcstr())
			return FALSE;
	}
	if (!clexec && kbdmode != PLAY)
		mlwrite(KTEX224);
	do {
		status = (magical && (curwp->w_bufp->b_mode & MDMAGIC) != 0)
		  ? mcscanner(mcpat, FORWARD, PTEND)
		  : scanner(pat, FORWARD, PTEND);
	}
	while ((--n > 0) && status);
	if (!clexec && kbdmode != PLAY)
		mlwrite((status == TRUE) ? KTEX230 : KTEX79);

	return status;
}

/*
----------------------------------------
	逆方向検索
----------------------------------------
*/

int backsearch(int f, int n)
{
	int status;

	if (n < 0)
		return forwsearch(f, -n);

	status = readpattern(KTEX81, pat, TRUE);
	if (status == TRUE) {
		if (!clexec && kbdmode != PLAY)
			mlwrite(KTEX224);
		do {
			status = (magical && (curwp->w_bufp->b_mode & MDMAGIC) != 0)
			  ? mcscanner(&tapcm[0], REVERSE, PTBEG)
			  : scanner(tap, REVERSE, PTBEG);
		}
		while ((--n > 0) && status);
		if (!clexec && kbdmode != PLAY)
			mlwrite((status == TRUE) ? KTEX230 : KTEX79);
	}
	return status;
}

/*
----------------------------------------
	逆方向次検索
----------------------------------------
*/

int backhunt(int f, int n)
{
	int status;

	if (n < 0)
		return forwhunt(f, -n);

	if (*tap == 0) {
		mlwrite(KTEX80);
		return FALSE;
	}
	if ((curwp->w_bufp->b_mode & MDMAGIC) != 0 && tapcm[0].mc_type == MCNIL) {
		if (!mcstr())
			return FALSE;
	}
	if (!clexec && kbdmode != PLAY)
		mlwrite(KTEX224);
	do {
		status = (magical && (curwp->w_bufp->b_mode & MDMAGIC) != 0)
		  ? mcscanner(&tapcm[0], REVERSE, PTBEG)
		  : scanner(tap, REVERSE, PTBEG);
	}
	while ((--n > 0) && status);
	if (!clexec && kbdmode != PLAY)
		mlwrite((status == TRUE) ? KTEX230 : KTEX79);

	return status;
}

/*
========================================
	meta パターン検索
========================================
*/

static int mcscanner(MC *mcpatrn, int direct, int beg_or_end)
{
	int curoff;
	LINE *curline;
	WINDOW *wp = curwp;

	beg_or_end ^= direct;
	mlenold = matchlen;
	curline = wp->w_dotp;
	curoff = wp->w_doto;

	while (!boundry(curline, curoff, direct)) {
		matchline = curline;
		matchoff = curoff;
		matchlen = 0;

		if (amatch(mcpatrn, direct, &curline, &curoff)) {
			if (beg_or_end == PTEND) {
				wp->w_dotp = curline;
				wp->w_doto = curoff;
			} else {
				wp->w_dotp = matchline;
				wp->w_doto = matchoff;
			}
			wp->w_flag |= WFMOVE;
			savematch(direct);
			return TRUE;
		}
		nextch(&curline, &curoff, direct);
	}
	return FALSE;
}

/*
========================================
	meta パターン検索 2
========================================
*/

static int amatch(MC *mcptr, int direct, LINE ** pcwline, int *pcwoff)
{
	int curoff;
	LINE *curline;

	curline = *pcwline;
	curoff = *pcwoff;

	if (mcptr->mc_type == BOL) {
		if (curoff != 0)
			return FALSE;
		mcptr++;
	}
	if (mcptr->mc_type == EOL) {
		if (curoff != llength(curline))
			return FALSE;
		mcptr++;
	}
	while (mcptr->mc_type != MCNIL) {
		int c;

		c = nextch(&curline, &curoff, direct);
		if (mcptr->mc_type & CLOSURE) {
			int nchars = 0;

			while (c != I_NEWLINE && mceq(c, mcptr)) {
				c = nextch(&curline, &curoff, direct);
				nchars++;
			}
			mcptr++;
			while (1) {
				c = nextch(&curline, &curoff, direct ^ REVERSE);
				if (amatch(mcptr, direct, &curline, &curoff)) {
					matchlen += nchars;
					goto success;
				}
				if (nchars-- == 0)
					return FALSE;
			}
		} else {
			if (mcptr->mc_type == BOL) {
				if (curoff == llength(curline)) {
					nextch(&curline, &curoff, direct ^ REVERSE);
					goto success;
				} else
					return FALSE;
			}
			if (mcptr->mc_type == EOL) {
				if (curoff == 0) {
					nextch(&curline, &curoff, direct ^ REVERSE);
					goto success;
				} else
					return FALSE;
			}
			if (!mceq(c, mcptr))
				return FALSE;
		}
		matchlen++;
		mcptr++;
	}

success:

	*pcwline = curline;
	*pcwoff = curoff;

	return TRUE;
}

/*
========================================
	検索ルーチン
========================================
*/

int scanner(char *patrn, int direct, int beg_or_end)
{
	int jump, curoff;
	LINE *curline;
	WINDOW *wp = curwp;

	beg_or_end ^= direct;
	curline = wp->w_dotp;
	curoff = wp->w_doto;
	jump = padd;

	while (!fbound(jump, &curline, &curoff, direct)) {
		int c, scanoff;
		char *patptr;
		LINE *scanline;

		matchline = curline;
		matchoff = curoff;
		scanline = curline;
		scanoff = curoff;
		patptr = patrn;

		while (c = *patptr++) {
			if (!eq(c, nextch(&scanline, &scanoff, direct)))
				goto fail;
		}
		if (direct == FORWARD) {
			if (nthctype(matchline->l_text, matchoff + 1) == CT_KJ2)
				goto fail;
		} else {
			if (nthctype(scanline->l_text, scanoff + 1) == CT_KJ2)
				goto fail;
		}
		if (beg_or_end == PTEND) {
			wp->w_dotp = scanline;
			wp->w_doto = scanoff;
		} else {
			wp->w_dotp = matchline;
			wp->w_doto = matchoff;
		}
		wp->w_flag |= WFMOVE;
		savematch(direct);
		return TRUE;

	fail:

		jump = (direct == FORWARD) ? lastf : lastb;
	}

	return FALSE;
}

/*
========================================
	終端チェック
========================================
*/

static int fbound(int jump, LINE **pcurline, int *pcuroff, int dir)
{
	int spare, curoff;
	LINE *curline;

	curline = *pcurline;
	curoff = *pcuroff;

	if (dir == FORWARD) {
		if (curline == curbp->b_linep)
			return TRUE;
		while (jump) {
			curoff += jump;
			spare = curoff - llength(curline);
			while (spare > 0) {
				curline = lforw(curline);
				curoff = spare - 1;
				spare = curoff - llength(curline);
				if (curline == curbp->b_linep)
					return TRUE;
			}
			jump = delf[(int) (spare ? lgetc(curline, curoff) : I_NEWLINE)];
		}
		curoff -= padd;
		while (curoff < 0) {
			curline = lback(curline);
			curoff += llength(curline) + 1;
		}
	} else {
		jump++;
		while (jump) {
			curoff -= jump;
			while (curoff < 0) {
				curline = lback(curline);
				curoff += llength(curline) + 1;
				if (curline == curbp->b_linep)
					return TRUE;
			}
			jump = delb[lgetc2(curline, curoff)];
		}
		curoff += matchlen;
		spare = curoff - llength(curline);
		while (spare > 0) {
			curline = lforw(curline);
			curoff = spare - 1;
			spare = curoff - llength(curline);
		}
	}

	*pcurline = curline;
	*pcuroff = curoff;

	return FALSE;
}

/*
========================================
	ジャンプテーブルの設定
========================================
*/

void setjtable(char *apat)
{
	int i;
	char kanji, check[NPAT + 20];

	rvstrcpy(tap, apat);
	mlenold = matchlen = strlen(apat);
	padd = matchlen - 1;

	for (i = 0; i < HICHAR; i++) {
		delf[i] = matchlen;
		delb[i] = matchlen;
	}

	for (kanji = ANK, i = 0; i <= padd; i++) {
		switch (kanji) {
		case ANK:
		case KANJI2:
			kanji = iskanji(apat[i]) ? KANJI1 : ANK;
			break;
		case KANJI1:
			kanji = KANJI2;
			break;
		}
		check[i] = kanji;
	}

	for (i = 0; i < padd; i++) {
		int jv = padd - i;

		if (isletter(apat[i]) && check[i] == ANK)
			delf[CHCASE(apat[i])] = jv;
		delf[apat[i]] = jv;
		if (isletter(tap[i]) && check[jv - 1] == ANK)
			delb[CHCASE(tap[i])] = jv;
		delb[tap[i]] = jv;
	}

	lastf = padd + delf[apat[padd]];
	lastb = padd + delb[apat[0]];

	if (isletter(apat[padd]) && check[padd] == ANK)
		delf[CHCASE(apat[padd])] = 0;
	delf[apat[padd]] = 0;
	if (isletter(apat[0]) && check[0] == ANK)
		delb[CHCASE(apat[0])] = 0;
	delb[apat[0]] = 0;
}

/*
========================================
	文字の比較
========================================
*/

int eq(char bc, char pc)
{
	if ((curwp->w_bufp->b_mode & MDEXACT) == 0) {
		if (islower(bc))
			bc = CHCASE(bc);
		if (islower(pc))
			pc = CHCASE(pc);
	}
	return bc == pc;
}

/*
========================================
	文字列の設定
========================================
*/

static int readpattern(char *prompt, char *apat, int srch)
{
	int status;
	char exp[NPAT + 20], tpat[NPAT + 20];

	expandp(apat, exp, NPAT / 2);
	sprintf(tpat, "%s [%s]%s: ", prompt, exp, termstr());

	his_enable(bhissearchp);
	status = mltreply(tpat, tpat, NPAT, sterm);
	if (status == TRUE) {
		strcpy(apat, tpat);
		if (srch)
			setjtable(apat);
		if ((curwp->w_bufp->b_mode & MDMAGIC) == 0) {
			mcclear();
			rmcclear();
		} else
			status = srch ? mcstr() : rmcstr();
	} else if (status == FALSE && *apat)
		status = TRUE;

	return status;
}

/*
========================================
	マッチング記録
========================================
*/

static void savematch(int direct)
{
	char *p;

	if (patmatch)
		free(patmatch);

	p = patmatch = (char *) malloc(matchlen + 1);
	if (p) {
		int j, curoff;
		LINE *curline;

		curoff = curwp->w_doto;
		curline = curwp->w_dotp;
		if (direct == FORWARD) {
			for (j = 0; j < matchlen; j++)
				p[matchlen - j - 1] = nextch(&curline, &curoff, REVERSE);
		} else {
			for (j = 0; j < matchlen; j++)
				p[j] = nextch(&curline, &curoff, FORWARD);
		}
		p[j] = 0;
	}
}

/*
----------------------------------------
	置換
----------------------------------------
*/

int sreplace(int f, int n)
{
	return replaces(FALSE, f, n);
}

/*
----------------------------------------
	確認置換
----------------------------------------
*/

int qreplace(int f, int n)
{
	return replaces(TRUE, f, n);
}

/*
========================================
	置換本体
========================================
*/

int replaces(int kind, int f, int n)
{
	int status;
	char tpat[NPAT];

	if (n < 0)
		return FALSE;

	status = readpattern(kind == FALSE ? KTEX84 : KTEX85, pat, TRUE);
	if (status != TRUE)
		return status;

	status = readpattern(KTEX86, rpat, FALSE);
	if (status == ABORT)
		return status;

	{
		int rlength, nlflag, nlrepl;
		int lastoff = 0;
		LINE *lastline = 0;

		rlength = strlen(rpat);
		nlflag = (pat[matchlen - 1] == I_NEWLINE);
		nlrepl = FALSE;

		if (kind) {
			char tmp1[NPAT + 20], tmp2[NPAT + 20];

			expandp(pat, tmp1, NPAT / 3);
			expandp(rpat, tmp2, NPAT / 3);
			sprintf(tpat, "%s%s%s%s'? ", KTEX87, tmp1, KTEX88, tmp2);

			lastline = 0;
			lastoff = 0;
		}

		{
			int numsub = 0, nummatch = 0;
			int origoff = curwp->w_doto;
			LINE *origline = curwp->w_dotp;

			while ((f == FALSE || n > nummatch) && (nlflag == FALSE || nlrepl == FALSE)) {
				status = (magical && (curwp->w_bufp->b_mode & MDMAGIC) != 0)
				  ? mcscanner(mcpat, FORWARD, PTBEG)
				  : scanner(pat, FORWARD, PTBEG);
				if (status == FALSE)
					break;

				nummatch++;
				nlrepl = (lforw(curwp->w_dotp) == curwp->w_bufp->b_linep);

				if (kind) {
					int c;

				pprompt:

					mlwrite(tpat, pat, rpat);

				qprompt:

					ena_zcursor = 1;
					update(TRUE);
					disp_cross_cur();
					ena_zcursor = 0;
					c = tgetc();
					mlerase();
					switch (tolower(c)) {
					case 'y':
					case ' ':
						break;
					case 'n':
						forwchar(FALSE, 1);
						continue;
					case '!':
						kind = FALSE;
						break;
					case 'u':
						if (lastline == 0) {
							H68beep();
							goto pprompt;
						}
						curwp->w_dotp = lastline;
						curwp->w_doto = lastoff;
						lastline = 0;
						lastoff = 0;
						backchar(FALSE, rlength);
						status = delins(rlength, patmatch, FALSE);
						if (status != TRUE)
							return status;
						numsub--;
						backchar(FALSE, mlenold);
						matchline = curwp->w_dotp;
						matchoff = curwp->w_doto;
						goto pprompt;
					case '.':
						curwp->w_dotp = origline;
						curwp->w_doto = origoff;
						curwp->w_flag |= WFMOVE;
					case BELL:
						mlwrite(KTEX89);
						return FALSE;
					default:
						H68beep();
					case '?':
						mlwrite(KTEX90);
						goto qprompt;
					}
				}
				if (curwp->w_dotp == origline) {
					origline = 0;
					lastline = lback(curwp->w_dotp);
				}
				status = delins(matchlen, &rpat[0], TRUE);
				if (origline == 0) {
					origline = lforw(lastline);
					origoff = 0;
				}
				if (status != TRUE)
					return status;
				if (kind) {
					lastline = curwp->w_dotp;
					lastoff = curwp->w_doto;
				} else if (matchlen == 0) {
					mlwrite(KTEX91);
					return FALSE;
				}
				numsub++;
			}
			mlwrite(KTEX92, numsub);
		}
	}

	if (!kind)
		lchange(WFHARD);

	return TRUE;
}

/*
========================================
	削除兼挿入
========================================
*/

static int delins(int dlength, char *instr, int use_meta)
{
	int status;
	WINDOW *wp = curwp;

	status = ldelete(dlength, FALSE);
	if (status != TRUE)
		mlwrite(KTEX93);
	else if (rmagical && use_meta && (wp->w_bufp->b_mode & MDMAGIC) != 0) {
		RMC *rmcptr = &rmcpat[0];

		while (rmcptr->mc_type != MCNIL && status == TRUE) {
			status = linstr((rmcptr->mc_type == LITCHAR) ? rmcptr->rstr : patmatch);
			rmcptr++;
		}
	} else
		status = linstr(instr);

	return status;
}

/*
========================================
	キーコードの展開
========================================
*/

int expandp(char *srcstr, char *deststr, int maxlength)
{
	char c;

	while (c = *srcstr++) {
		if (c == I_NEWLINE) {
			*deststr++ = '<';
			*deststr++ = 'N';
			*deststr++ = 'L';
			*deststr++ = '>';
			maxlength -= 4;
		} else if (c < 0x20 || c == 0x7f) {
			*deststr++ = '^';
			*deststr++ = c ^ 0x40;
			maxlength -= 2;
		} else if (c == '%') {
			*deststr++ = '%';
			*deststr++ = '%';
			maxlength -= 2;
		} else {
			*deststr++ = c;
			maxlength--;
		}

		if (maxlength < 4) {
			*deststr++ = '$';
			*deststr = 0;
			return FALSE;
		}
	}
	*deststr = 0;
	return TRUE;
}

/*
========================================
	境界チェック
========================================
*/

int boundry(LINE *curline, int curoff, int dir)
{
	return (dir == FORWARD)
	? (curline == curbp->b_linep)
	: (curoff == 0) && (lback(curline) == curbp->b_linep);
}

/*
========================================
	次の文字を得る
========================================
*/

static int nextch(LINE ** pcurline, int *pcuroff, int dir)
{
	int c, curoff;
	LINE *curline;

	curline = *pcurline;
	curoff = *pcuroff;

	if (dir == FORWARD) {
		if (curoff == llength(curline)) {
			curline = lforw(curline);
			curoff = 0;
			c = I_NEWLINE;
		} else
			c = lgetc(curline, curoff++);
	} else {
		if (curoff == 0) {
			curline = lback(curline);
			curoff = llength(curline);
			c = I_NEWLINE;
		} else
			c = lgetc(curline, --curoff);
	}

	*pcurline = curline;
	*pcuroff = curoff;

	return c;
}

/*
========================================
	配列設定
========================================
*/

static int mcstr(void)
{
	int status = TRUE;

	if (magical)
		mcclear();

	{
		int pchr, mj = 0;
		int does_closure = 0;
		char *patptr = pat;
		MC *mcptr = mcpat;

		while ((pchr = *patptr) && status) {
			switch (pchr) {
			case MC_CCL:
				status = cclmake(&patptr, mcptr);
				magical = TRUE;
				does_closure = TRUE;
				break;
			case MC_BOL:
				if (mj != 0)
					goto litcase;
				mcptr->mc_type = BOL;
				magical = TRUE;
				break;
			case MC_EOL:
				if (patptr[1])
					goto litcase;
				mcptr->mc_type = EOL;
				magical = TRUE;
				break;
			case MC_ANY:
				mcptr->mc_type = ANY;
				magical = TRUE;
				does_closure = TRUE;
				break;
			case MC_CLOSURE:
				if (!does_closure)
					goto litcase;
				mj--;
				mcptr--;
				mcptr->mc_type |= CLOSURE;
				magical = TRUE;
				does_closure = FALSE;
				break;
			case MC_ESC:
				if (patptr[1]) {
					pchr = *++patptr;
					magical = TRUE;
				}
			default:
				litcase:
				mcptr->mc_type = LITCHAR;
				mcptr->u.lchar = pchr;
				if (iskanji (pchr)) {
					mcptr++;
					patptr++;
					mj++;
					pchr = *patptr;
					if (pchr) {
						mcptr->mc_type = LITCHAR;
						mcptr->u.lchar = pchr;
					} else {
						mcptr--;
						patptr--;
						mj--;
					}
				}
				does_closure = (pchr != I_NEWLINE);
				break;
			}
			mcptr++;
			patptr++;
			mj++;
		}
		mcptr->mc_type = MCNIL;
		if (status) {
			MC *rtpcm = &tapcm[0];

			while (--mj >= 0)
				*rtpcm++ = *--mcptr;
			rtpcm->mc_type = MCNIL;
		} else {
			(--mcptr)->mc_type = MCNIL;
			mcclear();
		}
	}

	return status;
}

/*
========================================
	配列設定 2
========================================
*/

static int rmcstr(void)
{
	int mj, status = TRUE;
	char *patptr;
	RMC *rmcptr;

	patptr = rpat;
	rmcptr = &rmcpat[0];
	mj = 0;
	rmagical = FALSE;

	while (*patptr && status == TRUE) {
		switch (*patptr) {
		case MC_DITTO:
			if (mj != 0) {
				rmcptr->mc_type = LITCHAR;
				rmcptr->rstr = (char *) malloc(mj + 1);
				if (rmcptr->rstr == 0) {
					mlwrite(KTEX94);
					status = FALSE;
					break;
				}
				memcpy(rmcptr->rstr, patptr - mj, mj);
				rmcptr->rstr[mj] = 0;
				rmcptr++;
				mj = 0;
			}
			rmcptr->mc_type = DITTO;
			rmcptr++;
			rmagical = TRUE;
			break;
		case MC_ESC:
			rmcptr->mc_type = LITCHAR;
			rmcptr->rstr = (char *)malloc(mj + 2);
			if (rmcptr->rstr == 0) {
				mlwrite(KTEX94);
				status = FALSE;
				break;
			}
			memcpy(rmcptr->rstr, patptr - mj, mj + 1);
			rmcptr->rstr[mj + 1] = 0;
			if (patptr[1])
				rmcptr->rstr[mj] = *++patptr;
			rmcptr++;
			mj = 0;
			rmagical = TRUE;
			break;
		default:
			mj++;
		}
		patptr++;
	}

	if (rmagical && mj > 0) {
		rmcptr->mc_type = LITCHAR;
		rmcptr->rstr = (char *) malloc(mj + 1);
		if (rmcptr->rstr == 0) {
			mlwrite(KTEX94);
			status = FALSE;
		}
		memcpy(rmcptr->rstr, patptr - mj, mj);
		rmcptr->rstr[mj] = 0;
		rmcptr++;
	}
	rmcptr->mc_type = MCNIL;

	return status;
}

/*
========================================
	クリア
========================================
*/

void mcclear(void)
{
	MC *mcptr;

	mcptr = mcpat;
	while (mcptr->mc_type != MCNIL) {
		if ((mcptr->mc_type & MASKCL) == CCL || (mcptr->mc_type & MASKCL) == NCCL)
			free(mcptr->u.cclmap);
		mcptr++;
	}
	mcpat[0].mc_type = tapcm[0].mc_type = MCNIL;
	magical = FALSE;
}

/*
========================================
	クリア 2
========================================
*/

static void rmcclear(void)
{
	RMC *rmcptr;

	rmcptr = &rmcpat[0];
	while (rmcptr->mc_type != MCNIL) {
		if (rmcptr->mc_type == LITCHAR)
			free(rmcptr->rstr);
		rmcptr++;
	}
	rmcpat[0].mc_type = MCNIL;
}

/*
========================================
	meta 文字比較
========================================
*/

static int mceq(char bc, MC *mt)
{
	int result;

	switch (mt->mc_type & MASKCL) {
	case LITCHAR:
		result = eq(bc, mt->u.lchar);
		break;
	case ANY:
		result = (bc != I_NEWLINE);
		break;
	case CCL:
		result = biteq(bc, mt->u.cclmap);
		if (!result) {
			if ((curwp->w_bufp->b_mode & MDEXACT) == 0 && isletter(bc))
				result = biteq(CHCASE(bc), mt->u.cclmap);
		}
		break;
	case NCCL:
		result = !biteq(bc, mt->u.cclmap);
		if ((curwp->w_bufp->b_mode & MDEXACT) == 0 && isletter(bc))
			result &= !biteq(CHCASE(bc), mt->u.cclmap);
		break;
	default:
		mlwrite(KTEX95, mt->mc_type);
		result = FALSE;
		break;
	}

	return result;
}

/*
========================================
	BITMAP 作成
========================================
*/

static int cclmake(char **ppatptr, MC *mcptr)
{
	int pchr, ochr;
	char *patptr;
	BITMAP bmap;

	bmap = clearbits();
	if (bmap == 0) {
		mlwrite(KTEX94);
		return FALSE;
	}
	mcptr->u.cclmap = bmap;
	patptr = *ppatptr;
	if (*++patptr == MC_NCCL) {
		patptr++;
		mcptr->mc_type = NCCL;
	} else
		mcptr->mc_type = CCL;

	ochr = *patptr;
	if (ochr == MC_ECCL) {
		mlwrite(KTEX96);
		free(bmap);
		return FALSE;
	} else {
		if (ochr == MC_ESC)
			ochr = *++patptr;
		setbit(ochr, bmap);
		patptr++;
	}

	while (ochr && (pchr = *patptr, pchr != MC_ECCL)) {
		switch (pchr) {
		case MC_RCCL:
			if (*(patptr + 1) == MC_ECCL)
				setbit(pchr, bmap);
			else {
				pchr = *++patptr;
				while (++ochr <= pchr)
					setbit(ochr, bmap);
			}
			break;
		case MC_ESC:
			pchr = *++patptr;
		default:
			setbit(pchr, bmap);
			break;
		}
		patptr++;
		ochr = pchr;
	}

	*ppatptr = patptr;

	if (ochr == 0) {
		mlwrite(KTEX97);
		free(bmap);
		return FALSE;
	}
	return TRUE;
}

/*
========================================
	BITMAP クリア
========================================
*/

static BITMAP clearbits(void)
{
	BITMAP cclstart, cclmap;

	cclmap = cclstart = (BITMAP) malloc(HIBYTE);
	if (cclmap) {
		int j;

		for (j = 0; j < HIBYTE; j++)
			*cclmap++ = 0;
	}
	return cclstart;
}

/*
---------------------------------------
	終了文字
---------------------------------------
*/

char *termstr(void)
{
	static char cmd[NSTRING];

	cmdstr(sterm, cmd);
	if (*cmd == '^') {
		switch (cmd[1]) {
		case '[':
			return "<META>";
		case 'M':
			return ("<CR>");
		case 'J':
			return ("<NL>");
		case 'I':
			return ("<TAB>");
		}
	} else if (*cmd == 'F' && cmd[1] == 'N' && cmd[2] == '%')
		return ("<DEL>");

	return cmd;
}
