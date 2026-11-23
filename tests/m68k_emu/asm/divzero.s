; Divide by zero
	section code,code
	moveq	#0,d1
	divu.w	d1,d0
	rts
