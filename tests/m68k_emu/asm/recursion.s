; Unbounded recursion
	section code,code
recurse:	move.l	d0,-(sp)
	bsr.s	recurse
	rts
