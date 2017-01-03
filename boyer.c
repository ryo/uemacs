/*
 * boyer.c -- Boyer-Moore 検索
 */
static char rcsid[] = "$Id: boyer.c,v 3.1 1992/10/14 00:13:42 candy Exp candy $";
/*
 * $Log: boyer.c,v $
 * Revision 3.1  1992/10/14  00:13:42  candy
 * more.c has been entirely rewritten
 *
 * Revision 2.3  1992/10/13  04:34:56  candy
 * is2sjis 使用
 *
 * Revision 2.2  1992/06/09  10:52:12  candy
 * gmalloc バグフィクス記念
 *
 * Revision 2.1  1992/06/09  10:02:24  candy
 * CD-ROM 発売記念
 *
 * Revision 1.3  91/02/05  11:07:38  CANDY
 * V1.05
 * more.c (Search): s/bm_search/bm_msearch/ #モーチバイト対応
 * boyer.c boyer.h: bm_msearch() 追加。発見位置としてモーチバイトの中を返さない。
 * 
 * Revision 1.2  90/11/16  19:45:22  CANDY
 * boyer_moore_buffer 改名 bm_pattern_buffer
 * rcsid 付けた
 * 
 * Revision 1.1  90/11/16  19:15:26  CANDY
 * Initial revision
 * 
 */
#include <string.h>
#include "boyer.h"
#define NULL 0
#define FALSE 0
#define TRUE 1
/*
 * 文字列 string を Boyer-Moore 検索用に設定する。
 * エラーならエラーメッセージを返す。
 */
extern char *
bm_compile_pattern(struct bm_pattern_buffer *bmbuf, const char *string)
{
	int i, len;
	int *tbl;

	tbl = bmbuf->tbl;
	len = strlen(string);
	if (len >= 256) {
		return "too long pattern";
	}
	bmbuf->klen = len;
	strcpy(bmbuf->key, string);
	for (i = 0; i < 256; i++)
		*tbl++ = len;
	tbl = bmbuf->tbl;
	for (i = 1; i < len; i++)
		tbl[(unsigned char)*string++] = len - i;
	return NULL;
}/* bm_compile_pattern */

/*
 * 文字列 string から bmbuf に設定された検索文字列を検索する。
 * 最初に現れる位置を返す。
 * 見つからない時は -1 を返す。
 */
extern int
bm_search(struct bm_pattern_buffer *bmbuf, const char *string)
{
	int klen, found;
	const char *key, *term, *org_string;
	int *tbl;

	klen = bmbuf->klen;
	if (klen == 0)
		return 0;
	org_string = string;
	key = bmbuf->key;
	tbl = bmbuf->tbl;
	term = string;
	while (*term++ != '\0');
	term--;
	string += klen - 1;
	key += klen - 1;
	found = FALSE;
	while (string < term && !found) {
		if (*string == *key) {
			int i;
			const char *mv, *src;
			found = TRUE;
			i = klen - 1;
			mv = string;
			src = key;
			while (found && i > 0) {
				found = *--mv == *--src;
				i--;
			}/* while */
		}
		if (!found) {
			string += tbl[(unsigned char)*string];
		}
	}/* while */
	if (found)
		return string - (klen - 1) - org_string;
	else
		return -1;
}/* bm_search */

/*
 *
 */
#define is1kanji(c) ((0x81<=(unsigned char)(c))&&((unsigned char)(c)<=0x9f)||(0xe0<=(unsigned char)(c))&&((unsigned char)(c)<=0xfc))
/*
 * str[n] が S-JIS の２バイトコードの１バイト目なら non 0 を返す。
 * C-MAGAZINE 1992-5 p.77
 */
static int
is1sjis(const char *str, unsigned int n)
{
	if (!is1kanji(str[n]))
		return 0;
	else {
		int pre = n; /* pre は signed でないとだめ */
		/* n == 0 の時も正しい */
		while (--pre >= 0 && is1kanji(str[pre]))
			;
		return (pre ^ n) & 1;
	}
}/* is1sjis */

/*
 * str[n] が S-JIS の２バイトコードの２バイト目のなら non 0 を返す。
 */
#define is2sjis(s,n) ((n)!=0&&is1sjis(s,(n)-1))

/*
 *
 */
extern int
bm_msearch(struct bm_pattern_buffer *bmbuf, const char *string)
{
	const char *mv = string;
	int pos;
	while ((pos = bm_search(bmbuf, mv)) >= 0 && is2sjis(mv, pos)) { /* 見つかった位置がモーチバイトの中 */
		mv += pos + 1;
	}/* while */
	if (pos < 0)
		return pos;
	else
		return mv - string + pos;
}/* bm_msearch */
