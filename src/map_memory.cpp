#include "map_memory.h"

static const memorized_terrain_tile default_tile{ "", 0, 0 };

memorized_terrain_tile map_memory::get_tile( const tripoint &pos ) const
{
    return tile_cache.get( pos, default_tile );
}

void map_memory::memorize_tile( int limit, const tripoint &pos, const std::string &ter,
                                const int subtile, const int rotation )
{
    tile_cache.insert( limit, pos, memorized_terrain_tile{ ter, subtile, rotation } );
}

int map_memory::get_symbol( const tripoint &pos ) const
{
    return symbol_cache.get( pos, 0 );
}

void map_memory::memorize_symbol( int limit, const tripoint &pos, const int symbol )
{
    symbol_cache.insert( limit, pos, symbol );
}

void map_memory::clear_memorized_tile( const tripoint &pos )
{
    tile_cache.remove( pos );
    symbol_cache.remove( pos );
}
