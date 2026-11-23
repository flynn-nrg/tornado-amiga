; Absolute and pc-relative references between code, data and chip bss.
	xdef	_reloc_sum
	xdef	_crash
	section	code,code
_reloc_sum:
	lea	table,a0		; RELOC32 code -> data
	moveq	#0,d0
.loop:	move.l	(a0)+,d1
	beq.s	.done
	add.l	d1,d0
	bra.s	.loop
.done:	lea	table2(pc),a1		; RELOC16: pc-relative code -> data
	add.l	(a1),d0
	lea	chipbuf,a0		; RELOC32 code -> chip bss
	move.l	d0,(a0)
	move.l	codeptr,d1		; data holds a pointer back to code
	rts

_crash:	bsr.s	inner_fault
	rts
inner_fault:				; not exported: only in the SYMBOL hunk
	clr.l	(a0)
	rts

	section	data,data
table:	dc.l	1,2,3,4,0
table2:	dc.l	100
codeptr: dc.l	_reloc_sum		; RELOC32 data -> code

	section	chipbss,bss_c
chipbuf: ds.l	4
