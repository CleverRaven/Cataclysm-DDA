#if defined(TILES)

#include "pixel_minimap_projectors.h"

#include "game_constants.h"


pixel_minimap_ortho_projector::pixel_minimap_ortho_projector(
    const point &total_tiles_count,
    const SDL_Rect &max_screen_rect,
    bool square_pixels )
{
    tile_size.x = std::max( max_screen_rect.w / total_tiles_count.x, 1 );
    tile_size.y = std::max( max_screen_rect.h / total_tiles_count.y, 1 );

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }
}

SDL_Rect pixel_minimap_ortho_projector::get_chunk_rect(
    const point &p,
    const point &tiles_count ) const
{
    return {
        p.x * tile_size.x,
        p.y * tile_size.y,
        tiles_count.x * tile_size.x,
        tiles_count.y *tile_size.y
    };
}

point pixel_minimap_ortho_projector::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_ortho_projector::get_tiles_size( const point &tiles_count ) const
{
    return {
        tiles_count.x * tile_size.x,
        tiles_count.y *tile_size.y
    };
}

point pixel_minimap_ortho_projector::get_tile_pos( const point &p,
        const point &/*tiles_count*/ ) const
{
    return { p.x * tile_size.x, p.y * tile_size.y };
}


pixel_minimap_iso_projector::pixel_minimap_iso_projector(
    const point &total_tiles_count,
    const SDL_Rect &max_screen_rect,
    bool square_pixels ) :

    total_tiles_count( total_tiles_count )
{
    tile_size.x = std::max( max_screen_rect.w / ( 2 * total_tiles_count.x - 1 ), 2 );
    tile_size.y = std::max( max_screen_rect.h / total_tiles_count.y, 2 );

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }
}

SDL_Rect pixel_minimap_iso_projector::get_chunk_rect(
    const point &p,
    const point &tiles_count ) const
{
    const auto size = get_tiles_size( tiles_count );
    const auto offset = point{ 0, tile_size.y *tiles_count.y / 2 };
    const auto pos = get_tile_pos( p, total_tiles_count ) - offset;

    return { pos.x, pos.y, size.x, size.y };
}

point pixel_minimap_iso_projector::get_tile_size() const
{
    return tile_size;
}

point pixel_minimap_iso_projector::get_tiles_size( const point &tiles_count ) const
{
    return {
        tile_size.x *( 2 * tiles_count.x - 1 ),
        tile_size.y *tiles_count.y
    };
}

point pixel_minimap_iso_projector::get_tile_pos( const point &p, const point &tiles_count ) const
{
    return {
        tile_size.x *( p.x + p.y ),
        tile_size.y *( tiles_count.y + p.y - p.x - 1 ) / 2,
    };
}

#endif // TILES
