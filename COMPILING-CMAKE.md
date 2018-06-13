#Disclaimer

**WARNING**: CMake build is **NOT** official and should be used for *dev purposes ONLY*.

For official way to build CataclysmDDA See:
  * The latest instructions on how to compile can be found on [our wiki](http://tools.cataclysmdda.com/wiki). 
  * [COMPILING.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/COMPILING.md)


#Contents

  1. Prerequisites
  2. Build Environment
    * UNIX Environment
    * Windows Environment
  3. CMake Build
    * MinGW,MSYS,MSYS2
  5. Build Options
    * CMake specific options
    * CataclysmDDA specific options

#1. Prerequisites

You'll need to have these libraries and their development headers installed in
order to build CataclysmDDA:

 * General
   * `cmake`                     >= 2.6.11
   * `gcc-libs`
   * `glibc`
   * `zlib`
   * `bzip2`
 * Optional
   * `lua51`
   * `gettext`
 * Curses
   * `ncurses`
 * Tiles
   * `SDL`                       >= 2.0.0
   * `SDL_image`                 >= 2.0.0 (with PNG and JPEG support)
   * `SDL_mixer`                 >= 2.0.0 (with Ogg Vorbis support)
   * `SDL_ttf`                   >= 2.0.0
   * `freetype`
 * Sound
   * `vorbis`
   * `libbz2`
   * `libz`
   * `libintl`
   * `iconv`


#2. Build Environment

You can obtain the source code tarball for the latest version from [git](https://github.com/CleverRaven/Cataclysm-DDA).


## UNIX Environment

The following build systems are fully supported for compiling CataclysmDDA on Linux, BSD, and other Unix-like platforms:

 * `CMake` >= 2.6.11


## Windows Environment (MSYS2)

 1. Follow steps from here: https://msys2.github.io/
 2. Install CataclysmDDA build deps:
 
 ```
	pacman -S mingw-w64-i686-toolchain msys/git \
		  mingw-w64-i686-cmake \
		  mingw-w64-i686-SDL2_{image,mixer,ttf} \
		  mingw-w64-i686-lua51 \
		  ncurses-devel \
		  gettext-devel
 ```

 This should get your environment set up to build console and tiles version of windows. 

 **NOTE**: This is only for 32bit builds. 64bit requires the x86_64 instead of the i686
       packages listed above:

 ```
	pacman -S mingw-w64-x86_64-toolchain msys/git \
		  mingw-w64-x86_64-cmake \
		  mingw-w64-x86_64-SDL2_{image,mixer,ttf} \
		  mingw-w64-x86_64-lua51 \
		  ncurses-devel \
		  gettext-devel
 ```

 **NOTE**: If you're trying to test with Jetbrains CLion, point to the cmake version in the
       msys32/mingw32 path instead of using the built in. This will let cmake detect the
       installed packages.


# CMake Build

 CMake has separate configuration and build steps. Configuration
 is done using CMake itself, and the actual build is done using `make`.

 There are two ways to build CataclysmDDA with CMake: inside the source tree or
 outside of it. Out-of-source builds have the advantage that you can have
 multiple builds with different options from one source directory.

 **WARNING**: Inside the source tree build is **NOT** supported.

 To build CataclysmDDA out of source:

 ```
	$ mkdir build && cd build
	$ cmake .. -DCMAKE_BUILD_TYPE=Release
	$ make
 ```

 To install CataclysmDDA after building (as root using su or sudo if necessary):

 ```
	# make install
 ```

 To change build options, you can either pass the options on the command line:

 ```
	$ cmake .. -DOPTION_NAME=option_value
 ```

 Or use either the `ccmake` or `cmake-gui` front-ends, which display all options
 and their cached values on a console and graphical UI, respectively.

 ```
	$ ccmake ..
	$ cmake-gui ..
 ```


## CMake Build Mingw,MSYS,MSYS2

 **NOTE**: For development purposes it is preferred to use `MinGW Win64 Shell` or `MinGW Win32 Shell` instead of `MSYS2 Shell`. In other case, you will need to set `PATH` variable manually.

 For Mingw,MSYS,MSYS2 you should set [Makefiles generator](http://www.cmake.org/cmake/help/cmake-2.6.html#section_Generators)

 Valid choices for MINGW/MSYS are:
	
 * MSYS Makefiles
 * MinGW Makefiles

 Example:

 ```
	# cd <Path-to-CataclysmDDA-Sources>
	# mkdir build
	# cd build
 ```

 For MinGW32:

 ```
	$ cmake -G "MSYS Makefiles" \
		-DCMAKE_MAKE_PROGRAM=mingw32-make\
		-DCMAKE_C_COMPILER=i686-w64-mingw32-gcc\
		-DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++\
		-DCMAKE_SYSTEM_PREFIX_PATH=/c/msys32/mingw32\
		..
	$ mingw32-make
 ```

 For MinGW64:

 ```
        $ cmake -G "MSYS Makefiles" \
		-DCMAKE_MAKE_PROGRAM=mingw32-make\
		-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc\
		-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++\
		-DCMAKE_SYSTEM_PREFIX_PATH=/c/msys64/mingw64\
		..
	$ mingw32-make
 ```

 Shared libraries:
 
 If you got `libgcc_s_dw2-1.dll not found` error you need to copy shared libraries
 to directory with CataclysmDDA executables.

 **NOTE**: For `-DRELEASE=OFF` development builds, You can automate copy process with:
 
 ```
	$ mingw32-make install
 ```

 However, it likely will fail b/c you have different build environment setup :)

 Currently known depends (Maybe outdated use ldd.exe to correct it for Your system)

 * MINGW deps:
   * `libwinpthread-1.dll`
   * `libgcc_s_dw2-1.dll`
   * `libstdc++-6.dll`

 * LOCALIZE deps:
   * `libintl-8.dll`
   * `libiconv-2.dll`

 * LUA deps:
   * `lua51.dll`

 * TILES deps:
   * `SDL2.dll`
   * `SDL2_ttf.dll`
   * `libfreetype-6.dll`
   * `libbz2-1.dll`
   * `libharfbuzz-0.dll`
   * `SDL2_image.dll`
   * `libpng16-16.dll`
   * `libjpeg-8.dll`
   * `libtiff-5.dll`
   * `libjbig-0.dll`
   * `liblzma-5.dll`
   * `libwebp-5.dll`
   * `zlib1.dll`
   * `libglib-2.0-0.dll`

 * SOUND deps:
   * `SDL2_mixer.dll`
   * `libFLAC-8.dll`
   * `libogg-0.dll`
   * `libfluidsynth-1.dll`
   * `libportaudio-2.dll`
   * `libsndfile-1.dll`
   * `libvorbis-0.dll`
   * `libvorbisenc-2.dll`
   * `libmodplug-1.dll`
   * `smpeg2.dll`
   * `libvorbisfile-3.dll`

 
# Build Options

 A full list of options supported by CMake, you may either run the `ccmake`
 or `cmake-gui` front-ends, or run `cmake` and open the generated CMakeCache.txt
 from the build directory in a text editor.

 ```
	$ cmake -DOPTION_NAME1=option_value1 [-DOPTION_NAME2=option_value2 [...]]
 ```


## CMake specific options 

 * CMAKE_BUILD_TYPE=`<build type>`

 Selects a specific build configuration when compiling. `release` produces
 the default, optimized (-Os) build for regular use. `debug` produces a
 slower and larger unoptimized (-O0) build with full debug symbols, which is
 often needed for obtaining detailed backtraces when reporting bugs.

 **NOTE**: By default, CMake will produce `debug` builds unless a different
       configuration option is passed in the command line.


 * CMAKE_INSTALL_PREFIX=`<full path>`

 Installation prefix for binaries, resources, and documentation files.

 
## CataclysmDDA specific options

 * CURSES=`<boolean>`

 Build curses version.


 * TILES=`<boolean>`

 Build graphical tileset version.


 * SOUND=`<boolean>`

 Support for in-game sounds & music.


 * LUA=`<boolean>`

 This enables Lua support. Needed only for full-fledged mods.


 * USE_HOME_DIR=`<boolean>`

 Use user's home directory for save files.


 * LOCALIZE=`<boolean>`

 Support for language localizations. Also enable UTF support.


 * DYNAMIC_LINKING=`<boolean>`

 Use dynamic linking. Or use static to remove MinGW dependency instead.


 * LANGUAGES=`<str>`

 Compile localization files for specified languages. Example:
 ```
 -DLANGUAGES="cs;de;el;es_AR;es_ES"
 ```

 * LUA_BINARY=`<str>`

 Override default Lua binary name or path. You can try to use `luajit` for extra speed.


 * GIT_BINARY=`<str>`

 Override default Git binary name or path.
 
 So a CMake command for building Cataclysm-DDA in release mode with tiles, sound and lua support will look as follows, provided it is run in build directory located in the project.
```
cmake ../ -DCMAKE_BUILD_TYPE=Release -DTILES=ON -DSOUND=ON -DLUA=ON
```
