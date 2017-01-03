/*
=================================================================
			MicroEMACS 3.10

		Written by Daniel M. Lawrence
		based on code by Dave G.Conroy.

	(C)opyright 1988,1989 by Daniel M. Lawrence
	MicroEMACS 3.10 can be copied and distributed freely
	for any non-commercial purposes. MicroEMACS 3.10 can
	only be  incorporated  into commercial software with
	the permission of the current author.

	X68000版 日本語 MicroEMACS 3.10

		J1.3  制作 icam & homy.
		J1.40 制作 SALT, PEACE, SHUNA & rima.
		J1.42 制作 lika
		J1.43 制作 rima & SALT

-----------------------------------------------------------------
		MAIN.C: MicroEMACS 3.10
-----------------------------------------------------------------
*/

#define maindef

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ekanji.h"
#include "ehprint.h"
#include "ecall.h"
#include "fepctrl.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/main.c,v 1.10 1992/01/26 11:01:18 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用変数の定義
========================================
*/

static jmp_buf intbuf;
static int setinit = 0;
static int oldd = 0;
static int nowd = 0;
static int oldt = 0;
static int nowt = 0;
static int prefix = 0;

/*
========================================
	使用関数の定義
========================================
*/

static int ankins(int, int, int);
static void dcline(void);
static void edinit(char *);
static int editloop(void);
static int execute(int, int, int);
static int kanjiins(int, int, int);

/*
----------------------------------------
	MAIN PROGRAM
----------------------------------------
*/

void main(int argc, char **argv)
{
	extern long *getlnenv(int);
	int status = 0;

	getwd(current_dir);

	{
		char	*env;

		if (env = getenv("SLASH")) {
			if (!strcmp(env, "\\") || !strcmp(env, "/"))
				slash = *env;
		}
	}

	lndrv_info = getlnenv(_PSP);

	eexitflag = FALSE;
	H_INIT();
	H_CURINT(1);
	normalfont2cfont(font, exfont, 2048);
	normalfont2cfont(font_h, exfont_h, 1024);

	mlinit();
	if (eexitflag)
		goto abortrun;
	vtinit();
	if (eexitflag)
		goto abortrun;

	if (mouseflag)
		MS_CURON();

	pr_agc = argc;
	pr_agv = argv;
	if (setargv(TRUE) == FALSE)
		exit(1);

	varinit();
	keyinit();
	if (eexitflag)
		goto abortrun;

	edinit("*scratch*");

	if (signal(SIGINT, interruptquit) == (int (*)()) -1) {
		mlwrite("Internal Error : 割り込みを設定できません");
		meexit(1);
	}
	INTVCS(ERR_JOB, (void (*)()) interruptquit);

	dcline();

	if (_dump_flag < 0) {
		extern void _debug_print_info(int);
		extern void _clean_heap(void);
		extern char *_SEND;
		int fd;
		int size;
		char *begin;

		H_CURINT(0);
		_debug_print_info(_dump_flag);
		_clean_heap();

		LEDmode = 0;
		sres[0] = 0;

		status = 1;
		_dump_flag = 1;

		size = (int)_SEND - (int)_SSTA;
		if (size % 4) {
			fprintf(stderr, "Illegal stack size.\n");
			goto abortrun;
		}

		fd = CREATE("em.dmp", 0x20);
		if (fd < 0) {
			fprintf(stderr, "Can't open file : em.dmp\n");
			goto abortrun;
		}

		{
			extern void _main(void);
			int buf[2];

			buf[0] = _PSP + 0xf0;
			buf[1] = (int)_main - (_PSP + 0xf0);
			size = 8;
			if (WRITE(fd, (char *)buf, size) != size)
				goto write_fail;
		}

		begin = (char *)_PSP + 0xf0;
		size = (int)_SSTA - (int)begin;
		if (WRITE(fd, begin, size) != size)
			goto write_fail;

		begin = (char *)_SSTA;
		size = ((int)_SEND - (int)begin) / 4;
		{
			int i;
			char buf[size];

			begin = buf;
			memset(begin, 0, size);

			for(i = 0; i < 4; i++) {
				if (WRITE(fd, begin, size) != size)
					goto write_fail;
			}
		}

		begin = (char *)_SEND;
		size = (int)sbrk(0) - (int)begin;
		if (WRITE(fd, begin, size) != size)
			goto write_fail;

		if (CLOSE(fd) < 0) {
write_fail:
			fprintf(stderr, "Write error : em.dmp\n");
			goto abortrun;
		}

		H_CURINT(1);
		status = 0;
		goto abortrun;
	}

	status = editloop();

abortrun:

	vttidy();
	H_CURINT(0);
	cd(current_dir);
	exit(status);
}

/*
========================================
	オプション解
========================================
*/

static void dcline(void)
{
	int ex_argc = ex_agc;
	int firstfile, carg, startflag;
	int viewflag, gotoflag, gline = 0;
	int searchflag, errflag;
	char bname[NBUFN];
	char **ex_argv;
	BUFFER *firstbp = 0;

	viewflag = gotoflag = searchflag = 0;
	startflag = errflag = 0;
	firstfile = 1;

	ex_argv = (char **)malloc(ex_argc * 4);
	for(carg = 1; carg < ex_argc; ++carg) {
		char	*p;

		p = malloc(strlen(ex_agv[carg]) + 1);
		strcpy(p, ex_agv[carg]);
		ex_argv[carg] = p;
	}

	for (carg = 1; carg < ex_argc; ++carg) {
		if (*ex_argv[carg] == '-') {
			switch (tolower(ex_argv[carg][1])) {
			case 'd':
				if (_dump_flag == 0)
					_dump_flag = ex_argv[carg][1] == 'd' ? -1 : -2;
				break;
			case 'e':
				errflag = 1;
				break;
			case 'g':
				gotoflag = 1;
				gline = asc_int(ex_argv[carg] + 2);
				break;
			case 'i':
				{
					char *p = ex_argv[carg] + 2, *q = bname;
					VDESC vd;

					while (*p != '=' && *p)
						*q++ = *p++;
					*q = 0;
					findvar(bname, &vd, NVSIZE + 1);
					if (vd.v_type == (char)-1) {
						mlwrite(KTEX52, bname);
						break;
					}
					svar(&vd, *p ? ++p : "1");
				}
				break;
			case 'r':
				restflag = 1;
				break;
			case 's':
				searchflag = 1;
				strncpy(pat, ex_argv[carg] + 2, NPAT);
				setjtable(pat);
				break;
			case '-':
				debug_system = 1;
				break;
			default:
				break;
			}
		} else if (*ex_argv[carg] == '@') {
			if (startup(ex_argv[carg] + 1) == TRUE)
				startflag = 1;
		}
	}

	if (!startflag && !errflag)
		startup("");

	for (carg = 1; carg < ex_argc; ++carg) {
		if (*ex_argv[carg] == '-') {
			switch (tolower(ex_argv[carg][1])) {
			case 'c':
				viewflag = 0;
				break;
			case 'v':
				viewflag = 1;
			}
		}

		if (*ex_argv[carg] != '-' && *ex_argv[carg] != '@') {
			BUFFER *bp;

			makename(bname, ex_argv[carg]);
			unqname(bname);
			bp = bfind(bname, TRUE, 0);
			strcpy(bp->b_fname, ex_argv[carg]);
			bp->b_active = FALSE;
			if (firstfile) {
				firstbp = bp;
				firstfile = 0;
			}
			if (viewflag)
				bp->b_mode |= MDVIEW;
		}
	}

	if (errflag) {
		if (startup("error.cmd") != TRUE)
			startup("");
	}

	for(carg = 1; carg < ex_argc; ++carg)
		free(ex_argv[carg]);
	free((char *)ex_argv);

	show_filesize = TRUE;

	{
		BUFFER *bp;

		bp = bfind("*scratch*", FALSE, 0);
		if (!firstfile && (gflags & GFREAD)) {
			swbuffer(firstbp);
			update(TRUE);
			zotbuf(bp);
		}
	}

	if (gotoflag && searchflag) {
		update(FALSE);
		mlwrite(KTEX101);
	} else if (gotoflag) {
		if (gotoline(TRUE, gline) == FALSE) {
			update(FALSE);
			mlwrite(KTEX102);
		}
	} else if (searchflag) {
		if (forwhunt(FALSE, 0) == FALSE)
			update(FALSE);
	}
}

/*
========================================
	エディタ初期化
========================================
*/

static void edinit(char *bname)
{
	if (_dump_flag > 0)
		return;

	strcpy(bufhook, "");
	strcpy(cmdhook, "");
	strcpy(exbhook, "");
	strcpy(inshook, "");
	strcpy(readhook, "");
	strcpy(windhook, "");
	strcpy(wraphook, "wrap-word");
	strcpy(writehook, "");

	{
		BUFFER *bp;
		WINDOW *wp;

		bp				= bfind(bname, TRUE, 0);
		blistp			= bfind("*buffer-list*", TRUE, BFINVS);
		bcompp			= bfind("*completions*", TRUE, BFINVS);
		bdictp			= bfind("*dictionary*", TRUE, BFINVS);
		bdiredp			= bfind("*dired*", TRUE, BFINVS);
		bdmarkp			= bfind("*mark-list*", TRUE, BFINVS);
		bhisexecp		= bfind("*history:exec*", TRUE, BFINVS);
		bhisenvp		= bfind("*history:env*", TRUE, BFINVS);
		bhiscmdp		= bfind("*history:command*", TRUE, BFINVS);
		bhissearchp		= bfind("*history:search*", TRUE, BFINVS);
		bhisargp		= bfind("*history:input*", TRUE, BFINVS);
		bhisdebugp		= bfind("*history:debug*", TRUE, BFINVS);
		bhiscmpbufp		= bfind("*history:completion-buffer*", TRUE, BFINVS);
		bhiscmpcp		= bfind("*history:completion-c*", TRUE, BFINVS);
		bhiscmpcomp		= bfind("*history:completion-command*", TRUE, BFINVS);
		bhiscmpfnamep	= bfind("*history:completion-file*", TRUE, BFINVS);
		bhiscmpgenp		= bfind("*history:completion-general*", TRUE, BFINVS);
		bhiscmplatexp	= bfind("*history:completion-latex*", TRUE, BFINVS);
		bhiscmpmacp		= bfind("*history:completion-macro*", TRUE, BFINVS);
		bhiscmpmodep	= bfind("*history:completion-mode*", TRUE, BFINVS);
		bhiscmpvarp		= bfind("*history:completion-variables*", TRUE, BFINVS);
		bhiscmpdnamep	= bfind("*history:completion-directory*", TRUE, BFINVS);

		wp = (WINDOW *) malloc(sizeof(WINDOW));

		if (!bp || !wp || !blistp || !bcompp || !bdiredp || !bdmarkp
		    || !bhisexecp || !bhisenvp || !bhiscmdp || !bhissearchp
		    || !bhisargp || !bhisdebugp || !bhiscmpbufp || !bhiscmpcp
		    || !bhiscmpcomp || !bhiscmpfnamep || !bhiscmpgenp
		    || !bhiscmplatexp || !bhiscmpmacp || !bhiscmpmodep
		    || !bhiscmpvarp || !bhiscmpdnamep)
			meexit(1);

		curbp = bp;
		wheadp = wp;
		curwp = wp;
		wp->w_wndp = 0;
		wp->w_bufp = bp;
		bp->b_nwnd = 1;
		wp->w_linep = bp->b_linep;
		wp->w_dotp = bp->b_linep;
		wp->w_doto = 0;
		wp->w_jumpno = 0;
		wp->w_markno = 0;

		{
			int i;

			for (i = 0; i < NMARKS; i++) {
				wp->w_markp[i] = 0;
				wp->w_marko[i] = 0;
			}
		}

		wp->w_toprow = 0;
/*		wp->w_bcolor = gbcolor;		*/
		wp->w_fcol = 0;
		wp->w_ntrows = term.t_nrow - 1;
		wp->w_force = 0;
		wp->w_flag = WFMODE | WFHARD;
	}
}

/*
========================================
	編集ループ
========================================
*/

static int editloop(void)
{
	lastflag = 0;
	setinit = 1;
	update(TRUE);

	while (1) {
		if (forceproc != 0)
			setjmp(intbuf);

		if (eexitflag)
			return eexitval;

		{
			int old_flag;

			old_flag = lastflag;
			exechook(cmdhook);
			lastflag = old_flag;
		}

		if (timeflag || dateflag) {
			nowt = GETTIME() & ~0x1f;
			nowd = GETDATE();
			if (nowt != oldt || nowd != oldd) {
				WINDOW *wp;

				oldt = nowt;
				oldd = nowd;
				for (wp = wheadp; wp; wp = wp->w_wndp)
					wp->w_flag |= WFMODE;
			}
		}
		update(FALSE);

		{
			int c, f, n;

			if (kbdmode == STOP && !prefix)
				ena_hscroll = 1;
			ena_zcursor = 1;
			ena_include_fep_mode = 1;

			if (!prefix && curbp->b_fep_mode != iskmode()) {
				if (curbp->b_fep_mode)
					fep_force_on();
				else
					fep_force_off();
			}

			c = getkey();

			ena_include_fep_mode = 0;
			ena_zcursor = 0;
			ena_hscroll = 0;

			if (mpresf != FALSE) {
				mlerase();
				update(FALSE);
			}
			if (prefix) {
				c |= prefix;
				f = predef;
				n = prenum;
			} else {
				f = FALSE;
				n = 1;
			}

			{
				int basec;

				basec = c & ~META;
				if ((c & META)
				    && ((basec >= '0' && basec <= '9') || basec == '-')
				    && (getbind(c) == 0)) {
					int mflag;

					f = TRUE;
					n = 0;
					mflag = 1;
					c = basec;
					while ((c >= '0' && c <= '9') || (c == '-')) {
						if (c == '-') {
							if (mflag == -1 || n != 0)
								break;
							mflag = -1;
						} else
							n = n * 10 + (c - '0');

						if (n == 0 && mflag == -1)
							mlwrite("Arg:");
						else
							mlwrite("Arg: %d", n * mflag);

						c = getkey();
					}
					n *= mflag;
				}
			}

			if (c == reptc) {
				int mflag;

				f = TRUE;
				n = 4;
				mflag = 0;
				mlwrite("Arg: 4");
				while (c = getkey(), (c >= '0' && c <= '9') || c == reptc || c == '-') {
					if (c == reptc) {
						n <<= 2;
						if (n < 0)
							n = 4;
					} else if (c == '-') {
						if (mflag)
							break;
						n = 0;
						mflag = -1;
					} else {
						if (!mflag) {
							n = 0;
							mflag = 1;
						}
						n = 10 * n + c - '0';
					}
					mlwrite("Arg: %d", (mflag >= 0) ? n : (n ? -n : -1));
				}
				if (mflag == -1)
					n = n ? -n : 1;
			}
			prefix = 0;
			execute(c, f, n);
		}
	}
}

/*
========================================
	入力処理
========================================
*/

static int execute(int c, int f, int n)
{
	int status;
	char work[16];

	if (iskanji(c))
		status = kanjiins(c, f, n);
	else {
		KEYTAB *key;

		key = getbind(change_key(c));
		if (key && key->k_ptr.fp == cex) {
			int cc;

			cmdstr2(c, work);
			if (kbdmode != PLAY) {
				fep_force_off();
				mlwrite(KTEX219, work);
			}
			thisflag = lastflag;
			update(FALSE);
			cc = getkey();
			c = cc | ((cc < 0x10000) ? (c << 16) : 0);
			if (mpresf != FALSE) {
				mlerase();
				update(FALSE);
			}
			key = getbind(change_key(c));
		}
		if (key) {
			if (curbp->b_fep_mode)
				fep_force_on();
			thisflag = 0;
			status = execkey(key, f, n);
			lastflag = thisflag;
		} else
			status = ankins(c, f, n);
	}
	return status;
}

/*
----------------------------------------
	終了
----------------------------------------
*/

int quickexit(int f, int n)
{
	char fname[NFILEN], out[NSTRING];
	BUFFER *bp, *oldcb;

	for (oldcb = curbp, bp = bheadp; bp; bp = bp->b_bufp) {
		if ((bp->b_flag & BFCHG) != 0
		    && (bp->b_flag & BFINVS) == 0
		    && (*bp->b_fname != 0 || quickext == 0)) {
			int status;

			if (*bp->b_fname == 0) {
				swbuffer(bp);
				update(TRUE);
				sprintf(out, KTEX236, bp->b_bname);
				status = mlyesno(out);
				if (status == FALSE)
					continue;
				else if (status == TRUE) {
					his_disable();
					status = mlreply(KTEX151, fname, NFILEN);
					if (status == ABORT)
						return ABORT;
					if (status == FALSE)
						*bp->b_fname = 0;
					else
						expname(fname, bp->b_fname);
				} else {
					curbp = bp;
					return status;
				}
			}
			curbp = bp;
			mlwrite(KTEX103, bp->b_fname);
			mlwrite("");
			status = filesave(FALSE, 1);
			if (status != TRUE) {
				curbp = oldcb;
				return status;
			}
		}
	}

	quit(f, n);

	return TRUE;
}

/*
----------------------------------------
	終了 2
----------------------------------------
*/

int quit(int f, int n)
{
	int status = TRUE;

	if (f != FALSE || anycb() == FALSE || (status = mlyesno(KTEX104), status == TRUE))
		status = meexit(f ? n : GOOD);

	mlerase();
	return status;
}

/*
----------------------------------------
	exit 処理
----------------------------------------
*/

int meexit(int status)
{
	eexitflag = TRUE;
	eexitval = status;
	return TRUE;
}

/*
----------------------------------------
	マクロ記録開始
----------------------------------------
*/

int ctlxlp(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite(KTEX105);
		return FALSE;
	}
	mlwrite(KTEX106);
	kbdptr = &kbdm[0];
	kbdend = kbdptr;
	kbdmode = RECORD;

	return TRUE;
}

/*
----------------------------------------
	マクロ記録終了
----------------------------------------
*/

int ctlxrp(int f, int n)
{
	if (kbdmode == STOP) {
		mlwrite(KTEX107);
		return FALSE;
	} else if (kbdmode == RECORD) {
		mlwrite(KTEX108);
		kbdmode = STOP;
	}
	return TRUE;
}

/*
----------------------------------------
	マクロ実行
----------------------------------------
*/

int ctlxe(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite(KTEX105);
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	kbdrep = n;
	kbdmode = PLAY;
	kbdptr = &kbdm[0];
	return TRUE;
}

/*
----------------------------------------
	ABORT
----------------------------------------
*/

int ctrlg(int f, int n)
{
	H68beep();
	kbdmode = STOP;
	mlwrite(KTEX8);
	return ABORT;
}

/*
========================================
	VIEW 警告
========================================
*/

int rdonly(void)
{
	H68beep();
	mlwrite(KTEX109);
	return FALSE;
}

/*
========================================
	RESTRICT 警告
========================================
*/

int resterr(void)
{
	H68beep();
	mlwrite(KTEX110);
	return FALSE;
}

/*
========================================
	(Dired)モードでなかった
========================================
*/

int bad_mode(void)
{
	H68beep();
	mlwrite(KTEX320);
	return FALSE;
}

/*
----------------------------------------
	NULL
----------------------------------------
*/

int nullproc(int f, int n)
{
	return TRUE;
}

/*
----------------------------------------
	META
----------------------------------------
*/

int meta(int f, int n)
{
	prefix |= META;
	prenum = n;
	predef = f;
	if (!clexec && kbdmode != PLAY && fepctrlesc) {
		fep_force_off();
		mlwrite(KTEX220);
	}
	thisflag = lastflag;

	return TRUE;
}

/*
----------------------------------------
	2 ストローク
----------------------------------------
*/

int cex(int f, int n)
{
	thisflag = lastflag;
	return TRUE;
}

/*
========================================
	UNIVERSAL ARGUMENT
========================================
*/

int unarg(int f, int n)
{
	thisflag = lastflag;
	return TRUE;
}

/*
========================================
	ANK 文字挿入
========================================
*/

static int ankins(int c, int f, int n)
{
	if (c < 0x20 || c > 0xff) {
		H68beep();
		mlwrite(KTEX19);
		lastflag = 0;
		return FALSE;
	}

	if (curbp->b_mode & (MDVIEW | MDDIRED))
		return rdonly();

	if (c == ' ' && n >= 0 && check_wrap())
		exechook(wraphook);

	{
		int status;

		if (n <= 0) {
			lastflag = 0;
			return (n < 0) ? FALSE : TRUE;
		}
		thisflag = 0;

		if (check_over())
			forwdel(FALSE, 1);

		status = linsert(n, c);
		if (curbp->b_mode & MDASAVE) {
			if (--gacount == 0) {
				upscreen(FALSE, 0);
				filesave(TRUE, 0);
				gacount = gasave;
			}
		}
		lastflag = thisflag;

		return status;
	}
}

/*
========================================
	漢字挿入
========================================
*/

static int kanjiins(int c, int f, int n)
{
	int status, c2, zenkakuchar;
	WINDOW *wp = curwp;

	c2 = getcmd();
	zenkakuchar = ((c << 8) | (c2 & CHARMASK)) & WORDMASK;

	if (curbp->b_mode & (MDVIEW | MDDIRED))
		return rdonly();

	if ((wp->w_bufp->b_mode & MDWRAP)
	    && fillcol > 0
	    && n >= 0
	    && getccol(FALSE) >= fillcol - 1
	    && (wp->w_bufp->b_mode & (MDVIEW | MDDIRED)) == FALSE
	    && forwbreakok(zenkakuchar))
		exechook(wraphook);

	if (n <= 0) {
		lastflag = 0;
		return (n < 0) ? FALSE : TRUE;
	}
	thisflag = 0;

	if (check_over())
		forwdel(FALSE, 1);

	for (status = TRUE; n-- && status; status = linsert(1, c) & linsert(1, c2));

	if (curbp->b_mode & MDASAVE) {
		if (--gacount == 0) {
			upscreen(FALSE, 0);
			filesave(TRUE, 0);
			gacount = gasave;
		}
	}
	lastflag = thisflag;

	return status;
}

/*
========================================
	割り込み処理
========================================
*/

int interruptquit(void)
{
	if (forceproc != 0 && mlyesno(KTEX235) != FALSE) {
		if (signal(SIGINT, interruptquit) == (int (*)()) -1) {
			mlwrite("Internal Error : 割り込みを設定できません");
			meexit(1);
		}
		mlforce(KTEX240);
		longjmp(intbuf, 1);
	}

	{
		BUFFER *bp;

		for (bp = bheadp; bp; bp = bp->b_bufp) {
			if ((bp->b_flag & BFCHG) && (bp->b_flag & BFINVS) == 0) {
				curbp = bp;
				filesave(TRUE, 0);
			}
		}
	}

	vttidy();
	H_CURINT(0);
	cd(current_dir);
	exit(999);
}

/*
========================================
	malloc 警告
========================================
*/

void warnfunc(char *mes)
{
	H68beep();
	mlforce(mes);
}

/*
========================================
	アボート処理
========================================
*/

void meabort(char *mes)
{
	warnfunc(mes);
	if (!setinit)
		asm volatile("move $0,d0");

	mlforce(KTEX240);
	longjmp(intbuf, 1);
}

/*
----------------------------------------
	check over
----------------------------------------
*/

int check_over(void)
{
	WINDOW *wp = curwp;

	return ((wp->w_bufp->b_mode & MDOVER)
		     && wp->w_doto < llength(wp->w_dotp)
		     && (lgetc(wp->w_dotp, wp->w_doto) != TAB
		         || (wp->w_doto) % wp->w_bufp->b_tabs == wp->w_bufp->b_tabs - 1))
		? TRUE : FALSE;
}

/*
----------------------------------------
	include_fep_status
----------------------------------------
*/

void include_fep_mode(int mode)
{
	if (ena_include_fep_mode)
		curbp->b_fep_mode = mode;
}
