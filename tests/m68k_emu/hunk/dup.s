; Defines _helper again: loading it after mod_b.s must fail.
	xdef	_helper
	section	code,code
_helper:
	rts
