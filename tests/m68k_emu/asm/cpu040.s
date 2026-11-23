; 68020+ integer and FPU instructions
	section code,code
	mulu.l	d1,d0
	extb.l	d2
	add.l	d2,d0
	fmove.l	d0,fp0
	fmul.s	#0.5,fp0
	fmove.l	fp0,d0
	rts
