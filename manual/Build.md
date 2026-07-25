The Build Process
=========

Tornado uses one [Makefile](https://en.wikipedia.org/wiki/Makefile) per target.

The Amiga target uses ```Makefile``` and the SDL/Posix version uses ```Makefile_sdl_posix```.

In some cases you might also want to create a ```Makefile-data``` file to automate the asset processing.

Directory structure
-----------

Tornado expects your demo to be structured this way: ```src/demo.c``` is your demo driver file, as described in [Anatomy Of A Demo](AnatomyOfADemo.md). Then each effect should be in its own directory, with the Amiga and SDL specific code in their own subdirectories.

Let's look at the ```zoom``` example and how it uses these files to build the executable and data.

Makefile
-----

```
#################################################################################
# Tornado root directory
#################################################################################
TORNADO_BASE = ../..

#################################################################################
# Your demo code comes here
#################################################################################

LOCAL_INCDIR  = ./src 

# Demo objects
DEMO_OBJS += demo.o

# Zoom
DEMO_OBJS += zoom/zoom.o
DEMO_OBJS += zoom/zoom_util.o
DEMO_OBJS += zoom/amiga/zoom_inner.o

# Save all object and source files to tmp.
BUILDDIR = /tmp/build-amiga

# Remove asserts while compiling.
CCFLAGS += -DNDEBUG

TARGET = ${SHARED}/zoom.68k

# Remove for final release
CCFLAGS += -D__DEBUG_CODE

# Tndo malloc debug.
#CCFLAGS += -DTNDO_MEMORY_DEBUG

#################################################################################
# Do NOT add anything below this line!!!
#################################################################################
include $(TORNADO_BASE)/tornado.mk
```

After declaring where Tornado resides, we have the object files that comprise our demo. You can see that there's an Amiga-specific inner loop file.

Then we set the build flags which configure various debugging options.

Last but not least we include the ```tornado.mk``` file which describes all the framework dependencies and the rules to build your project.

To build the Amiga version of this example we simply type ```make```:

```
mmendez$ make
/opt/homebrew/opt/vbcc/bin/vbccm68k -I../../include -I../../third_party -I/Users/mmendez/devel/rocket/lib -I../../third_party/m68k-amigaos-ahi/Developer/Include/C -I../../tools/compress -I../../tools/ddpcm -I../../third_party/Cinter/player -I./src -Iinclude -I/opt/homebrew/opt/vbcc/targets/m68k-amigaos/include -I../../third_party/ndk/Include_H -I../../third_party/ndk/Include_I -I../../third_party/ndk/ -DNDEBUG -D__DEBUG_CODE -DTORNADO_ASSET_MANAGER -DTORNADO_GRAPHICS -DTORNADO_SPLASH -DTORNADO_AHI -DTORNADO_P61 -DTORNADO_DDPCM -quiet -c99 -O=5279 -no-alias-opt -no-delayed-popping -inline-size=100 -inline-depth=10 -cpu=68060 -fpu=68060 -no-intz -D__AMIGA__ -D__VBCC__ ../../src/amiga/startup.c "-o="/tmp/build-amiga/amiga/startup.s
[...]
/opt/homebrew/opt/vlink/bin/vlink /opt/homebrew/opt/vbcc/targets/m68k-amigaos/lib/startup.o -Llib -L/opt/homebrew/opt/vbcc/targets/m68k-amigaos/lib /tmp/build-amiga/amiga/startup.o [...] /tmp/build-amiga/zoom/zoom.o /tmp/build-amiga/zoom/zoom_util.o /tmp/build-amiga/zoom/amiga/zoom_inner.o -Bstatic -bamigahunk -x -Cvbcc -nostdlib -lm060 -lamiga -lvc -lauto -o zoom.68k
mmendez$ file zoom.68k 
zoom.68k: AmigaOS loadseg()ble executable/binary
```

Makefile_sdl_posix
----------

The SDL/Posix target works in a similar way, except that it uses [Clang](https://en.wikipedia.org/wiki/Clang) instead of VBCC and links against the SDL/Posix version of the framework by including ```tornado_sdl_posix.mk```.

```
#################################################################################
# Tornado root directory
#################################################################################
TORNADO_BASE = ../../

#################################################################################
# Your demo code comes here
#################################################################################
LOCAL_INCDIR  = ./src

#Demo objects
DEMO_OBJS += demo.o

# Zoom
DEMO_OBJS += zoom/zoom.o
DEMO_OBJS += zoom/zoom_util.o

# Save all object and source files to tmp.
BUILDDIR = /tmp/build-posix

CCFLAGS += -D__DEBUG_CODE

# Tndo malloc debug.
#CCFLAGS += -DTNDO_MEMORY_DEBUG

TARGET = ./zoom.elf

#################################################################################
# Do NOT add anything below this line!!!
#################################################################################
include $(TORNADO_BASE)/tornado_sdl_posix.mk
```

Let's build the SDL/Posix version:

```
mmendez$ make -f Makefile_sdl_posix
cc -I../..//include -I../..//tools/compress -I../..//tools/ddpcm -I/Users/mmendez/devel/rocket/lib -I/Users/mmendez/devel/imgui -I/Users/mmendez/devel/imgui/backends -I./src -Iinclude -I../../libs -D__DEBUG_CODE -c --std=c99 -O0 -g -fno-omit-frame-pointer -Wall -Wfatal-errors -Wno-deprecated -Wno-unused-variable -Wno-unused-function -Wno-missing-braces -DUSE_GETADDRINFO -fsanitize=address -fsanitize=undefined [SDL3 / SDL2_mixer cflags from pkg-config ...] ../..//src/assets.c -o /tmp/build-posix/assets.o
[...]
cc [...] -I/Users/mmendez/devel/rocket/lib [...] /Users/mmendez/devel/rocket/lib/device.c -o /tmp/build-posix/device.o
c++ [...] -I/Users/mmendez/devel/imgui [...] --std=c++14 /Users/mmendez/devel/imgui/imgui.cpp -o /tmp/build-posix/imgui.o
[...]
cc [...] src/zoom/zoom.c -o /tmp/build-posix/zoom/zoom.o
cc [...] src/zoom/zoom_util.c -o /tmp/build-posix/zoom/zoom_util.o
c++ /tmp/build-posix/assets.o /tmp/build-posix/sdl_posix/display.o /tmp/build-posix/sdl_posix/startup.o [...] /tmp/build-posix/device.o /tmp/build-posix/track.o /tmp/build-posix/tcp.o /tmp/build-posix/imgui_impl_sdl3.o /tmp/build-posix/imgui_impl_sdlrenderer3.o /tmp/build-posix/imgui.o /tmp/build-posix/imgui_demo.o /tmp/build-posix/imgui_draw.o /tmp/build-posix/imgui_widgets.o /tmp/build-posix/imgui_tables.o /tmp/build-posix/demo.o /tmp/build-posix/zoom/zoom.o /tmp/build-posix/zoom/zoom_util.o -lc -lm -fno-omit-frame-pointer -L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib -lSDL3 -L/opt/homebrew/Cellar/sdl2_mixer/2.8.2/lib -lSDL2_mixer -L/opt/homebrew/lib -lSDL2main -lSDL2 -Wl,-framework,Cocoa -fsanitize=address -fsanitize=undefined -o zoom.elf
mmendez$ file zoom.elf 
zoom.elf: Mach-O 64-bit executable arm64
```

In this example we only have one effect, but if we want to have more all we have to do is add them like we did with the zoom effect. 

Makefile-data
--------

When working on large projects it is convenient to automate asset processing, as it can become quite a tedious and repetitive task.

The zoom example ships with such a file. The original graphics are stored in the ```srcdata/zoom``` directory and ```Makefile-data``` can be used to transform all the assets. It will also take care of building the required tools.

```
TORNADO_BASE = ../..
TARGETS += zoom_data

.PHONY: clean all


all: $(TARGETS)

clean: 
	rm -rf $(TARGETS)


# Zoom assets.
ZOOM_DATA = data/zoom.tndo data/zoom2.tndo data/zoom_frame0.tndo data/zoom.pal

zoom_data: $(ZOOM_DATA)

data/zoom.tndo: /tmp/tndo_compress srcdata/zoom/zoom_*.raw
		cat srcdata/zoom/zoom_*.raw > /tmp/zoom
		/tmp/tndo_compress -b -v -i /tmp/zoom -o $@
		rm /tmp/zoom

data/zoom2.tndo: /tmp/tndo_compress srcdata/zoom/zoom2_*.raw
		cat srcdata/zoom/zoom2_*.raw > /tmp/zoom2
		/tmp/tndo_compress -b -v -i /tmp/zoom2 -o $@
		rm /tmp/zoom2

data/zoom_frame0.tndo: /tmp/tndo_compress /tmp/png2raw srcdata/zoom/zoom_frame0.png
			 /tmp/png2raw -i srcdata/zoom/zoom_frame0.png -o /tmp/zoom_frame0.raw
			 /tmp/tndo_compress -b -v -i /tmp/zoom_frame0.raw -o $@
			 rm /tmp/zoom_frame0.raw
			 
data/zoom.pal:  srcdata/zoom/zoom_pal.png /tmp/png2raw
	       /tmp/png2raw -p -i $< -o $@


# Tools

/tmp/tndo_compress: 
	cd $(TORNADO_BASE)/tools/compress ; make ; mv tndo_compress /tmp/tndo_compress ; make clean

/tmp/png2raw:
	cd $(TORNADO_BASE)/tools/png2raw ; make ; mv png2raw /tmp/png2raw ; make clean
```

To rebuild the assets we simply type ```make -f Makefile-data```.

```
mmendez$ make -f Makefile-data 
cd ../../tools/compress ; make ; mv tndo_compress /tmp/tndo_compress ; make clean
make[1]: Nothing to be done for `all'.
rm -rf tndo_compress.o lzw.o lzss.o lzh.o test_lzw.o lzw.o lzw_unpack.o test_lzh.o lzh.o lzw.o lzss.o lzh_unpack.o lzss_unpack.o test_lzss.o lzss.o lzss_unpack.o tndo_compress test_lzw test_lzss test_lzh
cat srcdata/zoom/zoom_*.raw > /tmp/zoom
/tmp/tndo_compress -b -v -i /tmp/zoom -o data/zoom.tndo
--------------------------
Step 1 - Build dictionary.
--------------------------
Number of codes used: 16384
Maximum symbol length: 209
--------------------------------------------
Step 2 - Compress with pre-built dictionary.
--------------------------------------------
Uncompressed: 1048576, compressed: 713628 (68.1%)
Maximum symbol length: 209
---------------------------------------
[...]
/tmp/png2raw -p -i srcdata/zoom/zoom_pal.png -o data/zoom.pal
Image information: W: 640, H: 360, Bit depth: 4, Colours: 16
Palette size: 200.
mmendez$
```
