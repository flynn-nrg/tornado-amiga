;;; Pretracker C-callable wrapper.
;;; Created by Miguel Mendez in December of 2019.
;;; This file is public domain.

	section text
	public _prtInit
	public  _prtVBL
	public _getPrtVBL

;;; a1 = pointer to Chipmem buffer.
;;; a2 = pointer to Pretracker module.

_prtInit:
	movem.l d0-a6, -(sp)
	movem.l a1, -(sp)
	lea     player(pc),a6
        lea     myPlayer,a0
        lea     mySong,a1
;;; lea     song0,a2
        add.l   (0,a6),a6
        jsr     (a6)            ; songInit

        lea     player(pc),a6
        lea     myPlayer,a0
	movem.l (sp)+, a1
;;;     lea     chipmem,a1
        lea     mySong,a2
        add.l   (4,a6),a6
        jsr     (a6)            ; playerInit
	movem.l (sp)+, d0-a6
	rts
	
_getPrtVBL:
        lea	_prtVBL, a0
        move.l	a0,d0
        rts

_prtVBL:
	movem.l d1-a6, -(sp)
	lea     player(pc),a6
        lea     myPlayer,a0
	lea     mySong,a2
        add.l   (8,a6),a6
        jsr     (a6)            ; playerTick
	movem.l (sp)+, d1-a6
	rts

player:	incbin  "../../third_party/aYS_PreTracker_1_00/amiga_replayer/player.bin"

        section bss,bss
mySong  ds.w    16*1024/2
myPlayer        ds.l    16*1024/4
