/*
----------------------------------------
	SETARGV.C: MicroEMACS 3.10
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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/setargv.c,v 1.4 1991/09/19 13:36:28 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static void discard_argv(char **);
static int expand_wild(void);
static int setdata(char *);

/*
========================================
	スラッシュチェック
========================================
*/

int check_slash_if(char *ptr)
{
	int c, pos;

	pos = strlen(ptr);
	if (pos == 0)
		return FALSE;
	c = ptr[pos - 1];
	return (sep(c) == slash && nthctype(ptr, pos) == CT_ANK) ? TRUE : FALSE;
}

/*
========================================
	スラッシュ付加
========================================
*/

char *add_slash_if(char *ptr)
{
	if (check_slash_if(ptr) == FALSE) {
		char	temp[2] = {slash, 0};

		strcat(ptr, temp);
	}
	return ptr;
}

/*
========================================
	スラッシュ除去
========================================
*/

char *delete_slash_if(char *ptr)
{
	if (check_slash_if(ptr) == TRUE)
		ptr[strlen(ptr) - 1] = 0;
	return ptr;
}

/*
========================================
	スラッシュ変換
========================================
*/

char *cv_bslash_slash(char *ptr)
{
	char *p;

	for (p = ptr; *p; p++) {
		if (iskanji(*p))
			p++;
		else if (*p == '\\' || *p == '/')
			*p = slash;
	}
	return ptr;
}

/*
========================================
	パスを得る
========================================
*/

void getpath(char *path, char *orig)
{
	int i;

	strcpy(path, orig);
	for (i = strlen(path) - 1; i >= 0; i--) {
		if (nthctype(path, i + 1) == CT_KJ2)
			continue;
		if (sep(path[i]) == slash || path[i] == ':') {
			path[i + 1] = 0;
			return;
		}
	}
	*path = 0;
	cv_bslash_slash(path);
}

/*
========================================
	File 名、Address 設定
========================================
*/

static int setdata(char *fname)
{
	char *vp;

	vp = (char *) malloc(strlen(fname) + 1);
	if (vp == 0) {
		mlwrite(KTEX94);
		return FALSE;
	}
	strcpy(vp, fname);
	cv_bslash_slash(vp);
	ex_agv[ex_agc++] = vp;
	return TRUE;
}

/*
========================================
	ファイル名解読
========================================
*/

void expname(char *orig, char *after)
{
	struct NAMECKBUF namebuf;
	char t_fname[NFILEN];

	strcpy (t_fname, orig);
	expand_tild (t_fname);
	NAMECK(t_fname, &namebuf);

	sprintf (after, "%s%s%s", namebuf.drive, namebuf.name, namebuf.ext);
	cv_bslash_slash(after);
}

/*
========================================
	ワイルドカード展開
========================================
*/

static int expand_wild(void)
{
	int i, num;
	struct FILBUF file;
	char t_fname[NFILEN];

	for (num = pr_agc, i = 1; i < pr_agc; i++) {
		int val;

		strcpy (t_fname, pr_agv[i]);
		expand_tild (t_fname);
		val = FILES(&file, t_fname, 0x20);
		while (val == 0) {
			num++;
			val = NFILES(&file);
		}
	}

	ex_agc = 0;
	ex_agv = (char **) malloc(sizeof(char *)* (num + 1));
	if (ex_agv == 0) {
		mlwrite(KTEX94);
		return FALSE;
	}
	if (setdata(pr_agv[0]) == FALSE)
		return FALSE;

	for (i = 1; i < pr_agc; i++) {
		char *p = pr_agv[i];

		if (*p == '-' || *p == '@') {
			if (setdata(p) == FALSE)
				return FALSE;
		} else {
			int val;
			int have_wildcard;
			char fname[NFILEN];
			char path[NFILEN];

			expname(p, fname);
			getpath(path, fname);
			have_wildcard = (sindex (fname, "?") != 0);

			num = 0;
			val = FILES(&file, fname, 0x20);
			while (1) {
				switch (val) {
				case -3:
					mlwrite(KTEX226, p);
					goto nextarg;
				case -13:
					mlwrite(KTEX227, p);
					goto nextarg;
				}

				if (val != 0) {
					if (num == 0) {
						if (!have_wildcard) {
							if (setdata(fname) == FALSE)
								return FALSE;
						}
					}
					break;
				} else {
					char temp[NFILEN], ext[NFILEN];

					stcgfe(ext, file.name);

					if (!isignoreext(ext)) {
						if (have_wildcard) {
							strcpy(temp, path);
							strcat(temp, file.name);
							expname(temp, fname);
						}
						if (setdata(fname) == FALSE)
							return FALSE;
						num++;
					}
				}
				val = NFILES(&file);
			}
		nextarg:;
		}
	}

	ex_agv[ex_agc] = 0;

	return TRUE;
}

/*
========================================
	ARGVを捨てる
========================================
*/

static void discard_argv(char **argv)
{
	if (argv) {
		int i;

		for (i = 0; argv[i]; i++)
			free(argv[i]);
		free(argv);
	}
}

/*
========================================
	ワイルドカード処理
========================================
*/

int setargv(int flag)
{
	char temp[NSTRING];

	extern char *_cmdline;

	if (flag == FALSE) {
		discard_argv(ex_agv);
		return expand_wild();
	}
	ex_agc = 0;
	ex_agv = (char **) malloc(sizeof(char *)* (pr_agc + 1));
	if (ex_agv == 0) {
		mlwrite(KTEX94);
		return FALSE;
	}
	if (setdata(pr_agv[0]) == FALSE)
		return FALSE;

	{
		int quote = 0;
		char *p, *q;

		p = _cmdline + 1;
		q = temp;
		while (1) {
			if (iskanji(*p)) {
				*q++ = *p++;
				*q++ = *p++;
			} else {
				switch (*p) {
				case ' ':
					if (quote)
						*q++ = *p++;
					else {
						*q = 0;
						if (setdata(temp) == FALSE)
							return FALSE;
						while (*++p == ' ');
						q = temp;
					}
					break;
				case 0:
					{
						int status;

						*q = 0;
						if (*temp) {
							if (setdata(temp) == FALSE)
								return FALSE;
						}
						ex_agv[ex_agc] = 0;
						pr_agc = ex_agc;
						pr_agv = ex_agv;
						status = expand_wild();
						discard_argv(pr_agv);
						return status;
					}
					break;
				case '"':
					quote = !quote;
					p++;
					break;
				default:
					*q++ = *p++;
					break;
				}
			}
		}
	}
}


/*
========================================
	"~" をホームディレクトリ名に展開
========================================
*/

char *expand_tild (char *name)
{
	char *home;

	if (*name == '~' && (home = getenv ("HOME"))) {
		int name_len, home_len;

		name_len = strlen (name);
		home_len = strlen (home);
		memmove (name + home_len, name + 1, name_len);
		memcpy (name, home, home_len);
	}

	return name;
}
