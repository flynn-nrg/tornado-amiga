	section text
	public _renderPlasma_asm

	;; Same algorithm as renderPlasmaC() in plasma.c:
	;;
	;; pixel(x, y) = S[(2x + t) & 255] + S[(x + y + 3t) & 255]
	;;             + S[(3y - 2t) & 255]
	;;
	;; S is a 256 entry table of words (0..84), so a pixel is 0..252.
	;; The table indices are kept pre-doubled (byte offsets) and masked
	;; with 510 to wrap around.

	;; (a0) unsigned char *chunky, 320x256
	;; (a1) const uint16_t *sine
	;; (d0) int frame
_renderPlasma_asm:
	movem.l	d2-d7,-(sp)
	move.l	d0,d7		; d7 = t
	moveq	#0,d6		; d6 = y
.y:
	;; d5 = S[(3y - 2t) & 255], constant along the row.
	move.l	d6,d1
	add.l	d1,d1
	add.l	d6,d1
	sub.l	d7,d1
	sub.l	d7,d1
	and.w	#255,d1
	add.w	d1,d1
	move.w	(a1,d1.w),d5

	;; d4 = byte offset of S[x + y + 3t] for x = 0.
	move.l	d7,d4
	add.l	d4,d4
	add.l	d7,d4
	add.l	d6,d4
	add.l	d4,d4

	;; d3 = byte offset of S[2x + t] for x = 0.
	move.l	d7,d3
	add.l	d3,d3

	move.w	#320-1,d2
.x:
	move.w	d3,d1
	and.w	#510,d1
	move.w	(a1,d1.w),d0
	move.w	d4,d1
	and.w	#510,d1
	add.w	(a1,d1.w),d0
	add.w	d5,d0
	move.b	d0,(a0)+
	addq.w	#4,d3		; 2x + t moves 2 entries per pixel.
	addq.w	#2,d4		; x + y + 3t moves 1 entry per pixel.
	dbf	d2,.x

	addq.w	#1,d6
	cmp.w	#256,d6
	bne.s	.y

	movem.l	(sp)+,d2-d7
	rts
