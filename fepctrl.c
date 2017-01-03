/*
 * fepctrl.c 1.2 1990/11/25	Public Domain.
 *
 * General purpose Japanese FEP control routines for Human68k.
 * Written for MS-DOS by Junn Ohta <ohta@src.ricoh.co.jp>
 * Adjusted to Human68k by Sawayanagi Yosirou <willow@saru.cc.u-tokyo.ac.jp>
 *
 *	int fep_init()
 *		checks FEP and turn it off, returns FEP type.
 *	void fep_term()
 *		restore the status of FEP saved by fep_init().
 *	void fep_on()
 *		restore the status of FEP saved by fep_off().
 *	void fep_off()
 *		save the status of FEP and turn it off.
 *	void fep_force_on()
 *		turn FEP on by its default "on" status.
 *	void fep_force_off()
 *		don't save the status of FEP and turn it off.
 */
/*
 *	1992.02.11 little modified for jelvis68k by Y.Ogawa(GAPO)
 */
/*
 *  1992.02.29 little modified for MicroEMACS by SALT
 */

#include <stdio.h>

#include "fepctrl.h"

#ifndef __GNUC__
# include "*** ERROR: Can't compile this file without GNU CC. good bye! ***"
#endif

#define HENTRAP_HOOK_START (-1)
#define HENTRAP_HOOK_STOP  (0)

/*
 * default "on" status of FEP (used only in fep_force_on())
 */
static int fepon[NFEPS] = {
/* FEP_NONE	*/  0,
/* FEP_ASK68K	*/  1	/* 0=off, 1=on */
};

extern int fepctrl;

static int fep = FEP_NONE;
static int oldmode = 0;
static int keepmode = 0;

static int fep_find(void);
static void fep_open(void (*)(int));
static void fep_close(void);
static int fep_mode(int);
extern int hentrap(int, void (*)(int));	/* defined in hentrap.s */ 
extern int iskmode(void);				/* defined in hentrap.s */

int
fep_init(void (*fep_handle_func)(int))
{
	if (fep == FEP_NONE)
		fep = fep_find();
	fep_open(fep_handle_func);
	oldmode = keepmode = fep_mode(0);
	return fep;
}

void
fep_term(void)
{
	fep_mode(oldmode);
	fep_close();
}

void
fep_on(void)
{
	fep_mode(keepmode);
}

void
fep_off(void)
{
	keepmode = fep_mode(0);
}

void
fep_force_on(void)
{
	fep_mode(fepon[fep]);
}

void
fep_force_off(void)
{
	fep_mode(0);
}

/*--------------------------------------------------------------------*/
#ifdef TEST

static char *fepname[NFEPS] = {
/* FEP_NONE	*/  "(none)",
/* FEP_ASK68K	*/  "ASK68K"
};

static void
putstr(char *s)
{
	while (*s)
		putch(*s++);
}

static void
echoline(char *s)
{
	int c;
	putstr(s);
	while ((c = getch()) != '\r' && c != '\n')
		putch(c);
	putstr("\r\n");
}

main(void)
{
	putstr("fep = ");
	putstr(fepname[fep_init()]);
	putstr("\r\n");
	putstr("enter 4 lines of text\r\n");
	fep_force_on();
	echoline(" on: ");
	fep_off();
	echoline("off: ");
	fep_on();
	echoline(" on: ");
	fep_force_off();
	echoline("off: ");
	fep_term();
	exit(0);
}

#endif /* TEST */
/*--------------------------------------------------------------------*/

static int
fep_find (void)
{
	register int d0 asm ("d0");
	asm volatile ("move.l #2,-(sp)\n\t" /* ret KNJCTRL(2) */
				  "dc.w $ff22\n\t"
				  "addq.l #4,sp"
		: "=r" (d0)
		);
	return d0 == -1 ? FEP_NONE : FEP_ASK68K;
}

static void
fep_open(void (*fep_handle_func)(int))
{
	if (fep == FEP_ASK68K)
		hentrap(HENTRAP_HOOK_START, fep_handle_func);
}

static void
fep_close(void)
{
	if (fep == FEP_ASK68K)
		hentrap(HENTRAP_HOOK_STOP, NULL);
}

static int ask68k_get_mode(void);
static int ask68k_set_mode(int);

static int
fep_mode(int newmode)
{
	int curmode;

	if (!fepctrl)
		return 0;

	switch (fep) {
	case FEP_ASK68K:
		/* mode: 0 = off, 1 = on */
		curmode = ask68k_get_mode();
		if (newmode != curmode)
			ask68k_set_mode(newmode);
		return curmode;
	default:
		return 0;
	}
}

static int
ask68k_get_mode(void)
{
	return iskmode() ? 1 : 0;
}


static int
ask68k_set_mode (int flag)
{
  register int d0 asm ("d0");

  if (flag) {
	asm volatile ("move.l #2,-(sp)\n\t"	/* d0 = KNJCTRL(2) */
	              "dc.w $ff22\n\t"
	              "addq.l #4,sp\n\t"
	              "tst.l d0\n\t"
	              "bne FEPCTRL_SKIP\n\t"
	              "moveq.l #2,d0\n"
	              "FEPCTRL_SKIP:\n\t"
	              "move.l d0,-(sp)\n\t" /* KNJCTRL(1,d0) */
	              "move.l #1,-(sp)\n\t"
	              "dc.w $ff22\n\t"
	              "addq.l #8,sp\n\t"
		  : "=r" (d0)
		  );
  }
  else {
	asm volatile ( "clr.l -(sp)\n\t" /* KNJCTRL(1,0) */
	               "move.l #1,-(sp)\n\t"
	               "dc.w $ff22\n\t"
	               "addq.l #8,sp\n\t"
		   : "=r" (d0)
		   );
  }
  return d0;
}
