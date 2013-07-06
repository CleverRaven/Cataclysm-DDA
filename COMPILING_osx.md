### Instructions for compiling on Mac OSX with SDL frameworks

Tested with OS X 10.8.4, Xcode 4.6.2. I haven't played it extensively but it builds and runs.  
Not sure if it compiles with older OS/XCode versions.

This worked for me, your mileage may vary.

(The font looks a little weird in the SDL version. Not sure why).

todo: make it work with sdl libs, i.e. non-framework sdl installed via macports, homebrew... this would be most similar to the linux build, just different locations for search paths.

#### Prerequisites:

- ncursesw and gettext. Use fink, homebrew, or macports, or build them (see below).
- **SDL framework**: http://www.libsdl.org/release/SDL-1.2.15.dmg  
- **SDL\_ttf framework**: http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.11.dmg

#### Install frameworks

Copy SDL.framework and SDL\_ttf.framework to /Library/Frameworks
or /Users/name/Library/Frameworks.

#### Build ncurses and gettext

The `--prefix=$HOME/opt/catdda` paths below are just examples. If `--prefix=` is omitted, the prefix will be /usr/local, and 'make install' should be replaced with 'sudo make install'. 

**ncurses**: http://ftp.gnu.org/pub/gnu/ncurses/ncurses-5.9.tar.gz

From the ncurses source folder:

    $ ./configure --prefix=$HOME/opt/catdda --enable-widec --enable-ext-colors --enable-sigwinch
    $ make
    $ make install

**gettext**: http://ftp.gnu.org/pub/gnu/gettext/gettext-0.18.2.tar.gz

From the gettext source folder:

    $ ./configure --prefix=$HOME/opt/catdda --with-libncurses-prefix=$HOME/opt/catdda
    $ make
    $ make install
    
- `--with-libncurses-prefix=` is where ncurses was installed above, or if you already have it installed elsewhere, as long as it has wide character support.
- You may see prompts asking if you want to install Java; dismiss them.

#### Build Cataclysm-DDA

From the Cataclysm-DDA source folder:

    $ make NATIVE=osx TILES=1 RELEASE=1 OSX_MIN=10.6 LIBEXT=1 LIBEXT_DIR=~/opt/catdda
    $ ./cataclysm
    $ ./cataclysm-tiles

**Options:**

`NATIVE=osx` - Build for os x.

`TILES=1` - Build the SDL version, omit for ncurses/console version.

`RELEASE=1` - Build an optimized 'release' version.

`OSX_MIN=version` - Sets `-mmacosx-version-min=` (mine needs to be 10.6 to compile), omit for 10.5.

`LIBEXT=1` - Search for libs somewhere else (defaults to /usr/local)

`LIBEXT_DIR=~/opt/catdda` - Path to search for libs. This should be where ncursesw and gettext are installed.  
I use `~/opt/catdda` to keep it out of the way.
