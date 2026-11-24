; Embedded into the test runner, registered by a constructor.
	xdef	_emb_magic
	section	code,code
_emb_magic:
	moveq	#42,d0
	rts
