#include "pixel_minimap_drawers.h"

#include "game_constants.h"
#include "sdl_utils.h"


pixel_minimap_ortho_drawer::pixel_minimap_ortho_drawer(
    const point &tiles_count,
    const point &max_screen_size,
    bool square_pixels )
{
    tile_size.x = std::max( max_screen_size.x / tiles_count.x, 1 );
    tile_size.y = std::max( max_screen_size.y / tiles_count.y, 1 );

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }
}

SDL_Rect pixel_minimap_ortho_drawer::get_chunk_rect( const point &p,
        const point &/*tiles_count*/ ) const
{
    return {
        p.x * tile_size.x,
        p.y * tile_size.y,
        SEEX * tile_size.x,
        SEEY *tile_size.y
    };
}

point pixel_minimap_ortho_drawer::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_ortho_drawer::get_tiles_size( const point &tiles_count ) const
{
    return {
        tiles_count.x * tile_size.x,
        tiles_count.y *tile_size.y
    };
}

point pixel_minimap_ortho_drawer::get_tile_pos( const point &p, const point &/*tiles_count*/ ) const
{
    return { p.x * tile_size.x, p.y * tile_size.y };
}


pixel_minimap_iso_drawer::pixel_minimap_iso_drawer(
    const point &tiles_count,
    const point &max_screen_size,
    bool square_pixels )
{
    // Tile size must be an even number.
    tile_size.x = 2 * std::max( max_screen_size.x / ( 2 * tiles_count.x - 1 ), 2 ) / 2;
    tile_size.y = 2 * std::max( max_screen_size.y / tiles_count.y, 2 ) / 2;

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }
}

SDL_Rect pixel_minimap_iso_drawer::get_chunk_rect( const point &p, const point &tiles_count ) const
{
    const auto size = get_tiles_size( { SEEX, SEEY } );
    const auto offset = point{ 0, tile_size.y *SEEY / 2 };
    const auto pos = get_tile_pos( p, tiles_count ) - offset;

    return { pos.x, pos.y, size.x, size.y };
}

point pixel_minimap_iso_drawer::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_iso_drawer::get_tiles_size( const point &tiles_count ) const
{
    return {
        tile_size.x *( 2 * tiles_count.x - 1 ),
        tile_size.y *tiles_count.y
    };
}

point pixel_minimap_iso_drawer::get_tile_pos( const point &p, const point &tiles_count ) const
{
    return {
        tile_size.x *( p.x + p.y ),
        tile_size.y *( tiles_count.y + p.y - p.x - 1 ) / 2,
    };
}
