/*
========================================
	INIT.C: MicroEMACS 3.10
========================================
*/

#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "evar.h"
#include "elang.h"
#include "ebind.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/init.c,v 1.5 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用する関数の定義
========================================
*/

static void var_fnc_init(void);
static int scmp(const void *, const void *);

/*
========================================
	変数の初期化
========================================
*/

void varinit(void)
{
	if (_dump_flag <= 0) {
		var_fnc_init();
		init_env_in_word_set();
		init_com_in_word_set();
	}
}

/*
========================================
	文字列比較
========================================
*/

static int scmp(const void *s1, const void *s2)
{
	char *src = *((char **) s1);
	char *dst = *((char **) s2);

	while (*src++ == *dst) {
		if (*dst++ == 0)
			return 0;
	}
	return (src[-1] - *dst);
}

/*
========================================
	関数の初期化
========================================
*/

static void var_fnc_init(void)
{
	int n;
	UFUNC *sptr, *vptr, *eptr;

	n = 0;
	sptr = (UFUNC *) (((int) &fnc_word_table_top) + sizeof(int));
	eptr = (UFUNC *) & fnc_word_table_end;

	for (vptr = sptr; vptr < eptr; vptr++) {
		if (*vptr->f_name)
			n++;
	}
	func_name = (char **) malloc(sizeof(char *) * n);
	if (func_name == 0) {
		mlwrite(KTEX94);
		meexit(-1);
		return;
	}
	for (n = 0, vptr = sptr; vptr < eptr; vptr++) {
		if (*vptr->f_name)
			func_name[n++] = vptr->f_name;
	}
	qsort(func_name, n, sizeof(char *), scmp);
	nfuncs = n;
}

/*
========================================
	キーマップの初期化
========================================
*/

#define	NHASH	1024
#define	HASH(x)	({int _x = (x); (_x + (_x >> 3)) & (NHASH -1 );})

void keyinit(void)
{
	typedef struct fhash {
		int				(*f_func)(int, int);
		NBIND			*f_nbind;
		struct fhash	*f_next;
	} fhash;

	fhash **hash_head;
	fhash *hash_body;
	int i;
	int	synonim = 0;

	if (_dump_flag > 0)
		return;

	for (i = 0; i < NKEYMAPS; i++) {
		int		nmalloc;

		nmalloc = bindtab[i].bd_size + ((i == 0) ? NFBINDS0 : NFBINDS1);

		keytab[i] = (KEYTAB *)malloc(sizeof(KEYTAB) * nmalloc);
		if (keytab[i] == 0) {
			mlwrite(KTEX94);
			meexit(-1);
			return;
		}
	}

	{
		NBIND	*nb;
		fhash	*entry;

		hash_head = (fhash **)malloc(sizeof(fhash *) * NHASH);
		hash_body = (fhash *)malloc(sizeof(fhash) * numfunc);
		if (hash_head == 0 || hash_body == 0) {
			mlwrite(KTEX94);
			meexit(-1);
			return;
		}

		{
			int	i;

			for(i = 0; i < NHASH; i++)
				hash_head[i] = 0;
		}

		nb = command_table;
		entry = hash_body;
		for(i = 0; i < numfunc; i++) {
			fhash	**hhp;
			int	func;

			func = (int)nb->n_func;
			hhp = &hash_head[HASH(func)];
			while (*hhp)
				hhp = &((*hhp)->f_next);

			*hhp = entry;
			entry->f_func  = (int (*)(int, int))func;
			entry->f_nbind = nb;
			entry->f_next  = 0;

			nb++;
			entry++;
		}
	}

	for (i = 0; i < NKEYMAPS; i++) {
		int		j, size;
		KEYTAB	*kptr;
		BINDTAB	*btp;

		btp = bindtab[i].bd_tab;
		size = bindtab[i].bd_size;
		kptr = keytab[i];

		if (btp && size > 0) {
			for (j = 0; j < size; j++, btp++) {
				NBIND	*nb;
				fhash	*hp;
				int		func;

				func = (int)btp->bind_fp;
				hp = hash_head[HASH(func)];
				while (hp) {
					if (hp->f_func == (int (*)(int, int))func)
						break;
					hp = hp->f_next;
					synonim++;
				}
				if (hp) {
					nb = hp->f_nbind;
					if (nb->n_func == (int (*)(int, int))func) {
						kptr->k_code = btp->bind_code;
						kptr->k_rest = nb->n_rest;
						kptr->k_type = BINDFNC;
						kptr->k_ptr.fp = (int (*)(int, int))func;
						kptr++;
					}
				}
			}
		}
		kptr->k_type = BINDNUL;
		kptr->k_rest = 0;
		kptr->k_code = 0;
		kptr->k_ptr.fp = 0;
	}

	free(hash_body);
	free(hash_head);
}

#undef NHASH
