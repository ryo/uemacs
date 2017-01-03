/*
----------------------------------------
	ENVHASH.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "evar.h"

/*
========================================
	RCS id ‚ÌÝ’è
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/envhash.c,v 1.1 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	ŠÖ”ƒe[ƒuƒ‹
========================================
*/

#define NHASH	128

static UVAR *uvar_head[NHASH];
static UENVAR *env_head[NHASH];

UENVAR env_table[] = {
	{NULL,	"acount",				EVACOUNT,			&gacount},
	{NULL,	"add-eof",				EVADDEOF,			&addeof},
	{NULL,	"asave",				EVASAVE,			&gasave},
	{NULL,	"bkin",					EVBKIN,				backnonbreakchar},
	{NULL,	"blink",				EVBLINK,			&blink_count},
	{NULL,	"bufhook",				EVBUFHOOK,			&bufhook},
	{NULL,	"bufnum",				EVBUFNUM,			0},
	{NULL,	"bufpos",				EVBUFPOS,			0},
	{NULL,	"cbflags",				EVCBFLAGS,			0},
	{NULL,	"cbufdir",				EVCBUFDIR,			&cbufdir},
	{NULL,	"cbufname",				EVCBUFNAME,			0},
	{NULL,	"ccase-buffer",			EVCCASEBUF,			&compcase[0]},
	{NULL,	"ccase-c",				EVCCASEC,			&compcase[1]},
	{NULL,	"ccase-command",		EVCCASECOM,			&compcase[2]},
	{NULL,	"ccase-filename",		EVCCASEFNAME,		&compcase[3]},
	{NULL,	"ccase-general",		EVCCASEGEN,			&compcase[4]},
	{NULL,	"ccase-keyword",		EVCCASEKEYWORD,		&compcase[5]},
	{NULL,	"ccase-latex",			EVCCASELATEX,		&compcase[6]},
	{NULL,	"ccase-macro",			EVCCASEMAC,			&compcase[7]},
	{NULL,	"ccase-mode",			EVCCASEMODE,		&compcase[8]},
	{NULL,	"ccase-var",			EVCCASEVAR,			&compcase[9]},
	{NULL,	"cbold",				EVCBOLD,			&cbold},
	{NULL,	"cfname",				EVCFNAME,			0},
	{NULL,	"chigh",				EVCHIGH,			&chigh},
	{NULL,	"cmdarg",				EVCMDARG,			&cmdarg},
	{NULL,	"cmdcol",				EVCMDCOL,			&cmdcol},
	{NULL,	"cmdhook",				EVCMDHK,			&cmdhook},
	{NULL,	"cmdzencol",			EVCMDZENCOL,		&cmdzencol},
	{NULL,	"cmode",				EVCMODE,			0},
	{NULL,	"colvis",				EVCOLVIS,			&colvis},
	{NULL,	"comp-c",				EVCOMPC,			&comp_c},
	{NULL,	"comp-general",			EVCOMPGENERAL,		&comp_gene},
	{NULL,	"comp-keyword",			EVCOMPKEYWORD,		0},
	{NULL,	"comp-keyword-set",		EVCOMPKEYWORD_SET,	0},
	{NULL,	"comp-latex",			EVCOMPLATEX,		&comp_latex},
	{NULL,	"compbuf-sort",			EVCOMPSORT,			&comp_sort},
	{NULL,	"compbuf-sort-dir",		EVCOMPSORT_DIR,		&comp_sort_dir},
	{NULL,	"compbuf-sort-igncase",	EVCOMPSORT_ICASE,	&comp_sort_ignore_case},
	{NULL,	"crcol",				EVCRCOL,			&crcol},
	{NULL,	"curchar",				EVCURCHAR,			0},
	{NULL,	"curcol",				EVCURCOL,			0},
	{NULL,	"curline",				EVCURLINE,			0},
	{NULL,	"current-dir",			EVCURDIR,			0},
	{NULL,	"cursor-figure",		EVCURFIG,			0},
	{NULL,	"curwidth",				EVCURWIDTH,			&term.t_ncol},
	{NULL,	"curword",				EVCURWORD,			0},
	{NULL,	"cwline",				EVCWLINE,			0},
	{NULL,	"debug",				EVDEBUG,			&macbug},
	{NULL,	"diagflag",				EVDIAGFLAG,			&diagflag},
	{NULL,	"dict-path",			EVDICTPATH,			&std_dict_path},
	{NULL,	"dired-cdflag",			EVDCDFLAG,			&dired_cd_flag},
	{NULL,	"dired-cp",				EVDCP,				&dired_cp_cmd},
	{NULL,	"dired-cfname",			EVDCFNAME,			0},
	{NULL,	"dired-current-dir",	EVDCURDIR,			dired_current_dir},
	{NULL,	"dired-ls",				EVDLS,				&dired_ls_cmd},
	{NULL,	"dired-ls-name-pos",	EVDLSNAMEPOS,		&dired_ls_name_pos},
	{NULL,	"dired-marknum",		EVDMKNUM,			&dired_mark_num},
	{NULL,	"dired-mv",				EVDMV,				&dired_mv_cmd},
	{NULL,	"dired-rm",				EVDRM,				&dired_rm_cmd},
	{NULL,	"discmd",				EVDISCMD,			&discmd},
	{NULL,	"disinp",				EVDISINP,			&disinp},
	{NULL,	"disnum",				EVDISNUM,			&disnum},
	{NULL,	"dispcr",				EVDISPCR,			&dispcr},
	{NULL,	"distab",				EVDISTAB,			&distab},
	{NULL,	"diszenspc",			EVDISZEN,			&diszen},
	{NULL,	"edpage",				EVEDPAGE,			&edpage},
	{NULL,  "emphasis-text",		EVEMPTEXT,  		0},
	{NULL,	"english",				EVENGLISH,			&english},
	{NULL,	"eval",					EVEVAL,				&syseval},
	{NULL,	"exbhook",				EVEXBHOOK,			&exbhook},
	{NULL,	"fcol",					EVFCOL,				0},
	{NULL,	"fep-ctrl",				EVFEPCTRL,			&fepctrl},
	{NULL,	"fep-ctrl-esc",			EVFEPCTRLESC,		&fepctrlesc},
	{NULL,	"fep-mode",				EVFEPMODE,			0},
	{NULL,	"fillcol",				EVFILLCOL,			&fillcol},
	{NULL,	"fkin",					EVFKIN,				fornonbreakchar},
	{NULL,	"flicker",				EVFLICKER,			(char *)nsupm},
	{NULL,	"forceproc",			EVFORCEP,			&forceproc},
	{NULL,	"gflags",				EVGFLAGS,			&gflags},
	{NULL,	"gmode",				EVGMODE,			&gmode},
	{NULL,	"hardtab",				EVHARDTAB,			&tabsize},
	{NULL,	"help-load-path",		EVHELPLPATH,		help_load_path},
	{NULL,	"hjump",				EVHJUMP,			&hjump},
	{NULL,	"hscroll",				EVHSCROLL,			&hscroll},
	{NULL,	"ifsplit",				EVIFSPLIT,			&ifsplit},
	{NULL,	"ignext",				EVIGNEXT,			ignoreext},
	{NULL,	"ignmetacase",			EVIGNMETACASE,		&ignmetacase},
	{NULL,	"inshook",				EVINSHOOK,			&inshook},
	{NULL,	"isearch-micro",		EVISEARCHMICRO,		&isearch_micro},
	{NULL,	"kill",					EVKILL,				0},
	{NULL,	"language",				EVLANG,				0},
	{NULL,	"lastkey",				EVLASTKEY,			&lastkey},
	{NULL,	"lastmesg",				EVLASTMESG,			lastmesg},
	{NULL,	"latexenv",				EVLATEXENV,			latex_env},
	{NULL,	"led-mode",				EVLEDMODE,			0},
	{NULL,	"line",					EVLINE,				0},
	{NULL,	"lnumcol",				EVLNUMCOL,			&lnumcol},
	{NULL,	"lsepcol",				EVLSEPCOL,			&lsepcol},
	{NULL,	"localmap",				EVLOCALMAP,			0},
	{NULL,	"localmlform",			EVLOCALMLFORM,		0},
	{NULL,	"localtab",				EVLOCALTAB,			0},
	{NULL,	"localvar",				EVLOCALVAR,			0},
	{NULL,	"lwidth",				EVLWIDTH,			0},
	{NULL,	"makbak",				EVMAKBAK,			&makbak},
	{NULL,	"match",				EVMATCH,			0},
	{NULL,	"menu-code",			EVMENU_CODE,		&menu_code},
	{NULL,	"menu-cursor",			EVMENU_CURSOR,		&menu_cursor},
	{NULL,	"menu-exit",			EVMENU_EXIT,		menu_exit},
	{NULL,	"menu-frame-color",		EVMENU_FRAME,		&menu_frame_color},
	{NULL,	"menu-height",			EVMENU_HEIGHT,		&menu_height},
	{NULL,	"menu-home-x",			EVMENU_HOMEX,		&menu_home_x},
	{NULL,	"menu-home-y",			EVMENU_HOMEY,		&menu_home_y},
	{NULL,	"menu-item-color",		EVMENU_ITEM,		&menu_item_color},
	{NULL,	"menu-last-key",		EVMENU_LAST_K,		&menu_last_key},
	{NULL,	"menu-quickact",		EVMENU_QUICKACT,	&menu_quickact},
	{NULL,	"menu-roll",			EVMENU_ROLL,		&menu_roll},
	{NULL,	"menu-select-color",	EVMENU_SELECT,		&menu_select_color},
	{NULL,	"menu-select-key",		EVMENU_SELECT_K,	menu_select},
	{NULL,	"menu-title",			EVMENU_TITLE,		menu_title},
	{NULL,	"menu-width",			EVMENU_WIDTH,		&menu_width},
	{NULL,	"mlform",				EVMLFORM,			mlform},
	{NULL,	"modeflag",				EVMODEFLAG,			&modeflag},
	{NULL,	"msflag",				EVMSFLAG,			&mouseflag},
	{NULL,	"multimark-max",		EVMULMAX,			&mulmax},
	{NULL,	"newscroll",			EVNEWSCR,			&newscroll},
	{NULL,	"noglobal",				EVNOGLOBAL,			&noglobal},
	{NULL,	"nokill",				EVNOKILL,			&nokill},
	{NULL,	"opt-swap",				EVOPTSWAP,			&optswap},
	{NULL,	"pagelen",				EVPAGELEN,			0},
	{NULL,	"palette",				EVPALETTE,			palstr},
	{NULL,	"palette2",				EVPALETTE2,			palstr2},
	{NULL,	"pending",				EVPENDING,			0},
	{NULL,	"progname",				EVPROGNAME,			0},
	{NULL,	"quickact-buffer",		EVQUICKACTBUF,		&quickact[0]},
	{NULL,	"quickact-command",		EVQUICKACTCOM,		&quickact[2]},
	{NULL,	"quickact-filename",	EVQUICKACTFNAME,	&quickact[3]},
	{NULL,	"quickact-general",		EVQUICKACTGENE,		&quickact[4]},
	{NULL,	"quickact-macro",		EVQUICKACTMAC,		&quickact[7]},
	{NULL,	"quickact-mode",		EVQUICKACTMODE,		&quickact[8]},
	{NULL,	"quickact-var",			EVQUICKACTVAR,		&quickact[9]},
	{NULL,	"quickexit",			EVQUICKEXT,			&quickext},
	{NULL,	"ram",					EVRAM,				&envram},
	{NULL,	"readhook",				EVREADHK,			&readhook},
	{NULL,	"region",				EVREGION,			0},
	{NULL,	"replace",				EVREPLACE,			rpat},
	{NULL,	"rval",					EVRVAL,				&rval},
	{NULL,	"sccurcol",				EVSCCURCOL,			&sccurcol},
	{NULL,	"sccurline",			EVSCCURLINE,		&sccurline},
	{NULL,	"search",				EVSEARCH,			pat},
	{NULL,	"seed",					EVSEED,				&seed},
	{NULL,	"shbsize",				EVSHBSIZE,			&shbufsize},
	{NULL,	"softtab",				EVSOFTTAB,			&stabsize},
	{NULL,	"softtab-mode",			EVSOFTTABMODE,		&stabmode},
	{NULL,	"sres",					EVSRES,				sres},
	{NULL,	"ssave",				EVSSAVE,			&ssave},
	{NULL,	"sscroll",				EVSSCROLL,			&sscroll},
	{NULL,	"sscroll-slow",			EVSSCROLL_SLOW,		&sscroll_slow},
	{NULL,	"syskill",				EVSYSKILL,			&syskill},
	{NULL,	"status",				EVSTATUS,			&cmdstatus},
	{NULL,	"sterm",				EVSTERM,			&sterm},
	{NULL,	"target",				EVTARGET,			&curgoal},
	{NULL,	"temp",					EVTEMP,				system_temp},
	{NULL,	"texbquote",			EVTEXBQUOTE,		texbquote},
	{NULL,	"time",					EVTIME,				0},
	{NULL,	"tokana",				EVTOKANA,			&tokana},
	{NULL,	"tpause",				EVTPAUSE,			&term.t_pause},
	{NULL,	"trash",				EVTRASH,			trash},
	{NULL,	"unix-newline",			EVUNIXNL,			&unix_newline},
	{NULL,	"vbell",				EVVBELL,			&vbell},
	{NULL,	"version",				EVVERSION,			0},
	{NULL,	"windhook",				EVWINDHOOK,			&windhook},
	{NULL,	"windnum",				EVWINDNUM,			0},
	{NULL,	"windpos",				EVWINDPOS,			0},
	{NULL,	"wline",				EVWLINE,			0},
	{NULL,	"wrap-expand",			EVWRAPEXPAND,		&wrapexpand},
	{NULL,	"wrap-indent-head",		EVWRAPINDENTHEAD,	&wrapindenthead},
	{NULL,	"wrap-indent-item",		EVWRAPINDENTITEM,	&wrapindentitem},
	{NULL,	"wrap-item",			EVWRAPITEM,			&wrapitem},
	{NULL,	"wraphook",				EVWRAPHK,			&wraphook},
	{NULL,	"wrapmode",				EVWRAPMODE,			&breakmode},
	{NULL,	"writehook",			EVWRITEHK,			&writehook},
	{NULL,	"xpos",					EVXPOS,				&xpos},
	{NULL,	"ypos",					EVYPOS,				&ypos},
	{NULL,	"zcursor",				EVZCURSOR,			&zcursor},
	{NULL,	NULL,					0,					NULL}
};

int nevars = sizeof(env_table) / sizeof(env_table[0]) - 1;

/*
========================================
	init_env_in_word_set
========================================
*/

void init_env_in_word_set(void)
{
	int i;
	short hashnum;
	const char *p;

	for(i = nevars - 1; i >= 0; i--) {
		for(hashnum = 0, p = env_table[i].e_name; *p; p++)
			hashnum = (hashnum * 71 + *p) & (NHASH - 1);

		env_table[i].e_next = env_head[hashnum];
		env_head[hashnum] = &env_table[i];
	}
}

/*
========================================
	env_in_word_set
========================================
*/

UENVAR *env_in_word_set(const char *name)
{
	UENVAR *env;
	short hashnum;
	const char *p;

	for(hashnum = 0, p = name; *p; p++)
		hashnum = (hashnum * 71 + *p) & (NHASH - 1);

	for(env = env_head[hashnum]; env; env = env->e_next) {
		if (!strcmp(name, env->e_name))
			return env;
	}

	return NULL;
}

/*
========================================
	create_var
========================================
*/

UVAR *create_var(const char *name)
{
	UVAR *var;
	char *vname;

	var = (UVAR *)malloc(sizeof(UVAR));
	vname = (char *)malloc(strlen(name) + 1);
	if (!var || !vname)
		return NULL;

	if (uv_tail) {
		uv_tail->u_next = var;
		uv_tail = var;
	} else
		uv_head = uv_tail = var;

	strcpy(vname, name);
	var->u_next = NULL;
	var->u_name = vname;
	var->u_value = NULL;

	{
		short hashnum;
		const char *p;

		for(hashnum = 0, p = name; *p; p++)
			hashnum = (hashnum * 71 + *p) & (NHASH - 1);

		var->u_hnext = uvar_head[hashnum];
		uvar_head[hashnum] = var;
	}

	return var;
}

/*
========================================
	var_in_word_set
========================================
*/

UVAR *var_in_word_set(const char *name)
{
	UVAR *var;
	short hashnum;
	const char *p;

	for(hashnum = 0, p = name; *p; p++)
		hashnum = (hashnum * 71 + *p) & (NHASH - 1);

	for(var = uvar_head[hashnum]; var; var = var->u_hnext) {
		if (!strcmp(name, var->u_name))
			return var;
	}

	return NULL;
}
