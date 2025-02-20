#include <bitset>
#include <cstdio>
#include <sstream>
#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "lru_cache.h"
#include "map.h"
#include "map_memory.h"
#include "map_scale_constants.h"
#include "point.h"

static constexpr tripoint_abs_ms p1{ -SEEX - 2, -SEEY - 3, -1 };
static constexpr tripoint_abs_ms p2{ 5, 7, -1 };
static constexpr tripoint_abs_ms p3{ SEEX * 2 + 5, SEEY + 7, -1 };
static constexpr tripoint_abs_ms p4{ SEEX * 3 + 2, SEEY * 7 + 1, -1 };

TEST_CASE( "map_memory_keeps_region", "[map_memory]" )
{
    map_memory memory;
    CHECK( memory.prepare_region( p1, p2 ) );
    CHECK( !memory.prepare_region( p1, p2 ) );
    CHECK( !memory.prepare_region( p1 + tripoint::east, p2 + tripoint::east ) );
    CHECK( memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region( p1, p3 ) );
    CHECK( !memory.prepare_region( p1, p3 ) );
    CHECK( !memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region( p1, p4 ) );
    CHECK( !memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region(
               tripoint_abs_ms( p2.xy(), -p2.z() ),
               tripoint_abs_ms( p3.xy(), -p3.z() )
           ) );
}

TEST_CASE( "map_memory_defaults", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    CHECK( memory.get_tile( p1 ).symbol == 0 );
    memorized_tile default_tile = memory.get_tile( p1 );
    CHECK( default_tile.symbol == 0 );
    CHECK( default_tile.get_ter_id().empty() );
    CHECK( default_tile.get_ter_subtile() == 0 );
    CHECK( default_tile.get_ter_rotation() == 0 );
    CHECK( default_tile.get_dec_id().empty() );
    CHECK( default_tile.get_dec_subtile() == 0 );
    CHECK( default_tile.get_dec_rotation() == 0 );
}

TEST_CASE( "map_memory_remembers", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    memory.set_tile_symbol( p1, 1 );
    memory.set_tile_symbol( p2, 2 );
    CHECK( memory.get_tile( p1 ).symbol == 1 );
    CHECK( memory.get_tile( p2 ).symbol == 2 );

    const memorized_tile &mt = memory.get_tile( p2 );

    memory.set_tile_decoration( p2, "foo", 42, 3 );
    CHECK( mt.get_dec_id() == "foo" );
    CHECK( mt.get_dec_subtile() == 42 );
    CHECK( mt.get_dec_rotation() == 3 );
    CHECK( mt.get_ter_id().empty() );
    CHECK( mt.get_ter_subtile() == 0 );
    CHECK( mt.get_ter_rotation() == 0 );

    memory.set_tile_terrain( p2, "t_foo", 43, 2 );
    CHECK( mt.get_dec_id() == "foo" );
    CHECK( mt.get_dec_subtile() == 42 );
    CHECK( mt.get_dec_rotation() == 3 );
    CHECK( mt.get_ter_id() == "t_foo" );
    CHECK( mt.get_ter_subtile() == 43 );
    CHECK( mt.get_ter_rotation() == 2 );

    memory.set_tile_decoration( p2, "bar", 44, 1 );
    CHECK( mt.get_dec_id() == "bar" );
    CHECK( mt.get_dec_subtile() == 44 );
    CHECK( mt.get_dec_rotation() == 1 );
    CHECK( mt.get_ter_id() == "t_foo" );
    CHECK( mt.get_ter_subtile() == 43 );
    CHECK( mt.get_ter_rotation() == 2 );
}

TEST_CASE( "map_memory_overwrites", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    memory.set_tile_symbol( p1, 1 );
    memory.set_tile_symbol( p2, 2 );
    memory.set_tile_symbol( p2, 3 );
    CHECK( memory.get_tile( p1 ).symbol == 1 );
    CHECK( memory.get_tile( p2 ).symbol == 3 );
}

TEST_CASE( "map_memory_forgets", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    memory.set_tile_decoration( p1, "vp_foo", 42, 3 );
    memory.set_tile_terrain( p1, "t_foo", 43, 2 );
    const memorized_tile &mt = memory.get_tile( p1 );
    CHECK( mt.symbol == 0 );
    CHECK( mt.get_ter_id() == "t_foo" );
    CHECK( mt.get_ter_subtile() == 43 );
    CHECK( mt.get_ter_rotation() == 2 );
    CHECK( mt.get_dec_id() == "vp_foo" );
    CHECK( mt.get_dec_subtile() == 42 );
    CHECK( mt.get_dec_rotation() == 3 );
    memory.set_tile_symbol( p1, 1 );
    CHECK( mt.symbol == 1 );
    memory.clear_tile_decoration( p1, /* prefix = */ "vp_" );
    CHECK( mt.symbol == 0 );
    CHECK( mt.get_ter_id() == "t_foo" );
    CHECK( mt.get_ter_subtile() == 43 );
    CHECK( mt.get_ter_rotation() == 2 );
    CHECK( mt.get_dec_id().empty() );
    CHECK( mt.get_dec_subtile() == 0 );
    CHECK( mt.get_dec_rotation() == 0 );
}

// TODO: map memory save / load

#include <chrono>

TEST_CASE( "lru_cache_perf", "[.]" )
{
    constexpr int max_size = 1000000;
    lru_cache<tripoint, int> symbol_cache;
    const std::chrono::high_resolution_clock::time_point start1 =
        std::chrono::high_resolution_clock::now();
    for( int i = 0; i < 1000000; ++i ) {
        for( int j = -60; j <= 60; ++j ) {
            symbol_cache.insert( max_size, { i, j, 0 }, 1 );
        }
    }
    const std::chrono::high_resolution_clock::time_point end1 =
        std::chrono::high_resolution_clock::now();
    const long long diff1 = std::chrono::duration_cast<std::chrono::microseconds>
                            ( end1 - start1 ).count();
    printf( "completed %d insertions in %lld microseconds.\n", max_size, diff1 );
    /*
     * Original tripoint hash    completed 1000000 insertions in 96136925 microseconds.
     * Table based interleave v1 completed 1000000 insertions in 41435604 microseconds.
     * Table based interleave v2 completed 1000000 insertions in 40856530 microseconds.
     * Jbtw hash                 completed 1000000 insertions in 19049163 microseconds.
     *                                                     rerun 21152804
     * With 1024 batch           completed 1000000 insertions in 39902325 microseconds.
     * backed out batching       completed 1000000 insertions in 20332498 microseconds.
     * rerun                     completed 1000000 insertions in 21659107 microseconds.
     * simple batching, disabled completed 1000000 insertions in 18541486 microseconds.
     * simple batching, 1024     completed 1000000 insertions in 23102395 microseconds.
     * rerun                     completed 1000000 insertions in 31337290 microseconds.
     */
}

// There are 4 quadrants we want to check,
// 1 | 2
// -----
// 3 | 4
// The partitions are defined by partition.x and partition.y
// Each partition has an expected value, and should be homogenous.
static void check_quadrants( std::bitset<MAPSIZE *SEEX *MAPSIZE *SEEY> &test_cache,
                             const point &partition,
                             bool first_val, bool second_val, bool third_val, bool fourth_val )
{
    int y = 0;
    for( ; y < partition.y; ++y ) {
        size_t y_offset = y * SEEX * MAPSIZE;
        int x = 0;
        for( ; x < partition.x; ++x ) {
            INFO( x << " " << y );
            CHECK( first_val == test_cache[ y_offset + x ] );
        }
        for( ; x < SEEX * MAPSIZE; ++x ) {
            INFO( x << " " << y );
            CHECK( second_val == test_cache[ y_offset + x ] );
        }
    }
    for( ; y < SEEY * MAPSIZE; ++y ) {
        size_t y_offset = y * SEEX * MAPSIZE;
        int x = 0;
        for( ; x < partition.x; ++x ) {
            INFO( x << " " << y );
            CHECK( third_val == test_cache[ y_offset + x ] );
        }
        for( ; x < SEEX * MAPSIZE; ++x ) {
            INFO( x << " " << y );
            CHECK( fourth_val == test_cache[ y_offset + x ] );
        }
    }
}

static constexpr size_t first_twelve = SEEX;
static constexpr size_t last_twelve = ( SEEX *MAPSIZE ) - SEEX;

TEST_CASE( "shift_map_memory_bitset_cache" )
{
    std::bitset<MAPSIZE *SEEX *MAPSIZE *SEEY> test_cache;

    GIVEN( "all bits are set" ) {
        test_cache.set();
        WHEN( "positive x shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::east );
            THEN( "last 12 columns are 0, rest are 1" ) {
                check_quadrants( test_cache, point( last_twelve, 0 ),
                                 true, false, true, false );
            }
        }
        WHEN( "negative x shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::west );
            THEN( "first 12 columns are 0, rest are 1" ) {
                check_quadrants( test_cache, point( first_twelve, 0 ),
                                 false, true, false, true );
            }
        }
        WHEN( "positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::south );
            THEN( "last 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( 0, last_twelve ),
                                 true, true, false, false );
            }
        }
        WHEN( "negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::north );
            THEN( "first 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( 0, first_twelve ),
                                 false, false, true, true );
            }
        }
        WHEN( "positive x, positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::south_east );
            THEN( "last 12 columns and rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( last_twelve, last_twelve ),
                                 true, false, false, false );
            }
        }
        WHEN( "positive x, negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::north_east );
            THEN( "last 12 columns and first 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( last_twelve, first_twelve ),
                                 false, false, true, false );
            }
        }
        WHEN( "negative x, positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::south_west );
            THEN( "first 12 columns and last 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( first_twelve, last_twelve ),
                                 false, true, false, false );
            }
        }
        WHEN( "negative x, negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_rel_sm::north_west );
            THEN( "first 12 columns and rows are 0, rest are 1" ) {
                check_quadrants( test_cache, point( first_twelve, first_twelve ),
                                 false, false, false, true );
            }
        }
    }
}
