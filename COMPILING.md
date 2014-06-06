Instructions for compiling on a Debian-based system. The package names here are
valid for Ubuntu 12.10 and may or may not work on your system.

Building instructions, below, always assume you are running them from the
Cataclysm:DDA source directory.

## Linux (native) ncurses builds ##
Dependencies:
  * ncurses
  * build essentials

```
sudo apt-get install libncurses5-dev build-essential
```

### Building ###
```
make
```

## Cross-compiling to linux 32-bit from linux 64-bit ##
Dependencies:
  * 32-bit toolchain
  * 32-bit ncurses

```
sudo apt-get install libc6-dev-i386 lib32stdc++-dev g++-multilib lib32ncurses5-dev
```

### Building ###
```
make NATIVE=linux32
```

## Cross-compile to Windows from Linux ##
Dependencies:
  * [mxe](http://mxe.cc)

```
sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
mkdir -p ~/src/mxe
git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
cd ~/src/mxe
make gcc glib
```

### Building ###
```
PATH="${PATH}:~/src/mxe/usr/bin"
make CROSS=i686-pc-mingw32-
```

## Linux (native) SDL builds ##
Dependencies:
  * SDL
  * SDL_ttf
  * freetype
  * build essentials

```
sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libfreetype6-dev build-essential
```

### Building ###
```
make TILES=1
```

## Cross-compile to Windows SDL from Linux ##
Dependencies:
  * [mxe](http://mxe.cc)

```
sudo apt-get install autoconf bison flex cmake git automake intltool libtool scons yasm
mkdir -p ~/src/mxe
git clone -b stable https://github.com/mxe/mxe.git ~/src/mxe
cd ~/src/mxe
make sdl sdl_ttf
```

### Building ###
```
PATH="${PATH}:~/src/mxe/usr/bin"
make TILES=1 CROSS=i686-pc-mingw32-
```
