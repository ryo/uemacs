*-----------------------------------------------------------------------------
*	Hi-Speed PRINT routine written by SALT		1990/11/24
*
*	IRUKA さんの、H_PRINT を参考に emacs 用に作り直しました
*-----------------------------------------------------------------------------

		xref		__dump_flag
		xref		_issuper

*-----------------------------------------------------------------------------

		xdef		_LDIRL
		xdef		_H_INIT
		xdef		_H_MAKE_HALF
		xdef		_H_CURINT
		xdef		_H_DENSITY
		xdef		_H_CURON
		xdef		_H_CUROFF
		xdef		_H_LOCATE
		xdef		_H_ERA
		xdef		_H_ERA63
		xdef		_H_PRINT3
		xdef		_H_SCROLL

		xdef		_blink_count
		xdef		_font
		xdef		_font_h
		xdef		_exfont
		xdef		_exfont_h
		xdef		_cur_pat
		xdef		_cur_pat_h

*-----------------------------------------------------------------------------

CURSOR_X	equ		$000974
CURSOR_Y	equ		$000976

CRTC_R21	equ		$e8002a

_TXRASCPY	equ		$df

*===========================================================
*
*	RCS id
*
*===========================================================

		dc.b		'$Header: f:/SALT/emacs/RCS/hpr.s,v 1.2 1992/02/15 07:22:08 SALT Exp SALT $'
		even

*===========================================================
*
*	void	LDIRL(VIDEO *sou, VIDEO *dst, int size)
*
*	VIDEO	*sou		転送元
*	VIDEO	*dst		転送先
*	int	size		転送する大きさ
*
* note:
*	sou, dst は必ず偶数アドレス、size は４の倍数でなければいけない
*
*===========================================================

arg1		reg		20(sp)
arg2		reg		24(sp)
arg3		reg		28(sp)

_LDIRL:
		movem.l		d1/a0-a2,-(sp)

		movea.l		arg1,a0			* a0.l = sou
		movea.l		arg2,a1			* a1.l = dst
		move.l		arg3,d0			* d0.l = size
		moveq.l		#0,d1

		addq.l		#3,d0
		lsr.w		#2,d0
ldirl_l10	sub.w		#64,d0
		bhi		ldirl_b10
		moveq.l		#1,d1
		neg.w		d0
		add.w		d0,d0
		lea		ldirl_b10(pc),a2
		jmp		(a2,d0.w)

ldirl_b10	move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+

		tst.w		d1
		beq		ldirl_l10

		movem.l		(sp)+,d1/a0-a2
		rts


*===========================================================
*
*	void	H_INIT(void)
*
*	フォントの取り込み、６３行目をクリア
*
*===========================================================

_H_INIT:
		movem.l		a1,-(sp)

		move.l		#50,d0
		move.l		d0,_blink_count
		move.l		d0,blink_count
		move.w		#0,density

		moveq.l		#0,d0
		move.l		d0,-(sp)
		move.l		d0,-(sp)
		bsr		_H_LOCATE
		addq.l		#8,sp

		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		movea.l		d0,a1

		tst.l		__dump_flag
		bgt		hinit_b
		bsr		get_romfont
		bsr		get_romfont_h

hinit_b
		bsr		make_tate
		bsr		erase_line63

		move.l		a1,d0
		bmi		hinit_end
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hinit_end	movem.l		(sp)+,a1
		rts


get_romfont	movem.l		d1-d2/a0-a1,-(sp)
		move.l		#$00,d1
		move.w		#$7f,d2
		bsr		get_romfont2
		move.l		#$a0,d1
		move.w		#$40,d2
		bsr		get_romfont2

		lea		_font+$0d*16,a0
		lea		crchar,a1
		bsr		get_bit_pat

		lea		_font+$09*16,a0
		lea		tabchar0,a1
		bsr		get_bit_pat

		lea		_font+$0a*16,a0
		lea		tabchar1,a1
		bsr		get_bit_pat

		movem.l		(sp)+,d1-d2/a0-a1
		rts

get_romfont2	movem.l		d1-d2/a0-a1,-(sp)
get_romfont2_l	movem.l		d1-d2,-(sp)
		moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a0
		movem.l		(sp)+,d1-d2
		lea		_font,a1
		move.w		d1,d0
		lsl.w		#4,d0
		adda.w		d0,a1
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		addq.l		#1,d1
		subq.w		#1,d2
		bne		get_romfont2_l
		movem.l		(sp)+,d1-d2/a0-a1
		rts

get_romfont_h	movem.l		d1-d2/a0-a1,-(sp)
		move.l		#$00,d1
		move.w		#$7f,d2
		bsr		get_romfont2_h
		move.l		#$a0,d1
		move.w		#$40,d2
		bsr		get_romfont2_h

		lea		_font_h+$0d*8,a0
		lea		crchar_h,a1
		move.l		(a1)+,(a0)+
		move.l		(a1)+,(a0)+

		lea		_font_h+$09*8,a0
		lea		tabchar0_h,a1
		move.l		(a1)+,(a0)+
		move.l		(a1)+,(a0)+

		lea		_font_h+$0a*8,a0
		lea		tabchar1_h,a1
		move.l		(a1)+,(a0)+
		move.l		(a1)+,(a0)+

		movem.l		(sp)+,d1-d2/a0-a1
		rts

get_romfont2_h	movem.l		d1-d2/a0-a2,-(sp)
		lea		$ed0059,a2		* a2.l = FONT change flag
get_romfont2h_l	movem.l		d1-d2,-(sp)
		cmp.b		#$5c,d1			* '\'
		bne		get_romft2h_b10
		btst.b		#0,(a2)
		beq		get_romft2h_b30
		move.b		#$80,d1
		bra		get_romft2h_b30

get_romft2h_b10	cmp.b		#$7e,d1			* '~'
		bne		get_romft2h_b20
		btst.b		#1,(a2)
		beq		get_romft2h_b30
		move.b		#$81,d1
		bra		get_romft2h_b30

get_romft2h_b20	cmp.b		#$7c,d1			* '|'
		bne		get_romft2h_b30
		btst.b		#2,(a2)
		beq		get_romft2h_b30
		move.b		#$82,d1

get_romft2h_b30	add.w		#$f100,d1
		moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* _FNTADR
		movea.l		d0,a0
		movem.l		(sp)+,d1-d2
		lea		_font_h,a1
		move.w		d1,d0
		lsl.w		#3,d0
		adda.w		d0,a1
		move.l		(a0)+,(a1)+
		move.l		(a0)+,(a1)+
		addq.l		#1,d1
		subq.w		#1,d2
		bne		get_romfont2h_l
		movem.l		(sp)+,d1-d2/a0-a2
		rts

get_bit_pat	move.l		(a1)+,(a0)+
  		move.l		(a1)+,(a0)+
  		move.l		(a1)+,(a0)+
  		move.l		(a1)+,(a0)+
  		rts

make_tate	move.l		a0,-(sp)

		move.l		#$10101010,d0
		lea		_font+$01*16,a0
		move.l		d0,(a0)+
		move.l		d0,(a0)+
		move.l		d0,(a0)+
		move.l		d0,(a0)+
		lea		_font_h+$01*8,a0
		move.l		d0,(a0)+
		move.l		d0,(a0)+

		move.l		(sp)+,a0
		rts

erase_line63	movem.l		d1/a0,-(sp)

		movea.l		#$e00000+63*16*128,a0
		move.w		#128-1,d0
		moveq.l		#0,d1
erase_line63_l1	move.l		d1,(a0)+
		move.l		d1,(a0)+
		move.l		d1,(a0)+
		move.l		d1,(a0)+
		dbra		d0,erase_line63_l1

		adda.l		#$20000-16*128,a0
		move.w		#128-1,d0
		moveq.l		#0,d1
erase_line63_l2	move.l		d1,(a0)+
		move.l		d1,(a0)+
		move.l		d1,(a0)+
		move.l		d1,(a0)+
		dbra		d0,erase_line63_l2

		movem.l		(sp)+,d1/a0
		rts


*===========================================================
*
*	void	H_MAKE_HALF(int n, char *src, char *dst)
*
*	int	n		個数 (1-)
*	char	*src		オリジナルフォント
*	char	*dst		圧縮フォント
*
*	オリジナルフォントから圧縮フォントを作成する
*
*===========================================================

arg1		reg		5*4(sp)
arg2		reg		6*4(sp)
arg3		reg		7*4(sp)

_H_MAKE_HALF:
		movem.l		d1-d2/a0-a1,-(sp)

		move.l		arg1,d1			* d1.w = n
		movea.l		arg2,a0			* a0.l = src
		movea.l		arg3,a1			* a1.l = dst

		subq.w		#1,d1
hmake_half_l1	moveq.l		#8-1,d2
hmake_half_l2	move.b		(a0)+,d0
		or.b		(a0)+,d0
		move.b		d0,(a1)+
		dbra		d2,hmake_half_l2
		dbra		d1,hmake_half_l1

		movem.l		(sp)+,d1-d2/a0-a1
		rts


*===========================================================
*
*	void H_CURINT(int flag)
*
*	int	flag		カーソル表示処理フラグ
*				(0 ... システム / not 0 H_CUR)
*
*===========================================================

arg1		reg		2*4(sp)

_H_CURINT:

		move.l		d1,-(sp)

		move.l		old_timer_c,d1

		tst.l		arg1
		bne		hcurint_b

		tst.l		d1
		beq		hcurint_end

							* system on
		bsr		_H_CUROFF

		move.l		d1,-(sp)
		move.w		#$45,-(sp)
		dc.w		$ff25			* INTVCS
		addq.l		#6,sp
		clr.l		old_timer_c

		move.w		#17,-(sp)
		dc.w		$ff23			* CURSOR ON
		addq.l		#2,sp

		bra		hcurint_end

hcurint_b	tst.l		d1
		bne		hcurint_end

							* system off
		move.w		#18,-(sp)
		dc.w		$ff23			* CURSOR OFF
		addq.l		#2,sp

		move.w		#$45,-(sp)
		dc.w		$ff35			* INTVCG
		addq.l		#2,sp
		move.l		d0,old_timer_c

		pea		_hcurintjob(pc)
		move.w		#$45,-(sp)
		dc.w		$ff25			* INTVCS
		addq.l		#6,sp

		bsr		_H_CURON

hcurint_end	move.l		(sp)+,d1
		rts


*===========================================================
*
*	void H_DENSITY(int density)
*
*	int	density		表示密度 (0 / not 0)
*
*	H_ERA 、H_PRINT の表示密度の設定
*
*===========================================================

arg1		reg		1*4(sp)

_H_DENSITY:

		moveq.l		#0,d0
		move.l		d0,-(sp)
		move.l		d0,-(sp)
		bsr		_H_LOCATE
		addq.l		#8,sp

		move.l		arg1,d0
		beq		hdensity_b
		moveq.l		#1,d0
hdensity_b	move.w		d0,density

		rts


*===========================================================
*
*	int H_CURON(void)
*
*	カーソルの前の状態を返す
*
*===========================================================

_H_CURON:

		movem.l		d1/a0-a1,-(sp)

		moveq.l		#0,d1
		move.w		cur_flag,d1

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hcuron_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hcuron_b10	move.w		sr,-(sp)
		or.w		#$0700,sr

		move.w		#1,cur_flag
		tst.w		cur_disp
		bne		hcuron_b11
		move.w		#$ffff,cur_disp
		bsr		hcurxor
hcuron_b11	move.l		_blink_count,d0
		move.l		d0,blink_count

		move.w		(sp)+,sr

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hcuron_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hcuron_end	move.l		d1,d0

		movem.l		(sp)+,d1/a0-a1
		rts


*===========================================================
*
*	int H_CUROFF(void)
*
*	カーソルの前の状態を返す
*
*===========================================================

_H_CUROFF:

		movem.l		d1/a0-a1,-(sp)

		moveq.l		#0,d1
		move.w		cur_flag,d1

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hcuroff_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hcuroff_b10	move.w		sr,-(sp)
		or.w		#$0700,sr

		move.w		#0,cur_flag
		tst.w		cur_disp
		beq		hcuroff_b11
		bsr		hcurxor
		move.w		#0,cur_disp
hcuroff_b11	move.l		_blink_count,d0
		move.l		d0,blink_count

		move.w		(sp)+,sr

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hcuroff_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hcuroff_end	move.l		d1,d0

		movem.l		(sp)+,d1/a0-a1
		rts


*===========================================================
*
*	int H_LOCATE(int x, int y)
*
*	int	x		カーソル座標 (X)
*	int	y		カーソル座標 (Y)
*
*	座標が負の場合は座標を変更しない
*
*	X * 0x10000 + Y を返す
*
*===========================================================

arg1		reg		4*4(sp)
arg2		reg		5*4(sp)

_H_LOCATE:

		movem.l		d1/a0-a1,-(sp)

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hlocate_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hlocate_b10	move.l		arg1,d0
		bmi		hlocate_b20
		move.w		d0,cur_x

hlocate_b20	move.l		arg2,d0
		bmi		hlocate_b30
		move.w		d0,cur_y

hlocate_b30	moveq.l		#0,d1			* カーソルのアドレス計算
		move.l		d1,d0
		move.w		cur_y,d0
		swap		d0
		move.w		density,d1
		addq.w		#5,d1
		lsr.l		d1,d0
		move.w		cur_x,d1
		add.l		d1,d0
		add.l		#$e00000,d0

		cmp.l		cur_adr,d0
		beq		hlocate_b50

		move.w		sr,-(sp)
		or.w		#$0700,sr

		tst.w		cur_disp		* カーソル消去
		beq		hlocate_b40
		bsr		hcurxor
		move.w		#0,cur_disp
hlocate_b40	move.l		d0,cur_adr

		move.l		_blink_count,d0
		move.l		d0,blink_count

		move.w		(sp)+,sr

hlocate_b50	tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hlocate_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hlocate_end	move.w		cur_x,d0
		swap		d0
		move.w		cur_y,d0

		movem.l		(sp)+,d1/a0-a1
		rts


*===========================================================
*
*	void	_hcurintjob(void)
*
*	カーソルの反転を行う（割り込みルーチン)
*
*===========================================================

_hcurintjob:

		tst.w		cur_flag		* カーソルは on か？
		beq		hcurintjob_end

		tst.l		_blink_count
		beq		hcurintjob_end		* ブリンク無し
		subq.l		#1,blink_count
		bne		hcurintjob_end

		movem.l		d0/a0-a1,-(sp)
		move.l		_blink_count,d0
		move.l		d0,blink_count
		bsr		hcurxor
		movem.l		(sp)+,d0/a0-a1
		not.w		cur_disp

hcurintjob_end	move.l		old_timer_c,-(sp)
		rts


*===========================================================
*
*	void	hcurxor(void)
*
*	カーソルの反転を行う
*
*===========================================================

hcurxor:

		movea.l		cur_adr,a1
		tst.w		density
		bne		hcurxor_b

		lea		_cur_pat,a0

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
		move.b		(a0)+,d0
		eor.b		d0,12*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,13*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,14*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,15*128(a1)

		lea		-16(a0),a0
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
		move.b		(a0)+,d0
		eor.b		d0,12*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,13*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,14*128(a1)
		move.b		(a0)+,d0
		eor.b		d0,15*128(a1)

		rts

hcurxor_b	lea		_cur_pat_h,a0

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

		lea		-8(a0),a0
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

		rts


*===========================================================
*
*	void	H_ERA(int y)
*
*	int	y		Y 座標
*
*	カーソル行を消去
*
*===========================================================

arg1		reg		4*4+2(sp)

_H_ERA:
		movem.l		d1-d3,-(sp)

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hera_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hera_b10	move.w		density,d0
		beq		hera_b20
		moveq.l		#1,d0

hera_b20	move.w		arg1,d1			* d1.w = y
		asl.w		#2,d1
		asr.w		d0,d1
		and.w		#$ff,d1
		or.w		#$fc00,d1		* 63行目
		moveq.l		#4,d2
		asr.w		d0,d2
		moveq.l		#%0011,d3
		moveq.l		#_TXRASCPY,d0
		trap		#15

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hera_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hera_end	movem.l		(sp)+,d1-d3
		rts


*===========================================================
*
*	void	H_ERA63(void)
*
*	６３行目をクリア
*
*===========================================================

_H_ERA63:
		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hera_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hera63_b10
		bsr		erase_line63

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hera63_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hera63_end	rts


*===========================================================
*
*	void	H_PRINT3(int x, int y,
*			 char *mes0, char *attr0,
*			 char *mes1, char *attr1,
*			 char *mes2, char *attr2
*			 int disp_zenspc);
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

_arg9		reg		24*4(sp)

_H_PRINT3:
		movem.l		d1-d7/a0-a6,-(sp)

		move.l		#_font,font
		move.l		#_exfont,exfont

		move.w		density,d7		* d7.w = density
		beq		hprint3_b1
		moveq.l		#1,d7
		move.l		#_font_h,font
		move.l		#_exfont_h,exfont

hprint3_b1	tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hprint3_b10		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hprint3_b10	move.w		#$fcf8,d1		* clrar line 62
		moveq.l		#4,d2
		lsr.w		d7,d2
		moveq.l		#%0011,d3
		moveq.l		#_TXRASCPY,d0
		trap		#15

		move.w		CRTC_R21,d0
		move.w		d0,CRTC_R21_save

		move.w		arg1,d6

*		lea		_font,a2		* a2.l = font addr
		lea		char_table(pc),a3	* a3.l = char table
		lea		jump_table(pc),a4	* a4.l = jump table
		movea.l		#$e00000+62*2048,a5
		add.w		d6,a5			* a5.l = write addr

		moveq.l		#0,d5			* bound flag
		btst.l		d5,d6
		beq		hprint3_b11
		not.w		d5

hprint3_b11	movea.l		arg3,a0			* a0.l = arg3 (mes0)
		movea.l		arg4,a1			* a1.l = arg4 (attr0)
		bsr		__hprint

		movea.l		arg5,a0			* a0.l = arg5 (mes1)
		movea.l		arg6,a1			* a1.l = arg6 (attr1)
		bsr		__hprint

		movea.l		arg7,a0			* a0.l = arg7 (mes2)
		movea.l		arg8,a1			* a1.l = arg8 (attr2)
		bsr		__hprint

		move.w		arg2,d1
		lsl.w		#2,d1
		lsr.w		d7,d1
		and.w		#$00ff,d1
		or.w		#$f800,d1
		moveq.l		#4,d2
		lsr.w		d7,d2
		moveq.l		#%0011,d3
		moveq.l		#_TXRASCPY,d0
		trap		#15

*		move.w		d6,CURSOR_X
		move.w		CRTC_R21_save,d0
		move.w		d0,CRTC_R21

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hprint3_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hprint3_end	movem.l		(sp)+,d1-d7/a0-a6
		rts


__hprint	move.b		(a1)+,d4		* d4.b = attr
__hprint_l	moveq.l		#0,d0
		move.b		d4,d0
		and.b		#%11,d0			* d0.b = color
		lsl.w		#4,d0
		or.w		#%01_0000_0000,d0
		move.w		d0,CRTC_R21		* set write color

		movea.l		font,a2			* a2.l = font addr
		tst.b		d4
		bpl		__hprint_b10
		movea.l		exfont,a2		* a2.l = exfont addr

__hprint_b10	moveq.l		#0,d0
		move.b		d4,d0
		and.b		#%1100,d0
		add.w		d0,d0
		move.w		d7,d1
		lsl.w		#2,d1
		add.w		d1,d0
		movea.l		(a4,d0.w),a6
		jmp		(a6)

*-----------------------------------------------------------------------------

hpr_n_b		macro		num
if (num.eq.0)
		move.b		(a6)+,(a5)
else
		move.b		(a6)+,num*128(a5)
endif
		endm


hpr_n_w0	macro		num
if (num.eq.0)
		move.w		(a6)+,(a5)+
else
		move.w		(a6)+,num*128-2(a5)
endif
		endm


hpr_n_w1	macro		num
if (num.eq.0)
		move.b		(a6)+,(a5)+
		move.b		(a6)+,(a5)+
else
		move.b		(a6)+,num*128-2(a5)
		move.b		(a6)+,num*128-1(a5)
endif
		endm

*===========================================================
*
*	normal (density = 1)
*
*===========================================================

hprint_b100	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b150
		tst.b		(a3,d1.w)
		beq		hprint_b120		* ASCII
		bmi		hprint_b130		* SPACE

*漢字(偶数)
hprint_b110	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b111
		tst.l		_arg9
		beq		hprint_b111
		lea		zenspc,a6
		bra		hprint_b112

hprint_b111	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b121		* 8*16 dot font

hprint_b112	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b113
		hpr_n_w0	0			* a5.l = a5.l = + 2
		hpr_n_w0	1
		hpr_n_w0	2
		hpr_n_w0	3
		hpr_n_w0	4
		hpr_n_w0	5
		hpr_n_w0	6
		hpr_n_w0	7
		hpr_n_w0	8
		hpr_n_w0	9
		hpr_n_w0	10
		hpr_n_w0	11
		hpr_n_w0	12
		hpr_n_w0	13
		hpr_n_w0	14
		hpr_n_w0	15
		bra		hprint_b140

*漢字(奇数)
hprint_b113	hpr_n_w1	0			* a5.l = a5.l + 2
		hpr_n_w1	1
		hpr_n_w1	2
		hpr_n_w1	3
		hpr_n_w1	4
		hpr_n_w1	5
		hpr_n_w1	6
		hpr_n_w1	7
		hpr_n_w1	8
		hpr_n_w1	9
		hpr_n_w1	10
		hpr_n_w1	11
		hpr_n_w1	12
		hpr_n_w1	13
		hpr_n_w1	14
		hpr_n_w1	15
		bra		hprint_b140

*ASCII
hprint_b120	lsl.w		#4,d1
		lea		(a2,d1.w),a6
hprint_b121	hpr_n_b		0
		hpr_n_b		1
		hpr_n_b		2
		hpr_n_b		3
		hpr_n_b		4
		hpr_n_b		5
		hpr_n_b		6
		hpr_n_b		7
		hpr_n_b		8
		hpr_n_b		9
		hpr_n_b		10
		hpr_n_b		11
		hpr_n_b		12
		hpr_n_b		13
		hpr_n_b		14
		hpr_n_b		15

*SPACE
hprint_b130	addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b140	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b100
		move.b		d0,d4
		bra		__hprint_l

hprint_b150	rts


*-----------------------------------------------------------------------------

hpr_e_b		macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_e_w0	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_e_w1	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.b		d0,1(a5)
		lsr.w		#8,d0
		move.b		d0,(a5)
		addq.l		#2,a5
else
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.b		d0,num*128-1(a5)
		lsr.w		#8,d0
		move.b		d0,num*128-2(a5)
endif
		endm

*===========================================================
*
*	emphasis (density = 1)
*
*===========================================================

hprint_b200	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b250
		tst.b		(a3,d1.w)
		beq		hprint_b220		* ASCII
		bmi		hprint_b230		* SPACE

*漢字(偶数)
hprint_b210	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b211
		tst.l		_arg9
		beq		hprint_b211
		lea		zenspc,a6
		bra		hprint_b212

hprint_b211	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b221		* 8*16 dot font

hprint_b212	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b213
		hpr_e_w0	0			* a5.l = a5.l = + 2
		hpr_e_w0	1
		hpr_e_w0	2
		hpr_e_w0	3
		hpr_e_w0	4
		hpr_e_w0	5
		hpr_e_w0	6
		hpr_e_w0	7
		hpr_e_w0	8
		hpr_e_w0	9
		hpr_e_w0	10
		hpr_e_w0	11
		hpr_e_w0	12
		hpr_e_w0	13
		hpr_e_w0	14
		hpr_e_w0	15
		bra		hprint_b240

*漢字(奇数)
hprint_b213	hpr_e_w1	0			* a5.l = a5.l + 2
		hpr_e_w1	1
		hpr_e_w1	2
		hpr_e_w1	3
		hpr_e_w1	4
		hpr_e_w1	5
		hpr_e_w1	6
		hpr_e_w1	7
		hpr_e_w1	8
		hpr_e_w1	9
		hpr_e_w1	10
		hpr_e_w1	11
		hpr_e_w1	12
		hpr_e_w1	13
		hpr_e_w1	14
		hpr_e_w1	15
		bra		hprint_b240

*ASCII
hprint_b220	lsl.w		#4,d1
		lea		(a2,d1.w),a6
hprint_b221	hpr_e_b		0
		hpr_e_b		1
		hpr_e_b		2
		hpr_e_b		3
		hpr_e_b		4
		hpr_e_b		5
		hpr_e_b		6
		hpr_e_b		7
		hpr_e_b		8
		hpr_e_b		9
		hpr_e_b		10
		hpr_e_b		11
		hpr_e_b		12
		hpr_e_b		13
		hpr_e_b		14
		hpr_e_b		15

*SPACE
hprint_b230	addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b240	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b200
		move.b		d0,d4
		bra		__hprint_l

hprint_b250	rts


*-----------------------------------------------------------------------------

hpr_r_b		macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_r_w0	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		not.w		d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		not.w		d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_r_w1	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,(a5)+
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,(a5)+
else
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,num*128-2(a5)
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,num*128-1(a5)
endif
		endm

*===========================================================
*
*	reverse (density = 1)
*
*===========================================================

hprint_b300	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b350
		tst.b		(a3,d1.w)
		ble		hprint_b320		* ASCII

*漢字(偶数)
hprint_b310	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b311
		tst.l		_arg9
		beq		hprint_b311
		lea		zenspc,a6
		bra		hprint_b312

hprint_b311	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b321		* 8*16 dot font

hprint_b312	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b313
		hpr_r_w0	0			* a5.l = a5.l = + 2
		hpr_r_w0	1
		hpr_r_w0	2
		hpr_r_w0	3
		hpr_r_w0	4
		hpr_r_w0	5
		hpr_r_w0	6
		hpr_r_w0	7
		hpr_r_w0	8
		hpr_r_w0	9
		hpr_r_w0	10
		hpr_r_w0	11
		hpr_r_w0	12
		hpr_r_w0	13
		hpr_r_w0	14
		hpr_r_w0	15
		bra		hprint_b340

*漢字(奇数)
hprint_b313	hpr_r_w1	0			* a5.l = a5.l + 2
		hpr_r_w1	1
		hpr_r_w1	2
		hpr_r_w1	3
		hpr_r_w1	4
		hpr_r_w1	5
		hpr_r_w1	6
		hpr_r_w1	7
		hpr_r_w1	8
		hpr_r_w1	9
		hpr_r_w1	10
		hpr_r_w1	11
		hpr_r_w1	12
		hpr_r_w1	13
		hpr_r_w1	14
		hpr_r_w1	15
		bra		hprint_b340

*ASCII
hprint_b320	lsl.w		#4,d1
		lea		(a2,d1.w),a6
hprint_b321	hpr_r_b		0
		hpr_r_b		1
		hpr_r_b		2
		hpr_r_b		3
		hpr_r_b		4
		hpr_r_b		5
		hpr_r_b		6
		hpr_r_b		7
		hpr_r_b		8
		hpr_r_b		9
		hpr_r_b		10
		hpr_r_b		11
		hpr_r_b		12
		hpr_r_b		13
		hpr_r_b		14
		hpr_r_b		15

		addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b340	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b300
		move.b		d0,d4
		bra		__hprint_l

hprint_b350	rts


*-----------------------------------------------------------------------------

hpr_er_b	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_er_w0	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_er_w1	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.b		d0,1(a5)
		lsr.w		#8,d0
		move.b		d0,(a5)
		addq.l		#2,a5
else
		move.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.b		d0,num*128-1(a5)
		lsr.w		#8,d0
		move.b		d0,num*128-2(a5)
endif
		endm

*===========================================================
*
*	emphasis reverse (density = 1)
*
*===========================================================

hprint_b400	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b450
		tst.b		(a3,d1.w)
		ble		hprint_b420		* ASCII

*漢字(偶数)
hprint_b410	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b411
		tst.l		_arg9
		beq		hprint_b411
		lea		zenspc,a6
		bra		hprint_b412

hprint_b411	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b421		* 8*16 dot font

hprint_b412	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b413
		hpr_er_w0	0			* a5.l = a5.l = + 2
		hpr_er_w0	1
		hpr_er_w0	2
		hpr_er_w0	3
		hpr_er_w0	4
		hpr_er_w0	5
		hpr_er_w0	6
		hpr_er_w0	7
		hpr_er_w0	8
		hpr_er_w0	9
		hpr_er_w0	10
		hpr_er_w0	11
		hpr_er_w0	12
		hpr_er_w0	13
		hpr_er_w0	14
		hpr_er_w0	15
		bra		hprint_b440

*漢字(奇数)
hprint_b413	hpr_er_w1	0			* a5.l = a5.l + 2
		hpr_er_w1	1
		hpr_er_w1	2
		hpr_er_w1	3
		hpr_er_w1	4
		hpr_er_w1	5
		hpr_er_w1	6
		hpr_er_w1	7
		hpr_er_w1	8
		hpr_er_w1	9
		hpr_er_w1	10
		hpr_er_w1	11
		hpr_er_w1	12
		hpr_er_w1	13
		hpr_er_w1	14
		hpr_er_w1	15
		bra		hprint_b440

*ASCII
hprint_b420	lsl.w		#4,d1
		lea		(a2,d1.w),a6
hprint_b421	hpr_er_b	0
		hpr_er_b	1
		hpr_er_b	2
		hpr_er_b	3
		hpr_er_b	4
		hpr_er_b	5
		hpr_er_b	6
		hpr_er_b	7
		hpr_er_b	8
		hpr_er_b	9
		hpr_er_b	10
		hpr_er_b	11
		hpr_er_b	12
		hpr_er_b	13
		hpr_er_b	14
		hpr_er_b	15

		addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b440	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b400
		move.b		d0,d4
		bra		__hprint_l

hprint_b450	rts


*-----------------------------------------------------------------------------

hpr_n_bh	macro		num
if (num.eq.0)
		move.b		(a6)+,(a5)
else
		move.b		(a6)+,num*128(a5)
endif
		endm


hpr_n_bhh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_n_w0h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_n_w1h	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		(a6)+,d1
		or.b		(a6)+,d0
		or.b		(a6)+,d1
		move.b		d0,(a5)+
		move.b		d1,(a5)+
else
		move.b		(a6)+,d0
		move.b		(a6)+,d1
		or.b		(a6)+,d0
		or.b		(a6)+,d1
		move.b		d0,num*128-2(a5)
		move.b		d1,num*128-1(a5)
endif
		endm

*===========================================================
*
*	normal (density = 2)
*
*===========================================================

hprint_b100h	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b150h
		tst.b		(a3,d1.w)
		beq		hprint_b120h		* ASCII
		bmi		hprint_b130h		* SPACE

*漢字(偶数)
hprint_b110h	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b111h
		tst.l		_arg9
		beq		hprint_b111h
		lea		zenspc_h,a6
		bra		hprint_b112h

hprint_b111h	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b114h		* 8*16 dot font

hprint_b112h	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b113h
		hpr_n_w0h	0			* a5.l = a5.l = + 2
		hpr_n_w0h	1
		hpr_n_w0h	2
		hpr_n_w0h	3
		hpr_n_w0h	4
		hpr_n_w0h	5
		hpr_n_w0h	6
		hpr_n_w0h	7
		bra		hprint_b140h

*漢字(奇数)
hprint_b113h	hpr_n_w1h	0			* a5.l = a5.l + 2
		hpr_n_w1h	1
		hpr_n_w1h	2
		hpr_n_w1h	3
		hpr_n_w1h	4
		hpr_n_w1h	5
		hpr_n_w1h	6
		hpr_n_w1h	7
		bra		hprint_b140h

*漢字(半角)
hprint_b114h	hpr_n_bhh	0
		hpr_n_bhh	1
		hpr_n_bhh	2
		hpr_n_bhh	3
		hpr_n_bhh	4
		hpr_n_bhh	5
		hpr_n_bhh	6
		hpr_n_bhh	7
		bra		hprint_b140h

*ASCII
hprint_b120h	lsl.w		#3,d1
		lea		(a2,d1.w),a6
		hpr_n_bh	0
		hpr_n_bh	1
		hpr_n_bh	2
		hpr_n_bh	3
		hpr_n_bh	4
		hpr_n_bh	5
		hpr_n_bh	6
		hpr_n_bh	7

*SPACE
hprint_b130h	addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b140h	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b100h
		move.b		d0,d4
		bra		__hprint_l

hprint_b150h	rts


*-----------------------------------------------------------------------------

hpr_e_bh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_e_bhh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_e_w0h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_e_w1h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.b		d0,1(a5)
		lsr.w		#8,d0
		move.b		d0,(a5)
		addq.l		#2,a5
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		move.b		d0,num*128-1(a5)
		lsr.w		#8,d0
		move.b		d0,num*128-2(a5)
endif
		endm

*===========================================================
*
*	emphasis (density = 2)
*
*===========================================================

hprint_b200h	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b250h
		tst.b		(a3,d1.w)
		beq		hprint_b220h		* ASCII
		bmi		hprint_b230h		* SPACE

*漢字(偶数)
hprint_b210h	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b211h
		tst.l		_arg9
		beq		hprint_b211h
		lea		zenspc_h,a6
		bra		hprint_b212h

hprint_b211h	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b214h		* 8*16 dot font

hprint_b212h	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b213h
		hpr_e_w0h	0			* a5.l = a5.l = + 2
		hpr_e_w0h	1
		hpr_e_w0h	2
		hpr_e_w0h	3
		hpr_e_w0h	4
		hpr_e_w0h	5
		hpr_e_w0h	6
		hpr_e_w0h	7
		bra		hprint_b240h

*漢字(奇数)
hprint_b213h	hpr_e_w1h	0			* a5.l = a5.l + 2
		hpr_e_w1h	1
		hpr_e_w1h	2
		hpr_e_w1h	3
		hpr_e_w1h	4
		hpr_e_w1h	5
		hpr_e_w1h	6
		hpr_e_w1h	7
		bra		hprint_b240h

*漢字(半角)
hprint_b214h
		hpr_e_bhh	0
		hpr_e_bhh	1
		hpr_e_bhh	2
		hpr_e_bhh	3
		hpr_e_bhh	4
		hpr_e_bhh	5
		hpr_e_bhh	6
		hpr_e_bhh	7
		bra		hprint_b240h

*ASCII
hprint_b220h	lsl.w		#3,d1
		lea		(a2,d1.w),a6
		hpr_e_bh	0
		hpr_e_bh	1
		hpr_e_bh	2
		hpr_e_bh	3
		hpr_e_bh	4
		hpr_e_bh	5
		hpr_e_bh	6
		hpr_e_bh	7

*SPACE
hprint_b230h	addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b240h	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b200h
		move.b		d0,d4
		bra		__hprint_l

hprint_b250h	rts


*-----------------------------------------------------------------------------

hpr_r_bh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_r_bhh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_r_w0h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		not.w		d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		not.w		d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_r_w1h	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		(a6)+,d1
		or.b		(a6)+,d0
		or.b		(a6)+,d1
		not.b		d0
		move.b		d0,(a5)+
		not.b		d1
		move.b		d1,(a5)+
else
		move.b		(a6)+,d0
		move.b		(a6)+,d1
		or.b		(a6)+,d0
		or.b		(a6)+,d1
		not.b		d0
		move.b		d0,num*128-2(a5)
		not.b		d1
		move.b		d1,num*128-1(a5)
endif
		endm

*===========================================================
*
*	reverse (density = 2)
*
*===========================================================

hprint_b300h	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b350h
		tst.b		(a3,d1.w)
		ble		hprint_b320h		* ASCII

*漢字(偶数)
hprint_b310h	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b311h
		tst.l		_arg9
		beq		hprint_b311h
		lea		zenspc_h,a6
		bra		hprint_b312h

hprint_b311h	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b314h		* 8*16 dot font

hprint_b312h	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b313h
		hpr_r_w0h	0			* a5.l = a5.l = + 2
		hpr_r_w0h	1
		hpr_r_w0h	2
		hpr_r_w0h	3
		hpr_r_w0h	4
		hpr_r_w0h	5
		hpr_r_w0h	6
		hpr_r_w0h	7
		bra		hprint_b340h

*漢字(奇数)
hprint_b313h	hpr_r_w1h	0			* a5.l = a5.l + 2
		hpr_r_w1h	1
		hpr_r_w1h	2
		hpr_r_w1h	3
		hpr_r_w1h	4
		hpr_r_w1h	5
		hpr_r_w1h	6
		hpr_r_w1h	7
		bra		hprint_b340h

*漢字(半角)
hprint_b314h	hpr_r_bhh	0
		hpr_r_bhh	1
		hpr_r_bhh	2
		hpr_r_bhh	3
		hpr_r_bhh	4
		hpr_r_bhh	5
		hpr_r_bhh	6
		hpr_r_bhh	7
		bra		hprint_b340h

*ASCII
hprint_b320h	lsl.w		#3,d1
		lea		(a2,d1.w),a6
		hpr_r_bh	0
		hpr_r_bh	1
		hpr_r_bh	2
		hpr_r_bh	3
		hpr_r_bh	4
		hpr_r_bh	5
		hpr_r_bh	6
		hpr_r_bh	7

		addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b340h	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b300h
		move.b		d0,d4
		bra		__hprint_l

hprint_b350h	rts


*-----------------------------------------------------------------------------

hpr_er_bh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_er_bhh	macro		num
if (num.eq.0)
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,(a5)
else
		move.b		(a6)+,d0
		or.b		(a6)+,d0
		move.b		d0,d1
		lsr.b		#1,d1
		or.b		d1,d0
		not.b		d0
		move.b		d0,num*128(a5)
endif
		endm


hpr_er_w0h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.w		d0,(a5)+
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.w		d0,num*128-2(a5)
endif
		endm


hpr_er_w1h	macro		num
if (num.eq.0)
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.b		d0,1(a5)
		lsr.w		#8,d0
		move.b		d0,(a5)
		addq.l		#2,a5
else
		move.w		(a6)+,d0
		or.w		(a6)+,d0
		move.w		d0,d1
		lsr.w		#1,d1
		or.w		d1,d0
		not.w		d0
		move.b		d0,num*128-1(a5)
		lsr.w		#8,d0
		move.b		d0,num*128-2(a5)
endif
		endm

*===========================================================
*
*	emphasis reverse (density = 2)
*
*===========================================================

hprint_b400h	moveq.l		#0,d1
		move.b		(a0)+,d1		* d1.w = char code
		beq		hprint_b450h
		tst.b		(a3,d1.w)
		ble		hprint_b420h		* ASCII

*漢字(偶数)
hprint_b410h	addq.l		#1,a1			* skip attr
		lsl.w		#8,d1
		move.b		(a0)+,d1		* d1.w = char code
		cmp.w		#'　',d1
		bne		hprint_b411h
		tst.l		_arg9
		beq		hprint_b411h
		lea		zenspc_h,a6
		bra		hprint_b412h

hprint_b411h	moveq.l		#8,d2
		moveq.l		#$16,d0
		trap		#15			* FNTADR
		movea.l		d0,a6			* a6.l = font adr
		tst.w		d1
		beq		hprint_b414h		* 8*16 dot font

hprint_b412h	addq.w		#2,d6			* inc2 cursor x
		tst.w		d5
		bne		hprint_b413h
		hpr_er_w0h	0			* a5.l = a5.l = + 2
		hpr_er_w0h	1
		hpr_er_w0h	2
		hpr_er_w0h	3
		hpr_er_w0h	4
		hpr_er_w0h	5
		hpr_er_w0h	6
		hpr_er_w0h	7
		bra		hprint_b440h

*漢字(奇数)
hprint_b413h	hpr_er_w1h	0			* a5.l = a5.l + 2
		hpr_er_w1h	1
		hpr_er_w1h	2
		hpr_er_w1h	3
		hpr_er_w1h	4
		hpr_er_w1h	5
		hpr_er_w1h	6
		hpr_er_w1h	7
		bra		hprint_b440h

*漢字(半角)
hprint_b414h	hpr_er_bhh	0
		hpr_er_bhh	1
		hpr_er_bhh	2
		hpr_er_bhh	3
		hpr_er_bhh	4
		hpr_er_bhh	5
		hpr_er_bhh	6
		hpr_er_bhh	7
		bra		hprint_b440h

*ASCII
hprint_b420h	lsl.w		#3,d1
		lea		(a2,d1.w),a6
		hpr_er_bh	0
		hpr_er_bh	1
		hpr_er_bh	2
		hpr_er_bh	3
		hpr_er_bh	4
		hpr_er_bh	5
		hpr_er_bh	6
		hpr_er_bh	7

		addq.w		#1,d6			* inc cursor x
		addq.l		#1,a5
		not.w		d5

hprint_b440h	move.b		(a1)+,d0		* d0.b = attr
		cmp.b		d0,d4
		beq		hprint_b400h
		move.b		d0,d4
		bra		__hprint_l

hprint_b450h	rts


*-----------------------------------------------------------------------------


*===========================================================
*
*	void	H_SCROLL(int x, int y, int width, int height, int size)
*
*	int	x		０～
*	int	y		０～
*	int	width		０～
*	int	height		０～
*	int	size		±ｎ（ｎ≧０）
*
*	(x, y)-(x + width, y + height) を縦方向に size だけスクロールします
*
*	note:
*		width は必ず 4 以上でなければいけない
*
*===========================================================

arg1		reg		10*4(sp)
arg2		reg		11*4(sp)
arg3		reg		12*4(sp)
arg4		reg		13*4(sp)
arg5		reg		14*4(sp)

_H_SCROLL:
		movem.l		d1-d4/a0-a4,-(sp)

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hscroll_b100		* yes
		clr.l		a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER
		move.l		d0,ssp

hscroll_b100	move.w		CRTC_R21,d0
		move.w		d0,CRTC_R21_save
		move.w		#%00_0011_0011,CRTC_R21

		move.l		arg5,d0
		tst.w		d0			* scroll down ?
		bmi		hscroll_b200		* yes

		lea		$e00000,a0
		move.l		arg1,d1			* d1.w = x
		move.l		arg2,d2			* d2.w = y
		moveq.l		#5,d3
		swap		d2
		clr.w		d2
		lsr.l		d3,d2
		adda.w		d1,a0
		adda.l		d2,a0			* a0.l = dest (page 0)
		swap		d0
		clr.w		d0
		lsr.l		d3,d0
		lea		(a0,d0.l),a2		* a2.l = sou  (page 0)
		move.l		#$20000,d0
		lea		(a0,d0.l),a1		* a1.l = dest (page 1)
		lea		(a2,d0.l),a3		* a3.l = sou  (page 1)

		move.l		arg1,d1			* d1.w = x0
		move.l		arg3,d2
		move.w		#128-1,d0
		sub.w		d2,d0			* d0.w = 128 - width -1
		add.w		d1,d2			* d2.w = x1
		move.w		d1,d3
		move.w		d2,d4
		and.w		#$fffc,d3		* d3.w = x0 / 4 * 4
		and.w		#$fffc,d4		* d4.w = x1 / 4 * 4
		sub.w		d4,d3
		addq.w		#4,d3
		lea		hscroll_b110(pc),a4
		adda.w		d3,a4			* a4.l = jump ptr

		and.w		#3,d1
		neg.w		d1
		add.w		#3,d1			* d1.w = 3 - x0 % 4
		and.w		#3,d2			* d2.w = x1 % 4

		move.l		arg4,d3			* d3.w = height
		move.l		arg5,d4			* d4.w = size
		sub.w		d4,d3			* d3.w = roll size
		bmi		hscroll_b300
		addq.w		#1,d3
		lsl.w		#4,d3			* d3.w = d3.w * 16
		subq.w		#1,d3

hscroll_l100	move.w		d1,d4
hscroll_l101	move.b		(a2)+,(a0)+
		move.b		(a3)+,(a1)+
		dbra		d4,hscroll_l101

		jmp		(a4)

		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+

hscroll_b110	move.w		d2,d4
hscroll_l110	move.b		(a2)+,(a0)+
		move.b		(a3)+,(a1)+
		dbra		d4,hscroll_l110

		adda.w		d0,a0
		adda.w		d0,a1
		adda.w		d0,a2
		adda.w		d0,a3
		dbra		d3,hscroll_l100
		bra		hscroll_b300


hscroll_b200	lea		$e00780,a0
		move.l		arg1,d1			* d1.w = x
		move.l		arg2,d2
		move.l		arg4,d3
		add.w		d3,d2			* d2.w = y + height

		moveq.l		#5,d3
		swap		d2
		clr.w		d2
		lsr.l		d3,d2
		adda.w		d1,a0
		adda.l		d2,a0			* a0.l = dest (page 0)
		swap		d0
		clr.w		d0
		lsr.l		d3,d0
		lea		(a0,d0.l),a2		* a2.l = sou  (page 0)
		move.l		#$20000,d0
		lea		(a0,d0.l),a1		* a1.l = dest (page 1)
		lea		(a2,d0.l),a3		* a3.l = sou  (page 1)

		move.l		arg1,d1			* d1.w = x0
		move.l		arg3,d2
		move.w		#-129,d0
		sub.w		d2,d0			* d0.w = -128 - width - 1
		add.w		d1,d2			* d2.w = x1
		move.w		d1,d3
		move.w		d2,d4
		and.w		#$fffc,d3		* d3.w = x0 / 4 * 4
		and.w		#$fffc,d4		* d4.w = x1 / 4 * 4
		sub.w		d4,d3
		addq.w		#4,d3
		lea		hscroll_b210(pc),a4
		adda.w		d3,a4			* a4.l = jump ptr

		and.w		#3,d1
		neg.w		d1
		add.w		#3,d1			* d1.w = 3 - x0 % 4
		and.w		#3,d2			* d2.w = x1 % 4

		move.l		arg4,d3			* d3.w = height
		move.l		arg5,d4			* d4.w = size
		add.w		d4,d3			* d3.w = roll size
		bmi		hscroll_b300
		addq.w		#1,d3
		lsl.w		#4,d3			* d3.w = d3.w * 16
		subq.w		#1,d3

hscroll_l200	move.w		d1,d4
hscroll_l201	move.b		(a2)+,(a0)+
		move.b		(a3)+,(a1)+
		dbra		d4,hscroll_l201

		jmp		(a4)

		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+
		move.l		(a2)+,(a0)+
		move.l		(a3)+,(a1)+

hscroll_b210	move.w		d2,d4
hscroll_l210	move.b		(a2)+,(a0)+
		move.b		(a3)+,(a1)+
		dbra		d4,hscroll_l210

		adda.w		d0,a0
		adda.w		d0,a1
		adda.w		d0,a2
		adda.w		d0,a3
		dbra		d3,hscroll_l200

hscroll_b300	move.w		CRTC_R21_save,d0
		move.w		d0,CRTC_R21

		tst.l		_issuper		* 現在スーパーバイザーモードか？
		bne		hscroll_end		* yes
		movea.l		ssp,a1
		moveq.l		#$81,d0
		trap		#15			* B_SUPER

hscroll_end	movem.l		(sp)+,d1-d4/a0-a4
		rts


*-----------------------------------------------------------------------------

jump_table	dc.l		hprint_b100		* normal
		dc.l		hprint_b100h		* normal (half)
		dc.l		hprint_b200		* emphasis
		dc.l		hprint_b200h		* emphasis (half)
		dc.l		hprint_b300		* reverse
		dc.l		hprint_b300h		* reverse (half)
		dc.l		hprint_b400		* emphasis reverse
		dc.l		hprint_b400h		* emphasis reverse (half)

*-----------------------------------------------------------------------------

		even

char_table	dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$00
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$10
		dc.b		$ff,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$20
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$30
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$40
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$50
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$60
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$70
		dc.b		$00,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01	* +$80
		dc.b		$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01	* +$90
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$a0
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$b0
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$c0
		dc.b		$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$d0
		dc.b		$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01,$01	* +$e0
		dc.b		$01,$01,$01,$01,$01,$01,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00	* +$f0

		even

crchar		dc.b		%00000000		* 'CR' char pattern
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00111100
		dc.b		%00100100
		dc.b		%00100100
		dc.b		%00100111
		dc.b		%00100010
		dc.b		%00100100
		dc.b		%00101000
		dc.b		%00110000
		dc.b		%00100000
		dc.b		%00000000

		even

crchar_h	dc.b		%00000000		* 'CR' char pattern
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00111000
		dc.b		%00101000
		dc.b		%00100110
		dc.b		%00101000
		dc.b		%00110000

		even

tabchar0	dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00011000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000

		even

tabchar0_h	dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00011000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000

		even

tabchar1	dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00011000
		dc.b		%00010100
		dc.b		%00011000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000

		even

tabchar1_h	dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00011000
		dc.b		%00011100
		dc.b		%00000000
		dc.b		%00000000
		dc.b		%00000000

		even

zenspc		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0101010101010100
		dc.w		%0000000000000010
		dc.w		%0100000000000000
		dc.w		%0010101010101010
		dc.w		%0000000000000000

zenspc_h	dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0000000000000000
		dc.w		%0010101010101010
		dc.w		%0010101010101010
		dc.w		%0100000000000010
		dc.w		%0100000000000010
		dc.w		%0101010101010100
		dc.w		%0101010101010100

_cur_pat:
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111

_cur_pat_h:
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111
		dc.b		%11111111

*-----------------------------------------------------------------------------

		bss

ssp		ds.l		1
CRTC_R21_save	ds.w		1

font		ds.l		1
exfont		ds.l		1

old_timer_c	ds.l		1

cur_x		ds.w		1
cur_y		ds.w		1
cur_flag	ds.w		1
cur_disp	ds.w		1
cur_adr		ds.l		1
blink_count	ds.l		1
_blink_count	ds.l		1

density		ds.w		1

_font		ds.b		4096
_font_h		ds.b		2048
_exfont		ds.b		2048
_exfont_h	ds.b		1024

*-----------------------------------------------------------------------------

		end

