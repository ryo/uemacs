/*
----------------------------------------
	DIRED.C: MicroEMACS 3.10
----------------------------------------
*/

/* Tiny directory editor by t.mori */

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
#include "ekanji.h"
#include "ecall.h"
#include "fepctrl.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/dired.c,v 1.5 1992/01/26 11:09:54 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static char *dired_ask_dirname(BUFFER *);
static char *findmd(char *, char *);
static char *get_pathname(char *, char *);
static char *get_target_extname(LINE *);
static char *get_trash_name(char *);
static char *makefilename(LINE *);
static int chk_cp_ok(char *, char *, int, int *);
static int chk_enable_cp(char *, char *, int);
static int dired_readfile(int f);
static int dirname_cmp(char *, char *);
static int do_dired_mv_1(int f, int n, int c);
static int do_mark(int, int);
static int getfile2(char *, int, BUFFER **);
static int isdexist(char *);
static int isexist(char *);
static int isnamelist(LINE *);
static int iswc(char);
static int killmarklines(int f);
static int make_dired_buf(char *, char *);
static int open_marked_files(int);
static int timestamp_cmp(char *, char *);
static void disk_flush(void);

/*
========================================
	使用変数の定義
========================================
*/

static char current_dir[NFILEN];
static char target_dir[NFILEN];
static int current_drv, target_drv;
static int pushd_popd_flag = FALSE;
static int t_diskflush, t_disk_free;

/*
========================================
	フラッシュ
========================================
*/

static inline void disk_flush(void)
{
	t_diskflush = 0;
}

/*
----------------------------------------
	ディレクトリリスト読み込み
----------------------------------------
*/

static char *dired_ask_dirname(BUFFER *bp)
{
	struct NAMESTBUF namebuf;
	char cwd[NFILEN];
	char fwd[NFILEN];
	char *line;

	if (*bp->b_fname) {
		NAMESTS(bp->b_fname, &namebuf);
		fwd[0] = namebuf.drive + 'A';
		fwd[1] = ':';
		strcpy(fwd + 2, (char *)namebuf.path);
	} else
		strcpy(fwd, current_dir);

	getwd(cwd);
	cd(fwd);
	line = gtdirname(KTEX305);
	cd(cwd);

	if (line) {
		char c, *p;

		for (p = line; c = *p; p++) {
			if (iskanji(c))
				p++;
			else if (c == ' ' || c == TAB) {
				*p = 0;
				break;
			}
		}
		if (*line == 0)
			line = NULL;
	}
	return line;
}

/*
----------------------------------------
	dired
----------------------------------------
*/

int dired(int f, int n)
{
	WINDOW *wp = curwp;
	char dired_path[NFILEN];
	char *line;

	bdiredp->b_mode &= ~(MDC | MDLATEX | MDWRAP | MDVIEW);
	bdiredp->b_mode |= MDDIRED;
	bdiredp->b_keymap = 3;

	if (*bdiredp->b_fname == 0 || curbp == bdiredp) {
		line = dired_ask_dirname(bdiredp);
		if (!line)
			return FALSE;
		strcpy(dired_path, line);

		swbuffer(bdiredp);
		make_dired_buf(dired_ls_cmd, dired_path);
		if (lgetc(lforw(curbp->b_linep), 0) != '\xa0') {
			lchange(WFHARD);
			*dired_current_dir = 0;
			return FALSE;
		}
		return TRUE;
	}

	{
		WINDOW *wwp = wp;

		while (1) {
			wwp = curwp->w_wndp ? : wheadp;
			curwp = wwp;
			curbp = wwp->w_bufp;
			if (curwp == wp || curbp == bdiredp)
				break;
		}
		if (curwp != wp) {
			upmode();
			update(TRUE);
		}
		if (curbp != bdiredp) {
			swbuffer(bdiredp);
			update(TRUE);
		}
		return TRUE;
	}
}

/*
----------------------------------------
	ディレクトリリスト読み込み
----------------------------------------
*/

int do_dired_rb(int f, int n)
{
	char *fname;

	fname = curbp->b_fname;
	if (*fname) {
		make_dired_buf(dired_ls_cmd, fname);

		if (lgetc(lforw(curbp->b_linep), 0) != '\xa0') {
			lchange(WFHARD);
			*dired_current_dir = 0;
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

/*
----------------------------------------
	* マークを全てクリア
----------------------------------------
*/

int do_dired_cancel_all(int f, int n)
{
	LINE *lp;

	for (lp = lforw(curbp->b_linep); isnamelist(lp); lp = lforw(lp)) {
		if (*lp->l_text == '*')
			*lp->l_text = '\xa0';
	}
	lchange(WFHARD);
	return TRUE;
}

/*
----------------------------------------
	* マークをつける
----------------------------------------
*/

int do_dired_mark(int f, int n)
{
	while (n-- > 0) {
		int status;

		status = do_mark(f, TRUE);
		if (status != TRUE)
			return status;
	}
	return TRUE;
}

/*
----------------------------------------
	マークをつける（逆方向）
----------------------------------------
*/

int do_dired_mark_back(int f, int n)
{
	while (n-- > 0) {
		int status;

		status = do_mark(f, FALSE);
		if (status != TRUE)
			return status;
	}
	return TRUE;
}


/*
========================================
	マーク本体
========================================
*/

static int do_mark(int f, int type)
{
	WINDOW *wp = curwp;

	if (wp->w_bufp != bdiredp)
		return FALSE;

	if (type != TRUE) {
		int status;

		status = do_dired_backline(FALSE, 1);
		if (status != TRUE)
			return status;
		if (!clexec)
			update(TRUE);
	}

	{
		LINE *lp;

		lp = curwp->w_dotp;
		if (isnamelist(lp)) {
			if (llength(lp) >= 8) {
				if (!strncmp(lp->l_text + 3, "total", 5))
					goto no_mark;
			}
			if ((lgetc(lp, 6) == 'a') && (lgetc(wp->w_dotp, 7) != 's')) {
				lputc(lp, 0, ((*lp->l_text == '\xa0') ? '*' : '\xa0'));
				lchange(WFEDIT);
				curgoal = wp->w_doto = dired_ls_name_pos;
				if (!clexec)
					update(TRUE);
			} else
				mlwrite(KTEX309);

no_mark:
			if (type == TRUE) {
				int status;

				status = do_dired_forwline(FALSE, 1);
				if (!clexec)
					update(TRUE);
			}
			return TRUE;
		}
	}

	return FALSE;
}


/*
------------------------------------------------
	ファイルリスト全部マークする
------------------------------------------------
*/

int do_dired_mark_all(int f, int n)
{
	LINE *lp;
	BUFFER *bp = curbp;

	lp = lforw(bp->b_linep);
	if (llength(lp) >= 8 && !strncmp(lp->l_text + 3, "total", 5))
		lp = lforw(lp);
	while (isnamelist(lp)) {
		if (lp == bp->b_linep)
			break;
		if (*lp->l_text == '*')
			*lp->l_text = '\xa0';
		else if (lgetc(lp, 0) == '\xa0' && lgetc(lp, 6) == 'a' && lgetc(lp, 7) != 's')
			*lp->l_text = '*';
		lp = lforw(lp);
	}
	lchange(WFHARD);
	return TRUE;
}

/*
----------------------------------------
	マークファイルを外す
----------------------------------------
*/

int do_dired_drop_mark(int f, int n)
{
	if (marknum(lforw(curbp->b_linep)) == 0) {
		mlwrite(KTEX310);
		return FALSE;
	}

	if (f == FALSE) {
		if (mlyesno(KTEX318) == FALSE)
			return FALSE;
	}
	killmarklines(TRUE);
	return TRUE;
}

/*
----------------------------------------
	指定拡張子をマーク
----------------------------------------
*/

int do_dired_mark_ext(int f, int n)
{
	char extname[NPAT + 1];

	{
		int status;

		his_disable();
		status = mlreply(KTEX322, extname, NPAT - 1);
		if (status != TRUE)
			return status;
	}

	{
		LINE *lp;

		lchange(WFHARD);
		for (lp = lforw(curbp->b_linep); isnamelist(lp); lp = lforw(lp)) {
			if (lgetc(lp, 0) == '\xa0' && lgetc(lp, 6) == 'a' && lgetc(lp, 7) != 's') {
				if (!strcmpi(extname, get_target_extname(lp)))
					*lp->l_text = '*';
			}
		}
	}

	return TRUE;
}


/*
----------------------------------------
	マークのリストを作る
----------------------------------------
*/

int do_dired_list(int f, int n)
{
	char work[NLINE];

	{
		int clumn, mknum;
		LINE *lp;

		lp = lforw(curbp->b_linep);
		mknum = marknum(lp);
		if (mknum == 0)
			return TRUE;
		bdmarkp->b_flag &= ~BFCHG;
		if (bclear(bdmarkp) != TRUE)
			return FALSE;
		*bdmarkp->b_fname = 0;

		for (clumn = 0; mknum; lp = lforw(lp)) {
			if (isnamelist(lp) == TRUE && *lp->l_text == '*') {
				int i;
				char *w;

				w = work + (30 * clumn);
				strncpy(w, dired_get_target_filename(lp), 30);
				w[30] = 0;
				for (i = strlen(w), w += strlen(w); i < 30; i++)
					*w++ = ' ';
				*w = 0;
				if (clumn == 2) {
					if (addline(bdmarkp, work) == FALSE)
						return FALSE;
					*work = 0;
				}
				if (++clumn > 2)
					clumn = 0;
				mknum--;
			}
		}
		if (clumn != 0) {
			if (addline(bdmarkp, work) == FALSE)
				return FALSE;
		}
	}

	return TRUE;
}

/*
----------------------------------------
	ls の出力ならＮ行進む
----------------------------------------
*/

int do_dired_forwline(int f, int n)
{
	if (n < 0)
		return do_dired_backline(f, -n);

	{
		int status = TRUE;

		while (n--)
			status = isnamelist(lforw(curwp->w_dotp))
			    ? forwline(f, 1) : FALSE;
		thisflag |= CFCPCN;
		return status;
	}
}

/*
----------------------------------------
	ls の出力ならＮ行もどる
----------------------------------------
*/

int do_dired_backline(int f, int n)
{
	if (n < 0)
		return do_dired_forwline(f, -n);

	{
		int status = TRUE;

		while (n--)
			status = isnamelist(lback(curwp->w_dotp))
			    ? backline(f, 1) : FALSE;
		thisflag |= CFCPCN;
		return status;
	}
}

/*
----------------------------------------
	マークファイルをオープン
----------------------------------------
*/

int find_dired_files(int f, int n)
{
	return open_marked_files(FALSE);
}

/*
----------------------------------------
	ファイルを VIEW オープン
----------------------------------------
*/

int view_dired_files(int f, int n)
{
	return open_marked_files(TRUE);
}

/*
========================================
	マークファイルをオープン
========================================
*/

static int open_marked_files(int f)
{
	LINE *lp;
	BUFFER *ffound = NULL;

	lchange(WFHARD);

	for (lp = lforw(curbp->b_linep); isnamelist(lp); lp = lforw(lp)) {
		if (*lp->l_text == '*') {
			char *p;

			p = makefilename(lp);
			if (p) {
				if (getfile2(p, f, &ffound) == FALSE)
					return FALSE;
			}
			*lp->l_text = '\xa0';
			lp->l_text[2] = '#';
		}
	}

	if (ffound == 0)
		return FALSE;

	swbuffer(ffound);
	update(TRUE);

	return TRUE;
}

/*
----------------------------------------
	カーソル行をオープン
----------------------------------------
*/

int find_dired_curfile(int f, int n)
{
	return dired_readfile(FALSE);
}

/*
----------------------------------------
	カーソル行を VIEW オープン
----------------------------------------
*/

int view_dired_curfile(int f, int n)
{
	return dired_readfile(TRUE);
}

/*
========================================
	カーソル行をオープン
========================================
*/

static int dired_readfile(int f)
{
	LINE *lp;
	char *p;
	int status;

	lp = curwp->w_dotp;
	if (!isnamelist(lp) || lgetc(lp, 6) != 'a' || lgetc(lp, 7) == 's')
		return FALSE;

	p = makefilename(lp);
	if (p == 0)
		return FALSE;

	do_dired_forwline(FALSE, 1);
	status = expwild(p, f);
	if (status == TRUE)
		lp->l_text[2] = '#';

	return status;
}

/*
----------------------------------------
	マークファイルをコピー
----------------------------------------
*/

int do_dired_cp(int f, int n)
{
	char namebuf[NLINE];

	{
		LINE *eol;
		LINE *lp;

		eol = curbp->b_linep;
		if (!marknum(lp = lforw(eol))) {
			mlwrite(KTEX310);
			return FALSE;
		}

		{
			int emode;
			char *cpto, *cpfile;

			cpto = gtdirname(KTEX306);
			if (cpto == 0 || findmd(namebuf, cpto) == 0)
				return FALSE;

			lchange(WFHARD);
			disk_flush();

			emode = 0;
			while (lp != eol) {
				int status;

				cpfile = dired_get_target_filename(lp);
				if (!cpfile || *lp->l_text != '*')
					goto next_line;

				status = chk_cp_ok(cpfile, cpto, COPY, &emode);
				if (status != FALSE) {
					if (status == ABORT)
						return FALSE;
					{
						char cmdline[CMDLINESIZE + 1];

						mlwrite(KTEX303, cpfile, KTEX301);
						sprintf(cmdline, "%s %s %s", dired_cp_cmd, makefilename(lp), cpto);
						if (execprog(cmdline, 1) == FALSE) {
							refresh(FALSE, 0);
							update(TRUE);
							mlwrite(KTEX314, dired_cp_cmd, cpfile, cpto);
							return FALSE;
						}
					}
					*lp->l_text = '\xa0';
				}

			  next_line:

				lp = lforw(lp);
			}
			mlwrite(KTEX303, "", KTEX300);
		}
	}

	return TRUE;
}


/*
----------------------------------------
	マークファイルを移動する
----------------------------------------
*/

int do_dired_mv(int f, int n)
{
	return do_dired_mv_1(f, n, TRUE);
}

/*
----------------------------------------
	マークファイルを TRASH へ
----------------------------------------
*/

int do_dired_trash(int f, int n)
{
	return do_dired_mv_1(f, n, FALSE);
}

/*
========================================
	移動本体
========================================
*/

static int do_dired_mv_1(int f, int n, int c)
{
	char namebuf[NLINE];

	{
		LINE *eol;
		LINE *lp;

		eol = curbp->b_linep;
		if (!marknum(lp = lforw(eol))) {
			mlwrite(KTEX310);
			return FALSE;
		}

		{
			int emode;
			char *mvto, *mvfile;

			if (c == TRUE) {
				mvto = gtdirname(KTEX307);
				if (!mvto || findmd(namebuf, mvto) == 0)
					return FALSE;
			} else {
				mvto = *trash ? trash : get_trash_name(trash);
				if (findmd(namebuf, mvto) == 0)
					return FALSE;
				mvto = namebuf;
			}

			lchange(WFHARD);
			disk_flush();

			emode = 0;
			while (lp != eol) {
				mvfile = dired_get_target_filename(lp);
				if (!mvfile || *lp->l_text != '*') {
					lp = lforw(lp);
					continue;
				}

				if (c == TRUE) {
					int status;

					status = chk_cp_ok(mvfile, mvto, MOVE, &emode);
					if (status == ABORT)
						return FALSE;
					if (status == FALSE) {
						lp = lforw(lp);
						continue;
					}
				}

				{
					char cmdline[CMDLINESIZE + 1];

					mlwrite(KTEX304, mvfile, KTEX301);
					sprintf(cmdline, "%s %s %s", dired_mv_cmd, makefilename(lp), mvto);
					if (execprog(cmdline, 1) == FALSE) {
						refresh(FALSE, 0);
						update(TRUE);
						mlwrite(KTEX314, dired_mv_cmd, mvfile, mvto);
						return FALSE;
					}
				}
				{
					LINE *nlp;

					nlp = lforw(lp);
					lfree(lp);
					lp = nlp;
				}
			}
		}
		mlwrite(KTEX304, "", KTEX300);
	}

	return TRUE;
}

/*
----------------------------------------
	TRASH を空にする
----------------------------------------
*/

int do_empty_trash(int f, int n)
{
	if (*trash == 0)
		get_trash_name(trash);

	if (f != FALSE || mlyesno(KTEX312) == TRUE) {
		char cmd_line[NLINE];

		add_slash_if(trash);
		sprintf(cmd_line, "%s %s*", dired_rm_cmd, trash);
		if (system(cmd_line)) {
			mlwrite(KTEX314, dired_rm_cmd, "", "");
			return FALSE;
		}
		if (!dirname_cmp(dired_current_dir, trash))
			killmarklines(FALSE);

		return TRUE;
	}
	return FALSE;
}

/*
========================================
	TRASHが未設定 %TEMP% を参照
========================================
*/

static char *get_trash_name(char *buf)
{
	char *temp;

	temp = getenv("temp");
	if (temp == 0) {
		*buf = *current_dir;
		buf[1] = ':';
		buf[2] = slash;
		strcpy(buf + 3, "TRASH");
	} else {
		strcpy(buf, temp);
		add_slash_if(buf);
		strcat(buf, "TRASH");
	}
	return buf;
}

/*
----------------------------------------
	マークファイル削除
----------------------------------------
*/

int do_dired_rm(int f, int n)
{
	LINE *eol;
	LINE *lp;

	eol = curbp->b_linep;
	if (!marknum(lp = lforw(eol))) {
		mlwrite(KTEX310);
		return FALSE;
	}
	if (mlyesno(KTEX326) != TRUE)
		return FALSE;

	{
		char *rmfile;

		lchange(WFHARD);

		while (lp != eol) {
			int result;

			rmfile = dired_get_target_filename(lp);
			if (!rmfile || *lp->l_text != '*') {
				lp = lforw(lp);
				continue;
			}

			{
				char cmdline[CMDLINESIZE + 1];

				mlwrite(KTEX313, rmfile, KTEX301);
				sprintf(cmdline, "%s %s", dired_rm_cmd, makefilename(lp));
				result = execprog(cmdline, 1);
				if (result == FALSE) {
					mlwrite(KTEX314, dired_rm_cmd, "", "");
					make_dired_buf(dired_ls_cmd, dired_current_dir);
					return FALSE;
				}
			}
			{
				LINE *nlp;

				nlp = lforw(lp);
				lfree(lp);
				lp = nlp;
			}
		}
		mlwrite(KTEX313, "", KTEX300);
	}
	return TRUE;
}

/*
----------------------------------------
	ディレクトリを移動
----------------------------------------
*/

int do_dired_cd(int f, int n)
{
	char *p;
	LINE *lp;

	lp = curwp->w_dotp;
	if (!isnamelist(lp) || lgetc(lp, 5) != 'd')
		return FALSE;

	p = makefilename(lp);
	if (p == 0)
		return FALSE;

	if (!strcmp(p, "."))
		return TRUE;

	make_dired_buf(dired_ls_cmd, p);

	return TRUE;
}

/*
========================================
	ファイル名を求める
========================================
*/

static char *makefilename(LINE *lp)
{
	char *p;
	static char buf[NLINE];

	p = dired_get_target_filename(lp);
	if (p == NULL)
		return NULL;

	strcpy(buf, dired_current_dir);
	strcat(buf, p);
	return buf;
}

/*
========================================
	ファイル名のみ取り出し
========================================
*/

char *dired_get_target_filename(LINE *lp)
{
	static char filename[NFILEN];

	if (!isnamelist(lp))
		return NULL;

	{
		int i;
		int c;
		char *p, *q, *r;

		p = r = lp->l_text + dired_ls_name_pos;
		for (q = filename, i = llength(lp) - dired_ls_name_pos; i > 0; i--) {
			c = *r++;
			if (c == ' ')
				break;

			*q++ = c;
			if (iskanji(c) && i > 1) {
				*q++ = *r++;
				i--;
			} else if (sep(c) == slash || c == ':') {
				q = filename;
				*q = 0;
			}
		}
		*q = 0;
	}

	return filename;
}

/*
========================================
	拡張子のみ取り出し
========================================
*/

static char *get_target_extname(LINE *lp)
{
	char	*p, *q, *period;
	static char	extname[NFILEN];
	int		ch;

	p = dired_get_target_filename(lp);

	for(period = p, q = p; ch = *q; q++) {
		if (iskanji(ch))
			q++;
		else if (ch == '.')
			period = q;
		else if (sep(ch) == slash || ch == ':')
			period = p;
	}
	if (period == p || sep(period[-1]) == slash || period[-1] == ':')
		*extname = 0;
	else
		strcpy(extname, period + 1);

	return extname;
}


/*
========================================
	マークファイルの数を返す
========================================
*/

int marknum(LINE *lp)
{
	int num;

	for (num = 0; isnamelist(lp); lp = lforw(lp)) {
		if (*lp->l_text == '*')
			num++;
	}
	return num;
}

/*
========================================
	ls の 出力なら TRUE
========================================
*/

static int isnamelist(LINE *lp)
{
	return ((*lp->l_text == '\xA0' || *lp->l_text == '*')
		&& lp->l_text[1] == ' '
		&& llength(lp) >= 6)
	? TRUE : FALSE;
}

/*
========================================
	diredバッファ作成
========================================
*/

static int make_dired_buf(char *cmd, char *dirname)
{
	int	total_flag;
	char ls_temp_name[NSTRING];
	char ls_line[NSTRING], cmd_line[NLINE];

	{
		char temp[NLINE];

		if (get_pathname(temp, dirname) == NULL) {
			mlwrite(KTEX323, dirname);
			return FALSE;
		}
		if (!isdexist(temp)) {
			mlwrite(KTEX324, temp);
			return FALSE;
		}
		add_slash_if(temp);
		strcpy(dired_current_dir, temp);
	}

	bdiredp->b_flag &= ~BFCHG;
	{
		int status;

		status = bclear(bdiredp);
		if (status != TRUE)
			return status;
	}
	strcpy(bdiredp->b_fname, dired_current_dir);

	{
		char *tmp;

		tmp = getenv("temp");
		if (tmp == 0)
			*ls_temp_name = 0;
		else {
			strcpy(ls_temp_name, tmp);
			add_slash_if(ls_temp_name);
		}
		cv_bslash_slash(ls_temp_name);
		strcat(ls_temp_name, "shell.___");
	}

	mlwrite(KTEX302, dired_current_dir, KTEX301, "");
	sprintf(cmd_line, "%s %s > %s", cmd, dirname, ls_temp_name);
	system(cmd_line);

	if (dired_cd_flag) {
		cd(dired_current_dir);
		getwd(current_dir);
	}

	if (ffropen(ls_temp_name) != FIOSUC) {
		mlerase();
		return FALSE;
	}
	{
		int length;

		total_flag = 0;
		if (ffgetline(&length) == FIOSUC) {
			total_flag = !strncmp(fline, "total", 5);
			strcpy(ls_line, total_flag ? "\xa0  " : "\xa0    ");
			strcat(ls_line, fline);
			if (addline(curbp, ls_line) != TRUE)
				return FALSE;

			while (ffgetline(&length) == FIOSUC) {
				strcpy(ls_line, "\xa0    ");
				strcat(ls_line, fline);
				if (addline(curbp, ls_line) != TRUE)
					return FALSE;
			}
		}
	}

	ffclose();
	DELETE(ls_temp_name);

	mlwrite(KTEX302, dired_current_dir, KTEX301, KTEX300);

	curbp->b_mode |= MDDIRED;
	upmode();
	gotobob(FALSE, 1);
	gotobol(FALSE, 1);
	if (total_flag)
		forwline(FALSE, 1);
	setccol(dired_ls_name_pos);
	curgoal = getccol(FALSE);
	return TRUE;
}

/*
----------------------------------------
	パス取り出し
----------------------------------------
*/

static char *get_pathname(char *buff, char *dirname)
{
	char *last;
	char *p, *q;
	char ch;

	for(last = q = buff, p = dirname; ch = *p++;) {
		*q++ = ch;
		if (iskanji(ch)) {
			*q++ = *p++;
		} else {
			if (ch == ':' || sep(ch) == slash)
				last = q;
			if (iswc(ch)) {
				q = last;
				break;
			}
		}
	}
	*q = 0;

	return expand_pathname(buff);
}

/*
----------------------------------------
	pushd
----------------------------------------
*/

int pushd(int f, int n)
{
	if (pushd_popd_flag != FALSE)
		return FALSE;

	getwd(current_dir);
	current_drv = *current_dir - 'A';
	target_drv = *dired_current_dir - 'A';
	if (current_drv != target_drv) {
		CHGDRV(target_drv);
		getwd(target_dir);
	}
	cd(dired_current_dir);
	pushd_popd_flag = TRUE;

	return TRUE;
}

/*
----------------------------------------
	popd
----------------------------------------
*/

int popd(int f, int n)
{

	if (pushd_popd_flag != TRUE)
		return FALSE;

	if (current_drv != target_drv) {
		cd(target_dir);
		CHGDRV(current_drv);
	}
	cd(current_dir);
	pushd_popd_flag = FALSE;

	return TRUE;
}

/*
========================================
	ワイルドカード？
========================================
*/

static int iswc(char c)
{
	return (strchr("?*[]{}", (int) c) ? TRUE : FALSE);
}

/*
========================================
	filname は存在する？
========================================
*/

static int isexist(char *filname)
{
	struct stat st;

	if (stat(filname, &st) < 0)
		return FALSE;

	return (st.st_mode & S_IFREG) ? TRUE : FALSE;
}

/*
========================================
	dirname は存在する？
========================================
*/

static int isdexist(char *dirname)
{
	struct stat st;

	if (stat(dirname, &st) < 0)
		return FALSE;

	return (st.st_mode & S_IFDIR) ? TRUE : FALSE;
}

/*
========================================
	同じディレクトリ？
========================================
*/

static int dirname_cmp(char *f1, char *f2)
{
	for (; *f1 && *f2; f1++, f2++) {
		if (sep(toupper(*f1)) != sep(toupper(*f2)))
			break;
	}
	return (*f1 - *f2);
}

/*
========================================
	タイムスタンプを比較
========================================
*/

static int timestamp_cmp(char *file1, char *file2)
{
	struct stat st1, st2;

	if (stat(file1, &st1))
		return FALSE;
	if (stat(file2, &st2))
		return FALSE;

	return (st1.st_mtime > st2.st_mtime) ? TRUE : FALSE;
}

/*
========================================
	ディレクトリ検索・作成
========================================
*/

static char *findmd(char *namebuf, char *f)
{
	int len;

	len = strlen(f);
	if (len > 2 && f[1] == ':') {
		strcpy(namebuf, f);
		if (len > 3)
			delete_slash_if(namebuf);
	} else {
		if (sep(*f) == slash) {
			namebuf[0] = CURDRV() + 'A';
			namebuf[1] = ':';
			strcpy(namebuf + 2, f);
			if (len > 1)
				delete_slash_if(namebuf);
		} else {
			getwd(namebuf);
			add_slash_if(namebuf);
			strcat(namebuf, f);
			if (strlen(namebuf) > 3)
				delete_slash_if(namebuf);
		}
	}

	if (!isexist(namebuf)) {
		if (!isdexist(namebuf)) {
			if (mlyesno(KTEX319) == TRUE) {
				if (mkdir(namebuf) == 0)
					return namebuf;
			}
			mlwrite(KTEX89);
			return NULL;
		} else
			return namebuf;
	}

	mlwrite(KTEX311, namebuf);
	return NULL;
}

/*
========================================
	マークライン削除
========================================
*/

static int killmarklines(int f)
{
	int n;
	LINE *lp;

	lp = lforw(curbp->b_linep);
	n = marknum(lp);
	if (n == 0)
		return TRUE;

	while (n) {
		if (isnamelist(lp)
		    && lgetc(lp, 5) != 'd'
		    && (f == TRUE ? (*lp->l_text == '*') : 0)) {
			LINE *nlp;

			nlp = lforw(lp);
			lfree(lp);
			lp = nlp;
			n--;
		} else
			lp = lforw(lp);
	}

	lchange(WFHARD);
	return TRUE;
}

/*
========================================
	コピー可能か？
========================================
*/

static int chk_cp_ok(char *file, char *copy_to, int mode, int *emode)
{
	char source_filename[NLINE];
	char target_filename[NLINE];

	strcpy(source_filename, dired_current_dir);
	strcat(source_filename, file);
	strcpy(target_filename, copy_to);
	add_slash_if(target_filename);

	if (!dirname_cmp(dired_current_dir, target_filename)) {
		mlwrite(KTEX321);
		return ABORT;
	}
	strcat(target_filename, file);
	if (isdexist(target_filename)) {
		mlwrite(KTEX316, target_filename);
		return FALSE;
	}
	if (isexist(target_filename)) {
		int c;

		if (*emode == 't') {
		  time_stamp:
			if (timestamp_cmp(source_filename, target_filename) != TRUE)
				return FALSE;
			goto done;
		} else if (*emode == 'o') {
			goto done;
		} else {
			mlwrite(KTEX315, file);
			while (1) {
				if (kbdmode != PLAY)
					fep_off();
				c = getkey();
				if (kbdmode != PLAY)
					fep_on();
				if (c == 't' || c == 'T')
					goto time_stamp;
				else if (c == 'o' || c == 'O')
					goto done;
				else if (c == 's' || c == 'S')
					return FALSE;
				else if (c == '!') {
					*emode = 'o';
					goto done;
				} else if (c == '#') {
					*emode = 't';
					goto time_stamp;
				} else if (c == '?')
					mlwrite(KTEX325);
				else if (c == abortc) {
					ctrlg(FALSE, 0);
					return ABORT;
				} else
					H68beep();
			}
		}
	}

done:
	return chk_enable_cp(source_filename, target_filename, mode);
}

/*
========================================
	コピー可能？
========================================
*/

static int chk_enable_cp(char *s_file, char *t_file, int mode)
{
	struct FREEINF t_fbuf;

	if (mode == MOVE && toupper(*s_file) == toupper(*t_file))
		return TRUE;

	if (t_diskflush == 0) {
		t_diskflush = 1;
		DSKFRE(toupper(*t_file) - 'A' + 1, &t_fbuf);
		t_disk_free = t_fbuf.free;
	}

	{
		int s_fsize, t_fsize;

		if (ffropen(s_file) != FIOSUC)
			return ABORT;
		s_fsize = ffsize();
		ffclose();

		if (isexist(t_file) == FALSE) {
			if ((t_disk_free << 10) >= s_fsize) {
				t_disk_free += ((s_fsize >> 10) + (s_fsize & 1023) ? 1 : 0);
				return TRUE;
			}
			mlwrite(KTEX317);
			return ABORT;
		}
		if (ffropen(t_file) != FIOSUC)
			return ABORT;
		t_fsize = ffsize();
		ffclose();

		if ((t_disk_free << 10) + t_fsize >= s_fsize) {
			t_disk_free += ((s_fsize >> 10) + (s_fsize & 1023) ? 1 : 0);
			return TRUE;
		}
	}

	mlwrite(KTEX317);

	return ABORT;
}

/*
========================================
	getfile2
========================================
*/

static int getfile2(char *fname, int vflag, BUFFER ** rbp)
{
	char bname[NBUFN], sname[NFILEN];
	BUFFER *bp;
	WINDOW *wp = curwp;

	makename(bname, fname);
	getpath(sname, fname);
	strcat(sname, bname);

	for (bp = bheadp; bp; bp = bp->b_bufp) {
		if ((bp->b_flag & BFINVS) == 0 && strcmp(bp->b_fname, sname) == 0) {
			int i;
			LINE *lp;

			swbuffer(bp);
			lp = wp->w_dotp;
			i = wp->w_ntrows / 2;
			while (i-- && lback(lp) != curbp->b_linep)
				lp = lback(lp);
			wp->w_linep = lp;
			wp->w_flag |= WFMODE | WFHARD;
			mlwrite(KTEX135);
			if (*rbp == 0)
				*rbp = bp;

			return TRUE;
		}
	}

	bp = bfind(bname, FALSE, 0);
	if (bp)
		unqname(bname);

	bp = bfind(bname, TRUE, 0);
	strcpy(bp->b_fname, sname);
	bp->b_active = FALSE;
	if (vflag)
		bp->b_mode |= MDVIEW;
	if (*rbp == 0)
		*rbp = bp;

	return TRUE;
}
/*
========================================
	パス名をフルパスにする
========================================
*/

char *expand_pathname(char *path)
{
	struct NAMECKBUF namebuf;

	if (NAMECK(path, &namebuf) < 0)
		return NULL;

	strcpy(path, namebuf.drive);
	strcat(path, namebuf.name);
	strcat(path, namebuf.ext);

	return cv_bslash_slash(path);
}
