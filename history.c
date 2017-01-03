/*
----------------------------------------
	HISTORY.C: MicroEMACS 3.10
----------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id �̐ݒ�
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/history.c,v 1.4 1992/01/04 13:11:22 SALT Exp SALT $'\n""	even\n");

/*
========================================
	�}�N���̒�`
========================================
*/

#define	HISTORY_MAX		64

/*
========================================
	�g�p�֐��̒�`
========================================
*/

static void	his_get(LEDIT *, LINE *);
static int	his_comp(char *, LINE *);
static int	his_match(char *, LINE *);
static void	his_skipshortcut(LINE *, char **, int *);

/*
========================================
	�g�p�ϐ��̒�`
========================================
*/

static BUFFER	*curhbp;
static int		hisenable = FALSE;
static char		searchstr[NSTRING];

/*
========================================
	history�̏������i�g�p���� on�j
========================================
*/

void his_enable(BUFFER *bp)
{
	if (executinghook)
		return;

	bp->b_dotp = bp->b_linep;
	curhbp = bp;
	hisenable = TRUE;
}

/*
========================================
	history�̏������i�g�p���� off�j
========================================
*/

void his_disable(void)
{
	if (executinghook)
		return;

	hisenable = FALSE;
}

/*
========================================
	history�̑O������
========================================
*/

void his_forward(LEDIT *ep, int first)
{
	BUFFER	*bp;
	LINE	*start;
	LINE	*lp;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;

	if (first == TRUE) {
		strcpy(searchstr, ep->text);
		start = bp->b_dotp = bp->b_linep;
	} else
		start = bp->b_dotp;

	lp = start;
	while (1) {
		do {
			lp = lforw(lp);
		} while (lp != start && lp == bp->b_linep);
		if (lp == start)
			break;
		if (!his_comp(searchstr, lp)) {
			his_get(ep, lp);
			bp->b_dotp = lp;
			break;
		}
	}
}

/*
========================================
	history�̌������
========================================
*/

void his_backward(LEDIT *ep, int first)
{
	BUFFER	*bp;
	LINE	*start;
	LINE	*lp;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;

	if (first == TRUE) {
		strcpy(searchstr, ep->text);
		start = bp->b_dotp = bp->b_linep;
	} else
		start = bp->b_dotp;

	lp = start;
	while (1) {
		do {
			lp = lback(lp);
		} while (lp != start && lp == bp->b_linep);
		if (lp == start)
			break;
		if (!his_comp(searchstr, lp)) {
			his_get(ep, lp);
			bp->b_dotp = lp;
			break;
		}
	}
}

/*
========================================
	history�̑O�������ishortcut�t�̂݁j
========================================
*/

void his_sc_forward(LEDIT *ep, int first)
{
	BUFFER	*bp;
	LINE	*start;
	LINE	*lp;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;

	if (first == TRUE) {
		strcpy(searchstr, ep->text);
		start = bp->b_dotp = bp->b_linep;
	} else
		start = bp->b_dotp;

	lp = start;
	while (1) {
		do {
			lp = lforw(lp);
		} while (lp != start && lp == bp->b_linep);
		if (lp == start)
			break;
		if (llength(lp) && lp->l_text[0] != ':' && !his_comp(searchstr, lp)) {
			his_get(ep, lp);
			bp->b_dotp = lp;
			break;
		}
	}
}

/*
========================================
	history�̌�������ishortcut�t�̂݁j
========================================
*/

void his_sc_backward(LEDIT *ep, int first)
{
	BUFFER	*bp;
	LINE	*start;
	LINE	*lp;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;

	if (first == TRUE) {
		strcpy(searchstr, ep->text);
		start = bp->b_dotp = bp->b_linep;
	} else
		start = bp->b_dotp;

	lp = start;
	while (1) {
		do {
			lp = lback(lp);
		} while (lp != start && lp == bp->b_linep);
		if (lp == start)
			break;
		if (llength(lp) && lp->l_text[0] != ':' && !his_comp(searchstr, lp)) {
			his_get(ep, lp);
			bp->b_dotp = lp;
			break;
		}
	}
}

/*
========================================
	history�̎擾
========================================
*/

static void his_get(LEDIT *ep, LINE *lp)
{
	int		i;
	int		len;
	int		c;
	char	*p, *q;

	his_skipshortcut(lp, &p, &len);
	if (ep->limit < len)
		len = ep->limit;
	q = ep->text;
	for(i = len; i > 0; i--) {
		c = *q++ = *p++;
		if (iskanji(c)) {
			if (i > 1) {
				*q++ = *p++;
				i--;
			} else
				break;
		}
	}
	*q = 0;

	ep->pos = len;
	ep->modified = TRUE;
	curhbp->b_dotp = lp;
}

/*
========================================
	history�̓o�^
========================================
*/

void his_regist(LEDIT *ep)
{
	BUFFER	*bp;
	LINE	*lp;
	LINE	*start;
	int		n;
	int		len;
	char	*text;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;
	text = ep->text;
	len = strlen(text);

	if (len == 0)
		return;

	start = lp = bp->b_linep;
	n = 0;
	while (1) {
		do {
			lp = lforw(lp);
		} while (lp != start && lp == bp->b_linep);
		if (lp == start)
			break;
		if (his_match(text, lp) == TRUE)
			break;
		n++;
	}
	if (lp == start) {
		if (n >= HISTORY_MAX)
			lfree(lforw(lp));
		lp = lalloc(len + 1);
		if (lp == 0)
			return;
		lp->l_text[0] = ':';
		memcpy(lp->l_text + 1, text, len);
	} else {
		lp->l_bp->l_fp = lp->l_fp;
		lp->l_fp->l_bp = lp->l_bp;
	}
	lp->l_fp = start;
	lp->l_bp = start->l_bp;
	lp->l_fp->l_bp = lp;
	lp->l_bp->l_fp = lp;

	{
		WINDOW	*wp;

		for(wp = wheadp; wp; wp = wp->w_wndp) {
			if (wp->w_bufp == bp)
				wp->w_flag |= WFHARD;
		}
	}
}

/*
========================================
	history�̔�r
========================================
*/

static int his_comp(char *p, LINE *lp)
{
	int		c;
	int		len;
	char	*q;

	his_skipshortcut(lp, &q, &len);
	while ((c = *p++) && len-- > 0) {
		if (c != *q++)
			break;
	}
	return c;
}

/*
========================================
	history�̊��S��r
========================================
*/

static int his_match(char *p, LINE *lp)
{
	int		c;
	int		len;
	char	*q;

	his_skipshortcut(lp, &q, &len);
	while ((c = *p++) && len-- > 0) {
		if (c != *q++)
			break;
	}
	return (c == 0 && len == 0) ? TRUE : FALSE;
}

/*
========================================
	history��shortcut�������X�L�b�v
========================================
*/

static void his_skipshortcut(LINE *lp, char **p, int *len)
{
	int		c;
	char	*pp;
	int		llen;

	pp   = lp->l_text;
	llen = lp->l_used;
	while (llen > 0) {
		llen--;
		c = *pp++;
		if (c == ':')
			break;
	}
	*p   = pp;
	*len = llen;
}

/*
========================================
	history��shortcut
========================================
*/

void his_shortcut(LEDIT *ep, char *keyword)
{
	BUFFER	*bp;
	LINE	*lp;
	LINE	*start;
	int		len;

	if (executinghook || hisenable != TRUE)
		return;

	bp = curhbp;
	start = lp = bp->b_linep;
	len = strlen(keyword);

	while ((lp = lforw(lp)) != start) {
		char	*p;

		p = lp->l_text;
		if (llength(lp) > len && !strncmp(keyword, p, len) && p[len] == ':') {
			his_get(ep, lp);
			break;
		}
	}
}
