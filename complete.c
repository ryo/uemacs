/*
----------------------------------------
	COMPLETE.C: MicroEMACS 3.10
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
#include "ehprint.h"
#include "ekanji.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/complete.c,v 1.4 1992/02/15 06:51:10 SALT Exp SALT $'\n""	even\n");

/*
========================================
	ローカルな構造体の定義
========================================
*/

struct compbuf_t {
	struct compbuf_t *next;
	char buf[0];
};

/*
========================================
	使用関数の定義
========================================
*/

static int comp_string(void);
static int make_compbuf(char *);
static void getfirst(void);
static void getnext(void);
static void getbody(void);
static void comp_end(void);
static void cwin_draw(void);
static void cwin_push_scrstat(void);
static void cwin_pop_scrstat(void);
static void cwin_drawitem(int no);

static int init_compbuf (void);
static int tini_compbuf (void);
static int add_compbuf (char *);
static void sort_complist (int);

/*
========================================
	ローカル変数
========================================
*/

static void *sptr;
static char *name, *nametop, comp_name[NSTRING];
static char *compbufp, **complist;
static int type, sflag, sind, slim, open_status;
static int cwin_active = 0, cwin_home, cwin_height;
static int cwin_nitem, cwin_base, cwin_cur_x, cwin_cur_y;
static int ncomplist, complist_size;
static int compbuf_left;

static struct compbuf_t *compbuf;

/*
========================================
	get_alpha_pos
========================================
*/

static int get_alpha_pos (char *name)
{
	char ext, *p;

	for (p = name; !isalpha (ext = *p); p++) {
		if (!ext)
			return 0;
	}

	return p - name;
}

/*
========================================
	getfirst
========================================
*/

static void getfirst(void)
{
	char ext, fname[NFILEN];

	sind = 1;
	switch (type) {
	case CMP_BUFFER:
		sptr = bheadp;
		break;
	case CMP_COMMAND:
		sptr = command_table;
		break;
	case CMP_MACRO:
		{
			char *p;

			sptr = bheadp;
			p = ((BUFFER *)sptr)->b_bname;
			if (*p != '[' || p[strlen(p) - 1] != ']') {
				sind--;
				getnext();
			}
		}
		break;
	case CMP_FILENAME:
	case CMP_DIRNAME:
		sptr = getffile(name, (type == CMP_DIRNAME) ? 1 : 0);
		break;
	case CMP_MODE:
		slim = NCOLORS;
		sflag = 0;
		sptr = cname;
		break;
	case CMP_VARIABLE:
		sptr = (*name == '$') ? (void *)env_table : (void *)uv_head;
		break;
	case CMP_GENERAL:
		open_status = ffropen(comp_gene);
		goto same;
	case CMP_C:
		ext = name[get_alpha_pos (name)];
		sprintf(fname, "%s.em%c", comp_c, (int)ext);
		open_status = ffropen(fname);
		goto same;
	case CMP_KEYWORD:
		if (curbp->b_comp_keyword) {
			ext = name[get_alpha_pos (name)];
			sprintf(fname, "%s.em%c", curbp->b_comp_keyword, (int)ext);
			open_status = ffropen(fname);
		} else
			open_status = FIOERR;
		goto same;
	case CMP_LATEX:
		ext = name[get_alpha_pos (name)];
		sprintf(fname, "%s.em%c", comp_latex, (int)ext);
		open_status = ffropen(fname);

	same:
		if (open_status == FIOSUC) {
			sind--;
			getnext();
		} else
			sptr = 0;
	}
}

/*
========================================
	getnext
========================================
*/

static void getnext(void)
{
	sind++;
	switch (type) {
	case CMP_BUFFER:
		sptr = ((BUFFER *)sptr)->b_bufp;
		break;
	case CMP_COMMAND:
		if (sind <= numfunc) {
			sptr = (void *)((NBIND *)sptr + 1);
			break;
		}
		if (sind == numfunc + 1) {
			char *p;

			sptr = bheadp;
			p = ((BUFFER *)sptr)->b_bname;
			if (*p == '[' && p[strlen(p) - 1] == ']')
				 break;
		}
	case CMP_MACRO:
		while (1) {
			char *p;

			sptr = ((BUFFER *)sptr)->b_bufp;
			if (sptr == 0)
				break;
			p = ((BUFFER *)sptr)->b_bname;
			if (*p == '[' && p[strlen(p) - 1])
				break;
		}
		break;
	case CMP_FILENAME:
		sptr = getnfile(0);
		break;
	case CMP_DIRNAME:
		sptr = getnfile(1);
		break;
	case CMP_MODE:
		if (sind > slim) {
			if (sflag == 0) {
				sptr = modename;
				sind = 1;
				slim = NUMMODES;
				sflag = 1;
			} else
				sptr = 0;
		} else
			sptr = (void *)((char **)sptr + 1);
		break;
	case CMP_VARIABLE:
		if (*name == '$')
			sptr = (sind <= nevars) ? (void *)((UENVAR *)sptr + 1) : NULL;
		else
			sptr = (void *)(((UVAR *)sptr)->u_next);
		break;
	case CMP_GENERAL:
	case CMP_C:
	case CMP_KEYWORD:
	case CMP_LATEX:
		{
			int status, length;

			do {
				status = ffgetline(&length);
				if (status != FIOSUC)
					length = 1;
			} while (length == 0);
			sptr = (status == FIOSUC) ? fline : 0;
		}
		break;
	}
}

/*
========================================
	getbody
========================================
*/

static void getbody(void)
{
	static char buf[NBUFN];

	switch (type) {
	case CMP_BUFFER:
		nametop = ((BUFFER *)sptr)->b_bname;
		break;
	case CMP_COMMAND:
		if (sind <= numfunc) {
			nametop = ((NBIND *)sptr)->n_name;
			break;
		}
	case CMP_MACRO:
		{
			int len;
			char *p;

			p = ((BUFFER *)sptr)->b_bname;
			len = strlen(p);
			strcpy(buf, p + 1);
			buf[len - 2] = 0;
		}
		nametop = buf;
		break;
	case CMP_FILENAME:
	case CMP_DIRNAME:
		nametop = sptr;
		break;
	case CMP_MODE:
		nametop = *((char **)sptr);
		break;
	case CMP_VARIABLE:
		nametop = (*name == '$') ? ((UENVAR *)sptr)->e_name : ((UVAR *)sptr)->u_name;
		break;
	case CMP_GENERAL:
	case CMP_C:
	case CMP_KEYWORD:
	case CMP_LATEX:
		nametop = sptr;
	}
}

/*
========================================
	後処理
========================================
*/

static void comp_end(void)
{
	if (type == CMP_GENERAL || type == CMP_C || type == CMP_KEYWORD || type == CMP_LATEX) {
		if (open_status == FIOSUC)
			ffclose();
	}
}

/*
========================================
	文字列の比較
========================================
*/

int comp_string(void)
{
	int i, c1, ccase;
	char *p, *q;

	ccase = compcase[type == CMP_DIRNAME ? CMP_FILENAME : type];
	p = name + (type == CMP_VARIABLE ? 1 : 0);
	q = nametop;

	for (i = 0; c1 = *p++; i++) {
		int c2;

		c2 = *q++;
		if (iskanji(c1)) {
			if (c1 == c2 && *p++ == *q++)
				i++;
			else
				break;
		} else {
			if (ccase) {
				if (c1 != c2)
					break;
			} else {
				if (tolower(c1) != tolower(c2))
					break;
			}
		}
	}
	return i;
}

/*
========================================
	汎用補完
========================================
*/

int comp_general(char *compname, int *cpos, int comptype)
{
	int n, ccase, begin;
	int oldcpos;
	char *p;
	char matchnext[NSTRING];
	char match[NSTRING], match1[NSTRING];

	oldcpos = *cpos;
	type = comptype;
	name = compname;
	name[*cpos] = 0;
	ccase = compcase[(type == CMP_DIRNAME) ? CMP_FILENAME : type];
	begin = 0;

	if (type == CMP_VARIABLE) {
		if (*name != '$' && *name != '%') {
			H68beep();
			return FALSE;
		}
		n = 1;
	} else if (type == CMP_C || type == CMP_KEYWORD || type == CMP_LATEX)
		begin = get_alpha_pos (name) + 1;

	if (*cpos < begin) {
		H68beep();
		return FALSE;
	}

	{
		int matchlen = 0;

		begin = (type == CMP_VARIABLE) ? 1 : 0;

		{
			int status;

			*match = 0;
			n = 0;
			status = CHANGE_STANDBY1;
			getfirst();

			while (sptr) {
				int pos;

				getbody();
				pos = comp_string();
				if (begin + pos == *cpos) {
					int len;

					n++;
					len = strlen(nametop);
					if (n == 1) {
						strcpy(match1, nametop);
						match1[len] = 0;
						matchlen = len;
					} else {
						int i;
						char *q;

						p = match1 + pos;
						q = nametop + pos;

						for(i = pos; i < matchlen; i++) {
							if (iskanji(*q)) {
								if (*p++ != *q++)
									break;
								if (*p++ != *q++)
									break;
								i++;
							} else {
								if (ccase) {
									if (*p++ != *q++)
										break;
								} else {
									if (tolower(*p++) != tolower(*q++))
										break;
								}
							}
						}
						match1[i] = '\0';
						matchlen = i;
					}
					if (*match == 0 && strncmp(name + begin, nametop, pos) == 0) {
						strcpy(match, nametop);
						match[matchlen] = 0;
					}
					if (begin + len == *cpos) {
						switch (status) {
						case CHANGE_STANDBY1:
							strcpy(match1, nametop);
							status = CHANGE_STANDBY2;
						case CHANGE_STANDBY2:
							if (strcmp(name + begin, nametop) == 0)
								status = CHANGE_READY;
							break;
						case CHANGE_READY:
							if (strcmp(name + begin, nametop) != 0) {
								strcpy(matchnext, nametop);
								status = CHANGE_OVER;
							}
						}
					}
				}
				getnext();
			}
			comp_end();

			if (n == 0) {
				H68beep();
				return FALSE;
			}
			{
				int i;
				char *q;

				p = (status == CHANGE_STANDBY1 && *match)
				    ? match : ((status == CHANGE_OVER) ? matchnext : match1);
				for (q = name + begin, i = matchlen; i > 0; i--)
					*q++ = *p++;
				*q = 0;
				*cpos = matchlen + begin;
			}
		}
	}

	if (n == 1) {
		switch (type) {
		case CMP_DIRNAME:
		case CMP_FILENAME:
			{
				int len;

				len = strlen(name);
				if (len) {
					if (sep(name[len - 1]) == slash)
						return FALSE;
				}
			}
			return quickact[CMP_FILENAME];
		default:
			return quickact[type];
		}
	}

	if (oldcpos == *cpos)
		make_compbuf(0);

	return FALSE;
}

/*
========================================
	補完名バッファの初期化
========================================
*/

int init_compbuf (void)
{
	compbuf = NULL;
	compbufp = NULL;
	compbuf_left = 0;

	ncomplist = 0;
	complist_size = 128;
	complist = malloc (sizeof (complist[0]) * complist_size);

	return ((complist) ? 0 : -1);
}

/*
========================================
	補完名バッファの解放
========================================
*/

int tini_compbuf (void)
{
	while (compbuf) {
		struct compbuf_t *next;

		next = compbuf->next;
		free (compbuf);
		compbuf = next;
	}

	if (complist)
		free (complist);

	return 0;
}

/*
========================================
	補完名バッファに登録
========================================
*/

int add_compbuf (char *name)
{
	int name_size;
	char *p;

	name_size = strlen (name) + 1;
	if (compbuf_left < name_size) {
		struct compbuf_t *newbuf;

		compbuf_left = 16384;
		newbuf = malloc (sizeof (*newbuf) + compbuf_left);
		if (newbuf == NULL) {
			tini_compbuf ();
			return -1;
		}
		newbuf->next = compbuf;
		compbuf = newbuf;
		compbufp = compbuf->buf;
	}
	p = compbufp;
	memcpy (p, name, name_size);
	compbufp += name_size;
	compbuf_left -= name_size;

	if (ncomplist == complist_size) {
		complist_size += 128;
		complist = realloc (complist, sizeof (complist[0]) * complist_size);
		if (complist == NULL) {
			tini_compbuf ();
			return -1;
		}
	}
	complist[ncomplist++] = p;

	return 0;
}

/*
========================================
	汎用補完バッファ作成
========================================
*/

static int make_compbuf(char *match)
{
	int begin, len, count;
	int i, j;
	char work[256];
	char *compname, *p;

	bcompp->b_flag &= ~BFCHG;
	if (bclear(bcompp) != TRUE)
		return 0;
	*bcompp->b_fname = 0;

	if (type == CMP_VARIABLE) {
		if (*name != '$' && *name != '%') {
			H68beep();
			return 0;
		}
		begin = 1;
	} else
		begin = 0;

	if (init_compbuf () < 0)
		return 0;

	count = 0;
	len = strlen(name);
	getfirst();

	while (sptr) {
		int pos;

		getbody();
		pos = comp_string();

		if (name[begin + pos] == 0) {
			count++;
			switch (type) {
			case CMP_FILENAME:
			case CMP_DIRNAME:
				{
					char ch, *last;

					last = p = nametop;
					while (ch = *p++) {
						if (ch == ':' || sep(ch) == slash) {
							if (*p || ch == ':')
								last = p;
						} else if (iskanji(ch))
							p++;
					}
					compname = last;
				}
				break;
			default:
				compname = nametop;
				break;
			}
			if (count == 1 && match)
				strcpy(match, nametop);

			if (add_compbuf (compname) < 0) {
				comp_end ();
				return 0;
			}
		}
		getnext();
	}
	comp_end();

	if (comp_sort)
		sort_complist (type);

	*work = 0;
	for (i = 0; i < count; i++) {
		strcat (work, complist[i]);
		if ((i & 1) == 1) {
			if (addline(bcompp, work) == FALSE) {
				tini_compbuf ();
				return 0;
			}
			*work = 0;
		} else {
			for (j = strlen(work); j < 40; j++)
				work[j] = ' ';
			work[j] = 0;
		}
	}
	if (*work) {
		if (addline(bcompp, work) == FALSE) {
			tini_compbuf ();
			return 0;
		}
	}
	keydrops();

	tini_compbuf ();

	if (count <= 1)
		return count;

	if (kbdmode != PLAY) {
		cwin_active = 1;
		cwin_height = term.t_nrow / 2 - 1;
		cwin_home = term.t_nrow - cwin_height - 1;
		cwin_nitem = (count + 1) / 2;
		cwin_base = 0;
		cwin_draw();
	}

	return count;
}

/*
========================================
	ファイル名の比較
========================================
*/

int comp_fname (char *fn1, char *fn2)
{
	int fn1_len, fn2_len;
	int isdir_fn1, isdir_fn2;
	int result;

	if (comp_sort_dir) {
		fn1_len = strlen (fn1);
		fn2_len = strlen (fn2);
		isdir_fn1 = (fn1[fn1_len - 1] == slash) ? 1 : 0;
		isdir_fn2 = (fn2[fn2_len - 1] == slash) ? 1 : 0;
		fn1_len -= isdir_fn1;
		fn2_len -= isdir_fn2;

		result = isdir_fn2 - isdir_fn1;
		if (result)
			return result;
	}

	return ((comp_sort_ignore_case) ? strcmpi (fn1, fn2) : strcmp (fn1, fn2));
}

/*
========================================
	補完リストのソート
========================================
*/

static void sort_complist (int type)
{
	int (*compfunc) (char *, char *);
	int i, j, l, r;
	char *x, **names;

	if (type == CMP_FILENAME || type == CMP_DIRNAME)
		compfunc = (int (*) (char *, char *)) comp_fname;
	else {
		compfunc = (int (*) (char *, char *)) strcmp;
		if (comp_sort_ignore_case)
			compfunc = (int (*) (char *, char *)) strcmpi;
	}

	names = complist - 1;
	r = ncomplist;
	for (l = ncomplist / 2; l > 0; l--) {
		x = names[l];
		{
			i = l;
			j = i * 2;
			while (j <= r) {
				if (j < r) {
					if ((*compfunc) (names[j], names[j + 1]) < 0)
						j++;
				}
				if ((*compfunc) (x, names[j]) >= 0)
					break;
				names[i] = names[j];
				i = j;
				j = i * 2;
			}
			names[i] = x;
		}
	}
	l = 1;
	while (r > 1) {
		x = names[r];
		names[r] = names[1];
		r--;
		{
			i = l;
			j = i * 2;
			while (j <= r) {
				if (j < r) {
					if ((*compfunc) (names[j], names[j + 1]) < 0)
						j++;
				}
				if ((*compfunc) (x, names[j]) >= 0)
					break;
				names[i] = names[j];
				i = j;
				j = i * 2;
			}
			names[i] = x;
		}
	}
}

/*
========================================
	cwin_draw
========================================
*/

static void cwin_draw(void)
{
	int i;

	if (!cwin_active)
		return;

	cwin_push_scrstat();

	for (i = -1; i <= cwin_height; i++)
		cwin_drawitem(i);

	cwin_pop_scrstat();
}

/*
========================================
	cwin_push_scrstat
========================================
*/

static void cwin_push_scrstat(void)
{
	H68curoff();
	cwin_cur_x = H68move(-1, -1) >> 16;
	cwin_cur_y = H68move(-1, -1) & 0xffff;
}

/*
========================================
	cwin_pop_scrstat
========================================
*/

static void cwin_pop_scrstat(void)
{
	H68move(cwin_cur_x, cwin_cur_y);
	H68curon();
}

/*
========================================
	cwin_drawitem
========================================
*/

static void cwin_drawitem(int no)
{
	int limit;
	char out[NSTRING], attr[NSTRING];

	limit = term.t_ncol;
	if (no == -1 || no == cwin_height) {
		int i;

		for (i = 0; i < limit; i++) {
			out[i] = '=';
			attr[i] = bcompp->b_mlcolor;
		}
		out[i] = 0;
		if (no == cwin_height)
			memcpy(out + limit / 2 - 6, " *completions* ", 15);
	} else if (no + cwin_base < cwin_nitem) {
		int i;
		LINE *lp;

		for (lp = bcompp->b_linep, i = no + cwin_base; i >= 0; i--)
			lp = lforw(lp);
		if (leftmargin) {
			sprintf(out, "%5d" "\x01", cwin_base + no + 1);
			memset(attr, 0x01, 6);
			i = 6;
		} else
			i = 0;
		if (limit > lp->l_used + i)
			limit = lp->l_used + i;

		{
			char ch, *p;

			p = lp->l_text;
			while (i < limit) {
				ch = *p++;
				if (iskanji(ch)) {
					if (i + 1 >= limit)
						break;
					out[i] = ch;
					attr[i] = 0x03;
					i++;
					ch = *p++;
				}
				out[i] = ch;
				attr[i] = 0x03;
				i++;
			}
			out[i] = attr[i] = 0;
		}
	} else
		*out = *attr = 0;

	H_PRINT3(0, cwin_home + no, "", "", out, attr, "", "", 0);
}

/*
========================================
	cwin_up
========================================
*/

void cwin_up(void)
{
	int dat, bottom;
	int nras;

	if (!cwin_active || cwin_base <= 0)
		return;

	nras = (density >= 0) ? (4 >> density) : 3;
	cwin_push_scrstat();
	bottom = cwin_home + cwin_height;
	dat = ((((bottom - 1) * nras) - 1) << 8) + ((bottom * nras) - 1);
	TXRASCPY(dat, (cwin_height - 1) * nras, 0x8003);
	cwin_base--;
	cwin_drawitem(0);
	cwin_pop_scrstat();
}

/*
========================================
	cwin_push_down
========================================
*/

void cwin_down(void)
{
	int dat, top;
	int nras;

	if (!cwin_active || cwin_base + cwin_height >= cwin_nitem)
		return;

	nras = (density >= 0) ? (4 >> density) : 3;
	cwin_push_scrstat();
	top = cwin_home;
	dat = (((top + 1) * nras) << 8) + (top * nras);
	TXRASCPY(dat, (cwin_height - 1) * nras, 0x0003);
	cwin_base++;
	cwin_drawitem(cwin_height - 1);
	cwin_pop_scrstat();
}

/*
========================================
	cwin_rollup
========================================
*/

void cwin_rollup(void)
{
	int size;

	if (!cwin_active || cwin_base + cwin_height >= cwin_nitem)
		return;

	cwin_push_scrstat();

	{
		int top, pos, dat;
		int nras;

		nras = (density >= 0) ? (4 >> density) : 3;
		pos = cwin_base + cwin_height / 2;
		if (pos + cwin_height >= cwin_nitem)
			pos = cwin_nitem - cwin_height;
		size = pos - cwin_base;
		top = cwin_home;
		dat = (((top + size) * nras) << 8) + (top * nras);
		TXRASCPY(dat, (cwin_height - size) * nras, 0x0003);
		cwin_base = pos;
	}

	{
		int i;

		for (i = cwin_height - size; i < cwin_height; i++)
			cwin_drawitem(i);
	}

	cwin_pop_scrstat();
}

/*
========================================
	cwin_rolldown
========================================
*/

void cwin_rolldown(void)
{
	int size;

	if (!cwin_active || cwin_base <= 0)
		return;

	cwin_push_scrstat();

	{
		int pos, dat, bottom;
		int nras;

		nras = (density >= 0) ? (4 >> density) : 3;
		pos = cwin_base - cwin_height / 2;
		if (pos <= 0)
			pos = 0;
		size = cwin_base - pos;
		bottom = cwin_home + cwin_height;
		dat = ((((bottom - size) * nras) - 1) << 8) + ((bottom * nras) - 1);
		TXRASCPY(dat, (cwin_height - size) * nras, 0x8003);
		cwin_base = pos;
	}

	{
		int i;

		for (i = size - 1; i >= 0; i--)
			cwin_drawitem(i);
	}

	cwin_pop_scrstat();
}

/*
========================================
	cwin_close
========================================
*/

void cwin_close(void)
{
	WINDOW *wp;

	if (!cwin_active)
		return;
	for (wp = wheadp; wp; wp = wp->w_wndp)
		wp->w_flag = WFHARD | WFMODE;
	cwin_push_scrstat();
	update(TRUE);
	cwin_pop_scrstat();

	ttrow = -1;
	cwin_active = 0;
}

/*
----------------------------------------
	name complete
----------------------------------------
*/

int name_complete(int comptype)
{
	int i;
	char match[NSTRING];
	char comp_name1[NSTRING], comp_name2[NSTRING];

	{
		LINE *lp;
		WINDOW *wp = curwp;
		char *keyword_set;

		keyword_set = curbp->b_comp_keyword_set;
		if (!keyword_set)
			keyword_set = "";

		lp = wp->w_dotp;

		for (i = 0; wp->w_doto && i < NSTRING - 1; i++) {
			int c;

			backchar(FALSE, 1);
			c = lgetc(lp, wp->w_doto);
			switch (comptype) {
			case CMP_C:
				if (!iskeyword (c, 1)) {
					forwchar(FALSE, 1);
					goto loop_out;
				}
				break;
			case CMP_FILENAME:
				if (c == ' ' || c == '\t') {
					forwchar(FALSE, 1);
					goto loop_out;
				}
				break;
			case CMP_KEYWORD:
				if (!iscsym(c) && !strchr (keyword_set, c)) {
					forwchar(FALSE, 1);
					goto loop_out;
				}
				break;
			case CMP_LATEX:
				if (!iscsym(c) && c != '\\') {
					forwchar(FALSE, 1);
					goto loop_out;
				}
				break;
			default:
				if (!iscsym(c)) {
					forwchar(FALSE, 1);
					goto loop_out;
				}
			}
		}
	  loop_out:

		{
			char *p;
			int c;

			for (p = comp_name1; i > 0; i--) {
				c = lgetc(lp, wp->w_doto);
				*p++ = c;
				if (iskanji(c))
					*p++ = lgetc(lp, wp->w_doto + 1);
				forwchar(FALSE, 1);
			}
			*p = 0;
		}

		if ((lastflag & CFCOMPLETE) == 0) {
			strcpy(comp_name, comp_name1);
			strcpy(comp_name2, comp_name1);
		} else
			strcpy(comp_name2, comp_name);
	}

	type = comptype;
	name = comp_name2;

	{
		char *comped_name;

		if (make_compbuf(match) != 1) {
			comped_name = complete("complete", 0, comp_name2, comptype, NSTRING);
			if (comped_name == 0)
				return FALSE;
		} else
			comped_name = match;

		{
			char *p;

			p = comp_name1;
			for (i = 0; *p; i++, p++) {
				if (iskanji(*p))
					p++;
			}
		}

		backdel(FALSE, i);
		linstr(comped_name);
	}

	thisflag |= CFCOMPLETE;

	return TRUE;
}
