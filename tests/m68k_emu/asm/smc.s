; Self-modifying code
	section code,code
	lea	.patch(pc),a0
	move.w	#$7005,(a0)
.patch:	moveq	#1,d0
	rts
