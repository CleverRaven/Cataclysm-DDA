# Compiling CMAKE
# Disclaimer

**WARNING**: CMake build is **NOT** official and should be used for *dev purposes ONLY*.

For the official way to build CataclysmDDA, see:
  * [COMPILING.md](COMPILING.md)


# Contents

1. Prerequisites
1. Build Environment
   * UNIX Environment
   * Windows Environment
1. CMake Build
   * MinGW,MSYS,MSYS2
1. Build Options
   * CMake-specific options
   * CataclysmDDA-specific options


# Prerequisites

You'll need to have these libraries and their development headers installed in order to build CataclysmDDA:

* General
  * `cmake`                     >= 3.20.0
  * `gcc-libs`
  * `glibc`
  * `zlib`
  * `bzip2`
* Optional
  * `gettext`
* Curses
  * `ncurses`
* Tiles
  * `SDL`                       >= 2.0.0
  * `SDL_image`                 >= 2.0.0
    * `jpeg`
    * `png`
    * `tiff`
    * `jbig`
    * `LZMA`
    * `zstd`
  * `SDL_ttf`                   >= 2.0.0
    * `freetype`
    * `harfbuzz`
* Sound
  * `SDL_mixer`                 >= 2.0.0
    * `modplug`
    * `fluidsynth`
    * `vorbisfile`      
    * `FLAC`
    * `mpg123`
    * `opusfile`
  * `libbz2`
  * `libz`
  * `libintl`
  * `iconv`


# Build Environment

You can obtain the source code tarball for the latest version from [Github](https://github.com/CleverRaven/Cataclysm-DDA).


## UNIX Environment

Obtain the packages listed above with your system package manager.


## Windows Environment (MSYS2)

Please refer to `COMPILING-MSYS.md`

   **NOTE**: If you're trying to test with Jetbrains CLion, point to the CMake version in the msys32/mingw32 path instead of using the built in. This will let CMake detect the installed packages.


# CMake Build

CMake has separate configuration and build steps. Configuration is done using CMake itself, and the actual build is done using either `make` (for Makefiles generator) or the build-system-agnostic `cmake --build . `.

There are two ways to build CataclysmDDA with CMake: inside the source tree or outside of it. Out-of-source builds have the advantage that you can have multiple builds with different options from one source directory.

**WARNING**: Inside the source tree build is **NOT** supported.

To build CataclysmDDA out of source:

```
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ make
```

The above example creates a build directory inside the source directory, but that's not required - you can just as easily create it in a completely different location.

To install CataclysmDDA after building (as root using su or sudo if necessary):

```
# make install
```

To change build options, you can either pass the options on the command line:

```
$ cmake .. -DOPTION_NAME=option_value
```

Or use either the `ccmake` or `cmake-gui` front-ends, which display all options and their cached values on a console UI or a graphical UI, respectively.

```
$ ccmake ..
$ cmake-gui ..
```


## CMake Build for MSYS2 (MinGW)

1. Follow `COMPILING-MSYS.md` 

2. Run
```
cmake --preset windows-tiles-sounds-x64
cmake --build --preset windows-tiles-sounds-x64
```

## CMake Build for Visual Studio / MSBuild

CMake can generate  `.sln` and `.vcxproj` files used either by Visual Studio itself or by the MSBuild command line compiler (if you don't want a full fledged IDE) and have more "native" binaries than what MSYS/Cygwin can provide.

At the moment only a limited combination of options is supported (tiles only, no localizations, no backtrace).

### Get the tools

* CMake from the official site - https://cmake.org/download/.
* [Microsoft compiler](https://visualstudio.microsoft.com/downloads/?q=build+tools), choose "Build Tools for Visual Studio 2017". When installing, choose the "Visual C++ Build Tools" options.
  * Alternatively, you can download and install the complete Visual Studio, but that's not required.

### Get the required libraries

* `SDL2` - <https://www.libsdl.org/download-2.0.php> (you need the "(Visual C++ 32/64-bit)" version; same below)
* `SDL2_ttf` - <https://www.libsdl.org/projects/SDL_ttf/>
* `SDL2_image` - <https://www.libsdl.org/projects/SDL_image/>
* `SDL2_mixer` (optional, for sound support) - <https://www.libsdl.org/projects/SDL_mixer/>
* Unsupported (and unused in the following instructions) optional libs:
  *  `gettext`/`libintl` - <http://gnuwin32.sourceforge.net/packages/gettext.htm>
  * `ncurses` - ???

### Unpack the archives with the libraries

Open the Windows command line (or powershell) and set the environment variables to point to the libs above as follows (adjusting the paths as appropriate):

```
> set SDL2DIR=C:\path\to\SDL2-devel-2.0.9-VC
> set SDL2TTFDIR=C:\path\to\SDL2_ttf-devel-2.0.15-VC
> set SDL2IMAGEDIR=C:\path\to\SDL2_image-devel-2.0.4-VC
> set SDL2MIXERDIR=C:\path\to\SDL2_mixer-devel-2.0.4-VC
```

(for powershell the syntax is `$env:SDL2DIR="C:\path\to\SDL2-devel-2.0.9-VC"`).

### Make a build directory and run CMake's configuration step

```
> cd <path to cdda sources>
> mkdir build
> cd build
> cmake .. -DTILES=ON -DLOCALIZE=OFF -DBACKTRACE=OFF -DSOUND=ON
```

### Build!

```
> cmake --build . -j 2 -- /p:Configuration=Release
```

The `-j 2` flag controls build parallelism - you can omit it if you wish. The `/p:Configuration=Release` flag is passed directly to MSBuild and controls optimizations. If you omit it, the `Debug` configuration would be built instead. For powershell you'll need to have an extra ` -- ` after the first one.

The resulting files will be put into a `Release` directory inside your source Cataclysm-DDA folder. To make them run you'd need to first move them to the source Cataclysm-DDA directory itself (so that the binary has access to the game data), and second put the required `.dll`s into the same folder - you can find those inside the directories for dev libraries under `lib/x86/` or `lib/x64/` (you likely need the `x86` ones even if you're on 64-bit machine).

The copying of dlls is a one-time task, but you'd need to move the binary out of `Release/` each time it's built. To automate it a bit, you can configure cmake and set the desired binary's destination directory with `-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=`  option (and similar for `CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG`).

Run the game. Should work.


# Build Options

For a full list of options supported by CMake, you may either run the `ccmake` or `cmake-gui` front-ends, or run `cmake` and open the generated `CMakeCache.txt` from the build directory in a text editor.

```
$ cmake -DOPTION_NAME1=option_value1 [-DOPTION_NAME2=option_value2 [...]]
```


## CMake-specific options

* `CMAKE_BUILD_TYPE=<build type>`

  Selects a specific build configuration when compiling. `release` produces the default, optimized (`-Os`) build for regular use. `debug` produces a slower and larger unoptimized (`-O0`) build with full debug symbols, which is often needed for obtaining detailed backtraces when reporting bugs.

  **NOTE**: By default, CMake will produce `debug` builds unless a different configuration option is passed in the command line.


* `CMAKE_INSTALL_PREFIX=<full path>`

  Installation prefix for binaries, resources, and documentation files.


## CataclysmDDA-specific options

 * `CURSES=<boolean>`: Build curses version.
 * `TILES=<boolean>`: Build graphical tileset version.
 * `SOUND=<boolean>`: Support for in-game sounds & music.
 * `USE_XDG_DIR=<boolean>`: Use user's XDG directories for save and config files.
 * `USE_HOME_DIR=<boolean>`: Use user's home directory for save and config files.
 * `USE_PREFIX_DATA_DIR=<boolean>`: Use UNIX system directories for game data in release build.
 * `LOCALIZE=<boolean>`: Support for language localizations. Also enable UTF support.
 * `LANGUAGES=<str>`: Compile localization files for specified languages. Example:

   ```
   -DLANGUAGES="cs;de;el;es_AR;es_ES"
   ```

   Note that language files are only compiled automatically when building the `RELEASE` build type. For other build types, you need to add the `locale` target to the `make` command: for example `make all locale`.

 * `DYNAMIC_LINKING=<boolean>`: Use dynamic linking. Or use static to remove MinGW dependency instead.
 * `GIT_BINARY=<str>` Override the default Git binary name or path.

   So a CMake command for building Cataclysm-DDA in release mode with tiles and sound support will look as follows, provided it is run in the build directory located in the project.

   ```
   cmake ../ -DCMAKE_BUILD_TYPE=Release -DTILES=ON -DSOUND=ON
   ```
