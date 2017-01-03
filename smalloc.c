/*
================================================
  MicroEMACS+ Version 0.01
------------------------------------------------
  Author:
    Toshihiro Ishimaru

  Description:
    Memory Allocation Function Sets

  Note:
    icam 版 malloc を元に修正して使用.

================================================
*/

#include <stdio.h>
#include <string.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"

static void *data_space_start;
static int lim_data;
static int warnlevel;

extern void *sbrk (long);

/*
================================================
  Memory map
------------------------------------------------

                  +=============================+
+00  BLOCK_BEGIN  | BLOCK_SIZE / BLOCK_ATTR (4) | *1
                  +-----------------------------+
+04  BLOCK_BODY   |                             |
                  | ALLOCED BLOCK               |
                  |                             |
                  +-----------------------------+
-04  BLOCK_END    | BLOCK_SIZE / BLOCK_ATTR (4) | *1
                  +=============================+
+00  BLOCK_BEGIN  | BLOCK_SIZE / BLOCK_ATTR (4) | *2
                  +-----------------------------+
+04  BLOCK_BODY   |                             |
                  | FREE BLOCK                  |
                  |                             |
                  +-----------------------------+
                  | PREV FREE BLOCK (4)         | *3
                  +-----------------------------+
                  | NEXT FREE BLOCK (4)         | *3
                  +-----------------------------+
-04  BLOCK_END    | BLOCK_SIZE / BLOCK_ATTR (4) | *2
                  +=============================+

  (*1) 同じ内容
  (*2) 同じ内容
  (*3) FREE BLOCK へのポインタは BLOCK_END を指す

================================================
*/

#define BLOCK_ATTR_MASK  3
#define BLOCK_FREE  1
#define BLOCK_ALLOC 0

#define BLOCK_ALIGN_SIZE  4
#define BLOCK_INFO_SIZE   8
#define BLOCK_MIN_SIZE    16
#define GROW_HEAP_SIZE    0x1000

/*
----------------------------------------
	STACK_SIZE, HEAP_SIZE 指定
----------------------------------------
*/

__asm("		.xdef	_STACK_SIZE\n"
      "		.xdef	_HEAP_SIZE\n"
      "_STACK_SIZE	equ	24576\n"
      "_HEAP_SIZE	equ	0\n");


static inline char m_block_type (long *mp)
{
  return *mp & BLOCK_ATTR_MASK;
}

static inline long m_block_size (long *mp)
{
  return *mp & ~BLOCK_ATTR_MASK;
}

static inline long *m_next_block (long *mp_begin)
{
  return (long *)((char *)mp_begin + m_block_size (mp_begin));
}

static inline long *m_prev_block (long *mp_end)
{
  return (void *)((char *)mp_end - m_block_size (mp_end));
}

static inline long *m_next_free (long *mp_end)
{
  return ((long **)mp_end)[-2];
}

static inline long *m_prev_free (long *mp_end)
{
  return ((long **)mp_end)[-1];
}

static inline void *m_block_body (long *mp_begin)
{
  return (void *)&mp_begin[1];
}

static inline long *m_block_begin (long *mp_body)
{
  return &mp_body[-1];
}


long *free_block_head;
long *block_head, *block_tail;

#ifdef MALLOC_STRICT
long malloc_count, free_count;
#endif /* MALLOC_STRICT */

int init_xmalloc (void)
{
  long *top, *p;

  p = (long *)sbrk (256);
  if (p == (long *)-1)
    return -1;

  top = (long *)((long)p & ~(16 - 1));
  p = (long *)sbrk (256);
  if (p == (long *)-1)
    return -1;

  top[0] = BLOCK_ALLOC | 0;
  top[1] = BLOCK_ALLOC | 0;

  free_block_head = NULL;
  block_head = &top[0];
  block_tail = &top[1];

  {
    extern long sizmem (void);

    lim_data = (long)sizmem () * 4;
    data_space_start = sbrk (0);
    warnlevel = 0;
  }

  return 0;
}

int tini_xmalloc (void)
{
  return 0;
}

static void free_check (void)
{
  void *p;
  long size;

  p = sbrk(0);
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
        warnfunc (KTEX237);
      } else
        warnlevel = 0;
    case 1:
      if (size > (lim_data / 20) * 17) {
        warnlevel++;
        warnfunc (KTEX238);
      }
    case 2:
      if (size > (lim_data / 20) * 19) {
        warnlevel++;
        warnfunc (KTEX239);
      }
  }
}

static void add_to_free_block_chain (long *free_block_end)
{
  if (free_block_head) {
    ((long **)free_block_end)[-2] = free_block_head;
    ((long **)free_block_end)[-1] = NULL;
    ((long **)free_block_head)[-1] = free_block_end;
  } else {
    ((long **)free_block_end)[-2] = NULL;
    ((long **)free_block_end)[-1] = NULL;
  }

  free_block_head = free_block_end;
}

static void delete_from_free_block_chain (long *free_block_end)
{
  long *prev_free, *next_free;

  next_free = m_next_free (free_block_end);
  prev_free = m_prev_free (free_block_end);
  if (next_free)
    ((long **)next_free)[-1] = prev_free; /* next_free->prev_free = prev_free */
  if (prev_free)
    ((long **)prev_free)[-2] = next_free; /* prev_free->next_free = next_free */
  else
    free_block_head = next_free; /* if (free_block_head == free_block_end) */
}

void *grow_heap (long size)
{
  long grow_size;

  grow_size = size;
  if (grow_size < GROW_HEAP_SIZE)
    grow_size = GROW_HEAP_SIZE;

  grow_size = (grow_size + (BLOCK_ALIGN_SIZE - 1)) & ~(BLOCK_ALIGN_SIZE - 1);

  if (sbrk (grow_size) == (void *)-1)
    return NULL;

  free_check ();

  {
    long *last_block_end;
    long *new_block_tail;
    long *new_free_block_begin, *new_free_block_end;
    long new_free_block_size;

    new_block_tail = (long *)((char *)block_tail + grow_size);
    *new_block_tail = BLOCK_ALLOC | 0;

    new_free_block_size = grow_size;
    last_block_end = &block_tail[-1];
    if (m_block_type (last_block_end) == BLOCK_FREE) {
      new_free_block_begin = m_prev_block (last_block_end) + 1;
      new_free_block_size += m_block_size (last_block_end);
      delete_from_free_block_chain (last_block_end);
    } else
      new_free_block_begin = block_tail;
    new_free_block_end = &new_block_tail[-1];

    block_tail = new_block_tail;

    *new_free_block_begin = *new_free_block_end = BLOCK_FREE | new_free_block_size;
    add_to_free_block_chain (new_free_block_end);

    return new_free_block_begin;
  }
}

void *grow_heap_alloced (long size)
{
  long grow_size;

  grow_size = size;
  if (grow_size < GROW_HEAP_SIZE && _dump_flag >= 0)
    grow_size = GROW_HEAP_SIZE;

  grow_size = (grow_size + (BLOCK_ALIGN_SIZE - 1)) & ~(BLOCK_ALIGN_SIZE - 1);

  if (sbrk (grow_size) == (void *)-1)
    return NULL;

  free_check ();

  {
    long *new_block_tail, *new_block_begin;
    long new_free_block_size;

    new_block_begin = block_tail;
    new_block_tail = (long *)((char *)block_tail + grow_size);
    *new_block_tail = BLOCK_ALLOC | 0;
    block_tail = new_block_tail;

    new_free_block_size = grow_size - size;
    if (new_free_block_size < BLOCK_MIN_SIZE) {
      long *new_block_end;

      new_block_end = &block_tail[-1];
      *new_block_begin = *new_block_end = BLOCK_ALLOC | grow_size;
    } else {
      long *new_block_end;
      long *new_free_block_begin, *new_free_block_end;

      new_free_block_begin = (long *)((char *)new_block_begin + size);
      new_free_block_end = &block_tail[-1];
      new_block_end = &new_free_block_begin[-1];

      *new_block_begin = *new_block_end = BLOCK_ALLOC | size;

      *new_free_block_begin = *new_free_block_end = BLOCK_FREE | new_free_block_size;
      add_to_free_block_chain (new_free_block_end);
    }

    return m_block_body (new_block_begin);
  }
}

void *malloc (long size)
{
  long block_size;

  {
    static int inited;

    if (!inited) {
      init_xmalloc ();
      inited = 1;
    }
  }

#ifdef MALLOC_STRICT
  {
    malloc_count++;
    if (malloc_count > 0xab6) {
      if (xcheck_block_chain () < 0)
        cprintf ("(before) malloc_count = %d\r\n", malloc_count);
    }
  }
#endif /* MALLOC_STRICT */

  block_size = BLOCK_INFO_SIZE + (size + (BLOCK_ALIGN_SIZE - 1)) & ~(BLOCK_ALIGN_SIZE - 1);
  if (block_size < BLOCK_MIN_SIZE)
    block_size = BLOCK_MIN_SIZE;

  {
    long *free_block_begin, *free_block_end;
    long free_block_size, new_free_block_size;

    {
      long *p;

      for (p = free_block_head; p; p = m_next_free (p)) {
        if (m_block_size (p) >= block_size)
          break;
      }

      if (p == NULL)
        return grow_heap_alloced (block_size);

      free_block_begin = (long *)((char *)m_prev_block (p) + 4);
      free_block_end = p;
    }

    free_block_size = m_block_size (free_block_begin);
    new_free_block_size = free_block_size - block_size;
    if (new_free_block_size < BLOCK_MIN_SIZE) {
      delete_from_free_block_chain (free_block_end);

      *free_block_begin = *free_block_end = BLOCK_ALLOC | free_block_size;
    } else {
      long *new_free_block_begin, *new_free_block_end;

      new_free_block_begin = (long *)((char *)free_block_begin + block_size);
      new_free_block_end = free_block_end;
      free_block_end = &new_free_block_begin[-1];

      *free_block_begin = *free_block_end = BLOCK_ALLOC | block_size;

      *new_free_block_begin = *new_free_block_end = BLOCK_FREE | new_free_block_size;
    }

#ifdef MALLOC_STRICT
    {
      if (malloc_count > 0xab6) {
        if (xcheck_block_chain () < 0)
          cprintf ("(after) malloc_count = %d\r\n", malloc_count);
      }
    }
#endif /* MALLOC_STRICT */

    return m_block_body (free_block_begin);
  }
}

void *realloc (void *mp, long new_size)
{
  void *new_mp;
  long *block_begin;
  long size;

  if (mp == NULL)
    return malloc (new_size);

  block_begin = m_block_begin (mp);
  size = m_block_size (block_begin) - BLOCK_INFO_SIZE;
  if (size >= new_size)
    return mp;

  new_mp = malloc (new_size);
  if (new_mp)
    memcpy (new_mp, mp, size);

  return new_mp;
}

void free (void *mp)
{
  if (mp == NULL)
    return;

#ifdef MALLOC_STRICT
    {
      free_count++;

      if (free_count > 0x7c2) {
        if (xcheck_block_chain () < 0)
          cprintf ("(before) free_count = %d\r\n", free_count);
      }
    }
#endif /* MALLOC_STRICT */

  {
    long *prev_block_end, *next_block_begin;
    long *block_begin, *block_end;
    long block_size;

    block_begin = (long *)mp - 1;
    block_end = m_next_block (block_begin) - 1;
    block_size = m_block_size (block_begin);

    prev_block_end = block_begin - 1;
    next_block_begin = block_end + 1;

    if (m_block_type (prev_block_end) == BLOCK_FREE) {
      delete_from_free_block_chain (prev_block_end);
      block_size += m_block_size (prev_block_end);
      block_begin = m_prev_block (prev_block_end) + 1;
    }
    if (m_block_type (next_block_begin) == BLOCK_FREE) {
      block_size += m_block_size (next_block_begin);
      block_end = m_next_block (next_block_begin) - 1;
    } else
      add_to_free_block_chain (block_end);

    *block_begin = *block_end = BLOCK_FREE | block_size;
  }

#ifdef MALLOC_STRICT
  {
    if (free_count > 0x7c2) {
      if (xcheck_block_chain () < 0)
        cprintf ("(after) free_count = %d\r\n", free_count);
    }
  }
#endif /* MALLOC_STRICT */
}

#ifdef MALLOC_STRICT
void xprint_memmap (void)
{
  long *p;

  p = block_head;

  cprintf ("free_block_head = 0x%08x\r\n", free_block_head);

  cprintf ("             +==============+\r\n");
  cprintf ("(0x%08x) |  BLOCK HEAD  |\r\n", p);
  cprintf ("             +==============+\r\n");

  p++;
  while (*p) {
    long *block_begin, *block_end;
    long block_size;

    block_size = m_block_size (p);
    block_begin = p;
    block_end = m_next_block (p) - 1;

    if (m_block_type (p) == BLOCK_ALLOC) {
      cprintf ("(0x%08x) | (0x%08x) |\r\n", block_begin, block_size);
      cprintf ("             |              |\r\n");
      cprintf ("(0x%08x) | (0x%08x) |\r\n", block_end, block_size);
      cprintf ("             +==============+\r\n");
    } else {
      cprintf ("(0x%08x) | [0x%08x] |\r\n", block_begin, block_size);
      cprintf ("             |--------------|\r\n");
      cprintf ("             | <- %08x  |\r\n", block_end[-1]);
      cprintf ("             | %08x ->  |\r\n", block_end[-2]);
      cprintf ("             |--------------|\r\n");
      cprintf ("(0x%08x) | [0x%08x] |\r\n", block_end, block_size);
      cprintf ("             +==============+\r\n");
    }

    p = block_end + 1;
  }

  cprintf ("(0x%08x) |  BLOCK TAIL  |\r\n", p);
  cprintf ("             +==============+\r\n");
  cprintf ("\r\n");
}

int xcheck_block_chain (void)
{
  int rval = 0;
  long *p;

  p = free_block_head;
  {
    while (p) {
      long *next;

      if (p < block_head || p > block_tail) {
        cprintf ("free_chain probrem (0x%08x)\r\n", p);
        rval = -1;
      }
      next = m_next_free (p);
      if (next && m_prev_free (next) != p) {
        cprintf ("free_chain probrem (0x%08x)\r\n", p);
        rval = -1;
      }
      p = next;
    }
  }

  p = block_head + 1;
  while (*p) {
    long *block_begin, *block_end;
    long block_size;

    block_size = m_block_size (p);
    block_begin = p;
    block_end = m_next_block (p) - 1;

    if (block_size > 0x10000) {
      cprintf ("block_size probrem (0x%8x)\r\n", block_begin);
      rval = -1;
    }

    p = block_end + 1;
  }
  if (p != block_tail) {
    cprintf ("block_tail probrem (0x%8x)\r\n", p);
    rval = -1;
  }

  return rval;
}
#endif /* MALLOC_STRICT */

int moreheap (long size)
{
  long *last_block_end;
  long sz = size;

  last_block_end = &block_tail[-1];
  if (m_block_type (last_block_end) == BLOCK_FREE)
    sz -= m_block_size (last_block_end);

  free_check ();

  if (sz < 0)
    return 0;

  return (grow_heap (sz) == NULL) ? -1 : 0;
}

void _debug_print_info (void)
{
}

void _clean_heap (void)
{
}
