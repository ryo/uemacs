/*
----------------------------------------
	MENU.C: MicroEMACS 3.10
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
#include "ehprint.h"
#include "emenu.h"
#include "ecall.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/menu.c,v 1.4 1992/01/04 13:11:24 SALT Exp SALT $'\n""	even\n");

/*
========================================
	使用関数の定義
========================================
*/

static void freelayout(void);
static void make_keylist(char *, int *);
static void printitem(int, int, int);
static void check_cur_base(int, int *);
static void drawitem(void);
static void cursor_up(void);
static void cursor_down(void);
static void roll_up(void);
static void roll_down(void);
static int isenaitem(void);
static int move_prev(void);
static int move_next(void);

/*
========================================
	使用変数の定義
========================================
*/

static int enaroll, home_x, home_y, width;
static int height, base, cursor, frame_color;
static int item_color, select_color, nitem;
static int old_base, old_cursor;
static LINE *head;
static M_WINDOW *m_wheadp = 0;

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

/*
========================================
	free layout
========================================
*/

static void freelayout(void)
{
	M_WINDOW *wp;

	for (wp = m_wheadp; wp;) {
		M_WINDOW *next;

		next = wp->w_wndp;
		free(wp);
		wp = next;
	}
	m_wheadp = 0;
}

/*
========================================
	save layout
========================================
*/

int savelayout(int f, int n)
{
	WINDOW *wp;
	M_WINDOW **m_prev;

	if (m_wheadp)
		freelayout();

	for (m_prev = &m_wheadp, wp = wheadp; wp; wp = wp->w_wndp) {
		M_WINDOW *m_wp;

		m_wp = (M_WINDOW *) malloc(sizeof(M_WINDOW));
		if (m_wp == 0) {
			freelayout();
			return FALSE;
		}
		m_wp->w_wndp = 0;
		m_wp->w_bufp = wp->w_bufp;
		m_wp->w_linep = wp->w_linep;
		m_wp->w_dotp = wp->w_dotp;
		m_wp->w_doto = wp->w_doto;
		m_wp->w_toprow = wp->w_toprow;
		m_wp->w_ntrows = wp->w_ntrows;
		*m_prev = m_wp;
		m_prev = &m_wp->w_wndp;
	}
	return TRUE;
}

/*
========================================
	recover layout
========================================
*/

int recoverlayout(int f, int n)
{
	WINDOW *wp;

	for (wp = wheadp; wp; wp = wp->w_wndp) {
		M_WINDOW *m_wp;

		for (m_wp = m_wheadp; m_wp; m_wp = m_wp->w_wndp) {
			if ((m_wp->w_bufp == wp->w_bufp)
			    && (m_wp->w_dotp == wp->w_dotp)
			    && (m_wp->w_doto == wp->w_doto)
			    && (m_wp->w_toprow == wp->w_toprow)
			    && (m_wp->w_ntrows == wp->w_ntrows)) {
				wp->w_linep = m_wp->w_linep;
				wp->w_flag = WFHARD | WFMODE;
			}
		}
	}
	return TRUE;
}

/*
========================================
	make keylist
========================================
*/

static void make_keylist(char *input, int *keylist)
{
	int temp = 0;

	while (*input) {
		int c;

		c = *input++;
		switch (DCHAR(c, *input)) {
		case DCHAR('A', '-'):
			temp |= ALTD;
			input++;
			break;
		case DCHAR('S', '-'):
			temp |= SHFT;
			input++;
			break;
		case DCHAR('M', '-'):
			temp |= META;
			input++;
			break;
		case DCHAR('F', 'N'):
			temp |= SPEC;
			input++;
			break;
		default:
			if (c == '^')
				temp |= CTRL;
			else {
				if (c == '\\' && *input)
					c = *input++;
				if (temp & CTRL) {
					c = toupper(c);
					if (c == '@')
						c = ' ';
				}
				temp |= c;
				*keylist++ = temp;
				temp = 0;
			}
			break;
		}

		if (*input == ';')
			input++;
	}
	*keylist = 0;
}

/*
========================================
	メニュー
========================================
*/

int openmenu(int f, int n)
{
	int exit_key[256], select_key[256];
	char title[sizeof(LINE) + NSTRING];

	{
		int maxwidth;

		{
			LINE *lp;
			BUFFER *bp;

			bp = getdefb();
			bp = getcbuf(KTEX24, bp ? bp->b_bname : "*scratch*", TRUE);
			if (bp == 0)
				return ABORT;
			maxwidth = 0;
			nitem = 0;
			for (lp = lforw(bp->b_linep); lp != bp->b_linep; lp = lforw(lp)) {
				maxwidth = max(maxwidth, lp->l_used - 1);
				nitem++;
			}
			if (nitem == 0)
				return FALSE;

			head = (LINE *) title;
			head->l_used = strlen(menu_title);
			head->l_fp = lforw(bp->b_linep);
			strcpy(head->l_text, menu_title);
		}

		TXcurof();
		TXcuron();

		make_keylist(menu_exit, exit_key);
		make_keylist(menu_select, select_key);

		enaroll = menu_roll;
		home_x = menu_home_x;
		home_y = menu_home_y;
		width = menu_width;
		height = menu_height;
		cursor = menu_cursor;
		frame_color = menu_frame_color;
		item_color = menu_item_color;
		select_color = menu_select_color;

		old_base = -1;
		old_cursor = -1;

		if (width < 0)
			width = min(maxwidth + 4, term.t_ncol - 6);
	}

	{
		int ncol, nrow;

		ncol = term.t_ncol;
		nrow = term.t_nrow >> density;

		if (height < 0)
			height = min(nitem, nrow - 6);
		if (width > ncol - 6)
			width = ncol - 6;
		if (height > nrow - 6)
			height = nrow - 6;
		if (home_x + width > ncol - 1)
			home_x = ncol - width - 1;
		if (home_y + height > nrow - 4)
			home_y = nrow - height - 4;
		if (home_x < 0)
			home_x = (ncol - (width + 2)) / 2;
		if (home_y < 0)
			home_y = (nrow - (height + 2)) / 2;
	}

	{
		int color, loop;

		base = cursor - height / 2;
		if (base + height > nitem)
			base = nitem - height;
		if (base < 0)
			base = 0;

		color = H68color(-1);
		H68curoff();

		printitem(-1, frame_color, -1);
		drawitem();
		printitem(height, frame_color, -2);

		menu_code = -1;
		menu_last_key = 0;

		for (loop = 1; loop;) {
			int key;

			{
				int *p;

				key = getkey();
				for (p = select_key; *p; p++) {
					if (key == *p) {
						menu_last_key = key;
						menu_code = cursor;
						loop = 0;
						break;
					}
				}
				for (p = exit_key; *p; p++) {
					if (key == *p) {
						menu_last_key = key;
						loop = key = 0;
						break;
					}
				}
			}

			switch (key) {
			case CTRL | '[':
			case CTRL | 'G':
				menu_last_key = key;
				loop = 0;
				break;
			case CTRL | 'M':
				menu_last_key = key;
				menu_code = cursor;
				loop = 0;
				break;
			case CTRL | 'P':
			case CTRL | 'E':
			case SPEC | '&':
				cursor_up();
				break;
			case CTRL | 'N':
			case CTRL | 'X':
			case SPEC | '-':
				cursor_down();
				break;
			case CTRL | 'V':
			case CTRL | 'C':
			case SPEC | '!':
				roll_up();
				break;
			case CTRL | 'Z':
			case CTRL | 'R':
			case SPEC | '#':
				roll_down();
			}

			drawitem();

			{
				int i;
				LINE *lp;

				for (lp = lforw(head), i = 0; i < nitem; i++, lp = lforw(lp)) {
					if (key != ' ' && lp->l_used > 3) {
						if (tolower(lp->l_text[2]) == tolower(key) && lp->l_text[3] == '|') {
							cursor = i;
							base = cursor - (height - 1) / 2;
							if (base + height > nitem)
								base = nitem - height;
							if (base < 0)
								base = 0;

							drawitem();

							if (menu_quickact) {
								menu_last_key = tolower(key);
								menu_code = cursor;
								loop = 0;
							}
							break;
						}
					}
				}
			}
		}

		keydrops();

		H68move(0, term.t_nrow);
		H68color(color);
		H68curon();
	}

	if (kbdmode != PLAY) {
		WINDOW *wp;

		for (wp = wheadp; wp; wp = wp->w_wndp)
			wp->w_flag = WFHARD | WFMODE;
	}
	menu_roll = enaroll;
	menu_home_x = home_x;
	menu_home_y = home_y;
	menu_width = width;
	menu_height = height;
	menu_cursor = cursor;
	menu_frame_color = frame_color;
	menu_item_color = item_color;
	menu_select_color = select_color;

	return TRUE;
}

/*
========================================
	チェック
========================================
*/

static void check_cur_base(int i, int *flag)
{
	if (cursor == base + i) {
		*flag = 0;
		printitem(i, select_color, base + i);
	} else
		printitem(i, item_color, base + i);
}

/*
========================================
	全アイテムの表示
========================================
*/

static void drawitem(void)
{
	if (kbdmode == PLAY)
		return;

	if (old_base == -1) {
		int i;

		for (i = 0; i < height; i++)
			printitem(i, ((cursor != base + i) ? item_color : select_color), base + i);
		old_base = base;
		old_cursor = cursor;
		return;
	}
	if (old_base == base) {
		if (old_cursor != cursor) {
			if (old_cursor >= 0 && old_cursor < nitem)
				printitem(old_cursor - base, item_color, old_cursor);
			if (cursor >= 0 && cursor < nitem)
				printitem(cursor - base, select_color, cursor);
		}
	} else {
		int i, size, flag = 1;

		if (old_cursor >= 0 && old_cursor < nitem)
			printitem(old_cursor - old_base, item_color, old_cursor);

		if (old_base < base) {
			size = base - old_base;
			if (size < height) {
				H_SCROLL(home_x + 1, home_y + 1, width - 1, height - 1, size);
				for (i = height - size; i < height; i++)
					check_cur_base(i, &flag);
			} else {
				for (i = 0; i < height; i++)
					check_cur_base(i, &flag);
			}
		} else {
			size = base - old_base;
			if (-size < height) {
				H_SCROLL(home_x + 1, home_y + 1, width - 1, height - 1, size);
				for (i = -size - 1; i >= 0; i--)
					check_cur_base(i, &flag);
			} else {
				for (i = 0; i < height; i++)
					check_cur_base(i, &flag);
			}
		}

		if (flag && cursor >= 0 && cursor < nitem)
			printitem(cursor - base, select_color, cursor);

		KFLUSHIO(0xfe);
	}

	old_base = base;
	old_cursor = cursor;
}

/*
========================================
	アイテムの表示
========================================
*/

static void printitem(int y, int color, int pos)
{
	char item[256];

	if (kbdmode == PLAY)
		return;

	y = home_y + y + 1;

	pos++;
	if (pos >= 0 && pos <= nitem) {
		int i, n;
		char *p, *q;
		LINE *lp;

		for (lp = head, i = 0; i < pos; i++)
			lp = lforw(lp);

		if (lp->l_used >= 1 && lp->l_text[0] == ';')
			n = min(lp->l_used, width + 1);
		else
			n = min(lp->l_used, width);

		p = item;
		q = lp->l_text + 1;
		for (i = n; i > 1; i--) {
			if (iskanji(*q)) {
				if (i == 2) {
					*p++ = ' ';
					break;
				} else {
					*p++ = *q++;
					i--;
				}
			}
			*p++ = *q++;
		}

		for (i = width - n + 1; i > 1; i--)
			*p++ = ' ';

		*p = 0;
	} else {
		int i;
		char *p;

		for (p = item, i = width; i > 0; i--)
			*p++ = ' ';
		*p = 0;
	}

	B_PUTMES(frame_color, home_x, y, 0, " ");
	B_PUTMES(color, home_x + 1, y, width - 1, item);
	B_PUTMES(frame_color, home_x + width + 1, y, 0, " ");
}

/*
========================================
	is enable item
========================================
*/

static int isenaitem(void)
{
	int i;
	LINE *lp;

	for (lp = head, i = 0; i <= cursor; i++, lp = lforw(lp));
	if (lp->l_used < 2)
		return 0;
	return (*lp->l_text != ';') ? 1 : 0;
}

/*
========================================
	move prev & check enable item
========================================
*/

static int move_prev(void)
{
	if (cursor == base)
		base--;
	cursor--;
	return isenaitem();
}

/*
========================================
	move next & check enable item
========================================
*/

static int move_next(void)
{
	if (cursor == base + height - 1)
		base++;
	cursor++;
	return isenaitem();
}

/*
========================================
	cursor up
========================================
*/

static void cursor_up(void)
{
	if (cursor >= nitem) {
		base = cursor - height - 1;
		cursor = nitem - 1;
		if (base < 0)
			base = 0;
		if (isenaitem())
			return;
	}
	if (cursor < 0) {
		if (enaroll) {
			base = cursor - height - 1;
			cursor = nitem - 1;
			if (base < 0)
				base = 0;
		} else {
			base = 0;
			cursor = 0;
		}
		if (isenaitem())
			return;
	}

	{
		int old_base, old_cursor, i;

		old_base = base;
		old_cursor = cursor;
		if (enaroll) {
			for (i = 0; i < nitem; i++) {
				if (move_prev())
					break;
			}
			if (i == nitem) {
				base = old_base;
				cursor = old_cursor;
			}
		} else if (cursor > 0) {
			for (i = cursor; i > 0; i--) {
				if (move_prev())
					break;
			}
			if (i == 0) {
				base = old_base;
				cursor = old_cursor;
			}
		}
	}
}

/*
========================================
	cursor down
========================================
*/

static void cursor_down(void)
{
	if (cursor < 0) {
		base = 0;
		cursor = 0;
		if (isenaitem())
			return;
	}
	if (cursor >= nitem) {
		if (enaroll) {
			base = 0;
			cursor = 0;
		} else {
			base = cursor - height - 1;
			cursor = nitem - 1;
			if (base < 0)
				base = 0;
		}
		if (isenaitem())
			return;
	}

	{
		int old_base, old_cursor, i;

		old_base = base;
		old_cursor = cursor;
		if (enaroll) {
			for (i = 0; i < nitem; i++) {
				if (move_next())
					break;
			}
			if (i == nitem) {
				base = old_base;
				cursor = old_cursor;
			}
		} else if (cursor < nitem) {
			for (i = cursor; i < nitem; i++) {
				if (move_next())
					break;
			}
			if (i == nitem) {
				base = old_base;
				cursor = old_cursor;
			}
		}
	}
}

/*
========================================
	roll up
========================================
*/

static void roll_up(void)
{
	int old_base, old_cursor;
	int i, count, flag;

	if (cursor < 0)
		cursor = 0;
	if (cursor > nitem - 1)
		cursor = nitem - 1;

	old_base = base;
	old_cursor = cursor;

	count = nitem - cursor - 1;
	if (count >= height / 2) {
		if (height > 1)
			count = height / 2;
		else if (count > 0)
			count = 1;
	}
	flag = isenaitem();
	for (i = count; i > 0; i--) {
		if (flag == 0) {
			if (move_next())
				break;
		} else
			flag = move_next();
	}
	if (i != 0) {
		base = cursor - height / 2;
		if (base + height > nitem)
			base = nitem - height;
		if (base < 0)
			base = 0;
		return;
	}
	base = old_base;
	cursor = old_cursor;

	if (height == 1) {
		base++;
		cursor++;
	} else {
		base = base + height / 2;
		cursor = cursor + height / 2;
	}
	if (base > nitem - height)
		base = nitem - height;
	if (base < 0)
		base = 0;
	if (cursor > nitem - 1)
		cursor = nitem - 1;

	{
		int old_base2, old_cursor2;

		old_base2 = base;
		old_cursor2 = cursor;

		if (!isenaitem()) {
			for (i = cursor; i < nitem; i++) {
				if (move_next())
					break;
			}
			if (i == nitem) {
				base = old_base2;
				cursor = old_cursor2;

				for (i = cursor; i > 0; i--) {
					if (move_prev())
						break;
				}
				if (i == 0) {
					base = old_base;
					cursor = old_cursor;
				}
			}
		}
	}

	return;
}

/*
========================================
	roll down
========================================
*/

static void roll_down(void)
{
	int count, i, flag;
	int old_base, old_cursor;

	if (cursor < 0)
		cursor = 0;
	if (cursor > nitem - 1)
		cursor = nitem - 1;

	old_base = base;
	old_cursor = cursor;

	count = cursor;
	if (count >= height / 2) {
		if (height > 1)
			count = height / 2;
		else if (count > 0)
			count = 1;
	}
	flag = isenaitem();
	for (i = count; i > 0; i--) {
		if (flag == 0) {
			if (move_prev())
				break;
		} else
			flag = move_prev();
	}
	if (i != 0) {
		base = cursor - height / 2;
		if (base + height > nitem)
			base = nitem - height;
		if (base < 0)
			base = 0;
		return;
	}
	base = old_base;
	cursor = old_cursor;

	if (height == 1) {
		base--;
		cursor--;
	} else {
		base = base - height / 2;
		cursor = cursor - height / 2;
	}
	if (base > nitem - height)
		base = nitem - height;
	if (base < 0)
		base = 0;
	if (cursor < 0)
		cursor = 0;

	{
		int old_base2, old_cursor2;

		old_base2 = base;
		old_cursor2 = cursor;

		if (!isenaitem()) {
			for (i = cursor; i > 0; i--) {
				if (move_prev())
					break;
			}
			if (i == 0) {
				base = old_base2;
				cursor = old_cursor2;

				for (i = cursor; i < nitem; i++) {
					if (move_next())
						break;
				}
				if (i == nitem) {
					base = old_base;
					cursor = old_cursor;
				}
			}
		}
	}

	return;
}
