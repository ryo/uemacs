/*
----------------------------------------
	AUTOLOAD.C: MicroEMACS 3.10
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

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/autoload.c,v 1.2 1991/09/01 02:41:50 SALT Exp $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static void addautolist(char *, char *);
static ALOAD *searchautolist(char *);

/*
========================================
	オートロードリスト検索
========================================
*/

static inline ALOAD *searchautolist(char *macro)
{
	ALOAD *ptr;

	for (ptr = autolist; ptr; ptr = ptr->next) {
		if (!strcmp(ptr->macro, macro))
			return ptr;
	}
	return 0;
}

/*
----------------------------------------
	オートロード指定
----------------------------------------
*/

int autoload(int f, int n)
{
	int status;
	char *mac;
	char file[NFILEN], outseq[NSTRING], bufn[NBUFN];

	strcpy(outseq, KTEX249);
	mac = complete(outseq, 0, 0, CMP_MACRO, NBUFN - 2);
	if (mac == 0)
		return FALSE;
	strcat(outseq, mac);
	strcat(outseq, " ");
	his_disable();
	status = mlreply(outseq, file, NFILEN);
	if (status != TRUE)
		return status;
	sprintf(bufn, "[%s]", mac);
	if (bfind(bufn, FALSE, 0) == 0) {
		BUFFER *bp;

		bp = bfind(bufn, TRUE, BFINVS);
		bp->b_active = FALSE;
		addautolist(mac, file);
	}
	return TRUE;
}

/*
========================================
	オートロードリストへの追加
========================================
*/

static void addautolist(char *macro, char *file)
{
	ALOAD *ptr;

	ptr = searchautolist(macro);
	if (ptr == 0) {
		ptr = (ALOAD *) malloc(sizeof(ALOAD));
		ptr->next = autolist;
		autolist = ptr;
	}
	strcpy(ptr->macro, macro);
	strcpy(ptr->file, file);
}

/*
========================================
	ロード及び実行
========================================
*/

int loadexec(char *macro, int *ecode)
{
	ALOAD *aload;

	*ecode = 0;
	aload = searchautolist(macro);
	if (aload) {
		int status, savearg;
		char *fspec;

		fspec = flook(aload->file, TRUE);
		if (fspec == 0) {
			if (clexec == FALSE)
				mlwrite(KTEX214, aload->file);
			return FALSE;
		}
		savearg = cmdarg;
		status = dofile(fspec);
		cmdarg = savearg;
		return status;
	}
	*ecode = -1;
	return FALSE;
}
