Getting Started
============

This framework works on macOS and GNU/Linux. It might work on Windows using the [Windows Subsystem for Linux](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux), but this hasn't been tested.

**IMPORTANT**: Clone this repository. Do not download a ZIP file.

Before you can use Tornado there's a few things you need to setup:

* A working [clang](https://clang.llvm.org/) or [gcc](https://gcc.gnu.org/) compiler with [AddressSanitizer](https://en.wikipedia.org/wiki/AddressSanitizer) support. macOS uses clang by default and it's available in every GNU/Linux distribution as a package.
* [SDL2](https://www.libsdl.org/). This is used by the posix/SDL target and it's a dependency for ImGui and ImGuiSDL.
* [SDL_Mixer](https://github.com/libsdl-org/SDL_mixer). Used for audio replay.
* A cross-compiler targeting the Amiga (see below).

Amiga cross-compiler
----------------------------

Tornado supports two cross-compilers for generating Amiga code:

* **m68k-amiga-elf-gcc (GCC 14)** — Recommended. Generates better code overall.
* **VBCC** — The original compiler used by Tornado. Still fully supported.

Both compilers use [vasm](http://sun.hasenbraten.de/vasm/) for assembly and [vlink](http://sun.hasenbraten.de/vlink/) for linking.

The host operating system is detected automatically via `uname`. You do not need to set `OSX_HOST` or `LINUX_HOST` variables.

### Installing the GCC 14 toolchain (recommended)

The easiest way to get a working `m68k-amiga-elf-gcc` toolchain is to install [Bartman's Amiga Debugging VSCode extension](https://marketplace.visualstudio.com/items?itemName=BartmanAbyss.amiga-debug). The extension bundles a complete GCC 14 cross-compiler for m68k. Once installed, add the toolchain's `bin` directory to your `PATH` so that `m68k-amiga-elf-gcc` is accessible from the terminal.


### Installing vasm, vlink and vbcc on macOS

On macOS, install vbcc, vasm and vlink via [Homebrew](https://brew.sh/):

```
brew install vbcc vasm vlink
```

These are required regardless of whether you use GCC or VBCC as your C compiler, since assembly files are always processed by vasm and linking is always done by vlink using VBCC's runtime libraries.

### Installing the GCC, vasm, vlink and vbcc on GNU/Linux from pre-built binary packages

We have created a standalone repository holding pre-built binary packages for GCC 15, vasm, vlink and vbcc.

They are installed in a directory that doesn't conflict with any system-installed toolchain,
therefore you need to add the directories to the PATH in your shell.

1. Enable the repository

```bash
cat <<EOF >/etc/apt/sources.list.d/windenntw-debian-packages.list
deb     [trusted=yes] https://windenntw.github.io/debian-packages windenntw windenntw
deb-src [trusted=yes] https://windenntw.github.io/debian-packages windenntw windenntw
EOF
```

2. Update the package list and do the installation.

```bash
sudo apt update
sudo apt install barto-m68k-amiga-elf-toolchain vasm-vlink-vbcc-m68k-toolchain
```

4. Add the PATH to your shell startup file (eg: .bashrc)

```bash
export PATH=/opt/m68k/barto-m68k-amiga-elf-toolchain/bin:$PATH
export PATH=/opt/m68k/vasm-vlink-vbcc/bin:$PATH
```

### Installing the GCC, vasm, vlink and vbcc on GNU/Linux from source

On GNU/Linux, build vbcc, vasm and vlink from source and place them in a `toolchain` directory. Set the `TOOLCHAIN` environment variable to point to it. See the scripts in the `scripts/` directory for guidance.

Setting up the environment
----------------------------

To build with GCC 14 (recommended), set:

```
export GCC_ELF_HOST=true
export SHARED=.
```

To build with VBCC (default), only set:

```
export SHARED=.
```

The `SHARED` variable controls where the output binary is placed (`.` means the current directory).

You can either set these before you start working or permanently add them to your `.bashrc` or `.zshrc`.

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
│   └── ag
├── dacontrol+trackfile
│   ├── dacontrol
│   └── trackfile
│       └── goodies
├── developerdocumentation
│   └── memorypools
├── examples
│   ├── arexx
│   ├── backfill
│   └── bitmap
├── fd
[...]    
```

Local dependencies
----------------------

If you're on a macOS machine, use Homebrew to install SDL2, SDL_Mixer and pkg-config:

```
brew install pkg-config sdl2 sdl2_mixer
```

On GNU/Linux systems, use your package manager to do the same.

Testing that everything is setup correctly
-------------------------------------------

Let's do a quick test with GCC:

```
mmendez$ cd examples/simple_screen
mmendez$ make clean all
(CC) -> /tmp/build-amiga/amiga/startup.o
(AS) -> /tmp/build-amiga/amiga/cpu.o
(AS) -> /tmp/build-amiga/amiga/aga.o
[...]
(CC) -> /tmp/build-amiga/demo.o
(AS/CHIP) -> /tmp/build-amiga/amiga/paula_output.o
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
