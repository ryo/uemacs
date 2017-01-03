/*
 * $Id: exec.c,v 1.24 2017/01/02 15:17:50 ryo Exp $
 *
 * This file is for functions dealing with execution of commands, command
 * lines, buffers, files and startup files
 *
 * written 1986 by Daniel Lawrence
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"

/* namedcmd: execute a named command even if it is not bound */
int
namedcmd(int f, int n)
{
	int (*kfunc) (int,int);	/* ptr to the function to execute */
	char buffer[NSTRING];	/* buffer to store function name */
	int status;

	/* if we are non-interactive.... force the command interactivly */
	if (clexec == TRUE) {
		/* grab token and advance past */
		execstr = token(execstr, buffer, NPAT);

		/* evaluate it */
		strcpy(buffer, fixnull(getval(buffer)));
		if (strcmp(buffer, errorm) == 0)
			return FALSE;

		/* and look it up */
		if ((kfunc = (void*)fncmatch(buffer)) == NULL) {
			mlwrite(TEXT16);
			/* "[No such Function]" */
			return FALSE;
		}
		/* and execute it  INTERACTIVE */
		clexec = FALSE;
		status = (*kfunc) (f, n);	/* call the function */
		clexec = TRUE;
		return status;
	}
	/* prompt the user to type a named command */
	/* and get the function name to execute */
	{
		char *sp;
		sp = complete(": ",NULL, CMP_COMMAND, NSTRING);
		if (sp) {
			strcpy(buffer,sp);
			kfunc = (void*)fncmatch(sp);

			if (kfunc != NULL) {
				/* and then execute the command */
				return (*kfunc) (f, n);
			}
		}
	}

#if 1
	{
		char bufn[NBUFN + 2];	/* name of buffer to execute */
		BUFFER *bp;		/* buffer to execute */
		int oldcle;		/* old contents of clexec flag */
		int ostatus;		/* return status of function */

		(void)&ostatus;		/* stupid gcc */

		/* construct the buffer name */
		strcpy(bufn, "[");
		strcat(bufn, buffer);
		strcat(bufn, "]");

		/* find the pointer to that buffer */
		if ((bp = bfind(bufn, FALSE, 0)) == NULL) {
			mlwrite(TEXT16);
			/* "[No such Function]" */
			return FALSE;
		}
		if (bp->b_active == FALSE) {
			mlwrite(TEXT16);	/* XXX */
			return FALSE;
		}

		/* execute the buffer */
		oldcle = clexec;/* save old clexec flag */
		clexec = TRUE;	/* in cline execution */
		while (n-- > 0)
			if ((ostatus = dobuf(bp)) != TRUE)
				break;
		cmdstatus = ostatus;	/* save the status */
		clexec = oldcle;/* restore clexec flag */
		return ostatus;
	}
#else
	mlwrite(TEXT16);
	return FALSE;
#endif
}


/*
 * execcmd: Execute a command line command to be typed in by the user
 */
int
execcmd(int f, int n)
{
	char *scan;
	char tmpcmdstr[NSTRING];	/* string holding command to execute */

	scan = complete(": ", 0, CMP_COMMAND, NSTRING-1);
	if (scan == 0)
		return FALSE;

	strcpy(tmpcmdstr, scan);

	execlevel = 0;
	return docmd(tmpcmdstr);
}


int
exechook(char *cmdline)
{
	int status;

	execlevel = 0;
	status = *cmdline ? docmd(cmdline) : TRUE;

	return status;
}



/*
 * docmd: take a passed string as a command line and translate it to be
 * executed as a command. This function will be used by execute-command-line
 * and by all source and startup files. Lastflag/thisflag is also updated.
 *
 * format of the command line is:
 *
 * {# arg} <command-name> {<argument string(s)>}
 *
 */
int
docmd(char *cline)	/* command line to execute */
{
	int f;			/* default argument flag */
	int n;			/* numeric repeat value */
	int (*fnc) (int,int);		/* function to execute */
	BUFFER *bp;		/* buffer to execute */
	int status;		/* return status of function */
	int oldcle;		/* old contents of clexec flag */
	char *oldestr;		/* original exec string */
	char tkn[NSTRING];	/* next token off of command line */
	char bufn[NBUFN + 2];	/* name of buffer to execute */

	/* if we are scanning and not executing..go back here */
	if (execlevel)
		return TRUE;

	oldestr = execstr;	/* save last ptr to string to execute */
	execstr = cline;	/* and set this one as current */

	/* first set up the default command values */
	f = FALSE;
	n = 1;
	lastflag = thisflag;
	thisflag = 0;

	if ((status = macarg(tkn)) != TRUE) {	/* and grab the first token */
		execstr = oldestr;
		return status;
	}
	/* process leadin argument */
	if (gettyp(tkn) != TKCMD) {
		f = TRUE;
		strcpy(tkn, fixnull(getval(tkn)));
		n = asc_int(tkn);

		/* and now get the command to execute */
		if ((status = macarg(tkn)) != TRUE) {
			execstr = oldestr;
			return status;
		}
	}
	/* and match the token to see if it exists */
	if ((fnc = (void*)fncmatch(tkn)) == (void*)NULL) {

		/* construct the buffer name */
		strcpy(bufn, "[");
		strcat(bufn, tkn);
		strcat(bufn, "]");

		/* find the pointer to that buffer */
		if ((bp = bfind(bufn, FALSE, 0)) == NULL) {
			mlwrite(TEXT16);
			/* "[No such Function]" */
			execstr = oldestr;
			return FALSE;
		}
		/* execute the buffer */
		oldcle = clexec;/* save old clexec flag */
		clexec = TRUE;	/* in cline execution */
		while (n-- > 0)
			if ((status = dobuf(bp)) != TRUE)
				break;
		cmdstatus = status;	/* save the status */
		clexec = oldcle;/* restore clexec flag */
		execstr = oldestr;
		return status;
	}

	/* save the arguments and go execute the command */
	oldcle = clexec;	/* save old clexec flag */
	clexec = TRUE;		/* in cline execution */
	status = (*fnc) (f, n);	/* call the function */
	cmdstatus = status;	/* save the status */
	clexec = oldcle;	/* restore clexec flag */
	execstr = oldestr;
	return status;
}

/*
 * token -- chop a token off a string return a pointer past the token
 *
 *  src, tok: source string, destination token string
 *  size: maximum size of token
 */
char *
token(char *src, char *tok, int size)
{
	int quotef;		/* is the current string quoted? */
	char c;			/* temporary character */

	/* first scan past any whitespace in the source string */
	while (*src == ' ' || *src == '\t')
		++src;

	/* scan through the source string */
	quotef = FALSE;
	while (*src) {
		/* process special characters */
		if (*src == '~') {
			++src;
			if (*src == 0)
				break;
			switch (*src++) {
			case 'r':
				c = 13;
				break;
			case 'n':
				c = 13;
				break;
			case 'l':
				c = 10;
				break;
			case 't':
				c = 9;
				break;
			case 'b':
				c = 8;
				break;
			case 'f':
				c = 12;
				break;
			default:
				c = *(src - 1);
			}
			if (--size > 0) {
				*tok++ = c;
			}
		} else {
			/* check for the end of the token */
			if (quotef) {
				if (*src == '"')
					break;
			} else {
				if (*src == ' ' || *src == '\t')
					break;
			}

			/* set quote mode if quote found */
			if (*src == '"')
				quotef = TRUE;

			/* record the character */
			c = *src++;
			if (--size > 0)
				*tok++ = c;
		}
	}

	/* terminate the token and exit */
	if (*src)
		++src;
	*tok = 0;
	return src;
}

/* get a macro line argument */
int
macarg(char *tok)	/* buffer to place argument */
{
	int savcle;		/* buffer to store original clexec */
	int status;

	savcle = clexec;	/* save execution mode */
	clexec = TRUE;		/* get the argument */
	status = nextarg("", tok, NSTRING, ctoec('\r'));
	clexec = savcle;	/* restore execution mode */
	return status;
}

/*
 * nextarg -- get the next argument
 *
 *  prompt: prompt to use if we must be interactive
 *  buffer: buffer to put token into
 *  size: size of the buffer
 *  terminator: terminating char to be used on interactive fetch
 */
int
nextarg(char *prompt, char *buffer, int size, int terminator)
{
	char *sp;		/* return pointer from getval() */

	/* if we are interactive, go get it! */
	if (clexec == FALSE) {
		return getstring(prompt, buffer, size, terminator);
	}

	/* grab token and advance past */
	execstr = token(execstr, buffer, size);

	/* evaluate it */
	if ((sp = getval(buffer)) == NULL)
		return FALSE;
	strcpy(buffer, sp);
	return TRUE;
}

#if PROC
/*
 * storeproc: Set up a procedure buffer and flag to store all executed command lines there
 */
int
storeproc(int f, int n)
{
	struct BUFFER *bp;	/* pointer to macro buffer */
	int status;		/* return status */
	char bname[NBUFN];	/* name of buffer to use */

	/* get the name of the procedure */
	if ((status = mlreply(TEXT114, &bname[1], NBUFN - 2)) != TRUE)
		/* "Procedure name: " */
		return status;

	/* construct the macro buffer name */
	bname[0] = '[';
	strcat(bname, "]");

	/* set up the new macro buffer */
	if ((bp = bfind(bname, TRUE, BFINVS)) == NULL) {
		mlwrite(TEXT113);
		/* "Can not create macro" */
		return FALSE;
	}
	/* and make sure it is empty */
	bclear(bp);

	/* and set the macro store pointers to it */
	mstore = TRUE;
	bstore = bp;
	return TRUE;
}

/* execproc: Execute a procedure */
int
execproc(int f, int n)
{
	BUFFER *bp;		/* ptr to buffer to execute */
	int status;		/* status return */
	char bufn[NBUFN + 2];	/* name of buffer to execute */

	/* find out what buffer the user wants to execute */
	if ((status = mlreply(TEXT115, &bufn[1], NBUFN)) != TRUE)
		/* "Execute procedure: " */
		return status;

	/* construct the buffer name */
	bufn[0] = '[';
	strcat(bufn, "]");

	/* find the pointer to that buffer */
	if ((bp = bfind(bufn, FALSE, 0)) == NULL) {
		mlwrite(TEXT116);
		/* "No such procedure" */
		return FALSE;
	}
	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return status;
	return TRUE;
}
#endif

/* execbuf: Execute the contents of a buffer of commands */
int
execbuf(int f, int n)
{
	BUFFER *bp;		/* ptr to buffer to execute */
	int status;		/* status return */
	char bufn[NSTRING];	/* name of buffer to execute */

	/* find out what buffer the user wants to execute */
	if ((status = mlreply(TEXT117, bufn, NBUFN)) != TRUE)
		/* "Execute buffer: " */
		return status;

	/* find the pointer to that buffer */
	if ((bp = bfind(bufn, FALSE, 0)) == NULL) {
		mlwrite(TEXT118);
		/* "No such buffer" */
		return FALSE;
	}
	/* and now execute it as asked */
	while (n-- > 0)
		if ((status = dobuf(bp)) != TRUE)
			return status;
	return TRUE;
}

/*
 * dobuf: execute the contents of the buffer pointed to by the passed BP
 *
 * Directives start with a "!" and include:
 *
 * !endm		End a macro
 * !if (cond)		conditional execution
 * !else
 * !endif
 * !return		Return (terminating current macro)
 * !goto <label>	Jump to a label in the current macro
 * !force		Force macro to continue...even if command fails
 * !while (cond)	Execute a loop if the condition is true
 * !endwhile
 *
 * Line Labels begin with a "*" as the first nonblank char, like:
 * *LABEL01
 */
int
dobuf(BUFFER *bp)	/* buffer to execute */
{
	int status;		/* status return */
	LINE *lp;		/* pointer to line to execute */
	LINE *hlp;		/* pointer to line header */
	LINE *glp;		/* line to goto */
	LINE *mp;		/* Macro line storage temp */
	int dirnum;		/* directive index */
	int linlen;		/* length of line to execute */
	int i;			/* index */
	int force;		/* force TRUE result? */
	WINDOW *wp;		/* ptr to windows to scan */
	WHBLOCK *whlist;	/* ptr to !WHILE list */
	WHBLOCK *pscanner;	/* ptr during scan */
	WHBLOCK *whtemp;	/* temporary ptr to a WHBLOCK */
	char *einit;		/* initial value of eline */
	char *eline;		/* text of line to execute */
	char tkn[NSTRING];	/* buffer to evaluate an expresion in */
#if LOGFLG
	FILE *fp;		/* file handle for log file */
#endif

	/* clear IF level flags/while ptr */
	execlevel = 0;
	whlist = NULL;
	pscanner = NULL;

	/* scan the buffer to execute, building WHILE header blocks */
	hlp = bp->b_linep;
	lp = hlp->l_fp;
	while (lp != hlp) {
		/* scan the current line */
		eline = (char*)lp->l_text;
		i = lp->l_used;

		/* trim leading whitespace */
		while (i-- > 0 && (*eline == ' ' || *eline == '\t'))
			++eline;

		/* if theres nothing here, don't bother */
		if (i <= 0)
			goto nxtscan;

		/* if is a while directive, make a block... */
		if (eline[0] == '!' && eline[1] == 'w' && eline[2] == 'h') {
			whtemp = (WHBLOCK *)MALLOC(sizeof(WHBLOCK));
			if (whtemp == NULL) {
		noram:		mlwrite(TEXT119);
				/* "%%Out of memory during while scan" */
		failexit:	freewhile(pscanner);
				freewhile(whlist);
				return FALSE;
			}
			whtemp->w_begin = lp;
			whtemp->w_type = BTWHILE;
			whtemp->w_next = pscanner;
			pscanner = whtemp;
		}
		/* if is a BREAK directive, make a block... */
		if (eline[0] == '!' && eline[1] == 'b' && eline[2] == 'r') {
			if (pscanner == NULL) {
				mlwrite(TEXT120);
				/* "%%!BREAK outside of any !WHILE loop" */
				goto failexit;
			}
			whtemp = (WHBLOCK *)MALLOC(sizeof(WHBLOCK));
			if (whtemp == NULL)
				goto noram;
			whtemp->w_begin = lp;
			whtemp->w_type = BTBREAK;
			whtemp->w_next = pscanner;
			pscanner = whtemp;
		}
		/* if it is an endwhile directive, record the spot... */
		if (eline[0] == '!' && strncmp(&eline[1], "endw", 4) == 0) {
			if (pscanner == NULL) {
				mlwrite(TEXT121,
				/*
				 * "%%!ENDWHILE with no preceding !WHILE in
				 * '%s'"
				 */
					bp->b_bname);
				goto failexit;
			}
			/*
			 * move top records from the pscanner list to the
			 * whlist until we have moved all BREAK records and
			 * one WHILE record
			 */
			do {
				pscanner->w_end = lp;
				whtemp = whlist;
				whlist = pscanner;
				pscanner = pscanner->w_next;
				whlist->w_next = whtemp;
			} while (whlist->w_type == BTBREAK);
		}
nxtscan:			/* on to the next line */
		lp = lp->l_fp;
	}

	/* while and endwhile should match! */
	if (pscanner != NULL) {
		mlwrite(TEXT122,
		/* "%%!WHILE with no matching !ENDWHILE in '%s'" */
			bp->b_bname);
		goto failexit;
	}
	/* let the first command inherit the flags from the last one.. */
	thisflag = lastflag;

	/* starting at the beginning of the buffer */
	hlp = bp->b_linep;
	lp = hlp->l_fp;
	while (lp != hlp && eexitflag == FALSE) {
		/* allocate eline and copy macro line to it */
		linlen = lp->l_used;
		if ((einit = eline = (void*)MALLOC(linlen + 1)) == NULL) {
			mlwrite(TEXT123);
			/* "%%Out of Memory during macro execution" */
			freewhile(whlist);
			return FALSE;
		}
		bytecopy(eline, (char *)lp->l_text, linlen);
		eline[linlen] = 0;	/* make sure it ends */

		/* trim leading whitespace */
		while (*eline == ' ' || *eline == '\t')
			++eline;

		/* dump comments and blank lines */
		if (*eline == ';' || *eline == 0)
			goto onward;

#if LOGFLG
		/* append the current command to the log file */
		fp = fopen("emacs.log", "a");
		strcpy(outline, eline);
		fprintf(fp, "%s\n", outline);
		fclose(fp);
#endif

#if DEBUGM
		/* only do this if we are debugging */
		if (macbug && !mstore && (execlevel == 0))
			if (debug(bp, eline) == FALSE) {
				mlforce(TEXT54);
				/* "[Macro aborted]" */
				freewhile(whlist);
				return FALSE;
			}
#endif

		/* Parse directives here.... */
		dirnum = -1;
		if (*eline == '!') {
			/* Find out which directive this is */
			++eline;
			for (dirnum = 0; dirnum < NUMDIRS; dirnum++)
				if (strncmp(eline, dname[dirnum],
					    strlen(dname[dirnum])) == 0)
					break;

			/* and bitch if it's illegal */
			if (dirnum == NUMDIRS) {
				mlwrite(TEXT124);
				/* "%%Unknown Directive" */
				freewhile(whlist);
				return FALSE;
			}
			/* service only the !ENDM macro here */
			if (dirnum == DENDM) {
				mstore = FALSE;
				bstore = NULL;
				goto onward;
			}
			/* restore the original eline.... */
			--eline;
		}
		/* if macro store is on, just salt this away */
		if (mstore) {
			/* allocate the space for the line */
			linlen = strlen(eline);
			if ((mp = lalloc(linlen)) == NULL) {
				mlwrite(TEXT125);
				/* "Out of memory while storing macro" */
				return FALSE;
			}
			/* copy the text into the new line */
			for (i = 0; i < linlen; ++i)
				lputc(mp, i, eline[i]);

			/* attach the line to the end of the buffer */
			bstore->b_linep->l_bp->l_fp = mp;
			mp->l_bp = bstore->b_linep->l_bp;
			bstore->b_linep->l_bp = mp;
			mp->l_fp = bstore->b_linep;
			goto onward;
		}
		force = FALSE;

		/* dump comments */
		if (*eline == '*')
			goto onward;

		/* now, execute directives */
		if (dirnum != -1) {
			/* skip past the directive */
			while (*eline && *eline != ' ' && *eline != '\t')
				++eline;
			execstr = eline;

			switch (dirnum) {
			case DIF:	/* IF directive */
				/* grab the value of the logical exp */
				if (execlevel == 0) {
					if (macarg(tkn) != TRUE)
						goto eexec;
					if (stol(tkn) == FALSE)
						++execlevel;
				} else
					++execlevel;
				goto onward;

			case DWHILE:	/* WHILE directive */
				/* grab the value of the logical exp */
				if (execlevel == 0) {
					if (macarg(tkn) != TRUE)
						goto eexec;
					if (stol(tkn) == TRUE)
						goto onward;
				}
				/* drop down and act just like !BREAK */

			case DBREAK:	/* BREAK directive */
				if (dirnum == DBREAK && execlevel)
					goto onward;

				/* jump down to the endwhile */
				/* find the right while loop */
				whtemp = whlist;
				while (whtemp) {
					if (whtemp->w_begin == lp)
						break;
					whtemp = whtemp->w_next;
				}

				if (whtemp == NULL) {
					mlwrite(TEXT126);
					/* "%%Internal While loop error" */
					freewhile(whlist);
					return FALSE;
				}
				/* reset the line pointer back.. */
				lp = whtemp->w_end;
				goto onward;

			case DELSE:	/* ELSE directive */
				if (execlevel == 1)
					--execlevel;
				else
					if (execlevel == 0)
						++execlevel;
				goto onward;

			case DENDIF:	/* ENDIF directive */
				if (execlevel)
					--execlevel;
				goto onward;

			case DGOTO:	/* GOTO directive */
				/* .....only if we are currently executing */
				if (execlevel == 0) {

					/* grab label to jump to */
					eline = token(eline, golabel, NPAT);
					linlen = strlen(golabel);
					glp = hlp->l_fp;
					while (glp != hlp) {
						if (*glp->l_text == '*' &&
						    (strncmp((char *)&glp->l_text[1],
						    golabel, linlen) == 0)) {
							lp = glp;
							goto onward;
						}
						glp = glp->l_fp;
					}
					mlwrite(TEXT127);
					/* "%%No such label" */
					freewhile(whlist);
					return FALSE;
				}
				goto onward;

			case DRETURN:	/* RETURN directive */
				if (execlevel == 0)
					goto eexec;
				goto onward;

			case DENDWHILE:	/* ENDWHILE directive */
				if (execlevel) {
					--execlevel;
					goto onward;
				} else {
					/* find the right while loop */
					whtemp = whlist;
					while (whtemp) {
						if (whtemp->w_type == BTWHILE &&
						    whtemp->w_end == lp)
							break;
						whtemp = whtemp->w_next;
					}

					if (whtemp == NULL) {
						mlwrite(TEXT126);
						/*
						 * "%%Internal While loop
						 * error"
						 */
						freewhile(whlist);
						return FALSE;
					}
					/* reset the line pointer back.. */
					lp = whtemp->w_begin->l_bp;
					goto onward;
				}

			case DFORCE:	/* FORCE directive */
				force = TRUE;

			}
		}
		/* execute the statement */
		status = docmd(eline);
		if (force)	/* force the status */
			status = TRUE;

		/* check for a command error */
		if (status != TRUE) {
			/* look if buffer is showing */
			wp = wheadp;
			while (wp != NULL) {
				if (wp->w_bufp == bp) {
					/* and point it */
					wp->w_dotp = lp;
					wp->w_doto = 0;
					wp->w_flag |= WFHARD;
				}
				wp = wp->w_wndp;
			}
			/* in any case set the buffer . */
			bp->b_dotp = lp;
			bp->b_doto = 0;
			FREE(einit);
			execlevel = 0;
			freewhile(whlist);
			return status;
		}
onward:			/* on to the next line */
		FREE(einit);
		lp = lp->l_fp;
	}

eexec:				/* exit the current function */
	execlevel = 0;
	freewhile(whlist);
	return TRUE;
}

#if DEBUGM
/*
 * Interactive debugger
 *
 * if $debug == TRUE, The interactive debugger is invoked
 * commands are listed out with the ? key
 *
 *  bp: buffer to execute
 *  eline: text of line to debug
 */
int
debug(BUFFER *bp, char *eline)
{
	int oldcmd;		/* original command display flag */
	int oldinp;		/* original connamd input flag */
	int oldstatus;		/* status of last command */
	int c;			/* temp character */
	KEYTAB *key;		/* ptr to a key entry */
	static char track[NSTRING] = "";	/* expression to track value
						 * of */
	char temp[NSTRING];	/* command or expression */

dbuild:			/* Build the information line to be presented
				 * to the user */

	strcpy(outline, "<<<");

	/* display the tracked expression */
	if (track[0] != 0) {
		oldstatus = cmdstatus;
		docmd(track);
		cmdstatus = oldstatus;
		strcat(outline, "[=");
		strcat(outline, gtusr("track"));
		strcat(outline, "]");
	}
	/* debug macro name */
	strcat(outline, bp->b_bname);
	strcat(outline, ":");

	/* and lastly the line */
	strcat(outline, eline);
	strcat(outline, ">>>");

	/* expand the %'s so mlwrite() won't interpret them */
	makelit(outline);

	/* write out the debug line */
dinput:outline[term.t_ncol - 1] = 0;
	mlforce(outline);
	update(TRUE);

	/* and get the keystroke */
	c = getkey();

	/* META key turns off debugging */
	key = getbind(c);
	if (key && key->k_ptr.fp == meta)
		macbug = FALSE;

	else
		if (c == abortc) {
			return FALSE;

		} else
			switch (c) {

			case '?':	/* list commands */
				strcpy(outline, TEXT128);
				/*
				 * "(e)val exp, (c/x)ommand, (t)rack exp, (^G)abort,
				 * <SP>exec, <META> stop debug"
				 */
				goto dinput;

			case 'c':	/* execute statement */
				oldcmd = discmd;
				discmd = 1;
				oldinp = disinp;
				disinp = TRUE;
				execcmd(FALSE, 1);
				discmd = oldcmd;
				disinp = oldinp;
				goto dbuild;

			case 'x':	/* execute extended command */
				oldcmd = discmd;
				discmd = 1;
				oldinp = disinp;
				disinp = TRUE;
				oldstatus = cmdstatus;
				namedcmd(FALSE, 1);
				cmdstatus = oldstatus;
				discmd = oldcmd;
				disinp = oldinp;
				goto dbuild;

			case 'e':	/* evaluate expresion */
				strcpy(temp, "set %track ");
				oldcmd = discmd;
				discmd = 1;
				oldinp = disinp;
				disinp = TRUE;
				getstring("Exp: ", &temp[11], NSTRING, ctoec('\r'));
				discmd = oldcmd;
				disinp = oldinp;
				oldstatus = cmdstatus;
				docmd(temp);
				cmdstatus = oldstatus;
				strcpy(temp, " = [");
				strcat(temp, gtusr("track"));
				strcat(temp, "]");
				mlforce(temp);
				c = getkey();
				goto dinput;

			case 't':	/* track expresion */
				oldcmd = discmd;
				discmd = 1;
				oldinp = disinp;
				disinp = TRUE;
				getstring("Exp: ", temp, NSTRING, ctoec('\r'));
				discmd = oldcmd;
				disinp = oldinp;
				strcpy(track, "set %track ");
				strcat(track, temp);
				goto dbuild;

			case ' ':	/* execute a statement */
				break;

			default:	/* illegal command */
				TTbeep();
				goto dbuild;
			}
	return TRUE;
}
#endif

/* expand all "%" to "%%" */
int
makelit(char *s)	/* string to expand */
{
	char *sp;		/* temp for expanding string */
	char *ep;		/* ptr to end of string to expand */

	sp = s;
	while (*sp) {
		if (*sp++ == '%') {
			/* advance to the end */
			ep = --sp;
			while (*ep++);
			/* null terminate the string one out */
			*(ep + 1) = 0;
			/* copy backwards */
			while (ep-- > sp)
				*(ep + 1) = *ep;

			/* and advance sp past the new % */
			sp += 2;
		}
	}
	return TRUE;
}

/* free a list of while block pointers */int
freewhile(WHBLOCK *wp)	/* head of structure to free */
{
	if (wp != NULL) {
		freewhile(wp->w_next);
		FREE(wp);
	}
	return TRUE;
}

/* execute a series of commands in a file */
int
execfile(int f, int n)
{
	int status;		/* return status of name query */
	char fname[NSTRING];	/* name of file to execute */
	char *fspec;		/* full file spec */

	if ((status = mlreply(TEXT129, fname, NSTRING - 1)) != TRUE)
		/* "File to execute: " */
		return status;

	/* look up the path for the file */
	fspec = flook(fname, TRUE);

	/* if it isn't around */
	if (fspec == NULL) {
		/* complain if we are interactive */
		if (clexec == FALSE)
			mlwrite(TEXT214, fname);
		/* "%%No such file as %s" */
		return FALSE;
	}
	/* otherwise, execute it */
	while (n-- > 0)
		if ((status = dofile(fspec)) != TRUE)
			return status;

	return TRUE;
}

/*
 * dofile: yank a file into a buffer and execute it if there are no errors,
 *         delete the buffer on exit
 */
int
dofile(char *fname)	/* file name to execute */
{
	BUFFER *bp;		/* buffer to place file to exeute */
	BUFFER *cb;		/* temp to hold current buf while we read */
	int status;		/* results of various calls */
	char bname[NBUFN];	/* name of buffer */

	makename(bname, fname);	/* derive the name of the buffer */
	unqname(bname);		/* make sure we don't stomp things */
	if ((bp = bfind(bname, TRUE, 0)) == NULL)	/* get the needed buffer */
		return FALSE;

	bp->b_mode = MDVIEW;	/* mark the buffer as read only */
	cb = curbp;		/* save the old buffer */
	curbp = bp;		/* make this one current */
	/* and try to read in the file to execute */
	if ((status = readin(fname, FALSE)) != TRUE) {
		curbp = cb;	/* restore the current buffer */
		return status;
	}
	/* go execute it! */
	curbp = cb;		/* restore the current buffer */
	if ((status = dobuf(bp)) != TRUE)
		return status;

	/* if not displayed, remove the now unneeded buffer and exit */
	if (bp->b_nwnd == 0)
		zotbuf(bp);
	return TRUE;
}
