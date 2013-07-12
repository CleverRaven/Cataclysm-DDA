# Building Cataclysm-DDA on Mac OS X

Tested with OS X 10.8.4, Xcode 4.6.2.
I haven't played it extensively but it builds and runs.

Not sure if it compiles with older OS/XCode versions.
This worked for me, your mileage may vary.

## Requirements

Cataclysm-DDA uses SDL, SDL\_ttf, ncurses, and gettext.

SDL can be installed as frameworks or shared libraries.

### SDL Frameworks

[**SDL framework**](http://www.libsdl.org/download-1.2.php)  
http://www.libsdl.org/release/SDL-1.2.15.dmg

[**SDL\_image framework**](http://www.libsdl.org/projects/SDL_ttf/)  
http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

Copy `SDL.framework` and `SDL_ttf.framework` to `/Library/Frameworks`
or `/Users/name/Library/Frameworks`.

### ncurses, gettext, libsdl, lib\_ttf

Using a package manager:  
[Fink](http://fink.thetis.ig42.org), [Homebrew](http://mxcl.github.io/homebrew/),
[MacPorts](http://www.macports.org), or [pkgsrc](http://www.pkgsrc.org/).

Install gettext and ncurses. Also install libsdl and libsdl\_ttf if you're
not using frameworks as above.

Without a package manager, building from source isn't too difficult;
follow instructions in the source archives.

## Build

From the Cataclysm-DDA source folder:

    $ export CXXFLAGS="-I/path/to/include" LDFLAGS="-L/path/to/lib"
    
This adds search paths to find shared libraries outside of /usr and /usr/local.
Multiple `-I/...` or `-L/...` paths can be specified.

    $ make NATIVE=osx RELEASE=1 TILES=1 FRAMEWORK=1 OSX_MIN=10.6

## Run

    $ ./cataclysm

or

    $ ./cataclysm-tiles

### Options

`NATIVE=osx` builds for OS X.

`RELEASE=1` builds an optimized 'release' version.

`TILES=1` builds the SDL version; omit to build the ncurses/console version.

`FRAMEWORK=1` uses frameworks; omit for libsdl, libsdl\_ttf.

`OSX_MIN=version` sets `-mmacosx-version-min=` (mine needs to be 10.6); omit for 10.5.

### Archive

    $ make bindist NATIVE=osx TILES=1

Create a .tar.gz archive of the build; omit `TILES=1` for the console version.
