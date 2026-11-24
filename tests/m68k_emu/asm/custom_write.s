; Write COLOR00
	section code,code
	move.w	#$0f00,$dff180
	moveq	#1,d0
	rts
