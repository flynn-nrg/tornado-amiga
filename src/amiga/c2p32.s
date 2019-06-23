
; c2p1x1_5_c5_060
;
; 2010-04-26: bugfixed bpl4 output


BPLX	EQU	320
BPLY	EQU	180

BPLSIZE	EQU	BPLX*BPLY/8

    public _c2p1x1_5_c5_060 

	section	code,code

; a0 - c2pscreen
; a1 - bitplanes
; d0 - n. pixels

_c2p1x1_5_c5_060

	movem.l	d2-d7/a2-a6,-(sp)

	add.w	#BPLSIZE*2,a1

	move.l	d0,a2
	add.l	a0,a2
	cmp.l	a0,a2
	beq	.none

	move.l	(a0)+,d0
	move.l	(a0)+,d1
	move.l	(a0)+,d2
	move.l	(a0)+,d3
	move.l	(a0)+,d4
	move.l	(a0)+,d5
	move.l	(a0)+,a5
	move.l	(a0)+,a6

	swap	d4			; Swap 16x4
	swap	d5
	eor.w	d0,d4
	eor.w	d1,d5
	eor.w	d4,d0
	eor.w	d5,d1
	eor.w	d0,d4
	eor.w	d1,d5
	swap	d4
	swap	d5

	exg	d4,a5
	exg	d5,a6

	swap	d4
	swap	d5
	eor.w	d2,d4
	eor.w	d3,d5
	eor.w	d4,d2
	eor.w	d5,d3
	eor.w	d2,d4
	eor.w	d3,d5
	swap	d4
	swap	d5

	exg	d4,a5
	exg	d5,a6

	move.l	d2,d6			; Swap 8x2
	move.l	d3,d7
	lsr.l	#8,d6
	lsr.l	#8,d7
	eor.l	d0,d6
	eor.l	d1,d7
	and.l	#$00ff00ff,d6
	and.l	#$00ff00ff,d7
	eor.l	d6,d0
	eor.l	d7,d1
	lsl.l	#8,d6
	lsl.l	#8,d7
	eor.l	d6,d2
	eor.l	d7,d3

	exg	d2,a5
	exg	d3,a6

	move.l	d2,d6
	move.l	d3,d7
	lsr.l	#8,d6
	lsr.l	#8,d7
	eor.l	d4,d6
	eor.l	d5,d7
	and.l	#$00ff00ff,d6
	and.l	#$00ff00ff,d7

	eor.l	d6,d4
	eor.l	d7,d5
	lsl.l	#8,d6
	lsl.l	#8,d7
	eor.l	d6,d2
	eor.l	d7,d3

	exg	d2,a5
	exg	d3,a6

	move.l	d1,d6
	move.l	d3,d7
	lsr.l	#4,d6
	lsr.l	#4,d7
	eor.l	d0,d6
	eor.l	d2,d7
	and.l	#$0f0f0f0f,d6
	and.l	#$0f0f0f0f,d7
	eor.l	d6,d0
	eor.l	d7,d2
	lsl.l	#4,d6
	lsl.l	#4,d7
	eor.l	d6,d1
	eor.l	d7,d3

	bra	.start1
.x1
	move.l	(a0)+,d0
	move.l	(a0)+,d1
	move.l	(a0)+,d2
	move.l	(a0)+,d3
	move.l	(a0)+,d4
	move.l	(a0)+,d5
	move.l	(a0)+,a5
	move.l	(a0)+,a6

	move.l	d6,(a1)+

	swap	d4			; Swap 16x4
	swap	d5
	eor.w	d0,d4
	eor.w	d1,d5
	eor.w	d4,d0
	eor.w	d5,d1
	eor.w	d0,d4
	eor.w	d1,d5
	swap	d4
	swap	d5

	exg	d4,a5
	exg	d5,a6

	swap	d4
	swap	d5
	eor.w	d2,d4
	eor.w	d3,d5
	eor.w	d4,d2
	eor.w	d5,d3
	eor.w	d2,d4
	eor.w	d3,d5
	swap	d4
	swap	d5

	exg	d4,a5
	exg	d5,a6

	move.l	d7,-BPLSIZE*2-4(a1)

	move.l	d2,d6			; Swap 8x2
	move.l	d3,d7
	lsr.l	#8,d6
	lsr.l	#8,d7
	eor.l	d0,d6
	eor.l	d1,d7
	and.l	#$00ff00ff,d6
	and.l	#$00ff00ff,d7
	eor.l	d6,d0
	eor.l	d7,d1
	lsl.l	#8,d6
	lsl.l	#8,d7
	eor.l	d6,d2
	eor.l	d7,d3

	exg	d2,a5
	exg	d3,a6

	move.l	d2,d6
	move.l	d3,d7
	lsr.l	#8,d6
	lsr.l	#8,d7
	eor.l	d4,d6
	eor.l	d5,d7
	and.l	#$00ff00ff,d6
	and.l	#$00ff00ff,d7

	move.l	a3,BPLSIZE-4(a1)

	eor.l	d6,d4
	eor.l	d7,d5
	lsl.l	#8,d6
	lsl.l	#8,d7
	eor.l	d6,d2
	eor.l	d7,d3

	exg	d2,a5
	exg	d3,a6

	move.l	d1,d6
	move.l	d3,d7
	lsr.l	#4,d6
	lsr.l	#4,d7
	eor.l	d0,d6
	eor.l	d2,d7
	and.l	#$0f0f0f0f,d6
	and.l	#$0f0f0f0f,d7
	eor.l	d6,d0
	eor.l	d7,d2
	lsl.l	#4,d6
	lsl.l	#4,d7
	eor.l	d6,d1
	eor.l	d7,d3

	move.l	a4,-BPLSIZE-4(a1)
.start1
	add.l	d0,d0
	or.l	d2,d0

	exg	d1,a5
	exg	d3,a6

	move.l	d3,d6
	move.l	d5,d7
	lsr.l	#4,d6
	lsr.l	#4,d7
	eor.l	d1,d6
	eor.l	d4,d7
	and.l	#$0f0f0f0f,d6
	and.l	#$0f0f0f0f,d7
	eor.l	d6,d1
	eor.l	d7,d4
	lsl.l	#4,d6
	lsl.l	#4,d7
	eor.l	d6,d3
	eor.l	d7,d5

	add.l	d4,d4
	or.l	d4,d1

	lsl.l	#2,d0
	or.l	d1,d0

	move.l	d0,BPLSIZE*2(a1)

	move.l	a5,d0
	move.l	a6,d1

	move.l	d5,d6			; Swap 2x4
	move.l	d3,d7
	lsr.l	#2,d6
	lsr.l	#2,d7
	eor.l	d0,d6
	eor.l	d1,d7
	and.l	#$33333333,d6
	and.l	#$33333333,d7
	eor.l	d6,d0
	eor.l	d7,d1
	lsl.l	#2,d6
	lsl.l	#2,d7
	eor.l	d6,d5
	eor.l	d7,d3

	move.l	d1,d6			; Swap 1x2
	move.l	d3,d7
	lsr.l	#1,d6
	lsr.l	#1,d7
	eor.l	d0,d6
	eor.l	d5,d7
	and.l	#$55555555,d6
	and.l	#$55555555,d7
	eor.l	d6,d0
	eor.l	d7,d5
	add.l	d6,d6
	add.l	d7,d7
	eor.l	d1,d6
	eor.l	d3,d7

	move.l	d0,a3
	move.l	d5,a4

	cmpa.l	a0,a2
	bne	.x1
.x1end
	move.l	d6,(a1)+
	move.l	d7,-BPLSIZE*2-4(a1)
	move.l	a3,BPLSIZE-4(a1)
	move.l	a4,-BPLSIZE-4(a1)

.none
	movem.l	(sp)+,d2-d7/a2-a6
	rts


