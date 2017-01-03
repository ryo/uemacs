/*
----------------------------------------
	LATEX.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id ÇÃê›íË
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/latex.c,v 1.2 1991/09/01 03:00:20 SALT Exp $'\n""	even\n");

/*
========================================
	égópÇ∑ÇÈä÷êîÇÃíËã`
========================================
*/

static int insert_begin(char *);
static int insert_begin_end(char *);
static int insert_begin_mode_end(char *);
static int insert_latexcom(char *);
static int makeenv(int (*)(char *));

/*
----------------------------------------
	tex-insert-quote
----------------------------------------
*/

int tex_insquote(int f, int n)
{
	char cchar[4];

	if (n != 1) {
		if (n < 1)
			n = 1;
		return linsert(n, 0x22);
	} else {
		char *insstr;

		if (curwp->w_doto) {
			char *p, *q;

			backchar(FALSE, 1);

			p = curwp->w_dotp->l_text + curwp->w_doto;
			q = cchar;
			*q = *p;
			if (iskanji(*q))
				*++q = *++p;
			*++q = 0;

			if (sindex(texbquote, cchar))
				insstr = "``";
			else if (*cchar == '\\')
				insstr = "\"";
			else
				insstr = "''";

			forwchar(FALSE, 1);
		} else
			insstr = "``";

		return linstr(insstr);
	}
}

/*
----------------------------------------
	tex-terminate-paragraph
----------------------------------------
*/

int tex_termpara(int f, int n)
{
	while (n--) {
		int status;

		if (check_wrap())
			exechook(wraphook);
		status = lnewline();
		if (status != TRUE)
			return status;
	}
	return lnewline();
}

/*
----------------------------------------
	latex-mathmode
----------------------------------------
*/

int latex_mathmode(int f, int n)
{
	int status;

	status = linstr("$$");
	if (status != TRUE)
		return status;

	backchar(FALSE, 1);
	fmatch('$');

	return TRUE;
}

/*
----------------------------------------
	latex-close-block
----------------------------------------
*/

int latex_closeblock(int f, int n)
{
	int doto, doto2;
	char block[NSTRING];
	LINE *lp, *lp2;
	WINDOW *wp = curwp;

	lp = wp->w_dotp;
	doto = wp->w_doto;
	if (!search_begin_back()) {
		wp->w_dotp = lp;
		wp->w_doto = doto;
		mlwrite(KTEX259);
		return FALSE;
	}
	lp2 = wp->w_dotp;
	doto2 = wp->w_doto;

	{
		int i, j;

		strcpy(block, "\\end{");
		i = doto2 + 7;
		j = 5;
		while (i < lp2->l_used && j < NSTRING - 1) {
			int ch;

			ch = lp2->l_text[i];
			if (ch == TAB || ch == ' ')
				break;
			if (iskanji(ch)) {
				if (j < NSTRING - 2)
					break;
				block[j++] = ch, i++;
				block[j++] = lp2->l_text[i++];
			} else {
				block[j++] = ch, i++;
				if (ch == '}')
					break;
			}
		}
		block[j] = 0;
	}

	wp->w_dotp = lp;
	wp->w_doto = doto;

	{
		int i;

		for (i = doto - 1; i >= 0; i--) {
			int ch;

			ch = lp->l_text[i];
			if (ch == 0x40 && lp->l_text[i - 1] == 0x81)
				i--;
			else if (ch != ' ' && ch != TAB)
				goto close_end;
		}
	}

	wp->w_dotp = lp2;
	wp->w_doto = 0;
	while (1) {
		int ch;

		ch = lgetc(lp2, wp->w_doto);
		if (ch == TAB || ch == ' '
		    || (ch == 0x81 && lgetc(lp2, wp->w_doto + 1) == 0x40))
			forwchar(FALSE, 1);
		else
			break;
	}

	{
		int tsiz, target;

		tsiz = curbp->b_tabs;
		target = getccol(FALSE);
		wp->w_dotp = lp;
		wp->w_doto = doto;
		while (target != getccol(FALSE)) {
			if (target < getccol(FALSE)) {
				while (getccol(FALSE) > target)
					backdel(FALSE, 1);
			} else {
				while (target - getccol(FALSE) >= tsiz)
					linsert(1, TAB);
				linsert(target - getccol(FALSE), ' ');
			}
		}
	}

close_end:

	{
		int status;

		status = linstr(block);
		if (status != TRUE)
			return status;
	}

	show_match(lp2, doto2);

	return TRUE;
}

/*
----------------------------------------
	latex-verse
----------------------------------------
*/

int latex_verse(int f, int n)
{
	return insert_begin_end("verse");
}

/*
----------------------------------------
	latex-verbatim
----------------------------------------
*/

int latex_verbatim(int f, int n)
{
	return insert_begin_end("verbatim");
}

/*
----------------------------------------
	latex-verb
----------------------------------------
*/

int latex_verb(int f, int n)
{
	return linstr("\\verb ");
}

/*
----------------------------------------
	latex-tabular
----------------------------------------
*/

int latex_tabular(int f, int n)
{
	return insert_begin_mode_end("tabular");
}

/*
----------------------------------------
	latex-table
----------------------------------------
*/

int latex_table(int f, int n)
{
	return insert_begin_end("table");
}

/*
----------------------------------------
	latex-titlepage
----------------------------------------
*/

int latex_titlepage(int f, int n)
{
	return insert_begin_end("titlepage");
}

/*
----------------------------------------
	latex-sloppypar
----------------------------------------
*/

int latex_sloppypar(int f, int n)
{
	return insert_begin_end("sloppypar");
}

/*
----------------------------------------
	latex-section
----------------------------------------
*/

int latex_section(int f, int n)
{
	return insert_latexcom("\\section");
}

/*
----------------------------------------
	latex-ref
----------------------------------------
*/

int latex_ref(int f, int n)
{
	return insert_latexcom("\\ref");
}

/*
----------------------------------------
	latex-quotation
----------------------------------------
*/

int latex_quotation(int f, int n)
{
	return insert_begin_end("quotation");
}

/*
----------------------------------------
	latex-quote
----------------------------------------
*/

int latex_quote(int f, int n)
{
	return insert_begin_end("quote");
}

/*
----------------------------------------
	latex-picture
----------------------------------------
*/

int latex_picture(int f, int n)
{
	return insert_begin_end("picture");
}

/*
----------------------------------------
	latex-eqnarray
----------------------------------------
*/

int latex_eqnarray(int f, int n)
{
	return insert_begin_end("eqnarray");
}

/*
----------------------------------------
	latex-minipage
----------------------------------------
*/

int latex_minipage(int f, int n)
{
	return insert_begin_mode_end("minipage");
}

/*
========================================
	environment çÏê¨
========================================
*/

static int makeenv(int (*func) (char *))
{
	char *envname;
	char bak[NSTRING];

	strcpy(bak, comp_gene);
	strcpy(comp_gene, latex_env);
	envname = complete(KTEX260, 0, 0, CMP_GENERAL, NSTRING);
	strcpy(comp_gene, bak);
	return (envname == 0 || *envname == 0) ? FALSE : (*func) (envname);
}

/*
----------------------------------------
	latex-make-envionment
----------------------------------------
*/

int latex_makeenv(int f, int n)
{
	return makeenv(insert_begin_end);
}

/*
----------------------------------------
	latex-label
----------------------------------------
*/

int latex_label(int f, int n)
{
	return insert_latexcom("\\label");
}

/*
----------------------------------------
	latex-itemize
----------------------------------------
*/

int latex_itemize(int f, int n)
{
	return insert_begin_end("itemize");
}

/*
----------------------------------------
	latex-insbegin
----------------------------------------
*/

int latex_insbegin(int f, int n)
{
	return makeenv(insert_begin);
}

/*
----------------------------------------
	latex-item
----------------------------------------
*/

int latex_item(int f, int n)
{
	return linstr("\\item ");
}

/*
----------------------------------------
	latex-figure
----------------------------------------
*/

int latex_figure(int f, int n)
{
	return insert_begin_end("figure");
}

/*
----------------------------------------
	latex-footnote
----------------------------------------
*/

int latex_footnote(int f, int n)
{
	return insert_latexcom("\\footnote");
}

/*
----------------------------------------
	latex-example
----------------------------------------
*/

int latex_example(int f, int n)
{
	return insert_begin_end("example");
}

/*
----------------------------------------
	latex-enumerate
----------------------------------------
*/

int latex_enumerate(int f, int n)
{
	return insert_begin_end("enumerate");
}

/*
----------------------------------------
	latex-equation
----------------------------------------
*/

int latex_equation(int f, int n)
{
	return insert_begin_end("equation");
}

/*
----------------------------------------
	latex-description
----------------------------------------
*/

int latex_description(int f, int n)
{
	return insert_begin_end("description");
}

/*
----------------------------------------
	latex-document
----------------------------------------
*/

int latex_document(int f, int n)
{
	int status;

	status = linstr("\\documentstyle[]{}\n\n"
			"\\begin{document}\n\n"
			"\\end{document}");

	if (status != TRUE)
		return status;

	backline(FALSE, 3);
	backchar(FALSE, 2);
	thisflag = 0;

	return TRUE;
}

/*
----------------------------------------
	latex-chapter
----------------------------------------
*/

int latex_chapter(int f, int n)
{
	return insert_latexcom("\\chapter");
}

/*
----------------------------------------
	latex-center
----------------------------------------
*/

int latex_center(int f, int n)
{
	return insert_begin_end("center");
}

/*
----------------------------------------
	latex-cite
----------------------------------------
*/

int latex_cite(int f, int n)
{
	return insert_latexcom("\\cite");
}

/*
----------------------------------------
	latex-abstract
----------------------------------------
*/

int latex_abstruct(int f, int n)
{
	return insert_begin_end("abstract");
}

/*
----------------------------------------
	latex-array
----------------------------------------
*/

int latex_array(int f, int n)
{
	return insert_begin_mode_end("array");
}

/*
----------------------------------------
	tex-insert-braces
----------------------------------------
*/

int tex_insbraces(int f, int n)
{
	return insert_latexcom("");
}

/*
----------------------------------------
	up-list
----------------------------------------
*/

int latex_uplist(int f, int n)
{
	int ldoto;
	LINE *bot, *llp;
	WINDOW *wp = curwp;

	bot = curbp->b_linep;
	llp = wp->w_dotp;
	ldoto = wp->w_doto;

	while (1) {
		int doto;
		LINE *lp;

		lp = wp->w_dotp;
		doto = wp->w_doto;

		if (lp->l_used != doto) {
			int c;

			c = lgetc(lp, doto);
			if (c == '}') {
				forwchar(FALSE, 1);
				return TRUE;
			}
		}
		if (lp == bot && doto == lp->l_used)
			break;
		forwchar(FALSE, 1);
	}

	mlwrite(KTEX261);
	wp->w_dotp = llp;
	wp->w_doto = ldoto;

	return FALSE;
}

/*
----------------------------------------
	insert_begin_end
----------------------------------------
*/

static int insert_begin_end(char *env)
{
	int status;
	char work[256];

	sprintf(work, "\\begin{%s}", env);
	status = linstr(work);
	if (status != TRUE)
		return status;
	status = indent(TRUE, 2);
	if (status != TRUE)
		return status;

	sprintf(work, "\\end{%s}", env);
	status = linstr(work);
	if (status != TRUE)
		return status;

	update(FALSE);
	backline(TRUE, 1);
	thisflag = 0;

	return TRUE;
}

/*
----------------------------------------
	insert_begin
----------------------------------------
*/

static int insert_begin(char *env)
{
	int status;
	char work[256];

	sprintf(work, "\\begin{%s}", env);
	status = linstr(work);
	if (status != TRUE)
		return status;
	status = indent(TRUE, 1);
	if (status != TRUE)
		return status;
	thisflag = 0;

	return TRUE;
}

/*
----------------------------------------
	insert_begin_mode_end
----------------------------------------
*/

static int insert_begin_mode_end(char *env)
{
	int status, len;
	char work[256];

	sprintf(work, "\\begin{%s}{}", env);
	status = linstr(work);
	if (status != TRUE)
		return status;
	status = indent(TRUE, 2);
	if (status != TRUE)
		return status;

	sprintf(work, "\\end{%s}", env);
	status = linstr(work);
	if (status != TRUE)
		return status;

	update(FALSE);
	backline(TRUE, 2);
	if (len = llength(curwp->w_dotp))
		curwp->w_doto = len - 1;
	thisflag = 0;

	return TRUE;
}

/*
----------------------------------------
	insert-latex-command
----------------------------------------
*/

static int insert_latexcom(char *com)
{
	int status;

	status = linstr(com);
	if (status != TRUE)
		return status;
	status = linstr("{}");

	return (status != TRUE) ? status : backchar(FALSE, 1);
}

/*
----------------------------------------
	LaTeX ÇÃñºëOï‚äÆ
----------------------------------------
*/

int latex_complete(int f, int n)
{
	return name_complete(CMP_LATEX);
}

/*
----------------------------------------
	')' ë}ì¸
----------------------------------------
*/

int tex_insparen(int f, int n)
{
	return insmatch(f, n, ')');
}

/*
----------------------------------------
	'}' ë}ì¸
----------------------------------------
*/

int tex_insbrace(int f, int n)
{
	return insmatch(f, n, '}');
}

/*
----------------------------------------
	']' ë}ì¸
----------------------------------------
*/

int tex_insbracket(int f, int n)
{
	return insmatch(f, n, ']');
}

/*
----------------------------------------
	'$' ë}ì¸
----------------------------------------
*/

int tex_insdollar(int f, int n)
{
	return insmatch(f, n, '$');
}

/*
----------------------------------------
	search-begin-backward
----------------------------------------
*/

int search_begin_back(void)
{
	int level;
	LINE *top;
	WINDOW *wp = curwp;

	top = lforw(curbp->b_linep);
	level = 1;

	while (1) {
		int doto;
		LINE *lp;

		lp = wp->w_dotp;
		doto = wp->w_doto;

		if (lp->l_used != doto) {
			int c;

			c = lgetc(lp, doto);
			if (c == '\\') {
				char *text;

				text = lp->l_text;
				if (lp->l_used - doto >= 5) {
					if (memcmp(&text[doto], "\\end{", 5) == 0)
						level++;
				}
				if (lp->l_used - doto >= 7) {
					if (memcmp(&text[doto], "\\begin{", 7) == 0)
						level--;
				}
			}
		}
		if (level == 0 || (lp == top && doto == 0))
			break;
		backchar(FALSE, 1);
	}

	return level ? FALSE : TRUE;
}

/*
----------------------------------------
	search-end-forward
----------------------------------------
*/

int search_end_forw(void)
{
	int level, doto;
	LINE *bot;
	WINDOW *wp = curwp;

	bot = curbp->b_linep;
	level = 1;

	while (1) {
		LINE *lp;

		lp = wp->w_dotp;
		doto = wp->w_doto;

		if (lp->l_used != doto) {
			int c;

			c = lgetc(lp, doto);
			if (c == '\\') {
				char *text;

				text = lp->l_text;
				if (lp->l_used - doto >= 5) {
					if (memcmp(&text[doto], "\\end{", 5) == 0)
						level--;
				}
				if (lp->l_used - doto >= 7) {
					if (memcmp(&text[doto], "\\begin{", 7) == 0)
						level++;
				}
			}
		}
		if (level == 0 || (lp == bot && doto == lp->l_used))
			break;
		forwchar(FALSE, 1);
	}

	return level ? FALSE : TRUE;
}

/*
----------------------------------------
	show match
----------------------------------------
*/

void show_match(LINE *match_lp, int match_doto)
{
	WINDOW *wp = curwp;
	LINE *bot, *wlp, *lp;
	int nlines, doto;

	update(FALSE);

	bot = curbp->b_linep;
	lp = wp->w_dotp;
	doto = wp->w_doto;
	wp->w_dotp = match_lp;
	wp->w_doto = match_doto;
	wlp = wp->w_linep;
	nlines = wp->w_ntrows + ((modeflag == FALSE) ? 1 : 0);

	while (nlines > 0) {
		if (wlp == match_lp) {
			int i;

			for (i = 0; i < term.t_pause; i++)
				update(FALSE);
			break;
		}
		wlp = lforw(wlp);
		if (wlp == bot)
			break;
		nlines--;
	}

	wp->w_dotp = lp;
	wp->w_doto = doto;
}
