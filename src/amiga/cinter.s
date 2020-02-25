;;; C-callabler Cinter wrapper.
;;; Created by Miguel Mendez.
;;; This file is public domain.

	machine 68060

	section text
	public _CinterInit
	public _CinterPlay1
	public _CinterPlay2

CINTER_MANUAL_DMA = 0

	include "Cinter4.S"
	
;;; A2 = Music data
;;; A0 = Instrument data
;;; D0 = Intrument data size
;;; A6 = Cinter working memory
_CinterInit:
	movem.l d0-a6, -(sp)
	lea InstrumentSpace, a4
	cmp.l  #0, d0
	beq    .noinst
	;; Copy instrument data to instrument space.
	move.l a4, a1
	lsr.l #1, d0
.copy:	move.w (a0)+, (a1)+
	subq.l #1, d0
	bgt.b .copy
.noinst:
	lea CinterSpace, a6
	bsr CinterInit
	movem.l (sp)+, d0-a6
	rts

;;; A6 = Cinter working memory
_CinterPlay1:
	movem.l d0-a6, -(sp)
	lea CinterSpace, a6
	bsr CinterPlay1
	movem.l (sp)+, d0-a6
	rts

;;; A6 = Cinter working memory
_CinterPlay2:
	movem.l d0-a6, -(sp)
	lea CinterSpace, a6
	bsr CinterPlay1
	movem.l (sp)+, d0-a6
	rts

CinterSpace:
        ds.b    c_SIZE


        section inst,bss_c
InstrumentSpace:
        ds.b    200000
