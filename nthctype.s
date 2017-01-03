*
*	RCS id
*

		dc.b		'$Header: f:/SALT/emacs/RCS/nthctype.s,v 1.1 1991/08/11 11:03:42 SALT Exp $'
		even

*
*	nthctype.s - n ”Ô–Ú‚Ì•¶Žš‚ª ‚Ç‚Ì‚æ‚¤‚È•¶Žš‚©‚ð•Ô‚·
*
*	int nthctype(const char *str, int n)
*
*	n > 0
*
*	return value:
*	  CT_ANK	0
*	  CT_KJ1	1
*	  CT_KJ2	2
*	  CT_ILGL	-1
*

CT_ANK		equ		0
CT_KJ1		equ		1
CT_KJ2		equ		2
CT_ILGL		equ		-1

arg1		reg		4(sp)
arg2		reg		8(sp)

		xdef		_nthctype

_nthctype:
		movea.l		arg1,a0
		move.l		arg2,d1
		ble		nthctype_ilgl

nthctype_l1	subq.l		#1,d1
		beq		nthctype_b2
		move.b		(a0)+,d0
		beq		nthctype_ilgl
		cmp.b		#$80,d0
		bcs		nthctype_l1		* not KANJI
		cmp.b		#$a0,d0
		bcs		nthctype_b1		* KANJI
		cmp.b		#$e0,d0
		bcs		nthctype_l1		* not KANJI
		cmp.b		#$f6,d0
		bcs		nthctype_b1		* KANJI
		bra		nthctype_ilgl

nthctype_b1	subq.l		#1,d1
		beq		nthctype_b3
		move.b		(a0)+,d0
		beq		nthctype_ilgl
		cmp.b		#$40,d0
		bcs		nthctype_ilgl
		cmp.b		#$7f,d0
		beq		nthctype_ilgl
		bra		nthctype_l1

nthctype_b2	move.b		(a0)+,d0
		cmp.b		#$80,d0
		bcs		nthctype_ank
		cmp.b		#$a0,d0
		bcs		nthctype_kj1
		cmp.b		#$e0,d0
		bcs		nthctype_ank
		cmp.b		#$f6,d0
		bcs		nthctype_kj1
		bra		nthctype_ilgl

nthctype_b3	move.b		(a0)+,d0
		beq		nthctype_ilgl
		cmp.b		#$40,d0
		bcs		nthctype_ilgl
		cmp.b		#$7f,d0
		beq		nthctype_ilgl

nthctype_kj2	moveq.l		#CT_KJ2,d0
		rts

nthctype_kj1	moveq.l		#CT_KJ1,d0
		rts

nthctype_ank	moveq.l		#CT_ANK,d0
		rts

nthctype_ilgl	moveq.l		#CT_ILGL,d0
		rts

		end
