	section text
	public _blurRenderScanlineAsm
	public _blurRenderScanlineMixAsm

	cnop	0,16
_blurRenderScanlineAsm:
	tst.l	d7
	beq.b	.skip
	movem.l d1-a6,-(sp)

	moveq.l	#0,d0
	moveq.l	#0,d1
	move.l	#16,d5

	move.w	(a2)+,d0
	move.w	(a2)+,d1

	ror.l	#1,d7
	sub.w	#1,d7
	blt.b	.skip2

	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	bra.b	.st2

	cnop	0,16
.loop2:
	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	move.b	d3,(a6)+
.st2:	lsr.l	d5,d2
	move.l	(a1,d1.l*4),d3
	sub.l	(a1,d0.l*4),d3
	add.l	(a0,d0.l*4),d3
	sub.l	(a0,d1.l*4),d3
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d3
	move.b	d2,(a6)+
	lsr.l	d5,d3
	dbf	d7,.loop2
	move.b	d3,(a6)+
.skip2:

	rol.l	#1,d7
	sub.w	#1,d7
	blt.b	.skip1
.loop1:	
	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	lsr.l	d5,d2
	move.b	d2,(a6)+
	dbf	d7,.loop1
.skip1:
	movem.l (sp)+, d1-a6
.skip:	rts


	cnop	0,16
_blurRenderScanlineMixAsm:
	tst.l	d7
	beq.b	.skip
	movem.l d1-a6,-(sp)

	lea	.shadetab,a3
	moveq.l	#0,d0
	moveq.l	#0,d1
	moveq.l	#0,d4
	move.l	#16,d5

	move.w	(a2)+,d0
	move.w	(a2)+,d1

	ror.l	#1,d7
	sub.w	#1,d7
	blt.b	.skip2

	move.b	(a6),d4
	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	lea	(a3,d4.l),a4
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	bra.b	.st2

	cnop	0,16
.loop2:
	move.b	(a6),d4
	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	lea	(a3,d4.l),a4
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	move.b	(a4,d3.l),(a6)+
.st2:	lsr.l	d5,d2
	move.b	(a6),d4
	move.l	(a1,d1.l*4),d3
	sub.l	(a1,d0.l*4),d3
	add.l	(a0,d0.l*4),d3
	sub.l	(a0,d1.l*4),d3
	lea	(a3,d4.l),a4
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d3
	move.b	(a4,d2.l),(a6)+
	lsr.l	d5,d3
	dbf	d7,.loop2

	move.b	(a6),d4
	lea	(a3,d4.l),a4
	move.b	(a4,d3.l),(a6)+

.skip2:

	rol.l	#1,d7
	sub.w	#1,d7
	blt.b	.skip1
.loop1:
	move.b	(a6),d4
	move.l	(a1,d1.l*4),d2
	sub.l	(a1,d0.l*4),d2
	add.l	(a0,d0.l*4),d2
	sub.l	(a0,d1.l*4),d2
	lea	(a3,d4.l),a4
	move.w	(a2)+,d0
	move.w	(a2)+,d1
	mulu.l	d6,d2
	lsr.l	d5,d2
	move.b	(a4,d2.l),(a6)+
	dbf	d7,.loop1

.skip1:
	movem.l (sp)+, d1-a6
.skip:	rts

	cnop	0,16
.shadetab:
.x	set	0
	rept	256
	dc.b	.x
.x	set	.x+1
	endr
	rept	256
	dc.b	255
	endr
	
