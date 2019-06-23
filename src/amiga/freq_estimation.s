

    
    public  _freq_estimation:

    machine 68030

_freq_estimation:

    movem.l	d0-d4,-(sp)

    move.l  #8192,d4    ; 8192 * 16 = 0x20000
time_loop:
    add.l   d0,d1
    add.l   d1,d2
    add.l   d2,d3
    add.l   d3,d0

    add.l   d0,d1
    add.l   d1,d2
    add.l   d2,d3
    add.l   d3,d0

    add.l   d0,d1
    add.l   d1,d2
    add.l   d2,d3
    add.l   d3,d0

    add.l   d0,d1
    add.l   d1,d2
    add.l   d2,d3
    add.l   d3,d0

    dbf d4,time_loop

    movem.l	(sp)+,d0-d4
    rts

