### Building Cataclysm-DDA

tested with OS X 10.8.4, Xcode 4.6.2 
I haven't played it extensively but it builds and runs.
Not sure if it compiles with older versions of OS X.

#### how-to:

install SDL.framework and SDL_ttf.framework in /Library/Frameworks or /Users/name/Library/Frameworks

SDL: http://www.libsdl.org/download-1.2.php

SDL_ttf: http://www.libsdl.org/projects/SDL_ttf/

    $ make NATIVE=macosx TILES=1
    $ ./cataclysm-tiles

or

    $ make NATIVE=macosx
    $ ./cataclysm

for the ncurses build.
