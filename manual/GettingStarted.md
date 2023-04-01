Getting Started
============

This framework only works on Mac OS X and GNU/Linux. It might work on Windows using the [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), but this hasn't been tested.

**IMPORTANT**: Clone this repository. Do not download a ZIP file.

Before you can use Tornado there's a few things you need to setup:

* A working [clang](https://clang.llvm.org/) or [gcc](https://gcc.gnu.org/) compiler with [AddressSanitizer](https://en.wikipedia.org/wiki/AddressSanitizer) support. Mac OS X uses clang by default and it's available in every GNU/Linux distribution as a package.
* [SDL2](https://www.libsdl.org/). This is used by the posix/SDL target and it's a dependency for ImGui and ImGuiSDL.
* [SDL_Mixer](https://github.com/libsdl-org/SDL_mixer). Used for audio replay.
* A working [vbcc](http://sun.hasenbraten.de/vbcc/index.php?view=main) setup.

I suggest that you build your vbcc toolchain and add it to your repo. This makes everyone use the same set of tools.

Building the compiler
----------------------------

The current versions are:

* [vbcc 0.9g](http://sun.hasenbraten.de/vbcc/)
* [vasm 1.9a](http://sun.hasenbraten.de/vasm/)
* [vlink 0.16h](http://sun.hasenbraten.de/vlink/)

Tornado is guaranteed to work with these. Please make sure you stay up to date.

There's a set of shell scripts in the scripts directory that will build the toolchain for you.

Let's create a directory for the toolchain and copy the scripts there:

```
mmendez$ mkdir toolchain
mmendez$ cd toolchain
mmendez$ curl http://sun.hasenbraten.de/vasm/release/vasm.tar.gz | tar xzvf -
[...]
mmendez$ curl http://sun.hasenbraten.de/vlink/release/vlink.tar.gz | tar xzvf -
[...]
mmendez$ curl https://server.owl.de/~frank/tags/vbcc0_9g.tar.gz | tar xzvf -
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

Download the [Amiga NDK](http://aminet.net/dev/misc/NDK3.2.lha) and unpack it inside of the ```third_party/ndk``` directory. You should end up with a directory structure like this:

```
mmendez$ cd third_party/ndk
mmendez$ tree -d .
.
├── autodocs
│   └── ag
├── dacontrol+trackfile
│   ├── dacontrol
│   └── trackfile
│       └── goodies
├── developerdocumentation
│   └── memorypools
├── examples
│   ├── arexx
│   ├── backfill
│   └── bitmap
├── fd
[...]    
```

Local dependencies
----------------------

If you're on a MacOS machine, use brew to install SDL2, SDL_Mixer and pkg-config:

```
brew install pkg-config sdl2 sdl2_mixer
```

On GNU/Linux systems, use your package manager to do the same.

Testing that everything is setup correctly
-------------------------------------------

Let's do a quick test:

```
mmendez$ cd examples/simple_screen
mmendez$ make clean all
(CC) -> /tmp/build-amiga/amiga/startup.o
(AS) -> /tmp/build-amiga/amiga/cpu.o
(AS) -> /tmp/build-amiga/amiga/aga.o
[...]
(CC) -> /tmp/build-amiga/demo.o
>#warning "Building with debugging and profiling enabled!"
warning 325 in line 71 of "src/demo.c": #warning "Building with debugging and profiling enabled!"
(CC) -> /tmp/build-amiga/simple_screen/simple_screen.o
(LD) -> simple_screen.68k
mmendez$ ls -la simple_screen.68k
-rwxr-xr-x@ 1 mmendez  staff  263532  1 Apr 16:19 simple_screen.68k
```

And now let's build the posix version of the zoom effect:

```
mmendez$ cd ../zoom
mmendez$ make -f Makefile_sdl_posix clean all
rm -rf /tmp/build-posix ./zoom.elf
(CC) -> /tmp/build-posix/assets.o
(CC) -> /tmp/build-posix/sdl_posix/display.o
[...]
mmendez$ ls -la zoom.elf
-rwxr-xr-x@ 1 mmendez  staff  11864549  1 Apr 16:21 zoom.elf
```

We're ready to go!
