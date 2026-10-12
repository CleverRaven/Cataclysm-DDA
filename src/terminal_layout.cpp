#include "terminal_layout.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game_constants.h"

namespace terminal_layout
{

bool is_portrait( const point &window )
{
    return window.y > window.x;
}

int portrait_reserved_bottom( const int window_height, const int reserve_percent,
                              const int shortcut_strip )
{
    return std::max( window_height * reserve_percent / 100, shortcut_strip );
}

const std::vector<point> &text_size_presets()
{
    static const std::vector<point> presets = {
        point( 6, 12 ), point( 8, 14 ), point( 8, 16 ), point( 10, 18 ), point( 10, 20 ),
        point( 11, 22 ), point( 12, 24 ), point( 14, 28 ), point( 16, 32 )
    };
    return presets;
}

std::string text_size_id( const point &cell )
{
    return std::to_string( cell.x ) + "x" + std::to_string( cell.y );
}

std::optional<point> parse_text_size( const std::string &id )
{
    for( const point &cell : text_size_presets() ) {
        if( text_size_id( cell ) == id ) {
            return cell;
        }
    }
    return std::nullopt;
}

point custom_text_size( const int height )
{
    const int h = std::max( height, 1 );
    return point( ( h + 1 ) / 2, h );
}

int narrow_side( const point &area )
{
    return std::min( area.x, area.y );
}

int sizing_reference( const point &window, const point &outer )
{
    if( outer.x > 0 && outer.y > 0 ) {
        return narrow_side( outer );
    }
    return narrow_side( window );
}

point auto_text_size( const int narrow )
{
    point best = text_size_presets().front();
    for( const point &cell : text_size_presets() ) {
        if( cell.x * EVEN_MINIMUM_TERM_WIDTH <= narrow ) {
            best = cell;
        }
    }
    return best;
}

point resolve_text_size( const std::string &choice, const int custom_height, const int narrow )
{
    if( choice == "custom" ) {
        return custom_text_size( custom_height );
    }
    const std::optional<point> preset = parse_text_size( choice );
    return preset ? *preset : auto_text_size( narrow );
}

point fit_grid( const point &bounds, const point &cell, const point &caps )
{
    point grid( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT );
    if( cell.x > 0 && cell.y > 0 ) {
        grid = point( std::max( bounds.x, 0 ) / cell.x, std::max( bounds.y, 0 ) / cell.y );
    }
    if( caps.x > 0 ) {
        grid.x = std::min( grid.x, caps.x );
    }
    if( caps.y > 0 ) {
        grid.y = std::min( grid.y, caps.y );
    }
    return point( std::clamp( grid.x, EVEN_MINIMUM_TERM_WIDTH, MAXIMUM_TERM_WIDTH ),
                  std::clamp( grid.y, EVEN_MINIMUM_TERM_HEIGHT, MAXIMUM_TERM_HEIGHT ) );
}

int bitmap_scale( const int narrow, const int cell_width )
{
    if( cell_width <= 0 ) {
        return 1;
    }
    return std::max( 1, narrow / ( cell_width * EVEN_MINIMUM_TERM_WIDTH ) );
}

point present_size( const point &buffer, const point &bounds, const int scale )
{
    if( buffer.x <= 0 || buffer.y <= 0 || bounds.x <= 0 || bounds.y <= 0 ) {
        return point::zero;
    }
    for( int k = std::max( scale, 1 ); k >= 1; --k ) {
        if( buffer.x * k <= bounds.x && buffer.y * k <= bounds.y ) {
            return point( buffer.x * k, buffer.y * k );
        }
    }
    if( static_cast<int64_t>( bounds.x ) * buffer.y < static_cast<int64_t>( bounds.y ) * buffer.x ) {
        return point( bounds.x, static_cast<int>( static_cast<int64_t>( bounds.x ) * buffer.y /
                      buffer.x ) );
    }
    return point( static_cast<int>( static_cast<int64_t>( bounds.y ) * buffer.x / buffer.y ),
                  bounds.y );
}

} // namespace terminal_layout
