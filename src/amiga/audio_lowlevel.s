
	section text
	public _getMixRoutine16
	public _getMixRoutine8i
	public _getMixRoutine8

_getMixRoutine16:
	lea _mixRoutine16, a0
	move.l a0,d0
	rts

_getMixRoutine8i:
	lea _mixRoutine8i, a0
	move.l a0,d0
	rts

_getMixRoutine8:
	lea _mixRoutine8, a0
	move.l a0,d0
	rts

;;; 8bit Stereo mix routine for non-interleaved samples.
;;; d0 = number of samples to mix
;;; a0, a2 = output samples
;;; a4 = state
;;; a6 = state2
_mixRoutine8:
	movem.l d2-d3/a5-a6,-(sp)
	move.l (a4), a5		; sample ptr
	move.l (a6), a1
	cmp.l   #4, d0
	blt     tail8
word8_loop:
	rept 3
	move.b (a5)+, d2
	move.b (a1)+, d3
	lsl.l #8, d2
	lsl.l #8, d3
	endr
	move.b (a5)+, d2
	move.b (a1)+, d3
	move.l d2, (a0)+
	move.l d3, (a2)+
	subq #4, d0
	cmp     #4,d0
	bge     word8_loop		
tail8:	
	cmp.l   #0, d0
	beq     end8	
tail8_loop:
	move.b (a5)+, d2
	move.b d2, (a0)+
	move.b (a1)+, d2
	move.b d2, (a2)+
	dbf d0, tail8_loop
end8:	
	move.l	a5,(a4)     ; save current smp ptr
	move.l  a1,(a6) 
	movem.l	(sp)+,d2-d3/a5-a6
	rts

;;; 8bit Stereo mix routine for interleaved samples.
;;; d0 = number of samples to mix
;;; a0, a2 = output samples
;;; a4 = state
_mixRoutine8i:
	movem.l d2-d3/a5-a6,-(sp)
	move.l (a4), a5		; sample ptr
	cmp.l   #4, d0
	blt     tail8i
word8i_loop:
	rept 3
	move.b (a5)+, d2
	move.b (a5)+, d3
	lsl.l #8, d2
	lsl.l #8, d3
	endr
	move.b (a5)+, d2
	move.b (a5)+, d3
	move.l d2, (a0)+
	move.l d3, (a2)+
	subq #4, d0
	cmp     #4,d0
	bge     word8i_loop		
tail8i:	
	cmp.l   #0, d0
	beq     end8	
tail8i_loop:
	move.b (a5)+, d2
	move.b d2, (a0)+
	move.b (a5)+, d2
	move.b d2, (a2)+
	dbf d0, tail8i_loop
end8i:	
	move.l	a5,(a4)     ; save current smp ptr
	movem.l	(sp)+,d2-d3/a5-a6
	rts
	
;;; 14bit Stereo mix routine
;;; d0 = number of samples to mix
;;; d1 = current mix position  (??)
;;; a0, a1, a2, a3 = output samples
;;; a4 = state

read_smp    macro
	move.w  (a5)+,d1        ; low : high
	move.b  d1,\1           ; highs
	lsr.w   d2,d1
	move.b  d1,\2           ; lows
	endm

_mixRoutine16:
	movem.l	d2-d6/a5-a6,-(sp)
	move.l	(a4),a5 ; sample ptr
    
	moveq.l #10,d2
	cmp.l   #4, d0
	blt     tail

word_loop:	
	read_smp    d3,d4   ; chann left
	lsl.w   #8,d3
	lsl.w   #8,d4
	read_smp    d5,d6   ; chann right
	lsl.w   #8,d5
	lsl.w   #8,d6

	read_smp    d3,d4
	swap    d3
	swap    d4
	read_smp    d5,d6
	swap    d5
	swap    d6

	read_smp    d3,d4   ; chann left
	lsl.w   #8,d3
	lsl.w   #8,d4
	read_smp    d5,d6   ; chann right
	lsl.w   #8,d5
	lsl.w   #8,d6

	read_smp    d3,d4   ; chann left
	read_smp    d5,d6   ; chann right

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
	move.b	d1,(a0)+        ; high
	lsr.w   d2,d1
	move.b	d1,(a1)+
	
	move.w  (a5)+,d1        ; low : high
	move.b	d1,(a2)+        ; high
	lsr.w	d2,d1
	move.b	d1,(a3)+

	subq.l	#1,d0
	bne.s	tail_loop
end:	
	move.l	a5,(a4)     ; save current smp ptr
	movem.l	(sp)+,d2-d6/a5-a6
	rts


