/*
 * $Id: evar.h,v 1.46 2017/01/02 18:22:09 ryo Exp $
 *
 * EVAR.H: Environment and user variable definitions for MicroEMACS
 *
 * written 1986 by Daniel Lawrence
 */

/* structure to hold user variables and their definitions	 */

typedef struct UVAR {
	char u_name[NVSIZE + 1];	/* name of user variable */
	char *u_value;/* value (string) */
} UVAR;

/* current user variables (This structure will probably change)	 */

#define MAXVARS	512

UVAR uv[MAXVARS+1];	/* user variables */

/* list of recognized environment variables	 */

char   *envars[] = {
	"aborthook",		/* abort-command execution hook */
	"acount",		/* # of chars until next auto-save */
	"ansiesc",		/* ansi escape sequence (for color text :P) */
	"asave",		/* # of chars between auto-saves */
	"bom",			/* need BOM */
	"bufhook",		/* enter buffer switch hook */
	"bufnum",		/* number of buffer */
	"bufpos",		/* poistion of buffer */
	"cbflags",		/* current buffer flags */
	"cbufname",		/* current buffer name */
	"cfname",		/* current file name */
	"cmdcol",		/* command color */
	"cmdhook",		/* command loop hook */
	"cmode",		/* mode of current buffer */
	"comment-in",		/* comment start */
	"comment-line",		/* line comment */
	"comment-out",		/* comment end */
	"commentcol",		/* C mode comment color */
	"crchar",		/* CR character */
	"crcol",		/* color of CR */
	"curchar",		/* current character under the cursor */
	"curcol",		/* current column pos of cursor */
	"curlabel",		/* current line label */
	"curline",		/* current line in file */
	"curtext",		/* current text */
	"curwidth",		/* current screen width */
	"curword",		/* current word */
	"cwline",		/* current screen line in window */
	"debug",		/* macro debugging */
	"diagflag",		/* diagonal mouse movements enabled? */
	"discmd",		/* display commands on command line */
	"disinp",		/* display command line input characters */
	"dislabel",		/* display line label */
	"disnum",		/* display line number */
	"dispcr",		/* display CR */
	"dispeof",		/* display EOF */
	"distab",		/* display tab code */
	"diszenspc",		/* display zenkaku space */
	"emphasisbold",		/* emphasis bold type */
	"emphasiscol",		/* emphasis color */
	"eofcol",		/* color of EOF */
	"eofreturn",		/* need return at EOF */
	"exbhook",		/* exit buffer switch hook */
	"fcol",			/* first displayed column in curent window */
	"fillcol",		/* current fill column */
	"flicker",		/* flicker supression */
	"funccol",		/* C mode function color */
	"gflags",		/* global internal emacs flags */
	"gmode",		/* global modes */
	"hardtab",		/* current hard tab size */
	"hjump",		/* horizontal screen jump size */
	"hscroll",		/* horizontal scrolling flag */
	"ifsplit",
	"kanjicode",		/* Input/Output kanji code */
	"kanjidisp",		/* display kanji code */
	"kanjiinp",		/* input kanji code */
	"kbdmacro",		/* playing in KBD-macro */
	"kill",			/* kill buffer (read only) */
	"language",		/* language of text messages */
	"lastkey",		/* last keyboard char struck */
	"lastmesg",		/* last string mlwrite()ed */
	"line",			/* text of current line */
	"localmlcol",		/* buffer local mlform color */
	"localmlform",		/* buffer local mlform */
	"lwidth",		/* width of current line */
	"makbak",		/* make backup file */
	"match",		/* last matched magic pattern */
	"mlcol",		/* modeline color */
	"mlform",		/* modeline format */
	"modeflag",		/* Modelines displayed flag */
	"modifyhook",		/* modify buffer hook */
	"msflag",		/* activate mouse? */
	"newlinehook",		/* newlne execution hook */
	"numcol",		/* color of line number*/
	"oldcrypt",		/* use old(broken) encryption */
	"pagelen",		/* number of lines used by editor */
	"palette",		/* current palette string */
	"pending",		/* type ahead pending flag */
	"postreadhook",		/* read file execution hook */
	"postwritehook",	/* write file hook */
	"progname",		/* returns current prog name - "MicroEMACS" */
	"quotecol",		/* single/doublequote string color */
	"ram",			/* ram in use by malloc */
	"readhook",		/* read file execution hook */
	"region",		/* current region (read only) */
	"replace",		/* replacement pattern */
	"rval",			/* child process return value */
	"search",		/* search pattern */
	"seed",			/* current random number seed */
	"softtab",		/* current soft tab size */
	"sres",			/* current screen resolution */
	"ssave",		/* safe save flag */
	"sscroll",		/* smooth scrolling flag */
	"status",		/* returns the status of the last command */
	"sterm",		/* search terminator character */
	"tabcol",		/* TAB color */
	"target",		/* target for line moves */
	"time",			/* date and time */
	"tpause",		/* length to pause for paren matching */
	"version",		/* current version number */
	"windnum",
	"windpos",
	"wline",		/* # of lines in current window */
	"wraphook",		/* wrap word execution hook */
	"writehook",		/* write file hook */
	"xpos",			/* current mouse X position */
	"ypos"			/* current mouse Y position */
};

#define NEVARS	(sizeof(envars)/sizeof(char *))

/* and its preprocesor definitions */

enum EVVAR {
	EVABORTHOOK,
	EVACOUNT,
	EVANSIESC,
	EVASAVE,
	EVBOM,
	EVBUFHOOK,
	EVBUFNUM,
	EVBUFPOS,
	EVCBFLAGS,
	EVCBUFNAME,
	EVCFNAME,
	EVCMDCOL,
	EVCMDHOOK,
	EVCMODE,
	EVCOMMENT_IN,
	EVCOMMENT_LINE,
	EVCOMMENT_OUT,
	EVCOMMENTCOL,
	EVCRCHAR,
	EVCRCOL,
	EVCURCHAR,
	EVCURCOL,
	EVCURLABEL,
	EVCURLINE,
	EVCURTEXT,
	EVCURWIDTH,
	EVCURWORD,
	EVCWLINE,
	EVDEBUG,
	EVDIAGFLAG,
	EVDISCMD,
	EVDISINP,
	EVDISLABEL,
	EVDISNUM,
	EVDISPCR,
	EVDISPEOF,
	EVDISTAB,
	EVDISZENSPC,
	EVEMPHASISBOLD,
	EVEMPHASISCOL,
	EVEOFCOL,
	EVEOFRETURN,
	EVEXBHOOK,
	EVFCOL,
	EVFILLCOL,
	EVFLICKER,
	EVFUNCCOL,
	EVGFLAGS,
	EVGMODE,
	EVHARDTAB,
	EVHJUMP,
	EVHSCROLL,
	EVIFSPLIT,
	EVKANJICODE,
	EVKANJIDISP,
	EVKANJIINP,
	EVKBDMACRO,
	EVKILL,
	EVLANG,
	EVLASTKEY,
	EVLASTMESG,
	EVLINE,
	EVLOCALMLCOL,
	EVLOCALMLFORM,
	EVLWIDTH,
	EVMAKBAK,
	EVMATCH,
	EVMLCOL,
	EVMLFORM,
	EVMODEFLAG,
	EVMODIFYHOOK,
	EVMSFLAG,
	EVNEWLINEHOOK,
	EVNUMCOL,
	EVOCRYPT,
	EVPAGELEN,
	EVPALETTE,
	EVPENDING,
	EVPREADHOOK,
	EVPWRITEHOOK,
	EVPROGNAME,
	EVQUOTECOL,
	EVRAM,
	EVREADHOOK,
	EVREGION,
	EVREPLACE,
	EVRVAL,
	EVSEARCH,
	EVSEED,
	EVSOFTTAB,
	EVSRES,
	EVSSAVE,
	EVSSCROLL,
	EVSTATUS,
	EVSTERM,
	EVTABCOL,
	EVTARGET,
	EVTIME,
	EVTPAUSE,
	EVVERSION,
	EVWINDNUM,
	EVWINDPOS,
	EVWLINE,
	EVWRAPHOOK,
	EVWRITEHOOK,
	EVXPOS,
	EVYPOS
};

/* list of recognized user functions	 */

typedef struct UFUNC {
	char *f_name;	/* name of function */
	int f_type;	/* 1 = monamic, 2 = dynamic */
} UFUNC;

#define NILNAMIC	0
#define MONAMIC		1
#define DYNAMIC		2
#define TRINAMIC	3

UFUNC funcs[] = {
	{	"abs",	MONAMIC },	/* absolute value of a number */
	{	"add",	DYNAMIC },	/* add two numbers together */
	{	"and",	DYNAMIC },	/* logical and */
	{	"asc",	MONAMIC },	/* char to integer conversion */
	{	"ban",	DYNAMIC },	/* bitwise and */
	{	"bin",	MONAMIC },	/* loopup what function name is bound to a key */
	{	"bno",	MONAMIC },	/* bitwise not */
	{	"bor",	DYNAMIC },	/* bitwise or */
	{	"bxo",	DYNAMIC },	/* bitwise xor */
	{	"cat",	DYNAMIC },	/* concatinate string */
	{	"chr",	MONAMIC },	/* integer to char conversion */
	{	"cwd",	MONAMIC },	/* get absolute pathname */
	{	"div",	DYNAMIC },	/* division */
	{	"env",	MONAMIC },	/* retrieve a system environment var */
	{	"equ",	DYNAMIC },	/* logical equality check */
	{	"exi",	MONAMIC },	/* check if a file exists */
	{	"fin",	MONAMIC },	/* look for a file on the path... */
	{	"fti",	MONAMIC },	/* strftime */
	{	"gre",	DYNAMIC },	/* logical greater than */
	{	"gtc",	NILNAMIC  },	/* get 1 emacs command */
	{	"gtk",	NILNAMIC },	/* get 1 charater */
	{	"ind",	MONAMIC },	/* evaluate indirect value */
	{	"lef",	DYNAMIC },	/* left string(string, len) */
	{	"len",	MONAMIC },	/* string length */
	{	"les",	DYNAMIC },	/* logical less than */
	{	"low",	MONAMIC },	/* lower case string */
	{	"mat",	DYNAMIC },	/* regexp string matchtest */
	{	"mid",	TRINAMIC },	/* mid string(string, pos, len) */
	{	"mod",	DYNAMIC },	/* mod */
	{	"neg",	MONAMIC },	/* negate */
	{	"not",	MONAMIC },	/* logical not */
	{	"or",	DYNAMIC },	/* logical or */
	{	"rep",	DYNAMIC },	/* regexp string replace */
	{	"rig",	DYNAMIC },	/* right string(string, pos) */
	{	"rnd",	MONAMIC },	/* get a random number */
	{	"rsi",	DYNAMIC },	/* find the rindex of one string in another */
	{	"seq",	DYNAMIC },	/* string logical equality check */
	{	"sgr",	DYNAMIC },	/* string logical greater than */
	{	"sin",	DYNAMIC },	/* find the index of one string in another */
	{	"sle",	DYNAMIC },	/* string logical less than */
	{	"sub",	DYNAMIC },	/* subtraction */
	{	"tim",	DYNAMIC },	/* multiplication */
	{	"tri",	MONAMIC },	/* trim whitespace off the end of a string */
	{	"tru",	MONAMIC },	/* Truth of the universe logical test */
	{	"upp",	MONAMIC },	/* uppercase string */
	{	"xla",	TRINAMIC }	/* XLATE character string translation */
};

#define NFUNCS	(sizeof(funcs)/sizeof(UFUNC))

/* and its preprocesor definitions */

enum UFFUNC {
	UFABS,
	UFADD,
	UFAND,
	UFASCII,
	UFBAND,
	UFBIND,
	UFBNOT,
	UFBOR,
	UFBXOR,
	UFCAT,
	UFCHR,
	UFCWD,
	UFDIV,
	UFENV,
	UFEQUAL,
	UFEXIST,
	UFFIND,
	UFFTIME,
	UFGREATER,
	UFGTCMD,
	UFGTKEY,
	UFIND,
	UFLEFT,
	UFLENGTH,
	UFLESS,
	UFLOWER,
	UFMATCH,
	UFMID,
	UFMOD,
	UFNEG,
	UFNOT,
	UFOR,
	UFREGEXP,
	UFRIGHT,
	UFRND,
	UFRSINDEX,
	UFSEQUAL,
	UFSGREAT,
	UFSINDEX,
	UFSLESS,
	UFSUB,
	UFTIMES,
	UFTRIM,
	UFTRUTH,
	UFUPPER,
	UFXLATE,
};
