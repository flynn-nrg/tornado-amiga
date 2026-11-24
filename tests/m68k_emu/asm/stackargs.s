; C ABI: int f(int a, int b) returns a + b
	section code,code
	move.l	4(sp),d0
	add.l	8(sp),d0
	rts
