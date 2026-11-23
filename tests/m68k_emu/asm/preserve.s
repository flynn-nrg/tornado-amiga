; Preserves d2-d7/a2-a6 properly
	section code,code
	movem.l	d2-d7/a2-a6,-(sp)
	moveq	#0,d2
	moveq	#0,d7
	sub.l	a2,a2
	sub.l	a6,a6
	movem.l	(sp)+,d2-d7/a2-a6
	moveq	#42,d0
	rts
