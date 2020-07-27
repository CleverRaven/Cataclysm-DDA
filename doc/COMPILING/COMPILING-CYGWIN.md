# Compilation guide for 64 bit Windows (using CYGWIN)

This guide contains steps required to allow compilation of Cataclysm-DDA on Windows under CYGWIN.

Steps from current guide were tested on Windows 10 (64 bit) and CYGWIN (64 bit), but should work for other versions of Windows and also CYGWIN (32 bit) if you download 32 bit version of all files.

## Prerequisites:

* Computer with 64 bit version of modern Windows operating system installed (Windows 10, Windows 8.1 or Windows 7);
* NTFS partition with ~10 Gb free space (~2 Gb for CYGWIN installation, ~3 Gb for repository and ~5 Gb for ccache);
* 64 bit version of CYGWIN (installer can be downloaded from [CYGWIN homepage](https://cygwin.com/));

## Installation:

1. Go to [CYGWIN homepage](https://cygwin.com/) and download 64 bit installer (e.g. [setup-x86_64.exe](https://cygwin.com/setup-x86_64.exe).

2. Run downloaded file and install CYGWIN (click `Next` button, select instalation source (e.g. `Install from Internet`), click `Next` button, specify directory where CYGWIN 64 bit will be installed (e.g. `C:\cygwin64`), select whether to install for all users or just you, click `Next` button again, select local package directory (e.g. `C:\Distr\Cygwin`), click `Next` button, select Internet connection settings, click `Next` button again, choose a download site from the list of available download sites,  click `Next` button again).

3. After CYGWIN installation is complete select following packages and press `Next` button to download and install them:

* `wget`.

4. System will tell you it will install following packages (you will install everything else later):

```
...
```

5. Check boxes to add shortcuts to Start Menu and/or Desktop, then press `Finish` button.

## Configuration:

1. Install `apt-cyg` for easier package installation with:

```bash
wget rawgit.com/transcode-open/apt-cyg/master/apt-cyg -O /bin/apt-cyg
chmod 755 /bin/apt-cyg
```

2. Install packages required for compilation with:

```bash
apt-cyg install git make astyle gcc-g++ libintl-devel libiconv-devel libSDL2_image-devel libSDL2_ttf-devel libSDL2_mixer-devel libncurses-devel xinit
```

## Cloning and compilation:

1. Clone Cataclysm-DDA repository with following command line:

**Note:** This will download whole CDDA repository. If you're just testing you should probably add `--depth=1`.

```bash
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

2. Compile with following command line:

```bash
make CCACHE=1 RELEASE=1 CYGWIN=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 RUNTESTS=0
```

**Note**: This will compile release version with Sound and Tiles support and all localization languages, skipping checks and tests and using ccache for faster build. You can use other switches, but `CYGWIN=1`, `DYNAMIC_LINKING=1` and probably `RELEASE=1` are required to compile without issues.

## Running:

1. Run from within CYGWIN XWin Server (found in Start Menu) with following command line:

```bash
./cataclysm-tiles
```
