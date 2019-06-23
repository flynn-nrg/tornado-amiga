
; A0 - Chun 
; A1 - Fin Chun
; A2 - Bitplane

bplsize	equ	((320/8)*200)

  public  _c2p64

_c2p64

  Movem.l	d0-a6,-(a7)

	Move.l #bplsize*2,D0
	Lea (A2,D0.l),A2	; 3er. bitplane
	Move.l #$33333333,A3
	
	Movem.l (A0)+,D0/D1/D2/D3
	Move.l #$0f0f0f0f,D7	
	Move.l D1,D4
	Lsr.l #4,D4
	Eor.l D0,D4
	And.l D7,D4
	Eor.l D4,D0	; D0- 00,04,01,05,02,06,03,07 - Bpl. 56
	Lsl.l #4,D4
	Eor.l D4,D1	; D1- 00,04,01,05,02,06,03,07 - Bpl. 0123
	Move.l D3,D4
	Lsr.l #4,D4
	Eor.l D2,D4
	And.l D7,D4
	Eor.l D4,D2	; D2- 08,0C,09,0D,0A,0E,0B,0F - Bpl. 56
	Lsl.l #4,D4
	Eor.l D4,D3	; D3- 08,0C,09,0D,0A,0E,0B,0F - Bpl. 0123
	Lsl.l #2,D0
	Or.l D2,D0	; D0- 00,08,04,0C,01,09,05,0D,02,0A,06,0E,03,0B,07,0F - Bpl. 56
	Move.l D0,A6	; ATENCION - Guardar
	Movem.l (A0)+,D0/D4/D5/D6
	Move.l D4,D2
	Lsr.l #4,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	; D0- 10,14,11,15,12,16,13,17 - Bpl. 56
	Lsl.l #4,D2
	Eor.l D2,D4	; D4- 10,14,11,15,12,16,13,17 - Bpl. 0123
	Move.l D6,D2
	Lsr.l #4,D2
	Eor.l D5,D2
	And.l D7,D2
	Eor.l D2,D5	; D5- 18,1C,19,1D,1A,1E,1B,1F - Bpl. 56
	Lsl.l #4,D2
	Eor.l D2,D6	; D6- 18,1C,19,1D,1A,1E,1B,1F - Bpl. 0123
	Lsl.l #2,D0
	Or.l D5,D0	; D0- 10,18,14,1C,11,19,15,1D,12,1A,16,1E,13,1B,17,1F - Bpl. 56
	Move.l A6,D5
	Swap D5
	Move.w D0,D2	
	Move.w D5,D0
	Move.w D2,D5	; 16
	Swap D0		
	Move.l A3,D7
	Move.l D5,D2
	Lsr.l #2,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	
	Lsl.l #2,D2
	Eor.l D2,D5	; 2
	Move.l #$00ff00ff,D7
	Move.l D5,D2
	Lsr.l #8,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	
	Lsl.l #8,D2
	Eor.l D2,D5	; 8
	Move.l D5,D2
	Lsr.l #1,D2
	Eor.l D0,D2
	And.l #$55555555,D2
	Eor.l D2,D0	; D0- bpl 6
	Lsl.l #1,D2
	Eor.l D2,D5	; D5- bpl 5	
	Move.l D0,bplsize*3(A2)
	Move.l D5,bplsize*2(A2)
	Move.l D1,D2	; 1er. paso - 16 bits
	Swap D2
	Move.w D4,D2	; D2 - 26372637
	Swap D4
	Move.w D4,D1	; D1 - 04150415	
	Move.l D3,D4
	Swap D4
	Move.w D6,D4	; D4 - AEBFAEBF
	Swap D6
	Move.w D6,D3	; D3 - 8C9D8C9D
	Move.l  D3,D6	; 2o. paso - 8 bits
	Lsr.l   #8,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 048C048C 
	Lsl.l   #8,D6
	Eor.l   D6,D3	; D3 - 159D159D
	Move.l  D4,D6
	Lsr.l   #8,D6
	Eor.l   D2,D6
	And.l   D7,D6
	Eor.l   D6,D2	; D2 - 26AE26AE 
	Lsl.l   #8,D6
	Eor.l   D6,D4	; D4 - 37BF37BF
	Move.l A3,D7
	Move.l  D2,D6	; 3er. paso - 2 bits
	Lsr.l   #2,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 0101010..
	Lsl.l   #2,D6
	Eor.l   D6,D2	; D2 - 232323... 
	Move.l  D4,D6
	Lsr.l   #2,D6
	Eor.l   D3,D6
	And.l   D7,D6
	Eor.l   D6,D3	; D3 - 0101010.. 
	Lsl.l   #2,D6
	Eor.l   D6,D4	; D4 - 2323232...
	Move.l #$55555555,D7
	Move.l  D3,D6	; 4o. paso - 1 bit
	Lsr.l   #1,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 33333...
	Lsl.l   #1,D6
	Eor.l   D6,D3	; D3 - 22222...
	Move.l D1,bplsize(A2)
	Move.l  D4,D6
	Lsr.l   #1,D6
	Eor.l   D2,D6
	And.l   D7,D6
	Eor.l   D6,D2	; D2 - 11111.... 
	Lsl.l   #1,D6
	Eor.l   D6,D4	; D4 - 00000....

	Move.l D2,A4
	Move.l D3,A5

  	
BucC2P:
	Movem.l (A0)+,D0/D1/D2/D3

	Move.l D4,-bplsize*2(A2)	; DESCARGA

	Move.l #$0f0f0f0f,D7
	
	Move.l D1,D4
	Lsr.l #4,D4
	Eor.l D0,D4
	And.l D7,D4
	Eor.l D4,D0	; D0- 00,04,01,05,02,06,03,07 - Bpl. 56
	Lsl.l #4,D4
	Eor.l D4,D1	; D1- 00,04,01,05,02,06,03,07 - Bpl. 0123

	Move.l D3,D4
	Lsr.l #4,D4
	Eor.l D2,D4
	And.l D7,D4
	Eor.l D4,D2	; D2- 08,0C,09,0D,0A,0E,0B,0F - Bpl. 56
	Lsl.l #4,D4
	Eor.l D4,D3	; D3- 08,0C,09,0D,0A,0E,0B,0F - Bpl. 0123
	
	Lsl.l #2,D0
	Or.l D2,D0	; D0- 00,08,04,0C,01,09,05,0D,02,0A,06,0E,03,0B,07,0F - Bpl. 56
	Move.l D0,A6	; ATENCION - Guardar
	
	Movem.l (A0)+,D0/D4/D5/D6
	
	Move.l A4,-bplsize(A2)	; DESCARGA

	Move.l D4,D2
	Lsr.l #4,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	; D0- 10,14,11,15,12,16,13,17 - Bpl. 56
	Lsl.l #4,D2
	Eor.l D2,D4	; D4- 10,14,11,15,12,16,13,17 - Bpl. 0123

	Move.l D6,D2
	Lsr.l #4,D2
	Eor.l D5,D2
	And.l D7,D2
	Eor.l D2,D5	; D5- 18,1C,19,1D,1A,1E,1B,1F - Bpl. 56
	Lsl.l #4,D2
	Eor.l D2,D6	; D6- 18,1C,19,1D,1A,1E,1B,1F - Bpl. 0123
	
	Lsl.l #2,D0
	Or.l D5,D0	; D0- 10,18,14,1C,11,19,15,1D,12,1A,16,1E,13,1B,17,1F - Bpl. 56

	Move.l A6,D5
	Swap D5
	Move.w D0,D2	
	Move.w D5,D0
	Move.w D2,D5	; 16
	Swap D0		

	Move.l A5,(A2)+		; DESCARGA

	Move.l A3,D7
	Move.l D5,D2
	Lsr.l #2,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	
	Lsl.l #2,D2
	Eor.l D2,D5	; 2

	Move.l #$00ff00ff,D7

	Move.l D5,D2
	Lsr.l #8,D2
	Eor.l D0,D2
	And.l D7,D2
	Eor.l D2,D0	
	Lsl.l #8,D2
	Eor.l D2,D5	; 8

	Move.l D5,D2
	Lsr.l #1,D2
	Eor.l D0,D2
	And.l #$55555555,D2
	Eor.l D2,D0	; D0- bpl 6
	Move.l D0,bplsize*3(A2)
	Add.l D2,D2
	Eor.l D2,D5	; D5- bpl 5
				
	Move.l D1,D2	; 1er. paso - 16 bits
	Swap D2
	Move.w D4,D2	; D2 - 26372637
	Swap D4
	Move.w D4,D1	; D1 - 04150415	
	Move.l D3,D4
	Swap D4
	Move.w D6,D4	; D4 - AEBFAEBF
	Swap D6
	Move.w D6,D3	; D3 - 8C9D8C9D

	Move.l  D3,D6	; 2o. paso - 8 bits
	Lsr.l   #8,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 048C048C 
	Lsl.l   #8,D6
	Eor.l   D6,D3	; D3 - 159D159D
	
	Move.l  D4,D6
	Lsr.l   #8,D6
	Eor.l   D2,D6
	And.l   D7,D6
	Eor.l   D6,D2	; D2 - 26AE26AE 
	Lsl.l   #8,D6
	Eor.l   D6,D4	; D4 - 37BF37BF

	Move.l D5,bplsize*2(A2)

	Move.l A3,D7
	Move.l  D2,D6	; 3er. paso - 2 bits
	Lsr.l   #2,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 0101010..
	Lsl.l   #2,D6
	Eor.l   D6,D2	; D2 - 232323... 

	Move.l  D4,D6
	Lsr.l   #2,D6
	Eor.l   D3,D6
	And.l   D7,D6
	Eor.l   D6,D3	; D3 - 0101010.. 
	Lsl.l   #2,D6
	Eor.l   D6,D4	; D4 - 2323232...

	Move.l #$55555555,D7
	Move.l  D3,D6	; 4o. paso - 1 bit
	Lsr.l   #1,D6
	Eor.l   D1,D6
	And.l   D7,D6
	Eor.l   D6,D1	; D1 - 33333...
	Move.l D1,bplsize(A2)		; DESCARGA
	Add.l   D6,D6
	Eor.l   D6,D3	; D3 - 22222...
	
	Move.l  D4,D6
	Lsr.l   #1,D6
	Eor.l   D2,D6
	And.l   D7,D6
	Eor.l   D6,D2	; D2 - 11111.... 
	Add.l   D6,D6
	Eor.l   D6,D4	; D4 - 00000....
	
	Move.l D2,A4
	Move.l D3,A5
	  
	Cmp.l A0,A1
	Bne BucC2P
	
	Move.l D4,-bplsize*2(A2)	; DESCARGA
	Move.l A4,-bplsize(A2)		; DESCARGA
	Move.l A5,(A2)+			; DESCARGA

  Movem.l	(a7)+,d0-a6
	Rts

