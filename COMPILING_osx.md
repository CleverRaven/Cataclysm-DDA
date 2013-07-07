# Building on Mac OS X with SDL frameworks

Tested with OS X 10.8.4, Xcode 4.6.2. I haven't played it extensively but it builds and runs.  
Not sure if it compiles with older OS/XCode versions.

This worked for me, your mileage may vary.

## Prerequisites

[**ncurses**](http://www.gnu.org/software/ncurses/) with wide character support  
http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.9.tar.gz

[**gettext**](http://www.gnu.org/software/gettext/)  
http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.2.tar.gz

[**SDL framework**](http://www.libsdl.org/download-1.2.php)  
http://www.libsdl.org/release/SDL-1.2.15.dmg

[**SDL\_ttf framework**](http://www.libsdl.org/projects/SDL_ttf/)  
http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

## Install frameworks

Copy `SDL.framework` and `SDL_ttf.framework` to `/Library/Frameworks`
or `/Users/name/Library/Frameworks`.

## Install ncurses and gettext

Use [Fink](http://fink.thetis.ig42.org), [Homebrew](http://mxcl.github.io/homebrew/),
[MacPorts](http://www.macports.org), or build them (see below).

`--prefix=$HOME/opt/catdda` is just an example. If `--prefix=/install/path` is omitted,
it defaults to `/usr/local`and you may need to use `sudo make install` instead of `make install`

From the ncurses source folder:

    $ ./configure --prefix=$HOME/opt/catdda --enable-widec --enable-ext-colors --enable-sigwinch
    $ make
    $ make install

From the gettext source folder:

    $ ./configure --prefix=$HOME/opt/catdda --with-libncurses-prefix=$HOME/opt/catdda
    $ make
    $ make install
    
`--with-libncurses-prefix=` is where ncurses was installed above, or already installed elsewhere,
as long as it has wide character support.

* You may see prompts asking to install Java; dismiss them.

## Build Cataclysm-DDA

From the Cataclysm-DDA source folder:

    $ export CXXFLAGS="-I/path/to/include" LDFLAGS="-L/path/to/lib"
    
This adds search paths for where to find ncurses and gettext. Following the above example,
I would use `export CXXFLAGS="-I$HOME/opt/catdda/include" LDFLAGS="-L$HOME/opt/catdda/lib"`.

    $ make NATIVE=osx TILES=1 RELEASE=1 OSX_MIN=10.6
    $ ./cataclysm
    $ ./cataclysm-tiles

### Options

`NATIVE=osx` builds for OS X.

`TILES=1` builds the SDL version; omit for ncurses/console version.

`RELEASE=1` builds an optimized 'release' version.

`OSX_MIN=version` sets `-mmacosx-version-min=` (mine needs to be 10.6 to compile); omit for 10.5.

### Archive

    $make bindist NATIVE=osx TILES=1

Create a .tar.gz archive of the build; omit `TILES=1` if you built the console version.

## Fonts

Something in sdlcurse.cpp is making the font look a bit funny in the SDL version -
the baseline seems to vary a bit for certain letters, making the lines look wobbly.

Here are a few ways to fix it (not sure these are actual solutions, but they seem to help):

Replace the line `typeface = "data/font/fixedsys.ttf";` with `typeface = "data/termfont";`.

Or, replace the line `fontblending = (blending=="blended");` with `fontblending = (blending=="solid");`.

Or, if you've already compiled - it's a hack - but you can create a symlink to the font you want to use:

    $ cd data/fonts
    $ mv fixedsys.ttf fixedsys.ttf~
    $ ln -s ../termfont fixedsys.ttf

## TODO

make it work with sdl libs, i.e. non-framework sdl installed via macports, homebrew... this would be most similar to the linux build, just different locations for search paths.
