# Compilation guide for 64-bit Windows (using MSYS2)

This guide contains instructions for compiling Cataclysm-DDA on Windows under MSYS2. **PLEASE NOTE:** These instructions *are not intended* to produce a redistributable copy of CDDA. Please download the official builds from the website or [cross-compile from Linux](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/COMPILING/COMPILING.md#cross-compile-to-windows-from-linux) if that is your intention.

These instructions were written using 64-bit Windows 7 and the 64-bit version of MSYS2; the steps should be the same for other versions of Windows.

## Prerequisites:

* Windows 7, 8, 8.1, or 10
* NTFS partition with ~10 Gb free space (~2 Gb for MSYS2 installation, ~3 Gb for repository and ~5 Gb for ccache)
* 64-bit version of MSYS2

**Note:** Windows XP is unsupported!

## Installation:

1. Go to the [MSYS2 homepage](http://www.msys2.org/) and download the installer.

2. Run the installer. It is suggested that you install to a dev-specific folder (C:\dev\msys64\ or similar), but it's not strictly necessary.

3. After installation, run MSYS2 64bit now.

## Configuration:

1. Update the package database and core system packages:

```bash
pacman -Syyu
```

2. MSYS will inform you of a cygheap base mismatch and inform you a forked process died unexpectedly; these errors appear to be due to the nature of `pacman`'s upgrades and *may be safely ignored.* You will be prompted to close the terminal window; do so, then re-start using the MSYS2 MinGW 64-bit menu item.

3. Update remaining packages:

```bash
pacman -Su
```

4. Install packages required for compilation:

```bash
pacman -S git make mingw-w64-x86_64-{astyle,ccache,gcc,libmad,libwebp,pkg-config,SDL2} mingw-w64-x86_64-SDL2_{image,mixer,ttf}
```

5. Close MSYS2.

6. Update path variables in the system-wide profile file (e.g. `C:\dev\msys64\etc\profile`) as following:

- find lines:

```
    MSYS2_PATH="/usr/local/bin:/usr/bin:/bin"
    MANPATH='/usr/local/man:/usr/share/man:/usr/man:/share/man'
    INFOPATH='/usr/local/info:/usr/share/info:/usr/info:/share/info'
```

and

```
    PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig"
```

- and replace them with:

```
    MSYS2_PATH="/usr/local/bin:/usr/bin:/bin:/mingw64/bin"
    MANPATH='/usr/local/man:/usr/share/man:/usr/man:/share/man:/mingw64/share/man'
    INFOPATH='/usr/local/info:/usr/share/info:/usr/info:/share/info:/mingw64/share/man'
```

and

```
    PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/share/pkgconfig:/lib/pkgconfig:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig"
```

## Cloning and compilation:

1. Open MSYS2 and clone the Cataclysm-DDA repository:

```bash
cd /c/dev/
git clone https://github.com/CleverRaven/Cataclysm-DDA.git ./Cataclysm-DDA
```

**Note:** This will download the entire CDDA repository and all of its history (3GB). If you're just testing, you should probably add `--depth=1` (~350MB).

2. Compile with following command line:

```bash
cd Cataclysm-DDA
make -j$((`nproc`+0)) CCACHE=1 RELEASE=1 MSYS2=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0
```

You will receive warnings about unterminated character constants; they do not impact the compilation as far as this writer is aware.

**Note**: This will compile a release version with Sound and Tiles support and all localization languages, skipping checks and tests, and using ccache for build acceleration. You can use other switches, but `MSYS2=1`, `DYNAMIC_LINKING=1` and probably `RELEASE=1` are required to compile without issues.

## Running:

1. Run inside MSYS2 from Cataclysm's directory with the following command:

```bash
./cataclysm-tiles
```

**Note:** If you want to run the compiled executable outside of MSYS2, you will also need to update your user or system `PATH` variable with the path to MSYS2's runtime binaries (e.g. `C:\dev\msys64\mingw64\bin`).
