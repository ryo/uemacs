/*
 * $Id: eval.c,v 1.74 2017/01/02 18:14:48 ryo Exp $
 *
 * eval.c: Expresion evaluation functions for MicroEMACS
 *
 * written 1986 by Daniel Lawrence
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>
#include <time.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "evar.h"
#include "kanji.h"

static char *gtfun(char *);
static char *funval(int);
static char *funval3(int);
static char *getkill(void);
static void setkill(char *);
static int sindex(char *,char *);
static int rsindex(char *,char *);
static char *xlat(char *,char *,char *);

static char *getword(void);
static char *getword2(void);
static int getbufnum(void);
static int getbufpos(void);
static int getwindnum(void);
static int getwindpos(void);
static int iscsym(int);
static void pad(char *,int);
static int reg_match(char *, char *);
static char *reg_replace(char *, char *);
static int kanjinametonum(char *);
static char *numtokanjiname(int);

/* initialize the user variable list */
int
varinit(void)
{
	int i;

	for (i = 0; i < MAXVARS; i++)
		uv[i].u_name[0] = 0;

	return 0;
}

/* initialize the user variable list */
int
varclean(void)
{
	int i;

	for (i = 0; i < MAXVARS; i++)
		if (uv[i].u_name[0] != 0)
			FREE(uv[i].u_value);

	return 0;
}

/* evaluate a function */
static char *
gtfun(char *fname)	/* name of function to evaluate */
{
	int fnum;	/* index to function to eval */
	int arg;	/* value of some arguments */
	char arg1[NSTRING];	/* value of first argument */
	char arg2[NSTRING];	/* value of second argument */
	char arg3[NSTRING];	/* value of third argument */
	static char result[NSTRING];	/* string result */

	/* look the function up in the function table */
	fname[3] = 0;	/* only first 3 chars significant */
	mklower(fname);	/* and let it be upper or lower case */

	fnum = binary(fname, funval, NFUNCS);

	/* return errorm on a bad reference */
	if (fnum == -1) {
		fnum = binary(fname, funval3, NFUNCS);
		if (fnum == -1)
			return (char*)errorm;
	}

	/* if needed, retrieve the first argument */
	if (funcs[fnum].f_type >= MONAMIC) {
		if (macarg(arg1) != TRUE)
			return (char*)errorm;

		/* if needed, retrieve the second argument */
		if (funcs[fnum].f_type >= DYNAMIC) {
			if (macarg(arg2) != TRUE)
				return (char*)errorm;

			/* if needed, retrieve the third argument */
			if (funcs[fnum].f_type >= TRINAMIC)
				if (macarg(arg3) != TRUE)
					return (char*)errorm;
		}
	}
	/* and now evaluate it! */
	switch (fnum) {
	case UFADD:
		return int_asc(asc_int(arg1) + asc_int(arg2));
	case UFSUB:
		return int_asc(asc_int(arg1) - asc_int(arg2));
	case UFTIMES:
		return int_asc(asc_int(arg1) * asc_int(arg2));
	case UFDIV:
		return int_asc(asc_int(arg1) / asc_int(arg2));
	case UFMOD:
		return int_asc(asc_int(arg1) % asc_int(arg2));
	case UFNEG:
		return int_asc(-asc_int(arg1));
	case UFCAT:
		strcpy(result, arg1);
		return strcat(result, arg2);
	case UFLEFT:
		return bytecopy(result, arg1, asc_int(arg2));
	case UFRIGHT:
		arg = asc_int(arg2);
		if (arg > strlen(arg1))
			arg = strlen(arg1);
		return strcpy(result, &arg1[strlen(arg1) - arg]);
	case UFMID:
		arg = asc_int(arg2);
		if (arg > strlen(arg1))
			arg = strlen(arg1);
		return bytecopy(result, &arg1[arg - 1], asc_int(arg3));
	case UFNOT:
		return ltos(stol(arg1) == FALSE);
	case UFEQUAL:
		return ltos(asc_int(arg1) == asc_int(arg2));
	case UFLESS:
		return ltos(asc_int(arg1) < asc_int(arg2));
	case UFGREATER:
		return ltos(asc_int(arg1) > asc_int(arg2));
	case UFSEQUAL:
		return ltos(strcmp(arg1, arg2) == 0);
	case UFSLESS:
		return ltos(strcmp(arg1, arg2) < 0);
	case UFSGREAT:
		return ltos(strcmp(arg1, arg2) > 0);
	case UFIND:
		return strcpy(result, fixnull(getval(arg1)));
	case UFAND:
		return ltos(stol(arg1) && stol(arg2));
	case UFOR:
		return ltos(stol(arg1) || stol(arg2));
	case UFLENGTH:
		return int_asc(strlen(arg1));
	case UFUPPER:
		return mkupper(arg1);
	case UFLOWER:
		return mklower(arg1);
	case UFTRUTH:
		return ltos(asc_int(arg1) == 42);
	case UFASCII:
		return int_asc((int)arg1[0]);
	case UFCHR:
		result[0] = asc_int(arg1);
		result[1] = 0;
		return result;
	case UFCWD:
		{
			if (arg1[0] == '\0') {
				realpath(".", result);
			} else {
				realpath(arg1, result);
			}
			return fixnull(result);
		}
	case UFGTCMD:
		cmdstr(getcmd(), result);
		return result;
	case UFGTKEY:
		result[0] = tgetc();
		result[1] = 0;
		return result;
	case UFRND:
		return int_asc((ernd() % absv(asc_int(arg1))) + 1);
	case UFABS:
		return int_asc(absv(asc_int(arg1)));
	case UFSINDEX:
		return int_asc(sindex(arg1, arg2));
	case UFRSINDEX:
		return int_asc(rsindex(arg1, arg2));
	case UFENV:
#if ENVFUNC
		return fixnull(getenv(arg1));
#else
		return "";
#endif
	case UFBIND:
		return transbind(arg1);
	case UFEXIST:
		return ltos(fexist(arg1));
	case UFFIND:
		return fixnull(flook(arg1, TRUE));
	case UFFTIME:
		{
			time_t tval;
			struct tm tm;

			time(&tval);
			localtime_r(&tval, &tm);
			strftime(result, sizeof(result), arg1, &tm);
			return result;
		}
	case UFBAND:
		return int_asc(asc_int(arg1) & asc_int(arg2));
	case UFBOR:
		return int_asc(asc_int(arg1) | asc_int(arg2));
	case UFBXOR:
		return int_asc(asc_int(arg1) ^ asc_int(arg2));
	case UFBNOT:
		return int_asc(~asc_int(arg1));
	case UFXLATE:
		return xlat(arg1, arg2, arg3);
	case UFMATCH:
		return int_asc(reg_match(arg1, arg2));
	case UFREGEXP:
		return reg_replace(arg1, arg2);
	}

	meexit(-11);		/* never should get here */
	return (char*)0;
}

/* look up a user var's value */
char *
gtusr(char *vname)	/* name of user variable to fetch */
{
	int vnum;		/* ordinal number of user var */
	char *vptr;		/* temp pointer to function value */

	/* scan the list looking for the user var name */
	for (vnum = 0; vnum < MAXVARS; vnum++) {
		if (uv[vnum].u_name[0] == 0)
			return (char*)errorm;
		if (strcmp(vname, uv[vnum].u_name) == 0) {
			vptr = uv[vnum].u_value;
			if (vptr)
				return vptr;
			else
				return (char*)errorm;
		}
	}

	/* return errorm if we run off the end */
	return (char*)errorm;
}

static char *
funval(int i)
{
	return funcs[i].f_name;
}

static char *
funval3(int i)
{
	static char tmp[4];
	strncpy(tmp,funcs[i].f_name,3);
	tmp[3] = '\0';
	return tmp;
}

int
envnum(void)
{
	return NEVARS;
}

char *
envval(int i)
{
	return envars[i];
}

/*
 * key: key string to look for
 * tval: ptr to function to fetch table value with
 * tlength: length of table to search
 */
int
binary(char *key, char *(*tval)(int), int tlength)
{
	int l, u;	/* lower and upper limits of binary search */
	int i;		/* current search index */
	int cresult;	/* result of comparison */

	/* set current search limit as entire list */
	l = 0;
	u = tlength - 1;

	/* get the midpoint! */
	while (u >= l) {
		i = (l + u) >> 1;

		/* do the comparison */
		cresult = strcmp(key, (*tval) (i));
		if (cresult == 0)
			return i;
		if (cresult < 0)
			u = i - 1;
		else
			l = i + 1;
	}
	return -1;
}

char *
gtenv(char *vname)	/* name of environment variable to retrieve */
{
	int vnum;	/* ordinal number of var refrenced */
	static char result[NSTRING];	/* string result */

	/* scan the list, looking for the referenced name */
	vnum = binary(vname, envval, NEVARS);

	/* return errorm on a bad reference */
	if (vnum == -1)
		return (char*)errorm;

	/* otherwise, fetch the appropriate value */
	switch (vnum) {
	case EVFILLCOL:
		return int_asc(fillcol);
	case EVPAGELEN:
		return int_asc(term.t_nrow + 1);
	case EVCURCOL:
		return int_asc(getccol(FALSE));
	case EVCURLABEL:
		return getclabel();
	case EVCURLINE:
		return int_asc(getcline());
	case EVRAM:
		return int_asc((int) (envram / 1024l));
	case EVFLICKER:
		return ltos(flickcode);
	case EVCURWIDTH:
		return int_asc(term.t_ncol);
	case EVCBFLAGS:
		return int_asc(curbp->b_flag);
	case EVCBUFNAME:
		return curbp->b_bname;
	case EVCFNAME:
		return curbp->b_fname;
	case EVSRES:
		return sres;
	case EVDEBUG:
		return ltos(macbug);
	case EVSTATUS:
		return ltos(cmdstatus);
	case EVPALETTE:
		return palstr;
	case EVASAVE:
		return int_asc(gasave);
	case EVACOUNT:
		return int_asc(gacount);
	case EVBUFNUM:
		return int_asc(getbufnum());
	case EVBUFPOS:
		return int_asc(getbufpos());
	case EVWINDNUM:
		return int_asc(getwindnum());
	case EVWINDPOS:
		return int_asc(getwindpos());


	case EVLASTKEY:
		return int_asc(lastkey);
	case EVCURCHAR:
		{
			unsigned char c1;
			unsigned char c2;

			c1 = (curwp->w_dotp->l_used == curwp->w_doto) ?
			    '\r' : lgetc(curwp->w_dotp, curwp->w_doto);

			if (kanji1st(KANJI_SJIS,c1)) {
				c2 = (curwp->w_dotp->l_used == curwp->w_doto+1) ?
				    '\r' : lgetc(curwp->w_dotp, curwp->w_doto+1);

				return int_hex(c1*256+c2,4);
			}
			return int_hex(c1,2);
		}
	case EVCURWORD:
		return getword();
	case EVCURTEXT:
		return getword2();
	case EVDISCMD:
		return int_asc(discmd);
	case EVANSIESC:
		return ltos(ansiesc);
	case EVMAKBAK:
		return ltos(makbak);
	case EVVERSION:
		return VERSION;
	case EVPROGNAME:
		return PROGNAME;
	case EVLANG:
		return LANGUAGE;
	case EVSEED:
		return int_asc(seed);
	case EVDISINP:
		return ltos(disinp);
	case EVWLINE:
		return int_asc(curwp->w_ntrows);
	case EVCWLINE:
		return int_asc(getwpos());
	case EVTARGET:
		saveflag = lastflag;
		return int_asc(curgoal);
	case EVSEARCH:
		return pat;
	case EVTIME:
		return timeset();
	case EVREPLACE:
		return rpat;
	case EVMATCH:
		return fixnull(patmatch);
	case EVLOCALMLFORM:
		return fixnull(curbp->b_mlform);
	case EVLOCALMLCOL:
		return int_hex(curbp->b_mlcol,4);
	case EVMLFORM:
		return fixnull(mlform);
	case EVMLCOL:
		return int_hex(mlcol,4);
	case EVKILL:
		return getkill();
	case EVREGION:
		return getreg();
	case EVCMODE:
		return int_asc(curbp->b_mode);
	case EVGMODE:
		return int_asc(gmode);
	case EVTPAUSE:
		return int_asc(term.t_pause);
	case EVPENDING:
#if TYPEAH
		return ltos(typahead());
#else
		return falsem;
#endif
	case EVLWIDTH:
		return int_asc(llength(curwp->w_dotp));
	case EVLINE:
		return getctext();
	case EVGFLAGS:
		return int_asc(gflags);
	case EVRVAL:
		return int_asc(rval);
	case EVABORTHOOK:
		return aborthook;
	case EVREADHOOK:
		return readhook;
	case EVPREADHOOK:
		return postreadhook;
	case EVWRAPHOOK:
		return wraphook;
	case EVCMDHOOK:
		return cmdhook;
	case EVNEWLINEHOOK:
		return newlinehook;
	case EVXPOS:
		return int_asc(xpos);
	case EVYPOS:
		return int_asc(ypos);
	case EVSTERM:
		cmdstr(sterm, result);
		return result;
	case EVMODEFLAG:
		return ltos(modeflag);
	case EVSSCROLL:
		return ltos(sscroll);
	case EVLASTMESG:
		return lastmesg;
	case EVHARDTAB:
		return int_asc(curbp->b_tabs);
	case EVSOFTTAB:
		return int_asc(stabsize);
	case EVSSAVE:
		return ltos(ssave);
	case EVKANJIDISP:
		return numtokanjiname(dispkanji);
	case EVKANJIINP:
		return numtokanjiname(inpkanji);
	case EVKANJICODE:
		return numtokanjiname(curbp->b_kanjicode);
	case EVKBDMACRO:
		return ltos(kbdmode == PLAY);
	case EVDISLABEL:
		return ltos(dislabel);
	case EVDISNUM:
		return ltos(disnum);
	case EVDISPCR:
		return ltos(dispcr);
	case EVDISPEOF:
		return ltos(dispeof);
	case EVDISTAB:
		return ltos(distab);
	case EVDISZENSPC:
		return ltos(diszen);
	case EVCRCHAR:
		return int_hex(crchar,2);
	case EVCRCOL:
		return int_hex(crcol,4);
	case EVEOFCOL:
		return int_hex(eofcol,4);
	case EVEOFRETURN:
		return ltos(curbp->b_eofreturn);
	case EVTABCOL:
		return int_hex(tabcol,4);
	case EVCMDCOL:
		return int_hex(cmdcol,4);
	case EVNUMCOL:
		return int_hex(numcol,4);
	case EVEMPHASISBOLD:
		return int_asc(emphasisbold);
	case EVEMPHASISCOL:
		return int_hex(emphasiscol,2);
	case EVCOMMENT_IN:
		return comment_in;
	case EVCOMMENT_LINE:
		return comment_line;
	case EVCOMMENT_OUT:
		return comment_out;
	case EVCOMMENTCOL:
		return int_hex(commentcol,4);
	case EVQUOTECOL:
		return int_hex(quotecol,4);
	case EVFCOL:
		return int_asc(curwp->w_fcol);
	case EVFUNCCOL:
		return int_hex(funccol,4);
	case EVHSCROLL:
		return ltos(hscroll);
	case EVIFSPLIT:
		return ltos(ifsplit);
	case EVHJUMP:
		return int_asc(hjump);
	case EVBOM:
		return ltos(curbp->b_bom);
	case EVBUFHOOK:
		return bufhook;
	case EVEXBHOOK:
		return exbhook;
	case EVMODIFYHOOK:
		return modifyhook;
	case EVWRITEHOOK:
		return writehook;
	case EVPWRITEHOOK:
		return postwritehook;
	case EVDIAGFLAG:
		return ltos(diagflag);
	case EVMSFLAG:
		return ltos(mouseflag);
	case EVOCRYPT:
		return ltos(oldcrypt);
	}
	meexit(-12);		/* again, we should never get here */
	return NULL;
}

/* Don't return NULL pointers! */
char *
fixnull(char *s)
{
	if (s == NULL)
		return "";
	else
		return s;
}

static void
setkill(char *value)
{
	if ((lastflag & CFKILL) == 0)
		kdelete();
	thisflag |= CFKILL;

	while (*value) {
		kinsert(*value++);
	}
}

/* return some of the contents of the kill buffer */
static char *
getkill(void)
{
	int size;	/* max number of chars to return */
	static char value[NSTRING];	/* temp buffer for value */

	if (kbufh == NULL)
		/* no kill buffer....just a null string */
		value[0] = 0;
	else {
		/* copy in the contents... */
		if (kused < NSTRING)
			size = kused;
		else
			size = NSTRING - 1;
		bytecopy(value, kbufh->d_chunk, size);
	}

	/* and return the constructed value */
	return value;
}

/* set a variable */
int
setvar(int f, int n)
{
	int status;		/* status return */
	VDESC vd;		/* variable num/type */
	char var[NVSIZE + 1];	/* name of variable to fetch */
	char value[NSTRING];	/* value to set variable to */

	/* first get the variable to set.. */
	if (clexec == FALSE) {
		status = getvar(TEXT51, &var[0], NVSIZE + 1);
		/* "Variable to set: " */
		if (status != TRUE)
			return status;
	} else {		/* macro line argument */
		/* grab token and skip it */
		execstr = token(execstr, var, NVSIZE + 1);
	}

	/* check the legality and find the var */
	findvar(var, &vd, NVSIZE + 1);

	/* if its not legal....bitch */
	if (vd.v_type == -1) {
		mlwrite(TEXT52, var);
		/* "%%No such variable as '%s'" */
		return FALSE;
	}
	/* get the value for that variable */
	if (f == TRUE)
		strcpy(value, int_asc(n));
	else {
		status = mlreply(TEXT53, &value[0], NSTRING);
		/* "Value: " */
		if (status != TRUE)
			return status;
	}

	/* and set the appropriate value */
	status = svar(&vd, value);

#if DEBUGM
	/*
	 * if $debug == TRUE, every assignment will echo a statment to that
	 * effect here.
	 */

	if (macbug && (strcmp(var, "%track") != 0)) {
		strcpy(outline, "(((");

		strcat(outline, var);
		strcat(outline, " <- ");

		/* and lastly the value we tried to assign */
		strcat(outline, value);
		strcat(outline, ")))");

		/* expand '%' to "%%" so mlwrite wont bitch */
		makelit(outline);

		/* write out the debug line */
		mlforce(outline);
		update(TRUE);

		/* and get the keystroke to hold the output */
		if (getkey() == abortc) {
			mlforce(TEXT54);
			/* "[Macro aborted]" */
			status = FALSE;
		}
	}
#endif

	/* and return it */
	return status;
}

/* find a variables type and name */
int
findvar(char *var, VDESC *vd, int size)
{
	int vnum;	/* subscript in varable arrays */
	int vtype;	/* type to return */

	(void)&vnum; /* stupid gcc */

fvar:
	vtype = -1;
	switch (var[0]) {

	case '$':		/* check for legal enviromnent var */
		for (vnum = 0; vnum < NEVARS; vnum++)
			if (strcmp(&var[1], envars[vnum]) == 0) {
				vtype = TKENV;
				break;
			}
		break;

	case '%':		/* check for existing legal user variable */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (strcmp(&var[1], uv[vnum].u_name) == 0) {
				vtype = TKVAR;
				break;
			}
		if (vnum < MAXVARS)
			break;

		/* create a new one??? */
		for (vnum = 0; vnum < MAXVARS; vnum++)
			if (uv[vnum].u_name[0] == 0) {
				vtype = TKVAR;
				strcpy(uv[vnum].u_name, &var[1]);
				uv[vnum].u_value = NULL;
				break;
			}
		break;

	case '&':		/* indirect operator? */
		var[4] = 0;
		if (strcmp(&var[1], "ind") == 0) {
			/* grab token, and eval it */
			execstr = token(execstr, var, size);
			strcpy(var, fixnull(getval(var)));
			goto fvar;
		}
	}

	/* return the results */
	vd->v_num = vnum;
	vd->v_type = vtype;
	return 0;
}

/* set a variable */
int
svar(VDESC *var, char *value)
{
	int vnum;	/* ordinal number of var refrenced */
	int vtype;	/* type of variable to set */
	int status;	/* status return */
	int c;		/* translated character */
	char *sp;	/* scratch string pointer */

	/* simplify the vd structure (we are gonna look at it a lot) */
	vnum = var->v_num;
	vtype = var->v_type;

	/* and set the appropriate value */
	status = TRUE;
	switch (vtype) {
	case TKVAR:		/* set a user variable */
		if (uv[vnum].u_value != NULL)
			FREE(uv[vnum].u_value);
		sp = (void*)MALLOC(strlen(value) + 1);
		if (sp == NULL)
			return FALSE;
		strcpy(sp, value);
		uv[vnum].u_value = sp;
		break;

	case TKENV:		/* set an environment variable */
		status = TRUE;	/* by default */
		switch (vnum) {
		case EVFILLCOL:
			fillcol = asc_int(value);
			break;
		case EVPAGELEN:
			status = newsize(TRUE, asc_int(value));
			break;
		case EVCURCOL:
			status = setccol(asc_int(value));
			break;
		case EVCURLABEL:
			status = putclabel(value);
			if (leftmargin)
				upwind();
			break;
		case EVCURLINE:
			status = gotoline(TRUE, asc_int(value));
			break;
		case EVRAM:
			break;
		case EVFLICKER:
			flickcode = stol(value);
			break;
		case EVCURWIDTH:
			status = newwidth(TRUE, asc_int(value));
			break;
		case EVCBFLAGS:
			curbp->b_flag = (curbp->b_flag & ~(BFCHG | BFINVS))
				| (asc_int(value) & (BFCHG & BFINVS));
			lchange(WFMODE);
			break;
		case EVCBUFNAME:
			strcpy(curbp->b_bname, value);
			curwp->w_flag |= WFMODE;
			break;
		case EVCFNAME:
			strcpy(curbp->b_fname, value);
			curwp->w_flag |= WFMODE;
			break;
		case EVSRES:
			status = TTrez(stol(value));
			break;
		case EVDEBUG:
			macbug = stol(value);
			break;
		case EVSTATUS:
			cmdstatus = stol(value);
			break;
		case EVPALETTE:
			bytecopy(palstr, value, 48);
			spal(palstr);
			break;
		case EVASAVE:
			gasave = asc_int(value);
			break;
		case EVACOUNT:
			gacount = asc_int(value);
			break;
		case EVLASTKEY:
			lastkey = asc_int(value);
			break;
		case EVCURCHAR:
			ldelete(1L, FALSE);	/* delete 1 char */
			c = asc_int(value);
			if (c == '\r')
				lnewline();
			else
				linsert(1, c);
			backchar(FALSE, 1);
			break;
		case EVDISCMD:
			discmd = asc_int(value);
			break;
		case EVANSIESC:
			ansiesc = stol(value);
			upwind();
			break;
		case EVMAKBAK:
			makbak = stol(value);
			break;
		case EVVERSION:
			break;
		case EVPROGNAME:
			break;
		case EVLANG:
			break;
		case EVSEED:
			seed = asc_int(value);
			break;
		case EVDISINP:
			disinp = stol(value);
			break;
		case EVWLINE:
			status = resize(TRUE, asc_int(value));
			break;
		case EVCWLINE:
			status = forwline(TRUE,
					  asc_int(value) - getwpos());
			break;
		case EVTARGET:
			curgoal = asc_int(value);
			thisflag = saveflag;
			break;
		case EVSEARCH:
			strcpy(pat, value);
			setjtable();	/* Set up fast search arrays  */
#if MAGIC
			mcclear();
#endif
			break;
		case EVTIME:
			break;
		case EVREPLACE:
			strcpy(rpat, value);
			break;
		case EVMATCH:
			break;
		case EVMLFORM:
			strcpy(mlform, value);
			break;
		case EVMLCOL:
			mlcol = asc_int(value);
			break;
		case EVKILL:
			setkill(value);
			break;
		case EVREGION:
			break;
		case EVCMODE:
			curbp->b_mode = asc_int(value);
			curwp->w_flag |= WFMODE;
			break;
		case EVGMODE:
			gmode = asc_int(value);
			break;
		case EVTPAUSE:
			term.t_pause = asc_int(value);
			break;
		case EVPENDING:
			break;
		case EVLWIDTH:
			break;
		case EVLINE:
			putctext(value);
			break;
		case EVLOCALMLFORM:
			strcpy(curbp->b_mlform, value);
			upmode();
			break;
		case EVLOCALMLCOL:
			curbp->b_mlcol = asc_int(value);
			upmode();
			break;
		case EVGFLAGS:
			gflags = asc_int(value);
			break;
		case EVRVAL:
			break;
		case EVABORTHOOK:
			strcpy(aborthook, value);
			break;
		case EVREADHOOK:
			strcpy(readhook, value);
			break;
		case EVPREADHOOK:
			strcpy(postreadhook, value);
			break;
		case EVWRAPHOOK:
			strcpy(wraphook, value);
			break;
		case EVCMDHOOK:
			strcpy(cmdhook, value);
			break;
		case EVXPOS:
			xpos = asc_int(value);
			break;
		case EVYPOS:
			ypos = asc_int(value);
			break;
		case EVSTERM:
			sterm = stock(value);
			break;
		case EVMODEFLAG:
			modeflag = stol(value);
			upwind();
			break;
		case EVSSCROLL:
			sscroll = stol(value);
			break;
		case EVLASTMESG:
			strcpy(lastmesg, value);
			break;
		case EVHARDTAB:
			curbp->b_tabs = asc_int(value);
			upwind();
			break;
		case EVSOFTTAB:
			stabsize = asc_int(value);
			upwind();
			break;
		case EVSSAVE:
			ssave = stol(value);
			break;
		case EVKANJIDISP:
			dispkanji = kanjinametonum(value);
			upwind();
			break;
		case EVKANJIINP:
			inpkanji = kanjinametonum(value);
			break;
		case EVKBDMACRO:
			break;
		case EVKANJICODE:
			curbp->b_kanjicode = kanjinametonum(value);
			curwp->w_flag |= WFMODE;
			break;
		case EVDISLABEL:
			dislabel = stol(value);
			leftmargin = (dislabel ? CTX_LABELLEN : 0) + (disnum ? 6 : 0);
			if (dislabel)
				update_line_context_all(curbp);
			upwind();
			break;
		case EVDISNUM:
			disnum = stol(value);
			leftmargin = (dislabel ? CTX_LABELLEN : 0) + (disnum ? 6 : 0);
			if (dislabel)
				update_line_context_all(curbp);
			upwind();
			break;
		case EVDISPCR:
			dispcr = stol(value);
			upwind();
			break;
		case EVDISPEOF:
			dispeof = stol(value);
			upwind();
			break;
		case EVDISTAB:
			distab = stol(value);
			upwind();
			break;
		case EVDISZENSPC:
			diszen = stol(value);
			upwind();
			break;
		case EVEMPHASISBOLD:
			emphasisbold = asc_int(value);
			upwind();
			break;
		case EVEMPHASISCOL:
			emphasiscol = asc_int(value);
			upwind();
			break;
		case EVCOMMENT_IN:
			strcpy(comment_in, value);
			upwind();
			break;
		case EVCOMMENT_LINE:
			strcpy(comment_line, value);
			upwind();
			break;
		case EVCOMMENT_OUT:
			strcpy(comment_out, value);
			upwind();
			break;
		case EVCOMMENTCOL:
			commentcol = asc_int(value);
			upwind();
			break;
		case EVQUOTECOL:
			quotecol = asc_int(value);
			upwind();
			break;
		case EVCRCOL:
			crcol = asc_int(value);
			upwind();
			break;
		case EVEOFCOL:
			eofcol = asc_int(value);
			upwind();
			break;
		case EVEOFRETURN:
			curbp->b_eofreturn = stol(value);
			break;
		case EVTABCOL:
			tabcol = asc_int(value);
			upwind();
			break;
		case EVCMDCOL:
			cmdcol = asc_int(value);
			upwind();
			break;
		case EVNUMCOL:
			numcol = asc_int(value);
			upwind();
			break;
		case EVCRCHAR:
			crchar = asc_int(value);
			upwind();
			break;
		case EVFCOL:
			curwp->w_fcol = asc_int(value);
			if (curwp->w_fcol < 0)
				curwp->w_fcol = 0;
			curwp->w_flag |= WFHARD | WFMODE;
			break;
		case EVFUNCCOL:
			funccol = asc_int(value);
			upwind();
			break;
		case EVHSCROLL:
			hscroll = stol(value);
			lbound = 0;
			break;
		case EVIFSPLIT:
			ifsplit = stol(value);
			break;
		case EVHJUMP:
			hjump = asc_int(value);
			if (hjump < 1)
				hjump = 1;
			if (hjump > term.t_ncol - 1)
				hjump = term.t_ncol - 1;
			break;
		case EVBOM:
			curbp->b_bom = stol(value);
			break;
		case EVBUFHOOK:
			strcpy(bufhook, value);
			break;
		case EVEXBHOOK:
			strcpy(exbhook, value);
			break;
		case EVMODIFYHOOK:
			strcpy(modifyhook, value);
			break;
		case EVWRITEHOOK:
			strcpy(writehook, value);
			break;
		case EVPWRITEHOOK:
			strcpy(postwritehook, value);
			break;
		case EVNEWLINEHOOK:
			strcpy(newlinehook, value);
			break;
		case EVDIAGFLAG:
			diagflag = stol(value);
			break;
		case EVMSFLAG:
			mouseflag = stol(value);
			break;
		case EVOCRYPT:
			oldcrypt = stol(value);
			break;
		}
		break;
	}
	return status;
}

/*
 * asc_int: ascii string to integer.
 * This is too inconsistant to use the system's
 */
int
asc_int(char *st)
{
	return strtol(st, NULL, 0);
}

/*
 * int_asc: integer to ascii string
 * This is too inconsistant to use the system's
 */
char *
int_asc(int i)
{
	int digit;	/* current digit being used */
	char *sp;	/* pointer into result */
	int sign;	/* sign of resulting number */
	static char result[INTWIDTH + 1];	/* resulting string */

	/* record the sign... */
	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -i;
	}
	/* and build the string (backwards!) */
	sp = result + INTWIDTH;
	*sp = 0;
	do {
		digit = i % 10;
		*(--sp) = '0' + digit;	/* and install the new digit */
		i = i / 10;
	} while (i);

	/* and fix the sign */
	if (sign == -1) {
		*(--sp) = '-';	/* and install the minus sign */
	}
	return sp;
}

char *
int_hex(int i, int column)
{
	static char result[16];	/* resulting string */

	switch (column) {
	case 4:
		sprintf(result, "0x%04x", i);
		break;
	case 8:
		sprintf(result, "0x%08x", i);
		break;
	case 2:
	default:
		sprintf(result, "0x%02x", i);
		break;
	}
	return result;
}

/* find the type of a passed token */
int
gettyp(char *tkn)
{
	char c;	/* first char in token */

	/* grab the first char (this is all we need) */
	c = *tkn;

	/* no blanks!!! */
	if (c == 0)
		return TKNUL;

	/* a numeric literal? */
	if (isdigit((int)c))
		return TKLIT;

	switch (c) {
	case '"':
		return TKSTR;
	case '!':
		return TKDIR;
	case '@':
		return TKARG;
	case '#':
		return TKBUF;
	case '$':
		return TKENV;
	case '%':
		return TKVAR;
	case '&':
		return TKFUN;
	case '*':
		return TKLBL;
	default:
		return TKCMD;
	}
}

/* find the value of a token */
char *
getval(char *tkn)
{
	int status;	/* error return */
	BUFFER *bp;	/* temp buffer pointer */
	int blen;	/* length of buffer argument */
	int distmp;	/* temporary discmd flag */
	static char buf[NSTRING];	/* string buffer for some returns */

	switch (gettyp(tkn)) {
	case TKNUL:
		return "";

	case TKARG:		/* interactive argument */
		strcpy(tkn, fixnull(getval(&tkn[1])));
		distmp = discmd;/* echo it always! */
		discmd = 1;
		status = getstring(tkn,
				   buf, NSTRING, ctoec('\r'));
		discmd = distmp;
		if (status == ABORT)
			return NULL;
		return buf;

	case TKBUF:		/* buffer contents fetch */

		/* grab the right buffer */
		strcpy(tkn, fixnull(getval(&tkn[1])));
		bp = bfind(tkn, FALSE, 0);
		if (bp == NULL)
			return NULL;

		/*
		 * if the buffer is displayed, get the window vars instead of
		 * the buffer vars
		 */
		if (bp->b_nwnd > 0) {
			curbp->b_dotp = curwp->w_dotp;
			curbp->b_doto = curwp->w_doto;
		}
		/* make sure we are not at the end */
		if (bp->b_linep == bp->b_dotp)
			return NULL;

		/* grab the line as an argument */
		blen = bp->b_dotp->l_used - bp->b_doto;
		if (blen > NSTRING)
			blen = NSTRING;
		bytecopy(buf, (char *)(bp->b_dotp->l_text + bp->b_doto), blen);
		buf[blen] = 0;

		/* and step the buffer's line ptr ahead a line */
		bp->b_dotp = bp->b_dotp->l_fp;
		bp->b_doto = 0;

		/* if displayed buffer, reset window ptr vars */
		if (bp->b_nwnd > 0) {
			curwp->w_dotp = curbp->b_dotp;
			curwp->w_doto = 0;
			curwp->w_flag |= WFMOVE;
		}
		/* and return the spoils */
		return buf;

	case TKVAR:
		return gtusr(tkn+1);
	case TKENV:
		return gtenv(tkn+1);
	case TKFUN:
		return gtfun(tkn+1);
	case TKDIR:
		return NULL;
	case TKLBL:
		return NULL;
	case TKLIT:
		return tkn;
	case TKSTR:
		return tkn + 1;
	case TKCMD:
		return tkn;
	}
	return NULL;
}

/* convert a string to a numeric logical */
int
stol(char *val)
{
	/* check for logical values */
	if (val[0] == 'F')
		return FALSE;
	if (val[0] == 'T')
		return TRUE;

	/* check for numeric truth (!= 0) */
	return (asc_int(val) != 0);
}

/* numeric logical to string logical */
char *
ltos(int val)
{
	if (val)
		return (char*)truem;
	else
		return (char*)falsem;
}

/* make a string upper case */
char *
mkupper(char *str)
{
	char *sp;

	sp = str;
	while (*sp)
		uppercase(sp++);
	return str;
}

/* make a string lower case */
char *
mklower(char *str)
{
	char *sp;

	sp = str;
	while (*sp)
		lowercase(sp++);
	return str;
}

/* take the absolute value of an integer */
int
absv(int x)
{
	return x < 0 ? -x : x;
}

int
ernd(void)
{				/* returns a random integer */
	seed = absv(seed * 1721 + 10007);
	return seed;
}

/*
 * find pattern within source
 *
 *  source: source string to search
 *  pattern: string to look for
 */
static int
sindex(char *source, char *pattern)
{
	char *sp;	/* ptr to current position to scan */
	char *csp;	/* ptr to source string during comparison */
	char *cp;	/* ptr to place to check for equality */

	/* scanning through the source string */
	sp = source;
	while (*sp) {
		/* scan through the pattern */
		cp = pattern;
		csp = sp;
		while (*cp) {
			if (!eq(*cp, *csp))
				break;
			++cp;
			++csp;
		}

		/* was it a match? */
		if (*cp == 0)
			return (int) (sp - source) + 1;
		++sp;
	}

	/* no match at all.. */
	return 0;
}

static int
rsindex(char *buf, char *pattern)
{
	int blen, plen;
	char *b;

	blen = strlen(buf);
	plen = strlen(pattern);
	if (plen > blen)
		return 0;
	for (b = buf + blen - plen; b >= buf; b--) {
		while (*b != *pattern) {
			b--;
			if (b < buf)
				return 0;
		}
		if (!strncmp(b, pattern, plen))
			return b - buf + 1;
	}
	return 0;
}

/*
 * Filter a string through a translation table
 *
 *  source: string to filter
 *  lookup: characters to translate
 *  trans: resulting translated characters
 */
static char *
xlat(char *source, char *lookup, char *trans)
{
	char *sp;	/* pointer into source table */
	char *lp;	/* pointer into lookup table */
	char *rp;	/* pointer into result */
	static char result[NSTRING];	/* temporary result */

	/* scan source string */
	sp = source;
	rp = result;
	while (*sp) {
		/* scan lookup table for a match */
		lp = lookup;
		while (*lp) {
			if (*sp == *lp) {
				*rp++ = trans[lp - lookup];
				goto xnext;
			}
			++lp;
		}

		/* no match, copy in the source char untranslated */
		*rp++ = *sp;
xnext:
		++sp;
	}

	/* terminate and return the result */
	*rp = 0;
	return result;
}

#if DEBUGM
/* display a variable's value */
int
dispvar(int f, int n)
{
	int status;		/* status return */
	VDESC vd;		/* variable num/type */
	char var[NVSIZE + 1];	/* name of variable to fetch */

	/* first get the variable to display.. */
	if (clexec == FALSE) {
		status = getvar(TEXT55, &var[0], NVSIZE + 1);
		/* "Variable to display" */
		if (status != TRUE)
			return status;
	} else {		/* macro line argument */
		/* grab token and skip it */
		execstr = token(execstr, var, NVSIZE + 1);
	}

	/* check the legality and find the var */
	findvar(var, &vd, NVSIZE + 1);

	/* if its not legal....bitch */
	if (vd.v_type == -1) {
		mlwrite(TEXT52, var);
		/* "%%No such variable as '%s'" */
		return FALSE;
	}
	/* and display the value */
	strcpy(outline, var);
	strcat(outline, " = ");

	/* and lastly the current value */
	strcat(outline, fixnull(getval(var)));

	/* expand '%' to "%%" so mlwrite wont bitch */
	makelit(outline);

	/* write out the result */
	mlforce(outline);
	update(TRUE);

	/* and return */
	return TRUE;
}

/*
 * pad a string to indicated length
 *
 *  s: string to add spaces to
 *  len: wanted length of string
 */
static void
pad(char *s, int len)
{
	while (strlen(s) < len) {
		strcat(s, "          ");
		s[len] = 0;
	}
}

/*
 * describe-variables	Bring up a fake buffer and list the contents of all
 * the environment variables
 */
int
desvars(int f, int n)
{
	WINDOW *wp;		/* scanning pointer to windows */
	BUFFER *bp;		/* buffer to put binding list into */
	int uindex;		/* index into uvar table */
	int cmark;		/* current mark */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* split the current window to make room for the variable list */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and get a buffer for it */
	bp = bfind(TEXT56, TRUE, 0);
	/* "Variable list" */
	if (bp == NULL || bclear(bp) == FALSE) {
		mlwrite(TEXT57);
		/* "Can not display variable list" */
		return FALSE;
	}
	/* let us know this is in progress */
	mlwrite(TEXT58);
	/* "[Building variable list]" */

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

	/* build the environment variable list */
	for (uindex = 0; uindex < NEVARS; uindex++) {

		/* add in the environment variable name */
		strcpy(outseq, "$");
		strcat(outseq, envars[uindex]);
		pad(outseq, 18);

		/* add in the value */
		strcat(outseq, gtenv(envars[uindex]));
		strcat(outseq, "\r");

		/* and add it as a line into the buffer */
		if (linstr(outseq) != TRUE)
			return FALSE;
	}

	linstr("\r\r");

	/* build the user variable list */
	for (uindex = 0; uindex < MAXVARS; uindex++) {
		if (uv[uindex].u_name[0] == 0)
			break;

		/* add in the user variable name */
		strcpy(outseq, "%");
		strcat(outseq, uv[uindex].u_name);
		pad(outseq, 18);

		/* add in the value */
		strcat(outseq, uv[uindex].u_value);
		strcat(outseq, "\r");

		/* and add it as a line into the buffer */
		if (linstr(outseq) != TRUE)
			return FALSE;
	}

	curwp->w_bufp->b_mode |= MDVIEW;	/* put this buffer view mode */
	curbp->b_flag &= ~BFCHG;/* don't flag this as a change */
	wp->w_dotp = lforw(bp->b_linep);	/* back to the beginning */
	wp->w_doto = 0;
	upmode();
	mlerase();		/* clear the mode line */
	return TRUE;
}

/*
 * describe-functions
 * Bring up a fake buffer and list the names of all the functions
 */
int
desfunc(int f, int n)
{
	WINDOW *wp;		/* scanning pointer to windows */
	BUFFER *bp;		/* buffer to put binding list into */
	int uindex;		/* index into funcs table */
	int cmark;		/* current mark */
	char outseq[80];	/* output buffer for keystroke sequence */

	/* split the current window to make room for the variable list */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and get a buffer for it */
	bp = bfind(TEXT211, TRUE, 0);
	/* "Function list" */
	if (bp == NULL || bclear(bp) == FALSE) {
		mlwrite(TEXT212);
		/* "Can not display function list" */
		return FALSE;
	}
	/* let us know this is in progress */
	mlwrite(TEXT213);
	/* "[Building function list]" */

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

	/* build the function list */
	for (uindex = 0; uindex < NFUNCS; uindex++) {

		/* add in the environment variable name */
		strcpy(outseq, "&");
		strcat(outseq, funcs[uindex].f_name);
		strcat(outseq, "\r");

		/* and add it as a line into the buffer */
		if (linstr(outseq) != TRUE)
			return FALSE;
	}

	linstr("\r");

	curwp->w_bufp->b_mode |= MDVIEW;	/* put this buffer view mode */
	curbp->b_flag &= ~BFCHG;/* don't flag this as a change */
	wp->w_dotp = lforw(bp->b_linep);	/* back to the beginning */
	wp->w_doto = 0;
	upmode();
	mlwrite("");		/* clear the mode line */
	return TRUE;
}

#endif

static int
iscsym(int ch)
{
	if (ch == ' ')
		return 0;
	if (ch == '\t')
		return 0;
	if (ch == '\n')
		return 0;
	if (ch == ':')
		return 0;
	if (ch == ';')
		return 0;
	if (ch == '/')
		return 0;
	if (ch == ')')
		return 0;
	if (ch == '(')
		return 0;
	if (ch == '}')
		return 0;
	if (ch == '{')
		return 0;
	if (ch == '[')
		return 0;
	if (ch == ']')
		return 0;
	if (ch == '#')
		return 0;
	return 1;
}

static char *
getword(void)
{
	int pos;
	char *p;
	static char result[NSTRING];

	pos = curwp->w_doto;
	p = (char *)curwp->w_dotp->l_text;

	if (pos > 0) {
		pos--;
		if ((!iscsym(p[pos]) && p[pos] != '.'))
			pos++;
	}
	while (pos >= 0) {
		if (iscsym(p[pos]) || p[pos] == '.') {
			pos--;
		} else
			break;
	}

	if (pos == curwp->w_doto) {
		*result = 0;
		return result;
	}

	{
		int len, llen;

		pos++;
		len = 0;
		llen = curwp->w_dotp->l_used;
		while (pos < llen && llen < NSTRING - 1) {
			if (iscsym(p[pos]) || p[pos] == '.')
				result[len++] = p[pos++];
			else
				break;
		}
		result[len] = 0;
	}

	return result;
}


static char *
getword2(void)
{
	int pos;
	unsigned char *p;
	static char result[NSTRING];

	pos = curwp->w_doto;
	p = curwp->w_dotp->l_text;

	if (pos > 0) {
		pos--;
		if (isspace(p[pos]))
			pos++;
	}
	while (pos >= 0) {
		if (!isspace(p[pos]))
			pos--;
		else
			break;
	}

	if (pos == curwp->w_doto) {
		*result = 0;
		return result;
	}

	{
		int len, llen;

		pos++;
		len = 0;
		llen = curwp->w_dotp->l_used;
		while (pos < llen && llen < NSTRING - 1) {
			if (!isspace(p[pos]))
				result[len++] = p[pos++];
			else
				break;
		}
		result[len] = 0;
	}

	return result;
}

static int
getbufnum(void)
{
	int n;
	BUFFER *bp;

	for (bp = bheadp, n = 0; bp; n++, bp = bp->b_bufp) {
		if (bp->b_flag & BFINVS)
			n--;
	}
	return n;
}

static int
getbufpos(void)
{
	int n;
	BUFFER *bp;

	for (bp = bheadp, n = 1; bp != curbp; n++, bp = bp->b_bufp) {
		if (bp->b_flag & BFINVS)
			n--;
	}
	return n;
}

static int
getwindnum(void)
{
	int n;
	WINDOW *wp;

	for (wp = wheadp, n = 0; wp; n++, wp = wp->w_wndp);
	return n;
}

static int
getwindpos(void)
{
	int n;
	WINDOW *wp;

	for (wp = wheadp, n = 1; wp != curwp; n++, wp = wp->w_wndp);
	return n;
}

static int
kanjinametonum(char *val)	/* convert a name of kanjicode to numeric */
{
	if ((strcasecmp("ascii",val)==0) ||
	    (strcasecmp("lagin",val)==0))
		return KANJI_ASCII;

	if ((strcasecmp("iso2022-jp",val) == 0) ||
	    (strcasecmp("japanese-jis",val)==0) ||
	    (strcasecmp("jis",val)==0))
		return KANJI_JIS;

	if ((strcasecmp("euc",val)==0) ||
	    (strcasecmp("japanese-euc",val)==0) ||
	    (strcasecmp("euc-jp",val)==0))
		return KANJI_EUC;

	if ((strcasecmp("sjis",val)==0) ||
	    (strcasecmp("japanese-sjis",val)==0) ||
	    (strcasecmp("ms-kanji",val)==0))
		return KANJI_SJIS;

	if ((strcasecmp("utf8",val)==0) ||
	    (strcasecmp("utf-8",val)==0) ||
	    (strcasecmp("unicode",val)==0))
		return KANJI_UTF8;

	if ((strcasecmp("utf16",val)==0) ||
	    (strcasecmp("utf16le",val)==0) ||
	    (strcasecmp("utf-16",val)==0) ||
	    (strcasecmp("utf-16le",val)==0))
		return KANJI_UTF16LE;

	if ((strcasecmp("utf16be",val)==0) ||
	    (strcasecmp("utf-16be",val)==0))
		return KANJI_UTF16BE;

	return -1;
}

static char *
numtokanjiname(int val)	/* numeric to name of kanjicode */
{
	char *p = "ascii";

	switch (val) {
	case KANJI_ASCII:
		p = "ascii";
		break;
	case KANJI_JIS:
		p = "jis";
		break;
	case KANJI_EUC:
		p = "euc";
		break;
	case KANJI_SJIS:
		p = "sjis";
		break;
	case KANJI_UTF8:
		p = "utf8";
		break;
	case KANJI_UTF16BE:
		p = "utf16be";
		break;
	case KANJI_UTF16LE:
		p = "utf16le";
		break;
	default:
		break;
	}

	return p;
}

static int
reg_match(char *text, char *pattern)
{
	regex_t re_pattern;
	regmatch_t pmatch;
	int r, cflags, replaceall, result;
//	int start;
	int end;
	char ch, *patt, *opt;

	ch = pattern[0];

	patt = &pattern[1];
	opt = strchr(patt, ch);
	if (opt == NULL)
		return 0;
	*opt++ = '\0';

	replaceall = 0;
	if (strchr(opt, 'g') != NULL)
		replaceall = 1;

	cflags = REG_EXTENDED;
	if (strchr(opt, 's') != NULL)
		cflags |= REG_NEWLINE;
	if (strchr(opt, 'i') != NULL)
		cflags |= REG_ICASE;

	r = regcomp(&re_pattern, patt, REG_EXTENDED | cflags);
	if (r != 0)
		return 0;

	result = 0;
	pmatch.rm_so = 0;
	pmatch.rm_eo = end = strlen(text);
	do {
//		start = pmatch.rm_so;

		r = regexec(&re_pattern, text, 1, &pmatch, REG_STARTEND);
		if (r == 0) {
			result++;

			if (pmatch.rm_so == pmatch.rm_eo)
				break;

			pmatch.rm_so = pmatch.rm_eo;
			pmatch.rm_eo = end;
		}
	} while (replaceall && (r == 0));

	regfree(&re_pattern);

	return result;
}

static char *
reg_replace(char *text, char *pattern)
{
	static char result[NSTRING];
	char *resultptr;
	int start, end;

	regex_t re_pattern;
	regmatch_t pmatch;
	int r, cflags, replaceall, replen;
	char ch, *patt, *rep, *opt;

	ch = pattern[0];

	patt = &pattern[1];
	rep = strchr(patt, ch);
	if (rep == NULL)
		return NULL;
	*rep++ = '\0';
	opt = strchr(rep, ch);
	if (opt == NULL)
		return NULL;
	*opt++ = '\0';

	replen = strlen(rep);
	replaceall = 0;
	if (strchr(opt, 'g') != NULL)
		replaceall = 1;

	cflags = REG_EXTENDED;
	if (strchr(opt, 's') != NULL)
		cflags |= REG_NEWLINE;
	if (strchr(opt, 'i') != NULL)
		cflags |= REG_ICASE;

	r = regcomp(&re_pattern, patt, REG_EXTENDED | cflags);
	if (r != 0)
		return NULL;

	resultptr = result;
	pmatch.rm_so = 0;
	pmatch.rm_eo = end = strlen(text);
	do {
		start = pmatch.rm_so;

		r = regexec(&re_pattern, text, 1, &pmatch, REG_STARTEND);
		if (r == 0) {
			memcpy(resultptr, text + start, pmatch.rm_so - start);
			resultptr += pmatch.rm_so - start;
			memcpy(resultptr, rep, replen);
			resultptr += replen;

			if (pmatch.rm_so == pmatch.rm_eo)
				break;

			pmatch.rm_so = pmatch.rm_eo;
			pmatch.rm_eo = end;

		} else {
			strcat(resultptr, text + pmatch.rm_so);
		}
	} while (replaceall && (r == 0));

	if ((replaceall == 0) && (r == 0))
		strcat(resultptr, text + pmatch.rm_so);

	regfree(&re_pattern);

	return result;
}
