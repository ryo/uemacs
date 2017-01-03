/*
 * idx.h
 * Copyright (C) 1991-1993 candy
 * $Id: idx.h,v 1.2 1993/05/15 10:01:02 candy Exp candy $
 */
#ifndef __IDX_H
#define __IDX_H
/*
 * dyna.h が必要
 */
#ifndef __DYNA_H
#include "dyna.h"
#endif /* !__DYNA_H */

/*
 * タイトルとそのオフセットを保持する。
 */
#define ENTMORE 8192
struct ENTRY { /* タイトルエントリ構造体 */
	long offset;
	char *title;
};

typedef struct STRUCT_DYNA(struct ENTRY *) ENTLIST;

extern void * F_DYNA_IZ(ENTLIST *lp);
extern void list_bye(ENTLIST *lp);
extern struct ENTRY * ent_new_set(ENTLIST *lp, const char *title, long offset);
extern struct ENTRY ** list_sort(ENTLIST *lp);


/*
 * インデクスファイルのヘッダ
 */
struct head {
	long count; /* エントリ総数 */
	long tsize; /* 文字列領域バイト数 */
};

/*
 * タイトルとそのオフセットを保持する。
 */
struct ent { /* タイトルエントリ構造体 */
	long offset;
	union {
		long tidx;
		char *title;
	} u;
} ent;

typedef struct ENTPOOL {
	long count; /* タイトルの総数 */ 
	struct ent **ls; /* タイトルエントリ構造体へのポインタの配列のアドレス */
	struct ent *lsbuf; /* エントリは自前 */
	char *tbuf; /* タイトルも自前 */
} ENTPOOL;

extern void lp_bye(ENTPOOL *lp);
extern int lp_read(ENTPOOL *lp, FILE *fp);
extern int lp_search(ENTPOOL *lp, const char *title);

#endif /* !__IDX_H */
