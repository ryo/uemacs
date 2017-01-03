/* malloc for SX-WINDOW (is faster than NewPtr/DisposePtr)
 * version 0.01 (test)	Sep  3 1991	by Masaru Oki
 * version 0.02 (test)  Oct 15 1991	by Masaru Oki
 *		malloc() ����A�h���X��Ԃ��o�O��ގ��B
 *		realloc() ��p�ӁB
 *		_sx_killblocks() ��p�ӁB
 *		newblock() �� links �� 0 �ɂ��Ă����̂� 1 �ɏC���B
 *		newblock() �� siz>MEMSIZE �̂Ƃ� free �ɂ������Ȓl��
 *		�ݒ肵�Ă����̂ŏC���B
 *		newblock() �� _sx_termblock �̍X�V�����Ă��Ȃ��o�O��
 *		�ގ��B
 * version 0.03 (test)	Oct 16 1991	by Masaru Oki
 *		freeblock() �� DisposePtr �̈ʒu���ς������̂𒼂����B
 *		POOL * �̃|�C���^�Ƃ̉��Z�ɂ��o�O��ގ��B
 *		�e�X�g���[�`����t�������B
 */

#define NULL	0
#define MEMSIZE	65536

typedef struct POOL {
  struct POOL		*next;
  unsigned short	links;
  unsigned short	free;
  unsigned          size;
  char				mem[0];
} POOL;

#define POOLHEADER	sizeof(POOL)

static POOL *_sx_topblock = NULL;
static POOL *_sx_termblock = NULL;

extern void *NewPtr (unsigned long logicalSize);
extern void DisposePtr (void *pt);

static void *newblock (unsigned long blksiz, unsigned long siz);
static void freeblock (POOL *p);

void *malloc (unsigned long siz) {
  POOL *p = _sx_topblock;
  if (siz & 1) siz++;
  if (siz >= MEMSIZE)	return newblock (siz, siz);
  while (p) {
    if (p->free > siz) {
      void *pt = &p->mem[p->size - p->free];
      p->free -= siz;
      p->links++;
      return pt;
    }
    p = p->next;
  }
  return newblock (MEMSIZE, siz);
}

int free (void *pt) {
  POOL *p = _sx_topblock;
  while (p) {
    if (&p->mem[0] < pt && pt < &p->mem[p->size]) {
      if (--(p->links) == 0) {
	freeblock(p);
      }
      return 0;
    }
    p = p->next;
  }
  return -1;	/* illegal pointer requested */
}

void *realloc (void *pt, unsigned long siz) {
  void *newpt = malloc(siz);
  if (!newpt) return 0;
  if (pt) {
    memcpy(newpt, pt, siz);
    free(pt);
  }
  return newpt;
}

void _sx_killblocks (void) {
  POOL *bp = _sx_topblock;
  while (bp) {
    POOL *p = bp->next;
    MFREE(bp);
    bp = p;
  }
}

static void *newblock (unsigned long blksiz, unsigned long siz) {
  POOL *pt = (POOL *)MALLOC(POOLHEADER + blksiz);

  if (!pt) return 0;
  pt->next = NULL;
  pt->links = 1;
  pt->free = MEMSIZE > siz ? MEMSIZE - siz : 0 ;
  pt->size = blksiz;

  if (!_sx_topblock) {
    _sx_topblock = _sx_termblock = pt;
  } else {
    _sx_termblock = (_sx_termblock->next = pt);
  }
  return (POOL *)((void *)pt + POOLHEADER);
}

static void freeblock (POOL *p) {
  if (p == _sx_topblock) {
    if (p->next) {
      _sx_topblock = p->next;
    } else {
      _sx_topblock = _sx_termblock = NULL;
    }
  } else {
    POOL *bp = _sx_topblock;
    while (bp->next != p) {
      bp = bp->next;
    }
    if (p == _sx_termblock) {
      _sx_termblock = bp;
      bp->next = NULL;
    } else {
      bp->next = p->next;
    }
  }
  MFREE(p);
}

_debug_print_info() {}
_clean_heap() {}

moreheap() {}
