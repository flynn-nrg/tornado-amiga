; d0 = sum of d0 big-endian longs at a0
	section code,code
	move.l	d0,d1
	moveq	#0,d0
.loop:	add.l	(a0)+,d0
	subq.l	#1,d1
	bne.s	.loop
	rts
