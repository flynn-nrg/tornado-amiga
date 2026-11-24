; Nested calls, the innermost one crashes
	section code,code
outer:	movem.l	d2/a2,-(sp)
	bsr.s	middle
	movem.l	(sp)+,d2/a2
	rts
middle:	bsr.s	inner
	rts
inner:	move.l	#$BADC0DED,(a0)+
	move.l	#$40000000,a0
	move.l	d0,(a0)
	rts
