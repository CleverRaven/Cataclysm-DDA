# Compilation guide for 64-bit Windows (using MSYS2 and Code::Blocks)

This guide contains steps required to allow compilation of Cataclysm-DDA on Windows under MSYS2.

Steps from current guide were tested on Windows 10 (64-bit) and  MSYS2 (64-bit), but should work for other versions of Windows and also MSYS2 (32-bit) if you download 32-bit version of all files.

## Preqrequisites:

* Computer with 64-bit version of modern Windows operating system installed (_Windows XP is unsupported!_);
* NTFS partition with ~10 Gb free space (~2 Gb from MSYS2 installation, ~3 Gb for repository and ~5 Gb for ccache);
* 64-bit version of MSYS2 (installer can be downloaded from [MSYS2 homepage](http://www.msys2.org/));

## Installation:

1. Go to [MSYS2 homepage](http://www.msys2.org/) and download 64-bit installer (e.g. [msys2-x86_64-20180531.exe](http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20180531.exe)).

2. Run downloaded file and install MSYS2 (click `Next` button, specifiy directory where MSYS2 64 bit will be installed (e.g. `C:\msys64`), click `Next` button again, specify Start Menu folder name and click `Install` button).

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
pacman -S git git-extras-git make mingw-w64-x86_64-{astyle,ccache,gcc,libwebp,lua,ncurses,pkg-config,SDL2} mingw-w64-x86_64-SDL2_{image,mixer,ttf}
```

## Cloning and compilation:

1. Clone with following command-line:

**Note**: This will download whole CDDA repository. If you're just testing you should probably add `--depth=1`.

```bash
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
``` 

2. Compile with following command line:

```bash
make CCACHE=1 RELEASE=1 MSYS2=1 LUA=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0 DYNAMIC_LINKING=1 
```

## Running:

1. Run with following command line:

```bash
./cataclysm-tiles
```
