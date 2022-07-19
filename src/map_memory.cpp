#include "cata_assert.h"
#include "cached_options.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "filesystem.h"
#include "line.h"
#include "map_memory.h"
#include "path_info.h"
#include "string_formatter.h"
#include "translations.h"

const memorized_terrain_tile mm_submap::default_tile{ "", 0, 0 };
const int mm_submap::default_symbol = 0;

#define MM_SIZE (MAPSIZE * 2)

#define dbg(x) DebugLog((x),D_MMAP) << __FILE__ << ":" << __LINE__ << ": "

static std::string find_legacy_mm_file()
{
    return PATH_INFO::player_base_save_path() + SAVE_EXTENSION_MAP_MEMORY;
}

static std::string find_mm_dir()
{
    return string_format( "%s.mm1", PATH_INFO::player_base_save_path() );
}

static std::string find_region_path( const std::string &dirname, const tripoint &p )
{
    return string_format( "%s/%d.%d.%d.mmr", dirname, p.x, p.y, p.z );
}

/**
 * Helper class for converting global sm coord into
 * global mm_region coord + sm coord within the region.
 */
struct reg_coord_pair {
    tripoint reg;
    point sm_loc;

    explicit reg_coord_pair( const tripoint &p ) : sm_loc( p.xy() ) {
        reg = tripoint( sm_to_mmr_remain( sm_loc.x, sm_loc.y ), p.z );
    }
};

mm_submap::mm_submap() = default;
mm_submap::mm_submap( bool make_valid ) : valid( make_valid ) {}

mm_region::mm_region() : submaps {{ nullptr }} {}

bool mm_region::is_empty() const
{
    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        // NOLINTNEXTLINE(modernize-loop-convert)
        for( size_t x  = 0; x < MM_REG_SIZE; x++ ) {
            if( !submaps[x][y]->is_empty() ) {
                return false;
            }
        }
    }
    return true;
}

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
    const mm_submap &sm = get_submap( p.sm );
    return sm.tile( p.loc );
}

void map_memory::memorize_tile( const tripoint &pos, const std::string &ter,
                                const int subtile, const int rotation )
{
    coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    sm.set_tile( p.loc, memorized_terrain_tile{ ter, subtile, rotation } );
}

int map_memory::get_symbol( const tripoint &pos ) const
{
    coord_pair p( pos );
    const mm_submap &sm = get_submap( p.sm );
    return sm.symbol( p.loc );
}

void map_memory::memorize_symbol( const tripoint &pos, const int symbol )
{
    coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    sm.set_symbol( p.loc, symbol );
}

void map_memory::clear_memorized_tile( const tripoint &pos )
{
    coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    sm.set_symbol( p.loc, mm_submap::default_symbol );
    sm.set_tile( p.loc, mm_submap::default_tile );
}

bool map_memory::prepare_region( const tripoint &p1, const tripoint &p2 )
{
    cata_assert( p1.z == p2.z );
    cata_assert( p1.x <= p2.x && p1.y <= p2.y );

    tripoint sm_p1 = coord_pair( p1 ).sm + point_north_west;
    tripoint sm_p2 = coord_pair( p2 ).sm + point_south_east;

    tripoint sm_pos = sm_p1;
    point sm_size = sm_p2.xy() - sm_p1.xy();

    if( sm_pos.z == cache_pos.z ) {
        inclusive_rectangle<point> rect( cache_pos.xy(), cache_pos.xy() + cache_size );
        if( rect.contains( sm_p1.xy() ) && rect.contains( sm_p2.xy() ) ) {
            return false;
        }
    }

    dbg( D_INFO ) << "Preparing memory map for area: pos: " << sm_pos << " size: " << sm_size;

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

shared_ptr_fast<mm_submap> map_memory::fetch_submap( const tripoint &sm_pos )
{
    shared_ptr_fast<mm_submap> sm = find_submap( sm_pos );
    if( sm ) {
        return sm;
    }
    sm = load_submap( sm_pos );
    if( sm ) {
        return sm;
    }
    return allocate_submap( sm_pos );
}

shared_ptr_fast<mm_submap> map_memory::allocate_submap( const tripoint &sm_pos )
{
    // Since all save/load operations are done on regions of submaps,
    // we need to allocate the whole region at once.
    shared_ptr_fast<mm_submap> ret;
    tripoint reg = reg_coord_pair( sm_pos ).reg;

    dbg( D_INFO ) << "Allocated mm_region " << reg << " [" << mmr_to_sm_copy( reg ) << "]";

    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
            tripoint pos = mmr_to_sm_copy( reg ) + tripoint( x, y, 0 );
            shared_ptr_fast<mm_submap> sm = make_shared_fast<mm_submap>();
            if( pos == sm_pos ) {
                ret = sm;
            }
            submaps.insert( std::make_pair( pos, sm ) );
        }
    }

    return ret;
}

shared_ptr_fast<mm_submap> map_memory::find_submap( const tripoint &sm_pos )
{
    auto sm = submaps.find( sm_pos );
    if( sm == submaps.end() ) {
        return nullptr;
    } else {
        return sm->second;
    }
}

shared_ptr_fast<mm_submap> map_memory::load_submap( const tripoint &sm_pos )
{
    if( test_mode ) {
        return nullptr;
    }

    const std::string dirname = find_mm_dir();
    reg_coord_pair p( sm_pos );
    const std::string path = find_region_path( dirname, p.reg );

    if( !dir_exist( dirname ) ) {
        // Old saves don't have [plname].mm1 folder
        return nullptr;
    }

    mm_region mmr;
    const auto loader = [&]( JsonIn & jsin ) {
        mmr.deserialize( jsin );
    };

    try {
        if( !read_from_file_optional_json( path, loader ) ) {
            // Region not found
            return nullptr;
        }
    } catch( const std::exception &err ) {
        debugmsg( "Failed to load memory map region (%d,%d,%d): %s",
                  p.reg.x, p.reg.y, p.reg.z, err.what() );
        return nullptr;
    }

    dbg( D_INFO ) << "Loaded mm_region " << p.reg << " [" << mmr_to_sm_copy( p.reg ) << "]";

    shared_ptr_fast<mm_submap> ret;

    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
            tripoint pos = mmr_to_sm_copy( p.reg ) + tripoint( x, y, 0 );
            shared_ptr_fast<mm_submap> &sm = mmr.submaps[x][y];
            if( pos == sm_pos ) {
                ret = sm;
            }
            submaps.insert( std::make_pair( pos, sm ) );
        }
    }

    return ret;
}

static mm_submap null_mz_submap;
static mm_submap invalid_mz_submap{ false };

const mm_submap &map_memory::get_submap( const tripoint &sm_pos ) const
{
    if( cache_pos == tripoint_min ) {
        debugmsg( "Called map_memory with an " );
        return invalid_mz_submap;
    }
    const point idx = ( sm_pos - cache_pos ).xy();
    if( idx.x > 0 && idx.y > 0 && idx.x < cache_size.x && idx.y < cache_size.y ) {
        return *cached[idx.y * cache_size.x + idx.x];
    } else {
        return null_mz_submap;
    }
}

mm_submap &map_memory::get_submap( const tripoint &sm_pos )
{
    if( cache_pos == tripoint_min ) {
        return invalid_mz_submap;
    }
    const point idx = ( sm_pos - cache_pos ).xy();
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
    dbg( D_INFO ) << "[LOAD] Loading memory map around " << p.sm << ". Loading submaps within " << start
                  << "->" << start + tripoint( MM_SIZE, MM_SIZE, 0 );
    for( int dy = 0; dy < MM_SIZE; dy++ ) {
        for( int dx = 0; dx < MM_SIZE; dx++ ) {
            fetch_submap( start + tripoint( dx, dy, 0 ) );
        }
    }
    dbg( D_INFO ) << "[LOAD] Done.";
}

bool map_memory::save( const tripoint &pos )
{
    tripoint sm_center = coord_pair( pos ).sm;
    const std::string dirname = find_mm_dir();
    assure_dir_exist( dirname );

    clear_cache();

    dbg( D_INFO ) << "N submaps before save: " << submaps.size();

    // Since mm_submaps are always allocated in regions,
    // we are certain that each region will be filled.
    std::map<tripoint, mm_region> regions;
    for( auto &it : submaps ) {
        const reg_coord_pair p( it.first );
        regions[p.reg].submaps[p.sm_loc.x][p.sm_loc.y] = it.second;
    }
    submaps.clear();

    constexpr point MM_HSIZE_P = point( MM_SIZE / 2, MM_SIZE / 2 );
    rectangle<point> rect_keep( sm_center.xy() - MM_HSIZE_P, sm_center.xy() + MM_HSIZE_P );

    dbg( D_INFO ) << "[SAVE] Saving memory map around " << sm_center << ". Keeping submaps within " <<
                  rect_keep.p_min << "->" << rect_keep.p_max;

    bool result = true;

    for( auto &it : regions ) {
        const tripoint &regp = it.first;
        mm_region &reg = it.second;
        if( !reg.is_empty() ) {
            const std::string path = find_region_path( dirname, regp );
            const std::string descr = string_format(
                                          _( "memory map region for (%d,%d,%d)" ),
                                          regp.x, regp.y, regp.z
                                      );

            const auto writer = [&]( std::ostream & fout ) -> void {
                fout << serialize_wrapper( [&]( JsonOut & jsout )
                {
                    reg.serialize( jsout );
                } );
            };

            const bool res = write_to_file( path, writer, descr.c_str() );
            result = result & res;
        }
        tripoint regp_sm = mmr_to_sm_copy( regp );
        half_open_rectangle<point> rect_reg( regp_sm.xy(), regp_sm.xy() + point( MM_REG_SIZE,
                                             MM_REG_SIZE ) );
        if( rect_reg.overlaps( rect_keep ) ) {
            dbg( D_INFO ) << "Keeping mm_region " << regp << " [" << regp_sm << "]";
            // Put submaps back
            for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
                for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
                    tripoint p = regp_sm + tripoint( x, y, 0 );
                    shared_ptr_fast<mm_submap> &sm = reg.submaps[x][y];
                    submaps.insert( std::make_pair( p, sm ) );
                }
            }
        } else {
            dbg( D_INFO ) << "Dropping mm_region " << regp << " [" << regp_sm << "]";
        }
    }

    dbg( D_INFO ) << "[SAVE] Done.";
    dbg( D_INFO ) << "N submaps after save: " << submaps.size();

    return result;
}

void map_memory::clear_cache()
{
    cached.clear();
    cache_pos = tripoint_min;
    cache_size = point_zero;
}
