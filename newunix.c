/***  UNIX driver/Microemacs 3.10b, Copyright 1989 D. Lawrence, C. Smith ***/

/**
	New features:

	1. Timeouts waiting on a function key have been changed from
	35000 to 500000 microseconds.

	2. Additional keymapping entries can be made from the command
	language by issuing a 'set $palette xxx'.  The format of
	xxx is a string as follows:
		"KEYMAP keybinding escape-sequence".
	To add "<ESC><[><A>" as a keybinding of FNN, issue:
		"KEYMAP FNN ^[[A".
	Note that the escape character!  It is a read escape character
	and it's pretty difficult to enter.

	3. Colors are supported.  Under AIX the colors will be pulled
	in automaticly.  For other environments, you can either add
	the termcap entries, C0 to D7.  Or the colors may be defined
	using the command language by issuing a 'set $palette xxx'
	command.  The format of xxx is a string as follows:
		"CLRMAP # escape-sequence".
	The number is a number from 0 to 15, where 0 to 7 is the
	foreground colors, and 8 to 15 as background colors.
	To add foreground color 0 for ansi terminals, issue:
		"CLRMAP 0 ^[[30m".
	
	'Porting notes:

	I have tried to create this file so that it should work
	as well as possible without changes on your part.

	However, if something does go wrong, read the following
	helpful hints:

	1. On SUN-OS4, there is a problem trying to include both
	the termio.h and ioctl.h files.  I wish Sun would get their
	act together.  Even though you get lots of redefined messages,
	it shouldn't cause any problems with the final object.

	2. In the type-ahead detection code, the individual UNIX
	system either has a FIONREAD or a FIORDCHK ioctl call.
	Hopefully, your system uses one of them and this be detected
	correctly without any intervention.

	3. Also lookout for directory handling.  The SCO Xenix system
	is the weirdest I've seen, requiring a special load file
	(see below).  Some machine call the result of a readdir() call
	a "struct direct" or "struct dirent".  Includes files are
	named differently depending of the O/S.  If your system doesn't
	have an opendir()/closedir()/readdir() library call, then
	you should use the public domain utility "ndir".

	To compile:
		Compile all files normally.
	To link:
		Select one of the following operating systems:
			SCO Xenix:
				use "-ltermcap" and "-lx";
			SUN 3 and 4:
				use "-ltermcap";
			IBM-RT, IBM-AIX, ATT UNIX, Altos UNIX, Interactive:
				use "-lcurses".
**/

/** Include files **/
#include <stdio.h>			/* Standard I/O definitions	*/
#include "estruct.h"			/* Emacs definitions		*/

/** Do nothing routine **/
int scnothing()
{
	return(0);
}

/** Only compile for UNIX machines **/
#if BSD || USG || SMOS || HPUX || SUN || XENIX

/** Include files **/
#include "eproto.h"			/* Function definitions		*/
#include "edef.h"			/* Global variable definitions	*/
#include "elang.h"			/* Language definitions		*/

/** Kill predefined **/
#undef CTRL				/* Problems with CTRL		*/

/** Overall include files **/
#include <sys/types.h>			/* System type definitions	*/
#include <sys/stat.h>			/* File status definitions	*/
#include <sys/ioctl.h>			/* I/O control definitions	*/

/** Additional include files **/
#if BSD
#include <sys/time.h>			/* Timer definitions		*/
#endif /* BSD */

#if BSD || SUN
#include <signal.h>			/* Signal definitions		*/
#endif /* BSD || SUN */

#if USG || SMOS || HPUX || SUN || XENIX
#include <termio.h>			/* Terminal I/O definitions	*/
#endif /* USG || SMOS || HPUX || SUN || XENIX */

/** Completion include files **/
/* Directory accessing: Try and figure this out... if you can! */
#if BSD
#include <sys/dir.h>			/* Directory entry definitions	*/
#define DIRENTRY	direct
#endif /* BSD */

#if XENIX
#include <sys/ndir.h>			/* Directory entry definitions	*/
#define DIRENTRY	direct
#endif /* XENIX */

#if USG || SMOS || HPUX || SUN
#include <dirent.h>			/* Directory entry definitions	*/
#define DIRENTRY	dirent
#endif /* USG || SMOS || HPUX || SUN */

/** Restore predefined definitions **/
#undef CTRL				/* Restore CTRL			*/
#define CTRL 0x0100

/** Parameters **/
#define NKEYENT		300		/* Number of keymap entries	*/
#define NINCHAR		64		/* Input buffer size		*/
#define NOUTCHAR	256		/* Output buffer size		*/
#if TERMCAP
#define NCAPBUF		1024		/* Termcap storage size		*/
#endif /* TERMCAP */
#define MARGIN		8		/* Margin size			*/
#define SCRSIZ		64		/* Scroll for margin		*/
#define NPAUSE		10		/* # times thru update to pause */

/** CONSTANTS **/
#define TIMEOUT		255		/* No character available	*/

/** Type definitions **/
struct keyent {				/* Key mapping entry		*/
	struct keyent * samlvl;		/* Character on same level	*/
	struct keyent * nxtlvl;		/* Character on next level	*/
	unsigned char ch;		/* Character			*/
	int code;			/* Resulting keycode		*/
};
#if TERMCAP
struct capbind {			/* Capability binding entry	*/
	char * name;			/* Termcap name			*/
	char * store;			/* Storage variable		*/
};
struct keybind {			/* Keybinding entry		*/
	char * name;			/* Termcap name			*/
	int value;			/* Binding value		*/
};
#endif /* TERMCAP */

/** Local variables **/
#if BSD
static struct sgttyb cursgtty;		/* Current modes		*/
static struct sgttyb oldsgtty;		/* Original modes		*/
static struct tchars oldtchars;		/* Current tchars		*/
static struct ltchars oldlchars;	/* Current ltchars		*/
static char blank[6] =			/* Blank out character set	*/
	{ -1, -1, -1, -1, -1, -1 };
#endif /* BSD */
#if USG || SMOS || HPUX || SUN || XENIX
static struct termio curterm;		/* Current modes		*/
static struct termio oldterm;		/* Original modes		*/
#endif /* USG || SMOS || HPUX || SUN || XENIX */
#if TERMCAP
static char tcapbuf[NCAPBUF];		/* Termcap character storage	*/
#define CAP_CL		0		/* Clear to end of page		*/
#define CAP_CM		1		/* Cursor motion		*/
#define CAP_CE		2		/* Clear to end of line		*/
#define CAP_SE		3		/* Standout ends		*/
#define CAP_SO		4		/* Standout (reverse video)	*/
#define CAP_IS		5		/* Initialize screen		*/
#define CAP_KS		6		/* Keypad mode starts		*/
#define CAP_KE		7		/* Keypad mode ends		*/
#define CAP_VB		8		/* Visible bell			*/
#if COLOR
#define CAP_C0		9		/* Foreground color #0		*/
#define CAP_C1		10		/* Foreground color #1		*/
#define CAP_C2		11		/* Foreground color #2		*/
#define CAP_C3		12		/* Foreground color #3		*/
#define CAP_C4		13		/* Foreground color #4		*/
#define CAP_C5		14		/* Foreground color #5		*/
#define CAP_C6		15		/* Foreground color #6		*/
#define CAP_C7		16		/* Foreground color #7		*/
#define CAP_D0		17		/* Background color #0		*/
#define CAP_D1		18		/* Background color #1		*/
#define CAP_D2		19		/* Background color #2		*/
#define CAP_D3		20		/* Background color #3		*/
#define CAP_D4		21		/* Background color #4		*/
#define CAP_D5		22		/* Background color #5		*/
#define CAP_D6		23		/* Background color #6		*/
#define CAP_D7		24		/* Background color #7		*/
#endif /* COLOR */
static struct capbind capbind[] = {	/* Capability binding list	*/
	{ "cl" },			/* Clear to end of page		*/
	{ "cm" },			/* Cursor motion		*/
	{ "ce" },			/* Clear to end of line		*/
	{ "se" },			/* Standout ends		*/
	{ "so" },			/* Standout (reverse video)	*/
	{ "is" },			/* Initialize screen		*/
	{ "ks" },			/* Keypad mode starts		*/
	{ "ke" },			/* Keypad mode ends		*/
	{ "vb" },			/* Visible bell			*/
#if COLOR
	{ "c0" },			/* Color #0			*/
	{ "c1" },			/* Color #0			*/
	{ "c2" },			/* Color #0			*/
	{ "c3" },			/* Color #0			*/
	{ "c4" },			/* Color #0			*/
	{ "c5" },			/* Color #0			*/
	{ "c6" },			/* Color #0			*/
	{ "c7" },			/* Color #0			*/
	{ "d0" },			/* Color #0			*/
	{ "d1" },			/* Color #0			*/
	{ "d2" },			/* Color #0			*/
	{ "d3" },			/* Color #0			*/
	{ "d4" },			/* Color #0			*/
	{ "d5" },			/* Color #0			*/
	{ "d6" },			/* Color #0			*/
	{ "d7" }			/* Color #0			*/
#endif /* COLOR */
};
static struct keybind keybind[] = {	/* Keybinding list		*/
	{ "bt", SHFT|CTRL|'i' },	/* Back-tab key			*/
	{ "k1", SPEC|'1' },		/* F1 key			*/
	{ "k2", SPEC|'2' },		/* F2 key			*/
	{ "k3", SPEC|'3' },		/* F3 key			*/
	{ "k4", SPEC|'4' },		/* F4 key			*/
	{ "k5", SPEC|'5' },		/* F5 key			*/
	{ "k6", SPEC|'6' },		/* F6 key			*/
	{ "k7", SPEC|'7' },		/* F7 key			*/
	{ "k8", SPEC|'8' },		/* F8 key			*/
	{ "k9", SPEC|'9' },		/* F9 key			*/
	{ "k0", SPEC|'0' },		/* F0 or F10 key		*/
	{ "kA", CTRL|'O' },		/* Insert line key		*/
	{ "kb", CTRL|'H' },		/* Backspace key		*/
	{ "kC", CTRL|'L' },		/* Clear screen key		*/
	{ "kD", SPEC|'D' },		/* Delete character key		*/
	{ "kd", SPEC|'N' },		/* Down arrow key		*/
	{ "kE", CTRL|'K' },		/* Clear to end of line key	*/
	{ "kF", CTRL|'V' },		/* Scroll forward key		*/
	{ "kH", SPEC|'>' },		/* Home down key		*/
	{ "kh", SPEC|'<' },		/* Home key			*/
	{ "kI", SPEC|'C' },		/* Insert character key		*/
	{ "kL", CTRL|'K' },		/* Delete line key		*/
	{ "kl", SPEC|'B' },		/* Left arrow key		*/
	{ "kN", SPEC|'V' },		/* Next page key		*/
	{ "kP", SPEC|'Z' },		/* Previous page key		*/
	{ "kR", CTRL|'Z' },		/* Scroll backward key		*/
	{ "kr", SPEC|'F' },		/* Right arrow key		*/
	{ "ku", SPEC|'P' }		/* Up arrow key			*/
};
#endif /* TERMCAP */
static int inbuf[NINCHAR];		/* Input buffer			*/
static int * inbufh =			/* Head of input buffer		*/
	inbuf;
static int * inbuft =			/* Tail of input buffer		*/
	inbuf;
static unsigned char outbuf[NOUTCHAR];	/* Output buffer		*/
static unsigned char * outbuft = 	/* Output buffer tail		*/
	outbuf;
static unsigned char keyseq[256];	/* Prefix escape sequence table	*/
static struct keyent keymap[NKEYENT];	/* Key map			*/
static struct keyent * nxtkey =		/* Next free key entry		*/
	keymap;
static DIR *dirptr = NULL;		/* Current directory stream	*/
static char path[NFILEN];		/* Path of file to find		*/
static char rbuf[NFILEN];		/* Return file buffer		*/
static char *nameptr;			/* Ptr past end of path in rbuf	*/

/** Terminal definition block **/
int scopen(), scclose(), ttgetc(), ttputc(), ttflush();
int scmove(), sceeol(), sceeop(), scbeep(), screv();
int sckopen(); int sckclose();
#if COLOR
int scfor(), scbac();
#endif /* COLOR */
TERM term = {	60,				/* Maximum number of rows	*/
	0,				/* Current number of rows	*/
	132,				/* Maximum number of columns	*/
	0,				/* Current number of columns	*/
	MARGIN,				/* Margin for extending lines	*/
	SCRSIZ,				/* Scroll size for extending	*/
	NPAUSE,				/* # times thru update to pause	*/
	scopen,				/* Open terminal routine	*/
	scclose,			/* Close terminal routine	*/
	sckopen,			/* Open keyboard routine	*/
	sckclose,			/* Close keyboard routine	*/
	ttgetc,				/* Get character routine	*/
	ttputc,				/* Put character routine	*/
	ttflush,			/* Flush output routine		*/
	scmove,				/* Move cursor routine		*/
	sceeol,				/* Erase to end of line routine	*/
	sceeop,				/* Erase to end of page routine	*/
	scbeep,				/* Beep! routine		*/
	screv,				/* Set reverse video routine	*/
	scnothing,			/* Set resolution routine	*/
#if COLOR
	scfor,				/* Set forground color routine	*/
	scbac				/* Set background color routine	*/
#endif
};

/** Open terminal device **/
int ttopen()
{
#if BSD
	/* Get tty modes */
	if (ioctl(0, TIOCGETP, &oldsgtty) ||
		ioctl(0, TIOCGETC, &oldtchars) ||
		ioctl(0, TIOCGLTC, &oldlchars))
		return -1;

	/* Save to original mode variables */
	cursgtty = oldsgtty;

	/* Set new modes */
	cursgtty.sg_flags |= RAW;	/* was CBREAK	bd, 16/02/90	*/
	cursgtty.sg_flags &= ~(ECHO|CRMOD);

	/* Set tty modes */
	if (ioctl(0, TIOCSETP, &cursgtty) ||
		ioctl(0, TIOCSETC, blank) ||
		ioctl(0, TIOCSLTC, blank))
		return -1;
#endif /* BSD */
#if USG || SMOS || HPUX || SUN || XENIX

#if	SMOS
	set_parm(0,-1,-1);		/* extended settings */
					/*	890619mhs A3 */
#endif

	/* Get modes */
	if (ioctl(0, TCGETA, &oldterm))
		return -1;

	/* Save to original mode variable */
	curterm = oldterm;

	/* Set new modes */
	curterm.c_iflag &= ~(INLCR|ICRNL|IGNCR);	/* =0 ?	*/
	curterm.c_lflag &= ~(ICANON|ISIG|ECHO);		/* =0 ?	*/
	curterm.c_cc[VMIN] = 1;
	curterm.c_cc[VTIME] = 0;

#if	SMOS
	/****THIS IS A BIG GUESS ON MY PART... the code changed
	  too much between versions for me to be sure this will work - DML */

	/* allow multiple (dual) sessions if already enabled */
	curterm.c_lflag = oldterm.c_lflag & ISIG;

	/* use old SWTCH char if necessary */
	if (curterm.c_lflag != 0)
		curterm.c_cc[VSWTCH] = oldterm.c_cc[VSWTCH];

	/* Copy VTI settings	*/
	curterm.c_cc[VTBIT] = oldterm.c_cc[VTBIT];

	set_parm(0,-1,-1);		/* extended settings */
#endif					/*	890619mhs A3 */

	/* Set tty mode */
	if (ioctl(0, TCSETA, &curterm))
		return -1;
#endif /* USG || SMOS || HPUX || SUN || XENIX */

	/* Success */
	return(0);
}

/** Close terminal device **/
int ttclose()
{
	/* Restore original terminal modes */
#if BSD
	if (	ioctl(0, TIOCSETP, &oldsgtty) ||
		ioctl(0, TIOCSETC, &oldtchars) ||
		ioctl(0, TIOCSLTC, &oldlchars))
		return -1;
#endif /* BSD */

#if	SMOS
	set_parm(0,-1,-1);		/* extended settings */
					/*	890619mhs A3 */
#endif

#if USG || SMOS || HPUX || SUN || XENIX
	if (ioctl(0, TCSETA, &oldterm))
		return -1;
#endif /* USG || SMOS || HPUX || SUN || XENIX */

	/* Success */
	return(0);
}

/** Flush output buffer to display **/
int ttflush()
{
	int len, status;

	/* Compute length of write */
	len = outbuft - outbuf;
	if (len == 0)
		return(0);

	/* Reset buffer position */
	outbuft = outbuf;

	/* Perform write to screen */
	status = (fwrite(outbuf, 1, len, stdout) != len);
	fflush(stdout);
	return(status);
}

/** Put character onto display **/
int ttputc(ch)
char ch;				/* Character to display		*/
{
	/* Check for buffer full */
	if (outbuft == &outbuf[sizeof(outbuf)])
		ttflush();

	/* Add to buffer */
	*outbuft++ = ch;
	return(0);
}

/** Add character sequence to keycode entry **/
void addkey(seq, fn)
unsigned char * seq;			/* Character sequence		*/
int fn;					/* Resulting keycode		*/
{
	int first;
	struct keyent * cur, * nxtcur;

	/* Skip on null sequences */
	if (!seq)
		return;

	/* Skip single character sequences */
	if (strlen(seq) < 2)
		return;

	/* If no keys defined, go directly to insert mode */
	first = 1;
	if (nxtkey != keymap) {
		
		/* Start at top of key map */
		cur = keymap;
		
		/* Loop until matches exhast */
		while (*seq) {
			
			/* Do we match current character */
			if (*seq == cur->ch) {
				
				/* Advance to next level */
				seq++;
				cur = cur->nxtlvl;
				first = 0;
			} else {
				
				/* Try next character on same level */
				nxtcur = cur->samlvl;
				
				/* Stop if no more */
				if (nxtcur)
					cur = nxtcur;
				else
					break;
			}
		}
	}
	
	/* Check for room in keymap */
	if (strlen(seq) > NKEYENT - (nxtkey - keymap))
		return;
		
	/* If first character in sequence is inserted, add to prefix table */
	if (first)
		keyseq[*seq] = 1;
		
	/* If characters are left over, insert them into list */
	for (first = 1; *seq; first = 0) {
		
		/* Make new entry */
		nxtkey->ch = *seq++;
		nxtkey->code = fn;
		
		/* If root, nothing to do */
		if (nxtkey != keymap) {
			
			/* Set first to samlvl, others to nxtlvl */
			if (first)
				cur->samlvl = nxtkey;
			else
				cur->nxtlvl = nxtkey;
		}

		/* Advance to next key */
		cur = nxtkey++;
	}
}

/** Grab input characters, with wait **/
unsigned char grabwait()
{
#if BSD
	unsigned char ch;

	/* Perform read */
	if (read(0, &ch, 1) != 1) {
		puts("** Horrible read error occured **");
		exit(1);
	}
	return ch;
#endif /* BSD */
#if USG || SMOS || HPUX || SUN || XENIX
	unsigned char ch;

	/* Change mode, if necessary */
	if (curterm.c_cc[VTIME]) {
		curterm.c_cc[VMIN] = 1;
		curterm.c_cc[VTIME] = 0;
		ioctl(0, TCSETA, &curterm);
	}

	/* Perform read */
	if (read(0, &ch, 1) != 1) {
		puts("** Horrible read error occured **");
		exit(1);
	}

	/* Return new character */
	return ch;
#endif /* USG || SMOS || HPUX || SUN || XENIX */
}

/** Grab input characters, short wait **/
unsigned char grabnowait()
{
#if BSD
	static struct timeval timout = { 0, 500000L };
	int count, r;

	/* Select input */
	r = 1;
	count = select(1, &r, NULL, NULL, &timout);
	if (count == 0)
		return TIMEOUT;
	if (count < 0) {
		puts("** Horrible select error occured **");
		exit(1);
	}

	/* Perform read */
	return grabwait();
#endif /* BSD */
#if USG || SMOS || HPUX || SUN || XENIX
	int count;
	unsigned char ch;

	/* Change mode, if necessary */
	if (curterm.c_cc[VTIME] == 0) {
		curterm.c_cc[VMIN] = 0;
		curterm.c_cc[VTIME] = 5;
		ioctl(0, TCSETA, &curterm);
	}

	/* Perform read */
	count = read(0, &ch, 1);
	if (count < 0) {
		puts("** Horrible read error occured **");
		exit(1);
	}
	if (count == 0)
		return TIMEOUT;

	/* Return new character */
	return ch;
#endif /* USG || SMOS || HPUX || SUN || XENIX */
}

/** Queue input character **/
void qin(ch)
int ch;					/* Character to add		*/
{
	/* Check for overflow */
	if (inbuft == &inbuf[sizeof(inbuf)]) {
		
		/* Annoy user */
		scbeep();
		return;
	}
	
	/* Add character */
	*inbuft++ = ch;
}

/** Cook input characters **/
void cook()
{
	unsigned char ch;
	struct keyent * cur;
	
	/* Get first character untimed */
	ch = grabwait();
	qin(ch);
	
	/* Skip if the key isn't a special leading escape sequence */
	if (keyseq[ch] == 0)
		return;

	/* Start at root of keymap */
	cur = keymap;

	/* Loop until keymap exhasts */
	while (cur) {

		/* Did we find a matching character */
		if (cur->ch == ch) {

			/* Is this the end */
			if (cur->nxtlvl == NULL) {

				/* Replace all character with new sequence */
				inbuft = inbuf;
				qin(cur->code);
				return;
			} else {
				/* Advance to next level */
				cur = cur->nxtlvl;
			
				/* Get next character, timed */
				ch = grabnowait();
				if (ch == TIMEOUT)
					return;

				/* Queue character */
				qin(ch);
			}
		} else
			/* Try next character on same level */
			cur = cur->samlvl;
	}
}

/** Return cooked characters **/
int ttgetc()
{
	int ch;

	/* Loop until character found */
	while (1) {
	
		/* Get input from buffer, if available */
		if (inbufh != inbuft) {
			ch = *inbufh++;
			if (inbufh == inbuft)
				inbufh = inbuft = inbuf;
			break;
		} else

			/* Fill input buffer */
			cook();
	}

	/* Return next character */
	return ch;
}

#if TYPEAH
int typahead()
{
	/* See if internal buffer is non-empty */
	if (inbufh != inbuft)
		return 1;

	/* Now check with system */
#ifdef FIONREAD  /* Watch out!  This could bite you! */
	{
		int x;

		/* Get number of pending characters */
		if (ioctl(0, FIONREAD, &x))
			return(0);
		return x;
	}
#else
	{
		int count;

		/* Ask hardware for count */
		count = ioctl(0, FIORDCHK, 0);
		if (count < 0)
			return(0);
		return count;
	}
#endif /* FIONREAD */
}
#endif /* TYPEAH */

#ifdef TIOCGWINSZ
/** Get window size from system **/
int getwinsize()
{
	struct winsize winsize;

	/* Call to system for size */
	if (ioctl(0, TIOCGWINSZ, &winsize))
		return 1;

	/* Check for validity */
	if (winsize.ws_row == 0 || winsize.ws_col == 0)
		return 1;

	/* Set values */
	term.t_nrow = winsize.ws_row - 1;
	term.t_ncol = winsize.ws_col;
	return(0);
}
#endif /* TIOCGWINSZ */

#if TERMCAP
/** Put out sequence, with padding **/
void putpad(seq)
char * seq;				/* Character sequence		*/
{
	/* Check for null */
	if (!seq)
		return;

	/* Call on termcap to send sequence */
	tputs(seq, 1, ttputc);
}
#endif /* TERMCAP */

/** Initialize screen package **/
int scopen()
{
#if TERMCAP
	char * cp, tcbuf[1024];
	int status;
	struct capbind * cb;
	struct keybind * kp;

	char * getenv(), * tgetstr();
	extern char PC, * UP;
	extern short ospeed;

	/* Get terminal type */
	cp = getenv("TERM");
	if (!cp) {
		puts("Environment variable \"TERM\" not define!");
		exit(1);
	}

	/* Try to load termcap */
	status = tgetent(tcbuf, cp);
	if (status == -1) {
		puts("Cannot open termcap file");
		exit(1);
	}
	if (status == 0) {
		printf("No entry for terminal type \"%s\"\n", cp);
		exit(1);
	}

	/* Get size of screen */
#ifdef TIOCGWINSZ
	/* Get size from system if this is supported */
	if (getwinsize())
#endif
	{
		/* Get size from termcap */
		term.t_nrow = tgetnum("li") - 1;
		term.t_ncol = tgetnum("co");
	}
	if (term.t_nrow < 3 || term.t_ncol < 3) {
		puts("Cannot determine the size of the current screen");
		exit(1);
	}
/* initialize max number of rows and cols	*/
	term.t_mrow = term.t_nrow;
	term.t_mcol = term.t_ncol;

	/* Start grabbing termcap commands */
	cp = tcapbuf;

	/* Get the pad character */
	if (tgetstr("pc", &cp))
		PC = tcapbuf[0];

	/* Get up line capability */
	UP = tgetstr("up", &cp);

	/* Get other capabilities */
	cb = capbind;
	while (cb < &capbind[sizeof(capbind)/sizeof(*capbind)]) {
		cb->store = tgetstr(cb->name, &cp);
		cb++;
	}

	/* Check for minimum */
	if (!capbind[CAP_CL].store && (!capbind[CAP_CM].store || !UP)) {
		puts("This terminal doesn't have enough power to run microEmacs!");
		exit(1);
	}

	/* Set reverse video and erase to end of line */
	if (capbind[CAP_SO].store && capbind[CAP_SE].store)
		revexist = TRUE;
	if (!capbind[CAP_CE].store)
		eolexist = FALSE;

	/* Get keybindings */
	kp = keybind;
	while (kp < &keybind[sizeof(keybind)/sizeof(*keybind)]) {
		addkey(tgetstr(kp->name, &cp), kp->value);
		kp++;
	}

	/* Open terminal device */
	if (ttopen()) {
		puts("Cannot open terminal");
		exit(1);
	}

	/* Set speed for padding sequences */
#if BSD
	ospeed = cursgtty.sg_ospeed;
#endif /* BSD */
#if USG || SMOS || HPUX || SUN || XENIX
	ospeed = curterm.c_cflag & CBAUD;
#endif /* USG || SMOS || HPUX || SUN || XENIX */
	
	/* Send out initialization sequences */
	putpad(capbind[CAP_IS].store);
#endif /* TERMCAP */

	/* Success */
	return(0);
}

int sckopen()
{
#if TERMCAP
	/* Turn on keypad mode */
	putpad(capbind[CAP_KS].store);
#endif /* TERMCAP */
	return(0);
} /* end of sckopen */


int sckclose()
{
#if TERMCAP
	/* Turn off keypad mode */
	putpad(capbind[CAP_KE].store);
#endif /* TERMCAP */
	return(0);
} /* end of sckclose */

/** Close screen package **/
int scclose()
{
#if TERMCAP
	/* Close terminal device */
	ttclose();
#endif /* TERMCAP */
 
	/* Success */
	return(0);
}

/** Move cursor **/
int scmove(row, col)
int row;				/* Row number			*/
int col;				/* Column number		*/
{
#if TERMCAP
	/* Call on termcap to create move sequence */
	putpad(tgoto(capbind[CAP_CM].store, col, row));
#endif /* TERMCAP */

	/* Success */
	return(0);
}

/** Erase to end of line **/
int sceeol()
{
#if TERMCAP
	/* Send erase sequence */
	putpad(capbind[CAP_CE].store);
#endif /* TERMCAP */

	/* Success */
	return(0);
}

/** Clear screen **/
int sceeop()
{
#if TERMCAP
	/* Send clear sequence */
	putpad(capbind[CAP_CL].store);
#endif /* TERMCAP */

	/* Success */
	return(0);
}

/** Set reverse video state **/
int screv(state)
int state;				/* New state			*/
{
#if TERMCAP
	/* Set reverse video state */
	putpad(state ? capbind[CAP_SO].store : capbind[CAP_SE].store);
#endif /* TERMCAP */

	/* Success */
	return(0);
}

/** Beep **/
scbeep()
{
#if TERMCAP
	/* Send out visible bell, if it exists */
	if (capbind[CAP_VB].store)
		putpad(capbind[CAP_VB].store);
	else
		/* The old standby method */
		ttputc('\7');
#endif /* TERMCAP */

	/* Success */
	return(0);
}

#if COLOR
/** Set foreground color **/
int scfor(c)
int c;					/* Color to set			*/
{
	/* Skip if color isn't defined */
	if (!capbind[CAP_C0].store)
		return(0);

	/* Send out color sequence */
	putpad(capbind[CAP_C0 + (c & 7)].store);
	return(0);
}

/** Set background color **/
int scbac(c)
int c;					/* Color to set			*/
{
	/* Skip if color isn't defined */
	if (!capbind[CAP_C0].store)
		return(0);

	/* Send out color sequence */
	putpad(capbind[CAP_D0 + (c & 7)].store);
	return(0);
}
#endif /* COLOR */

/** Set palette **/
int spal(cmd)
char * cmd;				/* Palette command		*/
{
	int code, dokeymap;
	char * cp;

	/* Check for keymapping command */
	if (strncmp(cmd, "KEYMAP ", 7) == 0)
		dokeymap = 1;
	else if (strncmp(cmd, "CLRMAP ", 7) == 0)
		dokeymap = 0;
	else
		return(0);
	cmd += 7;

	/* Look for space */
	for (cp = cmd; *cp != '\0'; cp++)
		if (*cp == ' ') {
			*cp++ = '\0';
			break;
		}
	if (*cp == '\0')
		return 1;

	/* Perform operation */
	if (dokeymap) {

		/* Convert to keycode */
		code = stock(cmd);

		/* Add to tree */
		addkey(cp, code);
#if COLOR
	} else {

		/* Convert to color number */
		code = atoi(cmd);
		if (code < 0 || code > 15)
			return 1;

		/* Move color code to capability structure */
		capbind[CAP_C0 + code].store = malloc(strlen(cp) + 1);
		if (capbind[CAP_C0 + code].store)
			strcpy(capbind[CAP_C0 + code].store, cp);
#endif	/* COLOR */
	}
	return(0);
}

#if BSD || SUN /* Surely more than just BSD systems do this */

/** Perform a stop signal **/
int bktoshell(f, n)
{
	/* Reset the terminal and go to the last line */
	vttidy();
	
	/* Okay, stop... */
	kill(getpid(), SIGTSTP);

	/* We should now be back here after resuming */

	/* Reopen the screen and redraw */
	scopen();
	sckopen();
	curwp->w_flag = WFHARD;
	sgarbf = TRUE;

	/* Success */
	return(0);
}

#endif /* BSD || SUN */

/** Get time of day **/
char * timeset()
{
	long int buf; /* Should be time_t */
	char * sp, * cp;

	char * ctime();

	/* Get system time */
	time(&buf);

	/* Pass system time to converter */
	sp = ctime(&buf);

	/* Eat newline character */
	for (cp = sp; *cp; cp++)
		if (*cp == '\n') {
			*cp = '\0';
			break;
		}
	return sp;
}

#if USG || SMOS || HPUX || XENIX
/** Rename a file **/
int rename(file1, file2)
char * file1;				/* Old file name		*/
char * file2;				/* New file name		*/
{
	struct stat buf1;
	struct stat buf2;

	/* No good if source file doesn't exist */
	if (stat(file1, &buf1))
		return -1;

	/* Check for target */
	if (stat(file2, &buf2) == 0) {

		/* See if file is the same */
		if (buf1.st_dev == buf2.st_dev &&
			buf1.st_ino == buf2.st_ino)

			/* Not necessary to rename file */
			return(0);
	}

	/* Get rid of target */
	unlink(file2);

	/* Link two files together */
	if (link(file1, file2))
		return -1;

	/* Unlink original file */
	return unlink(file1);
}
#endif /* USG || SMOS || HPUX || XENIX */

/** Callout to system to perform command **/
int callout(cmd)
char * cmd;				/* Command to execute		*/
{
	int status;

	/* Close down */
	scmove(term.t_nrow, 0);
	ttflush();
	ttclose();

	/* Do command */
	status = system(cmd) == 0;

	/* Restart system */
        sgarbf = TRUE;
	if (ttopen()) {
		puts("** Error reopening terminal device **");
		exit(1);
	}

	/* Success */
        return status;
}

/** Create subshell **/
int spawncli(f, n)
int f;					/* Flags			*/
int n;					/* Argument count		*/
{
	char * sh;

	char * getenv();

	/* Don't allow this command if restricted */
	if (restflag)
		return resterr();

	/* Get shell path */
	sh = getenv("SHELL");
	if (!sh)
#if BSD || SUN
		sh = "/bin/csh";
#endif /* BSD || SUN */
#if USG || SMOS || HPUX || XENIX
		sh = "/bin/sh";
#endif /* USG || SMOS || HPUX || XENIX */

	/* Do shell */
	return callout(sh);
}

/** Spawn a command **/
int spawn(f, n)
int f;					/* Flags			*/
int n;					/* Argument count		*/
{
	char line[NLINE];
	int s;

	/* Don't allow this command if restricted */
	if (restflag)
		return resterr();

	/* Get command line */
	s = mlreply("!", line, NLINE);
	if (!s)
		return s;

	/* Perform the command */
	s = callout(line);

	/* if we are interactive, pause here */
	if (clexec == FALSE) {
	        mlwrite("[End]");
		ttflush();
		ttgetc();
        }
        return s;
}

/** Execute program **/
int execprg(f, n)
int f;					/* Flags			*/
int n;					/* Argument count		*/
{
	/* Same as spawn */
	return spawn(f, n);
}

/** Pipe output of program to buffer **/
int pipecmd(f, n)
int f;					/* Flags			*/
int n;					/* Argument count		*/
{
	char line[NLINE];
	int s;
	BUFFER * bp;
	WINDOW * wp;
	static char filnam[] = "command";

	/* Don't allow this command if restricted */
	if (restflag)
		return resterr();

	/* Get pipe-in command */
	s = mlreply("@", line, NLINE);
	if (!s)
		return s;

	/* Get rid of the command output buffer if it exists */
	bp = bfind(filnam, FALSE, 0);
	if (bp) {
		/* Try to make sure we are off screen */
		wp = wheadp;
		while (wp) {
			if (wp->w_bufp == bp) {
				onlywind(FALSE, 1);
				break;
			}
			wp = wp->w_wndp;
		}
		if (!zotbuf(bp))
			return(0);
	}

	/* Add output specification */
	strcat(line, ">");
	strcat(line, filnam);

	/* Do command */
	s = callout(line);
	if (!s)
		return s;

	/* Split the current window to make room for the command output */
	if (!splitwind(FALSE, 1))
		return(0);

	/* ...and read the stuff in */
	if (!getfile(filnam, FALSE))
		return(0);

	/* Make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW;
	wp = wheadp;
	while (wp) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}

	/* ...and get rid of the temporary file */
	unlink(filnam);
	return 1;
}

/** Filter buffer through command **/
int filter(f, n)
int f;					/* Flags			*/
int n;					/* Argument count		*/
{
	char line[NLINE], tmpnam[NFILEN];
	int s;
	BUFFER * bp;
	static char bname1[] = "fltinp";
	static char filnam1[] = "fltinp";
	static char filnam2[] = "fltout";

	/* Don't allow this command if restricted */
	if (restflag)
		return resterr();

	/* Don't allow filtering of VIEW mode buffer */
	if (curbp->b_mode & MDVIEW)
		return rdonly();

	/* Get the filter name and its args */
	s = mlreply("#", line, NLINE);
	if (!s)
		return s;

	/* Setup the proper file names */
	bp = curbp;
	strcpy(tmpnam, bp->b_fname);	/* Save the original name */
	strcpy(bp->b_fname, bname1);	/* Set it to our new one */

	/* Write it out, checking for errors */
	if (!writeout(filnam1, "w")) {
		mlwrite("[Cannot write filter file]");
		strcpy(bp->b_fname, tmpnam);
		return(0);
	}

	/* Setup input and output */
	strcat(line," <fltinp >fltout");

	/* Perform command */
	s = callout(line);

	/* If successful, read in file */
	if (s) {
		s = readin(filnam2, FALSE);
		if (s)
			/* Mark buffer as changed */
			bp->b_flag |= BFCHG;
	}
			

	/* Reset file name */
	strcpy(bp->b_fname, tmpnam);

	/* and get rid of the temporary file */
	unlink(filnam1);
	unlink(filnam2);

	/* Show status */
	if (!s)
		mlwrite("[Execution failed]");
	return s;
}

/** Get first filename from pattern **/
char *getffile(fspec)
char *fspec;				/* Filename specification	*/

{
	int index, point, extflag;

	/* First parse the file path off the file spec */
	strcpy(path, fspec);
	index = strlen(path) - 1;
	while (index >= 0 && (path[index] != '/' &&
		path[index] != '\\' && path[index] != ':'))
		--index;
	path[index+1] = '\0';


	/* Check for an extension */
	point = strlen(fspec) - 1;
	extflag = FALSE;
	while (point >= 0) {
		if (fspec[point] == '.') {
			extflag = TRUE;
			break;
		}
		point--;
	}

	/* Open the directory pointer */
	if (dirptr) {
		closedir(dirptr);
		dirptr = NULL;
	}

	dirptr = opendir((path[0] == '\0') ? "./" : path);

	if (!dirptr)
		return NULL;

	strcpy(rbuf, path);
	nameptr = &rbuf[strlen(rbuf)];

	/* ...and call for the first file */
	return(getnfile());
}

/** Get next filename from pattern **/
char *getnfile()

{
	int index;
	struct DIRENTRY * dp;
	struct stat fstat;

	/* ...and call for the next file */
	do {
		dp = readdir(dirptr);
		if (!dp)
			return NULL;

		/* Check to make sure we skip all weird entries except directories */
		strcpy(nameptr, dp->d_name);

	} while (stat(rbuf, &fstat) &&
		((fstat.st_mode & S_IFMT) && (S_IFREG || S_IFDIR)) == 0);

	/* if this entry is a directory name, say so */
	if ((fstat.st_mode & S_IFMT) == S_IFDIR)
		strcat(rbuf, DIRSEPSTR);

	/* Return the next file name! */
	return rbuf;
}

#endif /* BSD || USG || SMOS || HPUX || SUN || XENIX */
