	section text, text
	public _checkAGA
	public _waitVBL

VPOSR EQU $dff004

_checkAGA:
	moveq #0, d0
	btst #9, VPOSR
	beq not_aga
is_aa   
	moveq  #1, d0
not_aga 
	rts

_waitVBL:
	move.l	$dff004,d0
	and.l	#$1ff00,d0
	cmp.l	#303<<8,d0
	bne.b	_waitVBL
	rts
