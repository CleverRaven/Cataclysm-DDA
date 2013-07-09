# Building on Mac OS X with SDL

Tested with OS X 10.8.4, Xcode 4.6.2. I haven't played it extensively but it builds and runs.  
Not sure if it compiles with older OS/XCode versions.

This worked for me, your mileage may vary.

## Requirements

Cataclysm-DDA uses SDL, SDL\_ttf, ncurses, and gettext libraries.

On OS X, SDL can be installed as frameworks or as libraries.
Each framework is a single folder; download them, copy them to your system, done.
SDL libraries are installed most easily via a package manager.

### Installing libraries via a package manager

The most straightforward way to install the libraries is by using a package manager.
Both the SDL libraries and frameworks should be available.

Use [Fink](http://fink.thetis.ig42.org), [Homebrew](http://mxcl.github.io/homebrew/),
[MacPorts](http://www.macports.org), or [pkgsrc](http://www.pkgsrc.org/).

An example, after installing [MacPorts](http://www.macports.org/):

    $ sudo port -v selfupdate
    $ sudo port install gettext
    $ sudo port install libsdl
    $ sudo port install libsdl_ttf

Then you should have the necessary requirements to build Cataclysm-DDA.
Using other package managers should be similar. See **No package manager** for some
advice on building requirements from source, without a package manager.

## Build Cataclysm-DDA

From the Cataclysm-DDA source folder:

    $ export CXXFLAGS="-I/path/to/include" LDFLAGS="-L/path/to/lib"
    
This adds search paths for where to find ncurses, gettext, and also -lSDL and -lSDL\_ttf
if you are using sdl libraries instead of frameworks. Multiple `-I/path/to/...` paths can
be specified.

    $ make NATIVE=osx RELEASE=1 TILES=1 FRAMEWORK=1 OSX_MIN=10.6
    $ ./cataclysm
    $ ./cataclysm-tiles

### Options

`NATIVE=osx` builds for OS X.

`RELEASE=1` builds an optimized 'release' version.

`TILES=1` builds the SDL version; omit for ncurses/console version.

`FRAMEWORK=1` uses .framework bundles in /Library/Frameworks; omit for SDL libs.

`OSX_MIN=version` sets `-mmacosx-version-min=` (mine needs to be 10.6); omit for 10.5.

### Archive

    $ make bindist NATIVE=osx TILES=1

Create a .tar.gz archive of the build; omit `TILES=1` if you built the console version.

## No package manager

Instead of using a package manager, it is relatively easy to install the requirements
without a package manager.

### SDL

Use the frameworks, or build either the frameworks or libraries.

[**SDL framework**](http://www.libsdl.org/download-1.2.php)  
http://www.libsdl.org/release/SDL-1.2.15.dmg

[**SDL\_ttf framework**](http://www.libsdl.org/projects/SDL_ttf/)  
http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

Copy `SDL.framework` and `SDL_ttf.framework` to `/Library/Frameworks`
or `/Users/name/Library/Frameworks`.

To build SDL from source, see `README.MacOSX` in
the [**SDL source archive**](http://www.libsdl.org/release/SDL-1.2.15.tar.gz).

### ncurses and gettext

[**ncurses**](http://www.gnu.org/software/ncurses/) with wide character support  
http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.9.tar.gz

[**gettext**](http://www.gnu.org/software/gettext/)  
http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.2.tar.gz

From the ncurses source folder:

    $ ./configure --prefix=$HOME/opt/catdda --enable-widec --enable-ext-colors --enable-sigwinch
    $ make
    $ make install

From the gettext source folder:

    $ ./configure --prefix=$HOME/opt/catdda --with-libncurses-prefix=$HOME/opt/catdda
    $ make
    $ make install
    
`--prefix=$HOME/opt/catdda` is just an example. If `--prefix=/install/path` is omitted,
it defaults to `/usr/local`and you may need to use `sudo make install` instead of `make install`

`--with-libncurses-prefix=` is where ncurses was installed above, or already installed elsewhere,
as long as it has wide character support.

* You may see prompts asking to install Java; dismiss them.

See **Build Cataclysm-DDA** above after the requirements are installed.
