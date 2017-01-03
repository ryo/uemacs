/*
 * ej.c - English-Japanese Japanese-English Dictinary
 * Copyright (C) 1990-1993 by candy
 */
static char rcsid[] = "$Id: ejw.c,v 1.1 1993/07/23 01:16:44 candy Exp candy $";
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __HUMAN68K__
#include <io.h> /* mktemp, dup, dup2 */
#endif
#include <sys/types.h> /* stat */
#include <sys/stat.h> /* stat */

#include "boyer.h"
#include "dyna.h"
#include "idx.h"

#ifndef NOT_STD_TOLOWER
#define TOLOWER(c) tolower(c)
#else
#define TOLOWER(c) (isupper(c)?tolower(c):(c))
#endif

#ifndef PATHOF
#define PATHOF(s) ((isalpha((unsigned char)(s)[0])&&(s)[1]==':')?((s)+2):(s))
#endif
#ifndef is1kanji
#define is1kanji(c) ((0x81<=(unsigned char)(c))&&((unsigned char)(c)<=0x9f)||(0xe0<=(unsigned char)(c))&&((unsigned char)(c)<=0xfc))
#endif
#define MACRONEXT1(s) ((s)+(is1kanji((s)[0])?(1+((s)[1]!='\0')):((s)[0]!='\0')))
#ifndef isslash
#define isslash(c) ((c)=='/'||(c)=='\\')
#endif

#define FALSE 0
#define TRUE 1
#define SUCCESS 0
#define ERROR (-1)

#ifndef STATIC
#define STATIC static
#endif
/*
 *
 */
STATIC int ignore_case = TRUE; /* -i: 小文字で比較 */
STATIC int before = 0; /* -Bn or -n */
STATIC int after = 0; /* -An on -n */
STATIC int strict = 0; /* -An on -n */
#ifdef EJ
#ifdef __HUMAN68K__
char *std_path = "a:/usr/dict;a:/usr/local/dict";
#else
char *std_path = "/usr/dict;/usr/local/dict";
#endif
#else /* EJ */
extern char std_dict_path[];
#endif /* EJ */

STATIC char *myname;

#ifdef EJ
/*
 * パス名の最後のファイル名のアドレスを返す。
 * パス名が (A:|)(/|)(SUBDIR/)*NAME なら NAME のアドレスを返す。
 */
static char *
last_name_of(const char *path)
{
	const char *lastname = PATHOF(path), *mv;
	mv = lastname;
	while (*mv != '\0') {
		if (isslash(*mv))
			lastname = mv + 1;
		mv = MACRONEXT1(mv);
	}/* while */
	return (char *)lastname;
}/* last_name_of */
#endif /* EJ */

/*
 * モーチバイト文字列 string の最後の文字のアドレスを返す。
 */
static char *
last_char_of(const char *str)
{
	const char *next;
	next = MACRONEXT1(str);
	while (*next != '\0') {
		str = next;
		next = MACRONEXT1(str);
	}/* while */
	return (char *)str;
}/* last_char_of */

/*
 * 文字列を小文字化する。
 */
STATIC char *
strlower(char *d, const char *s)
{
	char *d0 = d;
	while (*s != '\0') {
		if (is1kanji(*s)) {
			*d++ = *s++;
			if (d[-1] != '\0')
				*d++ = *s++;
		}
		else {
			*d++ = TOLOWER((unsigned char)*s);
			s++;
		}
	}/* while */
	*d = '\0';
	return d0;
}/* strlower */

#ifdef EJ
/*
 *
 */
static int
strcmpi(const char *d_, const char *s_)
{
	const unsigned char *d = (const unsigned char *)d_, *s = (const unsigned char *)s_;
	int cp;
	while ((cp = tolower(*d) - tolower(*s)) == 0 && *d != '\0') {
		d++;
		s++;
	}/* while */
	return cp;
}/* strcmpi */
#endif /* EJ */

#define ISDELIM(c) (isspace((unsigned char)(c))||(c)==','||(c)==';')

/*
 * [^,; ]*をコピーする。
 * コピーした文字数を返す。
 */
STATIC int
copy_token(char *dst, const char *src)
{
	char *d0 = dst;
	int ch;
	while ((ch = (unsigned char)*src++) != '\0' && !ISDELIM(ch)) {
		*dst++ = ch;
	}/* while */
	*dst = '\0';
	return dst - d0;
}/* copy_token */

/*
 * src の最初の要素を取り出し、２番目の要素の先頭のアドレスを返す。
 * 要素とは
 * [^,; ]*
 */
STATIC char *
get_first_token(char *dst, const char *src)
{
	while (*src != '\0' && ISDELIM(*src))
		src++;
	src += copy_token(dst, src);
	return (char *)src;
}/* get_first_token */

/*
 *
 */
STATIC int
call_system(const char *str)
{
	int err = system(str);
	if (err < 0) {
		fputs(str, stderr);
		fputc('\n', stderr);
		fprintf(stderr, "%s: system failed\n", myname);
	}
	return err;
}/* call_system */

/*
 *
 */
STATIC int
oputs(const char *s, int (*out)(int))
{
	int err = 0;
	while (err != EOF && *s != '\0') {
		err = out((unsigned char)*s);
		s++;
	}/* while */
	return err;
}/* oputs */

/*
 * ストリームの指定場所から、/^\f/ の前までを別のストリームに書き出す。
 */
STATIC int
seek_print(int (*out)(int), FILE *fp, long offset)
{
	int first = 1;
	char lbuf[1024];
	rewind(fp);
	fseek(fp, offset, SEEK_SET);
	while (fgets(lbuf, sizeof(lbuf), fp) != NULL && lbuf[0] != '\f') {
		if (first) {
			oputs("◆", out);
			first = 0;
		}
		oputs(lbuf, out);
	}/* while */
	return 0;
}/* seek_print*/

/*
 * start から end までのエントリを出力
 */
static int
multi_print(int (*out)(int), FILE *fp, ENTPOOL *lp, long start, long end)
{
	long i;
	struct ent **mv = &lp->ls[start];
	if (start < 0)
		start = 0;
	if (end >= lp->count)
		end = lp->count - 1;
	for (i = start; i <= end; i++) {
		seek_print(out, fp, (*mv)->offset);
		mv++;
	}/* for */
	return 0;
}

/*
 * 指定の語の辞書内容をストリームに書き出す。
 * 見つけた数でなく、出力した数を返す。
 */
STATIC int
find_manual(int (*out)(int), FILE *fp, ENTPOOL *lp, const char *title)
{
	int found = 0, output = 0;
	long idx = lp_search(lp, title), start = 0, end = 0;
	if (idx < 0 && !strict) {
		idx = -idx - 1;
	}
	if (idx >= 0) {
		start = idx - before;
		while (idx < lp->count && strcmp(lp->ls[idx]->u.title, title) == 0) {
			found++;
			idx++;
		}/* while */
		end = idx - 1 + after + (found == 0);
		if (start < 0)
			start = 0;
		if (end >= lp->count)
			end = lp->count - 1;
		multi_print(out, fp, lp, start, end);
		output = end - start + 1;
	}
	return output;
}/* find_manual */

/*
 * 索引ファイルを更新する。
 */
STATIC int
update_index(const char *index, const char *dict)
{
	struct stat stbuf;
	int err = stat(dict, &stbuf);
	if (err >= 0) {
		char buf[FILENAME_MAX * 3];
		sprintf(buf, "ejx %s -o %s", dict, index);
		err = call_system(buf);
	}
	return err;
}/* update_index */

/*
 * 辞書のタイムスタンプが新しかった場合、
 * 索引ファイルを更新する。
 * 辞書が無い時は何もしない。
 */
STATIC int
make_index(const char *index, const char *dict)
{
	time_t doc_time;
	struct stat stbuf;
	int err = stat(dict, &stbuf);
	if (err >= 0) {
		doc_time = stbuf.st_mtime;
		err = stat(index, &stbuf);
		if (err < 0 || stbuf.st_size == 0 || doc_time > stbuf.st_mtime) {
			err = update_index(index, dict);
		}
		else
			err = SUCCESS;
	}
	return err;
}/* make_index */

/*
 * 辞書名から索引名を生成する。
 * i.e. "/usr/dict/ej.dic" --> "/usr/dict/ej.dic.idx"
 */
STATIC char *
index_name(char *dst, const char *base)
{
	strcpy(dst, base);
	strcat(dst, ".idx");
	return dst;
}/* index_name */

/*
 *
 */
STATIC ENTPOOL *ej_list;
STATIC FILE *ej_dictfp;

/*
 * 指定された辞書の索引を作り、辞書をオープンする。
 */
STATIC int
ej_init(const char *dict)
{
	int err = ERROR;
	char index[FILENAME_MAX];
	index_name(index, dict);
	if (make_index(index, dict) == SUCCESS) { /* 索引ファイルの更新をする。 */
		FILE *idx = fopen(index, "rb");
		if (idx != NULL) {
			ej_list = malloc(sizeof(*ej_list));
			if (ej_list != NULL) {
				err = lp_read(ej_list, idx);
				if (err == SUCCESS) {
					ej_dictfp = fopen(dict, "rb");
					if (ej_dictfp != NULL) {
						err = SUCCESS;
					}
					if (err != SUCCESS) {
						lp_bye(ej_list);
					}
				}
				if (err != SUCCESS) {
					free(ej_list);
					ej_list = NULL;
				}
			}
			fclose(idx);
		}
	}
	return err;
}/* ej_init */

#ifdef EJ
/*
 *
 */
STATIC void
ej_end(void)
{
	if (ej_dictfp != NULL) {
		fclose(ej_dictfp);
		ej_dictfp = NULL;
	}
	if (ej_list != NULL) {
		lp_bye(ej_list);
		free(ej_list);
		ej_list = NULL;
	}
}/* ej_end */
#endif /* EJ */

/*
 * 指定されたディレクトリリストから辞書名を作り、初期化する。
 */
STATIC int
ej_init2(const char *path, const char *basename)
{
	int err = ERROR;
	while (err != SUCCESS && *path != '\0') {
		char dict[FILENAME_MAX], *p;
		path = get_first_token(dict, path);
		p = PATHOF(dict);
		if (*p == '\0') { /* "" は "." */
			*p++ = '.';
			*p = '\0';
		}
		p = last_char_of(dict);
		if (isslash(*p))
			*p = '\0';
		strcat(dict, "/");
		strcat(dict, basename);
		err = ej_init(dict);
	}/* while */
	return err;
}/* ej_init2 */

/*
 * 見つけた数を返す。
 */
STATIC int
ej_look_for(int (*out)(int), const char *title)
{
	int found = 0;
	if (ej_dictfp == NULL) {
		ej_init2(std_dict_path, "ej.dic");
	}
	if (ej_dictfp != NULL) {
		char lbuf[64];
		if (ignore_case && strlen(title) + 1 < sizeof(lbuf)) {
			title = strlower(lbuf, title);
		}
		found = find_manual(out, ej_dictfp, ej_list, title);
	}
	return found;
}/* ej_look_for */

/*
 *
 */
struct wpool_t STRUCT_DYNA(char);
STATIC struct wpool_t wpool;

/*
 *
 */
STATIC void
init_output(void)
{
	static int init_done;
	if (!init_done) {
		init_done = 1;
		DYNA_IZ(&wpool, 256);
	}
	DYNA_RESET(&wpool);
}/* init_output */

/*
 *
 */
STATIC char *
output_buffer(void)
{
	return DYNA_BUF(&wpool);
}/* output_buffer */

/*
 *
 */
STATIC int
output(int c)
{
	int err;
	char *next = DYNA_NEXT(&wpool);
	if (next != NULL) {
		*next = c;
		err = c;
	}
	else {
		err = EOF;
	}
	return err;
}/* output */

/*
 * 単語 word を検索し、内容を malloc() で確保した文字列として返す。
 * 後で free() してね。
 */
char *
ej_word(const char *word)
{
	int found;
	char *buf = NULL;
	init_output();
	found = ej_look_for(output, word);
	if (found > 0) {
		if (output('\0') != EOF) {
			buf = malloc(strlen(output_buffer()) + 1);
			if (buf != NULL) {
				strcpy(buf, output_buffer());
			}
		}
	}
	return buf;
}/* ej_word */

/*
 * buf に読み込んだテキストファイルを調べ、
 * *buf の '\n' を '\0' に変換し、
 * 各行へのポインタを持つ配列を malloc() で確保し、それを返す。
 * 後で free() してね。
 */
char **
ej_buf_to_list(char *buf)
{
	char **ls, *mv = buf;
	int nl = 1; /* (# of '\n') + 1 */
	while (*mv != '\0') {
		if (*mv == '\n')
			nl++;
		mv++;
	}/* while */
	ls = malloc(sizeof(*ls) * (nl + 1));
	if (ls != NULL) {
		mv = buf;
		nl = 0; /* # of lines,  */
		ls[nl++] = mv;
		while (*mv != '\0') {
			if (*mv == '\n') {
				ls[nl++] = mv + 1;
				*mv = '\0';
			}
			mv++;
		}/* while */
		if (ls[nl - 1][0] == '\0') {
			nl--;
		}
		ls[nl] = NULL;
	}
	return ls;
}/* ej_buf_to_list */

#ifdef TEST
int
main(int argc, char *argv[])
{
	char lbuf[256];
	while (gets(lbuf) != NULL) {
		char *s = ej_word(lbuf);
		if (s != NULL) {
			fputs(s, stdout);
			free(s);
		}
	}
	return 0;
}
#endif
