/*
 * boyer.h -- Boyer-Moore 検索
 *
 * $Id: boyer.h,v 3.1 1992/07/06 02:29:34 candy Exp candy $
 * $Log: boyer.h,v $
 * Revision 3.1  1992/07/06  02:29:34  candy
 * more.c has been entirely rewritten
 *
 * Revision 2.2  1992/06/09  10:52:16  candy
 * gmalloc バグフィクス記念
 *
 * Revision 2.1  1992/06/09  09:52:04  candy
 * CD-ROM 発売記念
 *
 * Revision 1.3  91/02/05  10:49:58  CANDY
 * V1.05
 * more.c (Search): s/bm_search/bm_msearch/ #モーチバイト対応
 * boyer.c boyer.h: bm_msearch() 追加。発見位置としてモーチバイトの中を返さない。
 * 
 * Revision 1.2  90/11/16  19:43:04  CANDY
 * boyer_moore_buffer 改名 bm_pattern_buffer
 * rcsid 付けた
 * 
 * Revision 1.1  90/11/16  19:07:46  CANDY
 * Initial revision
 * 
 */
#ifndef __BOYER_H
#define __BOYER_H

struct bm_pattern_buffer {
	int klen; /* key の長さ */
	char key[256];
	int tbl[256];
};

extern char *bm_compile_pattern(struct bm_pattern_buffer *bmbuf, const char *string);
extern int bm_search(struct bm_pattern_buffer *bmbuf, const char *string);
extern int bm_msearch(struct bm_pattern_buffer *bmbuf, const char *string);

#endif /* __BOYER_H */
