/*
----------------------------------------
	EVAL.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "evar.h"
#include "ekanji.h"
#include "ecall.h"
#include "fepctrl.h"
#include "ehprint.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/eval.c,v 1.9 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static char *getkill(char *);
static char *getword(void);
static char *gtfun(char *);
static char *mklower(char *);
static char *mkupper(char *);
static char *trimstr(char *);
static char *xlat(char *, char *, char *);
static int getbufnum(void);
static int getbufpos(void);
static int getwindnum(void);
static int getwindpos(void);
static int rsindex(char *, char *);
static void get_curfig(char *, char *, int);
static void pad(char *, int);
static void set_curfig(char *, char *, int);

/*
========================================
	乱数を得る
========================================
*/

inline int ernd(void)
{
	return seed = absv(seed * 1721 + 10007);
}

/*
========================================
	関数評価
========================================
*/

static char *gtfun(char *fname)
{
	char arg1[NSTRING];
	char arg2[NSTRING], arg3[NSTRING];
	UFUNC *fun;
	static char result[NSTRING * 2];

	*arg1 = *arg2 = 0;
	*arg3 = fname[3] = 0;
	mklower(fname);
	fun = func_in_word_set(fname, strlen(fname));
	if (fun == 0)
		return (char *)errorm;

	if (fun->f_type >= MONAMIC) {
		if (macarg(arg1) != TRUE)
			return (char *)errorm;
		if (fun->f_type >= DYNAMIC) {
			if (macarg(arg2) != TRUE)
				return (char *)errorm;
			if (fun->f_type >= TRINAMIC) {
				if (macarg(arg3) != TRUE)
					return (char *)errorm;
			}
		}
	}
	switch (fun->f_treat) {
	case TRINT:
		{
			int int_1, int_2, res;

			int_1 = asc_int(arg1);
			int_2 = asc_int(arg2);
			switch (fun->f_num) {
			case UFADD:
				res = int_1 + int_2;
				break;
			case UFSUB:
				res = int_1 - int_2;
				break;
			case UFTIMES:
				res = int_1 * int_2;
				break;
			case UFDIV:
				res = int_1 / int_2;
				break;
			case UFMOD:
				res = int_1 % int_2;
				break;
			case UFNEG:
				res = -int_1;
				break;
			case UFRND:
				res = (ernd() % absv(int_1)) + 1;
				break;
			case UFABS:
				res = absv(int_1);
				break;
			case UFBAND:
				res = int_1 & int_2;
				break;
			case UFBOR:
				res = int_1 | int_2;
				break;
			case UFBXOR:
				res = int_1 ^ int_2;
				break;
			case UFBNOT:
				res = ~int_1;
				break;
			default:
				res = 0;
				break;
			}
			return int_asc(res);
		}
		break;
	case TRINS:
		{
			int int_1, int_2, res;

			int_1 = asc_int(arg1);
			int_2 = asc_int(arg2);
			switch (fun->f_num) {
			case UFEQUAL:
				res = int_1 == int_2;
				break;
			case UFLESS:
				res = int_1 < int_2;
				break;
			case UFGREATER:
				res = int_1 >= int_2;
				break;
			default:
				res = 0;
				break;
			}
			return ltos(res);
		}
		break;
	case TRLOG:
		{
			int log_1, log_2, res;

			log_1 = stol(arg1);
			log_2 = stol(arg2);
			switch (fun->f_num) {
			case UFNOT:
				res = log_1 == FALSE;
				break;
			case UFAND:
				res = log_1 && log_2;
				break;
			case UFOR:
				res = log_1 || log_2;
				break;
			default:
				res = 0;
				break;
			}
			return ltos(res);
		}
		break;
	case TRCMP:
		{
			int cmp, res;

			cmp = strcmp(arg1, arg2);
			switch (fun->f_num) {
			case UFSEQUAL:
				res = cmp == 0;
				break;
			case UFSLESS:
				res = cmp < 0;
				break;
			case UFSGREAT:
				res = cmp >= 0;
				break;
			default:
				res = 0;
				break;
			}
			return ltos(res);
		}
		break;
	case TRLEN:
		{
			int len, int_2;

			len = strlen(arg1);
			int_2 = asc_int(arg2);
			switch (fun->f_num) {
			case UFSIZE:
				return int_asc(len);
			case UFRIGHT:
				if (int_2 > len)
					int_2 = len;
				strcpy(result, arg1 + len - int_2);
				return result;
			case UFMID:
				{
					int int_3 = asc_int(arg3);

					if (int_2 > len)
						int_2 = len;
					strncpy(result, arg1 + int_2 - 1, int_3);
					result[int_3] = 0;
				}
				return result;
			case UFREST:
				if (int_2 > len)
					int_2 = len + 1;
				strcpy(result, arg1 + int_2 - 1);
				return result;
			case UFISKANJI:
				if (int_2 > len)
					int_2 = len;
				return int_asc(nthctype(arg1, int_2 + 1));
			default:
				return fixnull(0);
			}
		}
		break;
	case TRSTR:
		{
			int tint;

			switch (fun->f_num) {
			case UFCAT:
				strcpy(result, arg1);
				strcat(result, arg2);
				break;
			case UFLEFT:
				{
					int int_2 = asc_int(arg2);

					strncpy(result, arg1, int_2);
					result[int_2] = 0;
				}
				break;
			case UFIND:
				strcpy(result, fixnull(getval(arg1)));
				break;
			case UFUPPER:
				strcpy(result, mkupper(arg1));
				break;
			case UFLOWER:
				strcpy(result, mklower(arg1));
				break;
			case UFTRUTH:
				return ltos(asc_int(arg1) == 42);
			case UFASCII:
				return int_asc(iskanji(tint = *arg1)
				  ? ((tint << 8) + (arg1[1] & CHARMASK)) & WORDMASK : tint);
			case UFCHR:
				tint = asc_int(arg1);
				if (tint < 0x100) {
					result[0] = tint;
					result[1] = 0;
				} else {
					result[0] = (tint >> 8) & CHARMASK;
					result[1] = tint & CHARMASK;
					result[2] = 0;
				}
				break;
			case UFGTCMD:
				cmdstr(getcmd(), result);
				break;
			case UFGTKEY:
				result[0] = tgetc();
				result[1] = 0;
				break;
			case UFSINDEX:
				return int_asc(sindex(arg1, arg2));
			case UFRSINDEX:
				return int_asc(rsindex(arg1, arg2));
			case UFENV:
				return fixnull(dosgetenv(arg1));
			case UFBIND:
				return transbind(arg1);
			case UFEXIST:
				return ltos(fexist(arg1));
			case UFFIND:
				return fixnull(flook(arg1, TRUE));
			case UFXLATE:
				return xlat(arg1, arg2, arg3);
			case UFTRIM:
				return trimstr(arg1);
			case UFLENGTH:
				return int_asc(jstrlen(arg1));
			default:
				return fixnull(0);
			}
			return result;
		}
		break;
	default:
		break;
	}
	meexit(-11);
	return 0;
}

/*
========================================
	ユーザ変数検索
========================================
*/

char *gtusr(char *name)
{
	UVAR *var;

	var = var_in_word_set(name);
	if (var && var->u_value)
		return var->u_value;

	return (char *)errorm;
}

/*
========================================
	環境変数を得る
========================================
*/

char *gtenv(char *vname)
{
	int res;
	const void *var;
	UENVAR *env;
	static char result[NSTRING * 2];

	env = env_in_word_set(vname);
	if (env == 0)
		return (char *)errorm;

	var = env->var;
	switch (env->e_num) {
	case EVFILLCOL:
	case EVCURWIDTH:
	case EVWRAPMODE:
	case EVASAVE:
	case EVACOUNT:
	case EVLASTKEY:
	case EVVBELL:
	case EVSEED:
	case EVGMODE:
	case EVTPAUSE:
	case EVGFLAGS:
	case EVRVAL:
	case EVXPOS:
	case EVYPOS:
	case EVHARDTAB:
	case EVHJUMP:
	case EVCMDCOL:
	case EVCMDARG:
	case EVCMDZENCOL:
	case EVSHBSIZE:
	case EVMENU_LAST_K:
	case EVMENU_CODE:
	case EVMENU_CURSOR:
	case EVMENU_FRAME:
	case EVMENU_HEIGHT:
	case EVMENU_HOMEX:
	case EVMENU_HOMEY:
	case EVMENU_ITEM:
	case EVMENU_ROLL:
	case EVMENU_SELECT:
	case EVMENU_WIDTH:
	case EVMULMAX:
	case EVZCURSOR:
	case EVDLSNAMEPOS:
	case EVBLINK:
	case EVSOFTTABMODE:
	case EVLNUMCOL:
	case EVLSEPCOL:
	case EVCRCOL:
		res = *((int *)var);
		break;
	case EVDEBUG:
	case EVSTATUS:
	case EVEVAL:
	case EVDISCMD:
	case EVDISINP:
	case EVMODEFLAG:
	case EVSSCROLL:
	case EVSSAVE:
	case EVHSCROLL:
	case EVEDPAGE:
	case EVENGLISH:
	case EVSYSKILL:
	case EVDIAGFLAG:
	case EVMSFLAG:
	case EVNEWSCR:
	case EVNOGLOBAL:
	case EVNOKILL:
	case EVMAKBAK:
	case EVDISNUM:
	case EVDISTAB:
	case EVDISZEN:
	case EVIFSPLIT:
	case EVDISPCR:
	case EVFORCEP:
	case EVQUICKEXT:
	case EVQUICKACTBUF:
	case EVQUICKACTCOM:
	case EVQUICKACTFNAME:
	case EVQUICKACTGENE:
	case EVQUICKACTMAC:
	case EVQUICKACTMODE:
	case EVQUICKACTVAR:
	case EVCBUFDIR:
	case EVCHIGH:
	case EVCOLVIS:
	case EVCCASEBUF:
	case EVCCASEC:
	case EVCCASECOM:
	case EVCCASEFNAME:
	case EVCCASEGEN:
	case EVCCASEKEYWORD:
	case EVCCASELATEX:
	case EVCCASEMAC:
	case EVCCASEMODE:
	case EVCCASEVAR:
	case EVCOMPSORT:
	case EVCOMPSORT_DIR:
	case EVCOMPSORT_ICASE:
	case EVMENU_QUICKACT:
	case EVDCDFLAG:
	case EVIGNMETACASE:
	case EVTOKANA:
	case EVCBOLD:
	case EVWRAPEXPAND:
	case EVSSCROLL_SLOW:
	case EVADDEOF:
	case EVOPTSWAP:
	case EVUNIXNL:
	case EVFEPCTRL:
	case EVFEPCTRLESC:
	case EVISEARCHMICRO:
		return ltos(*(int *)var);
	case EVBKIN:
	case EVFKIN:
	case EVSRES:
	case EVPALETTE:
	case EVPALETTE2:
	case EVSEARCH:
	case EVREPLACE:
	case EVLASTMESG:
	case EVIGNEXT:
	case EVMLFORM:
	case EVFLICKER:
	case EVMENU_EXIT:
	case EVMENU_SELECT_K:
	case EVMENU_TITLE:
	case EVTEMP:
	case EVTEXBQUOTE:
	case EVCOMPC:
	case EVCOMPGENERAL:
	case EVCOMPLATEX:
	case EVTRASH:
	case EVLATEXENV:
	case EVDCURDIR:
	case EVDCP:
	case EVDLS:
	case EVDMV:
	case EVDRM:
	case EVHELPLPATH:
	case EVWRAPINDENTHEAD:
	case EVWRAPINDENTITEM:
	case EVWRAPITEM:
	case EVBUFHOOK:
	case EVCMDHK:
	case EVEXBHOOK:
	case EVINSHOOK:
	case EVREADHK:
	case EVWINDHOOK:
	case EVWRAPHK:
	case EVWRITEHK:
	case EVDICTPATH:
		return (char *)var;
	case EVSOFTTAB:
		res = curbp->b_stabs;
		break;
	case EVSCCURCOL:
		{
			int column;

			column = *((int *)var);
			res = (density >= 0) ? column : (column * 6 / 8);
		}
		break;
	case EVSCCURLINE:
		{
			int row;

			row = *((int *)var);
			res = (density >= 0) ? (row >> density) : (row * 12 / 16);
		}
		break;
	case EVDCFNAME:
		return fixnull(dired_get_target_filename(curwp->w_dotp));
		break;
	case EVPAGELEN:
		res = term.t_nrow + 1;
		break;
	case EVCURCOL:
		res = getccol(FALSE);
		break;
	case EVCURLINE:
		res = getcline();
		break;
	case EVRAM:
		res = envram ? malloc_sbrk_unused : malloc_sbrk_used;
		break;
	case EVCBFLAGS:
		res = curbp->b_flag;
		break;
	case EVCBUFNAME:
		return curbp->b_bname;
	case EVCFNAME:
		return curbp->b_fname;
	case EVCURCHAR:
		res = lgetc2(curwp->w_dotp, curwp->w_doto);
		if (iskanji(res))
			res = (res << 8) + lgetc(curwp->w_dotp, curwp->w_doto + 1);
		break;
	case EVCURWORD:
		return getword();
	case EVVERSION:
		return VERSION " (" RELEASE ")";
	case EVPROGNAME:
		return PROGNAME;
	case EVLANG:
		return "JAPANESE";
	case EVWLINE:
		res = curwp->w_ntrows;
		break;
	case EVCWLINE:
		res = getwpos();
		break;
	case EVTARGET:
		saveflag = lastflag;
		res = curgoal;
		break;
	case EVTIME:
		return timeset();
	case EVMATCH:
		return fixnull(patmatch);
	case EVKILL:
		return getkill(result);
	case EVREGION:
		return getreg();
	case EVCMODE:
		res = curbp->b_mode;
		break;
	case EVPENDING:
		return ltos(typahead());
	case EVLWIDTH:
		res = llength(curwp->w_dotp);
		break;
	case EVLINE:
		return getctext();
	case EVSTERM:
		cmdstr(sterm, result);
		return result;
	case EVFCOL:
		res = curwp->w_fcol;
		break;
	case EVLOCALMAP:
		res = curbp->b_keymap;
		break;
	case EVBUFNUM:
		res = getbufnum();
		break;
	case EVBUFPOS:
		res = getbufpos();
		break;
	case EVWINDNUM:
		res = getwindnum();
		break;
	case EVWINDPOS:
		res = getwindpos();
		break;
	case EVDMKNUM:
		res = marknum(lforw(bdiredp->b_linep));
		break;
	case EVCURDIR:
		getwd(result);
		return result;
	case EVLOCALTAB:
		res = curbp->b_tabs;
		break;
	case EVLOCALMLFORM:
		strcpy(result, curbp->b_mlform);
		return result;
	case EVLOCALVAR:
		result[0] = 0;
		if (curbp->b_localvar)
			strcpy (result, curbp->b_localvar);
		return result;
	case EVCURFIG:
		get_curfig(result, cur_pat, 16);
		result[32] = '/';
		get_curfig(result + 33, cur_pat_h, 8);
		result[49] = '/';
		get_curfig(result + 50, cur_pat_6x12, 12);
		return result;
	case EVFEPMODE:
		return ltos(curbp->b_fep_mode);
	case EVLEDMODE:
		res = (B_SFTSNS () >> 8) & 0x7f;
		break;
	case EVEMPTEXT:
		return ltos (curbp->b_emp_text);
	case EVCOMPKEYWORD:
		strcpy (result, (curbp->b_comp_keyword) ? /* curbp->b_comp_keyword */ : "");
		return result;
	case EVCOMPKEYWORD_SET:
		strcpy (result, (curbp->b_comp_keyword_set) ? /* curbp->b_comp_keyword_set */ : "");
		return result;
	default:
		meexit(-12);
		return 0;
	}
	return int_asc(res);
}

/*
========================================
	$kill を得る
========================================
*/

static char *getkill(char *buf)
{
	if (kbufh == 0)
		*buf = 0;
	else {
		int size;

		size = (kused < NSTRING) ? kused : NSTRING - 1;
		strncpy(buf, kbufh->d_chunk, size);
		buf[size] = 0;
	}
	return buf;
}

/*
========================================
	BUFFER の数を得る
========================================
*/

static int getbufnum(void)
{
	int n;
	BUFFER *bp;

	for (bp = bheadp, n = 0; bp; n++, bp = bp->b_bufp) {
		if (bp->b_flag & BFINVS)
			n--;
	}
	return n;
}

/*
========================================
	BUFFER の場所を得る
========================================
*/

static int getbufpos(void)
{
	int n;
	BUFFER *bp;

	for (bp = bheadp, n = 1; bp != curbp; n++, bp = bp->b_bufp) {
		if (bp->b_flag & BFINVS)
			n--;
	}
	return n;
}

/*
========================================
	WINDOW の数を得る
========================================
*/

static int getwindnum(void)
{
	int n;
	WINDOW *wp;

	for (wp = wheadp, n = 0; wp; n++, wp = wp->w_wndp);
	return n;
}

/*
========================================
	WINDOW の場所を得る
========================================
*/

static int getwindpos(void)
{
	int n;
	WINDOW *wp;

	for (wp = wheadp, n = 1; wp != curwp; n++, wp = wp->w_wndp);
	return n;
}

/*
========================================
	余分な空白を削除
========================================
*/

static char *trimstr(char *s)
{
	char *sp;

	for (sp = s + strlen(s) - 1; sp >= s; sp--) {
		if (*sp == 0x40 && sp[-1] == 0x81)
			sp--;
		else if (*sp != ' ' && *sp != TAB)
			break;
	}
	sp[1] = 0;
	return s;
}

/*
----------------------------------------
	変数に値を設定
----------------------------------------
*/

int setvar(int f, int n)
{
	char var[NVSIZE + 1], value[NSTRING];

	if (clexec == FALSE) {
		char *varp;

		varp = complete(KTEX51, 0, 0, CMP_VARIABLE, NVSIZE + 1);
		if (varp == 0)
			return FALSE;
		strcpy(var, varp);
	} else
		execstr = token(execstr, var, NVSIZE + 1);

	{
		VDESC vd;

		findvar(var, &vd, NVSIZE + 1);
		switch (vd.v_type) {
		case -1:
			mlwrite(KTEX52, var);
			return FALSE;
		case -2:
			mlwrite(KTEX267);
			return FALSE;
		}
		if (f == TRUE)
			strcpy(value, int_asc(n));
		else {
			int status;

			his_disable();
			status = mlreply(KTEX53, value, NSTRING);
			if (status != TRUE)
				return status;
		}

		{
			int status;

			status = svar(&vd, value);
			if (macbug && (strcmp(var, "%track") != 0)) {
				sprintf(outline, "(((%s <- %s)))", var, value);
				makelit(outline);
				mlforce(outline);
				update(TRUE);
				if (getkey() == abortc) {
					mlforce(KTEX54);
					return FALSE;
				}
			}
			return status;
		}
	}
}

/*
========================================
	変数のタイプ、名前を得る
========================================
*/

int findvar(char *name, VDESC *vd, int size)
{
	int vnum, vtype;
	void *varp;

fvar:

	vnum = 0;
	vtype = -1;
	varp = 0;

	switch (*name) {
	case '$':
		{
			UENVAR *env;

			env = env_in_word_set(name + 1);
			if (env) {
				vtype = TKENV;
				vnum = env->e_num;
				varp = (void *)env->var;
			}
		}
		break;
	case '%':
		{
			UVAR *var;

			vtype = TKVAR;

			var = var_in_word_set(name + 1);
			if (var)
				varp = &var->u_value;
			else {
				var = create_var(name + 1);
				if (var)
					varp = &var->u_value;
				else
					vtype = -2;
			}
		}
		break;
	case '&':
		if (strncmp(name + 1, "ind", 3) == 0) {
			execstr = token(execstr, name, size);
			strcpy(name, fixnull(getval(name)));
			goto fvar;
		}
		break;
	}
	vd->v_num = vnum;
	vd->v_type = vtype;
	vd->var = varp;
	return TRUE;
}

/*
========================================
	変数設定本体
========================================
*/

int svar(VDESC *var, char *value)
{
	void *varp;
	extern int leftmargin;

	varp = var->var;
	switch (var->v_type) {
	case TKVAR:
		{
			char *sp;

			if (*(char **)varp)
				free(*(char **)varp);
			sp = (char *)malloc(strlen(value) + 1);
			if (sp == 0)
				return FALSE;
			strcpy(sp, value);
			*(char **)varp = sp;
		}
		return TRUE;
	case TKENV:
		{
			int status, intval, logval;
			int n;

			status = TRUE;
			intval = asc_int(value);
			logval = stol(value);
			switch (var->v_num) {
			case EVRAM:
			case EVASAVE:
			case EVACOUNT:
			case EVLASTKEY:
			case EVVBELL:
			case EVSEED:
			case EVGMODE:
			case EVGFLAGS:
			case EVXPOS:
			case EVYPOS:
			case EVCMDARG:
			case EVSHBSIZE:
			case EVMENU_LAST_K:
			case EVMENU_CODE:
			case EVMENU_CURSOR:
			case EVMENU_FRAME:
			case EVMENU_HEIGHT:
			case EVMENU_HOMEX:
			case EVMENU_HOMEY:
			case EVMENU_ITEM:
			case EVMENU_ROLL:
			case EVMENU_SELECT:
			case EVMENU_WIDTH:
			case EVMULMAX:
			case EVDLSNAMEPOS:
			case EVBLINK:
			case EVSOFTTABMODE:
				*(int *)varp = intval;
				break;
			case EVREPLACE:
			case EVLASTMESG:
			case EVTEMP:
			case EVDICTPATH:
				strcpy((char *)varp, value);
				break;
			case EVDEBUG:
			case EVSTATUS:
			case EVEVAL:
			case EVEDPAGE:
			case EVENGLISH:
			case EVDISCMD:
			case EVDISINP:
			case EVSSCROLL:
			case EVSSAVE:
			case EVSYSKILL:
			case EVDIAGFLAG:
			case EVMAKBAK:
			case EVIFSPLIT:
			case EVNEWSCR:
			case EVNOGLOBAL:
			case EVNOKILL:
			case EVFORCEP:
			case EVQUICKEXT:
			case EVQUICKACTBUF:
			case EVQUICKACTCOM:
			case EVQUICKACTFNAME:
			case EVQUICKACTGENE:
			case EVQUICKACTMAC:
			case EVQUICKACTMODE:
			case EVQUICKACTVAR:
			case EVCBUFDIR:
			case EVCCASEBUF:
			case EVCCASEC:
			case EVCCASECOM:
			case EVCCASEFNAME:
			case EVCCASEGEN:
			case EVCCASEKEYWORD:
			case EVCCASELATEX:
			case EVCCASEMAC:
			case EVCCASEMODE:
			case EVCCASEVAR:
			case EVCOMPSORT:
			case EVCOMPSORT_DIR:
			case EVCOMPSORT_ICASE:
			case EVMENU_QUICKACT:
			case EVDCDFLAG:
			case EVIGNMETACASE:
			case EVTOKANA:
			case EVCBOLD:
			case EVWRAPEXPAND:
			case EVSSCROLL_SLOW:
			case EVADDEOF:
			case EVUNIXNL:
			case EVFEPCTRL:
			case EVFEPCTRLESC:
			case EVISEARCHMICRO:
				*(int *)varp = logval;
				break;
			case EVBUFHOOK:
			case EVCMDHK:
			case EVEXBHOOK:
			case EVINSHOOK:
			case EVREADHK:
			case EVWINDHOOK:
			case EVWRAPHK:
			case EVWRITEHK:
				n = NBUFN - 1;
				goto str_common;
			case EVWRAPINDENTHEAD:
			case EVWRAPINDENTITEM:
				n = NIHEAD - 1;
				goto str_common;
			case EVCOMPC:
			case EVCOMPGENERAL:
			case EVCOMPLATEX:
			case EVLATEXENV:
			case EVTRASH:
			case EVHELPLPATH:
				n = NFILEN - 1;
				goto str_common;
			case EVBKIN:
			case EVFKIN:
			case EVIGNEXT:
			case EVWRAPITEM:
				n = NCHKSTR - 1;
				goto str_common;
			case EVDCP:
			case EVDLS:
			case EVDMV:
			case EVDRM:
				n = NDCMD - 1;
				goto str_common;
			case EVMENU_TITLE:
				n = NMTITLE - 1;
				goto str_common;
			case EVMENU_EXIT:
			case EVMENU_SELECT_K:
				n = NMKEY - 1;
				goto str_common;
			case EVTEXBQUOTE:
				n = NTQUOTE - 1;
			str_common:
				strncpy((char *)varp, value, n);
				((char *)varp)[n] = 0;
				break;
			case EVOPTSWAP:
				*(int *)varp = logval;
				opt1key = (logval != TRUE) ? 4 : 8;
				opt2key = (logval != TRUE) ? 8 : 4;
				break;
			case EVFILLCOL:
				if (intval < 8)
					intval = 8;
				*(int *)varp = intval;
					break;
			case EVPAGELEN:
				status = newsize(TRUE, intval);
				break;
			case EVCURCOL:
				status = setccol(intval);
				break;
			case EVCURLINE:
				status = gotoline(TRUE, intval);
				break;
			case EVFLICKER:
				break;
			case EVCURWIDTH:
				status = newwidth(TRUE, intval);
				break;
			case EVCBFLAGS:
				curbp->b_flag = (curbp->b_flag & ~(BFCHG | BFINVS)) | (intval & (BFCHG | BFINVS));
				upmode();
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
				status = H68cres(value);
				break;
			case EVWRAPMODE:
				breakmode = intval & 01;
				break;
			case EVPALETTE:
				strncpy(palstr, value, 48);
				spal(palstr);
				break;
			case EVPALETTE2:
				strncpy(palstr2, value, 48);
				spal2(palstr2);
				break;
			case EVCURCHAR:
				forwdel(FALSE, 1);
				if (intval == I_NEWLINE)
					lnewline();
				else {
					if (intval & 0xff00)
						linsert(1, intval >> 8);
					linsert(1, intval & CHARMASK);
				}
				backchar(FALSE, 1);
				break;
			case EVWLINE:
				status = resize(TRUE, intval);
				break;
			case EVCWLINE:
				status = forwline(TRUE, intval - getwpos());
				break;
			case EVTARGET:
				curgoal = intval;
				thisflag = saveflag;
				break;
			case EVSEARCH:
				strcpy(pat, value);
				setjtable(pat);
				mcclear();
				break;
			case EVCMODE:
				curbp->b_mode = intval;
				curwp->w_flag |= WFMODE;
				break;
			case EVTPAUSE:
				term.t_pause = intval;
				break;
			case EVLINE:
				putctext(value);
				break;
			case EVSTERM:
				sterm = stock(value);
				break;
			case EVMODEFLAG:
				modeflag = logval;
				upwind();
				break;
			case EVHARDTAB:
				tabsize = intval;
				if (tabsize <= 0)
					tabsize = 1;
				upwind();
				break;
			case EVSOFTTAB:
				if (intval < 0)
					intval = 0;
				curbp->b_stabs = intval;
				upwind();
				break;
			case EVFCOL:
				curwp->w_fcol = intval;
				if (curwp->w_fcol < 0)
					curwp->w_fcol = 0;
				curwp->w_flag |= WFHARD | WFMODE;
				break;
			case EVHSCROLL:
				hscroll = logval;
				lbound = 0;
				break;
			case EVHJUMP:
				hjump = intval;
				if (hjump < 1)
					hjump = 1;
				if (hjump > term.t_ncol - 1)
					hjump = term.t_ncol - 1;
				break;
			case EVMSFLAG:
				mouseflag = logval;
				if (mouseflag)
					MS_CURON();
				else
					MS_CUROF();
				break;
			case EVCMDCOL:
			case EVCMDZENCOL:
				*(int *)varp = intval;
				mlerase();
				break;
			case EVDISNUM:
				disnum = logval;
				leftmargin = logval ? 6 : 0;
				upwind();
				break;
			case EVDISPCR:
				dispcr = logval;
				upwind();
				break;
			case EVDISTAB:
				distab = logval;
				upwind();
				break;
			case EVDISZEN:
				diszen = logval;
				upwind();
				break;
			case EVCHIGH:
				chigh = logval;
				upwind();
				break;
			case EVCOLVIS:
				colvis = logval;
				upwind();
				break;
			case EVZCURSOR:
				zcursor = intval;
				TXcurof();
				if (zcursor > 0)
					TXcuron();
				break;
			case EVMLFORM:
				strncpy(mlform, value, NMLFORM - 1);
				mlform[NMLFORM - 1] = 0;
				break;
			case EVLOCALMLFORM:
				strncpy(curbp->b_mlform, value, NMLFORM - 1);
				curbp->b_mlform[NMLFORM - 1] = 0;
				{
					WINDOW *wp;
					char *ml;

					timeflag = memrflag = dateflag = FALSE;
					for(wp = wheadp; wp; wp = wp->w_wndp) {
						ml = wp->w_bufp->b_mlform;
						if (sindex(ml, "?t") || sindex(ml, "?T"))
							timeflag = TRUE;
						if (sindex(ml, "?r") || sindex(ml, "?R"))
							memrflag = TRUE;
						if (sindex(ml, "?d") || sindex(ml, "?D"))
							dateflag = TRUE;
					}
				}
				upmode();
				break;
			case EVLOCALMAP:
				curbp->b_keymap = intval % NKEYMAPS;
				break;
			case EVLOCALTAB:
				if (intval < 1)
					intval = 1;
				curbp->b_tabs = intval;
				upwind();
				break;
			case EVLOCALVAR:
				{
					char *p;

					p = malloc (strlen (value) + 1);
					if (p)
						strcpy (p, value);
					curbp->b_localvar = p;
				}
				break;
			case EVCURFIG:
				if (strlen(value) >= 49) {
					int cur_flag;

					cur_flag = H68curoff();
					set_curfig(value, cur_pat, 16);
					set_curfig(value + 33, cur_pat_h, 8);
					set_curfig(value + 50, cur_pat_6x12, 12);
					H6_CURSET ();
					if (cur_flag)
						H68curon();
				}
				break;
			case EVFEPMODE:
				if (logval)
					fep_force_on();
				else
					fep_force_off();
				curbp->b_fep_mode = iskmode();
				break;
			case EVLEDMODE:
				{
					int i, led;

					led = intval;
					for (i = 0; i < 7; i++) {
						if (i != 4)
							LEDMOD (i, (led & 1));
						led = led >> 1;
					}
				}
				break;
			case EVLNUMCOL:
				lnumcol = intval;
				upwind();
				break;
			case EVLSEPCOL:
				lsepcol = intval;
				upwind();
				break;
			case EVCRCOL:
				crcol = intval;
				upwind();
				break;
			case EVEMPTEXT:
				curbp->b_emp_text = logval;
				upwind ();
				break;
			case EVCOMPKEYWORD:
			case EVCOMPKEYWORD_SET:
				{
					char *p;
					char **dest;

					dest = (var->v_num == EVCOMPKEYWORD) ?
					         &curbp->b_comp_keyword : &curbp->b_comp_keyword_set;

					*dest = p = realloc (*dest, strlen (value) + 1);
					if (p)
						strcpy (p, value);
					else {
						mlwrite (KTEX94);

						return FALSE;
					}
				}
				break;
			case EVVERSION:
			case EVPROGNAME:
			case EVLANG:
			case EVTIME:
			case EVMATCH:
			case EVKILL:
			case EVREGION:
			case EVPENDING:
			case EVLWIDTH:
			case EVRVAL:
			case EVCURWORD:
			case EVSCCURCOL:
			case EVSCCURLINE:
			case EVBUFNUM:
			case EVBUFPOS:
			case EVWINDNUM:
			case EVWINDPOS:
			case EVDCURDIR:
			case EVDMKNUM:
			case EVCURDIR:
			case EVDCFNAME:
				break;
			default:
				status = FALSE;
			}
			return status;
		}
		break;
	default:
		break;
	}
	return TRUE;
}

/*
========================================
	文字列を整数に変換
========================================
*/

int asc_int(char *st)
{
	int result, sign;
	char c;

	result = 0;
	sign = 0;
	while (1) {
		if (*st == 0x81 && st[1] == 0x40)
			st += 2;
		else if (*st == ' ' || *st == TAB)
			st++;
		else
			break;
	}

	if (*st == '-') {
		sign = -1;
		st++;
	} else if (*st == '+')
		st++;

	if (*st == '0' && (st[1] == 'x' || st[1] == 'X')) {
		st += 2;
		while (c = *st++) {
			int tmp;

			if (c >= '0' && c <= '9')
				tmp = c - '0';
			else if (c >= 'a' && c <= 'f')
				tmp = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				tmp = c - 'A' + 10;
			else
				break;
			result = (result << 4) + tmp;
		}
		return sign ? -result : result;
	}
	while (c = *st++) {
		if (c >= '0' && c <= '9')
			result = result * 10 + c - '0';
		else
			break;
	}
	return sign ? -result : result;
}

/*
========================================
	整数を文字列に変換
========================================
*/

char *int_asc(int i)
{
	int digit, sign;
	char *sp;
	static char result[INTWIDTH + 1];

	sign = 1;
	if (i < 0) {
		sign = -1;
		i = -i;
	}
	*(sp = result + INTWIDTH) = 0;
	do {
		digit = i % 10;
		*(--sp) = '0' + digit;
		i /= 10;
	}
	while (i);

	if (sign < 0)
		*(--sp) = '-';
	return sp;
}

/*
========================================
	トークンのタイプを得る
========================================
*/

int gettyp(char *token)
{
	int c;
	char *p;
	static const char tyc[] = "\"!@?:#$%&*.";
	static const int typ[] =
	{
		TKSTR, TKDIR, TKARG, TKARGF, TKARGB, TKBUF,
		TKENV, TKVAR, TKFUN, TKLBL, TKCOMP
	};

	c = *token;
	if (c == 0)
		return TKNUL;
	else if (c >= '0' && c <= '9')
		return TKLIT;
	else if (c == '-' && (token[1] >= '0' && token[1] <= '9'))
		return TKLIT;
	p = strchr(tyc, c);
	return p ? typ[p - tyc] : TKCMD;
}

/*
========================================
	トークンの値を得る
========================================
*/

char *getval(char *token)
{
	int type;
	static char buf[NSTRING];

	switch (type = gettyp(token)) {
	case TKNUL:
		return fixnull(0);
	case TKARG:
		{
			int status, distmp = discmd;

			strcpy(token, fixnull(getval(token + 1)));
			discmd = TRUE;
			his_enable(bhisargp);
			status = getstring(token, buf, NSTRING, ctoec('\r'));
			discmd = distmp;
			return (status == ABORT) ? 0 : buf;
		}
	case TKARGF:
	case TKARGB:
	case TKCOMP:
		{
			int discle = clexec, distmp = discmd;
			char *inp = 0;

			strcpy(token, fixnull(getval(token + 1)));
			discmd = TRUE;
			clexec = FALSE;
			switch (type) {
			case TKARGF:
				inp = gtfilename(token);
				break;
			case TKARGB:
				inp = complete(token, 0, 0, CMP_BUFFER, NBUFN);
				break;
			case TKCOMP:
				inp = complete(token, 0, 0, CMP_GENERAL, NSTRING);
			}
			discmd = distmp;
			clexec = discle;
			return inp ? (strcpy(buf, inp), buf) : 0;
		}
	case TKBUF:
		{
			int blen;
			BUFFER *bp;

			strcpy(token, fixnull(getval(token + 1)));
			bp = bfind(token, FALSE, 0);
			if (bp == 0)
				return 0;
			if (bp->b_nwnd > 0) {
				curbp->b_dotp = curwp->w_dotp;
				curbp->b_doto = curwp->w_doto;
			}
			if (bp->b_linep == bp->b_dotp)
				return 0;
			blen = llength(bp->b_dotp) - bp->b_doto;
			if (blen > NSTRING)
				blen = NSTRING;
			strncpy(buf, bp->b_dotp->l_text + bp->b_doto, blen);
			buf[blen] = 0;
			bp->b_dotp = bp->b_dotp->l_fp;
			bp->b_doto = 0;
			if (bp->b_nwnd > 0) {
				curwp->w_dotp = curbp->b_dotp;
				curwp->w_doto = 0;
				curwp->w_flag |= WFMOVE;
			}
		}
		return buf;
	case TKVAR:
		return gtusr(token + 1);
	case TKENV:
		return gtenv(token + 1);
	case TKFUN:
		return gtfun(token + 1);
	case TKDIR:
	case TKLBL:
		return 0;
	case TKLIT:
	case TKCMD:
		return token;
	case TKSTR:
		return token + 1;
	}
	return 0;
}

/*
========================================
	文字列を論理数に変換
========================================
*/

int stol(char *val)
{
	int	x;

	x = tolower(*val);
	if (x == 'f')
		return FALSE;
	if (x == 't')
		return TRUE;
	if (asc_int(val))
		return TRUE;

	return FALSE;
}

/*
========================================
	文字列を大文字に変換
========================================
*/

static char *mkupper(char *str)
{
	char *sp;

	for (sp = str; *sp; sp++) {
		if (iskanji(*sp))
			sp++;
		else if (islower(*sp))
			*sp -= DIFCASE;
	}
	return str;
}

/*
========================================
	文字列を小文字に変換
========================================
*/

static char *mklower(char *str)
{
	char *sp;

	for (sp = str; *sp; sp++) {
		if (iskanji(*sp))
			sp++;
		else if (isupper(*sp))
			*sp += DIFCASE;
	}
	return str;
}

/*
========================================
	文字列サーチ
========================================
*/

int sindex(char *buf, char *pattern)
{
	int bpos = 0;

	for (bpos = 0; buf[bpos]; bpos++) {
		int cpos, spos;

		if (buf[bpos] != *pattern)
			continue;
		for (cpos = bpos, spos = 0; pattern[spos] && buf[cpos]; cpos++, spos++) {
			if (buf[cpos] != pattern[spos])
				break;
		}
		if (pattern[spos] == 0) {
			if (nthctype(buf, bpos + 1) != CT_KJ2)
				return bpos + 1;
		}
	}
	return 0;
}

/*
========================================
	逆文字列サーチ
========================================
*/

static int rsindex(char *buf, char *pattern)
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
========================================
	変換テーブルを通す
========================================
*/

static char *xlat(char *source, char *lookup, char *trans)
{
	char cs, cl, *sp, *rp;
	static char result[NSTRING];

	rp = result;
	sp = source;
	while (cs = *sp++) {
		char *lp;

		lp = lookup;
		while (cl = *lp++) {
			if (cs == cl) {
				cs = trans[lp - lookup - 1];
				break;
			}
		}
		*rp++ = cs;
	}
	*rp = 0;
	return result;
}

/*
----------------------------------------
	変数の値を表示
----------------------------------------
*/

int dispvar(int f, int n)
{
	VDESC vd;
	char var[NVSIZE + 1];

	if (clexec == FALSE) {
		char *varp;

		varp = complete(KTEX55, 0, 0, CMP_VARIABLE, NVSIZE + 1);
		if (varp == 0 || *varp == 0)
			return FALSE;
		strcpy(var, varp);
	} else
		execstr = token(execstr, var, NVSIZE + 1);

	findvar(var, &vd, NVSIZE + 1);
	if (vd.v_type == (char)-1) {
		mlwrite(KTEX52, var);
		return FALSE;
	}
	sprintf(outline, "%s = %s", var, fixnull(getval(var)));
	makelit(outline);
	mlforce(outline);
	update(TRUE);
	return TRUE;
}

/*
----------------------------------------
	変数リストを作る
----------------------------------------
*/

int desvars(int f, int n)
{
	char outseq[NSTRING * 2];
	BUFFER *bp, *savebp;

	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;
	bp = bfind(KTEX56, TRUE, 0);
	if (bp == 0 || bclear(bp) == FALSE) {
		mlwrite(KTEX57);
		return FALSE;
	}
	mlwrite(KTEX58);

	savebp = curbp;
	swbuffer(bp);

	{
		int i;

		for (i = 0; i < nevars; i++) {
			*outseq = '$';
			strcpy(outseq + 1, env_table[i].e_name);
			pad(outseq, 28);
			swbuffer (savebp);
			strcat(outseq, gtenv(env_table[i].e_name));
			swbuffer (bp);
			if (putline(outseq) != TRUE)
				return FALSE;
		}
		linstr(I_NEWSTR I_NEWSTR);
	}
	{
		UVAR *var;

		for(var = uv_head; var; var = var->u_next) {
			if (n == 1 && var->u_name[0] == '%')
				continue;

			*outseq = '%';
			strcpy(outseq + 1, var->u_name);
			pad(outseq, 28);
			strcat(outseq, fixnull(var->u_value));
			if (putline(outseq) != TRUE)
				return FALSE;
		}
	}

	winbob(bp);

	curwp->w_bufp->b_mode |= MDVIEW;
	bp->b_flag &= ~BFCHG;

	upmode();
	mlerase();
	return TRUE;
}

/*
----------------------------------------
	関数リストを作る
----------------------------------------
*/

int desfunc(int f, int n)
{
	char outseq[NSTRING * 2];
	BUFFER *bp;

	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;
	bp = bfind(KTEX211, TRUE, 0);
	if (bp == 0 || bclear(bp) == FALSE) {
		mlwrite(KTEX212);
		return FALSE;
	}
	mlwrite(KTEX213);

	swbuffer(bp);

	{
		int uindex;

		for (uindex = 0; uindex < nfuncs; uindex++) {
			strcpy(outseq, "&");
			strcat(outseq, func_name[uindex]);
			if (putline(outseq) != TRUE)
				return FALSE;
		}
	}

	winbob(bp);

	curwp->w_bufp->b_mode |= MDVIEW;
	bp->b_flag &= ~BFCHG;

	upmode();
	mlerase();
	return TRUE;
}

/*
========================================
	文字列の長さを調整
========================================
*/

static void pad(char *s, int len)
{
	int clen = strlen(s);

	while (clen < len) {
		strcpy(s + clen, "                    ");
		clen += 20;
	}
	s[len] = 0;
}

/*
----------------------------------------
	get word
----------------------------------------
*/

static char *getword(void)
{
	int pos;
	char *p;
	static char result[NSTRING];

	pos = curwp->w_doto;
	p = curwp->w_dotp->l_text;

	if (pos > 0) {
		pos--;
		if ((!iscsym(p[pos]) && p[pos] != '.') || nthctype(p, pos + 1) == CT_KJ2)
			pos++;
	}
	while (pos >= 0) {
		if (iscsym(p[pos]) || p[pos] == '.') {
			pos--;
			if (nthctype(p, pos + 1) == CT_KJ2)
				break;
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

/*
========================================
	カーソルパターン取得
========================================
*/

static void get_curfig(char *value, char *pat, int size)
{
	int i;

	for(i = 0; i < size; i++)
		sprintf(value + i * 2, "%02x", pat[i]);
}

/*
========================================
	カーソルパターン設定
========================================
*/

static void set_curfig(char *value, char *pat, int size)
{
	int i, c, mask;

	mask = (size == 12) ? 0xfc : 0xff;

	for(i = 0; i < size; i++) {
		sscanf(value + i * 2, "%02x", &c);
		pat[i] = c & mask;
	}
}
