# Building Cataclysm-DDA on Mac OS X

Tested with OS X 10.8.4, Xcode 4.6.2.
I haven't played it extensively but it builds and runs.

Not sure if it compiles with older OS/XCode versions.
This worked for me, your mileage may vary.

### SDL

SDL, SDL\_image, and SDL\_ttf are needed for the tiles build.

[**SDL framework**](http://www.libsdl.org/download-1.2.php)  
http://www.libsdl.org/release/SDL-1.2.15.dmg

[**SDL\_image framework**](http://www.libsdl.org/projects/SDL_image/)  
http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12.dmg

[**SDL\_ttf framework**](http://www.libsdl.org/projects/SDL_ttf/)  
http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

Copy `SDL.framework`, `SDL_image.framework`, and `SDL_ttf.framework`
to `/Library/Frameworks` or `/Users/name/Library/Frameworks`.

Alternately, shared libraries (libsdl, libsdl\_image, libsdl\_ttf) can be used
instead of frameworks. Install with a package manager (Fink, MacPorts,
Homebrew, pkgsrc) or build and install from source.

### ncurses, gettext

ncurses and gettext are needed for localization.
Install with a package manager, or build from source and install.

  * ncurses needs wide character support enabled.

## Build

From the Cataclysm-DDA source folder:

The easiest way to try it out is the console version without localization:

    $ make NATIVE=osx LOCALIZED=0 OSX_MIN=10.6

For localization support and/or to use libsdl (not frameworks):

    $ export CXXFLAGS="-I/path/to/include" LDFLAGS="-L/path/to/lib"
    
This adds paths to find shared libraries outside of /usr and /usr/local.
Multiple `-I/...` or `-L/...` paths can be specified.

### Example builds:

Build a release version with SDL graphical tiles:

    $ make FRAMEWORK=1 NATIVE=osx OSX_MIN=10.6 RELEASE=1 TILES=1 LOCALIZE=0

Build a debug version with SDL ASCII:

    $ make FRAMEWORK=1 NATIVE=osx OSX_MIN=10.6 SDL=1 LOCALIZE=0

## Run

    $ ./cataclysm

or

    $ ./cataclysm-tiles

### Make Options

`FRAMEWORK=1` use frameworks; omit to use libsdl, libsdl\_image, libsdl\_ttf.

`LOCALIZED=0` disable localization; enabled by default (requires gettext and ncurses with wide character support).

`NATIVE=osx` build for OS X.

`OSX_MIN=version` set `-mmacosx-version-min=` (for OS X > 10.5 set it to 10.6 or higher); omit for 10.5.

`RELEASE=1` build an optimized 'release' version.

`SDL=1` build the SDL version with ASCII characters.

`TILES=1` build the SDL version with graphical tiles.
