Getting Started
============

This framework only works on Mac OS X and GNU/Linux. It might work on Windows using the [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), but this hasn't been tested.

Before you can use Tornado there's a few things you need to setup:

* A working [clang](https://clang.llvm.org/). Mac OS X uses clang by default and it's available in every GNU/Linux distribution as a package.
* [SDL2](https://www.libsdl.org/). This is used by the posix/SDL target and it's a dependency for ImGui and ImGuiSDL.
* [BASS](https://www.un4seen.com/). BASS is used by the posix/SDL target to play the music.
* A working [vbcc](http://sun.hasenbraten.de/vbcc/index.php?view=main) setup.

I suggest that you build your vbcc toolchain and add it to your repo. This makes everyone use the same set of tools.

Building the compiler
----------------------------

There's a set of shell scripts in the scripts directory that will build the toolchain for you. 

Let's create a directory for the toolchain and copy the scripts there:

```
mmendez$ mkdir toolchain
mmendez$ cd toolchain
mmendez$ curl http://sun.hasenbraten.de/vasm/release/vasm.tar.gz | tar xzvf -
[...]
mmendez$ curl http://sun.hasenbraten.de/vlink/release/vlink.tar.gz | tar xzvf -
[...]
mmendez$ curl https://server.owl.de/~frank/tags/vbcc0_9fP1.tar.gz | tar xzvf -
[...]
```

Now we build the toolchain (use the scripts from the scripts directory):

```
mmendez$ mkdir bin-osx (mkdir bin for GNU/Linux users)
mmendez$ sh 01-vasm-osx.sh (01-vasm-linux.sh for GNU/Linux users.)
[...]
mmendez$ sh 02-vlink-osx.sh (02-vlink-linux.sh for GNU/Linux users.)
[...]
mmendez$ sh 03-vbcc-osx.sh (03-vbcc-linux.sh for GNU/Linux users.)
(Accept all the defaults)
```

Verify that you have the relevant binaries:

```
mmendez$ ls -la bin-osx/ (bin for GNU/Linux users.)
total 4680
drwxr-xr-x   9 mmendez  staff     288  2 Jun 15:48 .
drwxr-xr-x  15 mmendez  staff     480  2 Jun 15:47 ..
-rwxr-xr-x   1 mmendez  staff  394572  2 Jun 15:47 vasmm68k_mot
-rwxr-xr-x   1 mmendez  staff  829172  2 Jun 15:48 vbccm68k
-rwxr-xr-x   1 mmendez  staff  829204  2 Jun 15:48 vbccm68ks
-rwxr-xr-x   1 mmendez  staff   26216  2 Jun 15:48 vc
-rwxr-xr-x   1 mmendez  staff  272648  2 Jun 15:47 vlink
-rwxr-xr-x   1 mmendez  staff   17508  2 Jun 15:47 vobjdump
-rwxr-xr-x   1 mmendez  staff    9636  2 Jun 15:48 vprof
```

Now let's add the config and targets:

```
mmendez$ curl https://server.owl.de/~frank/vbcc/2017-08-14/vbcc_target_m68k-amigaos.lha -o vbcc_target_m68k-amigaos.lha
[...]
mmendez$ lha x vbcc_target_m68k-amigaos.lha 
[...]
mmendez$ cd vbcc_target_m68k-amigaos
mmendez$ mv targets ..
mmendez$ cd ..
mmendez$ curl https://server.owl.de/~frank/vbcc/2017-08-14/vbcc_unix_config.tar.gz | tar xzvf - 
[...]
```

In the scripts directory you will also find two examples that show how to setup the environment: 

```
mmendez$ ls go-vbcc-*
go-vbcc-linux.sh	go-vbcc-osx.sh
```

Or you can set the environment variables yourself. I want the executable file to be created in the same directory where the ```Makefile``` is and I'm pointing the ```TOOLCHAIN``` variable to
my recently built toolchain. I'm also adding the ```VBCC``` binaries to my ```PATH``` and telling Tornado that we have an OS X host.


```
mmendez$ export SHARED=.
mmendez$ export TOOLCHAIN=`pwd`
mmendez$ echo $TOOLCHAIN
/Users/mmendez/Amiga/toolchain
mmendez$ export OSX_HOST=true
mmendez$ export PATH=${TOOLCHAIN}/bin-osx:${PATH}
```

On GNU/Linux systems you would instead export ```LINUX_HOST``` and use ```bin``` instead of ```bin-osx``` in the ```PATH```.


You can either execute those scripts (adapted to your setup) before you start working or permanently set those environment variables in your ```.bashrc``` file.

Adding the external dependencies
----------------------------------------------

You need to initialise and update the submodules so that they get populated:

```
mmendez$ git submodule init
Submodule 'tornado2/third_party/imgui' (https://github.com/ocornut/imgui.git) registered for path './'
Submodule 'tornado2/third_party/imgui_sdl' (https://github.com/Tyyppi77/imgui_sdl.git) registered for path '../imgui_sdl'
Submodule 'tornado2/third_party/rocket' (https://github.com/rocket/rocket.git) registered for path '../rocket'
mmendez$ git submodule update
Cloning into '/Users/mmendez/Amiga/ClassicWB_UAE_v28/HardDisk/Devel/amiga-demo/tornado2/third_party/imgui'...
Cloning into '/Users/mmendez/Amiga/ClassicWB_UAE_v28/HardDisk/Devel/amiga-demo/tornado2/third_party/imgui_sdl'...
Cloning into '/Users/mmendez/Amiga/ClassicWB_UAE_v28/HardDisk/Devel/amiga-demo/tornado2/third_party/rocket'...
Submodule path './': checked out '00b3c830db849551a26dbaccf0cfc8bb2e7fa2b9'
Submodule path '../imgui_sdl': checked out '4c69d9a5dac35eb7b2550dcbb32e7d0ed323230b'
Submodule path '../rocket': checked out '901db86412a0d57600cb072c16deac9c3ebc709d'
```

Adding the NDK
----------------------

Download the [Amiga NDK](http://www.haage-partner.de/download/AmigaOS/NDK39.lha) and unpack it. Move the contents of the ```include/include_h``` and  ```include/include_i``` directories to the ```third_party/ndk``` directory. You should end up with a directory structure like this:

```
mmendez$ cd third_party/ndk
mmendez$ ls
classes		devices		exec		hardware	libraries	prefs		resources	workbench
clib		diskfont	gadgets		images		pragma		proto		rexx
datatypes	dos		graphics	intuition	pragmas		reaction	utility
```

Installing BASS
---------------

Download [BASS](https://www.un4seen.com) for OSX and GNU/Linux and unpack it in the ```third_party``` directory.

```
mmendez$ ls -la
total 24
drwxr-xr-x  12 mmendez  staff    384 19 May 16:28 .
drwxr-xr-x  13 mmendez  staff    416 22 Jun 16:57 ..
-rw-r--r--@  1 mmendez  staff  10244 27 Apr 12:15 .DS_Store
drwx------@ 28 mmendez  staff    896 27 Apr 12:15 bass24-linux
drwx------@ 29 mmendez  staff    928 27 Apr 12:15 bass24-osx
[...]
```

**IMPORTANT**: You need both if you use Mac OSX because Tornado will pick up the include files from the bass24-linux directory.

Let's do a quick test:

```
mmendez$ cd ../tornado2/examples/simple_screen/
mmendez$ make clean all
rm -rf /tmp/build-amiga ./simple_screen.68k
/Users/mmendez/Amiga/toolchain/bin-osx/vbccm68k -I../../include -I../../third_party -I../../third_party/rocket/lib -I../../tools/compress -I./src -Iinclude -I../../third_party/ndk -I/Users/mmendez/Amiga/toolchain/targets/m68k-amigaos/include  -DNDEBUG -D__DEBUG_CODE -quiet               -c99                 -O=5279              -no-alias-opt           -no-delayed-popping  -inline-size=100     -inline-depth=10     -cpu=68060           -fpu=68060           -no-intz             -D__AMIGA__ -D__VBCC__ ../../src/amiga/startup.c "-o="/tmp/build-amiga/amiga/startup.s
/Users/mmendez/Amiga/toolchain/bin-osx/vasmm68k_mot -I../../include -I../../third_party -I../../third_party/rocket/lib -I../../tools/compress -I./src -Iinclude -I../../third_party/ndk -I/Users/mmendez/Amiga/toolchain/targets/m68k-amigaos/include  -quiet       -Fhunk       -align       -phxass      -x           -noesc       -nosym       -m68060      -opt-allbra  -opt-fconst  -opt-lsl     -opt-movem   -opt-mul     -opt-div     -opt-pea     -opt-speed   -opt-st      -D__AMIGA__ -D__VASM__ -o /tmp/build-amiga/amiga/startup.o /tmp/build-amiga/amiga/startup.s
[...]
/Users/mmendez/Amiga/toolchain/bin-osx/vlink /Users/mmendez/Amiga/toolchain/targets/m68k-amigaos/lib/startup.o -Llib -L/Users/mmendez/Amiga/toolchain/targets/m68k-amigaos/lib /tmp/build-amiga/amiga/startup.o /tmp/build-amiga/amiga/cpu.o /tmp/build-amiga/amiga/aga.o /tmp/build-amiga/amiga/assets.o /tmp/build-amiga/hardware_check.o /tmp/build-amiga/amiga/graphics.o /tmp/build-amiga/memory.o /tmp/build-amiga/amiga/system.o /tmp/build-amiga/amiga/copper.o /tmp/build-amiga/debug.o /tmp/build-amiga/amiga/c2p1x1_8_c5.o /tmp/build-amiga/amiga/c2p1x1_8_c5_040_16_9.o /tmp/build-amiga/amiga/c2p32.o /tmp/build-amiga/amiga/c2p64.o /tmp/build-amiga/amiga/c2p1x1_6_c5_040.o /tmp/build-amiga/amiga/c2p1x1_8_c5_040.o /tmp/build-amiga/amiga/c2p1x1_4_c5_16_9.o /tmp/build-amiga/amiga/audio.o /tmp/build-amiga/amiga/paula_output.o /tmp/build-amiga/amiga/audio_lowlevel.o /tmp/build-amiga/wav_delta.o /tmp/build-amiga/tndo.o /tmp/build-amiga/amiga/splash.o /tmp/build-amiga/amiga/c2p1x1_8_c5_bm.o /tmp/build-amiga/amiga/mod_replay.o /tmp/build-amiga/amiga/c2p1x1_4_c5_16_9_h.o /tmp/build-amiga/amiga/chrono.o /tmp/build-amiga/amiga/freq_estimation.o /tmp/build-amiga/caps_loader.o /tmp/build-amiga/de-encapsulator.o /tmp/build-amiga/telemetry.o /tmp/build-amiga/prof.o /tmp/build-amiga/lzw_loader.o /tmp/build-amiga/amiga/lzw_unpack.o /tmp/build-amiga/amiga/lzw_unpack_inner.o /tmp/build-amiga/amiga/time.o /tmp/build-amiga/amiga/mod_replay_os_legal.o /tmp/build-amiga/lzss_loader.o /tmp/build-amiga/lzh_loader.o /tmp/build-amiga/amiga/lzss_unpack.o /tmp/build-amiga/amiga/lzh_unpack.o /tmp/build-amiga/amiga/tndo_assert.o /tmp/build-amiga/tndo_file.o /tmp/build-amiga/amiga/display.o /tmp/build-amiga/dprint.o /tmp/build-amiga/demo.o /tmp/build-amiga/simple_screen/simple_screen.o -Bstatic                    -bamigahunk                 -x                          -Cvbcc                      -nostdlib                   -lm060                      -lamiga                     -lvc                        -lauto                      -o simple_screen.68k
mmendez$ ls -la simple_screen.68k 
-rwxr-xr-x  1 mmendez  staff  109376 22 Jun 16:05 simple_screen.68k
```

We're ready to go!