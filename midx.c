/*
 * midx.c -- キー検索
 * Copyright (C) 1991-1993 by candy
 */
static char rcsid[] = "$Id: midx.c,v 1.2 1993/01/25 04:16:34 candy Exp candy $";
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "idx.h"

#define FALSE 0
#define TRUE 1
#define SUCCESS 0
#define ERROR (-1)

#ifndef STATIC
#define STATIC static
#endif
/*
 * リスト解放
 */
void
lp_bye(ENTPOOL *lp)
{
	if (lp != NULL) {
		if (lp->tbuf!= NULL)
			free(lp->tbuf);
		if (lp->lsbuf != NULL)
			free(lp->lsbuf);
		if (lp->ls != NULL)
			free(lp->ls);
	}
}/* lp_bye */

/*
 * インデクスファイルを読む。
 * ファイルの形式
 * １．ヘッダ
 *   struct head;
 * ２．エントリ領域 (エントリ総数個分)
 *   struct ent;
 * ３．文字列領域 (文字列領域バイト数 bytes)
 *   文字列 (ASCIIZ) の連続
 */
int
lp_read(ENTPOOL *lp, FILE *fp)
{
	int err = ERROR;
	struct head h;
	if (fread(&h, sizeof(h), 1, fp) == 1) {
		long cnt = h.count;
		lp->count = cnt;
		/* lp_bye() できるように全て初期化する */
		lp->ls = malloc(cnt * sizeof(*lp->ls));
		lp->lsbuf = malloc(cnt * sizeof(*lp->lsbuf));
		lp->tbuf = malloc(h.tsize * sizeof(*lp->tbuf));
		if (lp->ls != NULL && lp->lsbuf != NULL && lp->tbuf != NULL
		  && fread(lp->lsbuf, sizeof(*lp->lsbuf), cnt, fp) == cnt
		  && fread(lp->tbuf, sizeof(char), h.tsize, fp) == h.tsize) {
			long i;
			char *base = lp->tbuf;
			for (i = 0; i < cnt; i++) {
				lp->ls[i] = &lp->lsbuf[i];
				lp->lsbuf[i].u.title = base + lp->lsbuf[i].u.tidx;
			}/* for */
			err = SUCCESS;
		}
		else {
			lp_bye(lp);
		}
	}
	return err;
}/* lp_read */


/*
 *
 */
static void *
bnsearch(const void *key, const void *base, size_t cnt, size_t size, int (*cmp)(const void *d, const void *s))
{
	if (cnt == 0)
		return (void *)base;
	else {
		size_t half = cnt / 2;
		const void *mid = (const char *)base + half * size;
		int cp = (*cmp)(key, mid);
		if (cp == 0)
			return (void *)mid;
		if (cp < 0)
			return bnsearch(key, base, half, size, cmp);
		else
			return bnsearch(key, (const char *)mid + size, cnt - half - 1, size, cmp);
	}
}/* bnsearch */

/*
 * サーチ・クイックソート用の比較関数。
 */
static int
comp(const void *dst, const void *src)
{
	const struct ent * const *d = dst, * const *s = src;
	int cmp = (unsigned char)(*d)->u.title[0] - (unsigned char)(*s)->u.title[0];
	if (cmp == 0) {
		cmp = strcmp((*d)->u.title + 1, (*s)->u.title + 1);
	}
	return cmp;
}/* comp */

/*
 * タイトルのサーチ
 * 発見した位置または－１を返す。
 */
int
lp_search(ENTPOOL *lp, const char *title)
{
	struct ent k_, *k = &k_, **ent;
	long idx = -1;
	k->offset = 0;
	k->u.title = (char *)title;
	ent = bnsearch(&k, lp->ls, lp->count, sizeof(*lp->ls), comp);
	idx = ent - lp->ls;
	if (idx >= lp->count || comp(&k, ent) != 0) {
		idx = -idx - 1;
	}
	else {
		while (idx > 0 && comp(&k, &lp->ls[idx - 1]) == 0)
			idx--;
	}
	return idx;
}/* lp_search */
