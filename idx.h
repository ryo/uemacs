/*
 * idx.h
 * Copyright (C) 1991-1993 candy
 * $Id: idx.h,v 1.2 1993/05/15 10:01:02 candy Exp candy $
 */
#ifndef __IDX_H
#define __IDX_H
/*
 * dyna.h ���K�v
 */
#ifndef __DYNA_H
#include "dyna.h"
#endif /* !__DYNA_H */

/*
 * �^�C�g���Ƃ��̃I�t�Z�b�g��ێ�����B
 */
#define ENTMORE 8192
struct ENTRY { /* �^�C�g���G���g���\���� */
	long offset;
	char *title;
};

typedef struct STRUCT_DYNA(struct ENTRY *) ENTLIST;

extern void * F_DYNA_IZ(ENTLIST *lp);
extern void list_bye(ENTLIST *lp);
extern struct ENTRY * ent_new_set(ENTLIST *lp, const char *title, long offset);
extern struct ENTRY ** list_sort(ENTLIST *lp);


/*
 * �C���f�N�X�t�@�C���̃w�b�_
 */
struct head {
	long count; /* �G���g������ */
	long tsize; /* ������̈�o�C�g�� */
};

/*
 * �^�C�g���Ƃ��̃I�t�Z�b�g��ێ�����B
 */
struct ent { /* �^�C�g���G���g���\���� */
	long offset;
	union {
		long tidx;
		char *title;
	} u;
} ent;

typedef struct ENTPOOL {
	long count; /* �^�C�g���̑��� */ 
	struct ent **ls; /* �^�C�g���G���g���\���̂ւ̃|�C���^�̔z��̃A�h���X */
	struct ent *lsbuf; /* �G���g���͎��O */
	char *tbuf; /* �^�C�g�������O */
} ENTPOOL;

extern void lp_bye(ENTPOOL *lp);
extern int lp_read(ENTPOOL *lp, FILE *fp);
extern int lp_search(ENTPOOL *lp, const char *title);

#endif /* !__IDX_H */
