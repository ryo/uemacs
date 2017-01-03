*************************************************************************
*	C-RunTime Routine						*
*1987/12/10	ARGC,ARGV � DEBUG					*
*************************************************************************
* Copyright 1987 SHARP/Hudson		(1987/11/03)			*

_STACK_MIN	equ	4096
_HEAP_MIN	equ	64		* changed by SALT  1991 10/11(Fri)
LIB_STACK	equ	1024

	xdef	__main			. startup main
	xdef	__stack_over
	xdef	__fmode
	xdef	__liobuf
	xdef	__argbuffer
	xdef	__argvector
	xdef	__argc
	xdef	__argv
	xdef	__cmdline
	xdef	_environ
	xdef	__ENV0
	xdef	__ENV1
	xdef	__PSP
	xdef	__PSTA
	xdef	__PEND
	xdef	__DSTA
	xdef	__DEND
	xdef	__BSTA
	xdef	__BEND
	xdef	__SSTA
	xdef	__SEND
	xdef	__HSTA
	xdef	__HDEF
	xdef	__HEND
	xdef	MALLOCP
	xdef	_daylight
	xdef	_timezone
	xdef	_tzname
	xdef	_tzstn
	xdef	_tzdtn
	xdef	_errno
	xdef	__doserrno
	xdef	__iob
	xdef	__pback

	xref	_STACK_SIZE
	xref	_HEAP_SIZE
	xref	_main			. for user's main
	xref	_tzset
	xref	_malloc
	xref	__getiob		. for standard iob set
	xref	_exit
	xref	__exit
	xref	__dump_flag

	include	doscall.mac
	include	stdio.mac
	include	fcntl.mac

	comm	_errno,4
	comm	__doserrno,4
	comm	__iob,(SIZE_IOBUF*__NFILE)
	comm	__pback,2

*000000	mem pointer(16)
*000010	PSP	__PSP
*000100	-----	__PSTA
*	.text(program)
*	     (lib program & data)
*	-----	__PEND
*??????	-----	__DSTA
*	.data(data)
*	-----	__DEND
*??????	-----	__BSTA
*	.bss(C text bss)
*	.stack(clib bss)
*	-----	__BEND
*	ds.b	LIB_STACK�o�C�g�i�X�^�b�N�̗]�T�j
*??????	-----	__SSTA
*	ds.b	_STACK_SIZE(stack & auto)
*	-----	__SEND
*??????	-----	__HSTA
*	ds.b	_HEAP_SIZE
*??????	-----	_environ		* ���ϐ��͂����ֈړ� by SALT  1991 10/12(Sat)
*	ds.b	envlen			*
*	ds.b	envlen			*
*??????	-----	__HDEF(rblk�ł����ɂ��ǂ����Ƃ��o����)
*??????	-----	__HEND(sblk�ŕύX�\)
*	OS free area
*	child & level 2 mem�Ǘ�

	.text
	dc.b	'C library for X68000 XC���߲� v1.00',0
	dc.b	'Copyright 1987 SHARP/hudson',0
	.even

__main:
	lea	initstack,sp		. work

					* �X�g���[�W�̈��`�̏�����
					* added by SALT  1991 10/11(Fri)
	movem.l	d0/a0,-(sp)

	lea	__PSP,a0
	moveq.l	#0,d0

	move.l	d0,(a0)+		* __PSP
	move.l	d0,(a0)+		* __PSTA
	move.l	d0,(a0)+		* __PEND
	move.l	d0,(a0)+		* __DSTA
	move.l	d0,(a0)+		* __DEND
	move.l	d0,(a0)+		* __BSTA
	move.l	d0,(a0)+		* __BEND

	lea	__ENV0,a0
	move.w	#clear_end-clear_begin-1,d0
clear_loop:
	move.b	d0,(a0)+
	dbra	d0,clear_loop

	movem.l	(sp)+,d0/a0


	move.l	a2,__cmdline		. �R�}���h�E���C�� �A�h���X �Z�b�g
	move.l	a3,__ENV0		. ���A�h���X �Z�b�g
	bsr	_getarg			. /stack:100k /heep:65536 �\
					* / �̓I�v�V�����ƌ��Ȃ��Ȃ��悤�ɂ���
					* by SALT  1991 10/11(Fri)

	lea	$100(a0),a5
	move.l	a5,__PSTA
	move.l	$34(a0),d0
	move.l	d0,__PEND
	move.l	d0,__DSTA
	move.l	$30(a0),d0
	move.l	d0,__DEND
	move.l	d0,__BSTA
	move.l	a1,__BEND
	lea	16(a0),a5
	move.l	a1,d1
	addq.l	#1,d1
	and.l	#$fffffe,d1

					* added by SALT  1991 10/11(Fri)
	tst.l	__dump_flag
	bne	setblock

	add.l	#LIB_STACK,d1		. lib stack
	move.l	d1,__SSTA
	move.l	__STACK,d0
	cmp.l	#_STACK_MIN,d0
	bcc	stackok
	move.l	#_STACK_MIN,d0
stackok:
	addq.l	#1,d0
	and.l	#$fffffe,d0
	add.l	d0,d1
	move.l	d1,__SEND

heap:
	move.l	d1,__HSTA
	move.l	__HEAP,d0
	cmp.l	#_HEAP_MIN,d0
	bcc	heapok
	move.l	#_HEAP_MIN,d0
heapok:
	add.l	#12+1,d0
	and.l	#$fffffe,d0
	add.l	d0,d1
	move.l	d1,__HDEF
	move.l	d1,__HEND

setblock:
	move.l	a5,__PSP		. �������Ǘ��|�C���^�E�A�h���X �Z�b�g
	sub.l	a5,d1			. ���v���O�����E�T�C�Y
	move.l	d1,-(sp)		. �m�ۂ���̈�T�C�Y �Z�b�g
	move.l	a5,-(sp)		. �m�ۂ���̈�̐擪�A�h���X �Z�b�g
	DOS	_SETBLOCK		. �󂫗̈�m��
	addq.l	#8,sp
	tst.l	d0
	bmi	_memerr

	move.l	__SEND,sp

	tst.l	__dump_flag
	bne	env

*	�q�[�v�̈�̏�����		* deleted by SALT  1991 10/11(Fri)
	move.l	__HSTA,a0
	move.l	a0,MALLOCP
	lea	16(a0),a1
	clr.l	(a0)+
	move.l	a1,(a0)+
	clr.l	(a0)+
	move.l	#'HEAP',(a0)

env:					* moved here by SALT  1991 10/12(Sat)
	moveq.l	#0,d1
	move.l	d1,_environ
	move.l	__ENV0,a0
	move.l	(a0),d0
*	addq.l	#4,d0			. NULL space
	addq.l	#5,d0			. NULL space
	and.l	#$FFFFFE,d0		* add 1988/01/16
	add.l	d0,d1
	move.l	d1,__ENV1

	move.l	__ENV0,a0
	addq.l	#4,a0
	clr.l	d0
envlenc2:
	addq.l	#1,d0
	tst.b	(a0)+
	beq	envlenok
envlenck:
	addq.l	#1,d0
	tst.b	(a0)+
	bne	envlenck
	bra	envlenc2
envlenok:
	addq.l	#1,d0
	and.l	#$fffffe,d0
	add.l	d0,d1

	move.l	d1,-(sp)
	jsr	_malloc
	addq.l	#4,sp
	tst.l	d0
	beq	_memerr

	add.l	d0,_environ
	add.l	d0,__ENV1

*	env copy
	move.l	__ENV0,a0
	move.l	__ENV1,a1
	addq.l	#4,a0
envcpy:
	move.b	(a0)+,(a1)+
	beq	envcpe
envcp0:
	move.b	(a0)+,(a1)+
	bne	envcp0
	bra	envcpy
envcpe:
	move.l	__ENV1,a0
	move.l	_environ,a1
envcp1:
	tst.b	(a0)
	beq	envend
	move.l	a0,(a1)+
envskp:
	tst.b	(a0)+
	bne	envskp
	bra	envcp1
envend:
	clr.l	(a1)

envok:
*	�O���[�o���̈�̏�����
	clr.w	__pback
	clr.l	__fmode			. text mode
	lea	__liobuf,a0
	lea	liomod,a1
	clr.l	d1
lioinit:
	move.l	d1,-(sp)		. 0,d1.w
	DOS	_IOCTRL
	addq.l	#4,sp
	move.b	d1,(a0)+
	move.b	d0,(a0)+
	move.w	(a1)+,(a0)+
	addq.w	#1,d1
	cmp.w	#5,d1
	bne	lioinit

	move.l	#((__NFILE)*SIZE_IOBUF)-1,d0
	move.l	#__iob,a0
_loop:
	clr.b	(a0)+
	dbra	d0,_loop

	lea	__iob,a0
	lea	liomod,a1
	clr.l	d1
fioinit:
	move.l	d1,-(sp)		. 0,d1.w
	DOS	_IOCTRL
	addq.l	#4,sp
	btst.l	#OS_ISDEV,d0
	beq	blkdev
	move.l	#_IOCHARA+_IONBF,d0
	bra	fiook
blkdev:
	move.l	#_IOBDEV,d0
	movem.l	d0-d1/a1,-(sp)
	move.l	a0,-(sp)
	move.l	#_BUFSIZ,-(sp)
	jsr	_malloc
	addq.l	#4,sp
	tst.l	d0
	beq	_memerr
	move.l	(sp)+,a0
	move.l	a2,_PTR(a0)
	move.l	a2,_BASE(a0)
	move.l	#_BUFSIZ,_BSIZE(a0)
	movem.l	(sp)+,d0-d1/a1
fiook:
	or.w	(a1)+,d0
	clr.l	_CNT(a0)
	move.l	d0,_FLAG(a0)
	move.b	d1,_FILE(a0)
	lea	SIZE_IOBUF(a0),a0
	addq.w	#1,d1
	cmp.w	#5,d1
	bne	fioinit
	jsr	_tzset
	pea	ctrlca
	move.w	#_CTRLVC,-(sp)
	DOS	_INTVCS
	addq.l	#6,sp
	move.l	_environ,-(sp)
	move.l	__argv,-(sp)
	move.l	__argc,-(sp)
	jsr	_main
	lea	12(sp),sp
	move.l	d0,-(sp)
	jsr	_exit

ctrlca:
	DOS	_GETPDB
	move.l	#$200,d1
	cmp.l	__PSP,d0
	beq	mainex
	move.w	d1,-(sp)
	DOS	_EXIT2

__stack_over:
	pea	stkmsg
	DOS	_PRINT
	moveq.l	#127,d1
mainex:
	move.l	d1,-(sp)
	jsr	_exit

_memerr:
	pea	errmsg
	DOS	_PRINT
	addq.l	#4,sp
	move.l	#127,-(sp)
	jsr	__exit

*	argment list make
_getarg:
	movem.l	a0-a2,-(sp)
	lea	__argbuffer,a2		. argument buffer
	lea	__argvector,a1		. argv buffer
	move.l	a2,(a1)+		. argv[0] = 'C'
	movem.l	a0,-(sp)
	lea	$80(a0),a0

pthset:
	move.b	(a0)+,(a2)+		. �d�w�d�b�t�@�C���̃R�}���h�� �Z�b�g
	bne	pthset
	move.l	(sp)+,a0
	tst.b	-(a2)
	lea	$c4(a0),a0
cmdset:
	move.b	(a0)+,(a2)+		. �d�w�d�b�t�@�C���̃R�}���h�� �Z�b�g
	bne	cmdset

	clr.l	d0			. �������� �� �O
	move.l	__cmdline,a0		. _Base Get
	move.b	(a0)+,d1		. d1 = cmd line length get
	beq	ArgExit			. No argument

ArgLp0:
	move.b	(a0)+,d1
	beq	ArgEx			. �p�����[�^�I���R�[�h���o
	cmpi.b	#$09,d1			. �s�`�a �H
	beq	ArgLp0
	cmpi.b	#$20,d1			. �r�o�`�b�d �H
	beq	ArgLp0
	addq.l	#1,d0			. ���������{�{
	move.l	a2,(a1)+		. argv[1]= address
	cmp.b	#$27,d1			. '
	beq	ArgLp27
	cmp.b	#$22,d1			. "
	bne	ArgLoop
ArgLp22:
	move.b	(a0)+,d1
	beq	ArgEx
	cmp.b	#$22,d1			. "
	beq	ArgEx
	move.b	d1,(a2)+
	bra	ArgLp22
ArgLp27:
	move.b	(a0)+,d1
	beq	ArgEx
	cmp.b	#$27,d1			. "
	beq	ArgEx
	move.b	d1,(a2)+
	bra	ArgLp27
ArgLoop:
	move.b	d1,(a2)+		. �p�����[�^�Z�b�g�i�����������m���O�n�{�{�j
	move.b	(a0),d1
	beq	ArgEx
	cmp.b	#$22,d1
	beq	ArgEx
	cmp.b	#$27,d1
	beq	ArgEx
	addq.l	#1,a0
	cmpi.b	#$09,d1			. �s�`�a �H
	beq	ArgEx
	cmpi.b	#$20,d1			. �r�o�`�b�d �H
	bne	ArgLoop
ArgEx:
	clr.b	(a2)+			. �������� �������� �Z�b�g
	movem.l	d1/a0/a2,-(sp)
	move.l	-4(a1),a2
	move.b	(a2)+,d1
	cmp.b	#'-',d1
*	beq	chklbl			* deleted by SALT  1991 10/11(Fri)
*	cmp.b	#'/',d1			* deleted by SALT  1991 10/11(Fri)
	bne	nochk
chklbl:
	lea	stackms,a0
	bsr	a0a2ck
	beq	stkset
	lea	heapms,a0
	bsr	a0a2ck
	bne	nochk
	bsr	numget
	move.l	d1,__HEAP
a1a2bk:
	movem.l	(sp)+,d1/a0/a2
	move.l	-(a1),a2
	subq.l	#1,d0
	bra	argexc
stkset:
	bsr	numget
	move.l	d1,__STACK
	bra	a1a2bk
nochk:
	movem.l	(sp)+,d1/a0/a2
argexc:
	tst.b	d1
	bne	ArgLp0
ArgExit:
	move.b	d1,(a2)+		. �������� �������� �Z�b�g
	lea	__argvector,a0
	addq.l	#1,d0			. ���������{�{
	move.l	d0,__argc
	move.l	a0,__argv
	movem.l	(sp)+,a0-a2
	rts

a0a2ck:
	movem.l	a2,-(sp)
a0a2cl:
	move.b	(a0)+,d1
	beq	a0a2ok
	move.b	(a2)+,d2
	beq	a0a2ng
	or.b	#$20,d2
	cmp.b	d2,d1
	beq	a0a2cl
a0a2ng:
	tst.b	d1
	movem.l	(sp)+,a2
	rts
a0a2ok:
	addq.l	#4,sp
	rts

numget:
	clr.l	d1
	clr.l	d2
numgl:
	move.b	(a2)+,d2
	sub.b	#'0',d2
	cmp.b	#10,d2
	bcc	numgte
	add.l	d1,d1
	move.l	d1,d3
	add.l	d1,d1
	add.l	d1,d1
	add.l	d3,d1
	add.l	d2,d1
	bra	numgl
numgte:
	add.b	#'0',d2
	or.b	#$20,d2
	cmp.b	#'k',d2
	bne	numnok
	asl.l	#8,d1
	asl.l	#2,d1
numnok:
	bclr.l	#0,d1
	rts

*****************************************
*	�R���X�^���g�̈��`		*
*****************************************

liomod:
		dc.w	O_RDONLY+O_TEXT	. con in
		dc.w	O_WRONLY+O_TEXT	. con out
		dc.w	O_RDWR+O_TEXT	. con err
		dc.w	O_RDWR+O_TEXT	. aux
		dc.w	O_WRONLY+O_TEXT	. prn
_tzname:	dc.l	_tzstn
		dc.l	_tzdtn
_tzstn:		dc.b	'JST',0		. �p�V�t�B�b�N�E�^�C���]�[��
_tzdtn:		dc.b	0,0,0,0		. �Ď��Ԓ������s��
_daylight:	dc.l	0		. �Ď��ԍ̗p�̗L��
_timezone:	dc.l	-9*60*60	. �O���j�b�W���ԂƂ̌덷
	.even
__STACK:	dc.l	_STACK_SIZE
__HEAP:		dc.l	_HEAP_SIZE

errmsg:		dc.b	'��L�����s�����Ă��܂��I�I�I',$0D,$0A,0
stkmsg:		dc.b	'�X�^�b�N���s�����Ă��܂��I�I�I',$0D,$0A,0
stackms:	dc.b	'+-s:',0	* deleted by SALT  1991 10/22(Tue)
heapms:		dc.b	'+-h:',0	* deleted by SALT  1991 10/22(Tue)

	.stack

*	�X�g���[�W�̈��`

	.even

__PSP:		ds.l	1
__PSTA:		ds.l	1
__PEND:		ds.l	1
__DSTA:		ds.l	1
__DEND:		ds.l	1
__BSTA:		ds.l	1
__BEND:		ds.l	1
__SSTA:		ds.l	1
__SEND:		ds.l	1
__HSTA:		ds.l	1
__HDEF:		ds.l	1
__HEND:		ds.l	1
MALLOCP:	ds.l	1

clear_begin:				* added by SALT  1991 10/11(Fri)

__ENV0:		ds.l	1		. �e�̊�
__ENV1:		ds.l	1		. �����̊�
_environ:	ds.l	1		. �����̊��̃|�C���^�z��
__cmdline:	ds.l	1
__argc:		ds.l	1
__argv:		ds.l	1
__liobuf:
*		dc.b	�n�r�̃n���h��
*		dc.b	�h�n�b�s�q�k�f�o�C�X���
*		dc.w	�I�[�v�����[�h(0�łn�o�d�m����Ă��Ȃ�)
		ds.l	96
__fmode:	ds.l	1
__argbuffer:	ds.b	512
__argvector:	ds.b	20*4

clear_end:				* added by SALT  1991 10/11(Fri)

		ds.l	20
initstack:

	.end	__main
