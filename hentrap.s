*
*	Public Domain Software
*
*
*	Written by Masatoshi Yoshizawa (Yoz.) for GAPO jstevie Human68k
*	Added abort trap by Sawayanagi Yosirou <willow@saru.cc.u-tokyo.ac.jp>
*	1992.02.11 little modified by Y.Ogawa(GAPO) 
*	1992.02.23 little modified for MicroEMACS by SALT
*

		.globl	_hentrap,_iskmode

		.text
*****************************************
* void hentrap(int flag, void (*handle_func)(int))
* int flag  0 --- hentrap hook start.
*          -1 --- hentrap hook end.
* void (*handle_func)(int)
*   when fep on/off call this function
*****************************************
_hentrap:
		tst.l	4(SP)
		beq	hentrap0

		move.l	8(sp),fep_handle_func

		clr.l	-(sp)
		move.l	#1,-(sp)
		dc.w	$ff22		* KNJCTRL(1,0)
		addq.l	#8,sp

		clr.l	henflag

		pea	hentrapmain(pc)
		move.w	#$ff18,-(sp)
		dc.w	$ff25		* intvcs
		addq.l	#6,sp

		move.l	d0,oldvect

		pea	aborthentrap
		move.w	#$fff2,-(sp)
		dc.w	$ff25		* intvcs
		addq.l	#6,sp

		move.l	d0,abortvect

		rts

hentrap0:
		clr.l	fep_handle_func

		move.l	abortvect,-(sp)
		move.w	#$fff2,-(sp)
		dc.w	$ff25		* intvcs
		addq.l	#6,sp

		move.l	oldvect,-(sp)
		move.w	#$ff18,-(sp)
		dc.w	$ff25		* intvcs
		addq.l	#6,sp

		rts

*************************************
* int iskmode (void)
* return value
*    0  --- ask henkanmode off.
*    -1 --- ask henkanmode on.
*************************************
_iskmode:
		move.l	henflag,d0
		rts

hentrapmain:
		move.w	(A6),D0
		beq	henopen		* mode window open
		cmp.w	#3,D0
		beq	henclose	* mode window close
hencont:
		move.l	oldvect,A0
		jmp	(A0)

henopen:
		move.l	#-1,henflag
		bra	henhandle

henclose:
		clr.l	henflag
*		bra	henhandle

henhandle:
		movem.l	d0-d7/a0-a6,-(sp)
		move.l	fep_handle_func,d0
		beq	henhandle_end
		movea.l	d0,a0
		move.l	henflag,-(sp)
		jsr	(a0)
		addq.l	#4,sp
henhandle_end:
		movem.l	(sp)+,d0-d7/a0-a6
		bra	hencont

aborthentrap:
		bsr	hentrap0
		move.l	abortvect,A0
		jmp	(A0)

		.bss
henflag:
		ds.l	1
oldvect:
		ds.l	1
abortvect:
		ds.l	1
fep_handle_func:
		ds.l	1

		.end
