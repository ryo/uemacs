/*
 * $Id: edef.h,v 1.35 2017/01/02 18:22:09 ryo Exp $
 *
 * EDEF: Global variable definitions for MicroEMACS 3.9
 *
 * written by Daniel Lawrence based on code by Dave G. Conroy, Steve Wilhite and
 * George Jones
 */

#ifdef maindef

/* initialized global definitions */

int fillcol = 72;		/* Current fill column */
short kbdm[NKBDM];		/* Macro */
char *execstr = NULL;		/* pointer to string to execute */
char golabel[NPAT] = "";	/* current line to go to */
int execlevel = 0;		/* execution IF level */
int eolexist = TRUE;		/* does clear to EOL exist? */
int revexist = FALSE;		/* does reverse video exist? */
int flickcode = TRUE;		/* do flicker supression? */

int dispkanji = 0;		/* display kanji code */
int inpkanji = 0;		/* input kanji code */
int distab = FALSE;
int diszen = FALSE;
int dispcr = FALSE;
int disnum = FALSE;
int dislabel = FALSE;
int dispeof = FALSE;
char crchar = '/';
int tabcol = 0x04;		/* blue */
int cmdcol = 0x00;		/* normal */
int numcol = 0x00;		/* normal */
int crcol = 0x04;		/* blue */
int eofcol = 0x04;		/* blue */
int emphasiscol = 0x00;		/* normal color */
int commentcol = 0x00;		/* normal color */
int funccol = 0x00;			/* normal color */
int emphasisbold = 1;
int quotecol = 0x0008;		/* white */

char comment_in[NSTRING] = "/*";
char comment_out[NSTRING] = "*/";
char comment_line[NSTRING] = "//";

char mlform[NMLFORM] = "?#?*?n?k ?p (?m) ?=?= ?b ?=?=?f";
int mlcol = 0x40;		/* normal color */

/* name of modes */
CONST char *modename[] = {
	"WRAP",
	"C",
	"SPELL",
	"EXACT",
	"VIEW",
	"OVER",
	"MAGIC",
	"CRYPT",
	"ASAVE",
	"EMPHASIS",
	"CRLF",
	};
CONST char modecode[] = "WCSEVOMYAPL";	/* letters to represent modes */

int numfunc = NBINDFUNCS;	/* number of bindable functions */
int gmode = 0;			/* global editor mode */
int gflags = GFREAD;		/* global control flag */
int gfcolor = 7;		/* global forgrnd color (white) */
int gbcolor = 0;		/* global backgrnd color (black) */
int gasave = 256;		/* global ASAVE size */
int gacount = 256;		/* count until next ASAVE */
int sgarbf = TRUE;		/* TRUE if screen is garbage */
int mpresf = FALSE;		/* TRUE if message in last line */
int clexec = FALSE;		/* command line execution flag */
int mstore = FALSE;		/* storing text to macro flag */
int discmd = 1;			/* display command flag */
int ansiesc = FALSE;		/* ansi escape sequence for color text */
int makbak = TRUE;		/* make backup file */
int disinp = TRUE;		/* display input characters */
int modeflag = TRUE;		/* display modelines flag */
int sscroll = FALSE;		/* smooth scrolling enabled flag */
int hscroll = TRUE;		/* horizontal scrolling flag */
int ifsplit = TRUE;
int hjump = 1;			/* horizontal jump size */
int ssave = TRUE;		/* safe save flag */
struct BUFFER *bstore = NULL;	/* buffer to store macro text to */
int vtrow = 0;			/* Row location of SW cursor */
int vtcol = 0;			/* Column location of SW cursor */
int leftmargin = 0;		/* left margin of columns */
int ttrow = HUGE;		/* Row location of HW cursor */
int ttcol = HUGE;		/* Column location of HW cursor */
int lbound = 0;			/* leftmost column of current line being displayed */
int taboff = 0;			/* tab offset for display */
int tabsize = 8;		/* current hard tab size */
int stabsize = 0;		/* current soft tab size (0: use hard tabs) */
int reptc = CTRLBIT | 'U';	/* current universal repeat char */
int abortc = CTRLBIT | 'G';	/* current abort command char */
int sterm = CTRLBIT | '[';	/* search terminating character */

int prefix = 0;			/* currently pending prefix bits */
int prenum = 0;			/* numeric arg */
int predef = TRUE;		/* default flag */

int quotec = 0x11;		/* quote char during mlreply() */
CONST char *cname[] = {		/* names of colors */
	"BLACK",
	"RED",
	"GREEN",
	"YELLOW",
	"BLUE",
	"MAGENTA",
	"CYAN",
	"GREY",
	"GRAY",
	"LRED",
	"LGREEN",
	"LYELLOW",
	"LBLUE",
	"LMAGENTA",
	"LCYAN",
	"WHITE"
};

KILL *kbufp = NULL;		/* current kill buffer chunk pointer */
KILL *kbufh = NULL;		/* kill buffer header pointer */
int kused = KBLOCK;		/* # of bytes used in kill buffer */
WINDOW *swindow = NULL;		/* saved window pointer */
int cryptflag = FALSE;		/* currently encrypting? */
int oldcrypt = FALSE;		/* using old(broken) encryption? */
short *kbdptr;			/* current position in keyboard buf */
short *kbdend = &kbdm[0];	/* ptr to end of the keyboard */
int kbdmode = STOP;		/* current keyboard macro mode */
int kbdrep = 0;			/* number of repetitions */
int restflag = FALSE;		/* restricted use? */
int lastkey = 0;		/* last keystoke */
int seed = 0;			/* random number seed */
unsigned long envram = 0l;	/* # of bytes current in use by malloc */
unsigned long mlform_envram;	/* # of kbytes last displayed */
unsigned long envmalloc = 0l;	/* # of malloc */
int macbug = FALSE;		/* macro debugging flag */
int mouseflag = TRUE;		/* use the mouse? */
int diagflag = FALSE;		/* diagonal mouse movements? */
CONST char errorm[] = "ERROR";	/* error literal */
CONST char truem[] = "TRUE";	/* true literal */
CONST char falsem[] = "FALSE";	/* false litereal */
int cmdstatus = TRUE;		/* last command status */
char palstr[49] = "";		/* palette string */
char lastmesg[NSTRING] = "";	/* last message posted */
char *lastptr = NULL;		/* ptr to lastmesg[] */
int saveflag = 0;		/* Flags, saved with the $target var */
char *fline = NULL;		/* dynamic return line */
int flen = 0;			/* current length of fline */
int rval = 0;			/* return value of a subprocess */
int eexitflag = FALSE;		/* EMACS exit flag */
int eexitval = 0;		/* and the exit return value */
int xpos = 0;			/* current column mouse is positioned to */
int ypos = 0;			/* current screen row */
int nclicks = 0;		/* cleared on any non-mouse event */

/* uninitialized global definitions */
int currow;			/* Cursor row */
int curcol;			/* Cursor column */
int thisflag;			/* Flags, this command */
int lastflag;			/* Flags, last command */
int curgoal;			/* Goal for C-P, C-N */
WINDOW *curwp;			/* Current window */
BUFFER *curbp;			/* Current buffer */
WINDOW *wheadp;			/* Head of list of windows */
BUFFER *bheadp;			/* Head of list of buffers */
BUFFER *blistp;			/* Buffer for C-X C-B */

char sres[NBUFN];		/* current screen resolution */

char pat[NPAT];			/* Search pattern */
char tap[NPAT];			/* Reversed pattern array. */
char rpat[NPAT];		/* replacement pattern */

/* Various "Hook" execution variables */
char aborthook[NBUFN+2];	/* executed after abort-command */
char readhook[NBUFN+2];		/* executed on all file reads */
char postreadhook[NBUFN+2];	/* executed after all file reads */
char wraphook[NBUFN+2];		/* executed when wrapping text */
char cmdhook[NBUFN+2];		/* executed before looking for a command */
char modifyhook[NBUFN+2];	/* executed on first modify */
char writehook[NBUFN+2];	/* executed on all file writes */
char postwritehook[NBUFN+2];	/* executed after all file writes */
char exbhook[NBUFN+2];		/* executed when exiting a buffer */
char bufhook[NBUFN+2];		/* executed when entering a buffer */
char newlinehook[NBUFN+2];	/* executed on newline command */

/*
 * The variable matchlen holds the length of the matched string - used by the
 * replace functions. The variable patmatch holds the string that satisfies
 * the search command. The variables matchline and matchoff hold the line and
 * offset position of the *start* of match.
 */
unsigned int matchlen = 0;
unsigned int mlenold = 0;
char *patmatch = NULL;
LINE *matchline = NULL;
int matchoff = 0;

#if MAGIC
/*
 * The variables magical and rmagical determine if there
 * were actual metacharacters in the search and replace strings -
 * if not, then we don't have to use the slower MAGIC mode
 * search functions.
 */
short int magical = FALSE;
short int rmagical = FALSE;
MC mcpat[NPAT];		/* the magic pattern */
MC tapcm[NPAT];		/* the reversed magic pattern */
RMC rmcpat[NPAT];	/* the replacement magic array */

#endif

/*
 * directive name table: This holds the names of all the directives....
 */
CONST char *dname[] = {
	"if",
	"else",
	"endif",
	"goto",
	"return",
	"endm",
	"while",
	"endwhile",
	"break",
	"force"
};

#if DEBUGM
/* vars needed for macro debugging output */
char outline[NSTRING];	/* global string to hold debug line text */
#endif

#else

/* for all the other .C files */

/* initialized global external declarations */
extern int kanjicode;
extern int fillcol;
extern short kbdm[];
extern char *execstr;
extern char golabel[];
extern int execlevel;
extern int eolexist;
extern int revexist;
extern int flickcode;
extern int distab;
extern int diszen;
extern int tabcol;
extern int cmdcol;
extern int numcol;
extern int dispkanji;
extern int inpkanji;
extern int dispcr;
extern int dislabel;
extern int disnum;
extern int dispeof;
extern int crcol;
extern int eofcol;
extern char crchar;
extern int emphasisbold;
extern int emphasiscol;
extern int commentcol;
extern int funccol;
extern int quotecol;

extern char comment_in[];
extern char comment_out[];
extern char comment_line[];

extern char mlform[];
extern int mlcol;
CONST extern char *modename[];	/* text names of modes */
CONST extern char modecode[];	/* letters to represent modes */
extern int numfunc;		/* number of bindable functions */
extern KEYTAB keytab[];		/* key bind to functions table */
extern NBIND names[];		/* name to function table */
extern int gmode;		/* global editor mode */
extern int gflags;		/* global control flag */
extern int gfcolor;		/* global forgrnd color (white) */
extern int gbcolor;		/* global backgrnd color (black) */
extern int gasave;		/* global ASAVE size */
extern int gacount;		/* count until next ASAVE */
extern int sgarbf;		/* State of screen unknown */
extern int mpresf;		/* Stuff in message line */
extern int clexec;		/* command line execution flag */
extern int mstore;		/* storing text to macro flag */
extern int discmd;		/* display command flag */
extern int ansiesc;		/* ansi escape sequence for color text */
extern int makbak;		/* make backup file */
extern int disinp;		/* display input characters */
extern int modeflag;		/* display modelines flag */
extern int sscroll;		/* smooth scrolling enabled flag */
extern int hscroll;		/* horizontal scrolling flag */
extern int ifsplit;
extern int hjump;		/* horizontal jump size */
extern int ssave;		/* safe save flag */
extern struct BUFFER *bstore;	/* buffer to store macro text to */
extern int vtrow;		/* Row location of SW cursor */
extern int vtcol;		/* Column location of SW cursor */
extern int leftmargin;		/* left margin of columns */
extern int ttrow;		/* Row location of HW cursor */
extern int ttcol;		/* Column location of HW cursor */
extern int lbound;		/* leftmost column of current line being displayed */
extern int taboff;		/* tab offset for display */
extern int tabsize;		/* current hard tab size */
extern int stabsize;		/* current soft tab size (0: use hard tabs) */
extern int reptc;		/* current universal repeat char */
extern int abortc;		/* current abort command char */
extern int sterm;		/* search terminating character */

extern int prefix;		/* currently pending prefix bits */
extern int prenum;		/* numeric arg */
extern int predef;		/* default flag */

extern int quotec;		/* quote char during mlreply() */
extern CONST char *cname[];	/* names of colors */
extern KILL *kbufp;		/* current kill buffer chunk pointer */
extern KILL *kbufh;		/* kill buffer header pointer */
extern int kused;		/* # of bytes used in kill buffer */
extern WINDOW *swindow;		/* saved window pointer */
extern int cryptflag;		/* currently encrypting? */
extern int oldcrypt;		/* using old(broken) encryption? */
extern short *kbdptr;		/* current position in keyboard buf */
extern short *kbdend;		/* ptr to end of the keyboard */
extern int kbdmode;		/* current keyboard macro mode */
extern int kbdrep;		/* number of repetitions */
extern int restflag;		/* restricted use? */
extern int lastkey;		/* last keystoke */
extern int seed;		/* random number seed */
extern long envram;		/* # of bytes current in use by malloc */
extern long mlform_envram;	/* # of kbytes displayed */
extern long envmalloc;		/* # of malloc */
extern int macbug;		/* macro debugging flag */
extern int mouseflag;		/* use the mouse? */
extern int diagflag;		/* diagonal mouse movements? */
CONST extern char errorm[];	/* error literal */
CONST extern char truem[];	/* true literal */
CONST extern char falsem[];	/* false litereal */
extern int cmdstatus;		/* last command status */
extern char palstr[];		/* palette string */
extern char lastmesg[];		/* last message posted */
extern char *lastptr;		/* ptr to lastmesg[] */
extern int saveflag;		/* Flags, saved with the $target var */
extern char *fline;		/* dynamic return line */
extern int flen;		/* current length of fline */
extern int rval;		/* return value of a subprocess */
extern int eexitflag;		/* EMACS exit flag */
extern int eexitval;		/* and the exit return value */
extern int xpos;		/* current column mouse is positioned to */
extern int ypos;		/* current screen row */
extern int nclicks;		/* cleared on any non-mouse event */

/* uninitialized global external declarations */
extern int currow;		/* Cursor row */
extern int curcol;		/* Cursor column */
extern int thisflag;		/* Flags, this command */
extern int lastflag;		/* Flags, last command */
extern int curgoal;		/* Goal for C-P, C-N */
extern WINDOW *curwp;		/* Current window */
extern BUFFER *curbp;		/* Current buffer */
extern WINDOW *wheadp;		/* Head of list of windows */
extern BUFFER *bheadp;		/* Head of list of buffers */
extern BUFFER *blistp;		/* Buffer for C-X C-B */

extern char sres[NBUFN];	/* current screen resolution */

extern char pat[];		/* Search pattern */
extern char tap[];		/* Reversed pattern array. */
extern char rpat[];		/* replacement pattern */

/* Various "Hook" execution variables */

extern char aborthook[];	/* executed after abort-command */
extern char readhook[];		/* executed on all file reads */
extern char postreadhook[];	/* executed on all file reads */
extern char wraphook[];		/* executed when wrapping text */
extern char cmdhook[];		/* executed before looking for a cmd */
extern char modifyhook[];	/* executed on first modify */
extern char writehook[];	/* executed on all file writes */
extern char postwritehook[];	/* executed after all file writes */
extern char exbhook[];		/* executed when exiting a buffer */
extern char bufhook[];		/* executed when entering a buffer */
extern char newlinehook[];	/* executed on newline command */

extern unsigned int matchlen;
extern unsigned int mlenold;
extern char *patmatch;
extern LINE *matchline;
extern int matchoff;

#if MAGIC
extern short int magical;
extern short int rmagical;
extern MC mcpat[NPAT];		/* the magic pattern */
extern MC tapcm[NPAT];		/* the reversed magic pattern */
extern RMC rmcpat[NPAT];	/* the replacement magic array */
#endif

CONST extern char *dname[];	/* directive name table */

#if DEBUGM
/* vars needed for macro debugging output */
extern char outline[];		/* global string to hold debug line text */
#endif

#endif

/* terminal table defined only in TERM.C */

#ifndef termdef
extern TERM term;		/* Terminal information. */
#endif
