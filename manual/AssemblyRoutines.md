Calling assembly code from C
=======================

It is quite usual to write inner loops in assembly language to get the most performance out of the 68060 CPU.

One such example is the infinite zoom renderer which was first written in C and then manually translated to assembler to maximise register use.

You will first create an include file so that the C compiler knows how to pass the parameters to your routine:

```c
#ifndef INCLUDE_ZOOM_INNER_H
#define INCLUDE_ZOOM_INNER_H

#include "asmparm.h"

void renderZoom_asm(__ASMPARM("a0", unsigned char ***allTxtPtr),
                    __ASMPARM("a1", unsigned char *chunky),
                    __ASMPARM("a2", zoomIter *iteration));

#endif
```

`__ASMPARM` is a convenience macro that correctly formats the parameters for both GCC and VBCC.

Then in your code you will surround this include with an `#ifdef`statement, like this:

```c
#ifdef __AMIGA__
#include "amiga/zoom_inner.h"
#endif
```

The assembler implementation is in the `zoom_inner.s` file.

```
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
```

The three things you should always keep in mind are:

* You need to export the symbols so that the linker can find them. Note how in the assembler file the function is called `_renderZoom_asm` but referred to in the include file as `renderZoom_asm`.
* You need to save the registers in the stack and restore them before returning. D0, D1, A0 and A1 are considered scratch registers and are exempt from this rule.
* If an assembler function returns something, it is always put in register D0 regardless of its type.

Later in your effect code you will call the appropriate implementation:

```c
#ifdef __AMIGA__
  renderZoom_asm(allTxts, chunky, zi);
#else
  renderZoomC(allTxts, chunky, zi);
#endif
```

This code can of course only be tested on an emulator or real hardware, so I strongly recommend that you leave this kind of optimisation until the end and only if you're sure you will gain performance from doing this.
