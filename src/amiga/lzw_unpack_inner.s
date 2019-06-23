;;; Optimised tndo lzw decoder inner loop for mc68020+ cpus.
;;; Created by Miguel Mendez.
;;; This file is public domain.
;;; Version 1.0 December 2018.
	
	machine 68040

	section text
	public _lzw_unpack_inner_12
	public _lzw_unpack_inner_16

;;; (a0) uint8_t *compressed
;;; (a1) uint8_t *dest,
;;; (a2) uint8_t **symbols,
;;; (a3) uint8_t *lengths,
;;; (d2) uint32_t clear,
;;; (d3) uint32_t stop);
_lzw_unpack_inner_12:
	movem.l d7/a4, -(sp)
	moveq.l #0, d1		; nibbleFlag = 0;
_loop_12:
	moveq.l #0, d0
	move.b (a0)+, d0	; d = ((uint16_t) data[consumed++]) << 8;
	lsl.l #8, d0
	move.b (a0), d0		; d |= (uint16_t) data[consumed];
	cmp.b #0, d1		; if(!nibbleFlag)
	bne _nibble_is_1
	lsr.l #4, d0		; d = d >> 4;
	moveq.l #1, d1		; nibbleFlag = 1;
	bra.s _consume_done_12
_nibble_is_1:
	and.l #$fff, d0		; d = d & 0xfff;
	moveq.l #0, d1		; nibbleFlag = 0;
	addq.l #1, a0		; consumed++;
_consume_done_12:
	cmp.l d3, d0		; if(code == LZW_STOP) break;
	beq.s _unpack_done_12
	cmp.l d2, d0		; if(code < LZW_CLEAR)
	bgt _string_12
	move.b d0, (a1)+	; dstBuffer[produced++] = (uint8_t) code;
	bra.s _loop_12
_string_12:
	moveq.l #0, d7
	move.l (a2, d0.l*4), a4	; a4 = symbols[code]
	move.b (a3, d0.l), d7	; d7 = lengths[code]
 	cmp.l #16, d7 		; memcpy(&dstBuffer[produced], symbols[code], lengths[code]);
	blt _loop1_12
_loop16_12:
	move.l (a4)+, (a1)+	; Source memory is not 16byte-aligned, so we can't use move16.
	move.l (a4)+, (a1)+	; Since it might fall on odd addresses this makes this code '020+.
	move.l (a4)+, (a1)+
	move.l (a4)+, (a1)+
	sub.l #16, d7
	cmp #16, d7
	bgt _loop16_12
_tail_12:
	tst.l d7
	ble _copy_done_12
_loop1_12:	
	move.b (a4)+, (a1)+
	subq #1, d7
	bne _loop1_12
_copy_done_12:
	bra _loop_12
_unpack_done_12:
	movem.l (sp)+, d7/a4
	rts
	
;;; (a0) uint8_t *compressed
;;; (a1) uint8_t *dest,
;;; (a2) uint8_t **symbols,
;;; (a3) uint8_t *lengths,
;;; (d2) uint32_t clear,
;;; (d3) uint32_t stop);
_lzw_unpack_inner_16:
	movem.l d7/a4, -(sp)
_loop_16:
	moveq.l #0, d0
	move.w (a0)+, d0	; d = ((uint16_t) data[consumed++]);
	cmp.l d3, d0		; if(code == LZW_STOP) break;
	beq.s _unpack_done_16
	cmp.l d2, d0		; if(code < LZW_CLEAR)
	bgt _string_16
	move.b d0, (a1)+	; dstBuffer[produced++] = (uint8_t) code;
	bra.s _loop_16
_string_16:
	moveq.l #0, d7
	move.l (a2, d0.l*4), a4	; a4 = symbols[code]
	move.b (a3, d0.l), d7	; d7 = lengths[code]
 	cmp.l #16, d7 		; memcpy(&dstBuffer[produced], symbols[code], lengths[code]);
	blt _loop1_16
_loop16_16:
	move.l (a4)+, (a1)+	; Source memory is not 16byte-aligned, so we can't use move16.
	move.l (a4)+, (a1)+	; Since it might fall on odd addresses this makes this code '020+.
	move.l (a4)+, (a1)+
	move.l (a4)+, (a1)+
	sub.l #16, d7
	cmp #16, d7
	bgt _loop16_16
_tail_16:
	tst.l d7
	ble _copy_done_16
_loop1_16:	
	move.b (a4)+, (a1)+
	subq #1, d7
	bne _loop1_16
_copy_done_16:
	bra _loop_16
_unpack_done_16:
	movem.l (sp)+, d7/a4
	rts
