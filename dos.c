/*
----------------------------------------
	DOS.C: MicroEMACS 3.10
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
#include "ehprint.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/dos.c,v 1.5 1992/01/26 13:26:48 SALT Exp SALT $'\n""	even\n");

/*
----------------------------------------
	定数宣言
----------------------------------------
*/

#define	INS_KEY		0x0100

/*
========================================
	使用関数の定義
========================================
*/

static int wcom_do(int);
static void wcom_putc(int);
static void wcom_flush_buf(void);
static void wcom_doesc(void);
static void wcom_lover(char *);
static void shellprog(char *);
static int xstrcmp(char *, char *);

/*
========================================
	使用変数の定義
========================================
*/

#define WCOM_FLUSH_RATE 256

static char path[NFILEN], rbuf[NFILEN];
static struct FILBUF fileblock;
static int wcom_flush_buf_size, wcom_curpos;
static char wcom_escbuf[32], wcom_escbuf_pos;

/*
========================================
	ケース無視の比較
========================================
*/

static int xstrcmp(char *a, char *b)
{
	int p, q;

	while (1) {
		p = *a++;
		q = *b++;
		if (!p || !q)
			break;
		if (isupper(p))
			p += DIFCASE;
		if (isupper(q))
			q += DIFCASE;
		if (p != q)
			break;
	}
	return p - q;
}

/*
========================================
	シェル起動
========================================
*/

static void shellprog(char *cmd)
{
	int i, old_color[16];

	f_set_shell();
	for (i = 0; i < 16; i++)
		old_color[i] = TPALET2(i, -1);
	MS_CUROF();
	TXcurof();
	H68color(3);
	H68clear();
	H_CURINT(0);

	rval = system(cmd);
	updinsf = TRUE;

	H_CURINT(1);
	TXcuron();
	if (mouseflag)
		MS_CURON();
	SKEY_MOD(0, 0, 0);
	for (i = 0; i < 16; i++)
		TPALET2(i, old_color[i]);
	f_set_emacs();
}

/*
========================================
	コマンド実行
========================================
*/

int execprog(char *cmd, int type)
{
	static char comline[NSTRING * 2];
	static char *nargv[MAXARG];
	int c, result;
	char *p;

	strcpy(p = comline, cmd);
	while (isspace(*p))
		p++;
	nargv[0] = p;
	while (!isspace(c = *p++)) {
		if (!c)
			break;
		if (iskanji(c))
			p++;
		else if (c == '/')
			p[-1] = '\\';
	}
	if (c) {
		p[-1] = 0;
		nargv[1] = p;
		nargv[2] = 0;
	} else
		nargv[1] = 0;

	if (type)
		disable_print();

	if (!type)
		f_set_shell ();
	result = spawnvp(P_WAIT, nargv[0], nargv);
	if (!type)
		f_set_emacs ();

	if (result == 0) {
		rval = wait();
		result = rval < 0 ? FALSE : TRUE;
	} else {
		mlwrite(KTEX3);
		rval = -1;
		result = FALSE;
	}

	if (type)
		enable_print();

	return result;
}

/*
----------------------------------------
	spawn ルーチン
----------------------------------------
*/

int spawncli(int f, int n)
{
	mlmovecursor(0);

	shellprog("");

	sgarbf = TRUE;
	return TRUE;
}

/*
----------------------------------------
	spawn ルーチン 2
----------------------------------------
*/

int spawn(int f, int n)
{
	int status;
	char line[NLINE];

	his_enable(bhisexecp);
	status = mlreply("! ", line, NLINE);
	if (status != TRUE)
		return status;
	mlmovecursor(0);

	shellprog(line);

	if (clexec == FALSE) {
		mlwrite(KTEX6);
		TTgetc();
	}
	sgarbf = TRUE;
	return TRUE;
}

/*
----------------------------------------
	コマンド実行 2
----------------------------------------
*/

int execprg(int f, int n)
{
	int status;
	char line[NLINE];

	his_enable(bhisexecp);
	status = mlreply("$ ", line, NLINE);
	if (status != TRUE)
		return status;
	mlmovecursor(0);
	H_CURINT(0);

	execprog(line, 0);

	H_CURINT(1);
	if (clexec == FALSE) {
		mlwrite(KTEX6);
		TTgetc();
	}
	sgarbf = TRUE;
	return TRUE;
}

/*
----------------------------------------
	コマンド実行 3
----------------------------------------
*/

int silentexecprg(int f, int n)
{
	int status;
	char line[NLINE];

	his_enable(bhisexecp);
	status = mlreply("& ", line, NLINE);
	if (status != TRUE)
		return status;
	mlmovecursor(0);

	execprog(line, 1);

	return TRUE;
}

/*
----------------------------------------
	window command (SALT)
----------------------------------------
*/

int wcom(int f, int n)
{
	return wcom_do(TRUE);
}

/*
----------------------------------------
	window shell (SALT)
----------------------------------------
*/

int wshell(int f, int n)
{
	return wcom_do(FALSE);
}

/*
----------------------------------------
	do window command (SALT)
----------------------------------------
*/

static int wcom_do(int mode)
{
	char line[NLINE];

	if (mode == TRUE) {
		int status;

		his_enable(bhisexecp);
		status = mlreply("%% ", line, NLINE);
		if (status != TRUE)
			return status;
	} else
		*line = 0;

	forwline(FALSE, 1);
	update(TRUE);
	gotoeob(FALSE, 1);
	update(TRUE);

	{
		int bufsize;

		bufsize = (shbufsize < 32) ? 32 : shbufsize;
		if (moreheap(bufsize * 1024) < 0)
			return FALSE;
	}

	ena_zcursor = 1;
	wcom_curpos = 0;
	wcom_escbuf_pos = 0;
	wcom_flush_buf_size = 0;
	f_set_shell();
	intercept(wcom_putc, wcom_flush_buf);

	rval = system(line);
	updinsf = TRUE;

	intercept_end();
	issuper = 0;
	f_set_emacs();
	wcom_flush_buf();
	ena_zcursor = 0;

	return TRUE;
}

static void wcom_putc(int ch)
{
	char work[4];

	disable_int();

	if (wcom_escbuf_pos) {
		wcom_escbuf[wcom_escbuf_pos++] = ch;
		if (isalpha(ch)) {
			wcom_escbuf[wcom_escbuf_pos] = '\0';
			wcom_escbuf_pos = 0;
			wcom_doesc();
			update(TRUE);
			wcom_flush_buf_size = 0;
		}
		enable_int();
		return;
	}
	if (wcom_curpos) {
		int pos;

		if (ch == 0x0d)
			wcom_curpos = 1;
		if (llength(curwp->w_dotp) == 0) {
			wcom_lover(" ");
			backdel(FALSE, 1);
		}
		for (pos = 1; pos < wcom_curpos; pos++)
			wcom_lover(" ");
		update(TRUE);
		wcom_flush_buf_size = 0;
		wcom_curpos = 0;
	}
	if (ch < 0x100) {
		if (ch < 0x20) {
			switch (ch) {
			case 0x08:
				backchar(FALSE, 1);	/* BS */
				wcom_flush_buf_size++;
				break;
			case 0x09:
				work[0] = ch;	/* TAB */
				work[1] = 0;
				wcom_lover(work);
				wcom_flush_buf_size++;
				break;
			case 0x0a:
				wcom_curpos = curwp->w_doto + 1;
				update(TRUE);
				forwline(FALSE, 1);	/* LF */
				update(TRUE);
				wcom_flush_buf_size = 0;
				break;
			case 0x0c:
				forwchar(FALSE, 1);	/* FF */
				wcom_flush_buf_size++;
				break;
			case 0x0d:
				update(TRUE);
				gotobol(FALSE, 1);	/* CR */
				wcom_flush_buf_size = 1;
				break;
			case 0x1b:
				wcom_flush_buf();
				wcom_escbuf[wcom_escbuf_pos++] = ch;
			}
		} else {
			wcom_flush_buf_size++;
			work[0] = ch;
			work[1] = '\0';
			wcom_lover(work);
		}
	} else {
		wcom_flush_buf_size += 2;
		work[0] = ch >> 8;
		work[1] = ch & CHARMASK;
		work[2] = 0;
		wcom_lover(work);
	}

	if (wcom_flush_buf_size >= WCOM_FLUSH_RATE)
		wcom_flush_buf();

	enable_int();
}

static void wcom_lover(char *ostr)
{
	if (*ostr == ' ' && check_wrap())
		exechook(wraphook);
	lover(ostr);
}

static void wcom_flush_buf(void)
{
	if (wcom_curpos) {
		int pos = wcom_curpos;

		while (pos--)
			lowrite(' ');
		backchar(FALSE, 1);	/* BS */
		update(TRUE);
		wcom_curpos = 0;
		wcom_flush_buf_size = 0;
	} else {
		if (wcom_flush_buf_size && !wcom_escbuf_pos) {
			update(TRUE);
			wcom_flush_buf_size = 0;
		}
	}
}

static void wcom_doesc(void)
{
	int i = 1;

	switch (wcom_escbuf[i]) {
	case '[':
		{
			int ch, num;

			for (num = 0, i = 2; ch = wcom_escbuf[i], isdigit(ch); i++)
				num = num * 10 + ch - '0';
			switch (wcom_escbuf[i]) {
			case 'A':
				if (num == 0)
					num = 1;
				backline(FALSE, num);
				break;
			case 'B':
				if (num == 0)
					num = 1;
				forwline(FALSE, num);
				break;
			case 'C':
				if (num == 0)
					num = 1;
				forwchar(FALSE, num);
				break;
			case 'D':
				if (num == 0)
					num = 1;
				backchar(FALSE, num);
				break;
			case 'J':
				if (num == 2) {
					gotobol(FALSE, 1);
					{
						int nokill_old = nokill;

						nokill = 1;
						killtext(FALSE, 1);
						nokill = nokill_old;
					}
					wcom_flush_buf_size++;
				}
				break;
			case 'K':
				{
					int col;

					col = getccol(FALSE);
					switch (num) {
					case 0:
						{
							int nokill_old = nokill;

							nokill = 1;
							killtext(FALSE, 1);
							nokill = nokill_old;
						}
						wcom_flush_buf_size++;
						break;
					case 1:
						wcom_curpos = getccol(FALSE) + 1;
						gotobol(FALSE, 1);
						while (wcom_curpos--)
							wcom_lover(" ");
						backchar(FALSE, 1);	/* BS */
						update(TRUE);
						wcom_flush_buf_size = 0;
						break;
					case 2:
						update(TRUE);
						killtext(FALSE, 1);
						update(TRUE);
						wcom_flush_buf_size = 0;
					}
				}
				break;
			default:
				break;
			}
		}
	default:
		break;
	}
}

/*
----------------------------------------
	パイプ処理
----------------------------------------
*/

int pipecmd(int f, int n)
{
	char line[NLINE];
	static char filnam[NSTRING];

	{
		char *tmp;

		tmp = getenv("temp");
		if (tmp) {
			strcpy(filnam, tmp);
			add_slash_if(filnam);
		} else
			*filnam = 0;
		cv_bslash_slash(filnam);

		strcat(filnam, "shell.___");
	}

	{
		int status;

		his_enable(bhisexecp);
		status = mlreply("@ ", line, NLINE);
		if (status != TRUE)
			return status;
	}

	{
		BUFFER *bp;

		bp = bfind("*pipe*", FALSE, 0);
		if (bp) {
			WINDOW *wp;

			for (wp = wheadp; wp; wp = wp->w_wndp) {
				if (wp->w_bufp == bp) {
					onlywind(FALSE, 1);
					break;
				}
			}
			if (zotbuf(bp) != TRUE)
				return (FALSE);
		}
	}

	strcat(line, "> ");
	strcat(line, filnam);
	mlmovecursor(0);

	rval = system(line);
	updinsf = TRUE;

	{
		FILE *fp;

		fp = fopen(filnam, "r");
		if (fp == 0)
			return FALSE;
		fclose(fp);
	}

	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;
	if (getfile(filnam) == FALSE)
		return FALSE;
	strcpy(curbp->b_bname, "*pipe*");
	curwp->w_bufp->b_mode |= MDVIEW;
	*curwp->w_bufp->b_fname = 0;

	{
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp)
			wp->w_flag |= WFMODE;
	}

	DELETE(filnam);
	return TRUE;
}

/*
----------------------------------------
	フィルター処理
----------------------------------------
*/

int filter(int f, int n)
{
	char line[NLINE];
	char tmpline[NLINE], tmpnam[NFILEN];
	static char filnam1[NLINE];
	static char filnam2[NLINE];

	{
		char *tmp;

		tmp = getenv("temp");
		if (tmp == 0)
			*filnam1 = *filnam2 = 0;
		else {
			strcpy(filnam1, tmp);
			strcpy(filnam2, tmp);
			add_slash_if(filnam1);
			add_slash_if(filnam2);
		}
		cv_bslash_slash(filnam1);
		cv_bslash_slash(filnam2);
		strcat(filnam1, "fltinp");
		strcat(filnam2, "fltout");
	}

	{
		int status;

		his_enable(bhisexecp);
		status = mlreply("# ", line, NLINE);
		if (status != TRUE)
			return status;
	}

	{
		BUFFER *bp = curbp;

		strcpy(tmpnam, bp->b_fname);
		strcpy(bp->b_fname, "fltinp");
		if (writeout(filnam1) != TRUE) {
			mlwrite(KTEX2);
			strcpy(bp->b_fname, tmpnam);
			return FALSE;
		}
		sprintf(tmpline, " < %s > %s", filnam1, filnam2);
		strcat(line, tmpline);
		mlmovecursor(0);

		rval = system(line);
		updinsf = TRUE;

		if (readin(FALSE, 0, filnam2) == FALSE) {
			mlwrite(KTEX3);
			strcpy(bp->b_fname, tmpnam);
			DELETE(filnam1);
			DELETE(filnam2);
			return FALSE;
		}
		strcpy(bp->b_fname, tmpnam);
		bp->b_flag |= BFCHG;
	}

	DELETE(filnam1);
	DELETE(filnam2);
	return TRUE;
}

/*
========================================
	時間取得
========================================
*/

char *timeset(void)
{
	char *sp;
	time_t tbuf;

	time(&tbuf);
	sp = ctime(&tbuf);
	sp[24] = 0;
	return sp;
}

/*
========================================
	ファイル検索 1
========================================
*/

char *getffile(char *fspec, int type)
{
	int index;
	char fname[NFILEN], ext[4];
	char *fbname;

	fbname = fileblock.name;

	strcpy(path, fspec);
	strcpy(fname, fspec);
	for (index = strlen(path) - 1; index >= 0; index--) {
		if (nthctype(path, index + 1) == CT_ANK) {
			char c = path[index];

			if (sep(c) == slash || c == ':') {
				path[index + 1] = 0;
				break;
			}
		}
	}
	{
		char *p;

		p = jstrchr(&fname[index + 1], '.');
		if (p)
			*p = 0;
		strcat (fname, "*.*");
	}

	if (index < 0)
		*path = 0;

	expand_tild (fname);
	if (FILES(&fileblock, fname, 0x30) != 0)
		return 0;

	strcpy(rbuf, path);
	strcat(rbuf, fbname);

	if (lndrv_info && (fileblock.atr & 0x40)) {
		struct FILBUF tmp;

		FILES(&tmp, rbuf, 0xff);
		fileblock.atr &= ~0x30;
		fileblock.atr |= tmp.atr & 0x30;
	}
	if ((fileblock.atr & 0x20) && type)
		return getnfile(type);

	stcgfe(ext, fbname);
	if ((fileblock.atr & 0x20) && isignoreext(ext)
	    || !strcmp(fbname, ".") || !strcmp(fbname, ".."))
		return getnfile(type);

	if (fileblock.atr & 0x10) {
		char temp[2] = {slash, 0};

		strcat(rbuf, temp);
	}
	return rbuf;
}

/*
========================================
	ファイル検索 2
========================================
*/

char *getnfile(int type)
{
	char *fbname;
	char ext[4];

	fbname = fileblock.name;
	while (1) {
		if (NFILES(&fileblock) != 0)
			return 0;
		strcpy(rbuf, path);
		strcat(rbuf, fbname);

		if (lndrv_info && (fileblock.atr & 0x40)) {
			struct FILBUF tmp;

			FILES(&tmp, rbuf, 0xff);
			fileblock.atr &= ~0x30;
			fileblock.atr |= tmp.atr & 0x30;
		}
		if ((fileblock.atr & 0x20) && type)
			continue;

		stcgfe(ext, fbname);
		if (((fileblock.atr & 0x10) || !isignoreext(ext)) &&
		    strcmp(fbname, ".") && strcmp(fbname, ".."))
			break;
	}

	if (fileblock.atr & 0x10) {
		char temp[2] = {slash, 0};

		strcat(rbuf, temp);
	}
	return rbuf;
}

/*
----------------------------------------
	拡張子の検
----------------------------------------
*/

int isignoreext(char *ext)
{
	char *p = ignoreext;
	char ign[4];

	while (*p) {
		char *ip;

		for (ip = ign; *p && *p != ';'; *ip++ = *p++);
		*ip = 0;
		if (!xstrcmp(ign, ext))
			return TRUE;
		if (*p == ';')
			p++;
	}
	return FALSE;
}

/*
========================================
	get env by GETENV dos call
========================================
*/

char *dosgetenv(char *name)
{
	static char env[NSTRING];

	return (GETENV(name, 0, env) < 0) ? 0 : env;
}

/*
========================================
	set env by SETENV doscall
========================================
*/

int dossetenv(int f, int n)
{
	int status;
	char name[NSTRING];
	char value[NSTRING];

	his_enable(bhisenvp);
	status = mlreply(KTEX232, name, NSTRING);
	if (status != TRUE)
		return (status);
	if (*name == 0)
		return FALSE;
	if (f == TRUE)
		strcpy(value, int_asc(n));
	else {
		his_enable(bhisenvp);
		status = mlreply(KTEX53, value, NSTRING);
		if (status != TRUE && status != FALSE)
			return status;
		if (status == FALSE && *value)
			return status;
	}

	if (SETENV(name, 0, *value ? value : 0) < 0) {
		mlwrite(KTEX233);
		return FALSE;
	}
	return TRUE;
}
