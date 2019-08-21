;;; Optimised DDPCM frame decoder.
;;; Version 1.0 August 2019.
;;; Created by Miguel Mendez
;;; This file is public domain.

	section text
	public _unpack6to8_asm


;;; Unpacks 6 groups of 6 bytes containing 6bit deltas to 6 groups of 8 bytes with 8bit values.
;;; a0 = source
;;; a1 = destination

_unpack6to8_asm:
	movem.l d2-d7, -(sp)

	;; 6 times for a full frame.
	REPT 6
	
	;; unpacked1 = (((uint32_t)*src) >> 2) << 24
	;; unpacked1 |= (((uint32_t)*src++) & 0x3) << 20
	moveq.l #0, d0
	moveq.l #0, d1

	move.b (a0), d0
	move.b (a0)+, d1

	lsr.l #2, d0
	and.b #$3, d1

	moveq #24, d6
	moveq #20, d7

	lsl.l d6, d0	
	lsl.l d7, d1
	
	or.l d1, d0

	;; unpacked1 |= (((uint32_t)*src) >> 4) << 16
	;; unpacked1 |= (((uint32_t)*src++) & 0xf) << 10;	
	moveq.l #0, d2
	moveq.l #0, d3
	
	move.b (a0), d2
	move.b (a0)+, d3

	lsr.l #4, d2
	and.b #$f, d3

	moveq #16, d6
	moveq #10, d7

	lsl.l d6,  d2
	lsl.l d7, d3
	
	or.l d2, d0
	or.l d3, d0

	;; unpacked1 |= (((uint32_t)*src) >> 6) << 8;
	;; unpacked1 |= (((uint32_t)*src++) & 0x3f);
	moveq.l #0, d4
	moveq.l #0, d5

	move.b (a0), d4
	move.b (a0)+, d5
	
	lsr.l #6, d4
	and.b #$3f, d5

	lsl.l #8, d4
	or.l d5, d0

	or.l d4, d0


	move.l d0, (a1)+
	
	;; unpacked2 = (((uint32_t)*src) >> 2) << 24;
	;; unpacked2 |= (((uint32_t)*src++) & 0x3) << 20;	
	moveq.l #0, d6
	moveq.l #0, d1
	
	move.b (a0), d6
	move.b (a0)+, d1

	lsr.l #2, d6
	and.b #$3, d1

	moveq #24, d0
	moveq #20, d7

	lsl.l d0, d6
	lsl.l d7, d1
	
	or.l d1, d6

	;; unpacked2 |= (((uint32_t)*src) >> 4) << 16;
	;; unpacked2 |= (((uint32_t)*src++) & 0xf) << 10;
	moveq.l #0, d2
	moveq.l #0, d3
	
	move.b (a0), d2
	move.b (a0)+, d3

	lsr.l #4, d2
	and.b #$f, d3

	moveq #16, d0
	moveq #10, d7

	lsl.l d0,  d2
	lsl.l d7, d3

	or.l d2, d6
	or.l d3, d6

	;; unpacked2 |= (((uint32_t)*src) >> 6) << 8;
	;; unpacked2 |= (((uint32_t)*src++) & 0x3f);
	
	moveq.l #0, d4
	moveq.l #0, d5

	move.b (a0), d4
	move.b (a0)+, d5

	lsr.l #6, d4
	and.b #$3f, d5

	lsl.l #8, d4
	or.l d5, d6

	or.l d4, d6

	move.l d6, (a1)+

	ENDR
	
	movem.l (sp)+, d2-d7
	rts
