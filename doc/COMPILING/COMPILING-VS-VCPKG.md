# Compilation guide for Windows (using Visual Studio and vcpkg)

This guide contains steps required to allow compilation of Cataclysm-DDA on Windows using Visual Studio and vcpkg.

Steps from current guide were tested on Windows 10 (64 bit), Visual Studio 2017 (64 bit) and vcpkg, but should as well work with slight modifications for other versions of Windows and Visual Studio.

## Prerequisites:

* Computer with modern Windows operating system installed (Windows 10, Windows 8.1 or Windows 7);
* NTFS partition with ~15 Gb free space (~10 Gb for Visual Studio, ~1 Gb for vcpkg installation, ~3 Gb for repository and ~1 Gb for build cache);
* Git for Windows (installer can be downloaded from [Git homepage](https://git-scm.com/));
* Visual Studio 2017 (or 2015 Visual Studio Update 3 and above);
* Latest version of vcpkg (see instructions on [vcpkg homepage](https://github.com/Microsoft/vcpkg)).

**Note:** Windows XP is unsupported!

## Installation and configuration:

1. Install `Visual Studio` (installer can be downloaded from [Visual Studio homepage](https://visualstudio.microsoft.com/)).

2. Install `Git for Windows` (installer can be downloaded from [Git homepage](https://git-scm.com/)).

3. Install and configure `vcpkg` using instruction from [vcpkg homepage](https://github.com/Microsoft/vcpkg/blob/master/README.md#quick-start) with following command line:

```cmd
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
vcpkg integrate install
```

4. Install (or upgrade) neccessary packages with following command line:

#### install 64 bit dependencies:

```cmd
vcpkg --triplet x64-windows install sdl2 sdl2-image sdl2-mixer[libflac,mpg123,libmodplug,libvorbis] sdl2-ttf gettext
```

or (if you want to build statically linked executable)

```cmd
vcpkg --triplet x64-windows-static install sdl2 sdl2-image sdl2-mixer[libflac,mpg123,libmodplug,libvorbis] sdl2-ttf gettext
```


#### install 32 bit dependencies:

```cmd
vcpkg --triplet x86-windows install sdl2 sdl2-image sdl2-mixer[libflac,mpg123,libmodplug,libvorbis] sdl2-ttf gettext
```

or (if you want to build statically linked executable)

```cmd
vcpkg --triplet x86-windows-static install sdl2 sdl2-image sdl2-mixer[libflac,mpg123,libmodplug,libvorbis] sdl2-ttf gettext
```

#### upgrade all dependencies:

```cmd
vcpkg upgrade
```

## Cloning and compilation:

1. Clone Cataclysm-DDA repository with following command line:

**Note:** This will download whole CDDA repository. If you're just testing you should probably add `--depth=1`.

```cmd
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

2. Open one of provided solutions (`msvc-full-features\Cataclysm-vcpkg.sln` for dynamically linked executable or `msvc-full-features\Cataclysm-vcpkg-static.sln` for statically linked executable) in `Visual Studio`, select configuration (`Release` or `Debug`) and platform (`x64` or `x86`) and build it.

**Note**: This will compile release version with Sound, Tiles and Localization support (language files won't be automatically compiled).

3. Building Cataclysm with Visual Studio is very simple. Just build it like a normal Visual C++ project. The process may takes a long period of time, so you'd better prepare a cup of coffee and some books in front of your computer :)

4. If you need localization support, execute the bash script `lang/compile_mo.sh` inside Git Bash GUI just like on a UNIX-like system. This will compile the language files that were not automatically compiled in step 2 above.

### Debugging

After building Cataclysm, you may discover that after pressing the debug button in Visual Studio, Cataclysm just exits after launch with return code 1. That is because of the wrong working directory. You need to configure the working directory to `$(ProjectDir)..`.

When debugging, it is not strictly necessary to use a `Debug` build; `Release` builds run significantly faster, can still be run in the debugger, and most of the time will have most of the information you need.

### Make a distribution

There is a batch script in `msvc-full-features` folder `distribute.bat`. It will create a sub folder `distribution` and copy all required files(eg. `data/`, `Cataclysm.exe` and dlls) into that folder. Then you can zip it and share the archive on the Internet.
