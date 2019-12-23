#include "map_memory.h"

#include <string>

#include "debug.h"
#include "enum_conversions.h"

namespace io
{

template<>
std::string enum_to_string<map_memory_layer>( map_memory_layer data )
{
    switch( data ) {
            // *INDENT-OFF*
        case map_memory_layer::terrain: return "terrain";
        case map_memory_layer::furniture: return "furniture";
        case map_memory_layer::trap: return "trap";
        case map_memory_layer::vpart: return "vpart";
            // *INDENT-ON*
        case map_memory_layer::num_map_memory_layer:
            break;
    }
    debugmsg( "Invalid map_memory_layer" );
    abort();
}

}  // namespace io

static const map_memory_tile default_tile{ "", 0, 0 };

map_memory_tile map_memory::get_tile( const tripoint &pos, const map_memory_layer &layer ) const
{
    return tile_cache.at( layer ).get( pos, default_tile );
}

void map_memory::memorize_tile( int limit, const tripoint &pos, const std::string &tile_name,
                                const int subtile, const int rotation, const map_memory_layer &layer )
{
    tile_cache.at( layer ).insert( limit, pos, map_memory_tile{ tile_name, subtile, rotation } );
}

int map_memory::get_symbol( const tripoint &pos ) const
{
    return symbol_cache.get( pos, 0 );
}

void map_memory::memorize_symbol( int limit, const tripoint &pos, const int symbol )
{
    symbol_cache.insert( limit, pos, symbol );
}

void map_memory::clear_memorized_tile( const tripoint &pos, const map_memory_layer &layer )
{
    tile_cache.at( layer ).remove( pos );
    symbol_cache.remove( pos );
}
