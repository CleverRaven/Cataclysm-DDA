# Building Cataclysm-DDA on Mac OS X

To build Cataclysm on Mac you'll have to get XCode with command line tools (or just download them separately from https://developer.apple.com/downloads/) and Homebrew package manager.


### SDL

SDL, SDL\_image, and SDL\_ttf are needed for the tiles build.

Option (1):
[**SDL framework**](http://www.libsdl.org/download-1.2.php)  
http://www.libsdl.org/release/SDL-1.2.15.dmg

[**SDL\_image framework**](http://www.libsdl.org/projects/SDL_image/)  
http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12.dmg

[**SDL\_ttf framework**](http://www.libsdl.org/projects/SDL_ttf/)  
http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

Copy `SDL.framework`, `SDL_image.framework`, and `SDL_ttf.framework`
to `/Library/Frameworks` or `/Users/name/Library/Frameworks`.

Option (2):
Alternately, shared libraries (libsdl, libsdl\_image, libsdl\_ttf) can be used
instead of frameworks. Install with a package manager (Fink, MacPorts,
Homebrew, pkgsrc) or build and install from source.

For Homebrew:
`brew install sdl sdl_image sdl_ttf`

### ncurses, gettext

ncurses and gettext are needed for localization.
Install with a package manager, or build from source and install.

  * ncurses needs wide character support enabled.

For Homebrew:
`brew tap homebrew/dupes`
`brew install gettext ncurses`
`brew link --force gettext ncurses`

  * After you build Cataclysm remember to unlink gettext and ncurses with `brew unlink gettext ncurses` if you build other software, it might conflict with OSX versions. 

### Example builds:

Build a release version with SDL graphical tiles:

    $ make FRAMEWORK=1 NATIVE=osx OSX_MIN=10.6 RELEASE=1 TILES=1 LOCALIZE=0

Build SDL version with shared libraries:
   
   $ make NATIVE=osx OSX_MIN=10.6 RELEASE=1 TILES=1 LOCALIZE=0

Build a release console version without localization:

   $ make NATIVE=osx OSX_MIN=10.6 RELEASE=1 LOCALIZE=0

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

### Application bundle packaging:

Create new folder and name it `Cataclysm.app`.

Put compiled binaries (`./cataclysm-tiles` and/or `./cataclysm`) with `./gfx/` and `./data/` folders inside `/Cataclysm.app/Contents/Resources/`.

To bundle SDL libs copy `SDL.framework`, `SDL_image.framework`, and `SDL_ttf.framework` to `/Cataclysm.app/Contents/Resources/libs/` or shared libs homebrew installed from `/usr/local/Cellar/sdl*/version/lib/`.

Create folder `/Cataclysm.app/Contents/MacOS` and file ./Cataclysm within it with this content:

#!/bin/sh
PWD=`dirname "${0}"`
OSREV=`uname -r | cut -d. -f1`
if [ "$OSREV" -ge 11 ] ; then
   export DYLD_LIBRARY_PATH=${PWD}/../Resources/libs
   export DYLD_FRAMEWORK_PATH=${PWD}/../Resources/libs
else
   export DYLD_FALLBACK_LIBRARY_PATH=${PWD}/../Resources/libs
   export DYLD_FALLBACK_FRAMEWORK_PATH=${PWD}/../Resources/libs
fi
cd "${PWD}/../Resources/"; ./cataclysm-tiles


### Creating a DMG
Create an new folder named Cataclysm

Move your Cataclysm.app into it

Start Disk Utility

File / New -> Disk Image From Folder

Select the Cataclysm folder you created above.

Done!


# Troubleshooting

ISSUE: crash on startup due to libint.8.dylib aborting

Basically if you're compiling on Mountain Lion or above, it won't be possible to run successfully on older OS X versions due to libint.8 / pthreads version issue.

See below (quoted form https://wiki.gnome.org/GTK+/OSX/Building)

"There's another issue with building on Lion or Mountain Lion using either "native" or the 10.7 SDK: Apple has updated the pthreads implementation to provide recursive locking. This would be good except that Gettext's libintl uses this and if the pthreads implementation doesn't provide it it fabricates its own. Since the Lion pthreads does provide it, libintl links the provided function and then crashes when you try to run it against an older version of the library. The simplest solution is to specify the 10.6 SDK when building on Lion, but that won't work on Mountain Lion, which doesn't include it. See below for how to install and use XCode 3 on Lion and later for building applications compatible with earlier versions of OSX."

Workaround: install XCode 3 like that article describes, or disable localization support in Cataclysm so gettext/libint are not dependencies. Or else simply don't support OS X versions below 10.7. 

ISSUE: Colours don't show up correctly.

Open Terminal's preferences, turn on "Use bright colors for bold text" in "Preferences -> Settings -> Text"

