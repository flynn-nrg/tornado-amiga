; Routines for the data sharing and allocation checking tests.
	xdef	_write_longs
	xdef	_read_longs
	xdef	_deref2
	xdef	_sum_words
	section	code,code

; Writes d0 longs of value d1 to a0.
_write_longs:
	subq.l	#1,d0
.loop:	move.l	d1,(a0)+
	dbra	d0,.loop
	rts

; d0 = sum of the d0 longs at a0.
_read_longs:
	move.l	d0,d1
	moveq	#0,d0
.loop:	add.l	(a0)+,d0
	subq.l	#1,d1
	bne.s	.loop
	rts

; d0 = (a0)[d0][d1][0]: two levels of 68k pointer tables, like zoom's
; unsigned char ***allTxtPtr.
_deref2:
	move.l	(a0,d0.l*4),a1
	move.l	(a1,d1.l*4),a1
	moveq	#0,d0
	move.b	(a1),d0
	rts

; d0 = sum of the d0 signed words at a0.
_sum_words:
	move.l	d0,d1
	moveq	#0,d0
.loop:	move.w	(a0)+,a1	; sign extends
	add.l	a1,d0
	subq.l	#1,d1
	bne.s	.loop
	rts
