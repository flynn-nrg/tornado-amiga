	xdef	_helper
	xdef	_helper_data
	section	code,code
_helper:
	add.l	d0,d0
	rts
	section	data,data
_helper_data:
	dc.l	1000
