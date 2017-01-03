/*
----------------------------------------
	FILE.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <sys/stat.h>
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/file.c,v 1.8 1992/01/29 13:25:00 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static int ifile(char *);
static void makebackup(char *s);
static void set_cd(char *);

/*
----------------------------------------
	ファイルをバッファに読み込む
----------------------------------------
*/

int fileread(int f, int n)
{
	char *fname;
	char cwd[NLINE];

	set_cd(cwd);
	fname = singleexpwild(gtfilename(KTEX131));
	cd(cwd);
	return fname ? readin(f, n, fname) : FALSE;
}

/*
----------------------------------------
	ファイル挿入
----------------------------------------
*/

int insfile(int f, int n)
{
	char *fname;
	char cwd[NLINE];

	set_cd(cwd);
	fname = singleexpwild(gtfilename(KTEX132));
	cd(cwd);
	return fname ? ifile(fname) : FALSE;
}

/*
----------------------------------------
	ファイル読み込み
----------------------------------------
*/

int filefind(int f, int n)
{
	int	status;
	char *fname;
	char cwd[NLINE];

	fname = gtfilename(KTEX133);
	if (fname == 0 || *fname == 0)
		return FALSE;
	if (ifsplit)
		splitwind(f, n);

	set_cd(cwd);
	status = expwild(fname, FALSE);
	cd(cwd);
	return status;
}

/*
----------------------------------------
	ファイル参照
----------------------------------------
*/

int viewfile(int f, int n)
{
	int	status;
	char *fname;
	char cwd[NLINE];

	fname = gtfilename(KTEX134);
	if (fname == 0 || *fname == 0)
		return FALSE;
	if (ifsplit)
		splitwind(f, n);

	set_cd(cwd);
	status = expwild(fname, TRUE);
	cd(cwd);
	return status;
}

/*
----------------------------------------
	カレントディレクトリの設定
----------------------------------------
*/

static void set_cd(char *cwd)
{
	struct NAMESTBUF namebuf;
	char fwd[NLINE];

	getwd(cwd);
	if (cbufdir && *curbp->b_fname) {
		NAMESTS(curbp->b_fname, &namebuf);
		fwd[0] = namebuf.drive + 'A';
		fwd[1] = ':';
		strcpy(fwd + 2, (char *) namebuf.path);
		cd(fwd);
	} else
		cd(current_dir);
}

/*
========================================
	ファイル入力
========================================
*/

int getfile(char *fname)
{
	char bname[NBUFN];
	BUFFER *cbp = curbp;
	WINDOW *cwp = curwp;

	{
		BUFFER *bp;

		for (bp = bheadp; bp; bp = bp->b_bufp) {
			if ((bp->b_flag & BFINVS) == 0 && strcmp(bp->b_fname, fname) == 0) {
				int i;
				LINE *lp;

				swbuffer(bp);
				lp = cwp->w_dotp;
				i = cwp->w_ntrows / 2;
				while (i-- && lback(lp) != cbp->b_linep)
					lp = lback(lp);
				cwp->w_linep = lp;
				cwp->w_flag |= WFMODE | WFHARD;
				mlwrite(KTEX135);
				return TRUE;
			}
		}
	}

	makename(bname, fname);

	{
		BUFFER *bp;

		while (bp = bfind(bname, FALSE, 0)) {
			int status;

			his_disable();
			status = mlreply(KTEX136, bname, NBUFN);
			if (status == ABORT)
				return status;
			else if (status == FALSE) {
				makename(bname, fname);
				break;
			}
		}
		if (bp == 0 && (bp = bfind(bname, TRUE, 0)) == 0) {
			mlwrite(KTEX137);
			return FALSE;
		}
		swbuffer(bp);
	}

	return readin(FALSE, 0, fname);
}

/*
========================================
	読み込み
========================================
*/

int readin(int f, int n, char *fname)
{
	BUFFER *bp = curbp;
	int nline, status;
	char mesg[NSTRING];

	{
		status = bclear(bp);
		if (status != TRUE)
			return status;
		bp->b_flag &= (bp->b_flag & BFNAROW) ? ~BFINVS : ~(BFINVS | BFCHG);
		strcpy(bp->b_fname, fname);
		exechook(readhook);

		status = ffropen(fname);
		if (status == FIOERR)
			goto out;
		if (status == FIOFNF) {
			mlwrite(KTEX138);
			goto out;
		}
	}

	{
		LINE *lp, *lastlp;
		int report;
		int file_size, read_total = 0;
		int read_size = REPORT_SIZE;
		int nbytes;

		file_size = ffsize();
		report = show_filesize && file_size > REPORT_SIZE && f == FALSE;
		if (report)
			mlwrite(KTEX251, 0, file_size);
		else
			mlwrite(KTEX139);

		n = (n > 0) ? n : 1;
		nline = 0;
		lastlp = lback (bp->b_linep);
		while ((status = ffreadline(&lp, &nbytes)) == FIOSUC) {
			lastlp->l_fp = lp;
			lp->l_bp = lastlp;
			lastlp = lp;

			nline++;
			read_total += nbytes;
			read_size -= nbytes;
			if (read_size <= 0) {
				while (read_size <= 0)
					read_size += REPORT_STEP;
				if (report)
					mlwrite(KTEX251, read_total, file_size);
			}

			if (f == TRUE && nline >= n)
				break;
		}
		{
			LINE *b_linep = bp->b_linep;

			lastlp->l_fp = b_linep;
			b_linep->l_bp = lastlp;
		}
		if (report)
			mlwrite(KTEX251, read_total, file_size);
	}

	ffclose();

	{
		char *tmp;

		if (status == FIOERR) {
			tmp = KTEX141;
			curbp->b_flag |= BFTRUNC;
		} else if (status == FIOMEM) {
			tmp = KTEX142;
			curbp->b_flag |= BFTRUNC;
		} else
			tmp = "";

		sprintf(mesg, "[%s%s%d%s]" ,tmp ,KTEX140 ,nline ,KTEX143);
		mlwrite(mesg);
	}

out:

	winbob(curbp);

	return (status == FIOERR || status == FIOFNF) ? FALSE : TRUE;
}

/*
========================================
	ファイル名を作る
========================================
*/

char *makename(char *bname, char *fname)
{
	char c, *p, *pp;

	for (pp = fname, p = fname; c = *p; p++) {
		if (iskanji(c))
			p++;
		else if (sep(c) == slash || c == ':')
			pp = p + 1;
	}
	strcpy(bname, pp);
	return pp;
}

/*
========================================
	テンポラリファイル名を作成
========================================
*/

void unqname(char *name)
{
	while (bfind(name, FALSE, 0)) {
		char *sp;

		for (sp = name; *sp; sp++);
		if (sp == name || (sp[-1] < '0' || sp[-1] > '8')) {
			*sp++ = '0';
			*sp = 0;
		} else
			sp[-1]++;
	}
}

/*
----------------------------------------
	ファイル書き出
----------------------------------------
*/

int filewrite(int f, int n)
{
	char *fname;

	fname = gtfilename(KTEX144);
	if (fname == 0 || *fname == 0)
		return FALSE;

	{
		BUFFER *bp = curbp;
		int status;
		char cwd[NLINE];

		set_cd(cwd);
		expname(fname, fname);
		status = writeout(fname);
		if (status == TRUE && (bp->b_flag & BFNAROW) == 0) {
			strcpy(bp->b_fname, fname);
			bp->b_flag &= ~BFCHG;
			upmode();
		}
		cd(cwd);
		return status;
	}
}

/*
----------------------------------------
	ファイルセーブ
----------------------------------------
*/

int filesave(int f, int n)
{
	int status;
	char path[NFILEN], name[NFILEN];
	BUFFER *bp = curbp;

	if ((bp->b_flag & BFCHG) == 0)
		return TRUE;
	if (*bp->b_fname == 0) {
		mlwrite(KTEX145);
		return FALSE;
	}
	getpath(path, bp->b_fname);
	makename(name, bp->b_fname);
	strcat(path, "#");
	strcat(path, name);

	{
		int oldmak = makbak, oldsav = ssave;

		if (f == FALSE) {
			if (fexist(path))
				unlink(path);
			strcpy(path, bp->b_fname);
		} else {
			makbak = FALSE;
			ssave = FALSE;
		}
		if (bp->b_flag & BFTRUNC) {
			status = mlyesno(KTEX146);
			if (status != TRUE) {
				if (status != ABORT)
					mlwrite(KTEX8);
				makbak = oldmak;
				ssave  = oldsav;
				return status;
			}
		}
		if (bp->b_flag & BFNAROW) {
			status = mlyesno(KTEX147);
			if (status != TRUE) {
				if (status != ABORT)
					mlwrite(KTEX8);
				makbak = oldmak;
				ssave  = oldsav;
				return status;
			}
		}
		status = writeout(path);
		if (status == TRUE && f == FALSE) {
			bp->b_flag &= ~BFCHG;
			upmode();
		}
		makbak = oldmak;
		ssave = oldsav;
	}

	return status;
}

/*
========================================
	書き出し本体
========================================
*/

int writeout(char *fn)
{
	int status, safe, nline;
	int attr;
	char tname[NFILEN], realname[NFILEN], buf[NSTRING];

	strcpy (realname, fn);
	attr = CHMOD (realname, -1);
	if (attr >= 0) {
		/* SYMLINK */
		if (lndrv_info && (attr & 0x40)) {
			/* getrealpath */
			((int (*) (char *, char *))lndrv_info[37]) (realname, fn);
			attr = CHMOD (realname, -1);
		}
		/* READ ONLY */
		if (attr & 0x01) {
			mlwrite(KTEX180);
			return FALSE;
		}
		/* VOL or DIR */
		if (attr & (0x08 | 0x10)) {
			mlwrite(KTEX181);
			return FALSE;
		}
	} else
		attr = 0x20;

	exechook(writehook);

	safe = (ssave && fexist(realname)) ? TRUE : FALSE;
	if (safe) {
		getpath(tname, realname);
		strcat(tname, "#XXXXXX");
		mktemp(tname);
		status = ffwopen(tname, attr);
	} else {
		if (makbak)
			makebackup(realname);
		status = ffwopen(realname, attr);
	}
	if (status != FIOSUC)
		return FALSE;

	{
		int report;
		int file_size, write_size, write_total;
		LINE *lp;
		BUFFER *bp = curbp;

		for (file_size = 0, lp = lforw(bp->b_linep); lp != bp->b_linep; lp = lforw(lp))
			file_size += llength(lp) + 2;

		report = show_filesize && file_size > REPORT_SIZE;
		if (report)
			mlwrite(KTEX252, 0, file_size);
		else
			mlwrite(KTEX148);

		write_size = REPORT_STEP;
		write_total = nline = 0;
		for (lp = lforw(bp->b_linep); lp != bp->b_linep; lp = lforw(lp), nline++) {
			int line_length;

			line_length = llength(lp);
			status = ffwriteline(lp->l_text, line_length);
			if (status != FIOSUC)
				break;

			line_length += 2;
			write_total += line_length;
			write_size -= line_length;
			if (write_size <= 0) {
				while (write_size <= 0)
					write_size += REPORT_STEP;
				if (report)
					mlwrite(KTEX252, write_total, file_size);
			}
		}
		if (report)
			mlwrite(KTEX252, write_total, file_size);
	}

	if (addeof)
		status = ffwriteline("", -1);

	status |= ffclose();
	if (status == FIOSUC) {
		sprintf(buf, "[%d%s]", nline, KTEX143);
		if (safe) {
			if (makbak)
				makebackup(realname);
			else
				unlink(realname);
			if (rename(tname, realname) != 0)
				status = FIODEL;
		}
		mlwrite(buf);
	}
	return status == FIOSUC;
}

/*
========================================
	バックアップを作る
========================================
*/

static void makebackup(char *s)
{
	char bakname[NFILEN];
	struct stat buffer;

	if (stat(s, &buffer) < 0 || buffer.st_mode & S_IFDIR)
		return;

	{
		char *p, *period;
		int ch;

		strcpy(bakname, s);
		for(period = bakname, p = bakname; ch = *p; p++) {
			if (iskanji(ch))
				p++;
			else if (ch == '.')
				period = p;
			else if (sep(ch) == slash || ch == ':')
				period = bakname;
		}
		if (period == bakname || sep(period[-1]) == slash || sep(period[-1]) == ':')
			strcat(p, ".bak");
		else
			strcpy(period + 1, "bak");
	}
	if (stat(bakname, &buffer) >= 0)
		unlink(bakname);
	rename(s, bakname);
}

/*
----------------------------------------
	ファイル
----------------------------------------
*/

int filename(int f, int n)
{
	int saveact;
	char *fname;

	saveact = quickact[CMP_FILENAME];
	quickact[CMP_FILENAME] = FALSE;
	fname = gtfilename(KTEX151);
	quickact[CMP_FILENAME] = saveact;
	if (fname == 0 || *fname == 0)
		return FALSE;
	expname(fname, fname);
	strcpy(curbp->b_fname, fname);
	makename(curbp->b_bname, fname);
	upmode();
	curbp->b_flag |= BFCHG;
	curbp->b_mode &= ~MDVIEW;
	return TRUE;
}

/*
========================================
	挿入
========================================
*/

static int ifile(char *fname)
{
	WINDOW *wp = curwp;
	BUFFER *bp = curbp;
	int status, nline;
	char mesg[NSTRING];

	bp->b_flag |= BFCHG;
	bp->b_flag &= ~BFINVS;
	status = ffropen(fname);
	if (status == FIOERR)
		goto out;
	else if (status == FIOFNF) {
		mlwrite(KTEX152);
		return FALSE;
	}
	mlwrite(KTEX153);

	{
		LINE *lp0, *lp1, *lp2;
		int nbytes;

		wp->w_dotp = lback(wp->w_dotp);
		wp->w_doto = 0;
		for (nline = 0; (status = ffreadline(&lp1, &nbytes)) == FIOSUC; nline++) {
			lp0 = wp->w_dotp;
			lp2 = lp0->l_fp;
			lp2->l_bp = lp1;
			lp0->l_fp = lp1;
			lp1->l_bp = lp0;
			lp1->l_fp = lp2;
			wp->w_dotp = lp1;
		}
	}

	ffclose();

	{
		char *tmp;

		if (status == FIOERR) {
			tmp = KTEX141;
			bp->b_flag |= BFTRUNC;
		} else if (status == FIOMEM) {
			tmp = KTEX142;
			bp->b_flag |= BFTRUNC;
		} else
			tmp = "";

		sprintf(mesg, "[%s%s%d%s]" ,tmp ,KTEX154 ,nline ,KTEX143);
		mlwrite(mesg);
	}

out:

	{
		wp->w_dotp = lforw(wp->w_dotp);
		wp->w_flag |= WFHARD | WFMODE;
		bp->b_dotp = wp->w_dotp;
		bp->b_doto = wp->w_doto;
		bp->b_fcol = wp->w_fcol;
	}

	return (status == FIOERR) ? FALSE : TRUE;
}

/*
========================================
	ワイルドカード展開 M
========================================
*/

int expwild(char *fname, int vflag)
{
	char *argv[2];
	char bname[NBUFN], sname[NFILEN];

	argv[0] = "dum";
	argv[1] = fname;
	pr_agv = argv;
	pr_agc = 2;

	if (setargv(FALSE) == FALSE)
		return FALSE;
	if (ex_agc <= 1) {
		mlwrite(KTEX245);
		return FALSE;
	}

	{
		int i, wfound, first = 1;
		WINDOW *wp = curwp;
		BUFFER *fbp = 0;

		wfound = ((sindex(fname, "?") + sindex(fname, "*")) > 0);
		for (i = 1; i < ex_agc; i++) {
			BUFFER *bp;

			if (wfound) {
				makename(bname, ex_agv[i]);
				strcpy(sname, ex_agv[i]);
			} else {
				makename(bname, fname);
				getpath(sname, ex_agv[i]);
				strcat(sname, bname);
			}
			for (bp = bheadp; bp; bp = bp->b_bufp) {
				if ((bp->b_flag & BFINVS) == 0 && strcmp(bp->b_fname, sname) == 0) {
					int j;
					LINE *lp;

					swbuffer(bp);
					lp = wp->w_dotp;
					j = wp->w_ntrows >> 1;
					while (j-- && lback(lp) != curbp->b_linep)
						lp = lback(lp);
					wp->w_linep = lp;
					wp->w_flag |= WFMODE | WFHARD;
					mlwrite(KTEX135);
					goto nextarg;
				}
			}
			bp = bfind(bname, FALSE, 0);
			if (bp)
				unqname(bname);
			bp = bfind(bname, TRUE, 0);
			strcpy(bp->b_fname, sname);
			bp->b_active = FALSE;
			if (first) {
				fbp = bp;
				first = 0;
			}
			{
				struct stat sbuf;

				if (!stat(sname, &sbuf) && !(sbuf.st_mode & S_IWRITE) || vflag)
					bp->b_mode |= MDVIEW;
			}
nextarg:;
		}
		if (!first) {
			swbuffer(fbp);
			update(TRUE);
		}
	}

	return TRUE;
}

/*
========================================
	ワイルドカード展開 S
========================================
*/

char *singleexpwild(char *fname)
{
	char *argv[2];

	if (fname == 0 || *fname == 0)
		return 0;

	argv[0] = "dum";
	argv[1] = fname;
	pr_agc = 2;
	pr_agv = argv;

	if (setargv(FALSE) == FALSE)
		return 0;
	if (ex_agc <= 1) {
		mlwrite(KTEX245);
		return 0;
	}
	return ex_agv[1];
}
