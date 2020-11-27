#include "cata_assert.h"
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

void map_memory::prepare_region( const tripoint &p1, const tripoint &p2 )
{
    cata_assert( p1.z == p2.z );
    cata_assert( p1.x <= p2.x && p1.y <= p2.y );

    tripoint sm_pos = coord_pair( p1 ).sm - point( 1, 1 );
    point sm_size = ( coord_pair( p2 ).sm - sm_pos ).xy() + point( 1, 1 );
    if( ( sm_pos == cache_pos ) && ( sm_size == cache_size ) ) {
        return;
    }

    cache_pos = sm_pos;
    cache_size = sm_size;
    cached.clear();
    cached.reserve( static_cast<std::size_t>( cache_size.x ) * cache_size.y );
    for( int dy = 0; dy < cache_size.y; dy++ ) {
        for( int dx = 0; dx < cache_size.x; dx++ ) {
            cached.push_back( prepare_submap( cache_pos + point( dx, dy ) ) );
        }
    }
}

shared_ptr_fast<memorized_submap> map_memory::prepare_submap( const tripoint &sm_pos )
{
    auto sm = submaps.find( sm_pos );
    if( sm == submaps.end() ) {
        // TODO: load memorized submaps from disk
        shared_ptr_fast<memorized_submap> new_sm = make_shared_fast<memorized_submap>();
        submaps.insert( std::make_pair( sm_pos, new_sm ) );
        return new_sm;
    } else {
        return sm->second;
    }
}

static memorized_submap null_mz_submap;

const memorized_submap &map_memory::get_submap( const tripoint &sm_pos ) const
{
    point idx = ( sm_pos - cache_pos ).xy();
    if( idx.x > 0 && idx.y > 0 && idx.x < cache_size.x && idx.y < cache_size.y ) {
        return *cached[idx.y * cache_size.x + idx.x];
    } else {
        return null_mz_submap;
    }
}

memorized_submap &map_memory::get_submap( const tripoint &sm_pos )
{
    point idx = ( sm_pos - cache_pos ).xy();
    if( idx.x > 0 && idx.y > 0 && idx.x < cache_size.x && idx.y < cache_size.y ) {
        return *cached[idx.y * cache_size.x + idx.x];
    } else {
        return null_mz_submap;
    }
}
