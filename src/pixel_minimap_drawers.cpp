#include "pixel_minimap_drawers.h"

#include "game_constants.h"
#include "sdl_utils.h"

namespace
{

const point tiles_range = { MAPSIZE_X, MAPSIZE_Y };

point get_pixel_size( const point &tile_size, pixel_minimap_mode mode )
{
    switch( mode ) {
        case pixel_minimap_mode::solid:
            return tile_size;

        case pixel_minimap_mode::squares:
            return { std::max( tile_size.x - 1, 1 ), std::max( tile_size.y - 1, 1 ) };

        case pixel_minimap_mode::dots:
            return { 1, 1 };
    }

    return {};
}

}


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

    pixel_size = get_pixel_size( tile_size, mode );
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

    if( pixel_size.x == 1 && pixel_size.y == 1 ) {
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


pixel_minimap_iso_drawer::pixel_minimap_iso_drawer(
    const point &max_size,
    bool square_pixels,
    pixel_minimap_mode mode )
{
    tiles_count = tiles_range;

    // Tile size must be an even number.
    tile_size.x = 2 * std::max( max_size.x / ( 2 * tiles_count.x - 1 ), 2 ) / 2;
    tile_size.y = 2 * std::max( max_size.y / tiles_count.y, 2 ) / 2;

    if( square_pixels ) {
        tile_size.x = tile_size.y = std::min( tile_size.x, tile_size.y );
    }

    pixel_size = get_pixel_size( tile_size, mode );
}

point pixel_minimap_iso_drawer::get_size_on_screen() const
{
    return get_tiles_size( tiles_count );
}

point pixel_minimap_iso_drawer::get_tiles_count() const
{
    return tiles_count;
}

SDL_Rect pixel_minimap_iso_drawer::get_chunk_rect( const point &p ) const
{
    const auto size = get_tiles_size( { SEEX, SEEY } );
    const auto offset = point{ 0, tile_size.y *SEEY / 2 };
    const auto pos = get_tile_pos( p, tiles_count ) - offset;

    return { pos.x, pos.y, size.x, size.y };
}

SDL_Rect pixel_minimap_iso_drawer::get_critter_rect( const point &p ) const
{
    const auto tile_pos = get_tile_pos( p, tiles_count );
    return { tile_pos.x, tile_pos.y, tile_size.x, tile_size.y };
}

void pixel_minimap_iso_drawer::clear_chunk_tex( const SDL_Renderer_Ptr &renderer )
{
    SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0x00 );
    RenderClear( renderer );

    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            const auto tile_pos = get_tile_pos( { x, y }, { SEEX, SEEY } );
            const auto rect = SDL_Rect{ tile_pos.x, tile_pos.y, tile_size.x, tile_size.y };

            render_fill_rect( renderer, rect, 0x00, 0x00, 0x00 );
        }
    }
}

void pixel_minimap_iso_drawer::update_chunk_tex(
    const SDL_Renderer_Ptr &renderer,
    const point &p,
    const SDL_Color &c )
{
    const auto tile_pos = get_tile_pos( p, { SEEX, SEEY } );

    if( pixel_size.x == 1 && pixel_size.y == 1 ) {
        SetRenderDrawColor( renderer, c.r, c.g, c.b, c.a );
        RenderDrawPoint( renderer, tile_pos.x, tile_pos.y );
    } else {
        const auto rect = SDL_Rect{ tile_pos.x, tile_pos.y, pixel_size.x, pixel_size.y };
        render_fill_rect( renderer, rect, c.r, c.g, c.b );
    }
}

point pixel_minimap_iso_drawer::get_tile_pos( const point &p, const point &tiles_count ) const
{
    return {
        tile_size.x *( p.x + p.y ),
        tile_size.y *( tiles_count.y + p.y - p.x - 1 ) / 2,
    };
}

point pixel_minimap_iso_drawer::get_tiles_size( const point &tiles_count ) const
{
    return {
        tile_size.x *( 2 * tiles_count.x - 1 ),
        tile_size.y *tiles_count.y
    };
}
