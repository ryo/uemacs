*=====================================================================
*
*	B_PUTC, B_PRINT handle routine for micorEMACS J1.40
*				      icam/Hommy + PEACE/SALT version
*
*			1990/10/25 written by SALT
*
*=====================================================================


*---------------------------------------------------------------------

		xref		_issuper

*---------------------------------------------------------------------

		xdef		_disable_print
		xdef		_enable_print

		xdef		_disable_int
		xdef		_enable_int
		xdef		_kill_int

		xdef		_intercept
		xdef		_intercept_end

		xdef		_intercept_flag

*---------------------------------------------------------------------

FLUSH_COUNT	equ		8

*---------------------------------------------------------------------

jsrm		macro		adr
		local		ret_pos
		pea		ret_pos(pc)
		move.l		adr,-(sp)
		rts
ret_pos
		endm

*---------------------------------------------------------------------


*
*	RCS id
*

	dc.b	'$Header: f:/SALT/emacs/RCS/intercept.s,v 1.1 1991/08/11 09:02:10 SALT Exp $'
	even


*
*
*	void	disable_print(void)
*
*

_disable_print:
		pea		null_putc(pc)
		move.w		#$120,-(sp)		* B_PUTC
		dc.w		$ff25			* INTVCS
		move.l		d0,old_b_putc

		pea		null_print(pc)
		move.w		#$121,-(sp)		* B_PRINT
		dc.w		$ff25			* INTVCS
		move.l		d0,old_b_print

		lea		2*6(sp),sp

		rts


*
*
*	void	enable_print(void)
*
*

_enable_print:
		move.l		old_b_putc,-(sp)
		move.w		#$120,-(sp)		* B_PUTC
		dc.w		$ff25			* INTVCS

		move.l		old_b_print,-(sp)
		move.w		#$121,-(sp)		* B_PRINT
		dc.w		$ff25			* INTVCS

		lea		2*6(sp),sp
		rts


null_putc:
null_print:
		clr.l		d0
		rts


*
*
*	void	disable_int(void)
*
*

_disable_int:
		addq.w		#1,disable_flag
		rts


*
*
*	void	enable_int(void)
*
*

_enable_int:
		subq.w		#1,disable_flag
		rts


*
*
*	void	kill_int(void)
*
*

_kill_int:
		move.w		#0,int_flag
		rts


*
*
*	void	intercept(void (*put)(int), void (*flush_put)(void))
*
*

_intercept:

		link		a6,#0

		pea		_b_keyinp(pc)
		move.w		#$100,-(sp)		* B_KEYINP
		dc.w		$ff25
		move.l		d0,old_b_keyinp

		pea		_b_keysns(pc)
		move.w		#$101,-(sp)		* B_KEYSNS
		dc.w		$ff25
		move.l		d0,old_b_keysns

		pea		_b_putc(pc)
		move.w		#$120,-(sp)		* B_PUTC
		dc.w		$ff25			* INTVCS
		move.l		d0,old_b_putc

		pea		_b_print(pc)
		move.w		#$121,-(sp)		* B_PRINT
		dc.w		$ff25			* INTVCS
		move.l		d0,old_b_print

		lea		4*6(sp),sp

		move.l		 8(a6),put
		move.l		12(a6),flush_put
		clr.w		kanji_buf

		move.w		#FLUSH_COUNT,flush_count
		move.w		#0,disable_flag
		move.w		#-1,int_flag
		move.l		#-1,_intercept_flag

		unlk		a6
		rts


*
*
*	void	intercept_end(void)
*
*

_intercept_end:

		clr.l		_intercept_flag
		clr.w		int_flag

		move.l		old_b_keyinp,-(sp)
		move.w		#$100,-(sp)		* B_KEYINP
		dc.w		$ff25			* INTVCS

		move.l		old_b_keysns,-(sp)
		move.w		#$101,-(sp)		* B_KEYSNS
		dc.w		$ff25			* INTVCS

		move.l		old_b_putc,-(sp)
		move.w		#$120,-(sp)		* B_PUTC
		dc.w		$ff25			* INTVCS

		move.l		old_b_print,-(sp)
		move.w		#$121,-(sp)		* B_PRINT
		dc.w		$ff25			* INTVCS

		lea		4*6(sp),sp
		rts


*
*
*	_b_keyinp
*
*

_b_keyinp:
		tst.w		disable_flag
		bne		_b_keyinp_end

		move.w		#FLUSH_COUNT,flush_count
		movem.l		d1-d7/a1-a6,-(sp)
		addq.w		#1,disable_flag
		addq.l		#1,_issuper
		movea.l		flush_put,a0
		jsr		(a0)
		subq.l		#1,_issuper
		subq.w		#1,disable_flag
		movem.l		(sp)+,d1-d7/a1-a6

_b_keyinp_end	move.l		old_b_keyinp,-(sp)
		rts


*
*
*	_b_keysns
*
*

_b_keysns:
		tst.w		disable_flag
		bne		_b_keysns_end
		subq.w		#1,flush_count
		bne		_b_keysns_end

		move.w		#FLUSH_COUNT,flush_count
		movem.l		d1-d7/a1-a6,-(sp)
		addq.w		#1,disable_flag
		addq.l		#1,_issuper
		movea.l		flush_put,a0
		jsr		(a0)
		subq.l		#1,_issuper
		subq.w		#1,disable_flag
		movem.l		(sp)+,d1-d7/a1-a6

_b_keysns_end	move.l		old_b_keysns,-(sp)
		rts


*
*
*	_b_putc
*
*

_b_putc:
		tst.w		disable_flag
		bne		_b_putc_normal

		move.w		#FLUSH_COUNT,flush_count
		addq.w		#1,disable_flag
		addq.l		#1,_issuper
		move.l		d1,d0
		bsr		_put
		subq.l		#1,_issuper
		subq.w		#1,disable_flag
		moveq.l		#0,d0
		rts

_b_putc_normal	move.l		old_b_putc,-(sp)
		rts


*
*
*	_b_print
*
*

_b_print:
		tst.w		disable_flag
		bne		_b_print_normal

		move.w		#FLUSH_COUNT,flush_count
		addq.w		#1,disable_flag
		addq.l		#1,_issuper
_b_print_l	moveq.l		#0,d0
		move.b		(a1)+,d0
		beq		_b_print_end
		bsr		_put
		bra		_b_print_l

_b_print_end	subq.l		#1,_issuper
		subq.w		#1,disable_flag
		moveq.l		#0,d0
		rts

_b_print_normal	move.l		old_b_print,-(sp)
		rts


_put		tst.w		int_flag
		beq		_put_rutin_skip

		movem.l		d1-d7/a1-a6,-(sp)

		tst.w		kanji_buf
		beq		_put_b10
		and.l		#$ff,d0
		or.w		kanji_buf,d0
		bra		_put_b30

_put_b10	and.l		#$ffff,d0
		cmp.w		#$80,d0
		bcs		_put_b30	* ANK
		cmp.w		#$a0,d0
		bcs		_put_b20	* KANJI
		cmp.w		#$e0,d0
		bcs		_put_b30	* ANK

_put_b20	lsl.w		#8,d0
		move.w		d0,kanji_buf
		bra		_put_end

_put_b30	move.l		d0,-(sp)
		movea.l		put,a0
		jsr		(a0)
		addq.l		#4,sp
		clr.w		kanji_buf

_put_end
		movem.l		(sp)+,d1-d7/a1-a6
_put_rutin_skip
		rts


*---------------------------------------------------------------------

_intercept_flag:
		ds.l		1

int_flag	dc.w		0

flush_count	ds.w		1

old_PUTCHAR	ds.l		1
old_PRINT	ds.l		1
old_b_keyinp	ds.l		1
old_b_keysns	ds.l		1
old_b_putc	ds.l		1
old_b_print	ds.l		1

kanji_buf	ds.w		1
put		ds.l		1
flush_put	ds.l		1

disable_flag	ds.w		1

*=====================================================================

		end

