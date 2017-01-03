/*
 * $Id: bind.c,v 1.17 2017/01/02 15:17:50 ryo Exp $
 *
 * This file is for functions having to do with key bindings,
 * descriptions, help commands and startup file.
 *
 * written 11-feb-86 by Daniel Lawrence
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "epath.h"

/*
 * give me some help!!!!
 * bring up a fake buffer and read the help file into it
 * with view mode
 */
int
help(int f, int n)
{
	BUFFER *bp;		/* buffer pointer to help */
	char *fname;		/* file name of help file */

	(void)&fname;		/* stupid gcc... */

	/* first check if we are already here */
	bp = bfind("emacs.hlp", FALSE, BFINVS);

	if (bp == NULL) {
#if SHARED
		strcpy(tname, pathname[1]);
		fname = flook(tname, FALSE);
#else
		fname = flook(pathname[1], FALSE);
#endif
		if (fname == NULL) {
			mlwrite(TEXT12);
			/* "[Help file is not online]" */
			return FALSE;
		}
	}
	/* split the current window to make room for the help stuff */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	if (bp == NULL) {
		/* and read the stuff in */
		if (getfile(fname, FALSE) == FALSE)
			return FALSE;
	} else
		swbuffer(bp);

	/* make this window in VIEW mode, update all mode lines */
	curwp->w_bufp->b_mode |= MDVIEW;
	curwp->w_bufp->b_flag |= BFINVS;
	upmode();
	return TRUE;
}

/* describe the command for a certain key */
int
deskey(int f, int n)
{
	int c;			/* key to describe */
	char *ptr;		/* string pointer to scan output strings */
	char outseq[NSTRING];	/* output buffer for command sequence */

	/* prompt the user to type us a key to describe */
	mlwrite(TEXT13);
	/* ": describe-key " */

	/*
	 * get the command sequence to describe change it to something we can
	 * print as well
	 */
	cmdstr(c = getckey(FALSE), &outseq[0]);

	/* and dump it out */
	outstring(outseq);
	outstring(" ");

	/* find the right ->function */
	if ((ptr = getfname(getbind(c))) == NULL)
		ptr = "Not Bound";

	/* output the command sequence */
	outstring(ptr);

	return 0;
}

/*
 * bindtokey:	add a new key to the key binding table
 */
int
bindtokey(int f, int n)
{
	unsigned int c;		/* command key to bind */
	int (*kfunc) (int,int);	/* ptr to the requested function to bind to */
	KEYTAB *ktp;		/* pointer into the command table */
	int found;		/* matched command flag */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* prompt the user to type in a key to bind */
	/* get the function name to bind it to */
	kfunc = (void*)getname(TEXT15);
	/* ": bind-to-key " */
	if (kfunc == NULL) {
		mlwrite(TEXT16);
		/* "[No such function]" */
		return FALSE;
	}
	outstring(" ");
	TTflush();

	/* get the command sequence to bind */
	c = getckey((kfunc == (void*)meta) || (kfunc == (void*)cex) || (kfunc == (void*)cec) ||
		    (kfunc == (void*)unarg) || (kfunc == (void*)ctrlg));

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	outstring(outseq);

	/* if the function is a unique prefix key */
	if (kfunc == (void*)unarg || kfunc == (void*)ctrlg) {

		/* search for an existing binding for the prefix key */
		ktp = &keytab[0];
		while (ktp->k_ptr.fp != NULL) {
			if (ktp->k_ptr.fp == kfunc)
				unbindchar(ktp->k_code);
			++ktp;
		}

		/* reset the appropriate global prefix variable */
		if (kfunc == (void*)unarg)
			reptc = c;
		if (kfunc == (void*)ctrlg)
			abortc = c;
	}
	/* search the table to see if it exists */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_ptr.fp != NULL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (found) {		/* it exists, just change it then */
		ktp->k_ptr.fp = kfunc;
		ktp->k_type = BINDFNC;
	} else {		/* otherwise we need to add it to the end */
		/* if we run out of binding room, bitch */
		if (ktp >= &keytab[NBINDS]) {
			mlwrite(TEXT17);
			/* "Binding table FULL!" */
			return FALSE;
		}
		ktp->k_code = c;/* add keycode */
		ktp->k_ptr.fp = kfunc;	/* and the function pointer */
		ktp->k_type = BINDFNC;	/* and the binding type */
		++ktp;		/* and make sure the next is null */
		ktp->k_code = 0;
		ktp->k_type = BINDNUL;
		ktp->k_ptr.fp = NULL;
	}

	/*
	 * if we have rebound the meta key, make the search terminator follow
	 * it
	 */
	if (kfunc == meta)
		sterm = c;

	return TRUE;
}

/*
 * macrotokey: Bind a key to a macro in the key binding table
 */
int
macrotokey(int f, int n)
{
	unsigned int c;		/* command key to bind */
	BUFFER *kmacro;		/* ptr to buffer of macro to bind to key */
	KEYTAB *ktp;		/* pointer into the command table */
	int found;		/* matched command flag */
	int status;		/* error return */
	char outseq[80];	/* output buffer for keystroke sequence */
	char bufn[NBUFN];	/* buffer to hold macro name */

	/* get the buffer name to use */
	if ((status = mlreply(TEXT215, &bufn[1], NBUFN - 2)) != TRUE)
		/* ": macro-to-key " */
		return status;

	/* build the responce string for later */
	strcpy(outseq, TEXT215);
	/* ": macro-to-key " */
	strcat(outseq, &bufn[1]);

	/* translate it to a buffer pointer */
	bufn[0] = '[';
	strcat(bufn, "]");
	if ((kmacro = bfind(bufn, FALSE, 0)) == NULL) {
		mlwrite(TEXT130);
		/* "Macro not defined" */
		return FALSE;
	}
	strcat(outseq, " ");
	mlwrite(outseq);

	/* get the command sequence to bind */
	c = getckey(FALSE);

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	outstring(outseq);

	/* search the table to see if it exists */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_type != BINDNUL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	if (found) {		/* it exists, just change it then */
		ktp->k_ptr.buf = kmacro;
		ktp->k_type = BINDBUF;
	} else {		/* otherwise we need to add it to the end */
		/* if we run out of binding room, bitch */
		if (ktp >= &keytab[NBINDS]) {
			mlwrite(TEXT17);
			/* "Binding table FULL!" */
			return FALSE;
		}
		ktp->k_code = c;/* add keycode */
		ktp->k_ptr.buf = kmacro;	/* and the function pointer */
		ktp->k_type = BINDBUF;	/* and the binding type */
		++ktp;		/* and make sure the next is null */
		ktp->k_code = 0;
		ktp->k_type = BINDNUL;
		ktp->k_ptr.fp = NULL;
	}

	return TRUE;
}

/*
 * unbindkey: delete a key from the key binding table
 */
int
unbindkey(int f, int n)
{
	int c;			/* command key to unbind */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* prompt the user to type in a key to unbind */
	mlwrite(TEXT18);
	/* ": unbind-key " */

	/* get the command sequence to unbind */
	c = getckey(FALSE);	/* get a command sequence */

	/* change it to something we can print as well */
	cmdstr(c, &outseq[0]);

	/* and dump it out */
	outstring(outseq);

	/* if it isn't bound, bitch */
	if (unbindchar(c) == FALSE) {
		mlwrite(TEXT19);
		/* "[Key not bound]" */
		return FALSE;
	}
	return TRUE;
}

int
unbindchar(int c)	/* command key to unbind */
{
	KEYTAB *ktp;		/* pointer into the command table */
	KEYTAB *sktp;		/* saved pointer into the command table */
	int found;		/* matched command flag */

	/* search the table to see if the key exists */
	ktp = &keytab[0];
	found = FALSE;
	while (ktp->k_type != BINDNUL) {
		if (ktp->k_code == c) {
			found = TRUE;
			break;
		}
		++ktp;
	}

	/* if it isn't bound, bitch */
	if (!found)
		return FALSE;

	/* save the pointer and scan to the end of the table */
	sktp = ktp;
	while (ktp->k_ptr.fp != NULL)
		++ktp;
	--ktp;			/* backup to the last legit entry */

	/* copy the last entry to the current one */
	sktp->k_code = ktp->k_code;
	sktp->k_type = ktp->k_type;
	sktp->k_ptr.fp = ktp->k_ptr.fp;

	/* null out the last one */
	ktp->k_code = 0;
	ktp->k_type = BINDNUL;
	ktp->k_ptr.fp = NULL;
	return TRUE;
}

/*
 * Describe bindings:
 *   bring up a fake buffer and list the key bindings into it with view mode
 */
int
desbind(int f, int n)
#if APROP
{
	return buildlist(TRUE, "");
}

int
apro(int f, int n)
{
	/* Apropos (List functions that match a substring) */
	char mstring[NSTRING];	/* string to match cmd names to */
	int status;		/* status return */

	status = mlreply(TEXT20, mstring, NSTRING - 1);
	/* "Apropos string: " */
	if (status != TRUE)
		return status;

	return buildlist(FALSE, mstring);
}

/*
 * build a binding list (limited or full)
 *  type: true = full list, false = partial list
 *  mstring: match string if a partial list
 */
int
buildlist(int type, char *mstring)
#endif
{
	WINDOW *wp;		/* scanning pointer to windows */
	KEYTAB *ktp;		/* pointer into the command table */
	NBIND *nptr;		/* pointer into the name binding table */
	BUFFER *bp;		/* buffer to put binding list into */
	int cpos;		/* current position to use in outseq */
	int cmark;		/* current mark */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* split the current window to make room for the binding list */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and get a buffer for it */
	bp = bfind(TEXT21, TRUE, 0);
	/* "Binding list" */
	if (bp == NULL || bclear(bp) == FALSE) {
		mlwrite(TEXT22);
		/* "Can not display binding list" */
		return FALSE;
	}
	/* let us know this is in progress */
	mlwrite(TEXT23);
	/* "[Building binding list]" */

	/* disconect the current buffer */
	if (--curbp->b_nwnd == 0) {	/* Last use.            */
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		for (cmark = 0; cmark < NMARKS; cmark++) {
			curbp->b_markp[cmark] = curwp->w_markp[cmark];
			curbp->b_marko[cmark] = curwp->w_marko[cmark];
		}
		curbp->b_fcol = curwp->w_fcol;
	}
	/* connect the current window to this buffer */
	curbp = bp;		/* make this buffer current in current window */
	bp->b_mode = 0;		/* no modes active in binding list */
	bp->b_nwnd++;		/* mark us as more in use */
	wp = curwp;
	wp->w_bufp = bp;
	wp->w_linep = bp->b_linep;
	wp->w_flag = WFHARD | WFFORCE;
	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	for (cmark = 0; cmark < NMARKS; cmark++) {
		wp->w_markp[cmark] = NULL;
		wp->w_marko[cmark] = 0;
	}

	/* build the contents of this window, inserting it line by line */
	nptr = &names[0];
	while (nptr->n_func != NULL) {

		/* add in the command name */
		strcpy(outseq, nptr->n_name);
		cpos = strlen(outseq);

#if APROP
		/* if we are executing an apropos command..... */
		if (type == FALSE &&
		/* and current string doesn't include the search string */
		    strinc(outseq, mstring) == FALSE)
			goto fail;
#endif
		/* search down any keys bound to this */
		ktp = &keytab[0];
		while (ktp->k_type != BINDNUL) {
			if (ktp->k_type == BINDFNC &&
			    ktp->k_ptr.fp == nptr->n_func) {
				/* padd out some spaces */
				while (cpos < 25)
					outseq[cpos++] = ' ';

				/* add in the command sequence */
				cmdstr(ktp->k_code, &outseq[cpos]);
				strcat(outseq, "\r");

				/* and add it as a line into the buffer */
				if (linstr(outseq) != TRUE)
					return FALSE;

				cpos = 0;	/* and clear the line */
			}
			++ktp;
		}

		/* if no key was bound, we need to dump it anyway */
		if (cpos > 0) {
			outseq[cpos++] = '\r';
			outseq[cpos] = 0;
			if (linstr(outseq) != TRUE)
				return FALSE;
		}
fail:				/* and on to the next name */
		++nptr;
	}

	/* add a blank line between the key and macro lists */
	lnewline();

	/* scan all buffers looking for macroes and their bindings */
	bp = bheadp;
	while (bp) {

		/* is this buffer a macro? */
		if (bp->b_bname[0] != '[')
			goto bfail;

		/* add in the command name */
		strcpy(outseq, bp->b_bname);
		cpos = strlen(outseq);

#if APROP
		/* if we are executing an apropos command..... */
		if (type == FALSE &&
		/* and current string doesn't include the search string */
		    strinc(outseq, mstring) == FALSE)
			goto bfail;
#endif
		/* search down any keys bound to this macro */
		ktp = &keytab[0];
		while (ktp->k_ptr.fp != NULL) {
			if (ktp->k_type == BINDBUF &&
			    ktp->k_ptr.buf == bp) {
				/* padd out some spaces */
				while (cpos < 25)
					outseq[cpos++] = ' ';

				/* add in the command sequence */
				cmdstr(ktp->k_code, &outseq[cpos]);
				strcat(outseq, "\r");

				/* and add it as a line into the buffer */
				if (linstr(outseq) != TRUE)
					return FALSE;

				cpos = 0;	/* and clear the line */
			}
			++ktp;
		}

		/* if no key was bound, we need to dump it anyway */
		if (cpos > 0) {
			outseq[cpos++] = '\r';
			outseq[cpos] = 0;
			if (linstr(outseq) != TRUE)
				return FALSE;
		}
bfail:				/* and on to the next buffer */
		bp = bp->b_bufp;
	}

	curwp->w_bufp->b_mode |= MDVIEW;	/* put this buffer view mode */
	curbp->b_flag &= ~BFCHG;/* don't flag this as a change */
	wp->w_dotp = lforw(curbp->b_linep);	/* back to the beginning */
	wp->w_doto = 0;
	upmode();
	mlwrite("");		/* clear the mode line */
	return TRUE;
}

#if APROP

/*
 * does source include sub?
 *
 *  source: string to search in
 *  sub: substring to look for
 */
int
strinc(char *source, char *sub)
{
	char *sp;		/* ptr into source */
	char *nxtsp;		/* next ptr into source */
	char *tp;		/* ptr into substring */

	/* for each character in the source string */
	sp = source;
	while (*sp) {
		tp = sub;
		nxtsp = sp;

		/* is the substring here? */
		while (*tp) {
			if (*nxtsp++ != *tp)
				break;
			else
				tp++;
		}

		/* yes, return a success */
		if (*tp == 0)
			return TRUE;

		/* no, onward */
		sp++;
	}
	return FALSE;
}
#endif

/* get a command key sequence from the keyboard	 */

unsigned int
getckey(int mflag)	/* going for a meta sequence? */
{
	unsigned int c;		/* character fetched */
	char tok[NSTRING];	/* command incoming */

	/* check to see if we are executing a command line */
	if (clexec) {
		macarg(tok);	/* get the next token */
		return stock(tok);
	}
	/* or the normal way */
	if (mflag)
		c = getkey();
	else
		c = getcmd();

	return c;
}

/*
 * execute the startup file
 */
int
startup(char *sfname)	/* name of startup file (null if default) */
{
	char *fname;		/* resulting file name to execute */

	/* look up the startup file */
	if (*sfname != 0)
		fname = flook(sfname, TRUE);
	else
#if SHARED
	{
		strcpy(tname, pathname[0]);
		fname = flook(tname, TRUE);
	}
#else
		fname = flook(pathname[0], TRUE);
#endif

	/* if it isn't around, don't sweat it */
	if (fname == NULL)
		return TRUE;

	/* otherwise, execute the sucker */
	return dofile(fname);
}

/*
 * Look up the existance of a file along the normal or PATH environment
 * variable. Look first in the HOME directory if asked and possible
 *
 *  fname: base file name to search for
 *  hflag: Look in the HOME environment variable first?
 */
char *
flook(char *fname, int hflag)
{
	char *home;		/* path to home directory */
	char *path;		/* environmental PATH variable */
	char *sp;		/* pointer into path spec */
	int i;			/* index */
	static char fspec[NFILEN];	/* full path spec to search */

	/* if we have an absolute path.. check only there! */
	sp = fname;
	while (*sp) {
		if (*sp == ':' || *sp == '\\' || *sp == '/') {
			if (ffropen(fname) == FIOSUC) {
				ffclose();
				return fname;
			} else
				return NULL;
		}
		++sp;
	}

#if ENVFUNC
	if (hflag) {
		home = getenv("HOME");
		if (home != NULL) {
			/* build home dir file spec */
			strcpy(fspec, home);
			strcat(fspec, "/");
			strcat(fspec, fname);

			/* and try it out */
			if (ffropen(fspec) == FIOSUC) {
				ffclose();
				return fspec;
			}
		}
	}
#endif

	/* always try the current directory first */
	if (ffropen(fname) == FIOSUC) {
		ffclose();
		return fname;
	}
#if ENVFUNC
	/* get the PATH variable */
	path = getenv("PATH");
	if (path != NULL)
		while (*path) {

			/* build next possible file spec */
			sp = fspec;
			while (*path && (*path != PATHCHR))
				*sp++ = *path++;

			/* add a terminating dir separator if we need it */
			if (*(sp - 1) != DIRSEPCHAR)
				*sp++ = DIRSEPCHAR;
			*sp = 0;
			strcat(fspec, fname);

			/* and try it out */
			if (ffropen(fspec) == FIOSUC) {
				ffclose();
				return fspec;
			}
			if (*path == PATHCHR)
				++path;
		}
#endif

	/* look it up via the old table method */
	for (i = 2; i < NPNAMES; i++) {
		strcpy(fspec, pathname[i]);
		strcat(fspec, fname);

		/* and try it out */
		if (ffropen(fspec) == FIOSUC) {
			ffclose();
			return fspec;
		}
	}

	return (NULL);		/* no such luck */
}

/*
 * change a key command to a string we can print out
 *
 *  c: sequence to translate
 *  seq: destination string for sequence
 */
int
cmdstr(int c, char *seq)
{
	char *ptr;		/* pointer into current position in sequence */

	ptr = seq;

	/* apply ^X sequence if needed */
	if (c & CTLX) {
		*ptr++ = '^';
		*ptr++ = 'X';
	}
	/* apply ^C sequence if needed */
	if (c & CTLC) {
		*ptr++ = '^';
		*ptr++ = 'C';
	}
	/* apply ALT key sequence if needed */
	if (c & ALTD) {
		*ptr++ = 'A';
		*ptr++ = '-';
	}
	/* apply Shifted sequence if needed */
	if (c & SHFT) {
		*ptr++ = 'S';
		*ptr++ = '-';
	}
	/* apply MOUS sequence if needed */
	if (c & MOUS) {
		*ptr++ = 'M';
		*ptr++ = 'S';
	}
	/* apply meta sequence if needed */
	if (c & META) {
		*ptr++ = 'M';
		*ptr++ = '-';
	}
	/* apply SPEC sequence if needed */
	if (c & SPEC) {
		*ptr++ = 'F';
		*ptr++ = 'N';
	}
	/* apply control sequence if needed */
	if (c & CTRLBIT) {
		*ptr++ = '^';
	}
	c = c & 255;		/* strip the prefixes */

	/* and output the final sequence */

	*ptr++ = c;
	*ptr = 0;		/* terminate the string */

	return 0;
}

/*
 * This function looks a key binding up in the binding table
 */
KEYTAB *
getbind(int c)	/* key to find what is bound to it */
{
	KEYTAB *ktp;

	/* scan through the binding table, looking for the key's entry */
	ktp = &keytab[0];
	while (ktp->k_type != BINDNUL) {
		if (ktp->k_code == c)
			return ktp;
		++ktp;
	}

	/* no such binding */
	return (KEYTAB *) NULL;
}

/*
 * getfname:	This function takes a ptr to KEYTAB entry and gets the name
 * associated with it
 */
char *
getfname(KEYTAB *key)	/* key binding to return a name of */
{
	int (*func) (int,int);		/* ptr to the requested function */
	NBIND *nptr;		/* pointer into the name binding table */
	BUFFER *bp;		/* ptr to buffer to test */
	BUFFER *kbuf;		/* ptr to requested buffer */

	/* if this isn't a valid key, it has no name */
	if (key == NULL)
		return NULL;

	/* skim through the binding table, looking for a match */
	if (key->k_type == BINDFNC) {
		func = key->k_ptr.fp;
		nptr = &names[0];
		while (nptr->n_func != NULL) {
			if (nptr->n_func == func)
				return nptr->n_name;
			++nptr;
		}
		return NULL;
	}
	/* skim through the buffer list looking for a match */
	kbuf = key->k_ptr.buf;
	bp = bheadp;
	while (bp) {
		if (bp == kbuf)
			return bp->b_bname;
		bp = bp->b_bufp;
	}
	return NULL;
}

/*
 * fncmatch:	match fname to a function in the names table and return any
 * match or NULL if none
 */
void *
(*fncmatch(fname))(int,int)
char *fname;			/* name to attempt to match */
{
	int nval;		/* value of matched name */

	nval = binary(fname, namval, numfunc);
	if (nval == -1)
		return NULL;
	else
		return (void*)(names[nval].n_func);
}

char *
namval(int idx)	/* index of name to fetch out of the name table */
{
	return names[idx].n_name;
}


/*
 * stock() -- String key name TO Command Key
 *
 * A key binding consists of one or more prefix functions followed by a
 * keystroke.  Allowable prefixes must be in the following order:
 *
 * ^X preceeding control-X
 * A- simeltaneous ALT key (on PCs mainly)
 * S- shifted function key
 * MS mouse generated keystroke
 * M- Preceding META key
 * FN function key
 * ^ control key
 *
 * Meta and ^X prefix of lower case letters are converted to upper case.
 * Real control characters are automatically converted to the ^A form.
 */
unsigned int
stock(char *keyname)	/* name of key to translate to Command key form */
{
	unsigned int c;		/* key sequence to return */

	/* parse it up */
	c = 0;

	/* Do ^X prefix */
	if (*keyname == '^' && *(keyname + 1) == 'X') {
		if (*(keyname + 2) != 0) {	/* Key is not bare ^X */
			c |= CTLX;
			keyname += 2;
		}
	}
	/* Do ^C prefix */
	if (*keyname == '^' && *(keyname + 1) == 'C') {
		if (*(keyname + 2) != 0) {	/* Key is not bare ^C */
			c |= CTLC;
			keyname += 2;
		}
	}
	/* and the ALT key prefix */
	if (*keyname == 'A' && *(keyname + 1) == '-') {
		c |= ALTD;
		keyname += 2;
	}
	/* and the SHIFTED prefix */
	if (*keyname == 'S' && *(keyname + 1) == '-') {
		c |= SHFT;
		keyname += 2;
	}
	/* and the mouse (MOUS) prefix */
	if (*keyname == 'M' && *(keyname + 1) == 'S') {
		c |= MOUS;
		keyname += 2;
	}
	/* then the META prefix */
	if (*keyname == 'M' && *(keyname + 1) == '-') {
		c |= META;
		keyname += 2;
	}
	/* next the function prefix */
	if (*keyname == 'F' && *(keyname + 1) == 'N') {
		c |= SPEC;
		keyname += 2;
	}
	/* a control char?  (Always upper case) */
	if (*keyname == '^' && *(keyname + 1) != 0) {
		c |= CTRLBIT;
		++keyname;
		uppercase(keyname);
	}
	/* A literal control character? (Boo, hiss) */
	if (*keyname < 32) {
		c |= CTRLBIT;
		*keyname += '@';
	}
	/* make sure we are not lower case if used with ^X or M- */
	if (!(c & (MOUS | SPEC | ALTD | SHFT)))	/* If not a special key */
		if (c & (CTLC | CTLX | META))	/* If is a prefix */
			uppercase(keyname);	/* Then make sure it's upper
						 * case */

	/* the final sequence... */
	c |= *keyname;
	return c;
}

/* string key name to binding name.... */
char *
transbind(char *skey)
{
	char *bindname;

	bindname = getfname(getbind(stock(skey)));
	if (bindname == NULL)
		bindname = (char*)errorm;

	return bindname;
}

int
execkey(key, f, n)		/* execute a function bound to a key */
KEYTAB *key;			/* key to execute */
int f, n;			/* agruments to C function */
{
	int status;		/* error return */

	if (key->k_type == BINDFNC)
		return (*(key->k_ptr.fp))(f,n);

	if (key->k_type == BINDBUF) {
		while (n--) {
			status = dobuf(key->k_ptr.buf);
			if (status != TRUE)
				return status;
		}
	}
	return TRUE;
}

#if 0
/*
 * set a KEYTAB to the given name of the given type
 *
 *  key: ptr to key to set
 *  type: type of binding
 *  name: name of function or buffer
 */
int
setkey(KEYTAB *key, short type, char *name)
{
	key->k_type = type;
	if (type == BINDFNC)
		key->k_ptr.fp = (void*)fncmatch(name);
	else
		if (type == BINDBUF)
			 /* not quite yet... */ ;

	return 0;
}
#endif
