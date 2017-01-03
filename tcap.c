/*
 * $Id: tcap.c,v 1.30 2017/01/02 15:17:50 ryo Exp $
 *
 * tcap: Unix V5, SUN OS, SCO XENIX, V7 and BS4.2 Termcap video driver
 * for MicroEMACS 3.10
 *
 * 12-10-88 - Modifications made by Guy Turcotte to accomodate SunOS V4.0 and
 * Xenix V2.2.1 :
 *
 * SunOS mods:
 *
 * o p_seq field of TBIND struct augmented to 10 chars to take into account
 * longer definitions for keys (some Sun's keys definitions need at least 7
 * chars...) as such, the code in get1key has been modified to take care of
 * the longer p_seq string.
 *
 * o tcapopen modified to take care of the tgetstr problem (returns NULL on
 * undefined keys instead of a valid string pointer...)
 *
 * o The timout algorithm of get1key has been modified to take care of the
 * following select() function problem: if some chars are already in the
 * terminal buffer before select is called and no others char appears on the
 * terminal, it will timeout anyway... (maybe a feature of SunOs V4.0)
 *
 * Xenix mods:
 *
 * o The first two points indicated above are applicable for the Xenix OS
 *
 * o With my current knowledge, I can't find a clean solution to the timeout
 * problem of the get1key function under Xenix. I modified the code to get
 * rid of the BSD code (via the #if directive) and use the Xenix nap() and
 * rdchk() functions to make a 1/30 second wait. Seems to work as long as
 * there is not to much of activity from other processes on the system.
 * (The link command of the makefile must be modified to link with the x
 * library... you must add the option -lx)
 *
 * o The input.c file has been modified to not include the get1key function
 * defined there in the case of USG. The #if directive preceeding the get1key
 * definition has been modified from:
 *
 * #if (V7 == 0) && (BSD == 0)
 *
 * to:
 *
 * #if (V7 == 0) && (BSD == 0) && (USG == 0)
 *
 * o The following lines define the new termcap entry for the ansi kind of
 * terminal: it permits the use of functions keys F1 .. F10 and keys
 * HOME,END,PgUp,PgDn on the IBM PC keyboard (the last 3 lines of the
 * definition have been added):
 *
 * li|ansi|Ansi standard crt:\
 * :al=\E[L:am:bs:cd=\E[J:ce=\E[K:cl=\E[2J\E[H:cm=\E[%i%d;%dH:co#80:\
 * :dc=\E[P:dl=\E[M:do=\E[B:bt=\E[Z:ei=:ho=\E[H:ic=\E[@:im=:li#25:\
 * :nd=\E[C:pt:so=\E[7m:se=\E[m:us=\E[4m:ue=\E[m:up=\E[A:\
 * :kb=^h:ku=\E[A:kd=\E[B:kl=\E[D:kr=\E[C:eo:sf=\E[S:sr=\E[T:\
 * :GS=\E[12m:GE=\E[10m:GV=\63:GH=D:\
 * :GC=E:GL=\64:GR=C:RT=^J:G1=?:G2=Z:G3=@:G4=Y:GU=A:GD=B:\
 * :CW=\E[M:NU=\E[N:RF=\E[O:RC=\E[P:\ :WL=\E[S:WR=\E[T:CL=\E[U:CR=\E[V:\
 * :HM=\E[H:EN=\E[F:PU=\E[I:PD=\E[G:\
 * :k1=\E[M:k2=\E[N:k3=\E[O:k4=\E[P:k5=\E[Q:\
 * :k6=\E[R:k7=\E[S:k8=\E[T:k9=\E[U:k0=\E[V:\
 * :kh=\E[H:kH=\E[F:kA=\E[L:kN=\E[G:kP=\E[I:
 *
 */

#define termdef 1		/* don't define "term" external */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"

#if TERMCAP
# if 1 /* XXX */
#  include <termcap.h>
# else
   int tgetent(char*,char*);
   void tputs(char*,int,int(*)(int));
   char *tgetstr();
# endif
#endif

#define MARGIN	8
#define SCRSIZ	64
#define NPAUSE	200	/* # times thru update to pause */
#define BEL	0x07
#define ESC	0x1B

/* Termcap Sequence definitions	 */

typedef struct TBIND {
	char p_name[4];		/* sequence name */
	short p_code;		/* resulting keycode of sequence */
	char p_seq[10];		/* terminal escape sequence */
} TBIND;

TBIND ttable[] = {
	{	"bt", SHFT | CTRLBIT | 'i',	""	},	/* backtab */
	{	"k1", SPEC | '1',		""	},	/* function key 1 */
	{	"k2", SPEC | '2',		""	},	/* function key 2 */
	{	"k3", SPEC | '3',		""	},	/* function key 3 */
	{	"k4", SPEC | '4',		""	},	/* function key 4 */
	{	"k5", SPEC | '5',		""	},	/* function key 5 */
	{	"k6", SPEC | '6',		""	},	/* function key 6 */
	{	"k7", SPEC | '7',		""	},	/* function key 7 */
	{	"k8", SPEC | '8',		""	},	/* function key 8 */
	{	"k9", SPEC | '9',		""	},	/* function key 9 */
	{	"k0", SPEC | '0',		""	},	/* function key 10 */
	{	"kA", CTRLBIT | 'O',		""	},	/* insert line */
	{	"kb", CTRLBIT | 'H',		""	},	/* backspace */
	{	"kC", CTRLBIT | 'L',		""	},	/* clear screen */
	{	"kD", SPEC | 'D',		""	},	/* delete character */
	{	"kd", SPEC | 'N',		""	},	/* down cursor */
	{	"kE", CTRLBIT | 'K',		""	},	/* clear to end of line */
	{	"kF", CTRLBIT | 'V',		""	},	/* scroll down */
	{	"kH", SPEC | '>',		""	},	/* home down [END?] key */
	{	"kh", SPEC | '<',		""	},	/* home */
	{	"kI", SPEC | 'C',		""	},	/* insert character */
	{	"kL", CTRLBIT | 'K',		""	},	/* delete line */
	{	"kl", SPEC | 'B',		""	},	/* left cursor */
	{	"kN", SPEC | 'V',		""	},	/* next page */
	{	"kP", SPEC | 'Z',		""	},	/* previous page */
	{	"kR", CTRLBIT | 'Z',		""	},	/* scroll down */
	{	"kr", SPEC | 'F',		""	},	/* right cursor */
	{	"ku", SPEC | 'P',		""	}	/* up cursor */
};

#define	NTBINDS	sizeof(ttable)/sizeof(TBIND)

int tcapkopen(void);
int tcapkclose(void);
int tcapgetc(void);
int tcapmove(int,int);
int tcapeeol(void);
int tcapeeop(void);
int tcapbeep(void);

int tcaprev(int);
int tcapbold(int);
int tcapline(int);

int tcapcres(int);
int tcapopen(void);
int tcapclose(void);
int tcapattr(int);

int tcapti(void);
int tcapte(void);
int tcapiste(void);

int putnpad(char *,int);
unsigned int extcode(unsigned int);

int in_init(void);
int in_get(void);
int in_put(int);
int in_check(void);


#define TCAPSLEN 1024
char tcapbuf[TCAPSLEN];
char *UP, PC, *CM, *CE, *CL, *SO, *SE, *IS, *KS, *KE, *TI, *TE, *MD, *ME, *US, *UE;

TERM term = {
	0, 0, 0, 0,		/* these four values are set dynamically at open time */
	MARGIN,
	SCRSIZ,
	NPAUSE,
	tcapopen,
	tcapclose,
	tcapkopen,
	tcapkclose,
	tcapgetc,
	ttputc,
	ttflush,
	tcapmove,
	tcapeeol,
	tcapeeop,
	tcapbeep,
	tcaprev,
	tcapbold,
	tcapline,
	tcapcres,
	tcapattr
};

/* input buffers and pointers	 */

#define	IBUFSIZE	64	/* this must be a power of 2 */

static unsigned char in_buf[IBUFSIZE];	/* input character buffer */
static int in_next = 0;		/* pos to retrieve next input character */
static int in_last = 0;		/* pos to place most recent input character */

int
in_init(void)
{				/* initialize the input buffer */
	in_next = in_last = 0;
	return TRUE;
}

int
in_check(void)
{				/* is the input buffer non-empty? */
	if (in_next == in_last)
		return FALSE;
	else
		return TRUE;
}

int
in_put(int event)	/* event to enter into the input buffer */
{
	in_buf[in_last++] = event;
	in_last &= (IBUFSIZE - 1);
	return TRUE;
}

/* get an event from the input buffer */
int
in_get(void)
{
	int event;		/* event to return */

	event = in_buf[in_next++];
	in_next &= (IBUFSIZE - 1);
	return event;
}

/*
 * Open the terminal put it in RA mode learn about the screen size read
 * TERMCAP strings for function keys
 */
int
tcapopen(void)
{
	int idx;
	char *t, *p;
	char tcbuf[1024];
	char *tv_stype;
	char err_str[72];

	if ((tv_stype = getenv("TERM")) == (char *)NULL) {
		puts(TEXT182);
		/* "Environment variable TERM not defined!" */
		meexit(1);
	}
	if ((tgetent(tcbuf, tv_stype)) != 1) {
fprintf(stderr, "====%s====%d====\n", tv_stype, tgetent(tcbuf, tv_stype));
exit(1);
		sprintf(err_str, TEXT183, tv_stype);
		/* "Unknown terminal type %s!" */
		puts(err_str);
		meexit(1);
	}
	term.t_nrow = getwindow_lines();
	if (term.t_nrow == 0) {
		if ((term.t_nrow = (short) tgetnum("li") - 1) == -1) {
			puts(TEXT184);
			/* "termcap entry incomplete (lines)" */
			meexit(1);
		}
	}
#define	UMACS_MAXROW	512
#ifdef UMACS_MAXROW
	term.t_mrow = UMACS_MAXROW;
#else
	term.t_mrow = term.t_nrow;
#endif

	term.t_ncol = getwindow_columns();
	if (term.t_ncol == 0) {
		if ((term.t_ncol = (short) tgetnum("co")) == -1) {
			puts(TEXT185);
			/* "Termcap entry incomplete (columns)" */
			meexit(1);
		}
	}
	if (term.t_ncol < 8) {
		term.t_ncol = 8;
		puts("columns too short");
	}
	term.t_mcol = term.t_ncol;

	p = tcapbuf;
	t = tgetstr("pc", &p);
	if (t)
		PC = *t;

	CL = tgetstr("cl", &p);
	CM = tgetstr("cm", &p);
	CE = tgetstr("ce", &p);
	UP = tgetstr("up", &p);
	SE = tgetstr("se", &p);
	SO = tgetstr("so", &p);
	if (SO != NULL)
		revexist = TRUE;

	MD = tgetstr("md", &p);
	ME = tgetstr("me", &p);
	US = tgetstr("us", &p);
	UE = tgetstr("ue", &p);

	if (MD == NULL && US == NULL)
		term.t_bold = term.t_line = term.t_rev;
	else if (MD == NULL)
		term.t_bold = term.t_line;
	else if (US == NULL)
		term.t_line = term.t_bold;


	if (CL == NULL || CM == NULL || UP == NULL) {
		puts(TEXT186);
		/* "Incomplete termcap entry\n" */
		meexit(1);
	}
	if (CE == NULL)		/* will we be able to use clear to EOL? */
		eolexist = FALSE;

	IS = tgetstr("is", &p);	/* extract init string */
	TI = tgetstr("ti", &p);	/* extract terminal init */
	TE = tgetstr("te", &p);	/* extract terminal end */
	KS = tgetstr("ks", &p);	/* extract keypad transmit string */
	KE = tgetstr("ke", &p);	/* extract keypad transmit end string */

	/* read definitions of various function keys into ttable */
	for (idx = 0; idx < NTBINDS; idx++) {
		strcpy(ttable[idx].p_seq,
		       fixnull(tgetstr(ttable[idx].p_name, &p)));
	}

	/* tell unix we are goint to use the terminal */
	ttopen();

	/* make sure we don't over run the buffer (TOO LATE I THINK) */
	if (p >= &tcapbuf[TCAPSLEN]) {
		puts(TEXT187);
		/* "Terminal description too big!\n" */
		meexit(1);
	}
	/* send init strings if defined */
#if 0
	if (IS != NULL)
		putpad(IS);
#else
	if (TI != NULL)
		putpad(TI);
#endif

	if (KS != NULL)
		putpad(KS);

	/* initialize the input buffer */
	in_init();

	return TRUE;
}

int
tcapiste(void)
{
	if (TE != NULL)
		return TRUE;
	return FALSE;
}

int
tcapte(void)
{
	if (TE != NULL)
		putpad(TE);
	return TRUE;
}

int
tcapti(void)
{
	if (TI != NULL)
		putpad(TI);
	return TRUE;
}


int
tcapclose(void)
{
	tcapattr(0);

	if (TE != NULL)
		putpad(TE);

	/* send end-of-keypad-transmit string if defined */
	if (KE != NULL)
		putpad(KE);

	ttclose();
	return TRUE;
}

int
tcapkopen(void)
{
	strcpy(sres, "NORMAL");
	return TRUE;
}

int
tcapkclose(void)
{
	return TRUE;
}

unsigned int
extcode(unsigned int c)
{
	return c;
}

/*
 * TCAPGETC:	Get on character.  Resolve and setup all the appropriate
 * keystroke escapes as defined in the comments at the beginning of input.c
 */
int
tcapgetc(void)
{
	int c;	/* current extended keystroke */

	/* if there are already keys waiting.... send them */
	if (in_check())
		return in_get();

	/* otherwise... get the char for now */
	c = get1key();

	/* unfold the control bit back into the character */
	if ((CTRLBIT & c))
		c = (c & ~CTRLBIT) - '@';

	/* fold the event type into the input stream as an escape seq */
	if ((c & ~255) != 0) {
		in_put(0);	/* keyboard escape prefix */
		in_put(c >> 8);	/* event type */
		in_put(c & 255);/* event code */
		return tcapgetc();
	}
	return c;
}

/*
 * GET1KEY: Get one keystroke. The only prefixs legal here are the SPEC
 * and CTRLBIT prefixes.
 *
 * Note:
 *
 * Escape sequences that are generated by terminal function and cursor keys
 * could be confused with the user typing the default META prefix followed by
 * other chars... ie
 *
 * UPARROW  =  <ESC>A   on some terminals... apropos  =  M-A
 *
 * The difference is determined by measuring the time between the input of the
 * first and second character... if an <ESC> is types, and is not followed by
 * another char in 1/30 of a second (think 300 baud) then it is a user input,
 * otherwise it was generated by an escape sequence and should be SPECed.
 */

int
get1key(void)
{
	return ttgetc();
}

int
tcapmove(int row, int col)
{
	putpad(tgoto(CM, col, row));
	return TRUE;
}

int
tcapeeol(void)
{
	putpad(CE);
	return TRUE;
}

int
tcapeeop(void)
{
	putpad(CL);
	return TRUE;
}

static int current_attr;

/* change reverse video status */
int
tcaprev(int state)	/* FALSE = normal video, TRUE = reverse video */
{
	/* static int revstate = FALSE; */
	if (state) {
		if (SO != NULL)
			putpad(SO);
		current_attr |= TERMATTR_REV;
	} else {
		if (SE != NULL)
			putpad(SE);
		current_attr &= ~TERMATTR_REV;
	}
	return TRUE;
}

int
tcapbold(int state)
{
	/* static int revstate = FALSE; */
	if (state) {
		if (MD != NULL)
			putpad(MD);
		current_attr |= TERMATTR_BOLD;
	} else {
		if (ME != NULL)
			putpad(ME);
		current_attr &= ~TERMATTR_BOLD;
	}
	return TRUE;
}

int
tcapline(int state)
{
	/* static int revstate = FALSE; */
	if (state) {
		if (US != NULL)
			putpad(US);
		current_attr |= TERMATTR_LINE;
	} else {
		if (UE != NULL)
			putpad(UE);
		current_attr &= ~TERMATTR_LINE;
	}
	return TRUE;
}

/* change screen resolution */
int
tcapcres(int mode)
{
	return TRUE;
}

/* change palette string */
int
spal(char *dummy)
{
	/* Does nothing here	 */
	return TRUE;
}

/* no colors here, ignore this */
int
tcapattr(int attr)
{
	char tmp0[32];
	char tmp[64];
	int fgcol, bgcol;

	attr &= TERMATTR_MASK;

	if (attr == current_attr)
		return TRUE;

	if (ansiesc) {
		fgcol = TERMATTR_GETFG(attr);
		bgcol = TERMATTR_GETBG(attr);

		strcpy(tmp, "\e[0");

		if (fgcol) {
			sprintf(tmp0, ";%d", 30 + (fgcol & 7));
			strcat(tmp, tmp0);
		}
		if (bgcol) {
			sprintf(tmp0, ";%d", 40 + (bgcol & 7));
			strcat(tmp, tmp0);
		}
		if (attr & TERMATTR_BOLD)
			strcat(tmp, ";1");
		if (attr & TERMATTR_LINE)
			strcat(tmp, ";4");
		if (attr & TERMATTR_REV)
			strcat(tmp, ";7");
		strcat(tmp, "m");
		putpad(tmp);
	} else {
		tcapbold(attr & TERMATTR_BOLD);
		tcapline(attr & TERMATTR_LINE);
		tcaprev(attr & TERMATTR_REV);
	}
	current_attr = attr;
	return TRUE;
}

int
tcapbeep(void)
{
	ttputc(BEL);
	return TRUE;
}

int
putpad(char *str)
{
	tputs(str, 1, (void*)ttputc);
	return TRUE;
}

int
putnpad(char *str, int n)
{
	tputs(str, n, (void*)ttputc);
	return TRUE;
}
