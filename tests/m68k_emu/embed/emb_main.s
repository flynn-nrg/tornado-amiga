; Embedded into the test runner; calls into emb_lib.s.
	xdef	_emb_main
	xref	_emb_magic
	section	code,code
_emb_main:
	bsr	_emb_magic
	addq.l	#1,d0
	rts
