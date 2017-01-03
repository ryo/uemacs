/*
----------------------------------------
	LANGC.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id ‚ÌÝ’è
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/langc.c,v 1.2 1991/08/25 11:12:14 SALT Exp $'\n""	even\n");

/*
========================================
	Žg—pŠÖ”‚Ì’è‹`
========================================
*/

static int countbyte(int);

/*
========================================
	ƒoƒCƒg”ŒvŽZ
========================================
*/

static int countbyte(int n)
{
	int co, cnt;
	LINE *cp;
	WINDOW *wp = curwp;

	cp = wp->w_dotp;
	co = wp->w_doto;

	for (cnt = 0; n; n--, cnt++) {
		if (iskanji(lgetc2(wp->w_dotp, wp->w_doto)))
			cnt++;
		forwchar(FALSE, 1);
	}

	wp->w_dotp = cp;
	wp->w_doto = co;

	return cnt;
}

/*
----------------------------------------
	‚b ‚Ì–¼‘O•âŠ®
----------------------------------------
*/

int c_complete(int f, int n)
{
	return name_complete(CMP_C);
}

/*
----------------------------------------
	?? ‘}“ü
----------------------------------------
*/

int insmatch(int f, int n, int c)
{
	if (n <= 0) {
		lastflag = 0;
		return (n < 0) ? FALSE : TRUE;
	} else {
		int status;

		thisflag = 0;
		if (check_over())
			forwdel(FALSE, 1);
		status = linsert(n, c);
		fmatch(c);
		return status;
	}
}

/*
----------------------------------------
	')' ‘}“ü
----------------------------------------
*/

int c_insparen(int f, int n)
{
	return insmatch(f, n, ')');
}

/*
----------------------------------------
	'}' ‘}“ü
----------------------------------------
*/

int c_insbrace(int f, int n)
{
	int status;

	status = insbrace(n, '}');
	fmatch('}');
	return status;
}

/*
----------------------------------------
	']' ‘}“ü
----------------------------------------
*/

int c_insbracket(int f, int n)
{
	return insmatch(f, n, ']');
}

/*
----------------------------------------
	'#' ‘}“ü
----------------------------------------
*/

int c_inspound(int f, int n)
{
	if (n <= 0) {
		lastflag = 0;
		return (n < 0 ? FALSE : TRUE);
	} else {
		thisflag = 0;
		if (check_over())
			forwdel(FALSE, 1);
		return inspound();
	}
}

/*
----------------------------------------
	TAB ‘}“ü
----------------------------------------
*/

int c_instab(int f, int n)
{
	int stabsize, htabsize;
	int ccol, nspc = 0;

	stabsize = curbp->b_stabs;
	htabsize = curbp->b_tabs;
	ccol = getccol(FALSE);

	if (stabsize)
		nspc = stabsize - ccol % stabsize;

	if (stabsize && htabsize > stabsize) {
		if (!stabmode && (ccol + nspc) % htabsize == 0) {
			LINE *dotp;
			int doto;
			int i, base;

			base = ccol - ccol % htabsize;
			dotp = curwp->w_dotp;
			doto = curwp->w_doto;
			for (i = base; i < ccol; i++) {
				doto--;
				if (lgetc(dotp, doto) != ' ')
					break;
				backdel(FALSE, 1);
			}
			return linsert(1, TAB);
		} else
			return linsert(nspc, ' ');
	}

	return stabsize ? linsert(nspc, ' ') : linsert(1, TAB);
}

/*
----------------------------------------
	c-delete-next-character
----------------------------------------
*/

int c_forwdel(int f, int n)
{
	if (n < 0)
		return c_backdel(f, -n);

	{
		int stabsize, htabsize;

		stabsize = curbp->b_stabs;
		htabsize = curbp->b_tabs;

		if (stabsize) {
			int status;

			status = TRUE;
			while (n--) {
				int doto;

				doto = curwp->w_doto;
				if (lgetc(curwp->w_dotp, doto) == TAB) {
					ldelete(1, FALSE);
					linsert(htabsize, ' ');
					curwp->w_doto = doto;
				}
				status = ldelete(countbyte(1), FALSE);
				if (status != TRUE)
					break;
			}
			return status;
		} else
			return ldelete(countbyte(n), FALSE);
	}
}

/*
----------------------------------------
	c-delete-previous-character
----------------------------------------
*/

int c_backdel(int f, int n)
{
	if (n < 0)
		return c_forwdel(f, -n);

	if (f == TRUE) {
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}

	{
		int status;
		int stabsize, htabsize;

		stabsize = curbp->b_stabs;
		htabsize = curbp->b_tabs;

		status = TRUE;
		if (stabsize) {
			while (n--) {
				status = backchar(FALSE, 1);
				if (status != TRUE)
					break;
				if (lgetc(curwp->w_dotp, curwp->w_doto) == TAB) {
					ldelete(1, FALSE);
					linsert(htabsize, ' ');
					backchar(FALSE, 1);
				}
				status = ldelete(countbyte(1), FALSE);
				if (status != TRUE)
					break;
			}
			return status;
		} else {
			status = backchar(f, n);
			if (status == TRUE)
				status = ldelete(countbyte(n), FALSE);
			return status;
		}
	}
}

/*
----------------------------------------
	V‚½‚Ès
----------------------------------------
*/

int c_newline(int f, int n)
{
	if (n == 1 && curwp->w_dotp != curbp->b_linep)
		return cinsert();
	if (check_wrap())
		exechook(wraphook);
	while (n--) {
		int status;

		status = lnewline();
		if (status != TRUE)
			return status;
	}
	return TRUE;
}
