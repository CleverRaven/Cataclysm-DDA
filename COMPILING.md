* [General Linux Guide](#general-linux-guide)
  * [Compiler](#compiler)
  * [Tools](#tools)
  * [Dependencies](#dependencies)
  * [Make flags](#make-flags)
* [Debian](#debian)
  * [Linux (native) ncurses builds](#linux-native-ncurses-builds)
  * [Linux (native) SDL builds](#linux-native-sdl-builds)
  * [Cross-compiling to linux 32-bit from linux 64-bit](#cross-compiling-to-linux-32-bit-from-linux-64-bit)
  * [Cross-compile to Windows from Linux](#cross-compile-to-windows-from-linux)
  * [Cross-compile to Mac OS X from Linux](#cross-compile-to-mac-os-x-from-linux)
  * [Cross-compile to Android from Linux](#cross-compile-to-android-from-linux)
* [Mac OS X](#mac-os-x)
  * [Simple build using Homebrew](#simple-build-using-homebrew)
  * [Advanced info for Developers](#advanced-info-for-developers)
  * [Troubleshooting](#troubleshooting)
* [Windows](#windows)
  * [Visual Studio Guide](#visual-studio-guide)
  * [MinGW Guide](#mingw-guide)
  * [Rough guide to building with only MSYS2](#rough-guide-to-building-with-only-msys2)
* [BSDs](#bsds)

# General Linux Guide

To build Cataclysm from source you will need at least a C++ compiler, some basic developer tools, and necessary build dependencies. The exact package names vary greatly from distro to distro, so this part of the guide is intended to give you higher-level understanding of the process.

## Compiler

You have three major choices here: GCC, Clang and MXE.

  * GCC is almost always the default on Linux systems so it's likely you already have it
  * Clang is usually faster than GCC, so it's worth installing if you plan to keep up with the latest experimentals
  * MXE is a cross-compiler, so of any importance only if you plan to compile for Windows on your Linux machine

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

Most distros seem to package essential build tools as either a single package (Debian and derivatives have `build-essential`) or a package group (Arch has `base-devel`). You should use the above if available. Otherwise you'll at least need `make` and figure out the missing dependencies as you go (if any).

Besides the essentials you will need `git`.

If you plan on keeping up with experimentals you should also install `ccache`, which  will considerably speed-up partial builds.

## Dependencies

There are some general dependencies, optional dependencies and then specific dependencies for either curses or tiles builds. The exact package names again depend on the distro you're using, and whether your distro packages libraries and their development files separately (e.g. Debian and derivatives).

Rough list based on building on Arch:

  * General: `gcc-libs`, `glibc`, `zlib`, `bzip2`
  * Optional: `lua51`, `gettext`
  * Curses: `ncurses`
  * Tiles: `sdl2`, `sdl2_image`, `sdl2_ttf`, `sdl2_mixer`, `freetype2`

E.g. for curses build on Debian and derivatives you'll also need `libncurses5-dev` or `libncursesw5-dev`.

Note on optional dependencies:

  * `gettext` - for localization support; if you plan to only use English you can skip it
  * `lua` - for full-fledged mods; you'll probably prefer to have it

You should be able to figure out what you are missing by reading the compilation errors and/or the output of `ldd` for compiled binaries.

## Make flags

Given you're building from source you have a number of choices to make:

  * `NATIVE=` - you should only care about this if you're cross-compiling
  * `RELEASE=1` - without this you'll get a debug build (see note below)
  * `LTO=1` - enables link-time optimization with GCC/Clang
  * `TILES=1` - with this you'll get the tiles version, without it the curses version
  * `SOUND=1` - if you want sound; this requires `TILES=1`
  * `LOCALIZE=0` - this disables localizations so `gettext` is not needed
  * `LUA=1` - this enables Lua support; needed only for full-fledged mods
  * `CLANG=1` - use Clang instead of GCC
  * `CCACHE=1` - use ccache
  * `USE_LIBCXX=1` - use libc++ instead of libstdc++ with Clang (default on OS X)

There is a couple of other possible options - feel free to read the `Makefile`.

If you have a multi-core computer you'd probably want to add `-jX` to the options, where `X` should roughly be twice the number of cores you have available.

Example: `make -j4 CLANG=1 CCACHE=1 NATIVE=linux64 RELEASE=1 TILES=1`

The above will build a tiles release explicitly for 64 bit Linux, using Clang and ccache and 4 parallel processes.

Example: `make -j2 LOCALIZE=0`

The above will build a debug-enabled curses version for the architecture you are using, using GCC and 2 parallel processes.

**Note on debug**:
You should probably always build with `RELEASE=1` unless you experience segfaults and are willing to provide stack traces.

# Debian

Instructions for compiling on a Debian-based system. The package names here are valid for Ubuntu 12.10 and may or may not work on your system.

Building instructions, below, always assume you are running them from the Cataclysm:DDA source directory.

## Linux (native) ncurses builds

Dependencies:

  * ncurses or ncursesw (for multi-byte locales)
  * build essentials

Install:

    sudo apt-get install libncurses5-dev libncursesw5-dev build-essential astyle

### Building

Run:

    make

## Linux (native) SDL builds

Dependencies:

  * SDL
  * SDL_ttf
  * freetype
  * build essentials
  * lua5.2 and liblua5.2 - Only necessary if compiling with lua, which some mods like stats through skills use. Versions 5.1, 5.2 and 5.3 are supported.
  * libsdl2-mixer-dev - Used if compiling with sound support.

Install:

    sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev libfreetype6-dev build-essential lua5.2 liblua5.2-dev

### Building

A simple installation could be done by simply running:

    make TILES=1

A more comprehensive alternative is:

    make -j2 TILES=1 SOUND=1 RELEASE=1 LUA=1 USE_HOME_DIR=1

The -j2 flag means it will compile with two parallel processes. It can be omitted or changed to -j4 in a more modern processor. If there is no desire to use lua, or have sound, those flags can also be omitted. The USE_HOME_DIR flag places the user files, like configurations and saves into the home folder, making It easier for backups, and can also be omitted.



## Cross-compiling to linux 32-bit from linux 64-bit

Dependencies:

  * 32-bit toolchain
  * 32-bit ncursesw (compatible with both multi-byte and 8-bit locales)

Install:

    sudo apt-get install libc6-dev-i386 lib32stdc++-dev g++-multilib lib32ncursesw5-dev

### Building

Run:

    make NATIVE=linux32

## Cross-compile to Windows from Linux

To cross-compile to Windows from Linux, you will need MXE. The main difference between the native build process and this one, is the use of the CROSS flag for make. The other make flags are still applicable.

  * `CROSS=` - should be the full path to MXE g++ without the *g++* part at the end

Dependencies:

  * [MXE](http://mxe.cc)
  * [MXE Requirements](http://mxe.cc/#requirements)

Install:

    sudo apt-get install autoconf automake autopoint bash bison bzip2 cmake flex gettext git g++ gperf intltool libffi-dev libgdk-pixbuf2.0-dev libtool libltdl-dev libssl-dev libxml-parser-perl make openssl p7zip-full patch perl pkg-config python ruby scons sed unzip wget xz-utils g++-multilib libc6-dev-i386 libtool-bin
    mkdir -p ~/src/mxe
    git clone https://github.com/mxe/mxe.git ~/src/mxe
    cd ~/src/mxe
    make MXE_TARGETS='x86_64-w64-mingw32.static i686-w64-mingw32.static' sdl2 sdl2_ttf sdl2_image sdl2_mixer gettext lua ncurses

If you are not on a Debian derivative (Linux Mint, Ubuntu, etc), you will have to use a different command than apt-get to install [the MXE requirements](http://mxe.cc/#requirements). Building all these packages from MXE might take a while even on a fast computer. Be patient. If you are not planning on building for both 32-bit and 64-bit, you might want to adjust your MXE_TARGETS.

### Building (SDL)

Run:

    PLATFORM="i686-w64-mingw32.static"
    make CROSS="~/src/mxe/usr/bin/${PLATFORM}-" TILES=1 SOUND=1 LUA=1 RELEASE=1 LOCALIZE=1

Change PLATFORM to x86_64-w64-mingw32.static for a 64-bit Windows build.

To create nice zip file with all the required resources for a trouble free copy on Windows use the bindist target like this:

    PLATFORM="i686-w64-mingw32.static"
    make CROSS="~/src/mxe/usr/bin/${PLATFORM}-" TILES=1 SOUND=1 LUA=1 RELEASE=1 LOCALIZE=1 bindist

### Building (ncurses)

Run:

    PLATFORM="i686-w64-mingw32.static"
    make CROSS="~/src/mxe/usr/bin/${PLATFORM}-" LUA=1 RELEASE=1 LOCALIZE=1

## Cross-compile to Mac OS X from Linux

The procedure is very much similar to cross-compilation to Windows from Linux.
Tested on ubuntu 14.04 LTS but should work on other distros as well.

### Dependencies

  * OSX cross-compiling toolchain [osxcross](https://github.com/tpoechtrager/osxcross)

  * `genisoimage` and [libdmg-hfsplus](https://github.com/planetbeing/libdmg-hfsplus.git) to create dmg distributions

Make sure that all dependency tools are in search `PATH` before compiling.

### Setup

To set up the compiling environment execute the following commands
`git clone https://github.com/tpoechtrager/osxcross.git` to clone the toolchain
`cd osxcross`
`cp ~/MacOSX10.11.sdk.tar.bz2 ./tarballs/` copy prepared MacOSX SDK tarball on place. [Read more about it](https://github.com/tpoechtrager/osxcross/blob/master/README.md#packaging-the-sdk)
`OSX_VERSION_MIN=10.7 ./build.sh to build everything`
Note the targeted minimum supported version of OSX.

Have a prepackaged set of libs and frameworks in place, since compiling with `osxcross` built-in MacPorts is rather difficult and not supported at the moment.
Your directory tree should look like:

    ~/
    ├── Frameworks
    │   ├── SDL2.framework
    │   ├── SDL2_image.framework
    │   ├── SDL2_mixer.framework
    │   └── SDL2_ttf.framework
    └── libs
        ├── gettext
        │   ├── include
        │   └── lib
        ├── lua
        │   ├── include
        │   └── lib
        └── ncurses
            ├── include
            └── lib

Populated with respective frameworks, dylibs and headers.
Tested lib versions are libintl.8.dylib for gettext, liblua.5.2.4.dylib for lua, libncurses.5.4.dylib for ncurses.
These libs were obtained from `homebrew` binary distribution at OS X 10.11
Frameworks were obtained from SDL official website as described in the next [section](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/COMPILING.md#sdl)

### Building (SDL)

To build full feature tiles and sound enabled version with localizations and lua enabled:

    make dmgdist CROSS=x86_64-apple-darwin15- NATIVE=osx OSX_MIN=10.7 USE_HOME_DIR=1 CLANG=1
      RELEASE=1 LOCALIZE=1 LANGUAGES=all LUA=1 TILES=1 SOUND=1 FRAMEWORK=1
      OSXCROSS=1 LIBSDIR=../libs FRAMEWORKSDIR=../Frameworks

Make sure that `x86_64-apple-darwin15-clang++` is in `PATH` environment variable.

### Building (ncurses)

To build full curses version with localizations and lua enabled:

    make dmgdist CROSS=x86_64-apple-darwin15- NATIVE=osx OSX_MIN=10.7 USE_HOME_DIR=1 CLANG=1
      RELEASE=1 LOCALIZE=1 LANGUAGES=all LUA=1 OSXCROSS=1 LIBSDIR=../libs FRAMEWORKSDIR=../Frameworks

Make sure that `x86_64-apple-darwin15-clang++` is in `PATH` environment variable.

## Cross-compile to Android from Linux

The Android build uses [Gradle](https://gradle.org/) to compile the java and native C++ code, and is based heavily off SDL's [Android project template](https://hg.libsdl.org/SDL/file/f1084c419f33/android-project). See the official SDL documentation [README-android.md](https://hg.libsdl.org/SDL/file/f1084c419f33/docs/README-android.md) for further information.

The Gradle project lives in the repository under `android/`. You can build it via the command line or open it in [Android Studio](https://developer.android.com/studio/). For simplicity, it only builds the SDL version with all features enabled, including tiles, sound, localization and lua.

### Dependencies

  * Java JDK 8
  * SDL2 (tested with 2.0.8, though a custom fork is recommended with project-specific bugfixes)
  * SDL2_ttf (tested with 2.0.14)
  * SDL2_mixer (tested with 2.0.2)
  * SDL2_image (tested with 2.0.3)
  * libintl-lite (tested with a custom fork of libintl-lite 0.5)
  * lua (tested with lua 5.1.5)

The Gradle build process automatically installs dependencies from [deps.zip](android/app/deps.zip).

### Setup

Install Linux dependencies. For a desktop Ubuntu installation:

    sudo apt-get install lua5.2 openjdk-8-jdk-headless

Install Android SDK and NDK:

    wget https://dl.google.com/android/repository/sdk-tools-linux-4333796.zip
    unzip sdk-tools-linux-4333796.zip -d ~/android-sdk
    rm sdk-tools-linux-4333796.zip
    ~/android-sdk/tools/bin/sdkmanager --update
    ~/android-sdk/tools/bin/sdkmanager "tools" "platform-tools" "ndk-bundle"
    ~/android-sdk/tools/bin/sdkmanager --licenses

Export Android environment variables (you can add these to the end of `~/.bashrc`):

    export ANDROID_SDK_ROOT=~/android-sdk
    export ANDROID_HOME=~/android-sdk
    export ANDROID_NDK_ROOT=~/android-sdk/ndk-bundle
    export PATH=$PATH:$ANDROID_SDK_ROOT/platform-tools
    export PATH=$PATH:$ANDROID_SDK_ROOT/tools
    export PATH=$PATH:$ANDROID_NDK_ROOT

### Android device setup

Enable [Developer options on your Android device](https://developer.android.com/studio/debug/dev-options). Connect your device to your PC via USB cable and run:

    adb devices
    adb connect <devicename>

### Building

To build an APK, use the Gradle wrapper command line tool (gradlew). The Android Studio documentation provides a good summary of how to [build your app from the command line](https://developer.android.com/studio/build/building-cmdline).

To build a debug APK, from the `android/` subfolder of the repository run:

    ./gradlew assembleDebug

This creates a debug APK in `./android/app/build/outputs/apk/` ready to be installed on your device.

To build a debug APK and immediately deploy to your connected device over adb run:

    ./gradlew installDebug

To build a signed release APK (ie. one that can be installed on a device), [build an unsigned release APK and sign it manually](https://developer.android.com/studio/publish/app-signing#signing-manually).

### Additional notes

The app stores data files on the device in `/sdcard/Android/data/com.cleverraven/cataclysmdda/files`. The data is backwards compatible with the desktop version.

# Mac OS X

To build Cataclysm on Mac you'll need [Command Line Tools for Xcode](https://developer.apple.com/downloads/) and the [Homebrew](http://brew.sh) package manager. With Homebrew, you can easily install or build Cataclysm using the [Cataclysm](https://formulae.brew.sh/formula/cataclysm) forumla.

## Simple build using Homebrew

Homebrew installation will come with tiles, sound and lua suooprt enabled by default.

Once you have Homebrew installed, open Terminal and run one of the following commands.

For a tiles build:

    brew install cataclysm

For an experimental tiles build:

    brew install cataclysm --HEAD

Whichever build you choose, Homebrew will install the appropriate dependencies as needed. The installation will be in `/usr/local/Cellar/cataclysm` with a symlink named `cataclysm` in `/usr/local/bin`.

To launch Cataclysm, just open Terminal and run `cataclysm`.

To update an experimental build, you must uninstall Cataclysm, then reinstall using one of the above commands. Reinstall Cataclysm using the one of the following commands.

For a tiles build:

    brew reinstall cataclysm

For an experimental tiles build:

    brew reinstall cataclysm --HEAD

## Advanced info for Developers

For most people, the simple Homebrew installation is enough. For developers, here are some more technical details on building Cataclysm on Mac OS X.

### SDL

SDL2, SDL2_image, and SDL2_ttf are needed for the tiles build. Optionally, you can add SDL2_mixer for sound support. Cataclysm can be built using either the SDL framework, or shared libraries built from source.

The SDL framework files can be downloaded here:

* [**SDL2**](http://www.libsdl.org/download-2.0.php)
* [**SDL2_image**](http://www.libsdl.org/projects/SDL_image/)
* [**SDL2_ttf**](http://www.libsdl.org/projects/SDL_ttf/)

Copy `SDL2.framework`, `SDL2_image.framework`, and `SDL2_ttf.framework`
to `/Library/Frameworks` or `/Users/name/Library/Frameworks`.

If you want sound support, you will need an additional SDL framework:

* [**SDL2_mixer**](https://www.libsdl.org/projects/SDL_mixer/)

Copy `SDL2_mixer.framework` to `/Library/Frameworks` or `/Users/name/Library/Frameworks`.

Alternatively, SDL shared libraries can be installed using a package manager:

For Homebrew:

    brew install sdl2 sdl2_image sdl2_ttf

with sound:

    brew install sdl2_mixer libvorbis libogg

For MacPorts:

    sudo port install libsdl2 libsdl2_image libsdl2_ttf

with sound:

    sudo port install libsdl2_mixer libvorbis libogg

### ncurses and gettext

ncurses (with wide character support enabled) and gettext are needed if you want to build Cataclysm with localization.

For Homebrew:

    brew tap homebrew/dupes
    brew install gettext ncurses
    brew link --force gettext ncurses

Then, after compiling, be sure to unlink these libraries to prevent conflicts with the OS X shared libraries:

    brew unlink gettext ncurses

For MacPorts:

    sudo port install gettext ncurses
    hash -r

### gcc

The version of gcc/g++ installed with the [Command Line Tools for Xcode](https://developer.apple.com/downloads/) is actually just a front end for the same Apple LLVM as clang.  This doesn't necessarily cause issues, but this version of gcc/g++ will have clang error messages and essentially produce the same results as if using clang. To compile with the "real" gcc/g++, install it with homebrew:

    brew install gcc

However, homebrew installs gcc as gcc-6 (where 6 is the version) to avoid conflicts. The simplest way to use the homebrew version at `/usr/local/bin/gcc-6` instead of the Apple LLVM version at `/usr/bin/gcc` is to symlink the necessary.

    cd /usr/local/bin
    ln -s gcc-6 gcc
    ln -s g++-6 g++
    ln -s c++-6 c++

Or, to do this for everything in `/usr/local/bin/` ending with `-6`,

    find /usr/local/bin -name "*-6" -exec sh -c 'ln -s "$1" $(echo "$1" | sed "s/..$//")' _ {} \;

Also, you need to make sure that `/usr/local/bin` appears before `/usr/bin` in your `$PATH`, or else this will not work.

Check that `gcc -v` shows the homebrew version you installed.

### Compiling

The Cataclysm source is compiled using `make`.

### Make options

* `NATIVE=osx` build for OS X. Required for all Mac builds.
* `OSX_MIN=version` sets `-mmacosx-version-min=` (for OS X > 10.5 set it to 10.6 or higher); omit for 10.5.
* `TILES=1` build the SDL version with graphical tiles (and graphical ASCII); omit to build with `ncurses`.
* `SOUND=1` - if you want sound; this requires `TILES=1` and the additional dependencies mentioned above.
* `FRAMEWORK=1` (tiles only) link to SDL libraries under the OS X Frameworks folders; omit to use SDL shared libraries from Homebrew or Macports.
* `LOCALIZE=0` disable localization (to get around possible `gettext` errors if it is not setup correctly); omit to use `gettext`.
* `LANGUAGES="<lang_id_1>[lang_id_2][...]"` compile localization files for specified languages. e.g. `LANGUAGES="zh_CN zh_TW"`. You can also use `LANGUAGES=all` to compile all localization files.
* `RELEASE=1` build an optimized release version; omit for debug build.
* `CLANG=1` build with [Clang](http://clang.llvm.org/), the compiler that's included with the latest Command Line Tools for Xcode; omit to build using gcc/g++.
* `MACPORTS=1` build against dependencies installed via Macports, currently only `gettext` and `ncurses`.
* `USE_HOME_DIR=1` places user files (config, saves, graveyard, etc) in the user's home directory. For curses builds, this is `/Users/<user>/.cataclysm-dda`, for SDL builds it is `/Users/<user>/Library/Application Support/Cataclysm`.
* `DEBUG_SYMBOLS=1` retains debug symbols when building an optimized release binary, making it easy for developers to spot the crash site.

In addition to the options above, there is an `app` make target which will package the tiles build into `Cataclysm.app`, a complete tiles build in a Mac application that can run without Terminal.

For more info, see the comments in the `Makefile`.

### Make examples

Build a release SDL version using Clang without gettext:

    make NATIVE=osx OSX_MIN=10.7 RELEASE=1 TILES=1 LOCALIZE=0 CLANG=1

Build a release SDL version using Clang, link to libraries in the OS X Frameworks folders, don't use `gettext`, and package it into `Cataclysm.app`:

    make app NATIVE=osx OSX_MIN=10.7 RELEASE=1 TILES=1 FRAMEWORK=1 LOCALIZE=0 CLANG=1

Build a release curses version with gettext supplied by Macports:

    make NATIVE=osx OSX_MIN=10.7 RELEASE=1 LOCALIZE=1 MACPORTS=1 CLANG=1

### Compiling localization files

If you just want to compile localization files for specified languages, you can add `LANGUAGES="<lang_id_1>[lang_id_2][...]"` option to make command:

    make LANGUAGES="zh_CN zh_TW"

You can get the language ID from the filenames of `*.po` in `lang/po` directory. Setting `LOCALIZE=1` may not tell `make` to compile those localization files for you.

### Running

For curses builds:

    ./cataclysm

For SDL:

    ./cataclysm-tiles

For `app` builds, launch Cataclysm.app from Finder.

### Test suite

The build will also generate a test executable at tests/cata_test.
Invoke it as you would any other executable and it will run the full suite of tests.
Pass the ``--help`` flag to list options.

### dmg distribution

You can build a nice dmg distribution file with the `dmgdist` target. You will need a tool called [dmgbuild](https://pypi.python.org/pypi/dmgbuild). To install this tool, you will need Python first. If you are on Mac OS X >= 10.8, Python 2.7 is pre-installed with the OS. If you are on an older version of OS X, you can download Python [on their official website](https://www.python.org/downloads/) or install it with homebrew `brew install python`. Once you have Python, you should be able to install `dmgbuild` by running:

    # This install pip. It might not be required if it is already installed.
    curl --silent --show-error --retry 5 https://bootstrap.pypa.io/get-pip.py | sudo python
    # dmgbuild install
    sudo pip install dmgbuild pyobjc-framework-Quartz

Once `dmgbuild` is installed, you will be able to use the `dmgdist` target like this. The use of `USE_HOME_DIR=1` is important here because it will allow for an easy upgrade of the game while keeping the user config and his saves in his home directory.

    make dmgdist NATIVE=osx OSX_MIN=10.7 RELEASE=1 TILES=1 FRAMEWORK=1 LOCALIZE=0 CLANG=1 USE_HOME_DIR=1

You should see a `Cataclysm.dmg` file.

## Troubleshooting

### ISSUE: crash on startup due to libint.8.dylib aborting

If you're compiling on Mountain Lion or above, it won't be possible to run successfully on older OS X versions due to libint.8 / pthreads version issue.

From https://wiki.gnome.org/GTK+/OSX/Building:

> "There's another issue with building on Lion or Mountain Lion using either "native" or the 10.7 SDK: Apple has updated the pthreads implementation to provide recursive locking. This would be good except that Gettext's libintl uses this and if the pthreads implementation doesn't provide it it fabricates its own. Since the Lion pthreads does provide it, libintl links the provided function and then crashes when you try to run it against an older version of the library. The simplest solution is to specify the 10.6 SDK when building on Lion, but that won't work on Mountain Lion, which doesn't include it. See below for how to install and use XCode 3 on Lion and later for building applications compatible with earlier versions of OSX."

Workaround: install XCode 3 like that article describes, or disable localization support in Cataclysm so gettext/libint are not dependencies. Or else simply don't support OS X versions below 10.7.

### ISSUE: Colors don't show up correctly

Open Terminal's preferences, turn on "Use bright colors for bold text" in "Preferences -> Settings -> Text"


# Windows

## Visual Studio Guide

Visual Studio 2015 (or later) is required to build Cataclysm.  If you use a later version of Visual Studio, you will need to [enable the Visual Studio 2015 (v140) platform toolset](https://developercommunity.visualstudio.com/content/problem/48806/cant-find-v140-in-visual-studio-2017.html). We created solution and project files in directory `msvc-full-features`. Because of the complexity and how troublesome defining every combination of build feature options are, in Visual Studio project we added all build features, including tiles, sound, localization and lua.

### Dependencies

We've prepared an archive containing all the headers and libraries required to build Cataclysm: [http://dev.narc.ro/cataclysm/WinDepend-MSVC.zip](http://dev.narc.ro/cataclysm/WinDepend-MSVC.zip) or [http://dev.narc.ro/cataclysm/WinDepend-MSVC.7z](http://dev.narc.ro/cataclysm/WinDepend-MSVC.7z). The latter is smaller, but if you don't have a 7-zip archive extracter, the former one is easier to deal with.

Extract the 'WinDepend' folder and put it in the root folder of Cataclysm project. Run the "copy_dll_to_bin" batch file and then move the dll files from the bin folder into the root folder the Cataclysm project.

### Lua

The next thing you need to do is to install lua. Download the appropriate x86 or x64 lua from [http://lua-users.org/wiki/LuaBinaries](http://lua-users.org/wiki/LuaBinaries) or [http://dev.narc.ro/cataclysm/WinDepend-lua.zip](http://dev.narc.ro/cataclysm/WinDepend-lua.zip), and extract it to `C:\Windows\System32` or somewhere else on your path.

Once you have it installed, go to the project directory, then go to `src/lua`, and run `lua53 generate_bindings.lua catabindings.cpp`. This will generate the `catabindings.cpp` file which is necessary for compilation.

### Building

Building Cataclysm with Visual Studio is very simple. Just build it like a normal Visual C++ project. The process may takes a long period of time, so you'd better prepare a cup of coffee and some books in front of your computer :)

If you need localization support, execute the bash script `lang/compile_mo.sh` inside Git Bash GUI just like on a UNIX-like system.

### Debugging

After building Cataclysm, you may discover that after pressing the debug button in Visual Studio, Cataclysm just exits after launch with return code 1. That is because of the wrong working directory. You need to configure the working directory to `$(ProjectDir)..`.

### Make a distribution

There is a batch script in `msvc-full-features` folder `distribute.bat`. It will create a sub folder `distribution` and copy all required files(eg. `data/`, `Cataclysm.exe` and dlls) into that folder. Then you can zip it and share the archive on the Internet.

## MinGW Guide
To compile under windows MinGW you first need to download mingw. An automated GUI installer assistant called mingw-get-setup.exe will make everything a lot easier. I recommend installing it to `C:\MinGW`
https://sourceforge.net/projects/mingw/files/latest/download

### MinGW setup
once installed we need to get the right packages. In "Basic Setup", mark `mingw-developer-toolkit`, `mingw32-base` and `mingw32-gcc-g++`

Then install these components using `Installation -> Apply Changes`.

#### Localization
If we want to compile with localization, we will need gettext and libintl. In "All Packages -> MinGW -> MinGW Autotools" ensure that `mingw32-gettext` and `mingw32-libintl` are installed.

### Required Tiles(SDL) Libraries
If we want to compile with Tiles (SDL) we have to download a few libraries.
* `SDL2` http://www.libsdl.org/download-2.0.php chose `SDL2-devel-2.0.X-mingw.tar.gz`.
* `SDL_ttf` https://www.libsdl.org/projects/SDL_ttf/ chose `SDL2_ttf-devel-2.0.12-mingw.tar.gz`.
* `SDL_image` https://www.libsdl.org/projects/SDL_image/ chose ` SDL2_image-devel-2.0.0-mingw.tar.gz`
* `freetype` http://gnuwin32.sourceforge.net/packages/freetype.htm chose `Binaries` and `Developer files`

#### Bundled Libraries
The following archives were pre-bundled for convenience and reduction of head-aches, simply download and extract directly to the root directory of the CDDA source:
* `64-bit SDL \ Tiles \ Sound \ Lua \ Localization` http://dev.narc.ro/cataclysm/cdda-win64-codeblocks.7z

#### Installing Tiles(SDL) libraries.
For the first 3 (`SDL2`, `SDL_ttf` and `SDL_image`) you want to extract the include and lib folders from the `i686-w64-mingw32` folders into your MinGW installation folder. (Recommended `C:\MinGW`). And the `SDL2_image.dll` and `SDL2_ttf.dll` into your cataclysm root folder.

For freetype you want to grab the include and lib folders from the `freetype-2.X.X-X-lib.zip` and move them into your your MinGW installation folder. Then you want to get the freetype6.dll from the `freetype-2.X.X-X-bin.zip` and move it into your cataclysm root folder.

#### ISSUE - "winapifamily.h" no such file or directoyr
There seems to be at the moment of writing that a file in SDL is broken and needs to be replaced.
https://hg.libsdl.org/SDL/raw-file/e217ed463f25/include/SDL_platform.h
Replace SDL_platform.h in the MinGW/include/SDL2 folder and it should be fine.

### Makefile changes
This probably not the best way to do it. But it seems that you need to remove a few dependencies from the makefile or it will not build.
change the line `LDFLAGS += -lfreetype -lpng -lz -ljpeg -lbz2` to `LDFLAGS += -lfreetype`

### Compiling
Navigate to `MinGW\msys\1.0` and run `msys.bat`. This will start a cmd-like shell where the following entries will be made.

Add the MinGW toolchain to your PATH with `export PATH=$PATH:/c/MinGW/bin`. Replace /c/MinGW/ with the directory into which you installed MinGW (/c/ stands for drive C:, so if it's in F:/foo/bar, you'd use /f/foo/bar).

Navigate to the CDDA source code directory.

Compile using `make TILES=1 NATIVE=win32 LOCALIZE=1` and unless there are problems, it should produce a CDDA binary for you.

If you dont want tiles you can change `TILES` to 0.

If you dont want localization you can change `LOCALIZE` to 0.

## Rough guide to building with only MSYS2

This is a tentative step-by-step guide to building your own CDDA with Tiles, Localization and Lua using only MSYS2. You may want to follow it if the MinGW guide above doesn't work for you or you just feel adventurous. Feedback is very much welcome in terms of issues and/or pull-requests.

This guide assumes you're building on a x86_64 build of Windows. If not adjust the invocations appropriately. It has been tested and proven to work on Windows XP, Windows 7 and Windows 10. Your mileage may vary.

#### 1. Go to https://msys2.github.io/ and download appropriate MSYS (top of the page).

#### 2. Install MSYS2 and leave the Run ticker on. You should end up with a MSYS2 terminal.

#### 3. In the open terminal:

```bash
pacman --needed -Sy bash pacman pacman-mirrors msys2-runtime
```

Note: You may close the terminal now and reopen it from the Start menu (MSYS2 Shell, just to be on the safe-er side).

Note: You may need to bash the close button repeatedly. Or use the task manager to kill it.

#### 4. Open an editor that preserves line-endings

Note: Wordpad should do. Or Notepad++.

#### 5. Open `C:\msys64\etc\pacman.conf` and change:

```bash
# By default, pacman accepts packages signed by keys that its local keyring
# trusts (see pacman-key and its man page), as well as unsigned packages.
#SigLevel = Never
SigLevel    = Required DatabaseOptional
LocalFileSigLevel = Optional
#RemoteFileSigLevel = Required
```

To:

```bash
# By default, pacman accepts packages signed by keys that its local keyring
# trusts (see pacman-key and its man page), as well as unsigned packages.
SigLevel = Never
#SigLevel    = Required DatabaseOptional
LocalFileSigLevel = Optional
#RemoteFileSigLevel = Required
```

(Exchange the # on SigLevel). This disables signature checking as it is currently borked.

#### 6. Save the file

#### 7. Run in MSYS2 terminal:

```bash
update-core
pacman -Su
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_ttf
pacman -S mingw-w64-x86_64-ncurses
pacman -S mingw-w64-x86_64-pkg-config mingw-w64-x86_64-libwebp
pacman -S git make
pacman -S mingw-w64-x86_64-lua
```

#### 8. Close MSYS2 terminal and open MinGW-w64 Win64 Shell from Start menu and run:

Note: This will download whole CDDA repository. If you're just testing you should probably add `--depth=1`.

```bash
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

#### 9. Compile your CDDA by running:

```bash
make MSYS2=1 RELEASE=1 TILES=1 LOCALIZE=1 SOUND=1 LUA=1 NATIVE=win64
```

Note: You cannot naively use `-jX` to speed up your building process with `LUA=1`. You must first run `cd src/lua/ && lua generate_bindings.lua && cd ../..` if you want to use `-jX`. X should be the number of threads/cores your processor has.

That's it. You should get a `cataclysm-tiles.exe` binary in the same folder you've found the `Makefile` in. The make flags are the same as the ones described above. For instance, if you do not want to build with sound support, you can remove `SOUND=1`.

# BSDs

There are reports of CDDA building fine on recent OpenBSD and FreeBSD machines (with appropriately recent compilers), and there is some work being done on making the `Makefile` "just work", however we're far from that and BSDs support is mostly based on user contributions. Your mileage may vary. So far essentially all testing has been on amd64, but there is no (known) reason that other architectures shouldn't work, in principle.

### Building on FreeBSD/amd64 10.1 with the system compiler

FreeBSD uses clang as the default compiler as of 10.0, and combines it with libc++ to provide C++11 support out of the box. You will however need gmake (examples for binary packages):

`pkg install gmake`

Tiles builds will also require SDL2:

`pkg install sdl2 sdl2_image sdl2_mixer sdl2_ttf`

Then you should be able to build with something like this (you can of course set CXXFLAGS and LDFLAGS in your .profile or something):

```
export CXXFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"
gmake # ncurses builds
gmake TILES=1 # tiles builds
```

The author has not tested tiles builds, as the build VM lacks X; they do at least compile/link successfully.

### Building ncurses version on FreeBSD/amd64 9.3 with GCC 4.8.4 from ports

For ncurses build add to `Makefile`, before `VERSION`:

```Makefile
OTHERS += -D_GLIBCXX_USE_C99
CXX = g++48
CXXFLAGS += -I/usr/local/lib/gcc48/include
LDFLAGS += -rpath=/usr/local/lib/gcc48
```
Note: or you can `setenv` the above (merging `OTHERS` into `CXXFLAGS`), but you knew that.

And then build with `gmake LOCALIZE=0 RELEASE=1`.

### Building on OpenBSD/amd64 5.8 with GCC 4.9.2 from ports/packages

First, install g++, gmake, and libexecinfo from packages (g++ 4.8 or 4.9 should work; 4.9 has been tested):

`pkg_add g++ gmake libexecinfo`

Then you should  be able to build with something like:

`CXX=eg++ gmake`

Only an ncurses build is possible on 5.8-release, as SDL2 is broken. On recent -current or snapshots, however, you can install the SDL2 packages:

`pkg_add sdl2 sdl2-image sdl2-mixer sdl2-ttf`

and build with:

`CXX=eg++ gmake TILES=1`

### Building on NetBSD/amd64 7.0RC1 with the system compiler

NetBSD has (or will have) gcc 4.8.4 as of version 7.0, which is new enough to build cataclysm. You will need to install gmake and ncursesw:

`pkgin install gmake ncursesw`

Then you should be able to build with something like this (LDFLAGS for ncurses builds are taken care of by the ncurses configuration script; you can of course set CXXFLAGS/LDFLAGS in your .profile or something):

```
export CXXFLAGS="-I/usr/pkg/include"
gmake # ncurses builds
LDFLAGS="-L/usr/pkg/lib" gmake TILES=1 # tiles builds
```

SDL builds currently compile, but did not run in my testing - not only do they segfault, but gdb segfaults when reading the debug symbols! Perhaps your mileage will vary.
