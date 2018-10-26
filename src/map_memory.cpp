#include "map_memory.h"

#include <algorithm>

void map_memory::trim( int limit )
{
    while( tiles.size() > static_cast<size_t>( limit ) ) {
        tile_map.erase( tiles.back().first );
        tiles.pop_back();
    }
    while( symbols.size() > static_cast<size_t>( limit ) ) {
        symbol_map.erase( symbols.back().first );
        symbols.pop_back();
    }
}

memorized_terrain_tile map_memory::get_tile( const tripoint &pos ) const
{
    auto found_tile = tile_map.find( pos );
    if( found_tile != tile_map.end() ) {
        return found_tile->second->second;
    }
    return { "", 0, 0 };
}

void map_memory::memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                                const int subtile, const int rotation )
{
    memorized_terrain_tile new_tile{ ter, subtile, rotation };
    tiles.push_front( std::make_pair( pos, new_tile ) );
    auto found_tile = tile_map.find( pos );
    if( found_tile != tile_map.end() ) {
        // Remove redundant entry since we pushed one to the front.
        tiles.erase( found_tile->second );
        found_tile->second = tiles.begin();
    } else {
        tile_map[pos] = tiles.begin();
        trim( limit );
    }
}

long map_memory::get_symbol( const tripoint &pos ) const
{
    auto found_tile = symbol_map.find( pos );
    if( found_tile != symbol_map.end() ) {
        return found_tile->second->second;
    }
    return 0;
}

void map_memory::memorize_symbol( int limit, const tripoint &pos, const long symbol )
{
    symbols.emplace_front( pos, symbol );
    auto found_tile = symbol_map.find( pos );
    if( found_tile != symbol_map.end() ) {
        // Remove redundant entry since we pushed on to the front.
        symbols.erase( found_tile->second );
        found_tile->second = symbols.begin();
    } else {
        symbol_map[pos] = symbols.begin();
        trim( limit );
    }
}
