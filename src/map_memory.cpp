#include "coordinate_conversions.h"
#include "map_memory.h"

static const memorized_terrain_tile default_tile{ "", 0, 0 };
static const int default_symbol = 0;

memorized_submap::memorized_submap() : tiles{{ default_tile }}, symbols{{ default_symbol }} {}

struct coord_pair {
    tripoint sm;
    point loc;

    coord_pair( const tripoint &p ) : loc( p.xy() ) {
        sm = tripoint( ms_to_sm_remain( loc.x, loc.y ), p.z );
    }
};

memorized_terrain_tile map_memory::get_tile( const tripoint &pos ) const
{
    coord_pair p( pos );
    const memorized_submap &sm = get_submap( p.sm );
    return sm.tiles[p.loc.x][p.loc.y];
}

void map_memory::memorize_tile( const tripoint &pos, const std::string &ter,
                                const int subtile, const int rotation )
{
    coord_pair p( pos );
    memorized_submap &sm = get_submap( p.sm );
    sm.tiles[p.loc.x][p.loc.y] = memorized_terrain_tile{ ter, subtile, rotation };
}

int map_memory::get_symbol( const tripoint &pos ) const
{
    coord_pair p( pos );
    const memorized_submap &sm = get_submap( p.sm );
    return sm.symbols[p.loc.x][p.loc.y];
}

void map_memory::memorize_symbol( const tripoint &pos, const int symbol )
{
    coord_pair p( pos );
    memorized_submap &sm = get_submap( p.sm );
    sm.symbols[p.loc.x][p.loc.y] = symbol;
}

void map_memory::clear_memorized_tile( const tripoint &pos )
{
    coord_pair p( pos );
    memorized_submap &sm = get_submap( p.sm );
    sm.symbols[p.loc.x][p.loc.y] = default_symbol;
    sm.tiles[p.loc.x][p.loc.y] = default_tile;
}

const memorized_submap &map_memory::get_submap( const tripoint &sm_pos ) const
{
    std::map<tripoint, memorized_submap>::const_iterator sm = submaps.find( sm_pos );
    if( sm == submaps.end() ) {
        const static memorized_submap null_sm;
        return null_sm;
    } else {
        return sm->second;
    }
}

memorized_submap &map_memory::get_submap( const tripoint &sm_pos )
{
    return submaps[sm_pos];
}
