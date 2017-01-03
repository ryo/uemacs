/*
 * $Id: input.c,v 1.39 2017/01/02 15:17:50 ryo Exp $
 *
 * Input: Various input routines for MicroEMACS
 *
 * written by Daniel Lawrence 5/9/86
 */

/*
	Notes:

	MicroEMACS's kernel processes two distinct forms of
	characters.  One of these is a standard unsigned character
	which is used in the edited text.  The other form, called
	an EMACS Extended Character is a 2 byte value which contains
	both an ascii value, and flags for certain prefixes/events.

	Bit	Usage
	---	-----
	0 = 7	Standard 8 bit ascii character
	8	Control key flag
	9	META prefix flag
	10	^X prefix flag
	11	Function key flag
	12	Mouse prefix
	13	Shifted flag (not needed on alpha shifted characters)
	14	Alterate prefix (ALT key on PCs)

	The machine dependent driver is responsible for returning
	a byte stream from the various input devices with various
	prefixes/events embedded as escape codes.  Zero is used as the
	value indicating an escape sequence is next.  The format of
	an escape sequence is as follows:

	0		Escape indicator
	<prefix byte>	upper byte of extended character
	{<col><row>}	col, row position if the prefix byte
			indicated a mouse event
	<event code>	value of event

	Two successive zeroes are used to indicate an actual
	null being input.  These values are then interpreted by
	getkey() to construct the proper extended character
	sequences to pass to the MicroEMACS kernel.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "kanji.h"


static int comp_command(char *, int *);
static int comp_file(char *, int *);
static int comp_variable(char *, int *);
static int comp_mode(char *, int *);
static int comp_buffer(char *, int *);
static char *char2sym(char, char *);
static char *getfbuf(char *);
static char *getfmode(char *);
static void display_candidate(char *, char *(*)(char *), char *(*)(char *));
static void display_match(int, char *);

/*
 * Ask a yes or no question in the message line. Return either TRUE, FALSE, or
 * ABORT. The ABORT status is returned if the user bumps out of the question
 * with a ^G. Used any time a confirmation is required.
 */
int
mlyesno(char *prompt)
{
	int c;			/* input character */
	char buf[NPAT];		/* prompt to user */

	for (;;) {
		/* build and prompt the user */
		strcpy(buf, prompt);
		strcat(buf, TEXT162);
		/* " [y/n]? " */
		mlwrite(buf);

		/* get the response */
		c = getcmd();	/* getcmd() lets us check for anything that might */
		/* generate a 'y' or 'Y' in case use screws up */

		if (c == ectoc(abortc))	/* Bail out! */
			return ABORT;

		if ((c == 'n') || (c == 'N')
		  || (c & (SPEC | ALTD | CTRLBIT | META | CTLX | CTLC | MOUS)))
			return FALSE;	/* ONLY 'y' or 'Y' allowed!!! */

		if (c == 'y' || c == 'Y')
			return TRUE;

		return FALSE;
	}
}

/*
 * Write a prompt into the message line, then read back a response. Keep
 * track of the physical position of the cursor. If we are in a keyboard
 * macro throw the prompt away, and return the remembered response. This
 * lets macros run at full speed. The reply is always terminated by a carriage
 * return. Handle erase, kill, and abort keys.
 */
int
mlreply(char *prompt, char *buf, int nbuf)
{
	return nextarg(prompt, buf, nbuf, ctoec((int) '\r'));
}

int
mltreply(char *prompt, char *buf, int nbuf, int eolchar)
{
	return nextarg(prompt, buf, nbuf, eolchar);
}

/*
 * ectoc:	expanded character to character collapse the CTRLBIT and SPEC
 * flags back into an ascii code
 */
int
ectoc(int c)
{
	if (c & CTRLBIT)
		c = c & ~(CTRLBIT | 0x40);
	if (c & SPEC)
		c = c & 255;
	return c;
}

/*
 * ctoec:	character to extended character pull out the CTRLBIT and SPEC
 * prefixes (if possible)
 */
int
ctoec(int c)
{
	if (c >= 0x00 && c <= 0x1F)
		c = CTRLBIT | (c + '@');
	return c;
}

/*
 * get a command name from the command line. Command completion means that
 * pressing a <SPACE> will attempt to complete an unfinished command name if
 * it is unique.
 */
void *
(*getname(prompt))(int,int)
char *prompt;			/* string to prompt with */
{
	char *sp;		/* ptr to the returned string */

	sp = complete(prompt, NULL, CMP_COMMAND, NSTRING);
	if (sp == (char *)NULL)
		return NULL;

	return fncmatch(sp);
}

int
getvar(char *prompt, char *buf, int nbuf)
{
	char *sp;
	sp = complete(prompt, 0, CMP_VARIABLE, NPAT - 1);
	if (sp == NULL)
		return FALSE;

	strncpy(buf,sp,nbuf);
	return TRUE;
}

/*
 * getcbuf: get a completion from the user for a buffer name.
 *
 * I was goaded into this by lots of other people's completion code.
 *
 *  prompt: prompt to user on command line
 *  defval: default value to display to user
 *  createflag: should this create a new buffer?
 */
BUFFER *
getcbuf(char *prompt, char *defval, int createflag)

{
	char *sp;		/* ptr to the returned string */

	sp = complete(prompt, defval, CMP_BUFFER, NBUFN);
	if (sp == NULL)
		return NULL;

	return bfind(sp, createflag, 0);
}

char *
gtfilename(char *prompt)	/* prompt to user on command line */
{
	char *sp;		/* ptr to the returned string */

	sp = complete(prompt, NULL, CMP_FILENAME, NFILEN);
	if (sp == NULL)
		return NULL;

	if (sp[0] == '~' && sp[1] == '/') {
		char tmp[NFILEN];
		extern char home[NFILEN];
		strcpy(tmp, home);
		strcat(tmp, sp + 1);
		strcpy(sp, tmp);
	}
	return sp;
}


/*
 * complete
 *
 *  prompt: prompt to user on command line
 *  defval: default value to display to user
 *  type: type of what we are completing
 *  maxlen: maximum length of input field
 */
char *
complete(char *prompt,char *defval, int type, int maxlen)
{
	int c;			/* current input character */
	int cpos;		/* current column on screen output */

#define	NSTATICBUF	4
	static int nstaticbuf = 0;
	static char staticbuf[NSTRING * NSTATICBUF];	/* buffer to hold tentative name */
	char *buf;

	buf = &staticbuf[NSTRING * nstaticbuf];
	nstaticbuf++;
	if (nstaticbuf >= 4)
		nstaticbuf=0;

#if COMPLET == 0
	int status;
#endif

	/* if we are executing a command line get the next arg and match it */
	if (clexec) {
		if (macarg(buf) != TRUE)
			return NULL;
		return buf;
	}
#if COMPLET == 0
	strcpy(buf, prompt);
	if (type != CMP_COMMAND) {
		if (defval) {
			strcat(buf, "[");
			strcat(buf, defval);
			strcat(buf, "]: ");
		} else {
			strcat(buf, ": ");
		}
	}
	status = mlreply(buf, buf, maxlen);
	if (status == ABORT)
		return NULL;
	if (defval && (buf[0] == 0))
		return defval;
	return buf;
}
#else
	/* starting at the beginning of the string buffer */
	cpos = 0;

	/* if it exists, prompt the user for a buffer name */
	if (prompt) {
		if (type == CMP_COMMAND) {
			mlwrite("%s", prompt);
		} else if (defval) {
			mlwrite("%s[%s]: ", prompt, defval);
		} else {
			mlwrite("%s: ", prompt);
		}
	}

	/* build a name string from the keyboard */
	while (TRUE) {
		c = tgetc();
		buf[cpos] = '\0';

		/* if we are at the end, just match it */
		if (c == '\n' || c == '\r') {
			if (defval && cpos == 0)
				return defval;
			else {
				return buf;
			}

		} else if (c == ectoc(abortc)) {	/* Bell, abort */
			ctrlg(FALSE, 0);
			TTflush();
			return NULL;

		} else if (c == 0x7F || c == 0x08 || c == 0x02) {	/* rubout/erase */
			if (cpos != 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--ttcol;
				--cpos;
				TTflush();
			}
		} else if (c == 0x15) {	/* C-U, kill */
			while (cpos != 0) {
				mlout('\b');
				mlout(' ');
				mlout('\b');
				--cpos;
				--ttcol;
			}
			TTflush();

		} else if ((c == ' ') || (c == ectoc(sterm)) || (c == '\t')) {
			/*
			 * attempt a completion
			 */
			switch (type) {
			case CMP_BUFFER:
				comp_buffer(buf, &cpos);
				break;
			case CMP_COMMAND:
				comp_command(buf, &cpos);
				break;
			case CMP_FILENAME:
				comp_file(buf, &cpos);
				break;
			case CMP_VARIABLE:
				comp_variable(buf, &cpos);
				break;
			case CMP_MODE:
				comp_mode(buf, &cpos);
				break;
			}

			TTflush();
			if (cpos != 0 && buf[cpos - 1] == 0) {
				return buf;
			}
		} else {
			if (cpos < maxlen && c > ' ') {
				buf[cpos++] = c;
				mlout(c);
				++ttcol;
				TTflush();
			}
		}
	}
}







/* display matchfiles */
#define	DISPMATCH_START	0
#define	DISPMATCH_PRINT	1
#define	DISPMATCH_END	2
static void
display_match(int code, char *name)
{
	int savecol;
	int x, y, w, h;
	int nfield;
	static int n;

	h = term.t_nrow - 1;
	w = term.t_ncol / 2;
	nfield = h * 2;

	switch (code) {
	case DISPMATCH_START:
		n = 0;
		break;
	case DISPMATCH_END:
		while (n < nfield)
			display_match(DISPMATCH_PRINT, "");
		upwind();
		break;
	case DISPMATCH_PRINT:
		savecol = ttcol;
		x = 0;
		y = n;
		if (y >= h) {
			if (x != 0)
				return;
			y -= h;
			x = w;
		}
		if (n >= nfield)
			return;
		n++;

		vtmove(y,x);
		while (*name && (w>1)) {
			vtputc(*name, 0, 0, CT_ANK);
			name++;
			w--;
		}
		if (w)
			while (w--)
				vtputc(' ', 0, 0, CT_ANK);

		updupd(TRUE);

		TTmove(term.t_nrow,savecol);
		ttcol = savecol;
		break;
	default:
		break;
	}
}

static void
display_candidate(char *name, char *(*getfirst)(char *), char *(*getnext)(char *))
{
	char *xname;

	display_match(DISPMATCH_START, 0);
	xname = (*getfirst)(name);
	while (xname) {
		int len = strlen(name);
		if (strncmp(xname, name, len) == 0)
			display_match(DISPMATCH_PRINT, xname);
		xname = (*getnext)(name);
	}
	display_match(DISPMATCH_END, 0);
}

static int getfcomp_curbind;
static BUFFER *getfcomp_bp;

static char *
getncomp(char *name)
{
	NBIND *nbp;
	static char bufname[NBUFN];

	if (getfcomp_curbind <= numfunc) {
		nbp = &names[getfcomp_curbind];
		getfcomp_curbind++;
		return nbp->n_name;
	}

	while (getfcomp_bp) {
		char *p = getfcomp_bp->b_bname;
		int len = strlen(p);

		if (*p == '[') {
			strncpy(bufname, p + 1, len - 2);
			bufname[len - 2] = '\0';
			getfcomp_bp = getfcomp_bp->b_bufp;
			return bufname;
		} else {
			getfcomp_bp = getfcomp_bp->b_bufp;
		}
	}
	return 0;
}

static char *
getfcomp(char *name)
{
	getfcomp_curbind = 0;
	getfcomp_bp = bheadp;
	return getncomp(name);
}

/*
 * comp_command -- Attempt a completion on a command name
 *
 * name: command containing the current name to complete
 * cpos: ptr to position of next character to insert
 */
static int
comp_command(char *name, int *cpos)
{
	NBIND *nbp;		/* trial command to complete */
	BUFFER *bp;		/* trial buffer to complete */
	int idx;		/* index into strings to compare */
	int curbind;		/* index into the names[] array */
	NBIND *match;		/* last command that matches string */
	int matchflag;		/* did this command name match? */
	int comflag;		/* was there a completion at all? */

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	curbind = 0;
	while (*cpos < NSTRING) {

		/* first, we start at the first command and scan the list */
		match = NULL;

		curbind = 0;
		while (curbind <= numfunc) {
			/* is this a match? */
			nbp = &names[curbind];
			matchflag = TRUE;
			for (idx = 0; idx < *cpos; idx++)
				if (name[idx] != nbp->n_name[idx]) {
					matchflag = FALSE;
					break;
				}
			/* if it is a match */
			if (matchflag) {
				/*
				 * if this is the first match, simply record
				 * it
				 */
				if (match == NULL) {
					match = nbp;
					name[*cpos] = nbp->n_name[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != nbp->n_name[*cpos]) {
						name[*cpos] = '\0';

						display_candidate(name, getfcomp, getncomp);
						return TRUE;
					}
				}
			}
			/* on to the next command */
			curbind++;
		}

		/*
		 * search procedure buffer named
		 * "[user-defined-procedure-name]"
		 */
		bp = bheadp;
		while (bp) {
			if (bp->b_bname[0] == '[') {
				/* is this a match? */
				matchflag = TRUE;
				for (idx = 0; idx < *cpos; idx++) {
					if (name[idx] != bp->b_bname[idx+1]) {
						matchflag = FALSE;
						break;
					}
				}

				/* if it is a match */
				if (matchflag) {
					/*
					 * if this is the first match, simply record
					 * it
					 */
					if (match == NULL) {
						match = (void*)bp;
						name[*cpos] = bp->b_bname[*cpos+1];
					} else {
						/* if there's a difference, stop here */
						if (name[*cpos] != bp->b_bname[*cpos+1]) {
							name[*cpos] = '\0';
							display_candidate(name, getfcomp, getncomp);
							return TRUE;
						}
					}
				}
			}
			/* on to the next buffer */
			bp = bp->b_bufp;
		}

		/* with no match, we are done */
		if (match == NULL) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();

			display_candidate(name, getfcomp, getncomp);
			return TRUE;
		}

		/* if we have completed all the way... go back */
		if ((name[*cpos] == 0) || (name[*cpos] == ']')) {
			name[*cpos] = 0;
			(*cpos)++;
			return TRUE;
		}
		/* remember we matched, and complete one character */
		comflag = TRUE;

		mlout(name[(*cpos)++]);
		ttcol++;
		TTflush();
	}

	/*
	 * don't allow a completion past the end of the max command name
	 * length
	 */
	return TRUE;
}


static int getfmode_md;

static char *
getnmode(char *name)
{
	char *p;

	if (getfmode_md < NUMMODES) {
		p = (char*)(modename[getfmode_md]);
		getfmode_md++;
		return p;
	}
	return 0;
}

static char *
getfmode(char *name)
{
	getfmode_md = 0;
	return getnmode(name);
}


/*
 * comp_mode
 *
 * name: command containing the current name to complete
 * cpos: ptr to position of next character to insert
 */
static int
comp_mode(char *name, int *cpos)
{
	int md;			/* trial mode to complete */
	int idx;		/* index into strings to compare */
	int match;		/* last mode that matches string */
	int matchflag;		/* did this buffer name match? */
	int comflag;		/* was there a completion at all? */

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	while (*cpos < NPAT) {
		/* first, we start at the first buffer and scan the list */
		match = -1;
		for (md = 0; md < NUMMODES; md++) {
			/* is this a match? */
			matchflag = TRUE;
			for (idx = 0; idx < *cpos; idx++)
				if (toupper((int)name[idx]) != toupper((int)modename[md][idx])) {
					matchflag = FALSE;
					break;
				}

			/* if it is a match */
			if (matchflag) {
				/*
				 * if this is the first match, simply record it
				 */
				if (match == -1) {
					match = md;
					name[*cpos] = modename[md][*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != modename[md][*cpos]) {
						name[*cpos] = '\0';
						display_candidate(name, getfmode, getnmode);
						return TRUE;
					}
				}
			}
		}
		/* with no match, we are done */
		if (match == -1) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();
			display_candidate(name, getfmode, getnmode);
			return TRUE;
		}
		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			(*cpos)++;
			return TRUE;
		}
		/* remember we matched, and complete one character */
		comflag = TRUE;
		mlout((name[(*cpos)++]));
		ttcol++;
		TTflush();
	}
	return TRUE;
}




static int getfvar_index;

static char *
getnvar(char *name)
{
	static char varname[NSTRING];

	if (getfvar_index < envnum()) {
		varname[0] = '$';
		strcpy(varname + 1, envval(getfvar_index));
		getfvar_index++;
		return varname;
	}
	return 0;
}

static char *
getfvar(char *name)
{
	getfvar_index = 0;
	return getnvar(name);
}


/*
 * comp_variable
 *
 * name: command containing the current name to complete
 * cpos: ptr to position of next character to insert
 */
static int
comp_variable(char *name, int *cpos)
{
	int i;			/* trial index to complete */
	int idx;		/* index into strings to compare */
	int match;		/* last mode that matches string */
	int matchflag;		/* did this buffer name match? */
	int comflag;		/* was there a completion at all? */
	char tmpvar[NSTRING];

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	while (*cpos < NPAT) {
		/* first, we start at the first buffer and scan the list */
		match = -1;
		for (i = 0; i < envnum(); i++) {
			strcpy(tmpvar, "$");
			strcat(tmpvar, envval(i));
			/* is this a match? */
			matchflag = TRUE;
			for (idx = 0; idx < *cpos; idx++)
				if ((name[idx]) != tmpvar[idx]) {
					matchflag = FALSE;
					break;
				}

			/* if it is a match */
			if (matchflag) {
				/*
				 * if this is the first match, simply record it
				 */
				if (match == -1) {
					match = i;
					name[*cpos] = tmpvar[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != tmpvar[*cpos]) {
						name[*cpos] = '\0';
						display_candidate(name, getfvar, getnvar);
						return TRUE;
					}
				}
			}
		}
		/* with no match, we are done */
		if (match == -1) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();

			display_candidate(name, getfvar, getnvar);
			return TRUE;
		}
		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			(*cpos)++;
			return TRUE;
		}
		/* remember we matched, and complete one character */
		comflag = TRUE;
		mlout((name[(*cpos)++]));
		ttcol++;
		TTflush();
	}
	return TRUE;
}



static BUFFER *getfbuf_bp;

static char *
getnbuf(char *name)
{
	static char bufname[NBUFN];

	while (getfbuf_bp) {
		char *p = getfbuf_bp->b_bname;

		getfbuf_bp = getfbuf_bp->b_bufp;
		if (*p == '[' && *name != '[')
			continue;

		strcpy(bufname, p);
		return bufname;
	}
	return 0;
}

static char *
getfbuf(char *name)
{
	getfbuf_bp = bheadp;
	return getnbuf(name);
}


/*
 * comp_buffer -- Attempt a completion on a buffer name
 *
 *  name: buffer containing the current name to complete
 *  cpos: ptr to position of next character to insert
 */
static int
comp_buffer(char *name, int *cpos)
{
	BUFFER *bp;		/* trial buffer to complete */
	int idx;		/* index into strings to compare */
	BUFFER *match;		/* last buffer that matches string */
	int matchflag;		/* did this buffer name match? */
	int comflag;		/* was there a completion at all? */

	/* start attempting completions, one character at a time */
	comflag = FALSE;
	while (*cpos < NBUFN) {

		/* first, we start at the first buffer and scan the list */
		match = NULL;

		bp = bheadp;
		while (bp) {
			/* is this a match? */
			matchflag = TRUE;
			for (idx = 0; idx < *cpos; idx++)
				if (name[idx] != bp->b_bname[idx]) {
					matchflag = FALSE;
					break;
				}

			/* if it is a match */
			if (matchflag) {
				/*
				 * if this is the first match, simply record it
				 */
				if (match == NULL) {
					match = bp;
					name[*cpos] = bp->b_bname[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != bp->b_bname[*cpos]) {
						name[*cpos] = '\0';
						display_candidate(name, getfbuf, getnbuf);
						return TRUE;
					}
				}
			}
			/* on to the next buffer */
			bp = bp->b_bufp;
		}

		/* with no match, we are done */
		if (match == NULL) {
			/* beep if we never matched */
			if (comflag == FALSE)
				TTbeep();

			display_candidate(name, getfbuf, getnbuf);
			return TRUE;
		}
		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			(*cpos)++;
			return TRUE;
		}
		/* remember we matched, and complete one character */
		comflag = TRUE;
		mlout(name[(*cpos)++]);
		ttcol++;
		TTflush();
	}

	/*
	 * don't allow a completion past the end of the max buffer name
	 * length
	 */
	return TRUE;
}




/*
 * comp_file -- Attempt a completion on a file name
 *
 *  name: file containing the current name to complete
 *  cpos: ptr to position of next character to insert
 */
static int
comp_file(char *name, int *cpos)
{
	char *fname;		/* trial file to complete */
	int idx;		/* index into strings to compare */
	char *match;		/* last file that matches string */
	int matchflag;		/* did this file name match? */

	/* start attempting completions, one character at a time */
	while (*cpos < NBUFN) {
		/* first, we start at the first file and scan the list */
		match = NULL;

		name[*cpos] = 0;
		fname = getffile(name);

		while (fname) {
			/* is this a match? */
			matchflag = TRUE;

			for (idx = 0; idx < *cpos; idx++) {
				if (name[idx] != fname[idx]) {
					matchflag = FALSE;
					break;
				}
			}

			/* if it is a match */
			if (matchflag && idx) {
				/*
				 * if this is the first match, simply record it
				 */
				if (match == NULL) {
					match = fname;
					name[*cpos] = fname[*cpos];
				} else {
					/* if there's a difference, stop here */
					if (name[*cpos] != fname[*cpos]) {
						name[*cpos] = '\0';

						display_candidate(name, getffile, getnfile);
						return TRUE;
					}
				}
			}
			/* on to the next file */
			fname = getnfile(NULL);
		}

		/* with no match, we are done */
		if (match == NULL) {
			display_candidate(name, getffile, getnfile);
			return TRUE;
		}
		/* if we have completed all the way... go back */
		if (name[*cpos] == 0) {
			if (isdir(name)) {
				strcat(name, "/");
				(*cpos)++;
				mlout('/');
				ttcol++;
				TTflush();
			} else {
				(*cpos)++;
				return TRUE;
			}
		} else {
			/* remember we matched, and complete one character */
			mlout(name[(*cpos)++]);
			ttcol++;
			TTflush();
		}
	}

	/* don't allow a completion past the end of the max file name length */
	return TRUE;
}
#endif


/*
 * tgetc: Get a key from the terminal driver, resolve any keyboard macro action
 */
int
tgetc(void)
{
	int c;			/* fetched character */

	/* if we are playing a keyboard macro back, */
	if (kbdmode == PLAY) {

		/* if there is some left... */
		if (kbdptr < kbdend)
			return (int)*kbdptr++;

		/* at the end of last repitition? */
		if (--kbdrep < 1) {
			kbdmode = STOP;
#if VISMAC == 0
			/* force a screen update after all is done */
			update(FALSE);
#endif
		} else {

			/* reset the macro to the begining for the next rep */
			kbdptr = &kbdm[0];
			return (int)*kbdptr++;
		}
	}
	/* fetch a character from the terminal driver */
	c = TTgetc();

	;			/* record it for $lastkey */
	lastkey = c;

	/* save it if we need to */
	if (kbdmode == RECORD) {
		*kbdptr++ = c;
		kbdend = kbdptr;

		/* don't overrun the buffer */
		if (kbdptr == &kbdm[NKBDM - 1]) {
			kbdmode = STOP;
			TTbeep();
		}
	}
	/* and finally give the char back */
	return c;
}

/*
 * getkey: Get one keystroke. The only prefixs legal here are the SPEC and CTRLBIT prefixes.
 */
int
getkey(void)
{
	int c;			/* next input character */

	/* get a keystroke */
	c = tgetc();
	/* here return 0x00 */

	/* yank out the control prefix */
	if ((c & 255) >= 0x00 && (c & 255) <= 0x1F)
		c = CTRLBIT | (c + '@');

	/* return the character */
	return c;
}

/*
 * GETCMD: Get a command from the keyboard. Process all applicable prefix keys
 */
int
getcmd(void)
{
	int c;			/* fetched keystroke */
	KEYTAB *key;		/* ptr to a key entry */

	/* get initial character */
	c = getkey();
	key = getbind(c);

	/* resolve META ,CTLX and CTLC prefixes */
	if (key) {
		if (key->k_ptr.fp == meta) {
			c = getkey();
			c = upperc(c) | (c & ~255);	/* Force to upper */
			c |= META;
		} else if (key->k_ptr.fp == cex) {
			c = getkey();
			c = upperc(c) | (c & ~255);	/* Force to upper */
			c |= CTLX;
		} else if (key->k_ptr.fp == cec) {
			c = getkey();
			c = upperc(c) | (c & ~255);	/* Force to upper */
			c |= CTLC;
		}
	}
	/* return it */
	return c;
}

char *
char2sym(char c, char *buf)
{
	switch(c) {
	case '\r':
		strcpy(buf, "<NL>");
		return buf;
	case '\n':
		strcpy(buf, "<LF>");
		return buf;
	case '\e':
		strcpy(buf, "<META>");
		return buf;
	case '\t':
		strcpy(buf, "<TAB>");
		return buf;
	}

	if (c < ' ') {
		buf[0] = '^';
		buf[1] = c ^ 0x40;
		buf[2] = '\0';
	} else {
		buf[0] = c;
		buf[1] = '\0';
	}
	return buf;
}




/*
 * A more generalized prompt/reply function allowing the caller to specify
 * the proper terminator. If the terminator is not a return ('\r'), return
 * will echo as "<NL>"
 */
int
getstring(char *prompt, char *buf, int nbuf, int eolchar)
{
	int cpos;	/* current character position in string */
	int c;			/* current input character */
	int quotef;		/* are we quoting the next char? */
	char tmpbuf[NSTRING + 16];	/* control key expanded */

	expandp(prompt, tmpbuf, sizeof(tmpbuf) - 16);

	/* prompt the user for the input string */
	if (discmd>0)
		mlwrite("%s", tmpbuf);
	else
		movecursor(term.t_nrow, 0);

	cpos = 0;
	quotef = FALSE;

	for (;;) {
		/* get a character from the user */
		c = getkey();

		if (kanji1st(inpkanji, c)) {
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

			if (cpos < nbuf - 1) {
				buf[cpos++] = (sjis>>8) & 0xff;
				buf[cpos++] = sjis & 0xff;

				tmpbuf[0] = (sjis>>8) & 0xff;
				tmpbuf[1] = sjis & 0xff;
				tmpbuf[2] = '\0';
				outstring0(tmpbuf);
				ttcol+=2;
				TTflush();
			}

		} else {
			/* 1byte ascii or Control-Code */

			/* if they hit the line terminate, wrap it up */
			if (c == eolchar && quotef == FALSE) {
				buf[cpos++] = 0;

				/* clear the message line */
				mlwrite("");
				TTflush();

				/* if we default the buffer, return FALSE */
				if (buf[0] == 0)
					return FALSE;

				return TRUE;
			}

			/* change from command form back to character form */
			c = ectoc(c);

			if (c == ectoc(abortc) && quotef == FALSE) {
				/* Abort the input? */
				ctrlg(FALSE, 0);
				TTflush();
				return ABORT;
			} else if ((c == 0x7F || c == 0x08) && quotef == FALSE) {
				/* rubout/erase */

				if (cpos != 0) {
					int i;
					switch (nthctype((unsigned char *)buf, cpos - 1)) {
					case CT_ANK:
						char2sym(buf[--cpos], tmpbuf);
						for (i = 0; i < strlen(tmpbuf); i++) {
							outstring0("\b \b");
							--ttcol;
						}
						break;
					case CT_KJ2:
						outstring0("\b\b  \b\b");
						cpos -= 2;
						ttcol -= 2;
						break;
					case CT_KJ1:
					case CT_ILGL:
						outstring0("\b \b");
						cpos--;
						ttcol--;
						break;
					}
					TTflush();
				}

			} else if (c == 0x15 && quotef == FALSE) {
				/* C-U, kill */
				while (cpos > 0) {
					int i;
					switch (nthctype((unsigned char *)buf, cpos - 1)) {
					case CT_ANK:
						char2sym(buf[--cpos], tmpbuf);
						for (i = 0; i < strlen(tmpbuf); i++) {
							outstring0("\b \b");
							--ttcol;
						}
						break;
					case CT_KJ2:
						outstring0("\b\b  \b\b");
						cpos -= 2;
						ttcol -= 2;
						break;
					case CT_KJ1:
					case CT_ILGL:
						outstring0("\b \b");
						cpos--;
						ttcol--;
						break;
					}
				}
				TTflush();

			} else if (c == quotec && quotef == FALSE) {
				quotef = TRUE;
			} else {
				quotef = FALSE;
				if (cpos < nbuf - 1) {
					buf[cpos++] = c;
	
					char2sym(c, tmpbuf);
					outstring0(tmpbuf);
					ttcol += strlen(tmpbuf);
					TTflush();
				}
			}
		}
	}
}

/* output a string of input characters */
int
outstring(char *s)	/* string to output */
{
	if (discmd > 0 && disinp) {
		while (*s) {
			mlout(*s++);
			ttcol++;
		}
	}
	return TRUE;
}

int
outstring0(char *s)	/* string to output */
{
	if (discmd > 0 && disinp) {
		while (*s) {
			mlout(*s++);
		}
	}
	return TRUE;
}
