/*
 * $Id: main.c,v 1.47 2017/01/02 18:22:09 ryo Exp $
 *
 *  MicroEMACS 3.10
 *      written by Daniel M. Lawrence
 *      based on code by Dave G. Conroy.
 *
 *  (C)opyright 1988,1989 by Daniel M. Lawrence
 *  MicroEMACS 3.10 can be copied and distributed freely for any
 *  non-commercial purposes. MicroEMACS 3.10 can only be incorporated
 *  into commercial software with the permission of the current author.
 *
 * This file contains the main driving routine, and some keyboard processing
 * code, for the MicroEMACS screen editor.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

/* make global definitions not external */
#define maindef

#include "estruct.h"	/* global structures and defines */
#include "etype.h"	/* variable prototype definitions */
#include "efunc.h"	/* function declarations and name table */
#include "edef.h"	/* global definitions */
#include "elang.h"	/* human language definitions */
#include "ebind.h"	/* default key bindings */
#include "keyword.h"	/* hash keyword routines */
#include "kanji.h"

#ifndef GOOD
#define GOOD	0
#endif

static void changescreen(int);
static void usage(void);

/*
	This is the primary entry point that is used by command line
	invocation, and by applications that link with microemacs in
	such a way that each invocation of Emacs is a fresh environment.

	There is another entry point in VMS.C that is used when
	microemacs is "linked" (In quotes, because it is a run-time link
	rather than a link-time link.) with applications that wish Emacs
	to preserve it's context across invocations.  (For example,
	EMACS.RC is only executed once per invocation of the
	application, instead of once per invocation of Emacs.)

	Note that re-entering an Emacs that is saved in a kept
	subprocess would require a similar entrypoint.
*/

static void
changescreen(int signo)
{
	vtfree();
	term.t_close();

	term.t_open();
	vtinit();

	newwidth(TRUE, term.t_ncol);
	newsize(TRUE, term.t_nrow + 1);

	{
		VDESC vd;
		char value[NSTRING];
		findvar("$pagelen", &vd, NVSIZE + 1);
		strcpy(value, int_asc(term.t_nrow + 1));
		svar(&vd, value);
	}
	onlywind(TRUE, 1);
	update(TRUE);
}


#if CALLED
int emacs(int,char **);
#else
int main(int,char **);
#endif

int
#if CALLED
emacs(int argc, char **argv)
#else
main(int argc, char **argv)
#endif
{
	int status;

	(void)&status;		/* stupid gcc */

	if (isatty(0) == 0) {
#if 0
		fprintf(stderr,"stdin is not a tty\n");
		exit(0);
#else
		int ttyfd;
		open("/dev/tty", O_RDONLY);
		dup2(ttyfd, 0);
		close(ttyfd);
		freopen("/dev/tty", "r", stdin);
#endif
	}

	signal(SIGWINCH, changescreen);

#if COMPLET
	{
		extern char home[NFILEN];
		char *ep;
		ep = getenv("HOME");
		if (ep) {
			strcpy(home, ep);
		} else {
			home[0] = '\0';
		}
	}
#endif


	/* Initialize the editor */
	eexitflag = FALSE;
	vtinit();		/* Terminal */
	if (eexitflag)
		goto abortrun;
	edinit("*scratch*");	/* Buffers, windows */

	varinit();		/* user variables */

	/* Process the command line and let the user edit */
	dcline(argc, argv);
	status = editloop();
abortrun:
	vttidy();
#if CLEAN
	clean();
#endif
#if CALLED
	return status;
#else
	exit(status);
#endif
}

#if CLEAN
/*
 * On some primitive operation systems, and when emacs is used as
 * a subprogram to a larger project, emacs needs to de-alloc its
 * own used memory, otherwise we just exit.
*/

int
clean(void)
{
	BUFFER *bp;		/* buffer list pointer */
	WINDOW *wp;		/* window list pointer */
	WINDOW *tp;		/* temporary window pointer */

	/* first clean up the windows */
	wp = wheadp;
	while (wp) {
		tp = wp->w_wndp;
		FREE(wp);
		wp = tp;
	}
	wheadp = NULL;

	/* then the buffers */
	bp = bheadp;
	while (bp) {
		bp->b_nwnd = 0;
		bp->b_flag = 0;	/* don't say anything about a changed buffer! */
		zotbuf(bp);
		bp = bheadp;
	}

	/* and the kill buffer */
	kdelete();

	/* clear some search variables */
#if MAGIC
	mcclear();
	rmcclear();
#endif
	if (patmatch != NULL) {
		FREE(patmatch);
		patmatch = NULL;
	}
	/* dealloc the user variables */
	varclean();

	/* and the video buffers */
	vtfree();
}
#endif


static void
usage(void)
{
	vttidy();

	fprintf(stderr,"usage: uemacs [options] [@startup-macroes] [file ...]\n");
	fprintf(stderr,"	-g<line>	for initial goto\n");
	fprintf(stderr,"	-s<str>		for initial search string\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"	-i<var> <value>	set an initial value for a variable\n");
#if CRYPT
	fprintf(stderr,"	-k<key>		for code key\n");
#endif
	fprintf(stderr,"\n");
	fprintf(stderr,"	-c		for changable file\n");
	fprintf(stderr,"	-v		for View File\n");
	fprintf(stderr,"	-r		restrictive use\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"	-S		treat SJIS input\n");
	fprintf(stderr,"	-J		treat JIS input\n");
	fprintf(stderr,"	-E		treat EUC input\n");
#ifdef ICONV
	fprintf(stderr,"	-u		treat UTF-8 input\n");
	fprintf(stderr,"	-B		treat UTF-16BE input\n");
	fprintf(stderr,"	-L		treat UTF-16LE input\n");
#endif
	fprintf(stderr,"	-l		treat Latin input\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"	-h		show usage\n");

#if CLEAN
	clean();
#endif
	exit(0);
}

/*
 * Process a command line.   May be called any time.
 */
int
dcline(int argc, char **argv)
{
	BUFFER *bp;		/* temp buffer pointer */
	int firstfile;		/* first file flag */
	int carg;		/* current arg to scan */
	int startflag;		/* startup executed flag */
	BUFFER *firstbp = NULL;	/* ptr to first buffer in cmd line */
	int viewflag;		/* are we starting in view mode? */
	int kanjicode;		/* initial kanji code */
	int gotoflag;		/* do we need to goto a line at start? */
	int gline;		/* if so, what line? */
	int searchflag;		/* Do we need to search at start? */
	int errflag;		/* C error processing? */
	VDESC vd;		/* variable num/type */
	char bname[NBUFN];	/* buffer name of file to read */

#if CRYPT
	int crypt_flag;		/* encrypting on the way in? */
	char ekey[NPAT];	/* startup encryption key */
#endif
	viewflag = FALSE;	/* view mode defaults off in command line */
	kanjicode = KANJI_ASCII;
	gotoflag = FALSE;	/* set to off to begin with */
	searchflag = FALSE;	/* set to off to begin with */
	firstfile = TRUE;	/* no file to edit yet */
	startflag = FALSE;	/* startup file not executed yet */
	errflag = FALSE;	/* not doing C error parsing */
#if CRYPT
	crypt_flag = FALSE;	/* no encryption by default */
#endif

	(void)&gline;		/* stupid gcc */

	/* Parse a command line */
	for (carg = 1; carg < argc; ++carg) {

		/* Process Switches */
		if (argv[carg][0] == '-') {
			switch (argv[carg][1]) {
				/* Process Startup macroes */
			case 'c':	/* -c for changable file */
				viewflag = FALSE;
				break;
			case 'e':	/* -e process error file */
				errflag = TRUE;
				break;
			case 'g':	/* -g for initial goto */
				gotoflag = TRUE;
				gline = asc_int(&argv[carg][2]);
				break;
			case 'i':	/* -i<var> <value> set an initial value for a variable */
				bytecopy(bname, &argv[carg][2], NVSIZE);
				findvar(bname, &vd, NVSIZE + 1);
				if (vd.v_type == -1) {
					mlwrite(TEXT52, bname);
					/* "%%No such variable as '%s'" */
					break;
				}
				svar(&vd, argv[++carg]);
				break;
#if !CALLED
			case 'h':	/* -h show usage */
				usage();
				exit(0);
				break;
#endif
#if CRYPT
			case 'k':	/* -k<key> for code key */
				crypt_flag = TRUE;
				strcpy(ekey, &argv[carg][2]);
				break;
#endif
			case 'r':	/* -r restrictive use */
				restflag = TRUE;
				break;
			case 's':	/* -s for initial search string */
				searchflag = TRUE;
				bytecopy(pat, &argv[carg][2], NPAT);
				setjtable();
				break;
			case 'v':	/* -v for View File */
				viewflag = TRUE;
				break;
			case 'E':
				kanjicode = KANJI_EUC;
				break;
			case 'J':
				kanjicode = KANJI_JIS;
				break;
			case 'S':
				kanjicode = KANJI_SJIS;
				break;
			case 'l':
				kanjicode = KANJI_ASCII;
				break;
#ifdef ICONV
			case 'u':
				kanjicode = KANJI_UTF8;
				break;
			case 'L':
				kanjicode = KANJI_UTF16LE;
				break;
			case 'B':
				kanjicode = KANJI_UTF16BE;
				break;
#endif
			default:	/* unknown switch */
				/* ignore this for now */
				break;
			}
		} else if (argv[carg][0] == '@') {
			/* Process Startup macroes */
			if (startup(&argv[carg][1]) == TRUE) {
				/* don't execute emacs.rc */
				startflag = TRUE;
			}
		}
	}

	/*
	 * if invoked with no other startup files, run the system startup
	 * file here
	 */
	if (!startflag && !errflag)
		startup("");

	/* Parse a command line */
	for (carg = 1; carg < argc; ++carg) {
		if (argv[carg][0] == '-') {
			switch (argv[carg][1]) {
			case 'c':
				viewflag = FALSE;
				break;
			case 'v':
				viewflag = TRUE;
				break;
				break;
			case 'E':
				kanjicode = KANJI_EUC;
				break;
			case 'J':
				kanjicode = KANJI_JIS;
				break;
			case 'S':
				kanjicode = KANJI_SJIS;
				break;
			case 'l':
				kanjicode = KANJI_ASCII;
				break;
#ifdef ICONV
			case 'u':
				kanjicode = KANJI_UTF8;
				break;
			case 'L':
				kanjicode = KANJI_UTF16LE;
				break;
			case 'B':
				kanjicode = KANJI_UTF16BE;
				break;
#endif
			}
		} else if (argv[carg][0] != '@') {
			/* Process an input file */
			/* set up a buffer for this file */
			makename(bname, argv[carg]);
			unqname(bname);

			/* set this to inactive */
			bp = bfind(bname, TRUE, 0);
			strcpy(bp->b_fname, argv[carg]);
			bp->b_active = FALSE;
			if (firstfile) {
				firstbp = bp;
				firstfile = FALSE;
			}
			/* set the modes appropriatly */
			if (viewflag)
				bp->b_mode |= MDVIEW;

			bp->b_kanjicode = kanjicode;
#if CRYPT
			if (crypt_flag) {
				bp->b_mode |= MDCRYPT;
				p_crypt((char *) NULL, 0);
				p_crypt(ekey, strlen(ekey));
				bytecopy((char *)bp->b_key, ekey, NPAT);
			}
#endif
		}
	}

	/* if we are C error parsing... run it! */
	if (errflag) {
		if (startup("error.cmd") == TRUE)
			startflag = TRUE;
	}
	/* if there are any files to read, read the first one! */
	bp = bfind("*scratch*", FALSE, 0);

	/* update local variables */
	bp->b_mlcol = mlcol;
	bp->b_mode |= gmode;

	if (firstfile == FALSE && (gflags & GFREAD)) {
		swbuffer(firstbp);
		curbp->b_mode |= gmode;
		update(TRUE);
		mlwrite(lastmesg);
		zotbuf(bp);
	}

	/* Deal with startup gotos and searches */
	if (gotoflag && searchflag) {
		update(FALSE);
		mlwrite(TEXT101);
		/* "[Can not search and goto at the same time!]" */
	} else {
		if (gotoflag) {
			if (gotoline(TRUE, gline) == FALSE) {
				update(FALSE);
				mlwrite(TEXT102);
				/* "[Bogus goto argument]" */
			}
		} else {
			if (searchflag) {
				if (forwhunt(FALSE, 0) == FALSE)
					update(FALSE);
			}
		}
	}
	return TRUE;
}

/*
 * This is called to let the user edit something.
 * Note that if you arrange to be able to call this from a macro,
 * you will have invented the "recursive-edit" function.
*/

int
editloop(void)
{
	int c;		/* command character */
	int f;		/* default flag */
	int n;		/* numeric repeat count */
	int mflag;	/* negative flag on repeat */
	int basec;	/* c stripped of meta character */
	int oldflag;	/* old last flag value */

	/* setup to process commands */
	lastflag = 0;	/* Fake last flags.	 */

loop:
	/* if we were called as a subroutine and want to leave, do so */
	if (eexitflag)
		return eexitval;

	/* execute the "command" macro...normally null */
	oldflag = lastflag;	/* preserve lastflag through this */
	exechook(cmdhook);
	lastflag = oldflag;

	/* Fix up the screen */
	update(FALSE);

	/* get the next command from the keyboard */
	c = getkey();


	/* if there is something on the command line, clear it */
	if (mpresf != FALSE) {
		mlerase();
		update(FALSE);
#if CLRMSG
		if (c == ' ')	/* ITS EMACS does this */
			goto loop;
#endif
	}
	/* override the arguments if prefixed */
	if (prefix) {
		if (islower(c & 255))
			c = (c & ~255) | upperc(c & 255);
		c |= prefix;
		f = predef;
		n = prenum;
		prefix = 0;

	} else {
		f = FALSE;
		n = 1;
	}

	/* do META-# processing if needed */

	basec = c & ~META;	/* strip meta char off if there */
	if ((c & META) && ((basec >= '0' && basec <= '9') || basec == '-') &&
	    (getbind(c) == NULL)) {
		f = TRUE;	/* there is a # arg */
		n = 0;		/* start with a zero default */
		mflag = 1;	/* current minus flag */
		c = basec;	/* strip the META */
		while ((c >= '0' && c <= '9') || (c == '-')) {
			if (c == '-') {
				/* already hit a minus or digit? */
				if ((mflag == -1) || (n != 0))
					break;
				mflag = -1;
			} else {
				n = n * 10 + (c - '0');
			}
			if ((n == 0) && (mflag == -1))	/* lonely - */
				mlwrite("Arg:");
			else
				mlwrite("Arg: %d", n * mflag);

			c = getkey();	/* get the next key */
		}
		n = n * mflag;	/* figure in the sign */
	}
	/* do ^U repeat argument processing */

	if (c == reptc) {	/* ^U, start argument   */
		f = TRUE;
		n = 4;		/* with argument of 4 */
		mflag = 0;	/* that can be discarded. */
		mlwrite("Arg: 4");
		while ((((c = getkey()) >= '0') && (c <= '9')) || (c == reptc) || (c == '-')) {
			if (c == reptc)
				if ((n > 0) == ((n * 2) > 0))
					n = n * 2;
				else
					n = 1;
			/*
			 * If dash, and start of argument string, set arg.
			 * to -1.  Otherwise, insert it.
			 */
			else
				if (c == '-') {
					if (mflag)
						break;
					n = 0;
					mflag = -1;
				}
			/*
			 * If first digit entered, replace previous argument
			 * with digit and set sign.  Otherwise, append to arg.
			 */
				else {
					if (!mflag) {
						n = 0;
						mflag = 1;
					}
					n = 10 * n + c - '0';
				}
			mlwrite("Arg: %d", (mflag >= 0) ? n : (n ? -n : -1));
		}
		/*
		 * Make arguments preceded by a minus sign negative and change
		 * the special argument "^U -" to an effective "^U -1".
		 */
		if (mflag == -1) {
			if (n == 0)
				n++;
			n = -n;
		}
	}
	/* and execute the command */
	execute(c, f, n);
	goto loop;
}

/*
 * Initialize all of the buffers and windows. The buffer name is passed down
 * as an argument, because the main routine may have been told to read in a
 * file by default, and we want the buffer name to be right.
 */

int
edinit(char *bname)	/* name of buffer to initialize */
{
	BUFFER *bp;
	WINDOW *wp;
	int cmark;		/* current mark */

	/* initialize some important globals */

	strcpy(aborthook, "");
	strcpy(readhook, "");
	strcpy(postreadhook, "");
	strcpy(wraphook, "");
	strcpy(cmdhook, "");
	strcpy(modifyhook, "");
	strcpy(writehook, "");
	strcpy(postwritehook, "");
	strcpy(bufhook, "");
	strcpy(exbhook, "");
	strcpy(newlinehook, "");

	bp = bfind(bname, TRUE, 0);	/* First buffer */
	wp = (WINDOW *)MALLOC(sizeof(WINDOW));	/* First window */
	if (bp == NULL || wp == NULL)
		meexit(1);
	curbp = bp;		/* Make this current	 */
	wheadp = wp;
	curwp = wp;
	wp->w_wndp = NULL;	/* Initialize window	 */
	wp->w_bufp = bp;
	bp->b_nwnd = 1;		/* Displayed.		 */
	wp->w_linep = bp->b_linep;
	wp->w_dotp = bp->b_linep;
	wp->w_doto = 0;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		wp->w_markp[cmark] = NULL;
		wp->w_marko[cmark] = 0;
	}
	wp->w_toprow = 0;
	wp->w_fcol = 0;
	wp->w_ntrows = term.t_nrow - 1;	/* "-1" for mode line.	 */
	wp->w_force = 0;
	wp->w_flag = WFMODE | WFHARD;	/* Full.		 */

	return TRUE;
}

/*
 * This is the general command execution routine. It handles the fake binding
 * of all the keys to "self-insert". It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it. Return the status of command.
 */
int
execute(int c, int f, int n)
{
	int status;
	KEYTAB *key;		/* key entry to execute */

	/* if the keystroke is a bound function...do it */
	key = getbind(c);
	if (key != NULL) {
		thisflag = 0;
		status = execkey(key, f, n);
		lastflag = thisflag;
		return status;
	}
	/*
	 * If a space was typed, fill column is defined, the argument is non-
	 * negative, wrap mode is enabled, and we are now past fill column,
	 * and we are not read-only, perform word wrap.
	 */
	if (c == ' ' && (curwp->w_bufp->b_mode & MDWRAP) && fillcol > 0 &&
	    n >= 0 && getccol(FALSE) > fillcol &&
	    (curwp->w_bufp->b_mode & MDVIEW) == FALSE)
		exechook(wraphook);

	if ((c >= 0x20 && c <= 0xFF)) {	/* Self inserting.	 */
		if (n <= 0) {	/* Fenceposts.		 */
			lastflag = 0;
			return n < 0 ? FALSE : TRUE;
		}
		thisflag = 0;	/* For the future.	 */

		/*
		 * if we are in overwrite mode, not at eol, and next char is
		 * not a tab or we are at a tab stop, delete a char forword
		 */
		if (curwp->w_bufp->b_mode & MDOVER &&
		    curwp->w_doto < curwp->w_dotp->l_used &&
		    (lgetc(curwp->w_dotp, curwp->w_doto) != '\t' ||
		     (curwp->w_doto) % curwp->w_bufp->b_tabs == (curwp->w_bufp->b_tabs - 1)))
			ldelete(1L, FALSE);

		if (kanji1st(inpkanji,c)) {
			unsigned char c2;
			unsigned short sjis;
			c2 = getkey();

			c  &= 0xff;
			c2 &= 0xff;

			switch (inpkanji) {
			case KANJI_JIS:
				/* XXX */
			case KANJI_EUC:
				sjis = euc2sjis(c*256+c2);
				break;
			case KANJI_SJIS:
				sjis = c*256+c2;
				break;
			}

			if (sjis>>8)
				linsert(n, (sjis >> 8) & 0xff);
			status = linsert(n, (sjis & 0xff));
		} else {
			/* do the appropriate insertion */
			if (c == '}' && (curbp->b_mode & MDCMOD) != 0)
				status = insbrace(n, c);
			else
				if (c == '#' && (curbp->b_mode & MDCMOD) != 0)
					status = inspound();
				else
					status = linsert(n, c);

#if CFENCE
			/* check for CMODE fence matching */
			if ((c == '}' || c == ')' || c == ']') &&
			    (curbp->b_mode & MDCMOD) != 0)
				fmatch(c);
#endif
		}

		/* check auto-save mode */
		if (curbp->b_mode & MDASAVE) {
			if (--gacount == 0) {
				/* and save the file if needed */
				upscreen(FALSE, 0);
				filesave(FALSE, 0);
				gacount = gasave;
			}
		}
		lastflag = thisflag;
		return status;
	}
	TTbeep();
	mlwrite(TEXT19);	/* complain		 */
	/* "[Key not bound]" */
	lastflag = 0;		/* Fake last flags.	 */
	return FALSE;
}

/*
 * Fancy quit command, as implemented by Norm.
 * If the any buffer has changed do a write on that buffer and exit emacs,
 * otherwise simply exit.
*/
int
quickexit(int f, int n)
{
	BUFFER *bp;		/* scanning pointer to buffers */
	BUFFER *oldcb;		/* original current buffer */
	int status;

	oldcb = curbp;		/* save in case we fail */

	bp = bheadp;
	while (bp != NULL) {
		if ((bp->b_flag & BFCHG) != 0	/* Changed.		 */
		    && (bp->b_flag & BFINVS) == 0) {	/* Real.		 */
			curbp = bp;	/* make that buffer cur */
			mlwrite(TEXT103, bp->b_fname);
			/* "[Saving %s]" */
			mlwrite("\n");
			if ((status = filesave(f, n)) != TRUE) {
				curbp = oldcb;	/* restore curbp */
				return status;
			}
		}
		bp = bp->b_bufp;/* on to the next buffer */
	}
	quit(f, n);		/* conditionally quit	 */
	return TRUE;
}

/*
 * Quit command. If an argument, always quit. Otherwise confirm if a buffer
 * has been changed and not written out. Normally bound to "C-X C-C".
 */
int
quit(int f, int n)
{
	int status;		/* return status */

	if (f != FALSE		/* Argument forces it.	 */
	    || anycb() == FALSE	/* All buffers clean or user says it's OK. */
	    || (status = mlyesno(TEXT104)) == TRUE) {
		/* "Modified buffers exist. Leave anyway" */
		if (f)
			status = meexit(n);
		else
			status = meexit(GOOD);
	}
	mlerase();
	return status;
}

int
meexit(int status)	/* return status of emacs */
{
	eexitflag = TRUE;	/* flag a program exit */
	eexitval = status;

	/* and now.. we leave and let the main loop kill us */
	return TRUE;
}

/*
 * Begin a keyboard macro.
 * Error if not at the top level in keyboard processing. Set up variables and
 * return.
 */
int
ctlxlp(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite(TEXT105);
		/* "%%Macro already active" */
		return FALSE;
	}
	mlwrite(TEXT106);
	/* "[Start macro]" */
	kbdptr = &kbdm[0];
	kbdend = kbdptr;
	kbdmode = RECORD;
	return TRUE;
}

/*
 * End keyboard macro. Check for the same limit conditions as the above
 * routine. Set up the variables and return to the caller.
 */

int
ctlxrp(int f, int n)
{
	if (kbdmode == STOP) {
		mlwrite(TEXT107);
		/* "%%Macro not active" */
		return FALSE;
	}
	if (kbdmode == RECORD) {
		mlwrite(TEXT108);
		/* "[End macro]" */
		kbdmode = STOP;
	}
	return TRUE;
}

/*
 * Execute a macro.
 * The command argument is the number of times to loop. Quit as soon as a
 * command gets an error. Return TRUE if all ok, else FALSE.
 */

int
ctlxe(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite(TEXT105);
		/* "%%Macro already active" */
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	kbdrep = n;		/* remember how many times to execute */
	kbdmode = PLAY;		/* start us in play mode */
	kbdptr = &kbdm[0];	/* at the beginning */
	return TRUE;
}

/*
 * Abort.
 * Beep the beeper. Kill off any keyboard macro, etc., that is in progress.
 * Sometimes called as a routine, to do general aborting of stuff.
 */
int
ctrlg(int f, int n)
{
	TTbeep();
	kbdmode = STOP;

	mlwrite(TEXT8);
	/* "[Aborted]" */

	exechook(aborthook);

	discmd = 1;
	return ABORT;
}

/*
 * tell the user that this command is illegal while we are in VIEW
 * (read-only) mode
 */

int
rdonly(void)
{
	TTbeep();
	mlwrite(TEXT109);
	/* "[Key illegal in VIEW mode]" */
	return FALSE;
}

int
resterr(void)
{
	TTbeep();
	mlwrite(TEXT110);
	/* "[That command is RESTRICTED]" */
	return FALSE;
}

/* user function that does NOTHING */
/* yes, these are default and never used..but MUST be here */
int
nullproc(int f, int n)
{
	return TRUE;
}

/* set META prefixing pending */
int
meta(int f, int n)
{
	if (kbdmode != PLAY)
		mlwrite("[META]");

	prefix |= META;
	prenum = n;
	predef = f;
	return TRUE;
}

/* set ^X prefixing pending */
int
cex(int f, int n)
{
	mlwrite("[CTRL-X]");

	prefix |= CTLX;
	prenum = n;
	predef = f;
	return TRUE;
}

/* set ^C prefixing pending */
int
cec(int f, int n)
{
	mlwrite("[CTRL-C]");

	prefix |= CTLC;
	prenum = n;
	predef = f;
	return TRUE;
}


/* dummy function for binding to universal-argument */
int
unarg(int f, int n)
{
	return TRUE;
}

/*
 * bytecopy - copy a string with length restrictions ALWAYS null terminate
 *
 *  dst: destination of copied string
 *  src; source
 *  maxlen: maximum length
 *
 */
char *
bytecopy(char *dst, char *src, int maxlen)

{
	char *dptr;		/* ptr into dst */

	dptr = dst;
	while (*src && (maxlen-- > 0))
		*dptr++ = *src++;
	*dptr = 0;
	return dst;
}

/*****		Compiler specific Library functions	****/

#if RAMSIZE
/*
 * These routines will allow me to track memory usage by placing a layer on
 * top of the standard system malloc() and free() calls. with this code
 * defined, the environment variable, $RAM, will report on the number of
 * bytes allocated via malloc.
 *
 * with SHOWRAM defined, the number is also posted on the end of the bottom mode
 * line and is updated whenever it is changed.
 */

#undef	malloc
#undef	free

static void update_mlform_envram(void);

struct mallocinfo {
	unsigned long size;
};

static void
update_mlform_envram(void)
{
	if (mlform_envram != envram / 1024) {
		mlform_envram = envram / 1024;
		upmode();
	}
}

/* allocate nbytes and track */
void *
allocate(unsigned long nbytes)	/* # of bytes to allocate */
{
	void *mp;		/* ptr returned from malloc */
	FILE *track;		/* malloc track file */

	(void)&track;

	nbytes += sizeof(struct mallocinfo);
	mp = malloc(nbytes);
	if (mp) {
		((struct mallocinfo *)mp)->size = nbytes;
	}

#if RAMTRCK
	track = fopen("malloc.dat", "a");
	fprintf(track, "Allocating %lu bytes at %p [%lu/%lu]\n", nbytes, mp, envram, envmalloc);
	fclose(track);
#endif

	if (mp) {
		envram += nbytes;
		envmalloc++;
		update_mlform_envram();

#if RAMSHOW
		dspram();
#endif
		return (char*)mp+sizeof(struct mallocinfo);
	} else {
		return 0;
	}
}

void *
reallocate(void *mp, unsigned long nbytes)
{
	struct mallocinfo *lp;	/* ptr of malloc information (in top of malloc area) */
	void *newmp;

	lp = (struct mallocinfo *)((char*)mp - sizeof(struct mallocinfo));

	newmp = allocate(nbytes);
	if (newmp != NULL) {
		memcpy(newmp, mp, lp->size);
	}
	release(mp);

	return newmp;
}

/* release malloced memory and track */
void
release(void *mp)	/* chunk of RAM to release */
{
	struct mallocinfo *lp;	/* ptr of malloc information (in top of malloc area) */

#if RAMTRCK
	FILE *track;		/* malloc track file */
#endif

	if (mp) {
		/* update amount of ram currently malloced */
		lp = (struct mallocinfo *)((char*)mp - sizeof(struct mallocinfo));

#if RAMTRCK
	track = fopen("malloc.dat", "a");
	fprintf(track, "Freeing %p [%lu:%lu]\n",mp,envram,envmalloc);
	fclose(track);
#endif

		envram -= lp->size;
		envmalloc--;

		update_mlform_envram();

		free(lp);
#if RAMSHOW
		dspram();
#endif
	}
}

#if RAMSHOW
/* display the amount of RAM currently malloced */
void
dspram(void)
{
	char mbuf[128];
	char *sp;

	TTmove(term.t_nrow, term.t_ncol-32);
	sprintf(mbuf, "[%lu/%lu]", envram, envmalloc);
	sp = &mbuf[0];
	while (*sp)
		TTputc(*sp++);
	TTmove(term.t_nrow, 0);
	movecursor(term.t_nrow, 0);
}
#endif
#endif
