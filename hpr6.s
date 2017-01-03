*-----------------------------------------------------------------------------
*	Hi-Speed PRINT routine (6dot font version) by SALT	1993/08/15
*-----------------------------------------------------------------------------

		.xref		__dump_flag
		.xref		_issuper
		.xref		char_table
		.xref		cur_adr

		.xdef		_H6_INIT
		.xdef		_H6_CURSET
		.xdef		_H6_PRINT3
		.xdef		hcurxor6_table
		.xdef		_font6
		.xdef		_exfont6
		.xdef		_cur_pat_6x12


*============================================================================


_FNTADR		.equ		$16
_TXRASCPY	.equ		$df
_SUPER		.equ		$ff20
CRTC_R21	.equ		$ffe8002a
CRTC_R23	.equ		$ffe8002e		* MASK REG

*===========================================================
*
*	void	H6_INIT (void)
*
*===========================================================

_H6_INIT:
		tst.l		__dump_flag
		bgt		h6_init_b
		bsr		get_romfont
		bsr		cpy_exfont6
		bsr		make_tate
h6_init_b
		rts

cpy_exfont6	movem.l		a0-a1,-(sp)
		lea		_font6,a0
		lea		_exfont6,a1
		move.w		#(2048/4)-1,d0
cpy_exfont6_l	move.l		(a0)+,(a1)+
		dbra		d0,cpy_exfont6_l
		movem.l		(sp)+,a0-a1
		rts

get_romfont	movem.l		d1-d2/a0-a1,-(sp)
		move.l		#$00,d1
		move.w		#$7f,d2
		bsr		get_romfont2
		move.l		#$a0,d1
		move.w		#$40,d2
		bsr		get_romfont2

		lea		_font6+$0d*16,a0
		lea		crchar6,a1
		bsr		get_bit_pat

		lea		_font6+$09*16,a0
		lea		tabchar6_0,a1
		bsr		get_bit_pat

		lea		_font6+$0a*16,a0
		lea		tabchar6_1,a1
		bsr		get_bit_pat

		movem.l		(sp)+,d1-d2/a0-a1
		rts

get_romfont2	movem.l		d1-d2/a0-a1,-(sp)
get_romfont2_l	movem.l		d1-d2,-(sp)
		moveq.l		#6,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a0
		movem.l		(sp)+,d1-d2
		lea		_font6,a1
		move.w		d1,d0
		lsl.w		#4,d0
		adda.w		d0,a1
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		addq.l		#1,d1
		subq.w		#1,d2
		bne		get_romfont2_l
		movem.l		(sp)+,d1-d2/a0-a1
		rts

get_bit_pat	move.l		(a1)+,(a0)+
  		move.l		(a1)+,(a0)+
  		move.l		(a1)+,(a0)+
  		rts


make_tate	move.l		a0,-(sp)

		move.l		#$20202020,d0
		lea		_font6+$01*16,a0
		move.l		d0,(a0)+
		move.l		d0,(a0)+
		move.l		d0,(a0)+

		move.l		(sp)+,a0
		rts


*===========================================================
*
*	void	H6_CURSET (void)
*
*	カーソルパターンの展開を行う
*
*===========================================================

_H6_CURSET:
		movem.l		d1/a0-a2,-(sp)

		lea		cursor6_0,a0
		lea		cursor6_3,a1
		move.w		#12-1,d0
h6_curset_l1	move.b		(a0)+,d1
		lsr.b		#2,d1
		move.b		d1,(a1)+
		dbra		d0,h6_curset_l1

		lea		cursor6_0,a0
		lea		cursor6_1,a1
		lea		cursor6_2,a2
		move.w		#12-1,d0
h6_curset_l2	moveq.l		#0,d1
		move.b		(a0)+,d1
		lsl.w		#2,d1
		move.w		d1,(a1)+
		lsl.w		#2,d1
		move.w		d1,(a2)+
		dbra		d0,h6_curset_l2

		movem.l		(sp)+,d1/a0-a2
		rts


*===========================================================
*
*	void	hcurxor6 (void)
*
*	カーソルの反転を行う
*
*===========================================================

hcurxor6_table:
		.dc.l		hcurxor6_0
		.dc.l		hcurxor6_1
		.dc.l		hcurxor6_2
		.dc.l		hcurxor6_3

hcurxor6_0:
		lea		cursor6_0,a0
		movea.l		cur_adr,a1
		bra		hcurxor6_byte

hcurxor6_1:
		lea		cursor6_1,a0
		movea.l		cur_adr,a1
		bra		hcurxor6_word

hcurxor6_2:
		lea		cursor6_2,a0
		movea.l		cur_adr,a1
		bra		hcurxor6_word

hcurxor6_3:
		lea		cursor6_3,a0
		movea.l		cur_adr,a1
		bra		hcurxor6_byte

hcurxor6_byte:
		move.b		(a0)+,d0
		eor.b		d0,(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128(a1)

		lea		-12(a0),a0
		adda.l		#$20000,a1

		move.b		(a0)+,d0
		eor.b		d0,(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128(a1)
		rts

hcurxor6_word:
		move.b		(a0)+,d0
		eor.b		d0,(a1)
		move.b		(a0)+,d0
		eor.b		d0,1(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128+1(a1)

		lea		-24(a0),a0
		adda.l		#$20000,a1

		move.b		(a0)+,d0
		eor.b		d0,(a1)
		move.b		(a0)+,d0
		eor.b		d0,1(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,01*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,02*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,03*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,04*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,05*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,06*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,07*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,08*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,09*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,10*128+1(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128+0(a1)
		move.b		(a0)+,d0
		eor.b		d0,11*128+1(a1)
		rts


*===========================================================
*
*	void	H6_PRINT3 (int x, int y,
*			    char *mes0, char *attr0,
*			    char *mes1, char *attr1,
*			    char *mes2, char *attr2
*			    int disp_zenspc);
*
*	int	x		X 座標
*	int	y		Y 座標
*	char	*mes0		表示する文字列へのポインタ（行番号用）
*	char	*attr0		表示する文字のアトリビュート
*	char	*mes1		表示する文字列へのポインタ（テキスト用）
*	char	*attr1		表示する文字のアトリビュート
*	char	*mes2		表示する文字列へのポインタ（改行マーク用）
*	char	*attr2		表示する文字のアトリビュート
*	int	disp_zensp	0 以外で全角スペースを表示する
*
*===========================================================

arg1		reg		15*4+2(sp)
arg2		reg		16*4+2(sp)
arg3		reg		17*4(sp)
arg4		reg		18*4(sp)
arg5		reg		19*4(sp)
arg6		reg		20*4(sp)
arg7		reg		21*4(sp)
arg8		reg		22*4(sp)
arg9		reg		23*4(sp)

_arg9		reg		25*4(sp)

_H6_PRINT3:
		movem.l		d1-d7/a0-a6,-(sp)

		clr.l		-(sp)
		.dc.w		_SUPER
		addq.l		#4,sp
		move.l		d0,ssp

		move.w		#$fcf8,d1		* clrar line 62
		moveq.l		#3,d2
		moveq.l		#%0011,d3
		moveq.l		#_TXRASCPY,d0
		trap		#15

		lea		$e00000+62*2048,a1	* a1.l = TEXT RAM ADDR

		moveq.l		#0,d0
		move.w		arg1,d0
		move.l		d0,d1
		add.l		d0,d0
		add.l		d1,d0
		add.l		d0,d0			* d0.l = x * 6
		lsr.l		#4,d0
		lsl.l		#1,d0
		adda.l		d0,a1

		lea		char_table(pc),a6	* a6.l = char table

		move.w		arg1,d7			* d7.w = x location
		movea.l		arg3,a3			* a3.l = str ptr
		movea.l		arg4,a4			* a4.l = attr ptr
		bsr		hpr

							* d7.w = x location
		movea.l		arg5,a3			* a3.l = str ptr
		movea.l		arg6,a4			* a4.l = attr ptr
		bsr		hpr

							* d7.w = x location
		movea.l		arg7,a3			* a3.l = str ptr
		movea.l		arg8,a4			* a4.l = attr ptr
		bsr		hpr

		move.w		arg2,d0
		move.w		d0,d1
		add.w		d1,d1
		add.w		d0,d1
		and.w		#$00ff,d1
		or.w		#$f800,d1
		moveq.l		#3,d2
		moveq.l		#%0011,d3
		moveq.l		#_TXRASCPY,d0
		trap		#15

		move.l		ssp,d0
		bmi		not_user
		move.l		d0,-(sp)
		.dc.w		_SUPER
		addq.l		#4,sp
not_user:

		movem.l		(sp)+,d1-d7/a0-a6
		rts


hpr:
		lea		CRTC_R23,a2

		move.b		(a4)+,d6
		move.b		d6,d0
		and.w		#%11,d0			* d0.w = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		CRTC_R21,r21_save
		move.w		d0,CRTC_R21		* set write color

		move.b		d6,d0
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr_b10:

		lea		hpr_jmp_table,a0
		move.w		d7,d0
		and.w		#7,d0
		lsl.w		#2,d0
		movea.l		(a0,d0.w),a0
		jmp		(a0)

hpr_end:
		move.w		r21_save,CRTC_R21	* reset write color
		move.w		#0,(a2)
		rts


*============================================================================

hpr0_ank_put	macro		n
		move.b		(a0)+,128*n(a1)
		endm

hpr0_kanji_put	macro		n
		move.w		(a0)+,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr0:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr0_ank_end		* space
		bgt		hpr0_kanji

		jsr		2(a5)

hpr0_ank:
		move.w		#%00000011_11111111,(a2)
		hpr0_ank_put	0
		hpr0_ank_put	1
		hpr0_ank_put	2
		hpr0_ank_put	3
		hpr0_ank_put	4
		hpr0_ank_put	5
		hpr0_ank_put	6
		hpr0_ank_put	7
		hpr0_ank_put	8
		hpr0_ank_put	9
		hpr0_ank_put	10
		hpr0_ank_put	11

hpr0_ank_end:
		addq.w		#1,d7

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr1

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr0_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr0_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr1

hpr0_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr0_ank

		move.w		#%00000000_00001111,(a2)
		hpr0_kanji_put	0
		hpr0_kanji_put	1
		hpr0_kanji_put	2
		hpr0_kanji_put	3
		hpr0_kanji_put	4
		hpr0_kanji_put	5
		hpr0_kanji_put	6
		hpr0_kanji_put	7
		hpr0_kanji_put	8
		hpr0_kanji_put	9
		hpr0_kanji_put	10
		hpr0_kanji_put	11

		addq.w		#2,d7

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr2

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr0_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr0_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr2


*============================================================================

hpr1_ank_put	macro		n
		move.b		(a0)+,d0
		rol.w		#2,d0
		move.w		d0,128*n(a1)
		endm

hpr1_kanji_put	macro		n
		move.w		(a0)+,d0
		ror.l		#6,d0
		move.w		#%11111100_00000000,(a2)
		move.w		d0,128*n(a1)
		swap		d0
		move.w		#%00111111_11111111,(a2)
		move.w		d0,128*n+2(a1)
		endm

*----------------------------------------------------------------------------

hpr1:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr1_ank_end		* space
		bgt		hpr1_kanji

		jsr		2(a5)

hpr1_ank:
		move.w		#%11111100_00001111,(a2)
		hpr1_ank_put	0
		hpr1_ank_put	1
		hpr1_ank_put	2
		hpr1_ank_put	3
		hpr1_ank_put	4
		hpr1_ank_put	5
		hpr1_ank_put	6
		hpr1_ank_put	7
		hpr1_ank_put	8
		hpr1_ank_put	9
		hpr1_ank_put	10
		hpr1_ank_put	11

hpr1_ank_end:
		addq.w		#1,d7

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr2

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr1_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr1_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr2

hpr1_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr1_ank

		hpr1_kanji_put	0
		hpr1_kanji_put	1
		hpr1_kanji_put	2
		hpr1_kanji_put	3
		hpr1_kanji_put	4
		hpr1_kanji_put	5
		hpr1_kanji_put	6
		hpr1_kanji_put	7
		hpr1_kanji_put	8
		hpr1_kanji_put	9
		hpr1_kanji_put	10
		hpr1_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr3

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr1_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr1_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr3


*============================================================================

hpr2_ank_put	macro		n
		move.b		(a0)+,d0
		ror.l		#4,d0
		move.w		#%11111111_11110000,(a2)
		move.w		d0,128*n(a1)
		swap		d0
		move.w		#%00111111_11111111,(a2)
		move.w		d0,128*n+2(a1)
		endm

hpr2_kanji_put	macro		n
		move.w		(a0)+,d0
		rol.l		#4,d0
		move.w		#%00000000_11111111,(a2)
		move.w		d0,128*n+2(a1)
		swap		d0
		move.w		#%11111111_11110000,(a2)
		move.w		d0,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr2:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr2_ank_end		* space
		bgt		hpr2_kanji

		jsr		2(a5)

hpr2_ank:
		hpr2_ank_put	0
		hpr2_ank_put	1
		hpr2_ank_put	2
		hpr2_ank_put	3
		hpr2_ank_put	4
		hpr2_ank_put	5
		hpr2_ank_put	6
		hpr2_ank_put	7
		hpr2_ank_put	8
		hpr2_ank_put	9
		hpr2_ank_put	10
		hpr2_ank_put	11

hpr2_ank_end:
		addq.w		#1,d7
		addq.l		#2,a1

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr3

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr2_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr2_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr3

hpr2_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr2_ank

		hpr2_kanji_put	0
		hpr2_kanji_put	1
		hpr2_kanji_put	2
		hpr2_kanji_put	3
		hpr2_kanji_put	4
		hpr2_kanji_put	5
		hpr2_kanji_put	6
		hpr2_kanji_put	7
		hpr2_kanji_put	8
		hpr2_kanji_put	9
		hpr2_kanji_put	10
		hpr2_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr4

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr2_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr2_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr4


*============================================================================

hpr3_ank_put	macro		n
		move.b		(a0)+,d0
		ror.b		#2,d0
		move.b		d0,128*n(a1)
		endm

hpr3_kanji_put	macro		n
		move.w		(a0)+,d0
		ror.w		#2,d0
		move.w		d0,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr3:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr3_ank_end		* space
		bgt		hpr3_kanji

		jsr		2(a5)

hpr3_ank:
		move.w		#%11000000_11111111,(a2)
		hpr3_ank_put	0
		hpr3_ank_put	1
		hpr3_ank_put	2
		hpr3_ank_put	3
		hpr3_ank_put	4
		hpr3_ank_put	5
		hpr3_ank_put	6
		hpr3_ank_put	7
		hpr3_ank_put	8
		hpr3_ank_put	9
		hpr3_ank_put	10
		hpr3_ank_put	11

hpr3_ank_end:
		addq.w		#1,d7

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr4

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr3_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr3_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr4

hpr3_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr3_ank

		move.w		#%11000000_00000011,(a2)
		hpr3_kanji_put	0
		hpr3_kanji_put	1
		hpr3_kanji_put	2
		hpr3_kanji_put	3
		hpr3_kanji_put	4
		hpr3_kanji_put	5
		hpr3_kanji_put	6
		hpr3_kanji_put	7
		hpr3_kanji_put	8
		hpr3_kanji_put	9
		hpr3_kanji_put	10
		hpr3_kanji_put	11

		addq.w		#2,d7

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr5

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr3_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr3_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr5


*============================================================================

hpr4_ank_put	macro		n
		move.b		(a0)+,128*n+1(a1)
		endm

hpr4_kanji_put	macro		n
		move.w		(a0)+,d0
		ror.l		#8,d0
		move.w		#%11111111_00000000,(a2)
		move.w		d0,128*n(a1)
		swap		d0
		move.w		#%00001111_11111111,(a2)
		move.w		d0,128*n+2(a1)
		endm

*----------------------------------------------------------------------------

hpr4:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr4_ank_end		* space
		bgt		hpr4_kanji

		jsr		2(a5)

hpr4_ank:
		move.w		#%11111111_00000011,(a2)
		hpr4_ank_put	0
		hpr4_ank_put	1
		hpr4_ank_put	2
		hpr4_ank_put	3
		hpr4_ank_put	4
		hpr4_ank_put	5
		hpr4_ank_put	6
		hpr4_ank_put	7
		hpr4_ank_put	8
		hpr4_ank_put	9
		hpr4_ank_put	10
		hpr4_ank_put	11

hpr4_ank_end:
		addq.w		#1,d7

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr5

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr4_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr4_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr5

hpr4_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr4_ank

		hpr4_kanji_put	0
		hpr4_kanji_put	1
		hpr4_kanji_put	2
		hpr4_kanji_put	3
		hpr4_kanji_put	4
		hpr4_kanji_put	5
		hpr4_kanji_put	6
		hpr4_kanji_put	7
		hpr4_kanji_put	8
		hpr4_kanji_put	9
		hpr4_kanji_put	10
		hpr4_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr6

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr4_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr4_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr6


*============================================================================

hpr5_ank_put	macro		n
		move.b		(a0)+,d0
		ror.l		#6,d0
		move.w		#%11111111_11111100,(a2)
		move.w		d0,128*n(a1)
		swap		d0
		move.w		#%00001111_11111111,(a2)
		move.w		d0,128*n+2(a1)
		endm

hpr5_kanji_put	macro		n
		move.w		(a0)+,d0
		rol.l		#2,d0
		move.w		#%00000000_00111111,(a2)
		move.w		d0,128*n+2(a1)
		swap		d0
		move.w		#%11111111_11111100,(a2)
		move.w		d0,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr5:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr5_ank_end		* space
		bgt		hpr5_kanji

		jsr		2(a5)

hpr5_ank:
		hpr5_ank_put	0
		hpr5_ank_put	1
		hpr5_ank_put	2
		hpr5_ank_put	3
		hpr5_ank_put	4
		hpr5_ank_put	5
		hpr5_ank_put	6
		hpr5_ank_put	7
		hpr5_ank_put	8
		hpr5_ank_put	9
		hpr5_ank_put	10
		hpr5_ank_put	11

hpr5_ank_end:
		addq.w		#1,d7
		addq.l		#2,a1

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr6

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr5_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr5_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr6

hpr5_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr5_ank

		hpr5_kanji_put	0
		hpr5_kanji_put	1
		hpr5_kanji_put	2
		hpr5_kanji_put	3
		hpr5_kanji_put	4
		hpr5_kanji_put	5
		hpr5_kanji_put	6
		hpr5_kanji_put	7
		hpr5_kanji_put	8
		hpr5_kanji_put	9
		hpr5_kanji_put	10
		hpr5_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr7

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr5_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr5_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr7


*============================================================================

hpr6_ank_put	macro		n
		move.b		(a0)+,d0
		rol.w		#4,d0
		move.w		d0,128*n(a1)
		endm

hpr6_kanji_put	macro		n
		move.w		(a0)+,d0
		ror.w		#4,d0
		move.w		d0,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr6:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr6_ank_end		* space
		bgt		hpr6_kanji

		jsr		2(a5)

hpr6_ank:
		move.w		#%11110000_00111111,(a2)
		hpr6_ank_put	0
		hpr6_ank_put	1
		hpr6_ank_put	2
		hpr6_ank_put	3
		hpr6_ank_put	4
		hpr6_ank_put	5
		hpr6_ank_put	6
		hpr6_ank_put	7
		hpr6_ank_put	8
		hpr6_ank_put	9
		hpr6_ank_put	10
		hpr6_ank_put	11

hpr6_ank_end:
		addq.w		#1,d7

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr7

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr6_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr6_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr7

hpr6_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr6_ank

		move.w		#%11110000_00000000,(a2)
		hpr6_kanji_put	0
		hpr6_kanji_put	1
		hpr6_kanji_put	2
		hpr6_kanji_put	3
		hpr6_kanji_put	4
		hpr6_kanji_put	5
		hpr6_kanji_put	6
		hpr6_kanji_put	7
		hpr6_kanji_put	8
		hpr6_kanji_put	9
		hpr6_kanji_put	10
		hpr6_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr0

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr6_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr6_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr0


*============================================================================

hpr7_ank_put	macro		n
		move.b		(a0)+,d0
		ror.b		#2,d0
		move.b		d0,128*n+1(a1)
		endm

hpr7_kanji_put	macro		n
		move.w		(a0)+,d0
		rol.l		#6,d0
		move.w		#%00000011_11111111,(a2)
		move.w		d0,128*n+2(a1)
		swap		d0
		move.w		#%11111111_11000000,(a2)
		move.w		d0,128*n(a1)
		endm

*----------------------------------------------------------------------------

hpr7:
		moveq.l		#0,d1
		move.b		(a3)+,d1
		beq		hpr_end
		tst.b		(a6,d1.w)
*		bmi		hpr7_ank_end		* space
		bgt		hpr7_kanji

		jsr		2(a5)

hpr7_ank:
		move.w		#%11111111_11000000,(a2)
		hpr7_ank_put	0
		hpr7_ank_put	1
		hpr7_ank_put	2
		hpr7_ank_put	3
		hpr7_ank_put	4
		hpr7_ank_put	5
		hpr7_ank_put	6
		hpr7_ank_put	7
		hpr7_ank_put	8
		hpr7_ank_put	9
		hpr7_ank_put	10
		hpr7_ank_put	11

hpr7_ank_end:
		addq.w		#1,d7
		addq.l		#2,a1

		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr0

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr7_b10
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr7_b10:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr0

hpr7_kanji:
		rol.w		#8,d1
		move.b		(a3)+,d1

		jsr		(a5)
		tst.w		d1
		beq		hpr7_ank

		hpr7_kanji_put	0
		hpr7_kanji_put	1
		hpr7_kanji_put	2
		hpr7_kanji_put	3
		hpr7_kanji_put	4
		hpr7_kanji_put	5
		hpr7_kanji_put	6
		hpr7_kanji_put	7
		hpr7_kanji_put	8
		hpr7_kanji_put	9
		hpr7_kanji_put	10
		hpr7_kanji_put	11

		addq.w		#2,d7
		addq.l		#2,a1

		addq.l		#1,a4
		move.b		(a4)+,d0
		cmp.b		d0,d6
		beq		hpr1

		move.b		d0,d6
		and.w		#%1100,d0
		lea		getfont_jmp_table,a5
		movea.l		(a5,d0.w),a5		* a5.l = get font

		move.l		#_font6,d5		* d5.l = font addr
		tst.b		d6
		bpl		hpr7_b11
		move.l		#_exfont6,d5		* d5.l = exfont addr
hpr7_b11:

		move.b		d6,d0
		and.w		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%11_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color
		bra		hpr1


*============================================================================

getfont_nor:
		bra.s		gf_nor_kanji

gf_nor_ank:
		lsl.w		#4,d1
		movea.l		d5,a0
		lea		(a0,d1.w),a0
		rts

gf_nor_kanji:
		cmp.w		#'　',d1
		bne		gf_nor_kanji_b1
		tst.l		_arg9
		beq		gf_nor_kanji_b1
		lea		zenspc6,a0
		bra		gf_nor_kanji_b2
gf_nor_kanji_b1:
		moveq.l		#6,d2
		moveq.l		#_FNTADR,d0
		trap		#15
		movea.l		d0,a0
gf_nor_kanji_b2:
		rts

*----------------------------------------------------------------------------

getfont_em:
		bra.s		gf_em_kanji

gf_em_ank:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		lsl.w		#4,d1
		movea.l		d5,a1
		lea		(a1,d1.w),a1
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		move.l		d4,(a0)
		subq.l		#8,a0
		movea.l		d3,a1			* restore a1
		rts

gf_em_kanji:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		cmp.w		#'　',d1
		bne		gf_em_kanji_b1
		tst.l		_arg9
		beq		gf_em_kanji_b1
		lea		zenspc6,a1
		bra		gf_em_kanji_b2
gf_em_kanji_b1:
		moveq.l		#6,d2
		moveq.l		#_FNTADR,d0
		trap		#15
		movea.l		d0,a1
gf_em_kanji_b2:
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		move.l		d4,(a0)+
		lea		-24(a0),a0
		movea.l		d3,a1			* restore a1
		rts

*----------------------------------------------------------------------------

getfont_rev:
		bra.s		gf_rev_kanji

gf_rev_ank:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		lsl.w		#4,d1
		movea.l		d5,a1
		lea		(a1,d1.w),a1
		move.l		#%11111100_11111100_11111100_11111100,d0
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)
		subq.l		#8,a0
		movea.l		d3,a1			* restore a1
		rts

gf_rev_kanji:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		cmp.w		#'　',d1
		bne		gf_rev_kanji_b1
		tst.l		_arg9
		beq		gf_rev_kanji_b1
		lea		zenspc6,a1
		bra		gf_rev_kanji_b2
gf_rev_kanji_b1:
		moveq.l		#6,d2
		moveq.l		#_FNTADR,d0
		trap		#15
		movea.l		d0,a1
gf_rev_kanji_b2:
		move.l		#%1111111111110000_1111111111110000,d0
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		eor.l		d0,d4
		move.l		d4,(a0)+
		lea		-24(a0),a0
		movea.l		d3,a1			* restore a1
		rts

*----------------------------------------------------------------------------

getfont_emrev:
		bra.s		gf_emrev_kanji

gf_emrev_ank:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		lsl.w		#4,d1
		movea.l		d5,a1
		lea		(a1,d1.w),a1
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		eor.l		#%11111100_11111100_11111100_11111100,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		eor.l		#%11111100_11111100_11111100_11111100,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111100_01111100_01111100_01111100,d0
		or.l		d0,d4
		eor.l		#%11111100_11111100_11111100_11111100,d4
		move.l		d4,(a0)
		subq.l		#8,a0
		movea.l		d3,a1			* restore a1
		rts

gf_emrev_kanji:
		lea		font_work,a0
		move.l		a1,d3			* save a1
		cmp.w		#'　',d1
		bne		gf_emrev_kanji_b1
		tst.l		_arg9
		beq		gf_emrev_kanji_b1
		lea		zenspc6,a1
		bra		gf_emrev_kanji_b2
gf_emrev_kanji_b1:
		moveq.l		#6,d2
		moveq.l		#_FNTADR,d0
		trap		#15
		movea.l		d0,a1
gf_emrev_kanji_b2:
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%0111111111110000_0111111111110000,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111111_11111111_01111111_11111111,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		move.l		(a1)+,d4
		move.l		d4,d0
		ror.l		#1,d0
		and.l		#%01111111_11111111_01111111_11111111,d0
		or.l		d0,d4
		eor.l		#%1111111111110000_1111111111110000,d4
		move.l		d4,(a0)+
		lea		-24(a0),a0
		movea.l		d3,a1			* restore a1
		rts

*============================================================================

		.text

hpr_jmp_table:
		.dc.l		hpr0
		.dc.l		hpr1
		.dc.l		hpr2
		.dc.l		hpr3
		.dc.l		hpr4
		.dc.l		hpr5
		.dc.l		hpr6
		.dc.l		hpr7


getfont_jmp_table:
		.dc.l		getfont_nor
		.dc.l		getfont_em
		.dc.l		getfont_rev
		.dc.l		getfont_emrev


crchar6:
		.dc.b		%000000_00		* 'CR' char pattern
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%011100_00
		.dc.b		%010100_00
		.dc.b		%010100_00
		.dc.b		%010111_00
		.dc.b		%010010_00
		.dc.b		%010100_00
		.dc.b		%011000_00
		.dc.b		%010000_00
		.dc.b		%000000_00
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000

		.even

tabchar6_0:
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%001100_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000

		.even

tabchar6_1:
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%001100_00
		.dc.b		%001010_00
		.dc.b		%001100_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%000000_00
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000
		.dc.b		%00000000

		.even

zenspc6:
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%00000000000000_00
		.dc.w		%01010101010000_00
		.dc.w		%00000000001000_00
		.dc.w		%01000000000000_00
		.dc.w		%00101010101000_00
		.dc.w		%00000000000000_00
		.dc.w		%0000000000000000
		.dc.w		%0000000000000000
		.dc.w		%0000000000000000
		.dc.w		%0000000000000000

		.data

_cur_pat_6x12:
cursor6_0:
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00
		.dc.b		%111111_00

cursor6_1:
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000
		.dc.w		%000000_111111_0000

cursor6_2:
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000
		.dc.w		%0000_111111_000000

cursor6_3:
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111
		.dc.b		%00_111111

		.bss

font_work	.ds.w		12
ssp		.ds.l		1
r21_save	.ds.w		1

_font6		.ds.b		4096
_exfont6	.ds.b		2048

