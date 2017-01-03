/*
----------------------------------------
	ESTRUCT.H: MicroEMACS 3.10
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/estruct.h,v 1.7 1992/02/15 13:11:14 SALT Exp SALT $
----------------------------------------
*/

/*
----------------------------------------
	定数の定義
----------------------------------------
*/

#define	PROGNAME	"MicroEmacs"
#define	ORGVER		"3.10"
#define	VERSION		"J1.43"
#define	RELEASE		"rel.5"

#define	MODIFIER	"lika/homy/salt/peace/shuna/rima"
#define	I_NEWSTR	"\012"

#define	SLASH		'\\'
#define	SLASH_STR	"\\"

#define	MC_ANY		'.'
#define	MC_CCL		'['
#define	MC_NCCL		'^'
#define	MC_RCCL		'-'
#define	MC_ECCL		']'
#define	MC_BOL		'^'
#define	MC_EOL		'$'
#define	MC_CLOSURE	'*'
#define	MC_DITTO	'&'
#define	MC_ESC		'\\'

#define	GFREAD		1
#define	NFBINDS0	200
#define	NFBINDS1	50
#define	NCHKSTR		128
#define	NMLFORM		136
#define	NFILEN		90
#define	NDCMD		24
#define	NBUFN		32
#define	NLINE		256
#define	NSTRING		256
#define	NMTITLE		80
#define	NMKEY		96
#define	NKBDM		1024
#define	MAXARG		32
#define	NKEY		32
#define	NIHEAD		8
#define	NTQUOTE		8
#define	NPAT		128
#define	HUGE		9999
#define	NCOLORS		16
#define	KBLOCK		1500
#define	NBLOCK		16
#define	NVSIZE		32
#define	NMARKS		20
#define	NKEYMAPS	10
#define	NUMMODES	11
#define	CMDBUFLEN	256
#define	MAXVARS 	64
#define	HICHAR		256
#define	CLOSURE		256
#define	GOOD		0
#define	LARGE_LINES	3000
#define	TURBOSIZE	8192
#define	REPORT_SIZE	16384
#define	REPORT_STEP	10240
#define	CMDLINESIZE	255

#define	READ_MODE	0
#define	WRITE_MODE	1

#define	BINDNUL 	0
#define	BINDFNC		1
#define	BINDBUF		2

#define	FALSE		0
#define	TRUE		1
#define	ABORT		2
#define	FAILED		3

#define	COPY		0
#define	MOVE		1

#define	STOP		0
#define	PLAY		1
#define	RECORD		2

#define	CMP_BUFFER		0
#define	CMP_C			1
#define	CMP_COMMAND		2
#define	CMP_FILENAME	3
#define	CMP_GENERAL		4
#define	CMP_KEYWORD		5
#define	CMP_LATEX		6
#define	CMP_MACRO		7
#define	CMP_MODE		8
#define	CMP_VARIABLE	9
#define	CMP_DIRNAME		10

#define	PTBEG		0
#define	PTEND		1

#define	FORWARD		0
#define	REVERSE		1

#define	FIOSUC		0
#define	FIOFNF		1
#define	FIOEOF		2
#define	FIOERR		3
#define	FIOMEM		4
#define	FIOFUN		5
#define	FIODEL		6

#define	TKNUL		0
#define	TKARG		1
#define	TKBUF		2
#define	TKVAR		3
#define	TKENV		4
#define	TKFUN		5
#define	TKDIR		6
#define	TKLBL		7
#define	TKLIT		8
#define	TKSTR		9
#define	TKCMD		10
#define	TKARGF		11
#define	TKARGB		12
#define	TKCOMP		13

#define	BTWHILE 	1
#define	BTBREAK		2

#define	CHANGE_STANDBY1	0
#define	CHANGE_STANDBY2	1
#define	CHANGE_READY	2
#define	CHANGE_OVER		3

#define	MCNIL		0
#define	LITCHAR		1
#define	ANY			2
#define	CCL			3
#define	NCCL		4
#define	BOL			5
#define	EOL			6
#define	DITTO		7

#define	MNONE		0
#define	MMOVE		1
#define	MREG		2

#define	BREAK_NON	0
#define	BREAK_OK	1
#define	END_OF_LINE	2

#define	I_NEWLINE	0x0a
#define	BELL		0x07
#define	TAB			0x09
#define	DIFCASE 	0x20
#define	CHARMASK	0xff
#define	WORDMASK	0xffff
#define	ERR_JOB		0xfff2
#define	ISALLOC		0xf7
#define	ISFREE		0x54
#define	ISMEMALIGN	0xd6
#define	NIL			((void *)0)

#define	IS_REVERSE2	0x05
#define	IS_ABORT	0x07
#define	IS_BACKSP	0x08
#define	IS_TAB		0x09
#define	IS_NEWLINE	0x0A
#define	IS_FORWARD1	0x0e
#define	IS_REVERSE1	0x10
#define	IS_QUOTE	0x11
#define	IS_68QUOTE	0x16
#define	IS_FORWARD2	0x18
#define	IS_QUIT		0x1b
#define	IS_RUBOUT	0x7f

#define	QVIEW		0x0001
#define	QREST		0x0002
#define	QDIRED		0x0004
#define	QMINUS		0x0008
#define	QZMIN		0x0010
#define	QZERO		0x0020
#define	QKILL		0x0040

#define	WFFORCE		0x01
#define	WFMOVE		0x02
#define	WFEDIT		0x04
#define	WFHARD		0x08
#define	WFMODE		0x10
#define	WFCOLR		0x20

#define	BFINVS		0x01
#define	BFCHG		0x02
#define	BFTRUNC		0x04
#define	BFNAROW		0x08

#define	MDWRAP		0x0001
#define	MDC			0x0002
#define	MDLATEX		0x0004
#define	MDEXACT		0x0008
#define	MDVIEW		0x0010
#define	MDOVER		0x0020
#define	MDMAGIC		0x0040
#define	MDCRYPT		0x0080
#define	MDASAVE		0x0100
#define	MDDIRED		0x0200
#define	MDSHELL		0x0400

#define	SHFT		0x0100
#define	CTRL		0x0200
#define	META		0x0400
#define	SPEC		0x0800
#define	ALTD		0x1000
#define	MOUS		0x2000

#define	CFCPCN		0x0001
#define	CFKILL		0x0002
#define	CFCOMPLETE	0x0004

#define	VFCHG		0x0001
#define	VFEXT		0x0002
#define	VFREQ		0x0008
#define	VFCOL		0x0010

#define	MAXMOD		19
#define	NROW		64
#define	NCOL		128
#define	MARGIN		8
#define	SCRSIZ		64
#define	NPAUSE		100

#define	TTgetc			(*term.t_getchar)

#define	lforw(lp)		((lp)->l_fp)
#define	lback(lp)		((lp)->l_bp)
#define	lgetc(lp, n)	((lp)->l_text[n] & CHARMASK)
#define	lgetc2(lp, n)	(((n) == llength(lp)) ? I_NEWLINE : lgetc(lp, n))
#define	lputc(lp, n, c)	((lp)->l_text[n] = (c))
#define	llength(lp)		((lp)->l_used)
#define	nextab(a, b)	(((a) - ((a) % (b))) + (b))
#define	mlreply(a, b, c)	(nextarg ((a), (b), (c), (CTRL | 'M')))
#define	mltreply(a, b, c, d)	(nextarg ((a), (b), (c), (d)))
#define	ctoec(a)		(((a) < 0x20) ? (CTRL | ((a) + '@')) : (a))
#define	ltos(a)			((char *) ((a) ? truem : falsem))
#define	forceabort()	(4 & K_KEYBIT (0x0a))
#define	upperc(a)		((a) - (islower ((a) & CHARMASK) ? DIFCASE : 0))
#define	lowerc(a)		((a) + (isupper ((a) & CHARMASK) ? DIFCASE : 0))
#define	isletter(a)		(isalpha (a))
#define	iskanji(a)		((a) >= 0x80 && ((a) <= 0x9f || ((a) >= 0xe0 && (a) <= 0xf5)))
#define	iskeyword(a, m)		({unsigned int _x = (a); (_keyword[_x] & (m));})
#define	absv(a)			((a) < 0 ? -(a) : (a))
#define	fixnull(a)		((a) ? : "")
#define	sep(a)			(((a) == '\\' || (a) == '/') ? slash : (a))

#define	DCHAR(a, b)		((((a) << 8) | ((b) & CHARMASK)) & WORDMASK)
#define	CHCASE(c)		((c) ^ DIFCASE)
#define	HIBYTE			(HICHAR >> 3)
#define	INTWIDTH		(sizeof (int) * 3)
#define	BIT(n)			(1 << (n))
#define	MASKCL			(CLOSURE - 1)
#define	ADJUST(p, d)	((p) += sizeof (d))
#define	BSIZE(a)		((a + NBLOCK - 1) & (~(NBLOCK - 1)))
#define	CHAIN(a)		(*(struct mhead **) (sizeof (char *) + (char *) (a)))
#define	CAPS_sense()	(K_SFTSNS() & 0x80)
#define	keydrops()		if (!intercept_flag) \
							KFLUSHIO(0xfe)

/*
----------------------------------------
	変数型の定義
----------------------------------------
*/

typedef char * BITMAP;

/*
----------------------------------------
	構造体の定義
----------------------------------------
*/

typedef struct WINDOW
  {
    struct WINDOW *w_wndp;
    struct BUFFER *w_bufp;
    struct LINE *w_linep;
    struct LINE *w_dotp;
    short w_doto;
    short w_jumpno;
    short w_markno;
    struct LINE *w_markp[NMARKS];
    short w_marko[NMARKS];
    char w_toprow;
    char w_ntrows;
    char w_force;
    char w_flag;
    short w_fcol;
  }
WINDOW;

typedef struct BUFFER
  {
    struct BUFFER *b_bufp;
    struct LINE *b_dotp;
    short b_doto;
    short b_jumpno;
    short b_markno;
    struct LINE *b_markp[NMARKS];
    short b_marko[NMARKS];
    short b_fcol;
    short b_mode;
    struct LINE *b_linep;
    struct LINE *b_topline;
    struct LINE *b_botline;
    char b_active;
    char b_nwnd;
    char b_flag;
    char b_keymap;
    char b_tabs;
    char b_stabs;
    char b_emp_text;
    char b_fname[NFILEN];
    char b_bname[NBUFN];
    char b_key[NKEY];
    char b_mlform[NMLFORM];
    int b_mlcolor;
    short b_fep_mode;
    char *b_localvar;
    char *b_comp_keyword;
    char *b_comp_keyword_set;
  }
BUFFER;

typedef struct REGION
  {
    struct LINE *r_linep;
    int r_offset;
    int r_size;
  }
REGION;

typedef struct LINE
  {
    struct LINE *l_fp;
    struct LINE *l_bp;
    unsigned short l_size;
    unsigned short l_used;
    char l_text[0];
  }
LINE;

typedef struct TERM
  {
    short t_mrow;
    short t_nrow;
    short t_mcol;
    short t_ncol;
    short t_margin;
    short t_scrsiz;
    int t_pause;
    int (*t_getchar)(void);
  }
TERM;

union EPOINTER
  {
    int (*fp)(int, int);
    struct BUFFER *buf;
  };

typedef struct BINDTAB
  {
    int bind_code;
    int (*bind_fp)(int, int);
  }
BINDTAB;

typedef struct BINDLIST
  {
    struct BINDTAB *bd_tab;
    short bd_size;
  }
BINDLIST;

typedef struct KEYTAB
  {
    int k_code;
    short k_rest;
    short k_type;
    union EPOINTER k_ptr;
  }
KEYTAB;

typedef struct NBIND
  {
  	struct NBIND *n_next;
    char *n_name;
    int (*n_func)(int, int);
    short n_rest;
  }
NBIND;

typedef struct KILL
  {
    struct KILL *d_next;
    char d_chunk[KBLOCK];
  }
KILL;

typedef struct VDESC
  {
    char v_type;
    char v_num;
    void *var;
  }
VDESC;

typedef struct WHBLOCK
  {
    struct LINE *w_begin;
    struct LINE *w_end;
    short w_type;
    struct WHBLOCK *w_next;
  }
WHBLOCK;

typedef struct MC
  {
    short mc_type;
    union
      {
    short lchar;
    BITMAP cclmap;
      }
    u;
  }
MC;

typedef struct RMC
  {
    short mc_type;
    char *rstr;
  }
RMC;

typedef struct VIDEO
  {
    int v_linecnt;
    short v_crflag;
    char v_linestr[8];
    char v_high;
    char v_flag;
    char v_bcolor;
    char v_rbcolor;
    char v_text[0];
  }
VIDEO;

typedef struct UVAR
  {
  	struct UVAR *u_next;
  	struct UVAR *u_hnext;
    char *u_name;
    char *u_value;
  }
UVAR;

typedef struct UFUNC
  {
    char *f_name;
    char f_type;
    char f_treat;
    short f_num;
  }
UFUNC;

typedef struct UENVAR
  {
  	struct UENVAR *e_next;
    char *e_name;
    short e_num;
    const void *var;
  }
UENVAR;

typedef struct UDIRS
  {
    char *d_name;
    short d_num;
  }
UDIRS;

typedef struct ALOAD
  {
    char macro[NBUFN];
    char file[NFILEN];
    struct ALOAD *next;
  }
ALOAD;

typedef struct UCWORD
  {
    char *c_name;
  }
UCWORD;

typedef struct LEDIT {
    char    *text;
    int     pos;
    int     limit;
    int     insmode;
    int     home;
    int     base;
    int     modified;
    int		chg_insmode;
} LEDIT;

typedef struct _tinf {
    short hsize;
    short vsize;
    short funckey;
    short conmode;
} _tinf;
