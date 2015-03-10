# General Linux Guide #

To build Cataclysm from source you will need at least a C++ compiler, some
basic developer tools, and necessary build dependencies. The exact package names
vary greatly from distro to distro, so this part of the guide is intended to give
you higher-level understanding of the process.

## Compiler ##

You have three major choices here: GCC, Clang and MXE.

  * GCC is almost always the default on Linux systems so it's likely you already have it
  * Clang is usually faster than GCC, so it's worth installing if you plan to keep up with the latest experimentals
  * MXE is a cross-compiler, so of any importance only if you plan to compile for Windows on your Linux machine (not covered here)

(Note that your distro may have separate packages e.g. `gcc` only includes the C compiler,
and for C++ you'll need to install `g++`.)

Cataclysm is targeting C++11 standard and that means you'll need a compiler that supports it.
You can easily check if your version of `g++` supports C++11 by running:

```
$ g++ --std=c++11
g++: fatal error: no input files
compilation terminated.
```

If you get a line like:

```
g++: error: unrecognized command line option ‘--std=c++11’
```

This means you'll need a newer version of GCC (`g++`).

The general rule is the newer the compiler the better.

## Tools ##

Most distros seem to package essential build tools as either a single package (Debian and
derivatives have `build-essential`) or a package group (Arch has `base-devel`). You should
use the above if available. Otherwise you'll at least need `make` and figure out the missing
dependencies as you go.

You should also install `git` and `gettext` (optional if you don't care about localizations).

If you plan on keeping up with experimentals you should also install `ccache`, which  will
considerably speed-up partial builds.

## Dependencies ##

There are some general dependencies, optional dependencies and then specific dependencies
for either curses or tiles builds. The exact package names again depend on the distro
you're using, and whether your distro packages libraries and their development files
separately (e.g. Debian and derivatives).

Rough list based on building on Arch:

  * General: `gcc-libs`, `glibc`, `zlib`, `bzip2`
  * Optional: `lua51`
  * Curses: `ncurses`
  * Tiles: `sdl2`, `sdl2_image`, `sdl2_ttf`, `sdl2_mixer`, `freetype2`

E.g. for curses build on Debian and derivatives you'll also need `libncurses5-dev`.

## Make flags ##

Given you're building from source you have a number of choices to make:

  * `NATIVE=` - you should use `linux64` if you're on a 64 bit system, `linux32` for 32 bit systems, and leave it out if you're building on Cygwin
  * `RELEASE=1` - without this you'll get a debug build (see note below)
  * `TILES=1` - if you want the tiles version; note that with this the ncurses version won't be build
  * `SOUND=1` - if you want sound; this requires `TILES=1`
  * `LOCALIZE=0` - this disables localizations so `gettext` is not needed
  * `LUA=1` - this enables Lua *debug* support, may be useful if you plan writing mods
  * `CLANG=1` - use Clang instead of GCC
  * `CACHE=1` - use ccache

There is a couple of other possible options - feel free to read the `Makefile`.

If you have a multi-core computer you'd probably want to add `-jX` to the options,
where `X` should roughly be twice the number of cores you have available.

Example: `make -j4 CLANG=1 CCACHE=1 NATIVE=linux64 RELEASE=1 TILES=1`

The above will build a tiles release for 64 bit Linux, using Clang and ccache and
4 parallel processes.

*Note on debug*:
You should probably always build with `RELEASE=1` unless you experience segfaults and
are willing to provide stack traces.

# Debian #

Instructions for compiling on a Debian-based system. The package names here are
valid for Ubuntu 12.10 and may or may not work on your system.

Building instructions, below, always assume you are running them from the
Cataclysm:DDA source directory.

## Linux (native) ncurses builds ##
Dependencies:
  * ncurses
  * build essentials

```
sudo apt-get install libncurses5-dev build-essential
```

### Building ###
```
make
```

## Cross-compiling to linux 32-bit from linux 64-bit ##
Dependencies:
  * 32-bit toolchain
  * 32-bit ncurses

```
sudo apt-get install libc6-dev-i386 lib32stdc++-dev g++-multilib lib32ncurses5-dev
```

### Building ###
```
make NATIVE=linux32
```

## Cross-compile to Windows from Linux ##
Dependencies:
  * [mxe](http://mxe.cc)

```
sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
mkdir -p ~/src/mxe
git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
cd ~/src/mxe
make gcc glib
```

### Building ###
```
PATH="${PATH}:~/src/mxe/usr/bin"
make CROSS=i686-pc-mingw32-
```

## Linux (native) SDL builds ##
Dependencies:
  * SDL
  * SDL_ttf
  * freetype
  * build essentials

```
sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libfreetype6-dev build-essential
```

### Building ###
```
make TILES=1
```

## Cross-compile to Windows SDL from Linux ##
Dependencies:
  * [mxe](http://mxe.cc)

```
sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
mkdir -p ~/src/mxe
git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
cd ~/src/mxe
make sdl sdl_ttf
```

### Building ###
```
PATH="${PATH}:~/src/mxe/usr/bin"
make TILES=1 CROSS=i686-pc-mingw32-
```
