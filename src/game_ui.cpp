#include "game_ui.h"

#include <memory>

#include "game.h"

void game_ui::init_ui()
{
    g->init_ui( true );
}

#if !defined(TILES)

void reinitialize_framebuffer()
{
    return;
}

void to_map_font_dim_width( int & )
{
    return;
}

void to_map_font_dim_height( int & )
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
