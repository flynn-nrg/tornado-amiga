; Fill d0 bytes at a0 with d1.b
	section code,code
	subq.l	#1,d0
.loop:	move.b	d1,(a0)+
	dbra	d0,.loop
	rts
