/*
----------------------------------------
	EDEF.H: MicroEMACS 3.10
----------------------------------------
*/

/*
----------------------------------------
	RCS id : $Header: f:/SALT/emacs/RCS/edef.h,v 1.10 1992/01/26 11:06:08 SALT Exp SALT $
----------------------------------------
*/

/*
----------------------------------------
	main.c ïœêîÇÃèâä˙ílê›íË
----------------------------------------
*/

#ifdef	maindef

BUFFER *bheadp, *blistp, *bcompp, *curbp;
BUFFER *bdiredp, *bdmarkp;
BUFFER *bhisexecp, *bhisenvp, *bhiscmdp, *bhissearchp;
BUFFER *bhisargp, *bhisdebugp;
BUFFER *bhiscmpbufp, *bhiscmpcp, *bhiscmpcomp, *bhiscmpfnamep;
BUFFER *bhiscmpgenp, *bhiscmplatexp, *bhiscmpmacp, *bhiscmpmodep;
BUFFER *bhiscmpvarp, *bhiscmpdnamep;
KEYTAB *keytab[NKEYMAPS];
LINE *matchline;
MC mcpat[NPAT], tapcm[NPAT];
RMC rmcpat[NPAT];
UVAR *uv_head, *uv_tail;
VIDEO **vscreen;
WINDOW *curwp, *wheadp;
char **ex_agv, **pr_agv, **func_name;
char palstr[49], palstr2[49], sres[6], comp_gene[NFILEN], comp_c[NFILEN];
char current_dir[NFILEN];
char comp_latex[NFILEN], latex_env[NFILEN], help_load_path[NFILEN];
char golabel[NPAT], lastmesg[NSTRING];
char trash[NFILEN], dired_current_dir[NFILEN];
char outline[NSTRING], pat[NPAT], rpat[NPAT];
char tap[NPAT], system_temp[NSTRING];
char wrapindentitem[NIHEAD], wrapindenthead[NIHEAD];
char bufhook[NBUFN], cmdhook[NBUFN], exbhook[NBUFN], inshook[NBUFN];
char readhook[NBUFN], windhook[NBUFN], wraphook[NBUFN], writehook[NBUFN];
int *kbdptr, curcol, curgoal, currow, dired_mark_num, sccurcol;
int malloc_sbrk_unused, malloc_sbrk_used, thisflag;
int matchlen, matchoff, mlenold;
int sccurline, kbdm[NKBDM], lastflag;
int malloc_rating, malloc_old_rating;
int xpos, ypos, ex_agc, pr_agc, bufnum, bufpos;
int windnum, windpos, nfuncs, ttrow, ttcol;
int taboff, vtcol, vtrow;
int updinsf;
int LEDmode;
long *lndrv_info;

ALOAD *autolist = 0;
BUFFER *bstore = 0;
KILL *kbufh = 0;
KILL *kbufp = 0;
KILL *kbufh2 = 0;
KILL *kbufp2 = 0;
WINDOW *swindow = 0;

char *execstr = 0;
char *fline = 0;
char *lastptr = 0;
char *patmatch = 0;

char dired_cp_cmd[NDCMD] = "cp.x -f";
char dired_ls_cmd[NDCMD] = "ls.x -la";
char dired_mv_cmd[NDCMD] = "mv.x -f";
char dired_rm_cmd[NDCMD] = "rm.x -f";

char slash = '\\';

char backnonbreakchar[NCHKSTR]
  = "ÅLÅeÅgÅiÅkÅmÅoÅqÅsÅuÅwÅyÅÉÅî";
char fornonbreakchar[NCHKSTR]
  = "ÅAÅBÅCÅDÅHÅIÅJÅKÅMÅjÅlÅnÅpÅrÅtÅvÅxÅzÅÑÅ[Å`"
    "ÅåÅçÇüÇ°Ç£Ç•ÇßÇ¡Ç·Ç„ÇÂÇÏÉ@ÉBÉDÉFÉHÉbÉÉÉÖÉáÉéÉïÉñ";
char ignoreext[NCHKSTR]
  = "o;a;x;z;r;lzh;lzs;zoo;arc;zip;tar;taz;bak;"
    "win;dvi;pic;mki;mag;mdx;mdi;mdn";
char mlform[NMLFORM]
  = "?#?*?n ?p ?v ?l[?r%%] (?m) ?=?= ?b ?=?=?f";
char menu_title[NMTITLE]
  = "no title";
char menu_exit[NMKEY]
  = "^[;^G";
char menu_select[NMKEY]
  = "^M";
char texbquote[NTQUOTE]
  = "\t\' ";
char wrapitem[NCHKSTR]
  = "ÅEÅÊÅñÅ¶ÅõÅùÅúÅ†Å°ÅôÅöÅûÅü";

int _dump_flag = 0;

int *kbdend = &kbdm[0];
int abortc = CTRL | 'G';
int addeof = FALSE;
int breakmode = 0;
int cbufdir = FALSE;
int chigh = FALSE;
int cbold = FALSE;
int clexec = FALSE;
int cmdarg = 1;
int cmdcol = 3;
int cmdstatus = TRUE;
int cmdzencol = 1;
int colvis = FALSE;
int comp_sort = TRUE;
int comp_sort_dir = TRUE;
int comp_sort_ignore_case = FALSE;
int dateflag = FALSE;
int debug_system = 0;
int density = 0;
int diagflag = FALSE;
int dired_cd_flag = TRUE;
int dired_ls_name_pos = 41;
int discmd = TRUE;
int disinp = TRUE;
int disnum = TRUE;
int dispcr = TRUE;
int distab = FALSE;
int diszen = FALSE;
int edpage = TRUE;
int eexitflag = FALSE;
int eexitval = 0;
int ena_include_fep_mode = 0;
int ena_hscroll = 0;
int ena_wait_vdisp = 0;
int ena_zcursor = 0;
int english = FALSE;
int envram = 0;
int execlevel = 0;
int executinghook = FALSE;
int fepctrl = TRUE;
int fepctrlesc = TRUE;
int fillcol = 72;
int fkmode = TRUE;
int flen = 0;
int forceproc = TRUE;
int gacount = 256;
int gasave = 256;
int gbcolor = 13;
int gflags = GFREAD;
int gmode = 0;
int hjump = 8;
int hscroll = TRUE;
int ifsplit = FALSE;
int ignmetacase = FALSE;
int isearch_last_key = 0;
int issuper = 0;
int kbdmode = STOP;
int kbdrep = 0;
int kused = KBLOCK;
int kused2 = KBLOCK;
int lastkey = 0;
int lbound = 0;
int leftmargin = 6;
int macbug = FALSE;
int magical = FALSE;
int makbak = TRUE;
int memrflag = TRUE;
int menu_code = -1;
int menu_cursor = -1;
int menu_frame_color = 0x0d;
int menu_height = -1;
int menu_home_x = -1;
int menu_home_y = -1;
int menu_item_color = -1;
int menu_last_key = 0;
int menu_quickact = TRUE;
int menu_roll = 0;
int menu_select_color = 0x0e;
int menu_width = -1;
int modeflag = TRUE;
int mouseflag = FALSE;
int mpresf = FALSE;
int mstore = FALSE;
int mulmax = 10;
int nclicks = 0;
int newscroll = TRUE;
int noglobal = FALSE;
int nokill = FALSE;
int optswap = FALSE;
int opt1key = 4;
int opt2key = 8;
int predef = TRUE;
int prenum = 0;
int quickext = FALSE;
int quotec1 = CTRL | 'Q';
int quotec2 = CTRL | 'V';
int reptc = CTRL | 'U';
int restflag = FALSE;
int rmagical = FALSE;
int rval = 0;
int saveflag = 0;
int seed = 0;
int sgarbf = TRUE;
int shbufsize = 32;
int show_filesize = FALSE;
int ssave = TRUE;
int sscroll = TRUE;
int sscroll_slow = 0;
int stabsize = 0;
int stabmode = 0;
int sterm = CTRL | '[';
int syseval = TRUE;
int syskill = FALSE;
int syskillflg = FALSE;
int tabsize = 8;
int timeflag = FALSE;
int tokana = FALSE;
int unix_newline = FALSE;
int vbell = 1;
int wrapexpand = FALSE;
int xf1_key = 0;
int xf2_key = SPEC;
int xf3_key = META;
int xf4_key = 0;
int xf5_key = 0;
int zcursor = 0;

int compcase[] =
  {
    FALSE,      /* CMP_BUFFER   */
    FALSE,      /* CMP_C        */
    FALSE,      /* CMP_COMMAND  */
    FALSE,      /* CMP_FILENAME */
    FALSE,      /* CMP_GENERAL  */
    FALSE,      /* CMP_LATEX    */
    FALSE,      /* CMP_MACRO    */
    FALSE,      /* CMP_MODE     */
    FALSE       /* CMP_VARIABLE */
  };

int quickact[] =
  {
    TRUE,       /* CMP_BUFFER   */
    TRUE,       /* CMP_C        */
    TRUE,       /* CMP_COMMAND  */
    FALSE,      /* CMP_FILENAME */
    FALSE,      /* CMP_GENERAL  */
    TRUE,       /* CMP_LATEX    */
    TRUE,       /* CMP_MACRO    */
    TRUE,       /* CMP_MODE     */
    TRUE        /* CMP_VARIABLE */
  };

const char nsupm[] = "---";
const char errorm[] = "ERROR";
const char truem[] = "TRUE";
const char falsem[] = "FALSE";
const char modecode[] = "WCLEVOMYADS";
const char *modename[] =
  {
    "WRAP", "C", "LATEX", "EXACT", "VIEW", "OVER",
    "MAGIC", "CRYPT", "ASAVE", "DIRED", "SHELL"
  };
const char *cname[] =
  {
    "BLACK", "BLUE", "YELLOW", "WHITE",
    "HBLACK", "HBLUE", "HYELLOW", "HWHITE",
    "RBLACK", "RBLUE", "RYELLOW", "RWHITE",
    "HRBLACK", "HRBLUE", "HRYELLOW", "HRWHITE"
  };
const char breakchar[] =
  {
    "Å@ÅAÅBÅCÅDÅeÅfÅgÅhÅiÅjÅkÅlÅmÅnÅoÅpÅqÅrÅsÅtÅuÅvÅwÅxÅyÅzÅç"
  };
const char _cchar[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0x0? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0x1? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,     /* 0x2? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,     /* 0x3? */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* 0x4? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,     /* 0x5? */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* 0x6? */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,     /* 0x7? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0x8? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0x9? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xa? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xb? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xc? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xd? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xe? */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* 0xf? */
};

#else

/*
----------------------------------------
	ïœêîÇÃäOïîéQè∆êÈåæ
----------------------------------------
*/

extern BUFFER *bheadp, *blistp, *bcompp, *curbp;
extern BUFFER *bdiredp, *bdmarkp;
extern BUFFER *bhisexecp, *bhisenvp, *bhiscmdp, *bhissearchp;
extern BUFFER *bhisargp, *bhisdebugp;
extern BUFFER *bhiscmpbufp, *bhiscmpcp, *bhiscmpcomp, *bhiscmpfnamep;
extern BUFFER *bhiscmpgenp, *bhiscmplatexp, *bhiscmpmacp, *bhiscmpmodep;
extern BUFFER *bhiscmpvarp, *bhiscmpdnamep;
extern KEYTAB *keytab[NKEYMAPS];
extern LINE *matchline;
extern MC mcpat[NPAT], tapcm[NPAT];
extern RMC rmcpat[NPAT];
extern UVAR *uv_head, *uv_tail;
extern VIDEO **vscreen;
extern WINDOW *curwp, *wheadp;
extern char **ex_agv, **pr_agv, **func_name;
extern char palstr[49], palstr2[49], sres[6], comp_gene[NFILEN], comp_c[NFILEN];
extern char current_dir[NFILEN];
extern char comp_latex[NFILEN], latex_env[NFILEN], help_load_path[NFILEN];
extern char golabel[NPAT], lastmesg[NSTRING];
extern char trash[NFILEN], dired_current_dir[NFILEN];
extern char outline[NSTRING], pat[NPAT], rpat[NPAT];
extern char tap[NPAT], system_temp[NSTRING];
extern char wrapindentitem[NIHEAD], wrapindenthead[NIHEAD];
extern char bufhook[NBUFN], cmdhook[NBUFN], exbhook[NBUFN], inshook[NBUFN];
extern char readhook[NBUFN], windhook[NBUFN], wraphook[NBUFN], writehook[NBUFN];
extern int *kbdptr, curcol, curgoal, currow, dired_mark_num, sccurcol;
extern int malloc_sbrk_unused, malloc_sbrk_used, thisflag;
extern int matchlen, matchoff, mlenold;
extern int sccurline, kbdm[NKBDM], lastflag;
extern int malloc_rating, malloc_old_rating;
extern int xpos, ypos, ex_agc, pr_agc, bufnum, bufpos;
extern int windnum, windpos, nfuncs, ttrow, ttcol;
extern int taboff, vtcol, vtrow;
extern int updinsf;
extern int LEDmode;
extern long *lndrv_info;

extern ALOAD *autolist;
extern BUFFER *bstore;
extern KILL *kbufh;
extern KILL *kbufp;
extern KILL *kbufh2;
extern KILL *kbufp2;
extern WINDOW *swindow;

extern char *execstr;
extern char *fline;
extern char *lastptr;
extern char *patmatch;

extern char dired_cp_cmd[NDCMD];
extern char dired_ls_cmd[NDCMD];
extern char dired_mv_cmd[NDCMD];
extern char dired_rm_cmd[NDCMD];

extern char slash;

extern char backnonbreakchar[NCHKSTR];
extern char fornonbreakchar[NCHKSTR];
extern char ignoreext[NCHKSTR];
extern char mlform[NMLFORM];
extern char menu_title[NMTITLE];
extern char menu_exit[NMKEY];
extern char menu_select[NMKEY];
extern char texbquote[NTQUOTE];
extern char wrapitem[NCHKSTR];

extern int _dump_flag;

extern int *kbdend;
extern int abortc;
extern int addeof;
extern int breakmode;
extern int cbufdir;
extern int chigh;
extern int cbold;
extern int clexec;
extern int cmdarg;
extern int cmdcol;
extern int cmdstatus;
extern int cmdzencol;
extern int colvis;
extern int comp_sort;
extern int comp_sort_dir;
extern int comp_sort_ignore_case;
extern int dateflag;
extern int debug_system;
extern int density;
extern int diagflag;
extern int dired_cd_flag;
extern int dired_ls_name_pos;
extern int discmd;
extern int disinp;
extern int disnum;
extern int dispcr;
extern int distab;
extern int diszen;
extern int edpage;
extern int eexitflag;
extern int eexitval;
extern int ena_include_fep_mode;
extern int ena_hscroll;
extern int ena_wait_vdisp;
extern int ena_zcursor;
extern int english;
extern int envram;
extern int execlevel;
extern int executinghook;
extern int fepctrl;
extern int fepctrlesc;
extern int fillcol;
extern int fkmode;
extern int flen;
extern int forceproc;
extern int gacount;
extern int gasave;
extern int gbcolor;
extern int gflags;
extern int gmode;
extern int hjump;
extern int hscroll;
extern int ifsplit;
extern int ignmetacase;
extern int isearch_last_key;
extern int issuper;
extern int kbdmode;
extern int kbdrep;
extern int kused;
extern int kused2;
extern int lastkey;
extern int lbound;
extern int leftmargin;
extern int macbug;
extern int magical;
extern int makbak;
extern int memrflag;
extern int menu_code;
extern int menu_cursor;
extern int menu_frame_color;
extern int menu_height;
extern int menu_home_x;
extern int menu_home_y;
extern int menu_item_color;
extern int menu_last_key;
extern int menu_quickact;
extern int menu_roll;
extern int menu_select_color;
extern int menu_width;
extern int modeflag;
extern int mouseflag;
extern int mpresf;
extern int mstore;
extern int mulmax;
extern int nclicks;
extern int newscroll;
extern int noglobal;
extern int nokill;
extern int optswap;
extern int opt1key;
extern int opt2key;
extern int predef;
extern int prenum;
extern int quickext;
extern int quotec1;
extern int quotec2;
extern int reptc;
extern int restflag;
extern int rmagical;
extern int rval;
extern int saveflag;
extern int seed;
extern int sgarbf;
extern int shbufsize;
extern int show_filesize;
extern int ssave;
extern int sscroll;
extern int sscroll_slow;
extern int stabsize;
extern int stabmode;
extern int sterm;
extern int syseval;
extern int syskill;
extern int syskillflg;
extern int tabsize;
extern int timeflag;
extern int tokana;
extern int unix_newline;
extern int vbell;
extern int wrapexpand;
extern int xf1_key;
extern int xf2_key;
extern int xf3_key;
extern int xf4_key;
extern int xf5_key;
extern int zcursor;

extern int compcase[];
extern int quickact[];
extern const char nsupm[];
extern const char errorm[];
extern const char truem[];
extern const char falsem[];
extern const char modecode[];
extern const char *modename[];
extern const char *cname[];
extern const char breakchar[];
extern const char _cchar[256];

#endif /* maindef */

extern BINDLIST bindtab[];

extern UENVAR env_table[];
extern NBIND command_table[];

extern int blink_count;
extern int numfunc, nevars;
extern int issuper, intercept_flag;
extern char font[], font_h[], exfont[], exfont_h[];
extern char cur_pat[], cur_pat_h[];

extern int env_word_table_top, dir_word_table_top;
extern int fnc_word_table_top, c_word_table_top;
extern int env_word_table_end, dir_word_table_end;
extern int fnc_word_table_end, c_word_table_end;

/*
----------------------------------------
	human68.c ïœêîÇÃèâä˙ílê›íË
----------------------------------------
*/

#ifdef	human68def

_tinf tinf[MAXMOD] = {
	{ 64, 32, FALSE,  3},
	{ 64, 32, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 64, 32, FALSE,  3},
	{ 64, 32, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 64, 32, FALSE,  4},
	{ 64, 32, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 64, 32, FALSE,  5},
	{ 64, 32, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 32, 16, FALSE, -1},
	{ 96, 32,  TRUE,  0},
	{128, 26, FALSE, -1},
	{128, 53, FALSE, -1}
};

TERM term = {
	NROW,
	NROW - 1,
	NCOL,
	NCOL - 1,
	MARGIN,
	SCRSIZ,
	NPAUSE,
	H68getc,
};

#else

extern _tinf tinf[MAXMOD];
extern TERM term;

#endif /* human68def */
