<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
*Contents*

- [Compilation guide for 64 bit Windows (using Cygwin)](#compilation-guide-for-64-bit-windows-using-cygwin)
  - [Prerequisites:](#prerequisites)
  - [Installation:](#installation)
  - [Configuration:](#configuration)
  - [Cloning and compilation:](#cloning-and-compilation)
  - [Running:](#running)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Compilation guide for 64 bit Windows (using Cygwin)

This guide contains instructions for compiling Cataclysm-DDA on Windows under Cygwin. **PLEASE NOTE:** These instructions *are not intended* to produce a redistributable copy of CDDA. Please download the official builds from the website or [cross-compile from Linux](COMPILING.md#cross-compile-to-windows-from-linux) if that is your intention.

These instructions were written using 64-bit Windows 7 and the 64-bit version of Cygwin; the steps should be the same for other versions of Windows.

Due to slow environment setup and execution of the resulting binary, compilation using MSYS2 is preferred.

> **The SDL3 steps below are unverified.** They were reconstructed from the
> upstream Cygwin package listings after SDL2 support was removed, and have not
> been run on a Cygwin host. If you follow them, please report what worked so
> this note can be dropped.

## Prerequisites:

* 64-bit version of Windows 7, 8, 8.1, or 10
* NTFS partition with ~10 Gb free space (~2 Gb for Cygwin installation, ~3 Gb for repository and ~5 Gb for ccache)
* 64-bit version of Cygwin

## Installation:

1. Go to the [Cygwin homepage](https://cygwin.com/) and download the 64-bit installer (e.g. [setup-x86_64.exe](https://cygwin.com/setup-x86_64.exe)).

2. Run downloaded file and install Cygwin. Select `Install from Internet` and select the desired installation directory. It is suggested that you install into a dev-specific directory (e.g. `C:\dev\cygwin64`), but it's not strictly necessary. Install for all users.

3. Give the `\downloads\` folder as the local package directory (e.g. `C:\dev\cygwin64\downloads`).

4. Use system proxy settings unless you know you have a reason not to.

5. On the next screen, select any mirror you like.

6. Enter `wget` into the search box, expand the "Web" category and select the latest version in the drop-down (1.21.1-1 as of the time this guide was written).

7. Confirm that wget is shown in the following screen by scrolling to the bottom. This is the full list of packages that Cygwin will download and install.

8. Retry any packages that throw errors.

9. Check boxes to add shortcuts to Start Menu and/or Desktop, then press `Finish` button.

## Configuration:

1. Launch the Cygwin64 terminal from your desktop.

2. Install `apt-cyg` for easier package installation:

```bash
wget rawgit.com/transcode-open/apt-cyg/master/apt-cyg -O /bin/apt-cyg
chmod 755 /bin/apt-cyg
```

3. Install packages required for compilation:

```bash
apt-cyg install astyle ccache cmake gcc-g++ gettext-devel git libiconv-devel libintl-devel libSDL3-devel make pkg-config xinit
```

You will see messages saying packages are already installed, as well as Cygwin installing packages you didn't ask for; this is the result of Cygwin's package manager automatically resolving dependencies.

4. Build the SDL3 satellite libraries from source:

Cygwin packages the SDL3 core library (`libSDL3-devel`) but not `SDL3_image`,
`SDL3_ttf` or `SDL3_mixer`, so those three have to be built against it. The
versions below are the ones CI uses; `.github/actions/setup-sdl3-stack/action.yml`
carries the same pins and the full option list.

```bash
mkdir -p ~/src && cd ~/src

git clone --depth 1 --branch release-3.4.4 https://github.com/libsdl-org/SDL_image.git
cmake -S SDL_image -B SDL_image-build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=ON \
    -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_PNG=ON -DSDLIMAGE_JPG=ON \
    -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF
cmake --build SDL_image-build -j"$(nproc)" && cmake --install SDL_image-build

git clone --depth 1 --branch release-3.2.2 https://github.com/libsdl-org/SDL_ttf.git
cmake -S SDL_ttf -B SDL_ttf-build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=ON \
    -DSDLTTF_VENDORED=OFF -DSDLTTF_PLUTOSVG=OFF
cmake --build SDL_ttf-build -j"$(nproc)" && cmake --install SDL_ttf-build
```

Only build SDL3_mixer if you want `SOUND=1`:

```bash
cd ~/src
git clone --depth 1 --branch release-3.2.4 https://github.com/libsdl-org/SDL_mixer.git
cmake -S SDL_mixer -B SDL_mixer-build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=ON \
    -DSDLMIXER_VENDORED=OFF -DSDLMIXER_FLAC=ON -DSDLMIXER_MP3=ON \
    -DSDLMIXER_OPUS=OFF -DSDLMIXER_MOD=OFF -DSDLMIXER_MIDI=OFF -DSDLMIXER_GME=OFF
cmake --build SDL_mixer-build -j"$(nproc)" && cmake --install SDL_mixer-build
```

Each library needs its own codec headers installed first. Add the matching
`-devel` packages with `apt-cyg` if CMake reports one missing.

## Cloning and compilation:

1. Clone the Cataclysm-DDA repository with following command:

**Note:** This will download the entire CDDA repository and all of its history (3GB). If you're just testing, you should probably add `--depth=1` (~350MB).

**Note:** If you want to contribute to CDDA, see [example git workflow](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/CONTRIBUTING.md#example-workflow).

```bash
cd /cygdrive/c/dev
git clone https://github.com/CleverRaven/Cataclysm-DDA.git
cd Cataclysm-DDA
```

2. Compile:

```bash
make CCACHE=1 RELEASE=1 CYGWIN=1 DYNAMIC_LINKING=1 TILES=1 SOUND=1 LOCALIZE=1 LANGUAGES=all LINTJSON=0 ASTYLE=0 BACKTRACE=0 TESTS=0
```

You will receive warnings about unterminated character constants; they do not impact the compilation as far as this writer is aware.

This will compile release version with Sound and Tiles support and all localization languages, skipping checks and tests and using ccache for faster build. You can use other switches, but `CYGWIN=1`, `DYNAMIC_LINKING=1` and probably `RELEASE=1` are required to compile without issues. `TILES=1` implies SDL3; there is no SDL2 fallback.

## Running:

1. Execute the XWin Server from the Start menu.

2. When the icons appear in the system tray, right-click the one that looks like a black C (X Applications Menu.)

3. Point to System Tools, then click UXTerm.

```bash
cd /cygdrive/c/dev/Cataclysm-DDA
./cataclysm-tiles
```

There is no functionality for running Cygwin-compiled CDDA from outside of UXTerm.
