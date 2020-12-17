#include "cursesdef.h"
#include "debug.h"
#include "game.h"

[[ noreturn ]]
void exit_handler( int status )
{
    deinitDebug();
    g.reset();
    catacurses::endwin();
    exit( status );
}
