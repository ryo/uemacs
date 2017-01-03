/*
 * $Id: display.c,v 1.114 2017/01/02 18:14:48 ryo Exp $
 *
 * The functions in this file handle redisplay. There are two halves, the
 * ones that update the virtual display screen, and the ones that ake the
 * physical display screen the same as the virtual display screen. These
 * functions use hints that are left in the windows by the commands.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#if 1
#include <regex.h>
#endif
#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "keyword.h"
#include "kanji.h"

typedef struct VIDEO {
	struct line_context v_ctx;

	unsigned char v_flag;	/* Flags */
#define VFCHG	0x01		/* Changed flag */
	unsigned int v_mode;	/* buffer mode */
	int v_lineno;
	void *v_hashroot;	/* buffer's b_hashroot */
	unsigned char *v_mb;	/* multibyte type */
	unsigned char *v_text;	/* Screen data */
	unsigned short *v_attr;	/* Screen attribute */
#define V_ATTR_FGCOLOR_MASK	0x000F
#define V_ATTR_BGCOLOR_MASK	0x00F0
#define V_ATTR_COLORMASK	(V_ATTR_FGCOLOR_MASK | V_ATTR_BGCOLOR_MASK)
#define V_ATTR_FGCOLOR(n)	((n) & V_ATTR_FGCOLOR_MASK)
#define V_ATTR_BGCOLOR(n)	(((n) & V_ATTR_FGCOLOR_MASK) << 4)
#define V_ATTR_ATTR2FG(a)	((a) & V_ATTR_FGCOLOR_MASK)
#define V_ATTR_ATTR2BG(a)	(((a) & V_ATTR_BGCOLOR_MASK) >> 4)

#define V_ATTR_COLOR_NORMAL	0x0000
#define V_ATTR_COLOR_RED	0x0001
#define V_ATTR_COLOR_GREEN	0x0002
#define V_ATTR_COLOR_YELLOW	0x0003
#define V_ATTR_COLOR_BLUE	0x0004
#define V_ATTR_COLOR_MAGENTA	0x0005
#define V_ATTR_COLOR_CYAN	0x0006
#define V_ATTR_COLOR_WHITE	0x0007
#define V_ATTR_COLOR_BLACK	0x0008
#define V_ATTR_BOLD		0x0100
#define V_ATTR_LINE		0x0200
#define V_ATTR_REV		0x0400
#define V_ATTR_EMP		0x0800
#define V_ATTR_SPECIALCHAR	0x1000
#define V_ATTR_LABEL		0x2000
#define V_ATTR_RESERVED4	0x4000
#define V_ATTR_RESERVED8	0x8000
} VIDEO;

static VIDEO **vscreen;		/* Virtual screen. */
static VIDEO **pscreen;		/* Physical screen. */

static int mlformat(WINDOW *, char *, char *);
static int commentcmp(const char *, char *, int);
static int commentcmp2(const char *, unsigned char *, unsigned short *, int, int);
static int updateline(int, struct VIDEO *, struct VIDEO *);
static void updateline_done(struct VIDEO *, struct VIDEO *);
static int vtputs(unsigned char *, int, unsigned int, int, int, struct line_context *);

static void update_line_context(LINE *);
void update_line_context_all(BUFFER *);


int vtrow;	/* Row location of SW cursor */
int vtcol;	/* Column location of SW cursor */

static
int
getlinenum(WINDOW *wp)
{
	int i;
	LINE *lp, *cp;

	cp = wp->w_linep;
	for (i = 0, lp = wp->w_bufp->b_linep; lp != cp; lp = lforw(lp), i++)
		;
	return i;
}

/*
 * Initialize the data structures used by the display code. The edge vectors
 * used to access the screens are set up. The operating system's terminal I/O
 * channel is set up. All the other things get initialized at compile time.
 * The original window has "WFCHG" set, so that it will get completely
 * redrawn on the first call to "update".
 */
int
vtinit(void)
{
	int i;
	VIDEO *vp;

	TTopen();		/* open the screen */
	TTkopen();		/* open the keyboard */
	TTrev(FALSE);
	vscreen = (VIDEO **)MALLOC(term.t_mrow * sizeof(VIDEO *));

	if (vscreen == NULL)
		meexit(1);

	pscreen = (VIDEO **)MALLOC(term.t_mrow * sizeof(VIDEO *));

	if (pscreen == NULL)
		meexit(1);

	for (i = 0; i < term.t_mrow; ++i) {
		vp = (VIDEO *)MALLOC(sizeof(VIDEO));
		if (vp == NULL)
			meexit(1);

		memset(vp, sizeof(VIDEO), 0);

		vp->v_flag = 0;
		vp->v_mb = (void*)MALLOC(term.t_mcol * sizeof(vp->v_mb[0]));
		vp->v_text = (void*)MALLOC(term.t_mcol * sizeof(vp->v_text[0]));
		vp->v_attr = (void*)MALLOC(term.t_mcol * sizeof(vp->v_attr[0]));
		if (vp->v_text == NULL || vp->v_attr == NULL)
			meexit(1);
		memset(vp->v_attr, 0, term.t_mcol * sizeof(vp->v_attr[0]));

		vscreen[i] = vp;


		vp = (VIDEO *)MALLOC(sizeof(VIDEO));
		if (vp == NULL)
			meexit(1);

		vp->v_flag = 0;
		vp->v_mb = (void*)MALLOC(term.t_mcol * sizeof(vp->v_mb[0]));
		vp->v_text = (void*)MALLOC(term.t_mcol * sizeof(vp->v_text[0]));
		vp->v_attr = (void*)MALLOC(term.t_mcol * sizeof(vp->v_attr[0]));
		if (vp->v_text == NULL || vp->v_attr == NULL)
			meexit(1);
		memset(vp->v_attr, 0, term.t_mcol * sizeof(vp->v_attr[0]));

		pscreen[i] = vp;

	}
	return TRUE;
}

/* free up all the dynamically allocated video structures */
static void
video_free(struct VIDEO *vp)
{
	if (vp->v_mb)
		FREE(vp->v_mb);
	if (vp->v_text)
		FREE(vp->v_text);
	if (vp->v_attr)
		FREE(vp->v_attr);

	FREE(vp);
}

int
vtfree(void)
{
	int i;
	for (i = 0; i < term.t_mrow; ++i) {
		video_free(vscreen[i]);
		video_free(pscreen[i]);
	}
	FREE(vscreen);
	FREE(pscreen);

	return TRUE;
}

/*
 * Clean up the virtual terminal system, in anticipation for a return to the
 * operating system. Move down to the last line and clear it out (the next
 * system prompt will be written in the line). Shut down the channel to the
 * terminal.
 */
int
vttidy(void)
{
	mlerase();
	movecursor(term.t_nrow, 0);
	TTflush();
	TTclose();
	TTflush();
	TTkclose();

	return TRUE;
}


/*
 * Set the virtual cursor to the specified row and column on the virtual
 * screen. There is no checking for nonsense values; this might be a good
 * idea during the early stages.
 */
int
vtmove(int row, int col)
{
	vtrow = row;
	vtcol = col;

	return TRUE;
}

/*
 * Write a character to the virtual screen. The virtual row and column are
 * updated. If we are not yet on left edge, don't print it yet. If the line
 * is too long put a "$" in the last column. This routine only puts printing
 * characters into the virtual terminal buffers. Only column overflow is
 * checked.
 */
int
vtputc(int c, unsigned int tabs, int attr, int mbctype)
{
	VIDEO *vp;		/* ptr to line being updated */
	int tnc = term.t_ncol;

	vp = vscreen[vtrow];
	vp->v_flag |= VFCHG;

	if (c == '\t') {
		if (distab) {
			vtputc('|', tabs, tabcol | V_ATTR_SPECIALCHAR, 0);
			while (((vtcol - leftmargin + taboff) % (tabs)) != 0) {
				vtputc('.', tabs, tabcol | V_ATTR_SPECIALCHAR, CT_ANK);
			}
		} else {
			int tlen = tabs - ((vtcol - leftmargin + taboff) % tabs);
			for (; tlen > 0; tlen--)
				vtputc(' ', tabs, 0 | V_ATTR_SPECIALCHAR, CT_ANK);
		}
	} else {
		if (vtcol >= tnc) {
			vtcol++;
			vp->v_attr[tnc - 1] = V_ATTR_FGCOLOR(V_ATTR_COLOR_MAGENTA) | V_ATTR_SPECIALCHAR;
			vp->v_text[tnc - 1] = '$';
			vp->v_mb[tnc - 1] = CT_ANK;

			if (tnc > 2 && vp->v_mb[tnc - 2] == CT_KJ1) {
				vp->v_attr[tnc - 2] = V_ATTR_FGCOLOR(V_ATTR_COLOR_MAGENTA) | V_ATTR_SPECIALCHAR;
				vp->v_text[tnc - 2] = '$';
				vp->v_mb[tnc - 2] = CT_ANK;
			}
		} else {
			if (c < 0x20 || c == 0x7F) {
				vtputc('^', tabs, V_ATTR_FGCOLOR(V_ATTR_COLOR_CYAN) | V_ATTR_SPECIALCHAR, CT_ANK);
				vtputc(c ^ 0x40, tabs, V_ATTR_FGCOLOR(V_ATTR_COLOR_CYAN) | V_ATTR_SPECIALCHAR, CT_ANK);
			} else {
				if (vtcol >= 0) {
					vp->v_attr[vtcol] = attr;
					vp->v_text[vtcol] = c;
					vp->v_mb[vtcol] = mbctype;
				}
				vtcol++;
			}
		}
	}
	return TRUE;
}

int
vtputs(unsigned char *buf, int nlen, unsigned int tabs, int attr, int reacheof, struct line_context *ctx)
{
	VIDEO *vp;		/* ptr to line being updated */
	unsigned int c;
	int mbstate;

	vp = vscreen[vtrow];

	memset(&vp->v_attr[0], 0, term.t_ncol * sizeof(vp->v_attr[0]));
	memset(&vp->v_mb[0], 0, term.t_ncol * sizeof(vp->v_mb[0]));

	if (ctx != NULL) {
		vp->v_ctx = *ctx;
	} else {
		memset(&vp->v_ctx, 0, sizeof(vp->v_ctx));
	}

	if (vp->v_lineno && leftmargin) {
		char labelstr[NSTRING];
		char *linep = labelstr;

		if (disnum) {
			sprintf(linep, "%5d ", vp->v_lineno);
			linep += 6;
		}
		if (dislabel && (vp->v_ctx.ctx_label[0] != '\0')) {
			sprintf(linep, "%-*s", CTX_LABELLEN, vp->v_ctx.ctx_label);

			if (vp->v_ctx.ctx_labelno >= 10000) {
				sprintf(linep + CTX_LABELLEN - 7, ":%5d", vp->v_ctx.ctx_labelno);
			} else if (vp->v_ctx.ctx_labelno >= 1000) {
				sprintf(linep + CTX_LABELLEN - 6, ":%4d", vp->v_ctx.ctx_labelno);
			} else if (vp->v_ctx.ctx_labelno >= 100) {
				sprintf(linep + CTX_LABELLEN - 5, ":%3d", vp->v_ctx.ctx_labelno);
			} else if (vp->v_ctx.ctx_labelno >= 10) {
				sprintf(linep + CTX_LABELLEN - 4, ":%2d", vp->v_ctx.ctx_labelno);
			} else {
				sprintf(linep + CTX_LABELLEN - 3, ":%d", vp->v_ctx.ctx_labelno);
			}
			linep += CTX_LABELLEN;
		} else {
			memset(linep, ' ', CTX_LABELLEN);
			linep += CTX_LABELLEN;
		}

		*--linep = '|';
		*++linep = '\0';

		linep = labelstr;
		while (*linep) {
			vtputc(*linep++, tabs, numcol | V_ATTR_LABEL, CT_ANK);
		}
	}

	mbstate = CT_ANK;
	for (; nlen > 0; nlen--) {
		c = *buf++;

		if ((mbstate == CT_ANK) &&
		    (chctype(c) == CT_KJ1))
			mbstate = CT_KJ1;
		else if (mbstate == CT_KJ1)
			mbstate = CT_KJ2;

		vtputc(c, tabs, attr, mbstate);

		if (mbstate == CT_KJ2)
			mbstate = CT_ANK;
	}

	if (reacheof && dispcr)
		vtputc(crchar, tabs, crcol | V_ATTR_SPECIALCHAR, mbstate);

	return TRUE;
}


/*
 * Erase from the end of the software cursor to the end of the line on which
 * the software cursor is located.
 */
int
vteeol(void)
{
	VIDEO *vp;
	int tnc = term.t_ncol;

	vp = vscreen[vtrow];
	while (vtcol < tnc) {
		vp->v_text[vtcol] = ' ';
		vp->v_attr[vtcol] = 0;
		vp->v_mb[vtcol] = 0;
		vtcol++;
	}

	return TRUE;
}

/*
 * upscreen:
 *   user routine to force a screen update always finishes
 *   complete update
 */
int
upscreen(int f, int n)
{
	update(TRUE);
	return TRUE;
}

/*
 * Make sure that the display is right. This is a three part process. First,
 * scan through all of the windows looking for dirty ones. Check the framing,
 * and refresh the screen. Second, make sure that "currow" and "curcol" are
 * correct for the current window. Third, make the virtual and physical
 * screens the same.
 */
int
update(int force)	/* force update past type ahead? */
{
	WINDOW *wp;

#if TYPEAH
	if (typahead())
		return TRUE;
#endif
#if VISMAC == 0
	if (force == FALSE && kbdmode == PLAY)
		return TRUE;
#endif

	/* update any windows that need refreshing */
	wp = wheadp;
	while (wp != NULL) {
		if (wp->w_flag) {
			/* if the window has changed, service it */
			reframe(wp);	/* check the framing */

			if (force)
				wp->w_flag = WFHARD | WFFORCE | WFMODE;

#if 1
			 /* XXX: force update all lines for update line-context effect */
			if (commentcol | funccol) {
				wp->w_flag |= WFHARD | WFFORCE;
			}
#endif

#if 0
{
	FILE *fp = fopen("debug.log", "a");
	fprintf(fp, "w_flag=%02x\n", wp->w_flag);
	fclose(fp);
}
#endif
			if (wp->w_flag & (WFHARD | WFFORCE))
				updall(wp);	/* update all lines */
			else if ((wp->w_flag & (WFMOVE | WFEDIT)) == WFEDIT)
				updone(wp);	/* update EDITed line */
			else
				updall(wp);	/* update all lines */

			if (wp->w_flag & WFMODE)
				modeline(wp);	/* update modeline */

			wp->w_flag = 0;
			wp->w_force = 0;
		}
		/* on to the next window */
		wp = wp->w_wndp;
	}

	/* recalc the current hardware cursor location */
	updpos();

	/* if screen is garbage, re-plot it */
	if (sgarbf != FALSE)
		updgar();

	/* update the virtual screen to the physical screen */
	updupd(force);

	/* update the cursor and flush the buffers */
	movecursor_num(currow, curcol - lbound);
	TTflush();

	return TRUE;
}


/*
 * reframe: check to see if the cursor is on in the window and re-frame it
 *          if needed or wanted
 */
int
reframe(WINDOW *wp)
{
	LINE *lp;		/* search pointer */
	LINE *rp;		/* reverse search pointer */
	LINE *hp;		/* ptr to header line in buffer */
	LINE *tp;		/* temp debugging pointer */
	int i;			/* general index/# lines to scrolsl */
	int nlines;		/* number of lines in current window */

	/* figure out our window size */
	nlines = wp->w_ntrows;
	if (modeflag == FALSE)
		nlines++;

	/* if not a requested reframe, check for a needed one */
	if ((wp->w_flag & WFFORCE) == 0) {
		lp = wp->w_linep;
		for (i = 0; i < nlines; i++) {

			/* if the line is in the window, no reframe */
			if (lp == wp->w_dotp)
				return TRUE;

			/* if we are at the end of the file, reframe */
			if (lp == wp->w_bufp->b_linep)
				break;

			/* on to the next line */
			lp = lforw(lp);
		}
	}
	/* reaching here, we need a window refresh */
	i = wp->w_force;

	/*
	 * if smooth scrolling is enabled, first.. have we gone off the top?
	 */
	if (sscroll && ((wp->w_flag & WFFORCE) == 0)) {
		/* search thru the buffer looking for the point */
		tp = lp = rp = wp->w_linep;
		hp = wp->w_bufp->b_linep;
		while ((lp != hp) || (rp != hp)) {

			/* did we scroll downward? */
			if (lp == wp->w_dotp) {
				i = nlines - 1;
				break;
			}
			/* did we scroll upward? */
			if (rp == wp->w_dotp) {
				i = 0;
				break;
			}
			/* advance forward and back */
			if (lp != hp)
				lp = lforw(lp);
			if (rp != hp)
				rp = lback(rp);
			/* problems????? */
			if (lp == tp || rp == tp) {
				printf("BUG IN SMOOTH SCROLL--GET DAN!\n");
				TTgetc();
			}
		}
		/* how far back to reframe? */
	} else
		if (i > 0) {	/* only one screen worth of lines max */
			if (--i >= nlines)
				i = nlines - 1;
		} else
			if (i < 0) {	/* negative update???? */
				i += nlines;
				if (i < 0)
					i = 0;
			} else
				i = nlines / 2;

	/* backup to new line at top of window */
	lp = wp->w_dotp;
	while (i != 0 && lback(lp) != wp->w_bufp->b_linep) {
		--i;
		if (i < 0) {
			printf("OTHER BUG IN DISPLAY --- GET DAN!!!\n");
			TTgetc();
		}
		lp = lback(lp);
	}

	/* and reset the current line at top of window */
	wp->w_linep = lp;
	wp->w_flag |= WFHARD;
	wp->w_flag &= ~WFFORCE;
	return TRUE;
}

#if 1
/* XXX */
static regex_t re_pattern_funcstart;
static regex_t re_pattern_funcend;
#define RE_NMATCH	3
static regmatch_t re_pmatch[RE_NMATCH];

static char *regex_funcmatch(char *, int);

static char *
regex_funcmatch(char *text, int len)
{
	static int re_initted = 0;
	static char result[NSTRING];
	int r;

	if (re_initted == 0) {
		regcomp(&re_pattern_funcstart,
		    "^("
		        "inline[ \t]|"
		        "extern[ \t]|"
		        "static[ \t]|"
		        "const[ \t\\*]+|"
		        "void[ \t\\*]+|"
		        "unsigned[ \t\\*]+|"
		        "signed[ \t\\*]+|"
		        "char[ \t\\*]+|"
		        "short[ \t\\*]+|"
		        "int[ \t\\*]+|"
		        "long[ \t\\*]+|"
		        "float[ \t\\*]+|"
		        "double[ \t\\*]+|"
		        "[_a-z0-9]+_t[ \t\\*]+|"
		        "u_char[ \t\\*]+|"
		        "u_short[ \t\\*]+|"
		        "u_int[ \t\\*]+|"
		        "u_long[ \t\\*]+|"
		        "unchar[ \t\\*]+|"
		        "ushort[ \t\\*]+|"
		        "uint[ \t\\*]+|"
		        "ulong[ \t\\*]+|"
		        "struct[ \t\\*]*[0-9A-Za-z_]+[ \t\\*]+|"
		        "\\*[ \t]*)*"
		    "([0-9A-Za-z_]+)\\(.*[^;]?$", REG_EXTENDED);
		regcomp(&re_pattern_funcend, "^}", REG_EXTENDED);
		re_initted = 1;
	}

	/* start of function? */
	re_pmatch[0].rm_so = 0;
	re_pmatch[0].rm_eo = len;
	r = regexec(&re_pattern_funcstart, text, RE_NMATCH, &re_pmatch[0], REG_STARTEND);
	if (r == 0) {
		int l = re_pmatch[2].rm_eo - re_pmatch[2].rm_so;
		memcpy(result, text + re_pmatch[2].rm_so, l);
		result[l] = '\0';
		return result;
	}

	/* end of function? */
	re_pmatch[0].rm_so = 0;
	re_pmatch[0].rm_eo = len;
	r = regexec(&re_pattern_funcend, text, RE_NMATCH, &re_pmatch[0], REG_STARTEND);
	if (r == 0) {
		result[0] = '\0';
		return result;
	}
	return NULL;
}

#endif

static void
update_line_context(LINE *lp)
{
	LINE *prev;
	char *p;

	prev = lp->l_bp;
	if (prev != NULL)
		lp->l_context.ctx_incomment_begin = prev->l_context.ctx_incomment_end;
	else
		lp->l_context.ctx_incomment_begin = 1;

	lp->l_context.ctx_incomment_end = lp->l_context.ctx_incomment_begin;

#if 1
	{
		char *func;

		func = regex_funcmatch((char *)lp->l_text, llength(lp));
		if (func == NULL) {
			if (prev != NULL) {
				memcpy(lp->l_context.ctx_label, prev->l_context.ctx_label, sizeof lp->l_context.ctx_label);
				lp->l_context.ctx_labelno = prev->l_context.ctx_labelno + 1;
			} else {
				memset(lp->l_context.ctx_label, 0, sizeof lp->l_context.ctx_label);
				lp->l_context.ctx_labelno = 0;
			}
		} else {
			strncpy(lp->l_context.ctx_label, func, sizeof lp->l_context.ctx_label);
			lp->l_context.ctx_label[sizeof(lp->l_context.ctx_label) - 1] = '\0';
			lp->l_context.ctx_labelno = 0;
		}
	}
#endif


	if (llength(lp) >= 1) {
		int cin_len = strlen(comment_in);
		int cout_len = strlen(comment_out);
		int cline_len = strlen(comment_line);
		int left;
		int in_singlequote = 0;
		int in_doublequote = 0;

		p = (char *)lp->l_text;
		for (left = llength(lp); left > 0; p++, left--) {
			unsigned char c = *p;

			if (chctype(c) != CT_ANK) {
				p++;
				left--;
				continue;
			}

			if (in_singlequote) {
				if (c == '\'')
					in_singlequote = 0;
				continue;
			}
			if (in_doublequote) {
				if (c == '"')
					in_doublequote = 0;
				continue;
			}
			if ((lp->l_context.ctx_incomment_end == 0) && (c == '\'')) {
				in_singlequote = 1;
				continue;
			}
			if ((lp->l_context.ctx_incomment_end == 0) && (c == '"')) {
				in_doublequote = 1;
				continue;
			}

			if (cin_len && (cin_len <= left) &&
			    (commentcmp(p, comment_in, cin_len) == 0))
				lp->l_context.ctx_incomment_end = 1;

			if (cout_len && (cout_len <= left) &&
			    (commentcmp(p, comment_out, cout_len) == 0))
				lp->l_context.ctx_incomment_end = 0;

			if ((lp->l_context.ctx_incomment_end == 0) &&
			    cline_len && (commentcmp(p, comment_line, cline_len) == 0)) {
				break;
			}
		}
	}
}

void
update_line_context_all(BUFFER *bp)
{
	LINE *lp;

	lp = bp->b_linep;
	do {
		update_line_context(lp);
		lp = lforw(lp);
	} while (lp != bp->b_linep);

}

/* updone: update the current line to the virtual screen */
int
updone(WINDOW *wp)	/* window to update current line in */
{
	LINE *lp;	/* line to update */
	int sline;	/* physical screen line to update */
	int lineno = leftmargin ? getlinenum(wp) : 0;

	/* search down the line we want */
	lp = wp->w_linep;
	sline = wp->w_toprow;
	while (lp != wp->w_dotp) {
		++sline;
		lp = lforw(lp);
	}

	/* and update the virtual line */
	vscreen[sline]->v_lineno = leftmargin ? lineno + sline : 0;
	vscreen[sline]->v_flag |= VFCHG;
	vscreen[sline]->v_mode = wp->w_bufp->b_mode;
	vscreen[sline]->v_hashroot = wp->w_bufp->b_hashroot;

	taboff = wp->w_fcol;
	vtmove(sline, -taboff);

	update_line_context(lp);
	vtputs(&lp->l_text[0], llength(lp), wp->w_bufp->b_tabs, 0, 1, &lp->l_context);

	vteeol();
	taboff = 0;

	return TRUE;
}

/* updall: update all the lines in a window on the virtual screen */
int
updall(WINDOW *wp)	/* window to update lines in */
{
	LINE *lp;		/* line to update */
	int sline;		/* physical screen line to update */
	int nlines;		/* number of lines in the current window */
	int reach_eof = 0;
	int lineno;

	/* search down the lines, updating them */
	lp = wp->w_linep;
	sline = wp->w_toprow;
	nlines = wp->w_ntrows;


	if (modeflag == FALSE)
		nlines++;
	taboff = wp->w_fcol;


	lineno = leftmargin ? getlinenum(wp) : 0;

	while (sline < wp->w_toprow + nlines) {

		/* and update the virtual line */
		vscreen[sline]->v_lineno = reach_eof ? 0 : lineno++;
		vscreen[sline]->v_flag |= VFCHG;
		vscreen[sline]->v_mode = wp->w_bufp->b_mode;
		vscreen[sline]->v_hashroot = wp->w_bufp->b_hashroot;

		vtmove(sline, -taboff);
		if (lp != wp->w_bufp->b_linep) {
			/* if we are not at the end */
			update_line_context(lp);
			vtputs(&lp->l_text[0], llength(lp), wp->w_bufp->b_tabs, 0, 1, &lp->l_context);
			lp = lforw(lp);
		} else {
			if (!reach_eof) {
				if (dispeof) {
					if (leftmargin && !vscreen[sline]->v_lineno) {
						vtputs((unsigned char *)"      [EOF]", 11, wp->w_bufp->b_tabs, eofcol, 0, NULL);
					} else {
						vtputs((unsigned char *)"[EOF]", 5, wp->w_bufp->b_tabs, eofcol, 0, NULL);
					}

				}
				reach_eof = 1;
			} else {
				vtputs((unsigned char *)"", 0, wp->w_bufp->b_tabs, eofcol, 0, NULL);
			}
		}

		/* make sure we are on screen */
		if (vtcol < 0)
			vtcol = 0;

		/* on to the next one */
		vteeol();
		++sline;
	}
	taboff = 0;

	return TRUE;
}

/*
 * updpos: update the position of the hardware cursor and handle extended
 * lines. This is the only update for simple moves.
 */
int
updpos(void)
{
	LINE *lp;
	int c;
	int i;
	int tnc = term.t_ncol;

	/* find the current row */
	lp = curwp->w_linep;
	currow = curwp->w_toprow;
	while (lp != curwp->w_dotp) {
		++currow;
		lp = lforw(lp);
	}

	/* find the current column */
	curcol = 0;
	i = 0;
	while (i < curwp->w_doto) {
		c = lgetc(lp, i++);
		if (c == '\t')
			curcol += -(curcol % curwp->w_bufp->b_tabs) + (curwp->w_bufp->b_tabs - 1);
		else
			if (c < 0x20 || c == 0x7f)
				++curcol;

		++curcol;
	}

	/* adjust by the current first column position */
	curcol -= curwp->w_fcol;

	/* make sure it is not off the left side of the screen */
	while (curcol < 0) {
		if (curwp->w_fcol >= hjump) {
			curcol += hjump;
			curwp->w_fcol -= hjump;
		} else {
			curcol += curwp->w_fcol;
			curwp->w_fcol = 0;
		}
		curwp->w_flag |= WFHARD | WFMODE;
	}

	/* if horizontall scrolling is enabled, shift if needed */
	if (hscroll) {
		while (curcol >= tnc - leftmargin - 1) {
			curcol -= hjump;
			curwp->w_fcol += hjump;
			curwp->w_flag |= WFHARD | WFMODE;
		}
	} else {
		/* if extended, flag so and update the virtual line image */
		if (curcol >= tnc - leftmargin - 1) {
			vscreen[currow]->v_flag |= VFCHG;
			updext();
		} else
			lbound = 0;
	}

	/* update the current window if we have to move it around */
	if (curwp->w_flag & WFHARD)
		updall(curwp);
	if (curwp->w_flag & WFMODE)
		modeline(curwp);
	curwp->w_flag = 0;

	return TRUE;
}

/*
 * updgar: if the screen is garbage, clear the physical screen and the
 * virtual screen and force a full update
 */
int
updgar(void)
{
	int i;
	int j;

	for (i = 0; i < term.t_nrow; ++i) {
		int tnc = term.t_ncol;

		vscreen[i]->v_flag |= VFCHG;
		for (j = 0; j < tnc; ++j) {
			pscreen[i]->v_text[j] = ' ';
			pscreen[i]->v_attr[j] = 0;
			pscreen[i]->v_mb[j] = 0;
		}
	}

	movecursor(0, 0);	/* Erase the screen. */
	TTattr(0);
	(*term.t_eeop)();
	sgarbf = FALSE;		/* Erase-page clears */
	mpresf = FALSE;		/* the message area. */

	mlerase();

	return TRUE;
}

/* updupd: update the physical screen from the virtual screen */
int
updupd(int force)	/* forced update flag */
{
	VIDEO *vp1;
	int i;

	for (i = 0; i < term.t_nrow; ++i) {
		vp1 = vscreen[i];

		/* for each line that needs to be updated */
		if ((vp1->v_flag & VFCHG) != 0) {
#if TYPEAH
			if (typahead())
				return TRUE;
#endif
			updateline(i, vp1, pscreen[i]);
		}
	}
	return TRUE;
}

/*
 * updext: update the extended line which the cursor is currently on at a
 * column greater than the terminal width. The line will be scrolled right or
 * left to let the user see where the cursor is
 */
int
updext(void)
{
	int rcursor;		/* real cursor location */
	LINE *lp;		/* pointer to current line */
	int tnc = term.t_ncol;

	/*
	 * calculate what column the real cursor will end up in
	 */
	rcursor = ((curcol - tnc) % term.t_scrsiz) + term.t_margin;
	lbound = curcol - rcursor + 1;
	taboff = lbound + curwp->w_fcol;

	/*
	 * scan through the line outputing characters
	 * to the virtual screen once we reach the left edge
	 */
	vtmove(currow, -taboff);	/* start scanning offscreen */
	lp = curwp->w_dotp;		/* line to output */

	update_line_context(lp);
	vtputs(&lp->l_text[0], llength(lp), curwp->w_bufp->b_tabs, 0, 1, &lp->l_context);

	/*
	 * truncate the virtual line, restore tab offset
	 */
	vteeol();
	taboff = 0;

	/*
	 * and put a '$' in column 1
	 */
	vscreen[currow]->v_text[0] = '$';

	return TRUE;
}


/*
 * vp1: virtual screen image
 * vp2: physical screen image
 */
static void
updateline_done(struct VIDEO *vp1, struct VIDEO *vp2)
{
	memcpy(vp2->v_text, vp1->v_text, term.t_mcol * sizeof(vp1->v_text[0]));
	memcpy(vp2->v_attr, vp1->v_attr, term.t_mcol * sizeof(vp1->v_attr[0]));
}

static int
commentcmp(const char *word, char *text, int left)
{
	unsigned char c;

	for (; left > 0; left--) {
		c = *word++;
		if (c == '_') {
			if ((*text != ' ') &&
			    (*text != '\t'))
				return -1;
			text++;

		} else {
			if (*text++ != c)
				return -1;
		}
	}
	return 0;

}

static int
commentcmp2(const char *word, unsigned char *text, unsigned short *attr, int len, int left)
{
	unsigned char c;
	if (left < len)
		len = left;

	for (; len > 0; len--) {
		c = *word++;

		if (c == '_') {
			if ((*text != ' ') &&
			    (*text != '\t'))
				return -1;
			attr++;
			text++;
		} else {
			if (*attr++ & (V_ATTR_SPECIALCHAR | V_ATTR_LABEL))
				return -1;

			if (*text++ != c)
				return -1;
		}
	}
	return 0;
}


#define ISFUNCLABEL(c)	\
		((((c) >= '0') && ((c) <= '9')) ||	\
		 (((c) >= 'A') && ((c) <= 'Z')) ||	\
		 (((c) >= 'a') && ((c) <= 'z')) ||	\
		 ((c) == '_'))

static int
isfunctioncall(const unsigned char *str, int len)
{
	int match, i;
	unsigned char c;

	match = 0;
	for (i = 0; i < len; i++) {
		c = str[i];
		if (ISFUNCLABEL(c)) {
			match++;
		} else if (match && (c == '(')) {
			return match;
		} else {
			break;
		}
	}
	return 0;
}


/*
 * row: row of screen to update
 * vp1: virtual screen image
 * vp2: physical screen image
 */
static int
updateline(int row, struct VIDEO *vp1, struct VIDEO *vp2)
{
	/* UPDATELINE code for all other versions */
	unsigned char *kp1;
	unsigned char *cp1;
	unsigned char *cp2;
	unsigned short *ap1;
	unsigned short *ap2;

	unsigned char *cp3;
	unsigned char *cp4;
	unsigned short *ap3;
	unsigned short *ap4;

	unsigned char *cp5;
	unsigned short *ap5;
	int nbflag;		/* non-blanks to the right flag? */
	int upcol;		/* update column (KRS) */
	int tnc = term.t_ncol;

	/* set up pointers to virtual and physical lines */
	kp1 = &vp1->v_mb[0];
	cp1 = &vp1->v_text[0];
	cp2 = &vp2->v_text[0];
	ap1 = &vp1->v_attr[0];
	ap2 = &vp2->v_attr[0];

	/*
	 * set attribute for emphasis
	 */
	if (vp1->v_mode & MDEMPHASIS) {
		int i, j, left, len;

		for (i = 0; i < tnc; i++) {
			left = tnc - i;

			if (kp1[i] != CT_ANK) {
				i++;	/* skip 2byte chars */
				continue;
			}

			if (!iskeysym(vp1->v_hashroot, cp1[i]))
				continue;

			/* count keyword length */
			for (len = 0; len < left && iskeysym(vp1->v_hashroot, cp1[i + len]); len++)
				;

			if (len >= 1 && iskeyword(vp1->v_hashroot, (char *)(cp1 + i), len)) {
				for (j = i; j < i + len; j++) {
					if (emphasisbold & 0x01)
						ap1[j] |= V_ATTR_BOLD;
					if (emphasisbold & 0x02)
						ap1[j] |= V_ATTR_LINE;
					if (emphasisbold & 0x04)
						ap1[j] |= V_ATTR_REV;
					if (emphasiscol) {
						ap1[j] &= ~V_ATTR_COLORMASK;
						ap1[j] |= emphasiscol;
					}
				}
			}

			i += len;
		}
	}

#if 1 /* function emphasis */
	if ((vp1->v_mode & MDCMOD) && funccol) {
		int i, j, left, len;

		for (i = 0; i < tnc; i++) {
			left = tnc - i;

			if (kp1[i] != CT_ANK) {
				i++;	/* skip 2byte chars */
				continue;
			}

			len = isfunctioncall(&cp1[i], left);
			if (len) {
				for (j = i; j < i + len; j++) {
					ap1[j] &= ~V_ATTR_COLORMASK;
					ap1[j] |= funccol;
				}
			}
			i += len;
		}
	}
#endif

#if 1 /* comment emphasis */
	if ((vp1->v_mode & MDCMOD) && commentcol) {
		int i, left;
		unsigned char *p, *pe;
		int in_comment = 0;
		int in_singlequote = 0;
		int in_doublequote = 0;
		int cin_len = strlen(comment_in);
		int cout_len = strlen(comment_out);
		int cline_len = strlen(comment_line);

		if (vp1->v_ctx.ctx_incomment_begin)
			in_comment = 1;

		i = 0;
		p = cp1;
		pe = &cp1[tnc];

		for (i = 0; p < pe; p++, i++) {
			left = pe - p;

			if (kp1[i] != CT_ANK) {
				if (in_comment) {
					ap1[i] &= ~V_ATTR_COLORMASK;
					ap1[i] |= commentcol;
				}
				continue;
			}

			if (in_singlequote || in_doublequote) {
				if ((ap1[i] & (V_ATTR_SPECIALCHAR | V_ATTR_LABEL)) == 0) {
					ap1[i] &= ~V_ATTR_COLORMASK;
					ap1[i] |= quotecol;
				}
				if ((i > 0) && (cp1[i - 1] == '\\'))
					continue;
			}

			if (in_singlequote) {
				if (commentcmp2("'", &cp1[i], &ap1[i], 1, left) == 0)
					in_singlequote = 0;
				continue;
			}
			if (in_doublequote) {
				if (commentcmp2("\"", &cp1[i], &ap1[i], 1, left) == 0)
					in_doublequote = 0;
				continue;
			}
			if (in_comment == 0) {
				if (commentcmp2("'", &cp1[i], &ap1[i], 1, left) == 0) {
					in_singlequote = 1;
				}
				if (commentcmp2("\"", &cp1[i], &ap1[i], 1, left) == 0) {
					in_doublequote = 1;
				}

				if (in_singlequote || in_doublequote) {
					if ((ap1[i] & (V_ATTR_SPECIALCHAR | V_ATTR_LABEL)) == 0) {
						ap1[i] &= ~V_ATTR_COLORMASK;
						ap1[i] |= quotecol;
					}
					continue;
				}
			}

			if ((in_comment == 0) && cin_len &&
			    (commentcmp2(comment_in, &cp1[i], &ap1[i], cin_len, left) == 0)) {
				in_comment = 1;

			} else if ((in_comment == 0) && cline_len &&
			    (commentcmp2(comment_line, &cp1[i], &ap1[i], cline_len, left) == 0)) {
				in_comment = 2;	/* never clear */

			} else if ((in_comment == 1) && cout_len &&
			    (commentcmp2(comment_out, &cp1[i], &ap1[i], cout_len, left) == 0)) {

				int j;
				for (j = i; (p < pe) && (i < j + cout_len); p++, i++) {
					if ((ap1[i] & (V_ATTR_SPECIALCHAR | V_ATTR_LABEL)) == 0) {
						ap1[i] &= ~V_ATTR_COLORMASK;
						ap1[i] |= commentcol;
					}
				}

				in_comment = 0;
				continue;
			}

			if (in_comment) {
				if ((ap1[i] & (V_ATTR_SPECIALCHAR | V_ATTR_LABEL)) == 0) {
					ap1[i] &= ~V_ATTR_COLORMASK;
					ap1[i] |= commentcol;
				}

				if ((ap1[i] & V_ATTR_SPECIALCHAR) && (cp1[i] == '$') &&
				    ((p + 1) == pe)) {
					in_comment = 0;
				}
			}
		}
	}
#endif

#if 1 /* ZENKAKU SPACE */
	/* zenkaku space (0x8140) visible */
	/* $diszenspc */
	if (diszen) {
		int i;

		for (i = 0; i < tnc; i++) {
			if (kp1[i] == CT_KJ1) {
				if ((cp1[i] == 0x81) && (cp1[i + 1] == 0x40)) {
					ap1[i] &= ~V_ATTR_COLORMASK;
					ap1[i] |= V_ATTR_FGCOLOR(V_ATTR_COLOR_BLUE);
					ap1[i] |= V_ATTR_LINE;
					ap1[i + 1] &= ~V_ATTR_COLORMASK;
					ap1[i + 1] |= V_ATTR_FGCOLOR(V_ATTR_COLOR_BLUE);
					ap1[i + 1] |= V_ATTR_LINE;
				}
				i++;
			}
		}
	}
#endif


	upcol = 0;

	/* advance past any common chars at the left */
	{
		int i;

		for (i = 0; i < tnc; i++) {
			switch (chctype(cp1[i])) {
			case CT_KJ2:
			default:
				/* XXX: panic */
				break;

			case CT_ANK:
				if ((cp1[i] == cp2[i]) && (ap1[i] == ap2[i])) {
					continue;
				}
				break;
			case CT_KJ1:
				if ((cp1[i] == cp2[i])   && (ap1[i]   == ap2[i]) &&
				    (cp1[i + 1] == cp2[i + 1]) && (ap1[i + 1] == ap2[i + 1])) {
					i += 1;	/* skip 2byte chars */
					continue;
				}
				break;
			}
			break;
		}

		ap1 += i;
		ap2 += i;
		cp1 += i;
		cp2 += i;
		upcol += i;

		if (i >= tnc) {
			vp1->v_flag &= ~VFCHG;	/* flag this line as updated */
			updateline_done(vp1, vp2);
			return TRUE;
		}
	}

	/*
	 * This can still happen, even though we only call this routine on
	 * changed lines. A hard update is always done when a line splits, a
	 * massive change is done, or a buffer is displayed twice. This
	 * optimizes out most of the excess updating. A lot of computes are
	 * used, but these tend to be hard operations that do a lot of
	 * update, so I don't really care.
	 */
	/* if both lines are the same, no update needs to be done */
	if (cp1 == &vp1->v_text[tnc]) {
		vp1->v_flag &= ~VFCHG;	/* flag this line is changed */
		updateline_done(vp1, vp2);
		return TRUE;
	}

	/* find out if there is a match on the right */
	nbflag = FALSE;

	{
		int i = tnc;

		cp3 = &vp1->v_text[0];
		cp4 = &vp2->v_text[0];
		ap3 = &vp1->v_attr[0];
		ap4 = &vp2->v_attr[0];

		while (((i - 1) >= 0) &&
		    (cp3[i - 1] == cp4[i - 1]) &&
		    (ap3[i - 1] == ap4[i - 1]) &&
		    (kp1[i - 1] == CT_ANK)) {

			/* Note if any nonblank */
			if ((cp3[i - 1] != ' ') || ap3[i - 1])
				nbflag = TRUE;	/* in right match. */

			i--;
		}

		cp3 += i;
		cp4 += i;
		ap3 += i;
		ap4 += i;
	}

	cp5 = cp3;	/* position of End to update */
	ap5 = ap3;	/* position of End to update */

	/* Erase to EOL ? */
	if (nbflag == FALSE && eolexist == TRUE) {
		while (cp5 != cp1 && cp5[-1] == ' ') {
			--cp5;
			--ap5;
		}
	}

	/*
	 * cp1 ......... position of start to update
	 * cp3 = cp5 ... position of end to update
	 *
	 * movecursor(row, (int)(cp1 - &vp1->v_text[0]));  Go to start of line.
	 */
	movecursor(row, upcol);

	{
		while (cp1 < cp5) {	/* Ordinary. */
			/* attribute off */
			TTattr(*ap1);
			TTputc(*cp1);
			++ttcol;
			*cp2++ = *cp1++;
			*ap2++ = *ap1++;
		}

#if 0
{
  static unsigned int x = 0;
  char buf[32];
  char *p = buf;
  sprintf(buf, "[%d]", x++);
  TTputc('<');
  while (*p)
    TTputc(*p++);
  TTputc('>');
}
#endif
	}

	if (cp5 != cp3) {	/* Erase. */
		TTattr(0);
		TTeeol();
		while (cp1 != cp3) {
			*cp2++ = *cp1++;
		}
	}

	vp1->v_flag &= ~VFCHG;	/* flag this line as updated */
	updateline_done(vp1, vp2);
	return TRUE;
}

/*
 * Redisplay the mode line for the window pointed to by the "wp". This is the
 * only routine that has any idea of how the modeline is formatted. You can
 * change the modeline format by hacking at this routine. Called by "update"
 * any time there is a dirty window.
 */
int
modeline(WINDOW *wp)	/* window to update modeline for */
{
	char tline[NLINE];	/* buffer for part of mode line */

	/* don't bother if there is none! */
	if (modeflag == FALSE)
		return FALSE;

	{
		unsigned int n;

		n = wp->w_toprow + wp->w_ntrows;
		vscreen[n]->v_flag |= VFCHG;
		vscreen[n]->v_mode = 0;
		vscreen[n]->v_lineno = 0;
		vscreen[n]->v_hashroot = NULL;

		vtmove(n, 0);
		n = mlformat(wp, tline, wp->w_bufp->b_mlform);

		vtputs((unsigned char *)tline, n, wp->w_bufp->b_tabs, wp->w_bufp->b_mlcol, 0, NULL);
		vteeol();
	}
	return TRUE;
}


/* update all the mode lines */
int
upmode(void)
{
	WINDOW *wp;

	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE;
		wp = wp->w_wndp;
	}
	return TRUE;
}

/* force hard updates on all windows */
int
upwind(void)
{
	WINDOW *wp;

	wp = wheadp;
	while (wp != NULL) {
		wp->w_flag |= WFMODE | WFHARD;
		wp = wp->w_wndp;
	}

	return TRUE;
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col". The row and column arguments are origin 0. Optimize out
 * random calls. Update "ttrow" and "ttcol".
 */
int
movecursor(int row, int col)
{
	if (row != ttrow || col != ttcol) {
		ttrow = row;
		ttcol = col;
		TTmove(row, col);
	}

	return TRUE;
}

int
movecursor_num(int row, int col)
{
	return movecursor(row, col + leftmargin);
}

/*
 * Erase the message line. This is a special routine because the message line
 * is not considered to be part of the virtual screen. It always works
 * immediately; the terminal buffer is flushed via a call to the flusher.
 */
int
mlerase(void)
{
	int i;

	movecursor(term.t_nrow, 0);
	if (discmd <= 0)
		return TRUE;

	if (eolexist == TRUE) {
		TTeeol();
	} else {
		for (i = 0; i < term.t_ncol - 1; i++) {
			TTputc(' ');
		}
		movecursor(term.t_nrow, 1);	/* force the move! */
		movecursor(term.t_nrow, 0);
	}
	TTflush();
	mpresf = FALSE;

	return TRUE;
}

/*
 * Write a message into the message line. Keep track of the physical cursor
 * position. A small class of printf like format items is handled.
 * Set the "message line" flag TRUE. Don't write beyond the end of
 * the current terminal width.
 */
int
mlout(int c)	/* character to write on modeline */
{
	TTattr(cmdcol);
	if (ttcol + 1 < term.t_ncol) {
		TTputc(c);
	}
	*lastptr++ = c;

	return TRUE;
}

int
mlwrite(char *fmt, ...)
{
	int c;			/* current char in format string */
	va_list ap;		/* ptr to current data field */

	/* if we are not currently echoing on the command line, abort this */
	if (discmd <= 0)
		return TRUE;

	/* if we can not erase to end-of-line, do it manually */
	if (eolexist == FALSE) {
		mlerase();
		TTflush();
	}
	movecursor(term.t_nrow, 0);
	lastptr = &lastmesg[0];	/* setup to record message */

	va_start(ap, fmt);
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			mlout(c);
			++ttcol;
		} else {
			c = *fmt++;
			switch (c) {
			case 'd':
			case 'D':
				mlputi(va_arg(ap, int), 10);
				break;

			case 'o':
				mlputi(va_arg(ap, int), 8);
				break;

			case 'x':
				mlputi(va_arg(ap, int), 16);
				break;

			case 's':
				mlputs(va_arg(ap, char *));
				break;

			case 'f':
				mlputf(va_arg(ap, int));
				break;

			default:
				mlout(c);
				++ttcol;
			}
		}
	}
	va_end(ap);

	/* if we can, erase to the end of screen */
	if (eolexist == TRUE)
		TTeeol();

	TTflush();
	mpresf = TRUE;
	*lastptr = 0;		/* terminate lastmesg[] */

	return TRUE;
}

/*
 * Force a string out to the message line regardless of the current $discmd
 * setting. This is needed when $debug is TRUE and for the write-message and
 * clear-message-line commands
 */
int
mlforce(char *s)	/* string to force out */
{
	int oldcmd;		/* original command display flag */

	oldcmd = discmd;	/* save the discmd value */
	discmd = 1;		/* and turn display on */
	mlwrite(s);		/* write the string out */
	discmd = oldcmd;	/* and restore the original setting */

	return TRUE;
}

/*
 * Write out a string. Update the physical cursor position. This assumes that
 * the characters in the string all have width "1"; if this is not the case
 * things will get screwed up a little.
 */
int
mlputs(char *s)
{
	int c;

	while ((c = *s++) != 0) {
		mlout(c);
		++ttcol;
	}

	return TRUE;
}

/*
 * Write out an integer, in the specified radix. Update the physical cursor
 * position.
 */
int
mlputi(int i, int r)
{
	int q;
	static char hexdigits[] = "0123456789ABCDEF";

	if (i < 0) {
		i = -i;
		mlout('-');
	}
	q = i / r;

	if (q != 0)
		mlputi(q, r);

	mlout(hexdigits[i % r]);
	++ttcol;

	return TRUE;
}

/*
 *	write out a scaled integer with two decimal places
 */
int
mlputf(int s)	/* scaled integer to output */
{
	int i;			/* integer portion of number */
	int f;			/* fractional portion of number */

	/* break it up */
	i = s / 100;
	f = s % 100;

	/* send out the integer portion */
	mlputi(i, 10);
	mlout('.');
	mlout((f / 10) + '0');
	mlout((f % 10) + '0');
	ttcol += 3;

	return TRUE;
}



static int
mlformat(WINDOW * wp, char *line, char *format)
{
	char c, lchar;
	char *linep, var[NSTRING];
	BUFFER *bp;

	lchar = (wp == curwp) ? '=' : '-';
	bp = wp->w_bufp;
	for (linep = line; (c = *format); format++) {
		switch (c) {
		case '?':
			switch (c = *++format) {
			case '#':
				*linep++ = (bp->b_flag & BFTRUNC) ? '#' : lchar;
				break;
			case '*':
				*linep++ = (bp->b_flag & BFCHG) ? '*' : lchar;
				break;
			case 'n':
			case 'N':
				if (bp->b_flag & BFNAROW) {
					*linep++ = '<';
					*linep++ = '>';
				} else {
					*linep++ = lchar;
					*linep++ = lchar;
				}
				break;
			case 'p':
			case 'P':
				strcpy(linep, PROGNAME);
				linep += strlen(linep);
				break;
			case 'v':
			case 'V':
				strcpy(linep, VERSION);
				linep += strlen(linep);
				break;
			case 'l':
				if (wp->w_fcol > 0) {
					*linep++ = '[';
					*linep++ = '<';
					strcpy(linep, int_asc(wp->w_fcol));
					linep += strlen(linep);
					*linep++ = ']';
				}
				break;
			case 'L':
				if (wp->w_fcol > 0) {
					strcpy(linep, int_asc(wp->w_fcol));
					linep += strlen(linep);
				}
				break;
			case 'r':
#if RAMSIZE
				strcpy(linep, int_asc(envram/1024));
				linep += strlen(linep);
#endif
				break;
			case 'm':
			case 'M':
				{
					int i, mod, firstm = 1;

					for (i = 0, mod = 1; i < NUMMODES; i++, mod <<= 1) {
						if (wp->w_bufp->b_mode & mod) {
							if (!firstm)
								*linep++ = ' ';
							else
								firstm = 0;
							if (c == 'M')
								*linep++ = modecode[i];
							else {
								strcpy(linep, modename[i]);
								linep += strlen(linep);
							}
						}
					}
				}
				break;
			case 'b':
			case 'B':
				strcpy(linep, bp->b_bname);
				linep += strlen(linep);
				break;
			case 'f':
				if (*bp->b_fname) {
					*linep++ = ' ';
					strcpy(linep, "File:");
					strcat(linep, bp->b_fname);
					linep += strlen(linep);
					*linep++ = ' ';
				}
				break;
			case 'F':
				if (*bp->b_fname) {
					strcpy(linep, bp->b_fname);
					linep += strlen(linep);
				}
				break;
			case 'K':
				switch (bp->b_kanjicode) {
				case KANJI_JIS:
					strcpy(linep, "JIS");
					linep += strlen(linep);
					break;
				case KANJI_EUC:
					strcpy(linep, "EUC");
					linep += strlen(linep);
					break;
				case KANJI_SJIS:
					strcpy(linep, "SJIS");
					linep += strlen(linep);
					break;
				case KANJI_UTF8:
					strcpy(linep, "UTF8");
					linep += strlen(linep);
					break;
				case KANJI_UTF16BE:
					strcpy(linep, "UTF16BE");
					linep += strlen(linep);
					break;
				case KANJI_UTF16LE:
					strcpy(linep, "UTF16LE");
					linep += strlen(linep);
					break;
				case KANJI_ASCII:
				default:
					strcpy(linep, "ASCII");
					linep += strlen(linep);
					break;
				}
				break;
			case 'k':
				switch (bp->b_kanjicode) {
				case KANJI_JIS:
					*linep++ = 'J';
					break;
				case KANJI_EUC:
					*linep++ = 'E';
					break;
				case KANJI_SJIS:
					*linep++ = 'S';
					break;
				case KANJI_UTF8:
					*linep++ = 'u';
					break;
				case KANJI_UTF16LE:
					*linep++ = 'L';
					break;
				case KANJI_UTF16BE:
					*linep++ = 'B';
					break;
				case KANJI_ASCII:
				default:
					*linep++ = 'A';
					break;
				}
				break;

			case '=':
				*linep++ = lchar;
				break;
			case '?':
				*linep++ = '?';
				break;
			default:
				*linep++ = c;
				break;
			}
			break;
		case '$':
			if (format[1] == '$') {
				*linep++ = '$';
				format++;
			} else {
				char *varp;

				for (varp = var; (c = *++format) != '$' && c != 0;)
					*varp++ = c;
				if (c == 0)
					goto blockout;
				*varp = 0;
				varp = gtenv(var);
				if (varp) {
					strcpy(linep, varp);
					linep += strlen(linep);
				}
			}
			break;
		case '%':
			if (format[1] == '%') {
				*linep++ = '%';
				format++;
			} else {
				char *varp;

				for (varp = var; (c = *++format) != '%' && c != 0;)
					*varp++ = c;
				if (c == 0)
					goto blockout;
				*varp = 0;
				varp = gtusr(var);
				if (varp) {
					strcpy(linep, varp);
					linep += strlen(linep);
				}
			}
			break;
		default:
			*linep++ = c;
			break;
		}
	}

blockout:
	{
		int i;

		*linep = 0;
		for (i = strlen(line); i <= term.t_ncol; i++)
			*linep++ = lchar;
		*linep = 0;

		return i - 1;
	}
}
