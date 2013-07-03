### Instructions for compiling on Mac OSX with SDL frameworks

Tested with OS X 10.8.4, Xcode 4.6.2. I haven't played it extensively but it builds and runs. Not sure if it compiles with older OS/XCode versions.

This worked for me, your mileage may vary.

(The font looks a little weird in the SDL version. Not sure why).

### Prerequisites:

- ncursesw and gettext. Use fink, homebrew, or macports, or build them (see below).
- SDL framework: http://www.libsdl.org/download-1.2.php  
- SDL\_ttf framework: http://www.libsdl.org/projects/SDL_ttf/

#### Install frameworks

Copy SDL.framework and SDL\_ttf.framework to /Library/Frameworks
or /Users/name/Library/Frameworks.

#### Build ncurses and gettext

The --prefix=$HOME/opt/catdda paths below are just examples.  
If --prefix= is omitted, the prefix will be /usr/local, and 'make install' should be replaced with 'sudo make install'. 

ncurses - latest is 5.9: http://ftp.gnu.org/pub/gnu/ncurses/

From the ncurses source folder:

    $ ./configure --prefix=$HOME/opt/catdda --enable-widec --enable-ext-colors --enable-sigwinch
    $ make
    $ make install

gettext - latest is 0.18.2: http://www.gnu.org/software/gettext/

From the gettext source folder:

    $ ./configure --prefix=$HOME/opt/catdda --with-libncurses-prefix=$HOME/opt/catdda --disable-java --disable-native-java
    $ make
    $ make install

- You may see prompts asking if you want to install Java; dismiss them.

### Build Cataclysm-DDA

From the Cataclysm-DDA source folder:
11
    $ make NATIVE=osx TILES=1 RELEASE=1 OSX_MIN=10.6 LIBEXT=1 LIBEXT_DIR=~/opt/catdda
    $ ./cataclysm
    (or)
    $ ./cataclysm-tiles

Options:

    NATIVE=osx  
Build for os x.

    TILES=1  
Build the SDL version, omit for ncurses/console version.

    RELEASE=1  
Build an optimized 'release' version.

    OSX_MIN=version  
Setxs -mmacosx-version-min= (mine needs to be 10.6 to compile), omit for 10.5.

    LIBEXT=1  
Search for libs somewhere else (defaults to /usr/local)

    LIBEXT_DIR=~/opt/catdda  
Path to search for libs. This should be where ncursesw and gettext are installed. I use ~/opt/catdda to keep it out of the way of other stuff.
