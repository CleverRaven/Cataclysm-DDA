# General Linux Guide

To build Cataclysm from source you will need at least a C++ compiler, some basic developer tools, and necessary build dependencies. The exact package names vary greatly from distro to distro, so this part of the guide is intended to give you higher-level understanding of the process.

## Compiler

You have three major choices here: GCC, Clang and MXE.

  * GCC is almost always the default on Linux systems so it's likely you already have it
  * Clang is usually faster than GCC, so it's worth installing if you plan to keep up with the latest experimentals
  * MXE is a cross-compiler, so of any importance only if you plan to compile for Windows on your Linux machine (not covered here)

(Note that your distro may have separate packages e.g. `gcc` only includes the C compiler and for C++ you'll need to install `g++`.)

Cataclysm is targeting C++11 standard and that means you'll need a compiler that supports it. You can easily check if your version of `g++` supports C++11 by running:

    $ g++ --std=c++11
    g++: fatal error: no input files
    compilation terminated.

If you get a line like:

    g++: error: unrecognized command line option ‘--std=c++11’

This means you'll need a newer version of GCC (`g++`).

The general rule is the newer the compiler the better.

## Tools

Most distros seem to package essential build tools as either a single package (Debian and derivatives have `build-essential`) or a package group (Arch has `base-devel`). You should use the above if available. Otherwise you'll at least need `make` and figure out the missing dependencies as you go.

You should also install `git` and `gettext` (optional if you don't care about localizations).

If you plan on keeping up with experimentals you should also install `ccache`, which  will considerably speed-up partial builds.

## Dependencies

There are some general dependencies, optional dependencies and then specific dependencies for either curses or tiles builds. The exact package names again depend on the distro you're using, and whether your distro packages libraries and their development files separately (e.g. Debian and derivatives).

Rough list based on building on Arch:

  * General: `gcc-libs`, `glibc`, `zlib`, `bzip2`
  * Optional: `lua51`
  * Curses: `ncurses`
  * Tiles: `sdl2`, `sdl2_image`, `sdl2_ttf`, `sdl2_mixer`, `freetype2`

E.g. for curses build on Debian and derivatives you'll also need `libncurses5-dev`.

## Make flags

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

If you have a multi-core computer you'd probably want to add `-jX` to the options, where `X` should roughly be twice the number of cores you have available.

Example: `make -j4 CLANG=1 CCACHE=1 NATIVE=linux64 RELEASE=1 TILES=1`

The above will build a tiles release for 64 bit Linux, using Clang and ccache and 4 parallel processes.

**Note on debug**:
You should probably always build with `RELEASE=1` unless you experience segfaults and are willing to provide stack traces.

# Debian

Instructions for compiling on a Debian-based system. The package names here are valid for Ubuntu 12.10 and may or may not work on your system.

Building instructions, below, always assume you are running them from the Cataclysm:DDA source directory.

## Linux (native) ncurses builds

Dependencies:

  * ncurses
  * build essentials

    sudo apt-get install libncurses5-dev build-essential

### Building

    make

## Cross-compiling to linux 32-bit from linux 64-bit

Dependencies:

  * 32-bit toolchain
  * 32-bit ncurses

    sudo apt-get install libc6-dev-i386 lib32stdc++-dev g++-multilib lib32ncurses5-dev

### Building

    make NATIVE=linux32

## Cross-compile to Windows from Linux

Dependencies:

  * [mxe](http://mxe.cc)

    sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
    mkdir -p ~/src/mxe
    git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
    cd ~/src/mxe
    make gcc glib

### Building

    PATH="${PATH}:~/src/mxe/usr/bin"
    make CROSS=i686-pc-mingw32-

## Linux (native) SDL builds

Dependencies:

  * SDL
  * SDL_ttf
  * freetype
  * build essentials

    sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libfreetype6-dev build-essential

### Building

    make TILES=1

## Cross-compile to Windows SDL from Linux

Dependencies:

  * [mxe](http://mxe.cc)

    sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
    mkdir -p ~/src/mxe
    git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
    cd ~/src/mxe
    make sdl sdl_ttf

### Building

    PATH="${PATH}:~/src/mxe/usr/bin"
    make TILES=1 CROSS=i686-pc-mingw32-

# Mac OS X

To build Cataclysm on Mac you'll have to get XCode with command line tools (or just download them separately from https://developer.apple.com/downloads/) and Homebrew package manager.

## Step 1: Install SDL2

SDL2, SDL2\_image, and SDL2\_ttf are needed for the tiles build.

### Option 1: manual download and install

* [**SDL2 framework**](http://www.libsdl.org/download-1.2.php)
    * Direct download: http://www.libsdl.org/release/SDL2-2.0.3.dmg

* [**SDL2\_image framework**](http://www.libsdl.org/projects/SDL_image/)
    * Direct download: http://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.0.dmg

* [**SDL2\_ttf framework**](http://www.libsdl.org/projects/SDL_ttf/)
    * Direct download: http://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-2.0.12.dmg

Copy `SDL2.framework`, `SDL2_image.framework`, and `SDL2_ttf.framework`
to `/Library/Frameworks` or `/Users/name/Library/Frameworks`.

### Option 2: Using package manager

Alternately, shared libraries (libSDL2, libSDL2\_image, libSDL2\_ttf) can be used
instead of frameworks. Install with a package manager (Fink, MacPorts,
Homebrew, pkgsrc) or build and install from source.

For Homebrew (can also install lua):

    brew install sdl2 sdl2_image sdl2_ttf

For Macports:

    sudo port install libsdl2 libsdl2_image libsdl2_ttf

## Step 2: Install ncurses and gettext

ncurses and gettext are needed for localization.
Install with a package manager, or build from source and install.

**NOTE: ncurses needs wide character support enabled.**

For Homebrew:

    brew tap homebrew/dupes
    brew install gettext ncurses
    brew link --force gettext ncurses

**After you build Cataclysm** you might want to to unlink gettext and ncurses with:

    brew unlink gettext ncurses

Reason: if you build other software, these versions might conflict with what the other software expects.

For Macports:

    # gettext depends on ncurses, so you don't need to specify ncurses explicitly
    # gettext also includes libintl
    sudo port install gettext ncurses
    hash -r

## Step 3: Compile

### Sample compilation commands for current versions of OS X

One of the following commands will likely work for you. Tweak flags to suit your needs.

build a release version, use `SDL` + graphical tiles, don't use `gettext`, and use `Clang` for compilation:

    $ make NATIVE=osx OSX_MIN=10.7 RELEASE=1 TILES=1 LOCALIZE=0 CLANG=1

build a release version, use `SDL` + graphical tiles, link to libraries in the OS X `Frameworks` folders, don't use `gettext`, and use `Clang` for compilation:

    $ make NATIVE=osx OSX_MIN=10.7 RELEASE=1 TILES=1 FRAMEWORK=1 LOCALIZE=0 CLANG=1

build a release version, use curses and gettext supplied by Macports:

    $ make NATIVE=osx OSX_MIN=10.7 RELEASE=1 LOCALIZE=1 MACPORTS=1

### Compile localization files for specified languages

If you just want to compile localization files for specified languages, you can add `LANGUAGES="<lang_id_1>[ lang_id_2][ ...]"` option to make command, e.g.

    $ make LANGUAGES="zh_CN zh_TW"

You can get the language ID from the filenames of `*.po` in `lang/po` directory.

Note: Setting `LOCALIZE=1` may not tell `make` to compile those localization files for you.

### Make Options

Description of the options used above. Tweak until things work. More notes are in the `Makefile`.

* `FRAMEWORK=1` attempt to link to libraries under the OS X `Frameworks` folders; omit to use the usual libsdl, libsdl\_image, libsdl\_ttf (e.g. leave out when you `brew install` the packages).
* `LOCALIZE=0` disable localization (to get around possible `gettext` errors if it is not setup correctly); omit to use `gettext`.
* `LANGUAGES="<lang_id_1>[ lang_id_2][ ...]"` compile localization files for specified languages. e.g. `LANGUAGES="zh_CN zh_TW"`
* `NATIVE=osx` build for OS X.
* `OSX_MIN=version` sets `-mmacosx-version-min=` (for OS X > 10.5 set it to 10.6 or higher); omit for 10.5.
* `RELEASE=1` build an optimized 'release' version; omit for debug build.
* `TILES=1` build the SDL version with graphical tiles (and graphical ASCII); omit to build with `ncurses`.
* `CLANG=1` build with [Clang](http://clang.llvm.org/) to get the necessary support for c++11 without needing to reinstall gcc/g++ (which seems like it might cause havoc with certain compilations of things).
* `TILES=1` build the SDL version with graphical tiles (and graphical ASCII); omit to build with `ncurses`.
* `MACPORTS` build against dependencies installed via Macports, currently only `gettext` and `ncurses`.

## Step 4: Run

    $ ./cataclysm

or

    $ ./cataclysm-tiles

## Step 5 (optional): Application packaging

If you just want to build from source and run the game, this step is not for you.

*The `app` build rule will attempt to automate the instructions listed below.*

Create new folder and name it `Cataclysm.app`.

Put compiled binaries (`./cataclysm-tiles` and/or `./cataclysm`) with `./gfx/` and `./data/` folders inside `/Cataclysm.app/Contents/Resources/`.

To bundle SDL libs copy `SDL2.framework`, `SDL2_image.framework`, and `SDL2_ttf.framework` to `/Cataclysm.app/Contents/Resources/libs/` or shared libs homebrew installed from `/usr/local/Cellar/sdl2*/version/lib/lib*`.

Create folder `/Cataclysm.app/Contents/MacOS` and file ./Cataclysm within it with this content:

    #!/bin/sh
    PWD=`dirname "${0}"`
    OSREV=`uname -r | cut -d. -f1`
    if [ "$OSREV" -ge 11 ] ; then
       export DYLD_LIBRARY_PATH=${PWD}/../Resources/libs
       export DYLD_FRAMEWORK_PATH=${PWD}/../Resources/libs
    else
       export DYLD_FALLBACK_LIBRARY_PATH=${PWD}/../Resources/libs
       export DYLD_FALLBACK_FRAMEWORK_PATH=${PWD}/../Resources/libs
    fi
    cd "${PWD}/../Resources/"; ./cataclysm-tiles

### Creating a DMG

  * Create an new folder named Cataclysm
  * Move your Cataclysm.app into it
  * Start Disk Utility
  * File / New -> Disk Image From Folder
  * Select the Cataclysm folder you created above.

Done!

## Troubleshooting

### ISSUE: crash on startup due to libint.8.dylib aborting

Basically if you're compiling on Mountain Lion or above, it won't be possible to run successfully on older OS X versions due to libint.8 / pthreads version issue.

See below (quoted form https://wiki.gnome.org/GTK+/OSX/Building)

"There's another issue with building on Lion or Mountain Lion using either "native" or the 10.7 SDK: Apple has updated the pthreads implementation to provide recursive locking. This would be good except that Gettext's libintl uses this and if the pthreads implementation doesn't provide it it fabricates its own. Since the Lion pthreads does provide it, libintl links the provided function and then crashes when you try to run it against an older version of the library. The simplest solution is to specify the 10.6 SDK when building on Lion, but that won't work on Mountain Lion, which doesn't include it. See below for how to install and use XCode 3 on Lion and later for building applications compatible with earlier versions of OSX."

Workaround: install XCode 3 like that article describes, or disable localization support in Cataclysm so gettext/libint are not dependencies. Or else simply don't support OS X versions below 10.7.

### ISSUE: Colours don't show up correctly

Open Terminal's preferences, turn on "Use bright colors for bold text" in "Preferences -> Settings -> Text"

