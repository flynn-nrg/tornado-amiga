;;; Optimised DDPCM frame decoder.
;;; Version 1.0 August 2019.
;;; Created by Miguel Mendez
;;; This file is public domain.

	machine 68060
	fpu     1

	section text
	public _decodeFrame_asm
	public _initDDPCM_Decoder
	public _getDDPCMMixRoutine16
	public _ddpcmAHIPlayerFunc
	
INT16_MAX equ 32767
INT16_MIN equ -32768
DDPCM_FRAME_NUMSAMPLES equ 50
DDPCM_COMPRESSED_FRAME_SIZE equ 40
WORK_BUFFERS_SIZE equ 8192
	

	cnop 0,4
;;; decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale)
;;; a0 = uint8_t *src
;;; a6 = int16_t *dst
;;; a5 = int16_t *q_table
;;; d7 = uint8_t scale
_decodeFrame_asm:
	fmovem.x  _fp_st,-(sp)
	movem.l d0-d7/a0-a6, -(sp)
	
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
	moveq.l #DDPCM_FRAME_NUMSAMPLES, d6
_decodeLoop:
	move.l d7, d3
	lsl.l #1, d3
	lea (a6, d3.l), a2
	;; y1 = dst[i - 2];
	move.w (-4, a2), d0
	;; y2 = dst[i - 1];
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
	cmp.l #INT16_MAX, d0
	ble   _p1
	move.w  #INT16_MAX/2, d0
	bra.s _p2
_p1:	cmp.l   #INT16_MIN, d0
	bge     _p2
	move.w  #INT16_MIN/2, d0
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
	
	addq.l #1, d7	       ; i++
	
	;; dst[i] = p + (int16_t)floorf(fdelta)
	move.w d0, (a2)

	cmp.l d7, d6
	bhi   _decodeLoop
	
	movem.l (sp)+,d0-d7/a0-a6
_fp_st  freg    fp0-fp2
        fmovem.x        (sp)+,_fp_st
	rts

_getDDPCMMixRoutine16:
	lea _ddpcmMixRoutine16, a0
	move.l a0,d0
	rts
	
;;; 14bit Stereo mix routine for DDPCM streams.
;;; d0 = number of samples to mix
;;; a0, a1, a2, a3 = output samples
	
read_smp    macro
	move.w  (\1)+,d1        ; low : high
	lsr.w   #8, d1
	move.b  d1,\2           ; highs
	lsr.w   d2,d1
	move.b  d1,\3           ; lows
	endm
	
	cnop 0,4 
_ddpcmMixRoutine16:
	movem.l	d2-d6/a5-a6,-(sp)	
	movem.l	a0-a3, -(sp)
	
	lea	output_left, a0
	lea	output_right, a1
	lea	unpacked_samples_left, a2
	lea	unpacked_samples_right, a3
	
	move.l	d0, d1		; Samples to decode
	
	;; Copy any available samples to the output buffer
	move.l	available_samples, d7
	tst.l	d7
	bls	_no_avail
	cmp.l	d7, d1
	bgt	_smaller
	;; We have more unpacked samples than were requested.
	sub.l	d1, d7
	move.l	d7, available_samples
	move.l	unpacked_offset, d2
	add.l	d2, a2	
	add.l	d2, a3
	move.l	d1, d3
	lsl.l	#1, d3
	add.l	d3, d2
	move.l	d2, unpacked_offset
_copy_avail_2:
	move.w	(a2)+, (a0)+
	move.w	(a3)+, (a1)+
	subq.l	#1, d1
	bne	 _copy_avail_2
	bra	_mixer
_smaller:	
	sub.l	d7, d1
	move.l	unpacked_offset, d2
	add.l	d2, a2
	add.l	d2, a3
_copy_avail:
	move.w	(a2)+, (a0)+
	move.w	(a3)+, (a1)+
	subq.l	#1, d7
	bne	 _copy_avail
	
	moveq.l	#0, d2
	move.l	d2, available_samples
	move.l	d2, unpacked_offset
_no_avail:
	;; Unpack the remaining samples needed
	moveq.l	#0, d3		; number of samples we need to unpack
	moveq.l	#0, d4		; number of frames we need to unpack
	
	tst.l	d0
	beq	_have_enough
_not_enough:
	add.l	#DDPCM_FRAME_NUMSAMPLES, d3
	addq.l 	#1, d4
	cmp.l	d3, d0
	bhi	_not_enough
_have_enough:
	;; d0 -> samples to mix
	;; d1 -> samples to decode
	;; d4 -> frames to decode
	;; a0 -> output_left
	;; a1 -> output_right
	;; a2 -> unpacked_samples_left
	;; a3 -> unpacked_samples_right
	
	sub.l   d1, d3
	move.l  d3, available_samples 
	move.l  frames_done, d2
_decode_frames:
	;; left channel
	move.l  scales_left, a4
	move.b  (a4, d2.l), d7	; d7 -> scale

	move.l	frame_in_qtable, d3
	move.l	frames_per_qtable, d5
	move.l	current_qtable, d6
	cmp.l	d5, d3
	bcs	_inside_qtable
	moveq 	#0, d3
	move.l	d3, frame_in_qtable
	addq.l	#1, d6
	move.l	d6, current_qtable
_inside_qtable:
	;; d7 -> scale
	;; d6 -> current qtable

	move.l 	q_tables_left, a4
	move.l  (a4, d6.l*4), a5	; a5 -> qtable

	move.l  a0, a4		; a4 -> output_left
	lea	unpacked_samples_left, a6		; a6 -> dst
	move.l  unpacked_offset, d3
	add.l	d3, a6
	move.l  packed_left, a0
	move.l  packed_offset, d3
	add.l   d3, a0		; a0 -> src

	jsr	_decodeFrame_asm
	
	move.l   a4, a0		; a0 -> output_left
	
	;; right channel
	move.l  scales_right, a4
	move.l  frames_done, d2
	move.b  (a4, d2.l), d7	; d7 -> scale

	move.l	q_tables_right, a4
	move.l  (a4, d6.l*4), a5	; a5 -> qtable

	lea  	unpacked_samples_right, a6		; a6 -> dst
	move.l  unpacked_offset, d3
	add.l   d3, a6
	move.l  a0, a4
	move.l  packed_right, a0
	move.l  packed_offset, d3
	add.l   d3, a0		; a0 -> src

	jsr	_decodeFrame_asm

	move.l  a4, a0
	
	move.l  packed_offset, d3
	add.l	#DDPCM_COMPRESSED_FRAME_SIZE, d3
	move.l	d3, packed_offset
	addq.l	#1, d2
	move.l	d2, frames_done

	move.l  frame_in_qtable, d3
	addq.l	#1, d3
	move.l	d3, frame_in_qtable

	move.l  unpacked_offset, d3
	add.l	#DDPCM_FRAME_NUMSAMPLES*2, d3
	move.l 	d3, unpacked_offset
	subq.l	#1, d4
	bne	_decode_frames
	
	;; copy new unpacked samples to output buffer
	lea	unpacked_samples_left, a4
	lea	unpacked_samples_right, a5
	move.l	d1, d7
_copy_output:	
	move.w	(a4)+, (a0)+
	move.w	(a5)+, (a1)+
	subq.l	#1, d7
	bne.s	_copy_output
	
	move.l	unpacked_offset, d3
	move.l	available_samples, d4
	lsl.l	#1, d4
	sub.l	d4, d3
 	move.l	d3, unpacked_offset

_mixer:
	;; Mix and copy to chip buffers
	movem.l (sp)+, a0-a3
	
	lea  output_left, a5 ; left buffer
	lea  output_right, a4  ; left buffer
	moveq.l #10,d2
	cmp.l   #4, d0
	blt     tail

word_loop:	
	read_smp    a5, d3,d4   ; chann left
	lsl.w   #8,d3
	lsl.w   #8,d4
	read_smp    a4, d5,d6   ; chann right
	lsl.w   #8,d5
	lsl.w   #8,d6

	read_smp    a5, d3,d4
	swap    d3
	swap    d4
	read_smp    a4, d5,d6
	swap    d5
	swap    d6

	read_smp    a5, d3,d4   ; chann left
	lsl.w   #8,d3
	lsl.w   #8,d4
	read_smp    a4, d5,d6   ; chann right
	lsl.w   #8,d5
	lsl.w   #8,d6

	read_smp    a5, d3,d4   ; chann left
	read_smp    a4, d5,d6   ; chann right

	move.l	d3,(a0)+ 
	move.l	d4,(a1)+ 
	move.l	d5,(a2)+ 
	move.l	d6,(a3)+ 

	subq.l  #4,d0
	cmp     #4,d0
	bge     word_loop

tail:	
	cmp.l   #0, d0
	beq     end

tail_loop:	
	move.w  (a5)+,d1        ; low : high
	lsr.w   #8, d1
	move.b	d1,(a0)+        ; high
	lsr.w   d2,d1
	move.b	d1,(a1)+
	
	move.w  (a4)+,d1        ; low : high
	lsr.w   #8, d1
	move.b	d1,(a2)+        ; high
	lsr.w	d2,d1
	move.b	d1,(a3)+

	subq.l	#1,d0
	bne.s	tail_loop
end:
	movem.l	(sp)+,d2-d6/a5-a6
	rts

;;; AHI 16 bit Stereo mix routine for DDPCM streams.
;;; d0 = number of samples to mix
;;; a0, a1 = output samples
	
	cnop 0,4
_ddpcmAHIPlayerFunc:
	movem.l	d2-d6/a2-a6,-(sp)	
	movem.l	a0-a1, -(sp)
	
	lea	output_left, a0
	lea	output_right, a1
	lea	unpacked_samples_left, a2
	lea	unpacked_samples_right, a3
	
	move.l	d0, d1		; Samples to decode
	
	;; Copy any available samples to the output buffer
	move.l	available_samples, d7
	tst.l	d7
	bls	_no_avail_ahi
	cmp.l	d7, d1
	bgt	_smaller_ahi
	;; We have more unpacked samples than were requested.
	sub.l	d1, d7
	move.l	d7, available_samples
	move.l	unpacked_offset, d2
	add.l	d2, a2	
	add.l	d2, a3
	move.l	d1, d3
	lsl.l	#1, d3
	add.l	d3, d2
	move.l	d2, unpacked_offset
_copy_avail_2_ahi:
	move.w	(a2)+, (a0)+
	move.w	(a3)+, (a1)+
	subq.l	#1, d1
	bne	 _copy_avail_2_ahi
	bra	_mixer_ahi
_smaller_ahi:	
	sub.l	d7, d1
	move.l	unpacked_offset, d2
	add.l	d2, a2
	add.l	d2, a3
_copy_avail_ahi:
	move.w	(a2)+, (a0)+
	move.w	(a3)+, (a1)+
	subq.l	#1, d7
	bne	 _copy_avail_ahi
	
	moveq.l	#0, d2
	move.l	d2, available_samples
	move.l	d2, unpacked_offset
_no_avail_ahi:
	;; Unpack the remaining samples needed
	moveq.l	#0, d3		; number of samples we need to unpack
	moveq.l	#0, d4		; number of frames we need to unpack
	
	tst.l	d0
	beq	_have_enough_ahi
_not_enough_ahi:
	add.l	#DDPCM_FRAME_NUMSAMPLES, d3
	addq.l 	#1, d4
	cmp.l	d3, d0
	bhi	_not_enough_ahi
_have_enough_ahi:
	;; d0 -> samples to mix
	;; d1 -> samples to decode
	;; d4 -> frames to decode
	;; a0 -> output_left
	;; a1 -> output_right
	;; a2 -> unpacked_samples_left
	;; a3 -> unpacked_samples_right
	
	sub.l   d1, d3
	move.l  d3, available_samples 
	move.l  frames_done, d2
_decode_frames_ahi:
	;; left channel
	move.l  scales_left, a4
	move.b  (a4, d2.l), d7	; d7 -> scale

	move.l	frame_in_qtable, d3
	move.l	frames_per_qtable, d5
	move.l	current_qtable, d6
	cmp.l	d5, d3
	bcs	_inside_qtable_ahi
	moveq 	#0, d3
	move.l	d3, frame_in_qtable
	addq.l	#1, d6
	move.l	d6, current_qtable
_inside_qtable_ahi:
	;; d7 -> scale
	;; d6 -> current qtable

	move.l 	q_tables_left, a4
	move.l  (a4, d6.l*4), a5	; a5 -> qtable

	move.l  a0, a4		; a4 -> output_left
	lea	unpacked_samples_left, a6		; a6 -> dst
	move.l  unpacked_offset, d3
	add.l	d3, a6
	move.l  packed_left, a0
	move.l  packed_offset, d3
	add.l   d3, a0		; a0 -> src

	jsr	_decodeFrame_asm
	
	move.l   a4, a0		; a0 -> output_left
	
	;; right channel
	move.l  scales_right, a4
	move.l  frames_done, d2
	move.b  (a4, d2.l), d7	; d7 -> scale

	move.l	q_tables_right, a4
	move.l  (a4, d6.l*4), a5	; a5 -> qtable

	lea  	unpacked_samples_right, a6		; a6 -> dst
	move.l  unpacked_offset, d3
	add.l   d3, a6
	move.l  a0, a4
	move.l  packed_right, a0
	move.l  packed_offset, d3
	add.l   d3, a0		; a0 -> src

	jsr	_decodeFrame_asm

	move.l  a4, a0
	
	move.l  packed_offset, d3
	add.l	#DDPCM_COMPRESSED_FRAME_SIZE, d3
	move.l	d3, packed_offset
	addq.l	#1, d2
	move.l	d2, frames_done

	move.l  frame_in_qtable, d3
	addq.l	#1, d3
	move.l	d3, frame_in_qtable

	move.l  unpacked_offset, d3
	add.l	#DDPCM_FRAME_NUMSAMPLES*2, d3
	move.l 	d3, unpacked_offset
	subq.l	#1, d4
	bne	_decode_frames_ahi
	
	;; copy new unpacked samples to output buffer
	lea	unpacked_samples_left, a4
	lea	unpacked_samples_right, a5
	move.l	d1, d7
_copy_output_ahi:	
	move.w	(a4)+, (a0)+
	move.w	(a5)+, (a1)+
	subq.l	#1, d7
	bne.s	_copy_output_ahi
	
	move.l	unpacked_offset, d3
	move.l	available_samples, d4
	lsl.l	#1, d4
	sub.l	d4, d3
 	move.l	d3, unpacked_offset

_mixer_ahi:
	;; Mix and copy to AHI buffers
	movem.l (sp)+, a0-a1
	lea  output_left, a5 ; left buffer
	lea  output_right, a4  ; left buffer
	cmp.l   #4, d0
	blt     tail_ahi

word_loop_ahi:
	move.l	(a5)+,(a0)+ 
	move.l	(a4)+,(a1)+
	move.l	(a5)+,(a0)+ 
	move.l	(a4)+,(a1)+ 
	subq.l  #4,d0
	cmp     #4,d0
	bge     word_loop_ahi

tail_ahi:	
	cmp.l   #0, d0
	beq     end_ahi

tail_loop_ahi:	
	move.b	(a5)+,(a0)+ 
	move.b	(a4)+,(a1)+
	move.b	(a5)+,(a0)+ 
	move.b	(a4)+,(a1)+ 
	subq.l	#1,d0
	bne.s	tail_loop_ahi
end_ahi:
	movem.l	(sp)+,d2-d6/a2-a6
	rts
	
;;; a0 int16_t **qtablesLeft
;;; a1 int16_t **qtablesRight
;;; a2 uint8_t *scalesLeft
;;; a3 uint8_t *scalesRight
;;; a4 uint8_t *left
;;; a5 uint8_t *right
;;; d0 uint32_t numFrames
;;; d1 uint32_t framesPerQTable
_initDDPCM_Decoder:
	move.l	a0, q_tables_left
	move.l	a1, q_tables_right
	move.l	a2, scales_left
	move.l	a3, scales_right
	move.l	a4, packed_left
	move.l	a5, packed_right
	move.l	d0, num_frames
	move.l	d1, frames_per_qtable

	;; Initialise the rest
	moveq	#0, d0
	move.l	d0, available_samples
	move.l	d0, frames_done
	move.l	d0, current_qtable
	move.l	d0, frame_in_qtable
	move.l	d0, packed_offset
	move.l	d0, unpacked_offset
	
	rts
	
	section bss, bss
	cnop 0, 4
	
unpacked_samples_left:
	ds.w WORK_BUFFERS_SIZE

unpacked_samples_right:
	ds.w WORK_BUFFERS_SIZE

samples_to_copy:
	ds.l	1
	
unpacked_offset:
	ds.l	1

output_left:
	ds.w WORK_BUFFERS_SIZE

output_right:
	ds.w WORK_BUFFERS_SIZE

packed_left:
	ds.l 1

packed_right:
	ds.l 1

packed_offset:
	ds.l 1

available_samples:
	ds.l 1

frames_done:
	ds.l 1

num_frames:
	ds.l 1

q_tables_left:
	ds.l 1

q_tables_right:
	ds.l 1

scales_left:
	ds.l 1

scales_right:
	ds.l 1
	
current_qtable:
	ds.l 1

frame_in_qtable:
	ds.l 1
	
frames_per_qtable:
	ds.l 1
	
unpacked_buffer:
	ds.b DDPCM_FRAME_NUMSAMPLES - 2

