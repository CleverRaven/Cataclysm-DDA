# Compilation guide for 64 bit Windows (using MSYS2)

This guide contains steps required to allow compilation of Cataclysm-DDA on Windows under MSYS2.

Steps from current guide were tested on Windows 10 (64 bit) and MSYS2 (64 bit), but should work for other versions of Windows and also MSYS2 (32 bit) if you download 32 bit version of all files.

## Prerequisites:

* Computer with 64 bit version of modern Windows operating system installed (Windows 10, Windows 8.1 or Windows 7);
* NTFS partition with ~10 Gb free space (~2 Gb for MSYS2 installation, ~3 Gb for repository and ~5 Gb for ccache);
* 64 bit version of MSYS2 (installer can be downloaded from [MSYS2 homepage](http://www.msys2.org/));

**Note:** Windows XP is unsupported!

## Installation:

1. Go to [MSYS2 homepage](http://www.msys2.org/) and download 64 bit installer (e.g. [msys2-x86_64-20180531.exe](http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20180531.exe)).

2. Run downloaded file and install MSYS2 (click `Next` button, specify directory where MSYS2 64 bit will be installed (e.g. `C:\msys64`), click `Next` button again, specify Start Menu folder name and click `Install` button).

3. After MSYS2 installation is complete press `Next` button, tick `Run MSYS2 64 bit now` checkbox and press `Finish` button.

## Configuration:

1. Update the package database and core system packages with:

```bash
pacman -Syu
```

2. If asked close MSYS2 window and restart it from Start Menu or `C:\msys64\msys2_shell.cmd`.

3. Update remaining packages with:

```bash
pacman -Su
```

4. Install packages required for compilation with:

```bash
pacman -S git git-extras make mingw-w64-x86_64-{astyle,ccache,gcc,libmad,libwebp,ncurses,pkg-config,SDL2} mingw-w64-x86_64-SDL2_{image,mixer,ttf}
```

5. Update paths in system-wide profile file (e.g. `C:\msys64\etc\profile`) as following:

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

6. Restart MSYS2 to apply path changes.

## Cloning and compilation:

1. Clone Cataclysm-DDA repository with following command line:

**Note:** This will download whole CDDA repository. If you're just testing you should probably add `--depth=1`.

```bash
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

2. Compile with following command line:

```bash
make CCACHE=1 RELEASE=1 MSYS2=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0
```

**Note**: This will compile release version with Sound and Tiles support and all localization languages, skipping checks and tests and using ccache for faster build. You can use other switches, but `MSYS2=1`, `DYNAMIC_LINKING=1` and probably `RELEASE=1` are required to compile without issues.

## Running:

1. Run from within MSYS2 with following command line:

```bash
./cataclysm-tiles
```

**Note:** If you want to run compiled executable from Explorer you will also need to update user or system `PATH` variable with path to MSYS2 runtime binaries (e.g. `C:\msys64\mingw64\bin`).
