/*
----------------------------------------
	HUMAN68.C: MicroEMACS 3.10
----------------------------------------
*/

#define	human68def 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <ctype.h>

#include "estruct.h"
#include "etype.h"
#include "edef.h"
#include "elang.h"
#include "ehprint.h"
#include "ekanji.h"
#include "ecall.h"
#include "fepctrl.h"

/*
========================================
	RCS id の設定
========================================
*/

__asm("	dc.b	'$Header: f:/SALT/emacs/RCS/human68.c,v 1.9 1992/02/15 07:19:46 SALT Exp SALT $'\n""	even\n");

/*
----------------------------------------
	定数宣言
----------------------------------------
*/

#define	BEL			0x07
#define	ESC			0x1b
#define	SPACE		32
#define	BDCOLOR		0x0000
#define	FGCOLOR		0xFFFE
#define	MSBGCOLOR	0x4022
#define	MSFGCOLOR	0xDE6C
#define	IBUFSIZE	32
#define	LEFT_BUTTON	0
#define	RIGHT_BUTTON 1
#define	SHIFT_KEY	0x0001
#define	CTRL_KEY	0x0002
#define	XF1_KEY		0x0800
#define	XF2_KEY		0x1000
#define	XF3_KEY		0x2000
#define	XF4_KEY		0x4000
#define	XF5_KEY		0x8000
#define	CAPS_KEY	0x0080
#define	INS_KEY		0x0100
#define	FONTSIZE	2048

/*
----------------------------------------
	変数宣言
----------------------------------------
*/

static int old_palette[16];
static int old_screenmode, old_funckeymode;
static int in_next = 0, in_last = 0;
static int oldbut, normal_funckeymode = TRUE;
static char old_funckey[712];
static char emacs_funckey[712];
static char in_buf[IBUFSIZE];

static int curx, cury;
static struct XLINEPTR Xdraw = {2, 0, 0, 1024, 0xffff};
static struct XLINEPTR Xcls = {2, 0, 0, 1024, 0x0000};
static struct YLINEPTR Ydraw = {2, 0, 0, 1024, 0xffff};
static struct YLINEPTR Ycls = {2, 0, 0, 1024, 0x0000};

static char *default_funckey[32] =
{

#ifndef NOEDBIND

	"\xfe" "先頭行 " "\x1f\x31",
	"\xfe" "最終行 " "\x1f\x32",
	"\xfe" " 置換  " "\x1f\x33",
	"\xfe" " 検索  " "\x1f\x34",
	"\xfe" "次検索 " "\x1f\x35",
	"\xfe" " MARK  " "\x1f\x36",
	"\xfe" " 削除  " "\x1f\x37",
	"\xfe" " 複写  " "\x1f\x38",
	"\xfe" " 貼付  " "\x1f\x39",
	"\xfe" "TAGJMP " "\x1f\x30",

	"\xfe" "SCREEN " "\x1f\xb1",
	"\xfe" "DIRED  " "\x1f\xb2",
	"\xfe" "確置換 " "\x1f\xb3",
	"\xfe" " 検索  " "\x1f\xb4",
	"\xfe" "次検索 " "\x1f\xb5",
	"\xfe" " TAGS  " "\x1f\xb6",
	"\xfe" "       " "\x1f\xb7",
	"\xfe" "       " "\x1f\xb8",
	"\xfe" " MENU  " "\x1f\xb9",
	"\xfe" "COMPILE" "\x1f\xba",

#else

	"\xfe" "dired  " "\x1f\x31",
	"\xfe" "       " "\x1f\x32",
	"\xfe" "F 置換 " "\x1f\x33",
	"\xfe" "F 検索 " "\x1f\x34",
	"\xfe" "F次検索" "\x1f\x35",
	"\xfe" "m-mark " "\x1f\x36",
	"\xfe" " 削除  " "\x1f\x37",
	"\xfe" " 複写  " "\x1f\x38",
	"\xfe" " 貼付  " "\x1f\x39",
	"\xfe" "二重化 " "\x1f\x30",

	"\xfe" "       " "\x1f\xb1",
	"\xfe" "       " "\x1f\xb2",
	"\xfe" "確置換 " "\x1f\xb3",
	"\xfe" "R 検索 " "\x1f\xb4",
	"\xfe" "R次検索" "\x1f\xb5",
	"\xfe" "       " "\x1f\xb6",
	"\xfe" "       " "\x1f\xb7",
	"\xfe" "       " "\x1f\xb8",
	"\xfe" "       " "\x1f\xb9",
	"\xfe" "       " "\x1f\xba",

#endif

	"\x1f!",
	"\x1f#",
	"\x1f$",
	"\x1f%",
	"\x1f&",
	"\x1f(",
	"\x1f)",
	"\x1f-",
	"\x1f=",
	"\x1f^",
	"\x1f+",
	"\x1f\x1f"
};

/*
========================================
	使用関数の定義
========================================
*/

static int checkmouse(void);
static int doschar(void);
static int extcode(int, int);
static int get_keyin(void);
static int getpcode(char *);
static int hsize(void);
static int readfont(char *, int, int);
static void setcolortable(void);
static void setkeytable(void);
static int shift_sense(void);
static int vsize(void);

/*
----------------------------------------
	Shift センス
----------------------------------------
*/

static int shift_sense(void)
{
	return (((K_KEYBIT(0x0b) & 0x03) << 14)
		| ((K_KEYBIT(0x0a) & 0xe0) << 6)
		| ((K_KEYBIT(0x0e) & 0x0f)));
}


/*
========================================
	入力バッファ初期化
========================================
*/

static inline void in_init(void)
{
	in_next = in_last = 0;
}

/*
========================================
	バッファ残チェック
========================================
*/

static inline int in_check(void)
{
	return in_next == in_last ? FALSE : TRUE;
}

/*
========================================
	バッファリング
========================================
*/

static inline void in_put(int event)
{
	in_buf[in_last++] = event;
	in_last &= (IBUFSIZE - 1);
}

/*
========================================
	バッファから得る
========================================
*/

static inline int in_get(void)
{
	int event;

	event = in_buf[in_next++];
	in_next &= (IBUFSIZE - 1);
	return event;
}

/*
========================================
	OPT1 チェック
========================================
*/

static inline int OPT1_sense(void)
{
	return (K_KEYBIT(0x0e) & opt1key) != 0;
}

/*
========================================
	OPT2 チェック
========================================
*/

static inline int OPT2_sense(void)
{
	return (K_KEYBIT(0x0e) & opt2key) != 0;
}

/*
========================================
	画面オープン
========================================
*/

int H68open(void)
{
	int i, temp;
	char restmp[6];
	TERM *tp = &term;

	LEDmode = K_SFTSNS();

	old_funckeymode = C_FNKMOD(-1);
	old_screenmode = C_WIDTH(-1);
	sprintf(restmp, "%d", CRTMOD(-1));
	for (i = 0; i < 16; i++)
		old_palette[i] = TPALET2(i, -1);

	tp->t_mcol = NCOL - 1;
	tp->t_mrow = NROW - 1;
	temp = vsize();
	if (temp == -1)
		exit(1);
	tp->t_nrow = temp - 1;
	temp = hsize();
	if (temp == -1)
		exit(1);
	tp->t_ncol = temp;
	tp->t_scrsiz = tp->t_ncol - 10;
	ttrow = ttcol = 999;

	TXcuron();
	H68cres(restmp);
	SKEY_MOD(0, 0, 0);
	MS_CURST((tp->t_ncol - 2) << 3, 0);
	setkeytable();
	setcolortable();
	in_init();

	fep_init(include_fep_mode);

	return TRUE;
}

/*
========================================
	解像度設定
========================================
*/

int H68cres(char *v)
{
	int mode, old[16];
	int dens;
	TERM *tp = &term;

	if (strcmp(sres, v) == 0)
		return TRUE;

	mode = asc_int(v) & ~128;
	dens = (asc_int(v) & 128) ? 1 : 0;
	if (mode < 0 || mode >= MAXMOD || dens && mode > 16) {
		mlwrite(KTEX223);
		return FALSE;
	}

	density = dens;
	H_DENSITY(density);

	{
		int i;

		H68clear();

		fkmode = tinf[mode].funckey;
		for (i = 0; i <= 15; i++)
			old[i] = TPALET2(i, -1);

		if (fkmode) {
			int old_mode, old_funckeymode;

			old_mode = CRTMOD(-1);
			old_funckeymode = (C_FNKMOD(-1) == 3) ? FALSE : TRUE;

			if (mode != old_mode)
				C_WIDTH(tinf[mode].conmode);
			if (normal_funckeymode) {
				if (mode != old_mode || !old_funckeymode)
					C_FNKMOD(0);
			} else {
				fkmode = FALSE;
				if (mode != old_mode || old_funckeymode)
					C_FNKMOD(3);
			}
		} else {
			CRTMOD(mode | 0x100);
			fkmode = FALSE;
			C_FNKMOD(3);
		}
		for (i = 0; i <= 15; i++)
			TPALET2(i, old[i]);
	}

	newsize(TRUE, (vsize() + (!fkmode && tinf[mode].funckey ? 1 : 0)) << density);
	newwidth(TRUE, tinf[mode].hsize);
	B_CONSOL(0, 0, tp->t_ncol, tp->t_nrow >> density);
	MS_LIMIT(0, 0, (tp->t_ncol << 3) + 7, (tp->t_nrow << (4 - density)) + 15);
	strcpy(sres, v);

	return TRUE;
}

/*
========================================
	画面クローズ
========================================
*/

int H68close(void)
{
	int i;

	fep_term();

	H68clear();
	H68color(3);
	for (i = 0; i < 16; i++)
		TPALET2(i, old_palette[i]);
	MS_CUROF();
	TXcurof();
	H_CUROFF();
	OS_CURON();
	SKEY_MOD(-1, 0, 0);
	FNCKEYST(0, old_funckey);
	K_INSMOD((LEDmode & 0x0800) ? 0xff : 0);

	if (old_funckeymode != C_FNKMOD(-1))
		C_FNKMOD(old_funckeymode);
	if (old_screenmode != tinf[asc_int(sres)].conmode)
		C_WIDTH(old_screenmode);

	return TRUE;
}

/*
========================================
	カーソル on
========================================
*/

int H68curon(void)
{
	return H_CURON();
}

/*
========================================
	カーソル off
========================================
*/

int H68curoff(void)
{
	return H_CUROFF();
}

/*
========================================
	画面クリア
========================================
*/

void H68clear(void)
{
	int flag;

	flag = H_CUROFF();
	B_CLR_AL();
	H_ERA63();
	if (flag)
		H_CURON();
}

/*
========================================
	文字色設定
========================================
*/

int H68color(int color)
{
	return B_COLOR(color);
}

/*
========================================
	クロスカーソル
========================================
*/

void disp_cross_cur(void)
{
	if (ena_zcursor && zcursor > 0) {
		int pos, hcol, hrow;

		pos = H68move(-1, -1);
		hcol = ((pos >> 16) << 3);
		hrow = (((pos + 1) & 0xffff) << (4 - density)) - 1;

		if (curx != hcol && zcursor > 1) {
			Ycls.x = curx;
			TXYLINE(&Ycls);
			Ydraw.x = hcol;
			TXYLINE(&Ydraw);
			curx = hcol;
		}
		if (cury != hrow || zcursor > 1) {
			Xcls.y = cury;
			TXXLINE(&Xcls);
			Xdraw.y = hrow;
			TXXLINE(&Xdraw);
			cury = hrow;
		}
	}
}

/*
========================================
	カーソル移動
========================================
*/

int H68move(short col, short row)
{
	if (col >= 0 && row >= 0) {
		B_LOCATE(col, row >> density);
		H_LOCATE(col, row);
		disp_cross_cur();
	}

	return H_LOCATE(-1, -1);
}

/*
========================================
	テキストカーソルオン
========================================
*/

void TXcuron(void)
{
	curx = cury = -1;
}

/*
========================================
	テキストカーソルオフ
========================================
*/

void TXcurof(void)
{
	Xcls.y = cury;
	Ycls.x = curx;
	TXXLINE(&Xcls);
	TXYLINE(&Ycls);
}

/*
========================================
	一文字入力
========================================
*/

int H68getc(void)
{
	disp_cross_cur();

	while (1) {
		if (in_check())
			return in_get();
		if (typahead() || OPT2_sense())
			return doschar();

		while (1) {
			if (typahead() || OPT2_sense())
				break;
			if (checkmouse())
				break;
		}
	}
}

/*
========================================
	警告音を出す
========================================
*/

int H68beep(void)
{
	int old;

	keydrops();
	if (vbell == 0) {
		B_PUTC(BEL);
		return TRUE;
	}
	old = TPALET2(0, -1);

	{
		int i;

		for (i = 1; i <= vbell; i++) {
			int j;

			for (j = 0; j < 32; j += 8) {
				while (!(B_BPEEK(0xe88001) & 0x10));
				B_WPOKE(0xE82200, (j << 11) + (j << 6) + (j << 1));
				while (B_BPEEK(0xe88001) & 0x10);
			}
		}
	}

	TPALET2(0, old);
	return TRUE;
}

/*
========================================
	ファンクションキー処理
========================================
*/

static int extcode(int flag, int c)
{
	int shift, d = 0;

	shift = shift_sense();

	if (flag) {
		d |= SPEC;
		if (shift & SHIFT_KEY)
			d |= SHFT;
		if (shift & CTRL_KEY)
			d |= CTRL;
		if (shift & opt1key)
			d |= ALTD;
		if (shift & XF1_KEY)
			d |= xf1_key;
		if (shift & XF2_KEY)
			d |= xf2_key;
		if (shift & XF3_KEY)
			d |= xf3_key;
		if (shift & XF4_KEY)
			d |= xf4_key;
		if (shift & XF5_KEY)
			d |= xf5_key;
		if (c >= 0xb1 && c <= 0xb9)
			return (d | c - 0xb0 + '0');
		if (c == 0xba)
			return (d | '0');
		return (d | c);
	} else {
		if (shift & opt1key)
			d |= ALTD;
		if (shift & XF1_KEY)
			d |= xf1_key;
		if (shift & XF2_KEY)
			d |= xf2_key;
		if (shift & XF3_KEY)
			d |= xf3_key;
		if (shift & XF4_KEY)
			d |= xf4_key;
		if (shift & XF5_KEY)
			d |= xf5_key;
		if ((d & CTRL) && c >= '@')
			c = toupper(c) - '@';
		if (c == ' ' && (shift & CTRL_KEY))
			d |= CTRL, c = 0;
		return (d | c);
	}
}

/*
========================================
	キーボードから一文字得る
========================================
*/

static int get_keyin(void)
{
	int ver2;

	ver2 = (VERNUM() & 0xff00) >= 0x0200;
	while ((K_KEYSNS() & 0xff) == 0) {
		if (ver2) {
			/* CHANGE_PR (); for bgdrv */
			asm volatile("dc.w $ffff": /* no outputs */ : /* no inputs */ :"d0");
		}

		if (newscroll && ena_hscroll && OPT2_sense() && !OPT1_sense()) {
			int h_move = 0, v_move = 0;
			int mode, loop = 1;

			while (loop && OPT2_sense() && !OPT1_sense()) {
				int curkey;

				loop = 0;
				mode = curbp->b_mode & MDDIRED;

				curkey = K_KEYBIT(0x7);
				if (curkey & 8) {
					thisflag = 0;
					if (lastflag & CFCPCN != 0)
						curwp->w_doto = getgoal(curwp->w_dotp, curwp->w_bufp->b_tabs);
					if (!mode)
						backchar(TRUE, 1);
					h_move = 1;
					v_move = 0;
					loop = 1;
				} else if ((curkey & 16) || (curkey & 2)) {
					thisflag = 0;
					ena_wait_vdisp = -1;
					(mode ? do_dired_backline : backline) (TRUE, 1);
					curwp->w_doto = 0;
					h_move = 0;
					v_move = 1;
					loop = 1;
				} else if (curkey & 32) {
					thisflag = 0;
					if (lastflag & CFCPCN != 0)
						curwp->w_doto = getgoal(curwp->w_dotp, curwp->w_bufp->b_tabs);
					if (!mode)
						forwchar(TRUE, 1);
					h_move = 1;
					v_move = 0;
					loop = 1;
				} else if ((curkey & 64) || (curkey & 1)) {
					thisflag = 0;
					ena_wait_vdisp = -1;
					(mode ? do_dired_forwline : forwline) (TRUE, 1);
					curwp->w_doto = 0;
					h_move = 0;
					v_move = 1;
					loop = 1;
				}
				if (loop) {
					update(TRUE);
					lastflag = thisflag;
				}
				ena_wait_vdisp = 0;
			}

			if (h_move)
				curgoal = getccol(FALSE);
			if (v_move)
				curwp->w_doto = getgoal(curwp->w_dotp, curwp->w_bufp->b_tabs);
			if (v_move || h_move)
				update(TRUE);
			keydrops();
		}
	}
	return K_KEYINP();
}

/*
========================================
	ファンクションチェック
========================================
*/

static int doschar(void)
{
	int c, ret, func = 0;

	c = get_keyin();
	if (c == 0x1f) {
		func = 1;
		c = get_keyin();
	}
	ret = 0;
	c = extcode(func, c);
	if (c >= 0x1000000) {
		in_put((c >> 24) & CHARMASK);
		ret--;
	}
	if (c >= 0x10000) {
		in_put((c >> 16) & CHARMASK);
		ret--;
	}
	if (c >= 0x100) {
		in_put((c >> 8) & CHARMASK);
		in_put(c & CHARMASK);
		return (ret);
	}
	return (c);
}

/*
========================================
	キー押下チェック
========================================
*/

int typahead(void)
{
	return K_KEYSNS() & 0xff ? TRUE : FALSE;
}

/*
========================================
	パレットセット
========================================
*/

int spal(char *pstr)
{
	int r, g, b;
	int pal, color;

	if (sscanf(pstr, " %u %u %u %u", &pal, &r, &g, &b) != 4)
		return FALSE;

	color = ((r & 31) << 6) | ((g & 31) << 11) | ((b & 31) << 1);
	TPALET(pal & 15, color);
	return TRUE;
}

/*
========================================
	パレットセット２
========================================
*/

int spal2(char *pstr)
{
	int r, g, b;
	int pal, color;

	if (sscanf(pstr, " %u %u %u %u", &pal, &r, &g, &b) != 4)
		return FALSE;

	color = ((r & 31) << 6) | ((g & 31) << 11) | ((b & 31) << 1);
	TPALET2(pal & 15, color);
	return TRUE;
}

/*
========================================
	カラーコードを得る
========================================
*/

static int getpcode(char *ep)
{
	int r, g, b;

	if (sscanf(ep, " %u %u %u", &r, &g, &b) != 3)
		return FALSE;

	return (g << 11) | (r << 6) | (b << 1);
}

/*
----------------------------------------
	ファンクションキーのラベル
----------------------------------------
*/

int fnclabel(int f, int n)
{
	int len, status;
	char label[9], funckey_buff[32];

	if (f == FALSE) {
		mlwrite(KTEX159);
		return FALSE;
	}
	if (n < 1 || n > 20) {
		mlwrite(KTEX160);
		return FALSE;
	}
	FNCKEYGT(n, funckey_buff);
	his_disable();
	status = mlreply(KTEX161, label, 9);
	if (status != TRUE)
		return status;

	len = strlen(label);
	if (len > 7)
		len = 7;
	memset(label + len, ' ', 7 - len);

	memcpy(funckey_buff + 1, label, 7);
	memcpy(default_funckey[n - 1] + 1, label, 7);

	FNCKEYST(n, funckey_buff);
	FNCKEYGT(0, emacs_funckey);
	return TRUE;
}

/*
========================================
	マウスチェック
========================================
*/

static int checkmouse(void)
{
	int ret, newbut, shift;
	int mousecol, mouserow;

	if (mouseflag == FALSE)
		return FALSE;

	ret = MS_CURGT();
	mousecol = ret >> 19;
	mouserow = (ret & 0xffff) >> (4 - density);

	ret = MS_GETDT();
	shift = shift_sense();
	newbut = ((ret & 0x00ff) ? 2 : 0) + ((ret & 0xff00) ? 1 : 0);

	{
		int i, k;

		for (i = 0, k = 1; i < 2; i++, k <<= 1) {
			if ((oldbut & k) != (newbut & k)) {
				int event, extend = 0;
				int check = 0;

				in_put(0);
				extend |= MOUS;
				if (shift & SHIFT_KEY)
					check = 1;
				if (shift & CTRL_KEY)
					extend |= CTRL;
				if (shift & opt1key)
					extend |= ALTD;
				if (shift & XF1_KEY)
					extend |= xf1_key;
				if (shift & XF2_KEY)
					extend |= xf2_key;
				if (shift & XF3_KEY)
					extend |= xf3_key;
				if (shift & XF4_KEY)
					extend |= xf4_key;
				if (shift & XF5_KEY)
					extend |= xf5_key;
				in_put(extend >> 8);
				in_put(mousecol);
				in_put(mouserow);
				switch (i) {
				  case LEFT_BUTTON:
					  event = check ? 'A' : 'a';
					  break;
				  case RIGHT_BUTTON:
					  event = check ? 'E' : 'e';
					  break;
				  default:
					  event = 0;
					  break;
				}
				event += (newbut & k) ? 0 : 1;
				in_put(event);
				oldbut = newbut;
				return TRUE;
			}
		}
	}

	return FALSE;
}

/*
----------------------------------------
	ＣＡＰＳ反転
----------------------------------------
*/

int capsreverse(int f, int n)
{
	int status;

	status = K_SFTSNS();
	if (status & CAPS_KEY) {
		mlwrite(KTEX222);
		LEDMOD(3, 0);
	} else {
		mlwrite(KTEX221);
		LEDMOD(3, 1);
	}
	return TRUE;
}

/*
----------------------------------------
	フォントリセット
----------------------------------------
*/

int fontreset(int f, int n)
{
	H_INIT();
	upwind();
	f_set_emacs();
	return TRUE;
}

/*
----------------------------------------
	ＯＶＥＲトグル
----------------------------------------
*/

int toggleover(int f, int n)
{
	WINDOW *wp = curwp;

	wp->w_bufp->b_mode ^= MDOVER;
	updinsf = TRUE;
	upmode();
	return TRUE;
}

/*
========================================
	adjust ins mode
========================================
*/

void adj_ins_mode(void)
{
	K_INSMOD((curbp->b_mode & MDOVER) ? 0x00 : 0xff);
}

/*
========================================
	ファンクションキー設定
========================================
*/

static void setkeytable(void)
{
	int i;

	FNCKEYGT(0, old_funckey);
	for (i = 0; i < 20; i++)
		strcpy(emacs_funckey + i * 32, default_funckey[i]);
	for (i = 20; i < 32; i++)
		strcpy(emacs_funckey + 640 + (i - 20) * 6, default_funckey[i]);
	FNCKEYST(0, emacs_funckey);
}

/*
========================================
	カラー設定
========================================
*/

static void setcolortable(void)
{
	char *ep;

	ep = getenv("BDCOLOR");
	TPALET(0, ep ? getpcode(ep) : BDCOLOR);
	ep = getenv("FGCOLOR");
	TPALET(3, ep ? getpcode(ep) : FGCOLOR);
	ep = getenv("MSFGCOLOR");
	TPALET(4, ep ? getpcode(ep) : -2);
	ep = getenv("MSBGCOLOR");
	TPALET(8, ep ? getpcode(ep) : -2);
}

/*
========================================
	行数を得る
========================================
*/

static int vsize(void)
{
	int mode;

	mode = CRTMOD(-1);
	if (mode >= MAXMOD) {
		mlwrite(KTEX223);
		return -1;
	}
	return tinf[mode].vsize - (tinf[mode].funckey ? 1 : 0);
}

/*
========================================
	桁数を得る
========================================
*/

static int hsize(void)
{
	int mode;

	mode = CRTMOD(-1) & 0x1f;
	if (mode >= MAXMOD) {
		mlwrite(KTEX223);
		return -1;
	}
	return tinf[mode].hsize;
}

/*
----------------------------------------
	改行コード表示切り替え
----------------------------------------
*/

int togglecr(int f, int n)
{
	dispcr = (dispcr == TRUE) ? FALSE : TRUE;
	upwind();
	return TRUE;
}

/*
----------------------------------------
	ファンクションキーオン
----------------------------------------
*/

int turnonfunckey(int f, int n)
{
	if (fkmode)
		return TRUE;

	if (strcmp(sres, "16") != 0) {
		mlwrite(KTEX225);
		return FALSE;
	}
	C_FNKMOD(0);
	newsize(TRUE, vsize() << density);
	normal_funckeymode = fkmode = TRUE;
	return TRUE;
}

/*
----------------------------------------
	ファンクションキーオフ
----------------------------------------
*/

int turnofffunckey(int f, int n)
{
	if (!fkmode)
		return TRUE;

	if (strcmp(sres, "16") != 0) {
		mlwrite(KTEX225);
		return FALSE;
	}
	C_FNKMOD(3);
	newsize(TRUE, (vsize() + 1) << density);
	normal_funckeymode = fkmode = FALSE;
	return TRUE;
}

/*
----------------------------------------
	Function Key 設定
----------------------------------------
*/

void f_set_shell(void)
{
	FNCKEYST(0, old_funckey);
}

void f_set_emacs(void)
{
	FNCKEYST(0, emacs_funckey);
}

/*
----------------------------------------
	load normal font
----------------------------------------
*/

int loadfont(int f, int n)
{
	return readfont(font, 0, 0);
}

/*
----------------------------------------
	load c font
----------------------------------------
*/

int loadcfont(int f, int n)
{
	return readfont(exfont, 0, cbold);
}

/*
----------------------------------------
	load normal font (half)
----------------------------------------
*/

int loadfont_h(int f, int n)
{
	return readfont(font_h, 1, 0);
}

/*
----------------------------------------
	load c font (half)
----------------------------------------
*/

int loadcfont_h(int f, int n)
{
	return readfont(exfont_h, 1, cbold);
}

/*
----------------------------------------
	フォント読込本体
----------------------------------------
*/

static int readfont(char *fontarea, int fsize, int flag)
{
	char *fname;

	fname = singleexpwild(gtfilename(KTEX254));

	if (fname == 0) {
	  fail:
		mlwrite(KTEX253, fixnull(fname));
		return FALSE;
	}

	{
		FILE *fp;
		int head;
		int size;

		head = (32 * 16) >> fsize;
		size = (FONTSIZE >> fsize) - head;

		fp = fopen(fname, "rb");
		if (fp == 0)
			goto fail;
		if (fseek(fp, head, 0)) {
			fclose(fp);
			goto fail;
		}
		if (fread(fontarea + head, 1, size, fp) != size) {
			fclose(fp);
			goto fail;
		}
		fclose(fp);
	}
	if (flag) {
		int i, size;
		char *p;

		size = FONTSIZE >> fsize;
		for (p = fontarea, i = 0; i < size; i++)
			*p++ |= *p >> 1;
	}

	upwind();

	return TRUE;
}

/*
--------------------------------------------
	ノーマルフォントを C フォントエリアに展開
--------------------------------------------
*/

void normalfont2cfont(char *src, char *dst, int size)
{
	int i;
	char *p, *q;

	if (_dump_flag > 0)
		return;

	for (p = dst, q = src, i = 0; i < size; i++) {
		char c;

		c = *q++;
		*p++ = c | (c >> 1);
	}
}
