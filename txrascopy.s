*------------------------------------------------------------------
*---------------------------------------*
*
*	高速ラスターコピールーチン	by 卑弥呼☆
*
*	IOCS $DFと同じ条件でコールする
*

	.include	doscall.mac
	.include	iocscall.mac

	.xref		_ena_wait_vdisp

CRTC_R21	equ	$00e8002a
SHFTWORK	equ	$0000080e
	.public	_TXRASCOPY
	.public	TXRASCPY

	.text

*
*	RCS id
*

	dc.b	'$Header: f:/SALT/emacs/RCS/txrascopy.s,v 1.2 1991/09/01 10:42:12 SALT Exp $'
	even


_TXRASCOPY:
	movem.l	d1-d7,-(a7)
	movem.l	8*4(sp),d1-d3

	tst.l	_issuper
	bne	h_print_sup1

	clr.l	a1
	moveq.l	#$81,d0
	trap	#15		*super
	move.l	d0,(sp)

h_print_sup1:
	jsr	vdispchk
	jsr	TXRASCPY

	tst.l	_issuper
	bne	h_print_sup2
	movea.l	(sp),a1
	moveq.l	#$81,d0
	trap	#15

h_print_sup2:
	movem.l	(a7)+,d1-d7
	rts


vdispchk:				* ------- 帰線期間待ち -----------------
	btst.b	#1,SHFTWORK
	bne	vdisp_skip		* [CTRL]キーがおされていたら 帰線期間無視
	tst.w	(_ena_wait_vdisp)
	beq	vdisp_skip
	movea.l	#$e88001,a1
wait_vdisp:
	btst.b	#4,(a1)	
	bne	wait_vdisp
vdisp_skip:
	rts


TXRASCPY
	movem.l	D1-D2,-(A7)
	move.w	(CRTC_R21),-(A7)
	move.w	D3,D0
	and.w	#$000F,D0
	or.w	#$0100,D0
	move.w	D0,(CRTC_R21)
	move.w	#$0101,D0
	tst.w	D3
	bpl	TXRC1
	move.w	#$FEFF,D0
TXRC1
	exg	D0,D2
	jsr	RASCPY
	move.w	(A7)+,(CRTC_R21)
	movem.l	(A7)+,D1-D2
	rts
RASCPY
	subq.w	#1,D0
	bmi	RASCPYE
	clr.b	$00E88005
	bset.b	#$0000,(CRTC_R21)
	movem.l	d3-d4/a0-a2,-(sp)
	lea	$00E88001,a0
	lea	$00E8002C,a1
	lea	$00E80480,a2

	moveq	#$0007,d3
	moveq	#$0008,d4
RASCPY1
	ori.w	#$0700,SR
RASCPY22
	btst.b	d3,(a0)
	bne.w	RASCPY22
RASCPY2
	btst.b	d3,(a0)
	beq.w	RASCPY2
	move.w	D1,(a1)
	move.w	d4,(a2)
	andi.w	#$F8FF,SR
	add.w	D2,D1
	dbf	D0,RASCPY1
RASCPY33
	btst.b	d3,(a0)
	bne.w	RASCPY33
RASCPY3
	btst.b	d3,(a0)
	beq.w	RASCPY3
	clr.w	(a2)
	movem.l	(sp)+,d3-d4/a0-a2
	bclr.b	#$0000,(CRTC_R21)
RASCPYE
	rts
	.data
resp
	ds.l	1
resds
	ds.l	1
d3stor
	ds.l	1
