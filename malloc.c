/*
----------------------------------------
	MALLOC.C: MicroEMACS 3.10
----------------------------------------
*/

/* dynamic memory allocation for GNU.
   Copyright (C) 1985, 1987 Free Software Foundation, Inc.

                NO WARRANTY

  BECAUSE THIS PROGRAM IS LICENSED FREE OF CHARGE, WE PROVIDE ABSOLUTELY
NO WARRANTY, TO THE EXTENT PERMITTED BY APPLICABLE STATE LAW.  EXCEPT
WHEN OTHERWISE STATED IN WRITING, FREE SOFTWARE FOUNDATION, INC,
RICHARD M. STALLMAN AND/OR OTHER PARTIES PROVIDE THIS PROGRAM "AS IS"
WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY
AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE
DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR
CORRECTION.

 IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW WILL RICHARD M.
STALLMAN, THE FREE SOFTWARE FOUNDATION, INC., AND/OR ANY OTHER PARTY
WHO MAY MODIFY AND REDISTRIBUTE THIS PROGRAM AS PERMITTED BELOW, BE
LIABLE TO YOU FOR DAMAGES, INCLUDING ANY LOST PROFITS, LOST MONIES, OR
OTHER SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR
DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY THIRD PARTIES OR
A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) THIS
PROGRAM, EVEN IF YOU HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES, OR FOR ANY CLAIM BY ANY OTHER PARTY.

        GENERAL PUBLIC LICENSE TO COPY

  1. You may copy and distribute verbatim copies of this source file
as you receive it, in any medium, provided that you conspicuously and
appropriately publish on each copy a valid copyright notice "Copyright
(C) 1985 Free Software Foundation, Inc."; and include following the
copyright notice a verbatim copy of the above disclaimer of warranty
and of this License.  You may charge a distribution fee for the
physical act of transferring a copy.

  2. You may modify your copy or copies of this source file or
any portion of it, and copy and distribute such modifications under
the terms of Paragraph 1 above, provided that you also do the following:

    a) cause the modified files to carry prominent notices stating
    that you changed the files and the date of any change; and

    b) cause the whole of any work that you distribute or publish,
    that in whole or in part contains or is a derivative of this
    program or any part thereof, to be licensed at no charge to all
    third parties on terms identical to those contained in this
    License Agreement (except that you may choose to grant more extensive
    warranty protection to some or all third parties, at your option).

    c) You may charge a distribution fee for the physical act of
    transferring a copy, and you may at your option offer warranty
    protection in exchange for a fee.

Mere aggregation of another unrelated program with this program (or its
derivative) on a volume of a storage or distribution medium does not bring
the other program under the scope of these terms.

  3. You may copy and distribute this program (or a portion or derivative
of it, under Paragraph 2) in object code or executable form under the terms
of Paragraphs 1 and 2 above provided that you also do one of the following:

    a) accompany it with the complete corresponding machine-readable
    source code, which must be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    b) accompany it with a written offer, valid for at least three
    years, to give any third party free (except for a nominal
    shipping charge) a complete machine-readable copy of the
    corresponding source code, to be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    c) accompany it with the information you received as to where the
    corresponding source code may be obtained.  (This alternative is
    allowed only for noncommercial distribution and only if you
    received the program in object code or executable form alone.)

For an executable file, complete source code means all the source code for
all modules it contains; but, as a special exception, it need not include
source code for modules which are standard libraries that accompany the
operating system on which the executable file runs.

  4. You may not copy, sublicense, distribute or transfer this program
except as expressly provided under this License Agreement.  Any attempt
otherwise to copy, sublicense, distribute or transfer this program is void and
your rights to use the program under this License agreement shall be
automatically terminated.  However, parties who have received computer
software programs from you with this License Agreement will not have
their licenses terminated so long as such parties remain in full compliance.

  5. If you wish to incorporate parts of this program into other free
programs whose distribution conditions are different, write to the Free
Software Foundation at 675 Mass Ave, Cambridge, MA 02139.  We have not yet
worked out a simple rule that can be stated here, but we will often permit
this.  We will be guided by the two goals of preserving the free status of
all derivatives of our free software and of promoting the sharing and reuse of
software.


In other words, you are welcome to use, share and improve this program.
You are forbidden to forbid anyone else to use, share and improve
what you give them.   Help stamp out software-hoarding!  */


/*
 * @(#)nmalloc.c 1 (Caltech) 2/21/82
 *
 *  U of M Modified: 20 Jun 1983 ACT: strange hacks for Emacs
 *
 *  Nov 1983, Mike@BRL, Added support for 4.1C/4.2 BSD.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small
 * number of different sizes, and keeps free lists of each size.  Blocks
 * that don't exactly fit are passed up to the next larger size.  In this
 * implementation, the available sizes are (2^n)-4 (or -16) bytes long.
 * This is designed for use in a program that uses vast quantities of
 * memory, but bombs when it runs out.  To make it a little better, it
 * warns the user when he starts to get near the end.
 *
 * June 84, ACT: modified rcheck code to check the range given to malloc,
 * rather than the range determined by the 2-power used.
 *
 * Jan 85, RMS: calls malloc_warning to issue warning on nearly full.
 * No longer Emacs-specific; can serve as all-purpose malloc for GNU.
 * You should call malloc_init to reinitialize after loading dumped Emacs.
 * Call malloc_stats to get info on memory stats if MSTATS turned on.
 * realloc knows how to return same block given, just changing its size,
 * if the power of 2 is correct.
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/malloc.c,v 1.6 1991/10/21 13:08:38 SALT Exp SALT $'\n""	even\n");

/*
========================================
	Headder 定義
========================================
*/

struct mhead {
	char mh_alloc;
	char mh_index;
	unsigned short mh_size;
};

/*
========================================
	使用変数の定義
========================================
*/

static unsigned lim_data;
static char *heap_end = 0;
static char *virtual_heap_end = 0;
static char *data_space_start;
static int gotpool = 0;
static int warnlevel;
static struct mhead *nextf[30];
static char busy[30];

extern char *_HSTA;

/*
========================================
	使用関数の定義
========================================
*/

char *sbrk(int);
long sizmem(void);

static char *_sbrk(int size);
static void getpool(void);
static void morecore(int);

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
----------------------------------------
	morecore
----------------------------------------
*/

static void morecore(int nu)
{
	int nblks;
	unsigned siz;
	char *cp;

	if (!gotpool) {
		getpool();
		getpool();
		gotpool = 1;
	}
	cp = _sbrk(0);
	siz = cp - data_space_start;
	malloc_sbrk_used = siz;
	malloc_sbrk_unused = lim_data - siz;
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
		if (siz > (lim_data / 4) * 3) {
			warnlevel++;
			warnfunc(KTEX237);
		}
		break;
	case 1:
		if (siz > (lim_data / 20) * 17) {
			warnlevel++;
			warnfunc(KTEX238);
		}
		break;
	case 2:
		if (siz > (lim_data / 20) * 19) {
			warnlevel++;
			warnfunc(KTEX239);
		}
		break;
	}

	if ((int) cp & 0xf)
		_sbrk(16 - ((int) cp & 0xf));
	nblks = 1;
	siz = nu;
	if (siz < 8) {
		siz = 8;
		nblks = 1 << (8 - nu);
	}
	cp = _sbrk(1 << (siz + 3));
	if (cp == (char *) -1)
		return;
	if ((unsigned) cp & 7) {
		cp = (char *) (((unsigned int) cp + 8) & ~7);
		nblks--;
	}
	nextf[nu] = (struct mhead *) cp;
	siz = 1 << (nu + 3);
	while (1) {
		((struct mhead *) cp)->mh_alloc = ISFREE;
		((struct mhead *) cp)->mh_index = nu;
		if (--nblks <= 0)
			break;
		CHAIN((struct mhead *) cp) = (struct mhead *) (cp + siz);
		cp += siz;
	}
	CHAIN((struct mhead *) cp) = 0;
}

/*
----------------------------------------
	getpool
----------------------------------------
*/

static void getpool(void)
{
	char *cp = _sbrk(0);

	if ((int) cp & 0xf)
		_sbrk(16 - ((int) cp & 0xf));
	cp = _sbrk(04000);
	if (cp == (char *) -1)
		return;
	CHAIN(cp) = nextf[0];
	nextf[0] = (struct mhead *) cp;
	((struct mhead *) cp)->mh_alloc = ISFREE;
	((struct mhead *) cp)->mh_index = 0;
	cp += 8;

	{
		int nu;

		for (nu = 0; nu < 7; nu++) {
			CHAIN(cp) = nextf[nu];
			nextf[nu] = (struct mhead *) cp;
			((struct mhead *) cp)->mh_alloc = ISFREE;
			((struct mhead *) cp)->mh_index = nu;
			cp += (8 << nu);
		}
	}
}

/*
----------------------------------------
	malloc
----------------------------------------
*/

char *malloc(unsigned n)
{
	int nunits = 0;
	unsigned nbytes;
	struct mhead *p;

	if (lim_data == 0) {
		lim_data = sizmem() << 2;
		data_space_start = _HSTA;
		virtual_heap_end = heap_end = sbrk(0);
		warnlevel = 0;
	}

	nbytes = (n + ((sizeof(*p) + 7) & ~7) + 7) & ~7;
	if (nbytes > 0xffff)
		meabort(KTEX241);

	{
		int shiftr = (nbytes - 1) >> 2;

		while (shiftr >>= 1)
			nunits++;
	}

	while (busy[nunits])
		nunits++;
	busy[nunits] = 1;
	if (nextf[nunits] == 0)
		morecore(nunits);
	p = nextf[nunits];
	if (p == 0) {
		busy[nunits] = 0;
		kill_int();
		return 0;
	}
	nextf[nunits] = CHAIN(p);
	busy[nunits] = 0;
	if (p->mh_alloc != ISFREE || p->mh_index != nunits)
		meabort(KTEX242);
	p->mh_alloc = ISALLOC;
	p->mh_size = n;

	return (char *) p + ((sizeof(*p) + 7) & ~7);
}

/*
----------------------------------------
	free
----------------------------------------
*/

void free(char *mem)
{
	struct mhead *p;

	{
		if (mem == 0)
			return;
		p = (struct mhead *) (mem - ((sizeof(*p) + 7) & ~7));

		if (p->mh_alloc != ISALLOC)
			meabort(KTEX243);
	}

	{
		int nunits = p->mh_index;

		p->mh_alloc = ISFREE;
		busy[nunits] = 1;
		CHAIN(p) = nextf[nunits];
		nextf[nunits] = p;
		busy[nunits] = 0;
	}
}

/*
----------------------------------------
	realloc
----------------------------------------
*/

char *realloc(char *mem, unsigned n)
{
	int nunits;
	unsigned tocopy, nbytes;
	struct mhead *p;

	if (mem == 0)
		return malloc(n);

	p = (struct mhead *) (mem - ((sizeof *p + 7) & ~7));
	nunits = p->mh_index;
	tocopy = (p->mh_index >= 13)
	    ? (1 << (p->mh_index + 3)) - ((sizeof *p + 7) & ~7) : p->mh_size;
	nbytes = (n + ((sizeof *p + 7) & ~7) + 7) & ~7;
	if (nbytes > (4 << nunits) && nbytes <= (8 << nunits)) {
		p->mh_size = n;
		return mem;
	}
	if (n < tocopy)
		tocopy = n;

	{
		char *new;

		new = (char *) malloc(n);
		if (new == 0)
			return 0;
		memcpy(new, mem, tocopy);
		free(mem);
		return new;
	}
}

/*
----------------------------------------
	_sbrk
----------------------------------------
*/

static char *_sbrk(int size)
{
	char *prev_ptr, *new_virtual_heap_end;

	prev_ptr = virtual_heap_end;
	new_virtual_heap_end = virtual_heap_end + size;
	if (heap_end < new_virtual_heap_end) {
		char *cp;

		cp = sbrk((int) new_virtual_heap_end - (int) heap_end + 512);
		if (cp == (char *) -1)
			return cp;
		heap_end = sbrk(0);
	}
	virtual_heap_end = new_virtual_heap_end;
	return prev_ptr;
}

/*
----------------------------------------
	moreheap
----------------------------------------
*/

int moreheap(size_t size)
{
	char *new_heap_end;

	new_heap_end = virtual_heap_end + size;
	if (heap_end < new_heap_end) {
		char *cp;

		cp = sbrk((int) new_heap_end - (int) heap_end);
		if (cp == (char *) -1)
			return 0;

		heap_end = sbrk(0);
	}
	return 1;
}

void _clean_heap(void)
{
}

void _debug_print_core(void)
{
}

void _debug_print_block(void)
{
}

void _debug_print_info(void)
{
}
