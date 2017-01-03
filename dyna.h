/*
 * dyna.h -- 動的に大きくなるバッファ
 * Copyright (C) 1991-1993 by candy
 */
/*
 * $Id: dyna.h,v 1.2 1993/05/15 09:10:36 candy Exp candy $
 */
#ifndef __DYNA_H /* [ */
#define __DYNA_H

/*
 * NAME
 * 	STRUCT_DYNA - 構造体の宣言(の一部！！)
 *
 * SYNOPSIS
 * 	struct DYNA STRUCT_DYNA(TYPE);
 *
 * DESCRIPTION
 * 	TYPE 型を要素とする動的に大きくなる配列を扱う構造体
 * 		struct DYNA
 * 	を宣言します。
 *
 * EXAPMLE
 * 	struct CHAR_BUF STRUCT_DYNA(char);  文字バッファ
 * 	struct LIST_BUF STRUCT_DYNA(char *);  文字列のリスト
 * 	のように宣言します。
 * 	TYPE の書式には制限があって、定義
 * 		TYPE * identifier;
 * 	において、identifier が TYPE へのポインタとなるようなものしか書けません。
 * 	例えば
 * 		STRUCT_DYNA(char **) はいいけれど
 * 		STRUCT_DYNA(int (*)()) はだめです。
 */
#define STRUCT_DYNA(TYPE) \
	{ \
	unsigned int more, size, used; \
	TYPE *buf; \
	TYPE *spare; \
	}

/*
 * NAME
 * 	DYNA_IZ - 構造体の初期化
 *
 * SYNOPSIS
 * 	void DYNA_IZ(struct DYNA *dyna, unsigned int nmore);
 *
 * DESCRIPTION
 * 	構造体を初期化します。
 * 	dyna には STRUCT_DYNA で宣言した構造体のアドレスを与えます。
 * 	nmore には一度に増加する要素の数を指定します。
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *) list_buf;
 * 	の時、
 * 	DYNA_IZ(&list_buf, 64);
 * 	のように初期化します。
 */
#define DYNA_IZ(dyna, nmore) ((void)( \
	(dyna)->more = (nmore), \
	(dyna)->used = (dyna)->size = 0, \
	(dyna)->buf = (void *)0 \
	))

/*
 * NAME
 * 	DYNA_BRK - 配列の拡大
 *
 * SYNOPSIS
 * 	TYPE *DYNA_BRK(struct DYNA *dyna, unsigned int nmore);
 *
 * DESCRIPTION
 * 	構造体の配列を nmore 個だけ拡大します。
 * 	DYNA_NEXT() が自動的に実行するので普通は関係ありません。
 * 	成功なら NULL 以外を返し、失敗なら NULL を返します。
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *) list_buf;
 * 	DYNA_IZ(&list_buf, 64);
 * 	の時、
 * 	if (DYNA_BRK(&list_buf, 16) == NULL)
 * 		goto no_memory;
 * 	などとします。
 */
#define DYNA_BRK(dyna, nmore) ( \
	(dyna)->spare = (dyna)->buf, \
	(dyna)->size += (nmore), \
	(dyna)->buf = realloc((dyna)->buf, (dyna)->size * sizeof((dyna)->buf[0])), \
	(((dyna)->buf != (void *)0) ? (dyna)->buf \
	: ((dyna)->buf = (dyna)->spare, \
		(dyna)->size -= (nmore), \
		(void *)0)) \
	)

/*
 * NAME
 * 	DYNA_NEXT - バッファの拡大
 *
 * SYNOPSIS
 * 	TYPE *DYNA_NEXT(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	配列の新しい要素のアドレスを返します。
 * 	使用領域が 1 増えます。
 * 	足りない場合は DYNA_BRK() を呼び出します。
 * 	失敗なら NULL を返します。
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *)list_buf;
 * 	char **next;
 * 	DYNA_IZ(&list_buf, 64);
 * 	の時、
 * 	if ((next = DYNA_NEXT(&list_buf)) == NULL)
 * 		goto no_memory;
 * 	*next = malloc(256);
 * 	などとします。
 */
#define DYNA_NEXT(dyna) ( \
	((dyna)->used >= (dyna)->size && DYNA_BRK(dyna, (dyna)->more) == (void *)0) \
	? (void *)0 \
	: (dyna)->buf + (dyna)->used++ \
	)

/*
 * NAME
 * 	DYNA_ALLOC - バッファの拡大
 *
 * SYNOPSIS
 * 	TYPE *DYNA_ALLOC(struct DYNA *dyna, unsigned int n);
 *
 * DESCRIPTION
 * 	配列の中の新しい n 個の連続した要素のアドレスを返します。
 * 	使用領域が n 増えます。
 * 	足りない場合は DYNA_BRK() を呼び出します。
 * 	失敗なら NULL を返します。
 *
 * EXAMPLE
 * 	struct CHAR_BUF STRUCT_DYNA(char) char_buf;
 * 	char *next;
 * 	DYNA_IZ(&char_buf, 256);
 * 	の時、
 * 	if ((next = DYNA_ALLOC(&char_buf, strlen(s) + 1)) == NULL)
 * 		goto no_memory;
 * 	strcpy(next, s);
 * などとします。
 */
#define DYNA_ALLOC(dyna, nmore) ( \
	((dyna)->used + (nmore) > (dyna)->size && DYNA_BRK(dyna, (dyna)->more + (nmore)) == (void *)0) \
	? ((void *)0)\
	: ((dyna)->used += (nmore), (dyna)->buf + (dyna)->used - (nmore))\
	)

/*
 * NAME
 * 	DYNA_RESET - バッファを空にする
 *
 * SYNOPSIS
 * 	void DYNA_RESET(struct DYNA *dyna)
 *
 * DESCRIPTION
 * 	バッファを空にします(使用領域を 0 にします)。
 * 	dyna には STRUCT_DYNA で宣言した構造体のアドレスを与えます。
 */
#define DYNA_RESET(dyna) ((void)((dyna)->used = 0))

/*
 * NAME
 * 	DYNA_UNGROW - バッファの縮小
 *
 * SYNOPSIS
 * 	void DYNA_UNGROW(struct DYNA *dyna, unsigned int n);
 *
 * DESCRIPTION
 * 	バッファの大きさ(使用領域)が 0 でなければ、n 要素分縮小します。
 *
 */
#define DYNA_UNGROW(dyna, n) ((void)((dyna)->used > (n) ? (dyna)->used -= (n) : (dyna)->used = 0))

/*
 * NAME
 * 	DYNA_BUF - バッファのアドレス
 *
 * SYNOPSIS
 * 	TYPE *DYNA_BUF(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	バッファの最初の要素のアドレスを返します。
 * 	DYNA_BRK() や DYNA_NEXT() によって値が変化しますので、
 * 	それらを呼び出した後は以前の値は無効になります。
 */
#define DYNA_BUF(dyna) ((dyna)->buf)

/*
 * NAME
 * 	DYNA_USED - バッファの大きさ
 *
 * SYNOPSIS
 * 	unsigned int DYNA_USED(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	バッファの大きさを返します。
 * 	DYNA_IZ() 直後は 0 で、
 * 	DYNA_NEXT() する毎に + 1 します。
 */
#define DYNA_USED(dyna) ((dyna)->used)

#endif /* ] !__DYNA_H */
