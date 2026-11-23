; Privileged instruction in user mode
	section code,code
	move.w	#$2700,sr
	rts
