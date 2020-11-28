#include "cata_assert.h"
#include "cached_options.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "cuboid_rectangle.h"
#include "filesystem.h"
#include "game.h"
#include "line.h"
#include "map_memory.h"
#include "path_info.h"

static const memorized_terrain_tile default_tile{ "", 0, 0 };
static const int default_symbol = 0;

#define MM_SIZE (MAPSIZE * 2)

static std::string find_legacy_mm_file()
{
    return PATH_INFO::player_base_save_path() + SAVE_EXTENSION_MAP_MEMORY;
}

static std::string find_mm_dir()
{
    return string_format( "%s.mm1", PATH_INFO::player_base_save_path() );
}

static std::string find_submap_path( const std::string &dirname, const tripoint &p )
{
    return string_format( "%s/%d.%d.%d.mm", dirname, p.x, p.y, p.z );
}

memorized_submap::memorized_submap() : clean( true ),
    tiles{{ default_tile }}, symbols{{ default_symbol }} {}

map_memory::coord_pair::coord_pair( const tripoint &p ) : loc( p.xy() )
{
    sm = tripoint( ms_to_sm_remain( loc.x, loc.y ), p.z );
}

map_memory::map_memory()
{
    clear_cache();
}

const memorized_terrain_tile &map_memory::get_tile( const tripoint &pos ) const
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
    sm.clean = false;
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
    sm.clean = false;
    sm.symbols[p.loc.x][p.loc.y] = symbol;
}

void map_memory::clear_memorized_tile( const tripoint &pos )
{
    coord_pair p( pos );
    memorized_submap &sm = get_submap( p.sm );
    sm.symbols[p.loc.x][p.loc.y] = default_symbol;
    sm.tiles[p.loc.x][p.loc.y] = default_tile;
}

bool map_memory::prepare_region( const tripoint &p1, const tripoint &p2 )
{
    cata_assert( p1.z == p2.z );
    cata_assert( p1.x <= p2.x && p1.y <= p2.y );

    tripoint sm_pos = coord_pair( p1 ).sm - point( 1, 1 );
    point sm_size = ( coord_pair( p2 ).sm - sm_pos ).xy() + point( 1, 1 );
    if( sm_pos.z == cache_pos.z ) {
        half_open_rectangle<point> half_rect( cache_pos.xy(), cache_pos.xy() + cache_size );
        inclusive_rectangle<point> incl_rect( cache_pos.xy(), cache_pos.xy() + cache_size );
        if( half_rect.contains( sm_pos.xy() ) && incl_rect.contains( sm_pos.xy() + sm_size ) ) {
            return false;
        }
    }

    cache_pos = sm_pos;
    cache_size = sm_size;
    cached.clear();
    cached.reserve( static_cast<std::size_t>( cache_size.x ) * cache_size.y );
    for( int dy = 0; dy < cache_size.y; dy++ ) {
        for( int dx = 0; dx < cache_size.x; dx++ ) {
            cached.push_back( fetch_submap( cache_pos + point( dx, dy ) ) );
        }
    }
    return true;
}

shared_ptr_fast<memorized_submap> map_memory::fetch_submap( const tripoint &sm_pos )
{
    auto sm = submaps.find( sm_pos );
    if( sm == submaps.end() ) {
        shared_ptr_fast<memorized_submap> sm1 = load_submap( sm_pos );
        if( !sm1 ) {
            sm1 = allocate_submap();
        }
        submaps.insert( std::make_pair( sm_pos, sm1 ) );
        return sm1;
    } else {
        return sm->second;
    }
}

shared_ptr_fast<memorized_submap> map_memory::allocate_submap()
{
    return make_shared_fast<memorized_submap>();
}

shared_ptr_fast<memorized_submap> map_memory::load_submap( const tripoint &sm_pos )
{
    if( test_mode ) {
        return nullptr;
    }

    const std::string dirname = find_mm_dir();
    const std::string path = find_submap_path( dirname, sm_pos );

    if( !dir_exist( dirname ) ) {
        // Old saves don't have [plname].mm1 folder
        return nullptr;
    }

    shared_ptr_fast<memorized_submap> sm = nullptr;
    const auto loader = [&]( JsonIn & jsin ) {
        // Don't allocate submap unless we know its file exists
        sm = allocate_submap();
        sm->clean = false;
        sm->deserialize( jsin );
    };

    try {
        if( read_from_file_optional_json( path, loader ) ) {
            return sm;
        }
    } catch( const std::exception &err ) {
        debugmsg( "Failed to load memory submap (%d,%d,%d): %s",
                  sm_pos.x, sm_pos.y, sm_pos.z, err.what() );
    }

    return nullptr;
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

void map_memory::load( const tripoint &pos )
{
    const std::string dirname = find_mm_dir();

    clear_cache();

    if( !dir_exist( dirname ) ) {
        // Old saves have [plname].mm file and no [plname].mm1 folder
        const std::string legacy_file = find_legacy_mm_file();
        if( file_exist( legacy_file ) ) {
            try {
                read_from_file_optional_json( legacy_file, [&]( JsonIn & jsin ) {
                    this->load_legacy( jsin );
                } );
            } catch( const std::exception &err ) {
                debugmsg( "Failed to load legacy memory map file: %s", err.what() );
            }
        }
        return;
    }

    coord_pair p( pos );
    tripoint start = p.sm - tripoint( MM_SIZE / 2, MM_SIZE / 2, 0 );
    for( int dy = 0; dy < MM_SIZE; dy++ ) {
        for( int dx = 0; dx < MM_SIZE; dx++ ) {
            fetch_submap( start + tripoint( dx, dy, 0 ) );
        }
    }
}

bool map_memory::save( const tripoint &pos )
{
    tripoint sm_center = coord_pair( pos ).sm;
    const std::string dirname = find_mm_dir();
    assure_dir_exist( dirname );

    clear_cache();

    bool result = true;

    for( auto it = submaps.cbegin(); it != submaps.cend(); ) {
        const tripoint &sm_pos = it->first;
        if( !it->second->clean ) {
            const std::string path = find_submap_path( dirname, sm_pos );
            const std::string descr = string_format(
                                          _( "player map memory for (%d,%d,%d)" ),
                                          sm_pos.x, sm_pos.y, sm_pos.z
                                      );

            const auto writer = [&]( std::ostream & fout ) -> void {
                fout << serialize_wrapper( [&]( JsonOut & jsout )
                {
                    it->second->serialize( jsout );
                } );
            };

            const bool res = write_to_file( path, writer, descr.c_str() );
            result = result & res;
        }
        if( square_dist( sm_center, sm_pos ) > MM_SIZE ) {
            it = submaps.erase( it );
        } else {
            ++it;
        }
    }

    return result;
}

void map_memory::clear_cache()
{
    cached.clear();
    cache_pos = tripoint_min;
    cache_size = point_zero;
}
