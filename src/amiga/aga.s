	section text, text
	public _checkAGA
	public _waitVBL

LVOOpenLibrary EQU -552
LVOCloseLibrary EQU -414
	
_checkAGA:
	movem.l  d7/a1/a6, -(sp)
        lea     gfx_lib, a1
        moveq   #0,d0
        move.l  4.w,a6
        jsr     LVOOpenLibrary(a6)

        btst    #2, $ec(a0)
        bne.s   is_aa
        moveq   #0, d7
        bra.s   not_aga
is_aa   moveq  #1, d7

not_aga move.l  a0, a1
        jsr     LVOCloseLibrary(a6)
	move.l  d7, d0
	movem.l (sp)+, d7/a1/a6
        rts

_waitVBL:
	move.l	$dff004,d0
	and.l	#$1ff00,d0
	cmp.l	#303<<8,d0
	bne.b	_waitVBL
	rts
	
	section data, data

gfx_lib 	dc.b    "graphics.library",0,0
