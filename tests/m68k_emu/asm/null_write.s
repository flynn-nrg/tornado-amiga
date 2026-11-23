; Write through a NULL pointer in a0
	section code,code
	move.l	d0,(a0)
	rts
