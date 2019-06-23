Debugging
========

All large programs have bugs. It is highly likely that your demo will, too. Thankfully Tornado offers a few debugging facilities.

Note that it is always easier to debug code on the posix/SDL side than it is on the Amiga. Sometimes, however, there's no other option.

I strongly recommend that you implement most of your demo in C so that you can run the same code on both platforms and only re-write things in 68060 assembler once you are sure everything is ok.

Clang's sanitisers
-----------------------
Tornado builds the posix version of your demo with [memory](https://clang.llvm.org/docs/MemorySanitizer.html) and [address](https://clang.llvm.org/docs/AddressSanitizer.html) sanitisers enabled. You can see them in action in the ```buffer_overflow``` example. If you run this code on Amiga your effect will happily write past the end of the allocated buffer. On posix this will be caught by the sanitiser:

```
=================================================================
==3817==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x632000014810 at pc 0x000102e86ecf bp 0x7ffeed22f110 sp 0x7ffeed22f108
WRITE of size 1 at 0x632000014810 thread T0
    #0 0x102e86ece in renderBufferOverflow buffer_overflow.c:97
    #1 0x102e85a16 in demoMain demo.c:370
    #2 0x1029e6720 in main startup.c:206
    #3 0x7fff7db653d4 in start (libdyld.dylib:x86_64+0x163d4)
```

If you look at line 97 of ```buffer_overflow.c``` you will see the offending memory write. A close inspection of the for loop will reveal the issue.

Asserts
----------

The assert macro is a simple way to enforce certain preconditions and trigger an abort if they're not met. Tornado provides an assert implementation that will work both on posix and Amiga environments, regardless of whether Amiga OS is running or not.

It is compatible with the standard assert and used in the same way:

```c
#include "tndo_assert.h"

tndo_assert(strlen(path) < TNDO_VFS_MAX_PATH_LEN);
```

If this assertion fails, the error will be sent to the serial port on Amiga and to stderr on posix. The program will be aborted at this point.

The error will include the file and line where the failed assertion happened.

Logging
-----------
Sometimes good old printf logging is enough to troubleshoot a bug. During the initialisation phase you can of course make use of fprintf and friends and they will all work as expected.

When the demo is running this is no longer an option as Amiga OS has been shut down. You can log data to the serial port using the serialLog function:

```c
#include <debug.h>

char buffer[255];

snprintf(buffer, 255, "Flux capacitor failure. Value is %i\n", variable);
serialLog(buffer);
```

On posix ```serialLog``` is remapped to ```stderr```. If you use [fs-uae](https://fs-uae.net) or a similar emulator you can redirect the Amiga's serial port to a tcp port by adding this to your config file:

```
serial_port = tcp://127.0.0.1:1234
```

And then just run ```telnet localhost 1234``` in another terminal.

Enforcer
--------

[Enforcer](http://www.sinz.org/Michael.Sinz/Enforcer/index.html) works very well on current versions of [fs-uae](https://fs-uae.net/) and similar emulators and should be part of your tool repertoire.

Tornado does not have any ```Enforcer``` hits and neither should your code. It's useful to leave ```Enforcer``` running while you develop and test your code.


Running your code under a debugger
--------------------------------------------------

The posix/SDL version of your demo is compiled with debug symbols and can be executed under your favourite debugger. On Amiga you can also use fs-uae's built-in debugger. This is a last resort option and I've only ever used it once in 2 years.
