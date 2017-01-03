/*
----------------------------------------
	MALLOC.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/nalloc.c,v 1.2 1992/02/15 12:38:28 SALT Exp SALT $'\n""	even\n");

/*
========================================
	マクロ定義
========================================
*/

#define	ALIGN_SIZE	2
#define	ALIGN(x)	(((size_t)(x) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))
#define	FREESIZE(x)	((size_t)(x)->bh_next - (size_t)(x)->bh_body - (x)->bh_size)
#define	MAXNBLOCKS	256
#define	GROWSTEP	4096
#define	HUGEBLOCK	(GROWSTEP / 2 - sizeof(chead) - sizeof(bhead) * 2)

/*
========================================
	Header 定義
========================================
*/

typedef struct bhead {
	struct bhead	*bh_prev;
	struct bhead	*bh_next;
	size_t			 bh_size;
	char			 bh_body[0];
} bhead;

typedef struct chead {
	struct chead	*ch_prev;
	struct chead	*ch_next;
	size_t			 ch_size;
	long			 ch_nblocks;
	struct bhead	*ch_free1st;
	size_t			 ch_free1st_sz;
	struct bhead	*ch_free2nd;
	size_t			 ch_free2nd_sz;
	struct bhead	 ch_bhead[0];
} chead;

/*
----------------------------------------
	STACK_SIZE, HEAP_SIZE 指定
----------------------------------------
*/

__asm("		.xdef	_STACK_SIZE\n"
      "		.xdef	_HEAP_SIZE\n"
      "_STACK_SIZE	equ	24576\n"
      "_HEAP_SIZE	equ	0\n");

/*
========================================
	使用関数の定義
========================================
*/

static void *_nalloc_sbrk(long size);
static void free_check(void);
static chead *create_core(size_t n);
static chead *grow_core(chead *cp, size_t n);
static void free_core(chead *cp);

/*
========================================
	使用変数の定義
========================================
*/

void *(*_sbrk)(long) = (void *(*)(long))_nalloc_sbrk;

static chead *last_free_core;

static void *heap_end;
static void *virtual_heap_end;
static void *data_space_start;
static int lim_data;
static int warnlevel;

chead *_core_head;
chead *_core_tail;

/*
----------------------------------------
	moreheap
----------------------------------------
*/

int moreheap(size_t n)
{
	void *new_heap_end;

	new_heap_end = virtual_heap_end + n;
	if (heap_end < new_heap_end) {
		char *p;

		p = sbrk((long)new_heap_end - (long)heap_end);
		if (p == (char *) -1)
			return -1;

		heap_end = sbrk(0);
	}
	return 0;
}

/*
----------------------------------------
	_nalloc_sbrk
----------------------------------------
*/

static void *_nalloc_sbrk(long size)
{
	void *prev_ptr, *new_virtual_heap_end;

	if (lim_data == 0) {
		extern int sizmem(void);

		lim_data = (size_t)(sizmem() << 2);
		data_space_start = sbrk(0);
		virtual_heap_end = heap_end = sbrk(0);
		warnlevel = 0;
	}

	prev_ptr = virtual_heap_end;
	new_virtual_heap_end = virtual_heap_end + size;
	if (heap_end < new_virtual_heap_end) {
		void *cp;

		cp = sbrk((long)new_virtual_heap_end - (long)heap_end);
		if (cp == (char *)-1)
			return cp;
		heap_end = sbrk(0);
	}
	virtual_heap_end = new_virtual_heap_end;

	return prev_ptr;
}

/*
----------------------------------------
	free_check
----------------------------------------
*/

static void free_check(void)
{
	void *p;
	long size;

	p = _sbrk(0);
	size = (long)p - (long)data_space_start;
	malloc_sbrk_used = size;
	malloc_sbrk_unused = lim_data - size;
	if (memrflag) {
		malloc_rating = malloc_sbrk_used * 100 / lim_data;
		if (malloc_rating != malloc_old_rating) {
			WINDOW *wp;
				malloc_old_rating = malloc_rating;
			for (wp = wheadp; wp; wp = wp->w_wndp)
				wp->w_flag |= WFMODE;
		}
	}

	switch (warnlevel) {
	case 0:
		if (size > (lim_data / 4) * 3) {
			warnlevel++;
			warnfunc(KTEX237);
		} else
			warnlevel = 0;
	case 1:
		if (size > (lim_data / 20) * 17) {
			warnlevel++;
			warnfunc(KTEX238);
		}
	case 2:
		if (size > (lim_data / 20) * 19) {
			warnlevel++;
			warnfunc(KTEX239);
		}
	}
}

/*
----------------------------------------
	create_core
----------------------------------------
*/

static chead *create_core(size_t n)
{
	chead *np;
	bhead *head;
	bhead *tail;
	size_t sz;

	sz = (n + (GROWSTEP - 1)) & ~(GROWSTEP - 1);

	np = _sbrk(0);
	if (_sbrk(sz) == (char *)-1)
		return NULL;

	head = np->ch_bhead;
	tail = (void *)np + sz - sizeof(bhead);

	head->bh_prev = NULL;
	head->bh_next = tail;
	head->bh_size = 0;

	tail->bh_prev = head;
	tail->bh_next = NULL;
	tail->bh_size = 0;

	np->ch_prev       = NULL;
	np->ch_next       = NULL;
	np->ch_size       = sz;
	np->ch_nblocks    = 0;
	np->ch_free1st    = head;
	np->ch_free1st_sz = FREESIZE(head);
	np->ch_free2nd    = NULL;
	np->ch_free2nd_sz = 0;

	free_check();

	return np;
}

/*
----------------------------------------
	grow_core
----------------------------------------
*/

static chead *grow_core(chead *cp, size_t n)
{
	bhead *prev;
	bhead *tail;
	bhead *newtail;
	size_t sz;

	sz = (n + (GROWSTEP - 1)) & ~(GROWSTEP - 1);
	if ((void *)cp + cp->ch_size != _sbrk(0))
		return NULL;

	if (_sbrk(sz) == (char *)-1)
		return NULL;

	tail    = (void *)cp + cp->ch_size - sizeof(bhead);
	newtail = (void *)tail + sz;
	prev    = tail->bh_prev;

	*newtail = *tail;
	cp->ch_size += sz;
	prev->bh_next = newtail;

	if (cp->ch_free2nd == prev) {
		cp->ch_free2nd    = cp->ch_free1st;
		cp->ch_free2nd_sz = cp->ch_free1st_sz;
	}
	cp->ch_free1st    = prev;
	cp->ch_free1st_sz = FREESIZE(prev);

	free_check();

	return cp;
}

/*
----------------------------------------
	free_core
----------------------------------------
*/

static void free_core(chead *cp)
{
	if ((void *)cp + cp->ch_size != _sbrk(0))
		return;

	if (cp == last_free_core)
		last_free_core = NULL;

	if (cp->ch_prev) {
		_core_tail = cp->ch_prev;
		cp->ch_prev->ch_next = NULL;
	} else {
		_core_head = NULL;
		_core_tail = NULL;
	}

	_sbrk(-cp->ch_size);

	free_check();
}

/*
----------------------------------------
	malloc
----------------------------------------
*/

void *malloc(size_t n)
{
	chead *cp;
	bhead *ap;
	size_t sz;

	n = ALIGN(n);
	sz = ALIGN(sizeof(bhead)) + n;

	if (_core_head == NULL) {
		if ((cp = create_core(sz)) == NULL)
			return NULL;
		_core_head = _core_tail = cp;
	}

	/* search free core */
	{
		cp = _core_tail;
		while (cp->ch_free1st_sz < sz && (cp = cp->ch_prev));
		if (cp == NULL) {
			cp = _core_tail;
			if (cp->ch_nblocks < MAXNBLOCKS && n < HUGEBLOCK) {
				if ((cp = grow_core(cp, sz)) == NULL)
					return NULL;
			} else {
				if ((cp = create_core(sz)) == NULL)
					return NULL;
				cp->ch_prev = _core_tail;
				_core_tail->ch_next = cp;
				_core_tail = cp;
			}
		}
	}

	cp->ch_nblocks++;

	/* alloc memory */
	{
		bhead *prev;

		prev = cp->ch_free1st;
		ap = (bhead *)&prev->bh_body[prev->bh_size];

		(ap->bh_next = prev->bh_next)->bh_prev = ap;
		ap->bh_prev = prev;
		ap->bh_size = n;
		prev->bh_next = ap;
	}

	/* search free2nd */
	if ((cp->ch_free1st_sz -= sz) < cp->ch_free2nd_sz) {
		cp->ch_free1st    = cp->ch_free2nd;
		cp->ch_free1st_sz = cp->ch_free2nd_sz;
		{
			bhead *p, *pp;
			bhead *free1st, *free2nd;
			size_t tmp;
			size_t free2nd_sz;

			free1st    = cp->ch_free1st;
			free2nd    = NULL;
			free2nd_sz = 0;
			for(p = cp->ch_bhead; pp = p->bh_next; p = pp) {
				if ((tmp = FREESIZE(p)) > free2nd_sz && p != free1st) {
					free2nd    = p;
					free2nd_sz = tmp;
				}
			}
			cp->ch_free2nd    = free2nd;
			cp->ch_free2nd_sz = free2nd_sz;
		}
	} else
		cp->ch_free1st = ap;

	return ap->bh_body;
}

/*
----------------------------------------
	free
----------------------------------------
*/

void free(void *body)
{
	chead *cp;
	bhead *ap;
	bhead *head;
	bhead *tail;
	bhead *prev;
	bhead *next;
	size_t sz;

	ap = body - sizeof(bhead);

	prev = ap->bh_prev;
	next = ap->bh_next;

	/* search core */
	if (last_free_core) {
		cp = last_free_core;
		head = cp->ch_bhead;
		tail = (void *)head + cp->ch_size - sizeof(bhead);
		if (head <= ap && ap <= tail)
			goto found_core;
	}
	for(head = tail = NULL, cp = _core_head; cp; cp = cp->ch_next) {
		head = cp->ch_bhead;
		tail = (void *)head + cp->ch_size - sizeof(bhead);
		if (head <= ap && ap <= tail)
			break;
	}
	if (cp == NULL)
		return;

  found_core:
	if (prev->bh_next != ap || next->bh_prev != ap)
		return;

	cp->ch_nblocks--;

	if (cp->ch_nblocks == 0 && cp->ch_next == NULL) {
		chead *cprev;

		while (cp->ch_nblocks == 0) {
			cprev = cp->ch_prev;
			free_core(cp);
			cp = cprev;
		}
		return;
	}

	last_free_core = cp;

	prev->bh_next = next;
	next->bh_prev = prev;

	sz = FREESIZE(prev);

	if (cp->ch_free1st == prev) {
		cp->ch_free1st_sz = sz;
		if (cp->ch_free2nd == ap)
			goto search2nd;
	} else if (cp->ch_free1st == ap) {
		cp->ch_free1st    = prev;
		cp->ch_free1st_sz = sz;
		if (cp->ch_free2nd == prev) {
			bhead *p;
			bhead *free1st, *free2nd;
			size_t tmp;
			size_t free2nd_sz;

		  search2nd:

			free1st    = cp->ch_free1st;
			free2nd    = NULL;
			free2nd_sz = 0;
			for(p = cp->ch_bhead; p->bh_next; p = p->bh_next) {
				if ((tmp = FREESIZE(p)) > free2nd_sz && p != free1st) {
					free2nd    = p;
					free2nd_sz = tmp;
				}
			}
			cp->ch_free2nd    = free2nd;
			cp->ch_free2nd_sz = free2nd_sz;
		}
	} else {
		if (cp->ch_free1st_sz < sz) {
			cp->ch_free2nd    = cp->ch_free1st;
			cp->ch_free2nd_sz = cp->ch_free1st_sz;
			cp->ch_free1st    = prev;
			cp->ch_free1st_sz = sz;
		} else if (cp->ch_free2nd_sz < sz) {
			cp->ch_free2nd    = prev;
			cp->ch_free2nd_sz = sz;
		}
	}
}

void *realloc(void *ptr, size_t n)
{
	bhead *ap;
	void *tmp;
	size_t size;

	ap = ptr - sizeof(bhead);

	tmp = malloc(n);
	if (tmp == NULL)
		return NULL;

	size = (n >= ap->bh_size) ? ap->bh_size : n;
	memcpy(tmp, ptr, size);
	free(ptr);

	return tmp;
}

void _clean_heap(void)
{
	chead *cp;
	bhead *bp;
	size_t sz;

	for(cp = _core_head; cp; cp = cp->ch_next) {
		for(bp = cp->ch_bhead; bp->bh_next; bp = bp->bh_next) {
			sz = FREESIZE(bp);
			memset(&bp->bh_body[bp->bh_size], 0, sz);
		}
	}

	warnlevel = 0;
	lim_data = 0;

	malloc_rating = malloc_old_rating = 0;
	malloc_sbrk_used = malloc_sbrk_unused = 0;

	virtual_heap_end = heap_end = NULL;
	data_space_start = NULL;

	_core_head = _core_tail = NULL;
}

void _debug_print_block(chead *cp)
{
	bhead *bp;
	int count;

	bp = cp->ch_bhead;

	count = 0;
	while (bp) {
		cprintf("->{%8x:%8x}", bp, bp->bh_size);
		cprintf("->{%8x:%8x}", bp->bh_body, FREESIZE(bp));
		bp = bp->bh_next;

		count++;
		if (count == 2) {
			cprintf("\r\n");
			count = 0;
		}
	}
	if (count != 0)
		cprintf("\r\n");
}

void _debug_print_core(int level)
{
	chead *cp;

	cp = _core_head;
	if (cp == NULL)
		cprintf("(NULL)\r\n");

	cprintf("head = %8x  tail = %8x\r\n", _core_head, _core_tail);

	while (cp) {
		cprintf("(%8x)\r\n", cp);
		cprintf("ch_prev       = %8x\r\n", cp->ch_prev);
		cprintf("ch_next       = %8x\r\n", cp->ch_next);
		cprintf("ch_size       = %8u\r\n", cp->ch_size);
		cprintf("ch_nblocks    = %8u\r\n", cp->ch_nblocks);
		cprintf("ch_free1st    = %8x\r\n", cp->ch_free1st);
		cprintf("ch_free1st_sz = %8u\r\n", cp->ch_free1st_sz);
		cprintf("ch_free2nd    = %8x\r\n", cp->ch_free2nd);
		cprintf("ch_free2nd_sz = %8u\r\n", cp->ch_free2nd_sz);
		cprintf("-------------------------\r\n");
		if (level < -1)
			_debug_print_block(cp);
		cp = cp->ch_next;
	}
}

void _debug_print_info(int level)
{
	chead *cp;
	bhead *bp;
	size_t free_sz;
	size_t used_sz;
	size_t sysused_sz;
	size_t tmp;
	long ncores;
	long nblocks;
	long nfreeblocks;

	ncores = nblocks = nfreeblocks = free_sz = used_sz = sysused_sz = 0;

	cprintf("\r\n");
	cprintf("-------- malloc information --------\r\n");
	cprintf("\r\n");

	for(cp = _core_head; cp; cp = cp->ch_next) {
		ncores++;
		sysused_sz += sizeof(chead);
		for(bp = cp->ch_bhead; bp; bp = bp->bh_next) {
			nblocks++;
			sysused_sz += sizeof(bhead);
			tmp = FREESIZE(bp);
			if (tmp && bp->bh_next) {
				nfreeblocks++;
				free_sz += tmp;
			}
			used_sz += bp->bh_size;
		}
	}

	_debug_print_core(level);

	cprintf("\r\n");
	cprintf("------------------------------------\r\n");
	cprintf("number of cores        : %8d\r\n", ncores);
	cprintf("number of blocks       : %8d\r\n", nblocks);
	cprintf("number of free blocks  : %8d\r\n", nfreeblocks);
	cprintf("total free size        : %8x\r\n", free_sz);
	cprintf("total used size        : %8x\r\n", used_sz);
	cprintf("total system used size : %8x\r\n", sysused_sz);
	cprintf("------------------------------------\r\n");
	cprintf("total: %8x\r\n", free_sz + used_sz + sysused_sz);
}
