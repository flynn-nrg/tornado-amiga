This is a simple example that will show you how clang's sanitizers can quicky help you identify bugs in your code.

This program has a bug: When writing the for loop a mistake was made and _y_ was set to go from 0 to 319 instead of 255. 

When running on Amiga the program will happily overwrite memory past the buffer's end.
Clang's sanitizers, however, will correctly catch the error and immediately abort the program:

```
=================================================================
==3817==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x632000014810 at pc 0x000102e86ecf bp 0x7ffeed22f110 sp 0x7ffeed22f108
WRITE of size 1 at 0x632000014810 thread T0
    #0 0x102e86ece in renderBufferOverflow buffer_overflow.c:97
    #1 0x102e85a16 in demoMain demo.c:370
    #2 0x1029e6720 in main startup.c:206
    #3 0x7fff7db653d4 in start (libdyld.dylib:x86_64+0x163d4)

0x632000014810 is located 0 bytes to the right of 81936-byte region [0x632000000800,0x632000014810)
allocated by thread T0 here:
    #0 0x103522687 in wrap_calloc (libclang_rt.asan_osx_dynamic.dylib:x86_64+0x59687)
    #1 0x102a179cc in tndo_malloc_internal memory.c:332
    #2 0x102a1710f in tndo_malloc_ex memory.c:350
    #3 0x1029e0578 in display_init display.c:96
    #4 0x102e86bca in initBufferOverflow buffer_overflow.c:82
    #5 0x102e84df8 in demoInit demo.c:287
    #6 0x1029e639c in main startup.c:185
    #7 0x7fff7db653d4 in start (libdyld.dylib:x86_64+0x163d4)

SUMMARY: AddressSanitizer: heap-buffer-overflow buffer_overflow.c:97 in renderBufferOverflow
Shadow bytes around the buggy address:
  0x1c64000028b0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x1c64000028c0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x1c64000028d0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x1c64000028e0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x1c64000028f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x1c6400002900: 00 00[fa]fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x1c6400002910: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x1c6400002920: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x1c6400002930: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x1c6400002940: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x1c6400002950: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
  Shadow gap:              cc
==3817==ABORTING
Abort trap: 6
```
