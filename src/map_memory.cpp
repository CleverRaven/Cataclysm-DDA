#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <ostream>
#include <tuple>
#include <utility>

#include "cached_options.h"
#include "cata_assert.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "filesystem.h"
#include "game_constants.h"
#include "map_memory.h"
#include "path_info.h"
#include "string_formatter.h"
#include "translations.h"

const memorized_tile mm_submap::default_tile = {};

static constexpr int MM_SIZE = MAPSIZE * 2;

#define dbg(x) DebugLog((x),D_MMAP) << __FILE__ << ":" << __LINE__ << ": "

// Moved from coordinate_conversions.h to the only file using it.
static tripoint_abs_sm mmr_to_sm_copy( const tripoint &p )
{
    return tripoint_abs_sm( p.x * MM_REG_SIZE, p.y * MM_REG_SIZE, p.z );
}

static cata_path find_mm_dir()
{
    return PATH_INFO::player_base_save_path() + ".mm1";
}

static cata_path find_region_path( const cata_path &dirname, const tripoint &p )
{
    return dirname / string_format( "%d.%d.%d.mmr", p.x, p.y, p.z );
}

/**
 * Helper class for converting global sm coord into
 * global mm_region coord + sm coord within the region.
 */
struct reg_coord_pair {
    tripoint reg;
    point_abs_sm sm_loc;

    explicit reg_coord_pair( const tripoint_abs_sm &p ) : sm_loc( p.xy() ) {
        reg = tripoint( sm_to_mmr_remain( sm_loc.x(), sm_loc.y() ), p.z() );
    }
};

mm_submap::mm_submap( bool make_valid ) : valid( make_valid ) {}

bool mm_submap::is_empty() const
{
    return tiles.empty();
}

bool mm_submap::is_valid() const
{
    return valid;
}

const memorized_tile &mm_submap::get_tile( const point_sm_ms &p ) const
{
    if( tiles.empty() ) {
        return default_tile;
    }
    return tiles[p.y() * SEEX + p.x()];
}

void mm_submap::set_tile( const point_sm_ms &p, const memorized_tile &value )
{
    if( tiles.empty() ) {
        // call 'reserve' first to force allocation of exact size
        tiles.reserve( SEEX * SEEY );
        tiles.resize( SEEX * SEEY, default_tile );
    }
    tiles[p.y() * SEEX + p.x()] = value;
}

mm_region::mm_region() : submaps( nullptr ) {}

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

const std::string &memorized_tile::get_ter_id() const
{
    return ter_id.str();
}

const std::string &memorized_tile::get_dec_id() const
{
    return dec_id;
}

void memorized_tile::set_ter_id( std::string_view id )
{
    ter_id = ter_str_id( id );
}

void memorized_tile::set_dec_id( std::string_view id )
{
    dec_id = id;
}

int memorized_tile::get_ter_rotation() const
{
    return ter_rotation;
}

void memorized_tile::set_ter_rotation( int rotation )
{
    if( rotation < std::numeric_limits<decltype( ter_rotation )>::min() ||
        rotation > std::numeric_limits<decltype( ter_rotation )>::max() ) {
        debugmsg( "map memory can't store rotation value %d", rotation );
        rotation = 0;
    }
    ter_rotation = rotation;
}

int memorized_tile::get_dec_rotation() const
{
    return dec_rotation;
}

void memorized_tile::set_dec_rotation( int rotation )
{
    if( rotation < std::numeric_limits<decltype( dec_rotation )>::min() ||
        rotation > std::numeric_limits<decltype( dec_rotation )>::max() ) {
        debugmsg( "map memory can't store rotation value %d", rotation );
        rotation = 0;
    }
    dec_rotation = rotation;
}

int memorized_tile::get_ter_subtile() const
{
    return ter_subtile;
}

void memorized_tile::set_ter_subtile( int subtile )
{
    if( subtile < std::numeric_limits<decltype( ter_subtile )>::min() ||
        subtile > std::numeric_limits<decltype( ter_subtile )>::max() ) {
        debugmsg( "map memory can't store terrain subtile value %d", subtile );
        subtile = 0;
    }
    ter_subtile = subtile;
}

int memorized_tile::get_dec_subtile() const
{
    return dec_subtile;
}

void memorized_tile::set_dec_subtile( int subtile )
{
    if( subtile < std::numeric_limits<decltype( dec_subtile )>::min() ||
        subtile > std::numeric_limits<decltype( dec_subtile )>::max() ) {
        debugmsg( "map memory can't store decoration subtile value %d", subtile );
        subtile = 0;
    }
    dec_subtile = subtile;
}

bool memorized_tile::operator==( const memorized_tile &rhs ) const
{
    return symbol == rhs.symbol &&
           ter_rotation == rhs.ter_rotation &&
           dec_rotation == rhs.dec_rotation &&
           ter_subtile == rhs.ter_subtile &&
           dec_subtile == rhs.dec_subtile &&
           ter_id == rhs.ter_id &&
           dec_id == rhs.dec_id;
}

map_memory::coord_pair::coord_pair( const tripoint_abs_ms &p )
{
    std::tie( sm, loc ) = coords::project_remain<coords::sm>( p );
}

map_memory::map_memory()
{
    clear_cache();
}

const memorized_tile &map_memory::get_tile( const tripoint_abs_ms &pos ) const
{
    const coord_pair p( pos );
    const mm_submap &sm = get_submap( p.sm );
    return sm.get_tile( p.loc );
}

void map_memory::set_tile_terrain( const tripoint_abs_ms &pos, std::string_view id,
                                   int subtile, int rotation )
{
    const coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    memorized_tile mt = sm.get_tile( p.loc );
    mt.set_ter_id( id );
    mt.set_ter_subtile( subtile );
    mt.set_ter_rotation( rotation );
    sm.set_tile( p.loc, mt );
}

void map_memory::set_tile_decoration( const tripoint_abs_ms &pos, std::string_view id,
                                      int subtile, int rotation )
{
    const coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    memorized_tile mt = sm.get_tile( p.loc );
    mt.set_dec_id( id );
    mt.set_dec_subtile( subtile );
    mt.set_dec_rotation( rotation );
    sm.set_tile( p.loc, mt );
}

void map_memory::set_tile_symbol( const tripoint_abs_ms &pos, char32_t symbol )
{
    const coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    memorized_tile mt = sm.get_tile( p.loc );
    mt.symbol = symbol;
    sm.set_tile( p.loc, mt );
}

void map_memory::clear_tile_decoration( const tripoint_abs_ms &pos, std::string_view prefix )
{
    const coord_pair p( pos );
    mm_submap &sm = get_submap( p.sm );
    if( !sm.is_valid() ) {
        return;
    }
    memorized_tile mt = sm.get_tile( p.loc );
    if( string_starts_with( mt.get_dec_id(), prefix ) ) {
        mt.set_dec_id( "" );
        mt.set_dec_rotation( 0 );
        mt.set_dec_subtile( 0 );
        mt.symbol = 0;
    }
    sm.set_tile( p.loc, mt );
}

bool map_memory::prepare_region( const tripoint_abs_ms &p1, const tripoint_abs_ms &p2 )
{
    cata_assert( p1.z() == p2.z() );
    cata_assert( p1.x() <= p2.x() && p1.y() <= p2.y() );

    tripoint_abs_sm sm_p1 = coord_pair( p1 ).sm + point::north_west;
    tripoint_abs_sm sm_p2 = coord_pair( p2 ).sm + point::south_east;

    tripoint_abs_sm sm_pos = sm_p1;
    point_rel_sm sm_size = sm_p2.xy() - sm_p1.xy();

    if( sm_pos.z() == cache_pos.z() ) {
        inclusive_rectangle<point_abs_sm> rect(
            point_abs_sm( cache_pos.xy() ),
            point_abs_sm( cache_pos.xy() + cache_size ) );
        if( rect.contains( sm_p1.xy() ) && rect.contains( sm_p2.xy() ) ) {
            return false;
        }
    }

    dbg( D_INFO ) << "Preparing memory map for area: pos: " << sm_pos << " size: " << sm_size;

    cache_pos = sm_pos;
    cache_size = sm_size.raw();
    cached.clear();
    // Loop through each z-level in vision range
    for( int z = std::max( sm_pos.z() - fov_3d_z_range, -OVERMAP_DEPTH );
         z <= std::min( sm_pos.z() + fov_3d_z_range, OVERMAP_HEIGHT ); z++ ) {
        cached[z].reserve( static_cast<std::size_t>( cache_size.x ) * cache_size.y );
        for( int dy = 0; dy < cache_size.y; dy++ ) {
            for( int dx = 0; dx < cache_size.x; dx++ ) {
                // Store submap pointer in cache, categorized by z-level
                const tripoint_abs_sm smpos( cache_pos.x() + dx, cache_pos.y() + dy, z );
                cached[z].push_back( fetch_submap( smpos ) );
            }
        }
    }
    return true;
}

shared_ptr_fast<mm_submap> map_memory::fetch_submap( const tripoint_abs_sm &sm_pos )
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

shared_ptr_fast<mm_submap> map_memory::allocate_submap( const tripoint_abs_sm &sm_pos )
{
    // Since all save/load operations are done on regions of submaps,
    // we need to allocate the whole region at once.
    shared_ptr_fast<mm_submap> ret;
    tripoint reg = reg_coord_pair( sm_pos ).reg;

    dbg( D_INFO ) << "Allocated mm_region " << reg << " [" << mmr_to_sm_copy( reg ) << "]";

    for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
        for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
            const tripoint_abs_sm pos( mmr_to_sm_copy( reg ) + tripoint( x, y, 0 ) );
            shared_ptr_fast<mm_submap> sm = make_shared_fast<mm_submap>();
            if( pos == sm_pos ) {
                ret = sm;
            }
            submaps.insert( std::make_pair( pos, sm ) );
        }
    }

    return ret;
}

shared_ptr_fast<mm_submap> map_memory::find_submap( const tripoint_abs_sm &sm_pos )
{
    auto sm = submaps.find( sm_pos );
    if( sm == submaps.end() ) {
        return nullptr;
    } else {
        return sm->second;
    }
}

shared_ptr_fast<mm_submap> map_memory::load_submap( const tripoint_abs_sm &sm_pos )
{
    if( test_mode ) {
        return nullptr;
    }

    const reg_coord_pair p( sm_pos );
    const cata_path path = find_region_path( find_mm_dir(), p.reg );

    mm_region mmr;
    const auto loader = [&mmr]( const JsonValue & jsin ) {
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
            const tripoint_abs_sm pos( mmr_to_sm_copy( p.reg ) + tripoint( x, y, 0 ) );
            shared_ptr_fast<mm_submap> &sm = mmr.submaps[x][y];
            if( pos == sm_pos ) {
                ret = sm;
            }
            submaps.emplace( pos, sm );
        }
    }

    return ret;
}

static mm_submap null_mz_submap;
static mm_submap invalid_mz_submap{ false };
static const tripoint_abs_sm invalid_cache_pos = tripoint_abs_sm::invalid;

const mm_submap &map_memory::get_submap( const tripoint_abs_sm &sm_pos ) const
{
    return const_cast<map_memory *>( this )->get_submap( sm_pos );
}

mm_submap &map_memory::get_submap( const tripoint_abs_sm &sm_pos )
{
    if( !is_valid() ) {
        return invalid_mz_submap;
    }
    const point_rel_sm idx = ( sm_pos - cache_pos ).xy();
    if( idx.x() > 0 && idx.y() > 0 && idx.x() < cache_size.x && idx.y() < cache_size.y &&
        !cached[sm_pos.z()].empty() ) {
        return *cached[sm_pos.z()][idx.y() * cache_size.x + idx.x()];
    } else {
        return null_mz_submap;
    }
}

bool map_memory::is_valid() const
{
    return cache_pos != invalid_cache_pos;
}

void map_memory::load( const tripoint_abs_ms &pos )
{
    const coord_pair p( pos );
    const tripoint_abs_sm start = p.sm - tripoint_rel_sm( MM_SIZE / 2, MM_SIZE / 2, 0 );
    dbg( D_INFO ) << "[LOAD] Loading memory map around " << p.sm << ". Loading submaps within " << start
                  << "->" << start + tripoint( MM_SIZE, MM_SIZE, 0 );
    clear_cache();
    for( int dy = 0; dy < MM_SIZE; dy++ ) {
        for( int dx = 0; dx < MM_SIZE; dx++ ) {
            fetch_submap( start + tripoint_rel_sm( dx, dy, 0 ) );
        }
    }
    dbg( D_INFO ) << "[LOAD] Done.";
}

bool map_memory::save( const tripoint_abs_ms &pos )
{
    const tripoint_abs_sm sm_center = coord_pair( pos ).sm;
    const cata_path dirname = find_mm_dir();
    assure_dir_exist( dirname );

    clear_cache();

    dbg( D_INFO ) << "N submaps before save: " << submaps.size();

    // Since mm_submaps are always allocated in regions,
    // we are certain that each region will be filled.
    std::map<tripoint, mm_region> regions;
    for( auto &it : submaps ) {
        const reg_coord_pair p( it.first );
        regions[p.reg].submaps[p.sm_loc.x()][p.sm_loc.y()] = it.second;
    }
    submaps.clear();

    constexpr point MM_HSIZE_P = point( MM_SIZE / 2, MM_SIZE / 2 );
    rectangle<point_abs_sm> rect_keep( sm_center.xy() - MM_HSIZE_P, sm_center.xy() + MM_HSIZE_P );

    dbg( D_INFO ) << "[SAVE] Saving memory map around " << sm_center << ". Keeping submaps within " <<
                  rect_keep.p_min << "->" << rect_keep.p_max;

    bool result = true;

    for( auto &it : regions ) {
        const tripoint &regp = it.first;
        mm_region &reg = it.second;
        if( !reg.is_empty() ) {
            const cata_path path = find_region_path( dirname, regp );
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
        const tripoint_abs_sm regp_sm( mmr_to_sm_copy( regp ) );
        const half_open_rectangle<point_abs_sm> rect_reg(
            regp_sm.xy(),
            regp_sm.xy() + point( MM_REG_SIZE, MM_REG_SIZE ) );

        if( rect_reg.overlaps( rect_keep ) ) {
            dbg( D_INFO ) << "Keeping mm_region " << regp << " [" << regp_sm << "]";
            // Put submaps back
            for( size_t y = 0; y < MM_REG_SIZE; y++ ) {
                for( size_t x = 0; x < MM_REG_SIZE; x++ ) {
                    const tripoint_abs_sm p = regp_sm + tripoint( x, y, 0 );
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
    cache_pos = invalid_cache_pos;
    cache_size = point::zero;
}
