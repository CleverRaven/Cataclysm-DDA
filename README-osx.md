### Building Cataclysm-DDA

Tested with OS X 10.8.4, Xcode 4.6.2.  
I haven't played it extensively but it builds and runs.  
Not sure if it compiles with older OS/XCode versions.

#### how-to

Install SDL.framework and SDL\_ttf.framework in /Library/Frameworks
or /Users/name/Library/Frameworks.

SDL: http://www.libsdl.org/download-1.2.php  
SDL\_ttf: http://www.libsdl.org/projects/SDL_ttf/

    $ make NATIVE=macosx TILES=1
    $ ./cataclysm-tiles

builds the tiles/SDL version, or

    $ make NATIVE=macosx
    $ ./cataclysm

builds the terminal/ncurses version.
