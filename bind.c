/*
----------------------------------------
	BIND.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/bind.c,v 1.7 1992/01/04 13:11:20 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int buildlist(int, int, char *);
static int getckey(int);
static int strinc(char *, char *);
static int unbindchar(int, int);

/*
========================================
	キータブを設定
========================================
*/

int setkey(KEYTAB *key, int type, char *name)
{
	key->k_type = type;

	if (type == BINDFNC) {
		NBIND *nb;

		nb = com_in_word_set(name);
		key->k_ptr.fp = nb->n_func;
		key->k_rest = nb->n_rest;
	}
	return TRUE;
}

/*
----------------------------------------
	ヘルプ呼出
----------------------------------------
*/

int help(int f, int n)
{
	BUFFER *bp;
	char *fname = 0;

	bp = bfind("emacs.hlp", FALSE, BFINVS);
	if (bp == 0) {
		fname = flook("emacs.hlp", FALSE);
		if (fname == 0) {
			mlwrite(KTEX12);
			return FALSE;
		}
	}
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;
	if (bp == 0) {
		if (getfile(fname) == FALSE)
			return FALSE;
	} else
		swbuffer(bp);
	curwp->w_bufp->b_mode |= MDVIEW;
	curwp->w_bufp->b_flag |= BFINVS;
	upmode();
	return TRUE;
}

/*
----------------------------------------
	指定キーの割当てコマンド表示
----------------------------------------
*/

int deskey(int f, int n)
{
	int c;
	char *ptr, outseq[NSTRING];

	mlwrite(KTEX13);
	c = getckey(FALSE);
	cmdstr(c, outseq);
	ostring(outseq);
	ochar(' ');
	ptr = getfname(getbind(c));
	ostring(ptr ? ptr : "Not Bound");
	oupdate();
	return TRUE;
}

/*
----------------------------------------
	ctrl-prefix の交換
----------------------------------------
*/

int exgctrlprefix(int f, int n)
{
	int key1, key2;
	char str1[NSTRING], str2[NSTRING];

	{
		int status;

		his_disable();
		status = mlreply(KTEX255, str1, NSTRING - 1);
		if (status == ABORT)
			return status;

		key1 = stock(str1);
		if ((key1 & 0xff) == 0)
			return FALSE;
		cmdstr(key1, str1);
		key1 = (key1 & 0xffff) << 16;

		his_disable();
		status = mlreply(KTEX256, str2, NSTRING - 1);
		if (status == ABORT)
			return status;

		key2 = stock(str2);
		if ((key2 & 0xff) == 0)
			return FALSE;
		cmdstr(key2, str2);
		key2 = (key2 & 0xffff) << 16;
	}

	n = f ? (n % NKEYMAPS) : curbp->b_keymap;

	{
		KEYTAB *ktp = keytab[n];

		for (; ktp->k_type != BINDNUL; ktp++) {
			int key;

			key = ktp->k_code;
			if ((key & 0xffff0000) == key1)
				ktp->k_code = key2 | (key & 0xffff);
			else if ((key & 0xffff0000) == key2)
				ktp->k_code = key1 | (key & 0xffff);
			if (ktp->k_type == BINDFNC && ktp->k_ptr.fp == cex) {
				if (key == key1 >> 16)
					ktp->k_code = key2 >> 16;
				else if (key == key2 >> 16)
					ktp->k_code = key1 >> 16;
			}
		}
	}

	mlwrite(KTEX257, str1, str2);

	return TRUE;
}

/*
----------------------------------------
	キーバインド
----------------------------------------
*/

int bindtokey(int f, int n)
{
	int c, krest;
	int (*kfunc)(int, int);
	char outseq[NSTRING];
	KEYTAB *ktp;

	n = (f == TRUE) ? (n % NKEYMAPS) : 0;

	{
		NBIND *nb;

		nb = getname2(KTEX15);
		if (nb == 0)
			goto noname;
		kfunc = nb->n_func;
		krest = nb->n_rest;
	}

	if (kfunc == 0) {
noname:
		mlwrite(KTEX16);
		return FALSE;
	}
	ochar(' ');
	oupdate();
	c = getckey((kfunc == meta) || (kfunc == cex) || (kfunc == unarg) || (kfunc == ctrlg));
	cmdstr(c, outseq);
	ostring(outseq);
	oupdate();
	ktp = keytab[n];

	if (kfunc == unarg || kfunc == ctrlg) {
		while (ktp->k_ptr.fp) {
			if (ktp->k_ptr.fp == kfunc)
				unbindchar(n, ktp->k_code);
			ktp++;
		}
		if (kfunc == unarg)
			reptc = c;
		else if (kfunc == ctrlg)
			abortc = c;
	} else if (kfunc == meta)
		sterm = c;

	{
		int found;

		for (found = FALSE; ktp->k_ptr.fp; ktp++) {
			if (ktp->k_code == c) {
				found = TRUE;
				break;
			}
		}
		if (found) {
			ktp->k_ptr.fp = kfunc;
			ktp->k_type = BINDFNC;
			ktp->k_rest = krest;
		} else {
			int bind_max;

			bind_max = bindtab[n].bd_size + ((n == 0) ? NFBINDS0 : NFBINDS1) - 1;
			if (ktp >= keytab[n] + bind_max) {
				mlwrite(KTEX17);
				return FALSE;
			}

			ktp->k_code = c;
			ktp->k_ptr.fp = kfunc;
			ktp->k_type = BINDFNC;
			ktp->k_rest = krest;
			ktp++;
			ktp->k_code = 0;
			ktp->k_type = BINDNUL;
			ktp->k_rest = 0;
			ktp->k_ptr.fp = 0;
		}
	}

	return TRUE;
}

/*
========================================
	メタキーバインド本体
========================================
*/

int bindtoxfkey(int no)
{
	char work[NSTRING], str[NSTRING];
	int status, key;

	sprintf(work, KTEX258, no);
	his_disable();
	status = mlreply(work, str, NSTRING - 1);
	if (status == ABORT)
		return status;

	key = stock(str) & 0xffffff00;
	switch (no) {
	case 1:
		xf1_key = key;
		break;
	case 2:
		xf2_key = key;
		break;
	case 3:
		xf3_key = key;
		break;
	case 4:
		xf4_key = key;
		break;
	case 5:
		xf5_key = key;
	}

	return TRUE;
}

/*
----------------------------------------
	bind-to-xf1-key
----------------------------------------
*/

int bindtoxf1key(int f, int n)
{
	return bindtoxfkey(1);
}

/*
----------------------------------------
	bind-to-xf2-key
----------------------------------------
*/

int bindtoxf2key(int f, int n)
{
	return bindtoxfkey(2);
}

/*
----------------------------------------
	bind-to-xf3-key
----------------------------------------
*/

int bindtoxf3key(int f, int n)
{
	return bindtoxfkey(3);
}

/*
----------------------------------------
	bind-to-xf4-key
----------------------------------------
*/

int bindtoxf4key(int f, int n)
{
	return bindtoxfkey(4);
}

/*
----------------------------------------
	bind-to-xf5-key
----------------------------------------
*/

int bindtoxf5key(int f, int n)
{
	return bindtoxfkey(5);
}

/*
----------------------------------------
	マクローキーバインド
----------------------------------------
*/

int macrotokey(int f, int n)
{
	char *mac, outseq[NSTRING], bufn[NBUFN];
	BUFFER *kmacro;
	KEYTAB *ktp;

	n = (f == TRUE) ? (n % NKEYMAPS) : 0;
	mac = complete(KTEX215, 0, 0, CMP_MACRO, NBUFN - 2);
	if (mac == 0)
		return FALSE;
	strcpy(bufn + 1, mac);
	strcpy(outseq, KTEX215);
	strcat(outseq, bufn + 1);
	*bufn = '[';
	strcat(bufn, "]");
	kmacro = bfind(bufn, FALSE, 0);
	if (kmacro == 0) {
		mlwrite(KTEX130);
		return FALSE;
	}
	strcat(outseq, " ");
	mlwrite(outseq);

	{
		int c;

		c = getckey(FALSE);
		cmdstr(c, outseq);
		ostring(outseq);
		oupdate();
		ktp = keytab[n];

		{
			int found;

			for (found = FALSE; ktp->k_type != BINDNUL; ktp++) {
				if (ktp->k_code == c) {
					found = TRUE;
					break;
				}
			}
			if (found) {
				ktp->k_ptr.buf = kmacro;
				ktp->k_type = BINDBUF;
				ktp->k_rest = 0;
			} else {
				int bind_max;

				bind_max = bindtab[n].bd_size + ((n == 0) ? NFBINDS0 : NFBINDS1) - 1;
				if (ktp >= keytab[n] + bind_max) {
					mlwrite(KTEX17);
					return FALSE;
				}

				ktp->k_code = c;
				ktp->k_ptr.buf = kmacro;
				ktp->k_type = BINDBUF;
				ktp->k_rest = 0;
				ktp++;
				ktp->k_code = 0;
				ktp->k_type = BINDNUL;
				ktp->k_ptr.fp = 0;
				ktp->k_rest = 0;
			}
		}
	}

	return (TRUE);
}

/*
----------------------------------------
	キーバインド解除
----------------------------------------
*/

int unbindkey(int f, int n)
{
	int c;
	char outseq[80];

	n = (f == TRUE) ? (n % NKEYMAPS) : 0;
	mlwrite(KTEX18);
	c = getckey(FALSE);
	cmdstr(c, outseq);
	ostring(outseq);
	oupdate();
	if (unbindchar(n, c) == FALSE) {
		mlwrite(KTEX19);
		return FALSE;
	}
	return TRUE;
}

/*
========================================
	キーバインド解除本体
========================================
*/

static int unbindchar(int n, int c)
{
	int found;
	KEYTAB *ktp, *sktp;

	ktp = keytab[n];
	for (found = FALSE; ktp->k_type != BINDNUL; ktp++) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
	}
	if (!found)
		return FALSE;
	for (sktp = ktp; ktp->k_ptr.fp; ktp++);
	ktp--;
	sktp->k_code = ktp->k_code;
	sktp->k_type = ktp->k_type;
	sktp->k_ptr.fp = ktp->k_ptr.fp;
	sktp->k_rest = ktp->k_rest;
	ktp->k_code = 0;
	ktp->k_type = BINDNUL;
	ktp->k_ptr.fp = 0;
	ktp->k_rest = 0;
	return TRUE;
}

/*
----------------------------------------
	キーバインド表を表示
----------------------------------------
*/

int desbind(int f, int n)
{
	n = (f == TRUE) ? ((n >= 0) ? (n % NKEYMAPS) : 0) : -1;
	return buildlist(n, TRUE, "");
}

/*
----------------------------------------
	適合フンクションの表を表示
----------------------------------------
*/

int apro(int f, int n)
{
	int status;
	char mstring[NSTRING];

	his_disable();
	status = mlreply(KTEX20, mstring, NSTRING - 1);
	if (status == ABORT)
		return status;
	n = (f == TRUE) ? ((n >= 0) ? (n % NKEYMAPS) : 0) : -1;
	return buildlist(n, FALSE, mstring);
}

/*
========================================
	リスト表示本体
========================================
*/

static int buildlist(int n, int type, char *mstring)
{
	char outseq[NSTRING];
	int b_keymap;

	b_keymap = curbp->b_keymap;

	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	{
		BUFFER *bp;

		bp = bfind(KTEX21, TRUE, 0);
		if (bp == 0 || bclear(bp) == FALSE) {
			mlwrite(KTEX22);
			return FALSE;
		}

		mlwrite(KTEX23);

		bp->b_mode = 0;
		swbuffer(bp);
	}

	if (n != 0) {
		int mapn;

		mapn = (n == -1 ? b_keymap : n);
		if (mapn != 0) {
			KEYTAB *ktp;

			sprintf(outseq, KTEX268, mapn);
			if (putline(outseq) != TRUE)
				return FALSE;
			ktp = keytab[mapn];
			for (; ktp->k_type != BINDNUL; ktp++) {
				int cpos;
				NBIND *nptr;

				switch (ktp->k_type) {
				case BINDFNC:
					for (nptr = command_table; nptr->n_func; nptr++) {
						if (nptr->n_func == ktp->k_ptr.fp)
							break;
					}
					if (nptr->n_func)
						strcpy(outseq, nptr->n_name);
					break;
				case BINDBUF:
					strcpy(outseq, ktp->k_ptr.buf->b_bname);
					break;
				}
				cpos = strlen(outseq);
				while (cpos < 33)
					outseq[cpos++] = ' ';
				cmdstr(ktp->k_code, outseq + cpos);
				if (putline(outseq) != TRUE)
					return FALSE;
			}
			lnewline();
		}
	}
	if (n <= 0) {
		NBIND *nptr;

		if (putline(KTEX269) != TRUE)
			return FALSE;
		for (nptr = command_table; nptr->n_func; nptr++) {
			int cpos;
			KEYTAB *ktp;

			strcpy(outseq, nptr->n_name);
			cpos = strlen(outseq);
			if (type == FALSE && strinc(outseq, mstring) == FALSE)
				continue;
			for (ktp = keytab[0]; ktp->k_type != BINDNUL; ktp++) {
				if (ktp->k_type == BINDFNC && ktp->k_ptr.fp == nptr->n_func) {
					while (cpos < 33)
						outseq[cpos++] = ' ';
					cmdstr(ktp->k_code, outseq + cpos);
					if (putline(outseq) != TRUE)
						return FALSE;
					cpos = 0;
				}
			}
			if (cpos > 0) {
				outseq[cpos] = 0;
				if (putline(outseq) != TRUE)
					return FALSE;
			}
		}
		lnewline();

		{
			BUFFER *bp;

			for (bp = bheadp; bp; bp = bp->b_bufp) {
				int cpos;
				KEYTAB *ktp;

				if (*bp->b_bname != '[')
					continue;
				strcpy(outseq, bp->b_bname);
				cpos = strlen(outseq);
				if (type == FALSE && strinc(outseq, mstring) == FALSE)
					continue;
				for (ktp = keytab[0]; ktp->k_ptr.fp; ktp++) {
					if (ktp->k_type == BINDBUF && ktp->k_ptr.buf == bp) {
						while (cpos < 33)
							outseq[cpos++] = ' ';
						cmdstr(ktp->k_code, outseq + cpos);
						if (putline(outseq) != TRUE)
							return FALSE;
						cpos = 0;
					}
				}
				if (cpos > 0) {
					outseq[cpos] = 0;
					if (putline(outseq) != TRUE)
						return FALSE;
				}
			}
		}
	}

	winbob(curbp);

	curbp->b_flag &= ~BFCHG;
	curwp->w_bufp->b_mode |= MDVIEW;
	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	upmode();
	mlerase();
	return TRUE;
}

/*
========================================
	文字列を含むかチェック
========================================
*/

static int strinc(char *source, char *sub)
{
	char *sp;

	for (sp = source; *sp; sp++) {
		char *tp, *nextsp;

		for (tp = sub, nextsp = sp; *tp; tp++) {
			if (*nextsp++ != *tp)
				break;
		}
		if (*tp == 0)
			return TRUE;
	}
	return FALSE;
}

/*
========================================
	キーシーケンスを得る
========================================
*/

static int getckey(int mflag)
{
	char tok[NSTRING];

	if (clexec) {
		macarg(tok);
		return stock(tok);
	}
	return mflag ? getkey() : getcmd();
}

/*
========================================
	スタートアップ
========================================
*/

int startup(char *sfname)
{
	char *fname;

	fname = flook(*sfname ? sfname : "emacs.rc", TRUE);
	if (fname) {
		dofile(fname);
		return TRUE;
	}
	return FALSE;
}

/*
========================================
	ファイル検索
========================================
*/

char *flook(char *fname, int hflag)
{
	static char fspec[NFILEN];

	if (ffropen(fname) == FIOSUC) {
		ffclose();
		return (fname);
	}

#if 0
	{
		char *sp;

		for (sp = fname; *sp; sp++) {
			if (*sp == ':' || sep(*sp) == slash) {
				if (ffropen(sp + 1) == FIOSUC) {
					ffclose();
					return fname;
				} else
					return 0;
			}
		}
	}
#endif

	{
		char *home;

		if (hflag)
			home = getenv("HOME");
		else {
			strcpy(fspec, help_load_path);
			add_slash_if(fspec);
			strcat(fspec, fname);
			if (fexist(fspec))
				return fspec;
			home = getenv("HELP");
		}
		if (home) {
			strcpy(fspec, home);
			add_slash_if(fspec);
			strcat(fspec, fname);

			if (ffropen(fspec) == FIOSUC) {
				ffclose();
				return fspec;
			}
		}
	}

	{
		char *path;

		path = getenv("path");
		if (path) {
			while (*path) {
				char *sp;

				for (sp = fspec; *path && (*path != ';' && *path != ' '); *sp++ = *path++)
					;
				if (sep(sp[-1]) != slash)
					*sp++ = slash;
				*sp = 0;
				strcat(fspec, fname);

				if (fexist(fspec))
					return fspec;
				if (*path && (*path == ';' || *path == ' '))
					path++;
			}
		}
	}

	return 0;
}

/*
========================================
	キーコードを文字列に変換
========================================
*/

void cmdstr(int c, char *ptr)
{
	if (c >= 0x10000)
		c = ((c & 0xffff) << 16) | ((c & 0xffff0000) >> 16);

	while (c) {
		if (c & ALTD) {
			*ptr++ = 'A';
			*ptr++ = '-';
		}
		if (c & SHFT) {
			*ptr++ = 'S';
			*ptr++ = '-';
		}
		if (c & MOUS) {
			*ptr++ = 'M';
			*ptr++ = 'S';
		}
		if (c & META) {
			*ptr++ = 'M';
			*ptr++ = '-';
		}
		if (c & SPEC) {
			*ptr++ = 'F';
			*ptr++ = 'N';
		}
		if (c & CTRL) {
			*ptr++ = '^';
		}
		*ptr++ = c & CHARMASK;
		c = (c >> 16) & WORDMASK;
	}

	*ptr = 0;
}

/*
========================================
	キーコードを文字列に変換２
========================================
*/

void cmdstr2(int c, char *ptr)
{
	if (c >= 0x10000)
		c = ((c & 0xffff) << 16) | ((c & 0xffff0000) >> 16);

	while (c) {
		if (c & ALTD) {
			*ptr++ = 'A';
			*ptr++ = 'L';
			*ptr++ = 'T';
			*ptr++ = '-';
		}
		if (c & SHFT) {
			*ptr++ = 'S';
			*ptr++ = 'H';
			*ptr++ = 'I';
			*ptr++ = 'F';
			*ptr++ = 'T';
			*ptr++ = '-';
		}
		if (c & MOUS) {
			*ptr++ = 'M';
			*ptr++ = 'O';
			*ptr++ = 'U';
			*ptr++ = 'S';
			*ptr++ = 'E';
		}
		if (c & META) {
			*ptr++ = 'M';
			*ptr++ = 'E';
			*ptr++ = 'T';
			*ptr++ = 'A';
			*ptr++ = '-';
		}
		if (c & SPEC) {
			*ptr++ = 'F';
			*ptr++ = 'N';
			*ptr++ = 'C';
		}
		if (c & CTRL) {
			*ptr++ = 'C';
			*ptr++ = 'T';
			*ptr++ = 'R';
			*ptr++ = 'L';
			*ptr++ = '-';
		}
		*ptr++ = c & CHARMASK;
		c = (c >> 16) & WORDMASK;
	}

	*ptr = 0;
}

/*
========================================
	キーバインドを探す
========================================
*/

KEYTAB *getbind(int c)
{
	KEYTAB *ktp;

	ktp = keytab[curbp->b_keymap];
	for (; ktp->k_type != BINDNUL; ktp++) {
		if (ktp->k_code == c)
			return ktp;
	}
	if (curbp->b_keymap == 0 || noglobal)
		return 0;

	for (ktp = keytab[0]; ktp->k_type != BINDNUL; ktp++) {
		if (ktp->k_code == c)
			return ktp;
	}
	return 0;
}

/*
========================================
	コマンドの名前を得る
========================================
*/

char *getfname(KEYTAB *key)
{
	if (key == 0)
		return 0;

	if (key->k_type == BINDFNC) {
		int (*func)(int, int);
		NBIND *nptr;

		func = key->k_ptr.fp;
		for (nptr = command_table; nptr->n_func; nptr++) {
			if (nptr->n_func == func)
				return nptr->n_name;
		}
	} else {
		BUFFER *kbuf, *bp;

		kbuf = key->k_ptr.buf;
		for (bp = bheadp; bp; bp = bp->b_bufp) {
			if (bp == kbuf)
				return bp->b_bname;
		}
	}
	return 0;
}

/*
========================================
	文字列からキーコードに変換
========================================
*/

int stock(char *keyname)
{
	int i, c;

	for (c = 0, i = 0; *keyname && i < 2; i++) {
		c <<= 16;

		if (*keyname == 'A' && keyname[1] == '-') {
			c |= ALTD;
			keyname += 2;
		}
		if (*keyname == 'S' && keyname[1] == '-') {
			c |= SHFT;
			keyname += 2;
		}
		if (*keyname == 'M' && keyname[1] == 'S') {
			c |= MOUS;
			keyname += 2;
		}
		if (*keyname == 'M' && keyname[1] == '-') {
			c |= META;
			keyname += 2;
		}
		if (*keyname == 'F' && keyname[1] == 'N') {
			c |= SPEC;
			keyname += 2;
		}
		if (*keyname == '^' && keyname[1] != 0) {
			c |= CTRL;
			keyname++;
			if (islower(*keyname))
				*keyname -= DIFCASE;
		}
		if (*keyname) {
			c |= (*keyname < 32) ? (CTRL | (*keyname + '@')) : *keyname;
			keyname++;
		}
	}

	return c;
}

/*
========================================
	文字列からコマンドを得る
========================================
*/

char *transbind(char *skey)
{
	char *bindname;

	bindname = getfname(getbind(stock(skey)));
	return bindname ? bindname : (char *) errorm;
}

/*
========================================
	キーによりコマンド実行
========================================
*/

int execkey(KEYTAB *key, int f, int n)
{
	cmdarg = n;
	if (key->k_type == BINDFNC) {
		if ((key->k_rest & QREST) && restflag)
			return resterr();
		if ((key->k_rest & QVIEW) && (curbp->b_mode & (MDVIEW | MDDIRED)))
			return rdonly();
		if ((key->k_rest & QDIRED) && (curbp->b_mode & MDDIRED) == 0)
			return bad_mode();
		if ((key->k_rest & QMINUS) && n < 0)
			return FALSE;
		if ((key->k_rest & QZMIN) && n <= 0)
			return FALSE;
		if ((key->k_rest & QZERO) && n == 0)
			return TRUE;

		if (key->k_rest & QKILL) {
			if ((lastflag & CFKILL) == 0)
				kdelete();
			thisflag |= CFKILL;
		}
		return (*(key->k_ptr.fp))(f, n);
	} else if (key->k_type == BINDBUF) {
		int savearg, status;

		if (key->k_ptr.buf->b_active == FALSE) {
			char macro[NBUFN];
			int ecode;

			strcpy(macro, key->k_ptr.buf->b_bname + 1);
			macro[strlen(macro) - 1] = 0;
			status = loadexec(macro, &ecode);
			if (ecode < 0)
				mlwrite(KTEX116);
			if (status != TRUE)
				return status;
			if (key->k_ptr.buf->b_active == FALSE) {
				mlwrite(KTEX130);
				return FALSE;
			}
		}

		status = TRUE;
		savearg = cmdarg;
		if (cmdarg > 0) {
			for(savearg = cmdarg; cmdarg > 0; cmdarg--) {
				status = dobuf(key->k_ptr.buf);
				if (status != TRUE)
					break;
			}
		} else if (cmdarg > 0) {
			for(savearg = cmdarg; cmdarg < 0; cmdarg++) {
				status = dobuf(key->k_ptr.buf);
				if (status != TRUE)
					break;
			}
		}
		cmdarg = savearg;
		return status;
	}
	return TRUE;
}

/*
========================================
	コマンド実行
========================================
*/

int execnbind(NBIND *nb, int f, int n)
{
	if ((nb->n_rest & QREST) && restflag)
		return resterr();
	if ((nb->n_rest & QVIEW) && (curbp->b_mode & (MDVIEW | MDDIRED)))
		return rdonly();
	if ((nb->n_rest & QDIRED) && (curbp->b_mode & MDDIRED) == 0)
		return bad_mode();
	if ((nb->n_rest & QMINUS) && n < 0)
		return FALSE;
	if ((nb->n_rest & QZMIN) && n <= 0)
		return FALSE;
	if ((nb->n_rest & QZERO) && n == 0)
		return TRUE;
	if (nb->n_rest & QKILL) {
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}
	return (*(nb->n_func))(f, n);
}
