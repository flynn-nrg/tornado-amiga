; Uses _helper and _helper_data from mod_b.s.
	xdef	_twice_plus
	xref	_helper
	xref	_helper_data
	section	code,code
; d0 = 4 * d0 + helper_data
_twice_plus:
	move.l	d2,-(sp)
	move.l	d0,d2
	bsr	_helper			; EXT_REF16
	exg	d0,d2
	jsr	_helper			; EXT_REF32
	add.l	d2,d0
	add.l	_helper_data,d0		; EXT_REF32, data
	move.l	(sp)+,d2
	rts
