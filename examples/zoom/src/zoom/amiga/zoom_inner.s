	section text
	public _renderZoom_asm

	;; typedef struct {
	;; 	int mipMapIdx
	;; 	unsigned char ix[320]
	;; 	int iy[180]
	;; 	int iTileX[320]
	;; 	int iTileY[180]
	;; } zoomIter

mipMapIdx EQU 0
ix EQU 4
iy EQU ix + 320
iTileX EQU iy + 180*4
iTileY EQU iTileX + 320*4
 	
	;; (a0) unsigned char ***allTxtPtr
	;; (a1) unsigned char *chunky
	;; (a2) zoomIter *iteration
_renderZoom_asm:
	movem.l d1-a6, -(sp)

	;; txtPtr = allTxtPtr[iteration->mipMapIdx]
	move.l (a2), d0
	move.l (a0, d0.w*4), a3	; a3 = txtPtr
	
	;; iyPtr = iteration->iy
	move.l #iy, d0
	lea (a2, d0.w), a4	; a4 = iyPtr
	
	;; tileyPtr = iteration->iTileY
	move #iTileY, d0
	lea (a2, d0.w), a5	; a5 = tileyPtr
	
	;; for (int y = 0, y < 180, y++) {
	move.l #179, d7
	moveq #4, d6		; d6 = ixPtr offset
_y:	
	;; 	iy = *iyPtr++
	move.l (a4)+, d1	; d1 = iy
	
	;; 	tileY = *tileyPtr++
	move.l (a5)+, d2      ; d2 = tileY

	;; 	ixPtr = iteration->ix
	lea (a2, d6.l), a6	; a6 = ixPtr
	
	;; 	tilexPtr = iteration->iTileX
	move.l #iTileX, d0
	lea (a2, d0.l), a0     ; a0 = tilexPtr

	move.l a2, d4		; a2 -> d4 = iteration

	;; 	for (int x = 0, x < 320, x++) {
	move.l #(320/8) - 1, d3
_x:	
	REPT 8
	
	;; 		ix = *ixPtr++
	move.b (a6)+, d1      ; d1 = iy + ix
	move.l (a0)+, d0      ; d0 = tileX
	
	;; 		source = txtPtr[ix + iy]
	move.l (a3, d1.l*4), a2	; a2 = source
	
	;; 		tileX = *tilexPtr++
	add.l d2, d0			; d0 = tileY + tileX
	
	;; 		*chunky++ = source[tileX + tileY]
	move.b (a2, d0.l), (a1)+

	;; 	}
	ENDR
	dbf d3, _x 
	move.l d4, a2		; d4 -> a2 = iteration
	;; }
	dbf d7, _y
	
	movem.l (sp)+, d1-a6
	rts
	
