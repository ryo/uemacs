/*
 * dyna.h -- ���I�ɑ傫���Ȃ�o�b�t�@
 * Copyright (C) 1991-1993 by candy
 */
/*
 * $Id: dyna.h,v 1.2 1993/05/15 09:10:36 candy Exp candy $
 */
#ifndef __DYNA_H /* [ */
#define __DYNA_H

/*
 * NAME
 * 	STRUCT_DYNA - �\���̂̐錾(�̈ꕔ�I�I)
 *
 * SYNOPSIS
 * 	struct DYNA STRUCT_DYNA(TYPE);
 *
 * DESCRIPTION
 * 	TYPE �^��v�f�Ƃ��铮�I�ɑ傫���Ȃ�z��������\����
 * 		struct DYNA
 * 	��錾���܂��B
 *
 * EXAPMLE
 * 	struct CHAR_BUF STRUCT_DYNA(char);  �����o�b�t�@
 * 	struct LIST_BUF STRUCT_DYNA(char *);  ������̃��X�g
 * 	�̂悤�ɐ錾���܂��B
 * 	TYPE �̏����ɂ͐����������āA��`
 * 		TYPE * identifier;
 * 	�ɂ����āAidentifier �� TYPE �ւ̃|�C���^�ƂȂ�悤�Ȃ��̂��������܂���B
 * 	�Ⴆ��
 * 		STRUCT_DYNA(char **) �͂��������
 * 		STRUCT_DYNA(int (*)()) �͂��߂ł��B
 */
#define STRUCT_DYNA(TYPE) \
	{ \
	unsigned int more, size, used; \
	TYPE *buf; \
	TYPE *spare; \
	}

/*
 * NAME
 * 	DYNA_IZ - �\���̂̏�����
 *
 * SYNOPSIS
 * 	void DYNA_IZ(struct DYNA *dyna, unsigned int nmore);
 *
 * DESCRIPTION
 * 	�\���̂����������܂��B
 * 	dyna �ɂ� STRUCT_DYNA �Ő錾�����\���̂̃A�h���X��^���܂��B
 * 	nmore �ɂ͈�x�ɑ�������v�f�̐����w�肵�܂��B
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *) list_buf;
 * 	�̎��A
 * 	DYNA_IZ(&list_buf, 64);
 * 	�̂悤�ɏ��������܂��B
 */
#define DYNA_IZ(dyna, nmore) ((void)( \
	(dyna)->more = (nmore), \
	(dyna)->used = (dyna)->size = 0, \
	(dyna)->buf = (void *)0 \
	))

/*
 * NAME
 * 	DYNA_BRK - �z��̊g��
 *
 * SYNOPSIS
 * 	TYPE *DYNA_BRK(struct DYNA *dyna, unsigned int nmore);
 *
 * DESCRIPTION
 * 	�\���̂̔z��� nmore �����g�債�܂��B
 * 	DYNA_NEXT() �������I�Ɏ��s����̂ŕ��ʂ͊֌W����܂���B
 * 	�����Ȃ� NULL �ȊO��Ԃ��A���s�Ȃ� NULL ��Ԃ��܂��B
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *) list_buf;
 * 	DYNA_IZ(&list_buf, 64);
 * 	�̎��A
 * 	if (DYNA_BRK(&list_buf, 16) == NULL)
 * 		goto no_memory;
 * 	�ȂǂƂ��܂��B
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
 * 	DYNA_NEXT - �o�b�t�@�̊g��
 *
 * SYNOPSIS
 * 	TYPE *DYNA_NEXT(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	�z��̐V�����v�f�̃A�h���X��Ԃ��܂��B
 * 	�g�p�̈悪 1 �����܂��B
 * 	����Ȃ��ꍇ�� DYNA_BRK() ���Ăяo���܂��B
 * 	���s�Ȃ� NULL ��Ԃ��܂��B
 *
 * EXAMPLE
 * 	struct LIST_BUF STRUCT_DYNA(char *)list_buf;
 * 	char **next;
 * 	DYNA_IZ(&list_buf, 64);
 * 	�̎��A
 * 	if ((next = DYNA_NEXT(&list_buf)) == NULL)
 * 		goto no_memory;
 * 	*next = malloc(256);
 * 	�ȂǂƂ��܂��B
 */
#define DYNA_NEXT(dyna) ( \
	((dyna)->used >= (dyna)->size && DYNA_BRK(dyna, (dyna)->more) == (void *)0) \
	? (void *)0 \
	: (dyna)->buf + (dyna)->used++ \
	)

/*
 * NAME
 * 	DYNA_ALLOC - �o�b�t�@�̊g��
 *
 * SYNOPSIS
 * 	TYPE *DYNA_ALLOC(struct DYNA *dyna, unsigned int n);
 *
 * DESCRIPTION
 * 	�z��̒��̐V���� n �̘A�������v�f�̃A�h���X��Ԃ��܂��B
 * 	�g�p�̈悪 n �����܂��B
 * 	����Ȃ��ꍇ�� DYNA_BRK() ���Ăяo���܂��B
 * 	���s�Ȃ� NULL ��Ԃ��܂��B
 *
 * EXAMPLE
 * 	struct CHAR_BUF STRUCT_DYNA(char) char_buf;
 * 	char *next;
 * 	DYNA_IZ(&char_buf, 256);
 * 	�̎��A
 * 	if ((next = DYNA_ALLOC(&char_buf, strlen(s) + 1)) == NULL)
 * 		goto no_memory;
 * 	strcpy(next, s);
 * �ȂǂƂ��܂��B
 */
#define DYNA_ALLOC(dyna, nmore) ( \
	((dyna)->used + (nmore) > (dyna)->size && DYNA_BRK(dyna, (dyna)->more + (nmore)) == (void *)0) \
	? ((void *)0)\
	: ((dyna)->used += (nmore), (dyna)->buf + (dyna)->used - (nmore))\
	)

/*
 * NAME
 * 	DYNA_RESET - �o�b�t�@����ɂ���
 *
 * SYNOPSIS
 * 	void DYNA_RESET(struct DYNA *dyna)
 *
 * DESCRIPTION
 * 	�o�b�t�@����ɂ��܂�(�g�p�̈�� 0 �ɂ��܂�)�B
 * 	dyna �ɂ� STRUCT_DYNA �Ő錾�����\���̂̃A�h���X��^���܂��B
 */
#define DYNA_RESET(dyna) ((void)((dyna)->used = 0))

/*
 * NAME
 * 	DYNA_UNGROW - �o�b�t�@�̏k��
 *
 * SYNOPSIS
 * 	void DYNA_UNGROW(struct DYNA *dyna, unsigned int n);
 *
 * DESCRIPTION
 * 	�o�b�t�@�̑傫��(�g�p�̈�)�� 0 �łȂ���΁An �v�f���k�����܂��B
 *
 */
#define DYNA_UNGROW(dyna, n) ((void)((dyna)->used > (n) ? (dyna)->used -= (n) : (dyna)->used = 0))

/*
 * NAME
 * 	DYNA_BUF - �o�b�t�@�̃A�h���X
 *
 * SYNOPSIS
 * 	TYPE *DYNA_BUF(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	�o�b�t�@�̍ŏ��̗v�f�̃A�h���X��Ԃ��܂��B
 * 	DYNA_BRK() �� DYNA_NEXT() �ɂ���Ēl���ω����܂��̂ŁA
 * 	�������Ăяo������͈ȑO�̒l�͖����ɂȂ�܂��B
 */
#define DYNA_BUF(dyna) ((dyna)->buf)

/*
 * NAME
 * 	DYNA_USED - �o�b�t�@�̑傫��
 *
 * SYNOPSIS
 * 	unsigned int DYNA_USED(struct DYNA *dyna);
 *
 * DESCRIPTION
 * 	�o�b�t�@�̑傫����Ԃ��܂��B
 * 	DYNA_IZ() ����� 0 �ŁA
 * 	DYNA_NEXT() ���閈�� + 1 ���܂��B
 */
#define DYNA_USED(dyna) ((dyna)->used)

#endif /* ] !__DYNA_H */
