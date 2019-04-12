#include "pixel_minimap_drawers.h"

#include "game_constants.h"
#include "sdl_utils.h"


const point tiles_range = { ( MAPSIZE - 2 ) *SEEX, ( MAPSIZE - 2 ) *SEEY };


pixel_minimap_ortho_drawer::pixel_minimap_ortho_drawer(
    const point &max_size,
    bool square_pixels,
    pixel_minimap_mode mode )
{
    tile_size.x = std::max( max_size.x / tiles_range.x, 1 );
    tile_size.y = std::max( max_size.y / tiles_range.y, 1 );

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }

    tiles_count.x = std::min( max_size.x / tile_size.x, tiles_range.x );
    tiles_count.y = std::min( max_size.y / tile_size.y, tiles_range.y );

    draw_with_dots = false;

    switch( mode ) {
        case pixel_minimap_mode::solid:
            pixel_size = tile_size;
            break;

        case pixel_minimap_mode::squares:
            pixel_size = { std::max( tile_size.x - 1, 1 ), std::max( tile_size.y - 1, 1 ) };
            draw_with_dots = pixel_size.x == 1 && pixel_size.y == 1;
            break;

        case pixel_minimap_mode::dots:
            draw_with_dots = true;
            break;
    }
}

point pixel_minimap_ortho_drawer::get_size_on_screen() const
{
    return {
        tiles_count.x * tile_size.x,
        tiles_count.y *tile_size.y
    };
}

point pixel_minimap_ortho_drawer::get_tiles_count() const
{
    return tiles_count;
}

SDL_Rect pixel_minimap_ortho_drawer::get_chunk_rect( const point &p ) const
{
    return {
        p.x * tile_size.x,
        p.y * tile_size.y,
        SEEX * tile_size.x,
        SEEY *tile_size.y
    };
}

SDL_Rect pixel_minimap_ortho_drawer::get_critter_rect( const point &p ) const
{
    return {
        p.x * tile_size.x,
        p.y * tile_size.y,
        tile_size.x,
        tile_size.y
    };
}

void pixel_minimap_ortho_drawer::clear_chunk_tex( const SDL_Renderer_Ptr &renderer )
{
    SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF );
    RenderClear( renderer );
}

void pixel_minimap_ortho_drawer::update_chunk_tex(
    const SDL_Renderer_Ptr &renderer,
    const point &p,
    const SDL_Color &c )
{
    SetRenderDrawColor( renderer, c.r, c.g, c.b, c.a );

    if( draw_with_dots ) {
        RenderDrawPoint( renderer, p.x * tile_size.x, p.y * tile_size.y );
    } else {
        const auto rect = SDL_Rect{
            p.x * tile_size.x,
            p.y * tile_size.y,
            pixel_size.x,
            pixel_size.y
        };

        render_fill_rect( renderer, rect, c.r, c.g, c.b );
    }
}
