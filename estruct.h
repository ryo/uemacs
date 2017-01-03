/*
 * $Id: estruct.h,v 1.60 2017/01/02 18:22:09 ryo Exp $
 *
 * ESTRUCT: Structure and preprocesser defined for MicroEMACS 3.10
 *
 * written by Daniel Lawrence
 * based on code by Dave G. Conroy,
 *                  Steve Wilhite and George Jones
 */


#ifdef MSDOS
#undef MSDOS
#endif

/*
 * Program Identification...
 *
 * PROGNAME should always be MicroEMACS for a distribution unmodified version.
 * People using MicroEMACS as a shell for other products should change this
 * to reflect their product. Macros can query this via the $progname variable
 */

#define PROGNAME	"MicroEMACS"
#define VERSION		"3.10r"


/* Machine/OS definitions */
/* [Set one of these!!] */
#define MSDOS	0		/* MS-DOS */
#define BSD	1		/* UNIX BSD 4.2 and ULTRIX */


/* Debugging options */
#define RAMSIZE	1		/* dynamic RAM memory usage tracking */
#define RAMSHOW	0		/* auto dynamic RAM reporting */
#define RAMTRCK	0		/* send debug info to MALLOC.DAT */


/* Terminal Output definitions */
#define TERMCAP 1		/* Use TERMCAP */


/* Configuration options */
#define CVMVAS	1		/* arguments to page forward/back in pages */
#define CLRMSG	0		/* space clears the message line with no insert */
#define CFENCE	1		/* fench matching in CMODE */
#define TYPEAH	1		/* type ahead causes update to be skipped */
#define DEBUGM	1		/* $debug triggers macro debugging */
#define LOGFLG	0		/* send all executed commands to EMACS.LOG */
#define VISMAC	0		/* update display during keyboard macros */
#define NBRACE	1		/* new style brace matching command */
#define COMPLET	1		/* new completion code (as of 3.10) */
#define CLEAN	0		/* de-alloc memory on exit */
#define CALLED	0		/* is emacs a called subroutine? or stand alone */
#define ISRCH	1		/* Incremental searches like ITS EMACS */
#define WORDPRO	1		/* Advanced word processing features */
#define APROP	1		/* Add code for Apropos command */
#define CRYPT	1		/* file encryption enabled? */
#define MAGIC	1		/* include regular expression matching? */
#define AEDIT	1		/* advanced editing options: en/detabbing */
#define PROC	1		/* named procedures */
#define MOUSE	0		/* Include routines for mouse actions */

/* Character set options */
/* [Set one of these!!] */
#define ASCII	1		/* always using ASCII char sequences for now */

/* handle constant and voids properly */

#ifdef __STDC__
#define CONST	const
#else
#define CONST
#endif

/* System dependant library redefinitions, structures and includes */

/*
 * the following define allows me to initialize unions... otherwise we make
 * them structures (like the keybinding table)
 */

#if __STDC__ /* if ANSI C compatible */
#define ETYPE	union
#else
#define ETYPE	struct
#endif


/* define some ability flags */

#if MSDOS | BSD
#define ENVFUNC	1
#else
#define ENVFUNC 0
#endif

#define DIRSEPSTR	"/"
#define DIRSEPCHAR	'/'

/* Emacs global flag bit definitions (for gflags) */

#define GFREAD 1

/* internal constants	 */

#define NBINDS	512		/* max # of bound keys */
#define NFILEN	256		/* max length of filename */
#define NMLFORM	512
#define NBUFN	256		/* # of bytes, buffer name */
#define NLINE	512		/* # of bytes, input line */
#define NSTRING	1024		/* # of bytes, string buffers */
#define NPAT	(NSTRING/4)	/* # of bytes, pattern */
#define NKBDM	1024		/* # of strokes, keyboard macro */
#define HUGE	1000		/* Huge number */
#define NLOCKS	100		/* max # of file locks active */
#define KBLOCK	1024		/* sizeof kill buffer chunks */
#define NBLOCK	64		/* line block chunk size */
#define NVSIZE	64		/* max #chars in a var name */
#define NMARKS	20		/* number of marks */

#define CTRLBIT	0x00000100	/* Control flag, or'ed in */
#define META	0x00000200	/* Meta flag, or'ed in */
#define CTLX	0x00000400	/* ^X flag, or'ed in */
#define SPEC	0x00000800	/* special key (function keys) */
#define MOUS	0x00001000	/* alternative input device (mouse) */
#define SHFT	0x00002000	/* shifted (for function keys) */
#define ALTD	0x00004000	/* ALT key... */
#define CTLC	0x00008000	/* ^C flag, or'ed in */

#define BINDNUL	0		/* not bount to anything */
#define BINDFNC	1		/* key bound to a function */
#define BINDBUF	2		/* key bound to a buffer */

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#define FALSE	0		/* False, no, bad, etc. */
#define TRUE	1		/* True, yes, good, etc. */
#define ABORT	2		/* Death, ^G, abort, etc. */
#define FAILED	3		/* not-quite fatal false return */

#define STOP	0		/* keyboard macro not in use */
#define PLAY	1		/* playing */
#define RECORD	2		/* recording */

/* Competion types  */
#define CMP_BUFFER	0
#define CMP_COMMAND	1
#define CMP_FILENAME	2
#define	CMP_MODE	3
#define	CMP_VARIABLE	4

/* Directive definitions  */
#define DIF		0
#define DELSE		1
#define DENDIF		2
#define DGOTO		3
#define DRETURN		4
#define DENDM		5
#define DWHILE		6
#define DENDWHILE	7
#define DBREAK		8
#define DFORCE		9
#define NUMDIRS		10

/*
 * PTBEG, PTEND, FORWARD, and REVERSE are all toggle-able values for
 * the scan routines.
 */
#define PTBEG	0		/* Leave the point at the beginning on search	 */
#define PTEND	1		/* Leave the point at the end on search		 */

#define FORWARD	0		/* forward direction		 */
#define REVERSE	1		/* backwards direction		 */

#define FIOSUC	0		/* File I/O, success.		 */
#define FIOFNF	-1		/* File I/O, file not found.	 */
#define FIOEOF	-2		/* File I/O, end of file.	 */
#define FIOERR	-3		/* File I/O, error.		 */
#define FIOMEM	-4		/* File I/O, out of memory	 */
#define FIOFUN	-5		/* File I/O, eod of file/bad line */
#define FIODEL	-6		/* Can't delete/rename file	 */

#define CFCPCN	0x0001		/* Last command was C-P, C-N	 */
#define CFKILL	0x0002		/* Last command was a kill	 */

#define I_NEWLINE	0x0a		/* newline */
#define BELL		0x07		/* a bell character  */
#define TAB		0x09		/* a tab character  */


#if BSD
#define PATHCHR	':'
#else
#define PATHCHR ';'
#endif

#define INTWIDTH	(sizeof(int)*3)

/* Macro argument token types					 */
#define TKNUL	0		/* end-of-string		 */
#define TKARG	1		/* interactive argument		 */
#define TKBUF	2		/* buffer argument		 */
#define TKVAR	3		/* user variables		 */
#define TKENV	4		/* environment variables	 */
#define TKFUN	5		/* function....			 */
#define TKDIR	6		/* directive			 */
#define TKLBL	7		/* line label			 */
#define TKLIT	8		/* numeric literal		 */
#define TKSTR	9		/* quoted string literal	 */
#define TKCMD	10		/* command name			 */

/* Internal defined functions					 */
#define nextab(a,tabs)	((a - (a % (tabs))) + (tabs))

#define isletter(c)	(('a' <= (c) && 'z' >= (c)) || ('A' <= (c) && 'Z' >= (c)))

#define DIFCASE	0x20
#define CHCASE(c)	((c) ^ DIFCASE)	/* Toggle the case of a letter. */

/* Dynamic RAM tracking and reporting redefinitions	 */

#if RAMSIZE
#define MALLOC	allocate
#define REALLOC	reallocate
#define FREE	release
#else
#define MALLOC	malloc
#define REALLOC	realloc
#define FREE	free
#endif


/*
 * There is a window structure allocated for every active display window. The
 * windows are kept in a big list, in top to bottom screen order, with the
 * listhead at "wheadp". Each window contains its own values of dot and mark.
 * The flag field contains some bits that are set by commands to guide
 * redisplay. Although this is a bit of a compromise in terms of decoupling,
 * the full blown redisplay is just too expensive to run for every input
 * character.
 */
typedef struct WINDOW {
	struct WINDOW *w_wndp;		/* Next window */
	struct BUFFER *w_bufp;		/* Buffer displayed in window */
	struct LINE *w_linep;		/* Top line in the window */
	struct LINE *w_dotp;		/* Line containing "." */
	int w_doto;			/* Byte offset for "." */
	struct LINE *w_markp[NMARKS];	/* Line containing "mark" */
	int w_marko[NMARKS];		/* Byte offset for "mark" */
	int w_toprow;			/* Origin 0 top row of window */
	int w_ntrows;			/* # of rows of text in window */
	char w_force;			/* If NZ, forcing row */
	char w_flag;			/* Flags */
	int w_fcol;			/* first column displayed */
} WINDOW;

#define WFFORCE	0x01		/* Window needs forced reframe */
#define WFMOVE	0x02		/* Movement from line to line */
#define WFEDIT	0x04		/* Editing within a line */
#define WFHARD	0x08		/* Better to a full display */
#define WFMODE	0x10		/* Update mode line */

/*
 * Text is kept in buffers. A buffer header, described below, exists for every
 * buffer in the system. The buffers are kept in a big list, so that commands
 * that search for a buffer by name can find the buffer header. There is a
 * safe store for the dot and mark in the header, but this is only valid if
 * the buffer is not being displayed (that is, if "b_nwnd" is 0). The text for
 * the buffer is kept in a circularly linked list of lines, with a pointer to
 * the header line in "b_linep" Buffers may be "Inactive" which means the
 * files associated with them have not been read in yet. These get read in
 * at "use buffer" time.
 */
typedef struct BUFFER {
	struct BUFFER *b_bufp;		/* Link to next BUFFER           */
	struct LINE *b_dotp;		/* Link to "." LINE structure    */
	int b_doto;			/* Offset of "." in above LINE   */
	struct LINE *b_markp[NMARKS];	/* The same as the above two,    */
	int b_marko[NMARKS];		/* but for the "mark"            */
	int b_fcol;			/* first col to display          */
	struct LINE *b_linep;		/* Link to the header LINE       */
	struct LINE *b_topline;		/* Link to narrowed top text     */
	struct LINE *b_botline;		/* Link to narrowed bottom text  */
	void *b_hashroot;		/* keywords for emphasis         */

	unsigned int b_mode;		/* editor mode of this buffer    */
#define NUMMODES	11		/* # of defined modes            */
#define MDWRAP		0x00000001	/* word wrap                     */
#define MDCMOD		0x00000002	/* C indentation and fence match */
#define MDSPELL		0x00000004	/* spell error parsing           */
#define MDEXACT		0x00000008	/* Exact matching for searches   */
#define MDVIEW		0x00000010	/* read-only buffer              */
#define MDOVER		0x00000020	/* overwrite mode                */
#define MDMAGIC		0x00000040	/* regular expresions in search  */
#define MDCRYPT		0x00000080	/* encrytion mode active         */
#define MDASAVE		0x00000100	/* auto-save mode                */
#define MDEMPHASIS	0x00000200	/* emphasis-text mode            */
#define MDCRLF		0x00000400	/* CRLF mode                     */

	char b_active;			/* window activated flag         */
	char b_nwnd;			/* Count of windows on buffer    */
	char b_flag;			/* Flags                         */
#define BFINVS	0x01		/* Internal invisable buffer             */
#define BFCHG	0x02		/* Changed since last write              */
#define BFTRUNC	0x04		/* buffer was truncated when read        */
#define BFNAROW	0x08		/* buffer has been narrowed              */

	char b_eofreturn;		/* need return at EOF            */
	char b_bom;			/* need BOM (UTF8,UTF16)         */

	char b_tabs;
	int b_mlcol;			/* modeline color                */
	char b_mlform[NMLFORM];		/* modeline format               */
	char b_bname[NBUFN];		/* Buffer name                   */
	char b_fname[NFILEN];		/* File name                     */
	unsigned int b_mtime;		/* File mtime that read in       */
	int b_kanjicode;		/* File input kanjicode          */
#if CRYPT
	char b_key[NPAT];	/* current encrypted key         */
#endif
} BUFFER;






/*
 * The starting position of a region, and the size of the region in
 * characters, is kept in a region structure.  Used by the region commands.
 */
typedef struct {
	struct LINE *r_linep;		/* Origin LINE address.      */
	int r_offset;			/* Origin LINE offset.       */
	long r_size;			/* Length in characters.     */
} REGION;

/*
 * All text is kept in circularly linked lists of "LINE" structures. These
 * begin at the header line (which is the blank line beyond the end of the
 * buffer). This line is pointed to by the "BUFFER". Each line contains a the
 * number of bytes in the line (the "used" size), the size of the text array,
 * and the text. The end of line is not stored as a byte; it's implied.
 */
typedef struct LINE {
	struct LINE *l_fp;		/* Link to the next line         */
	struct LINE *l_bp;		/* Link to the previous line     */
	int l_size;			/* Allocated size                */
	int l_used;			/* Used size                     */
	struct line_context {
#define	CTX_LABELLEN	16
		char ctx_label[CTX_LABELLEN];
		int ctx_labelno;
		unsigned char ctx_incomment_begin:1,
		              ctx_incomment_end:1,
		              ctx_unused:6;
	} l_context;
	unsigned char l_text[1];	/* A bunch of characters.        */
} LINE;

#define lforw(lp)	((lp)->l_fp)
#define lback(lp)	((lp)->l_bp)
#define lgetc(lp, n)	((lp)->l_text[(n)]&0xFF)
#define lgetc2(lp, n)	(((n) == llength(lp)) ? I_NEWLINE : lgetc(lp, n))

#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)	((lp)->l_used)





/*
 * The editor communicates with the display using a high level interface. A
 * "TERM" structure holds useful variables, and indirect pointers to routines
 * that do useful operations. The low level get and put routines are here too.
 * This lets a terminal, in addition to having non standard commands, have
 * funny get and put character code too. The calls might get changed to
 * "termp->t_field" style in the future, to make it possible to run more than
 * one terminal type.
 */
typedef struct {
	int t_mrow;		/* max number of rows allowable  */
	int t_nrow;		/* current number of rows used   */
	int t_mcol;		/* max Number of columns.        */
	int t_ncol;		/* current Number of columns.    */
	int t_margin;		/* min margin for extended lines */
	int t_scrsiz;		/* size of scroll region "       */
	int t_pause;		/* # times thru update to pause  */
	int (*t_open)(void);	/* Open terminal at the start.   */
	int (*t_close)(void);	/* Close terminal at end.        */
	int (*t_kopen)(void);	/* Open keyboard                 */
	int (*t_kclose)(void);	/* close keyboard                */
	int (*t_getchar)(void);	/* Get character from keyboard.  */
	int (*t_putchar)(unsigned char);	/* Put character to display.     */
	int (*t_flush)(void);	/* Flush output buffers.         */
	int (*t_move)(int,int);	/* Move the cursor, origin 0.    */
	int (*t_eeol)(void);	/* Erase to end of line.         */
	int (*t_eeop)(void);	/* Erase to end of page.         */
	int (*t_beep)(void);	/* Beep.                         */
	int (*t_rev)(int);	/* set reverse video state       */
	int (*t_bold)(int);	/* set bold character state      */
	int (*t_line)(int);	/* set underline character state */
	int (*t_rez)(int);	/* change screen resolution      */
	int (*t_setattr)(int);	/* set bold/line/color attribute */
} TERM;

#define	TERMATTR_FG_MASK	0x000f
#define	TERMATTR_BG_MASK	0x00f0
#define	TERMATTR_GETFG(attr)	((attr) & TERMATTR_FG_MASK)
#define	TERMATTR_GETBG(attr)	(((attr) & TERMATTR_BG_MASK) >> 4)
#define	TERMATTR_SETFG(attr, c)	do { (attr) = (((attr) & 0xfff0) | (c) & 0x000f) } while (0)
#define	TERMATTR_SETBG(attr, c)	do { (attr) = (((attr) & 0xff0f) | ((c) << 4) & 0x00f0) } while (0)
#define	TERMATTR_BOLD		0x0100
#define	TERMATTR_LINE		0x0200
#define	TERMATTR_REV		0x0400
#define	TERMATTR_MASK		0x07ff

/*
 * TEMPORARY macros for terminal I/O  (to be placed in a machine dependant
 * place later)
 */

#define TTopen		(*term.t_open)
#define TTclose		(*term.t_close)
#define TTkopen		(*term.t_kopen)
#define TTkclose	(*term.t_kclose)
#define TTgetc		(*term.t_getchar)
#define TTputc		(*term.t_putchar)
#define TTflush		(*term.t_flush)
#define TTmove		(*term.t_move)
#define TTeeol		(*term.t_eeol)
#define TTeeop		(*term.t_eeop)
#define TTbeep		(*term.t_beep)
#define TTrev		(*term.t_rev)
#define TTbold		(*term.t_bold)
#define TTline		(*term.t_line)
#define TTrez		(*term.t_rez)
#define TTattr		(*term.t_setattr)

/* Structure for the table of current key bindings */

ETYPE EPOINTER {
	int (*fp)(int,int);	/* C routine to invoke */
	BUFFER *buf;		/* buffer to execute */
};

typedef struct {
	unsigned int k_code;	/* Key code			 */
	unsigned int k_type;	/* binding type (C function or EMACS buffer) */
	ETYPE EPOINTER k_ptr;	/* ptr to thing to execute */
} KEYTAB;

/* structure for the name binding table		 */

typedef struct {
	char *n_name;		/* name of function key */
	int (*n_func) (int,int);	/* function name is bound to */
} NBIND;

/*
 * The editor holds deleted text chunks in the KILL buffer. The kill buffer
 * is logically a stream of ascii characters, however due to its
 * unpredicatable size, it gets implemented as a linked list of chunks. (The
 * d_ prefix is for "deleted" text, as k_ was taken up by the keycode
 * structure)
 */

typedef struct KILL {
	struct KILL *d_next;	/* link to next chunk, NULL if last */
	char d_chunk[KBLOCK];	/* deleted text */
} KILL;

/*
 * When emacs' command interpetor needs to get a variable's name, rather than
 * it's value, it is passed back as a VDESC variable description structure.
 * The v_num field is a index into the appropriate variable table.
 */

typedef struct VDESC {
	int v_type;	/* type of variable */
	int v_num;	/* ordinal pointer to variable in list */
} VDESC;

/*
 * The !WHILE directive in the execution language needs to stack references
 * to pending whiles. These are stored linked to each currently open
 * procedure via a linked list of the following structure
 */

typedef struct WHBLOCK {
	LINE *w_begin;		/* ptr to !while statement */
	LINE *w_end;		/* ptr to the !endwhile statement */
	int w_type;		/* block type */
	struct WHBLOCK *w_next;	/* next while */
} WHBLOCK;

#define BTWHILE		1
#define BTBREAK		2

/*
 * Incremental search defines.
 */
#if ISRCH

#define CMDBUFLEN	256	/* Length of our command buffer */

#define IS_ABORT	0x07	/* Abort the isearch */
#define IS_BACKSP	0x08	/* Delete previous char */
#define IS_TAB		0x09	/* Tab character (allowed search char) */
#define IS_NEWLINE	0x0D	/* New line from keyboard (Carriage return) */
#define IS_QUOTE	0x11	/* Quote next character */
#define IS_REVERSE	0x12	/* Search backward */
#define IS_FORWARD	0x13	/* Search forward */
#define IS_VMSQUOTE	0x16	/* VMS quote character */
#define IS_VMSFORW	0x18	/* Search forward for VMS */
#define IS_QUIT		0x1B	/* Exit the search */
#define IS_RUBOUT	0x7F	/* Delete previous character */

/* IS_QUIT is no longer used, the variable metac is used instead */

#endif


/*
 * HICHAR - 1 is the largest character we will deal with. HIBYTE represents
 * the number of bytes in the bitmap.
 */
#define HICHAR		256
#define HIBYTE		HICHAR >> 3

#if MAGIC
/*
 * Defines for the metacharacters in the regular expression
 * search routines.
 */
#define MCNIL		0	/* Like the '\0' for strings. */
#define LITCHAR		1	/* Literal character, or string. */
#define ANY		2
#define CCL		3
#define NCCL		4
#define BOL		5
#define EOL		6
#define DITTO		7
#define CLOSURE 	0x0100	/* An or-able value. */
#define GRPBEG		0x1000	/* An or-able value. */
#define GRPEND		0x2000	/* An or-able value. */
#define MASKCL		0x00ff

#define MC_ANY		'.'	/* 'Any' character (except newline). */
#define MC_CCL		'['	/* Character class. */
#define MC_NCCL		'^'	/* Negate character class. */
#define MC_RCCL		'-'	/* Range in character class. */
#define MC_ECCL		']'	/* End of character class. */
#define MC_BOL		'^'	/* Beginning of line. */
#define MC_EOL		'$'	/* End of line. */
#define MC_CLOSURE	'*'	/* Closure - does not extend past newline. */
#define MC_DITTO	'&'	/* Use matched string in replacement. */
#define MC_ESC		'\\'	/* Escape - suppress meta-meaning. */

#define BIT(n)		(1 << (n))	/* An integer with one bit set. */

/*
 * Typedefs that define the bitmap type for searching (BITMAP), the
 * meta-character structure for MAGIC mode searching (MC), and the
 * meta-character structure for MAGIC mode replacment (RMC).
 */
typedef char *BITMAP;

typedef struct {
	unsigned short mc_type;
	union {
		int lchar;
		BITMAP cclmap;
	} u;
} MC;

typedef struct {
	unsigned short mc_type;
	char *rstr;
} RMC;
#endif

/*
	This is the message which should be added to any "About MicroEMACS"
	boxes on any of the machines with window managers.

	------------------------------------------
	|                                        |
	|        MicroEMACS v3.xx                |
	|               for the ............     |
	|                                        |
	|    Text Editor and Corrector           |
	|                                        |
	|    written by Daniel M. Lawrence       |
	|    [based on code by Dave Conroy]      |
	|                                        |
	|    Send inquiries and donations to:    |
	|    617 New York St                     |
	|    Lafayette, IN 47901                 |
	|                                        |
	------------------------------------------
*/
