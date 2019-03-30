#include "game_ui.h"

#include "game.h"

void game_ui::init_ui()
{
    g->init_ui( true );
}

#if ( !defined TILES )

void reinitialize_framebuffer()
{
    return;
}
void to_map_font_dimension( int &, int & )
{
    return;
}
void from_map_font_dimension( int &, int & )
{
    return;
}
void to_overmap_font_dimension( int &, int & )
{
    return;
}

#endif
