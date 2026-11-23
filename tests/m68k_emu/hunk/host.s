; Externals bound from the host: functions and data.
	xdef	_call_host
	xdef	_call_unbound
	xdef	_bsr_unbound
	xdef	_read_unbound
	xref	_host_add
	xref	_host_sub
	xref	_host_table
	xref	_not_bound
	xref	_not_bound_data
	section	code,code
; d0 = d0 + 3 + 4 + host_table[0] + host_table[1] + (5 - 7)
_call_host:
	moveq	#3,d1
	jsr	_host_add		; absolute, straight to the thunk
	moveq	#4,d1
	bsr	_host_add		; pc-relative, through the jump slot
	add.l	_host_table,d0		; data, absolute
	lea	_host_table(pc),a0	; data, pc-relative
	add.l	4(a0),d0
	pea	7.w
	pea	5.w
	jsr	_host_sub		; C ABI: stack arguments
	addq.l	#8,sp
	rts
_call_unbound:
	jsr	_not_bound
	rts
_bsr_unbound:
	bsr	_not_bound
	rts
_read_unbound:
	move.l	_not_bound_data,d0
	rts
