; Returns with sp 4 bytes too low
	section code,code
	move.l	(sp),-(sp)
	rts
