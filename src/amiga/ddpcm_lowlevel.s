;;; Optimised DDPCM frame decoder.
;;; Version 1.0 August 2019.
;;; Created by Miguel Mendez
;;; This file is public domain.

	machine 68060
	fpu     1

	section text
	public _decodeFrame_asm
	
INT16_MAX equ 32767
INT16_MIN equ -32768
DDPCM_FRAME_NUMSAMPLES equ 50
	
;;; decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale)
;;; a0 = uint8_t *src
;;; a6 = int16_t *dst
;;; a5 = int16_t *q_table
;;; d7 = uint8_t scale
_decodeFrame_asm:
	fmovem.x  _fp_st,-(sp)
	movem.l d2-d7/a2-a6, -(sp)
	
	;; int16_t *first = (int16_t *)src;
	;; dst[0] = (int16_t)first[0];
	;; dst[1] = (int16_t)first[1];
	move.l (a0)+, (a6)

	;; float inverseScale = 1.0f / (float)scale;
	fmove.s #$3f800000,fp2
	moveq #0, d0
	move.b d7, d0
	fmove.l d0, fp0
	fdiv.x fp0, fp2		; fp2 = inverseScale

;;; Unpacks 6 groups of 6 bytes containing 6bit deltas to 6 groups of 8 bytes with 8bit values.
;;; a0 = source
;;; a1 = destination

	lea unpacked_buffer, a1

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

	lea unpacked_buffer, a1
	
	;; for (uint32_t i = 2; i < DDPCM_FRAME_NUMSAMPLES; i++)
	moveq.l #2, d7
_decodeLoop:
	move.l d7, d3
	lsl.l #1, d3
	lea (a6, d3.l), a2
	move.w (-4, a2), d0
	move.w (-2, a2), d1
	
;;; Linear extrapolation predictor.
;;; y = y1 + (y2 - y1) * t.
;;; d0 = y1
;;; d1 = y2
	ext.l d0
	ext.l d1
	sub.l d0, d1
	lsl.l #1, d1
	add.l d1, d0
	cmp.l #32767, d0
	ble   _p1
	move.w  #16383, d0
	bra.s _p2
_p1:	cmp.l   #-32768, d0
	bge     _p2
	move.w  #-16384, d0
_p2:
	;; unpacked[i - 2]
	moveq   #0, d2
	move.b  (-2, a1, d7.l), d2
	;; q_table[unpacked[i - 2]]
	lsl.l   #1, d2
	lea (a5, d2.l), a4
	;; ((float)q_table[unpacked[i - 2]]
	fmove.w (a4), fp1
	;; fdelta = ((float)q_table[unpacked[i - 2]] * inverseScale)
	fmul.x fp2, fp1
	;; (int16_t)floorf(fdelta)
	fmove.x fp1, fp0
	fintrz.x fp0
	fcmp.x fp1, fp0
	fbole .skip
	fsub.s #$3f800000,fp0
.skip:
	fmove.w fp0, d3
	;;  p + (int16_t)floorf(fdelta)
	add.w d3, d0
	;; dst[i] = p + (int16_t)floorf(fdelta)
	move.w d0, (a2)

	addq.l #1, d7
	moveq #50, d0
	cmp.l d7, d0
	bhi   _decodeLoop
	
	movem.l (sp)+,d2-d7/a2-a6
_fp_st  freg    fp2
        fmovem.x        (sp)+,_fp_st
	rts
	
	section bss, bss
unpacked_buffer:
	ds.b DDPCM_FRAME_NUMSAMPLES - 2
	
	
