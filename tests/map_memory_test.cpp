#include <bitset>
#include <cstdio>
#include <sstream>
#include <string>

#include "catch/catch.hpp"
#include "game_constants.h"
#include "json.h"
#include "lru_cache.h"
#include "map.h"
#include "map_memory.h"
#include "point.h"
#include "string_formatter.h"

static constexpr tripoint p1{ -SEEX - 2, -SEEY - 3, -1 };
static constexpr tripoint p2{ 5, 7, -1 };
static constexpr tripoint p3{ SEEX * 2 + 5, SEEY + 7, -1 };
static constexpr tripoint p4{ SEEX * 3 + 2, SEEY * 7 + 1, -1 };

TEST_CASE( "map_memory_keeps_region", "[map_memory]" )
{
    map_memory memory;
    CHECK( memory.prepare_region( p1, p2 ) );
    CHECK( !memory.prepare_region( p1, p2 ) );
    CHECK( !memory.prepare_region( p1 + tripoint_east, p2 + tripoint_east ) );
    CHECK( memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region( p1, p3 ) );
    CHECK( !memory.prepare_region( p1, p3 ) );
    CHECK( !memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region( p1, p4 ) );
    CHECK( !memory.prepare_region( p2, p3 ) );
    CHECK( memory.prepare_region(
               tripoint( p2.x, p2.y, -p2.z ),
               tripoint( p3.x, p3.y, -p3.z )
           ) );
}

TEST_CASE( "map_memory_defaults", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    CHECK( memory.get_symbol( p1 ) == 0 );
    memorized_terrain_tile default_tile = memory.get_tile( p1 );
    CHECK( default_tile.tile.empty() );
    CHECK( default_tile.subtile == 0 );
    CHECK( default_tile.rotation == 0 );
}

TEST_CASE( "map_memory_remembers", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    memory.memorize_symbol( p1, 1 );
    memory.memorize_symbol( p2, 2 );
    CHECK( memory.get_symbol( p1 ) == 1 );
    CHECK( memory.get_symbol( p2 ) == 2 );
}

TEST_CASE( "map_memory_overwrites", "[map_memory]" )
{
    map_memory memory;
    memory.prepare_region( p1, p2 );
    memory.memorize_symbol( p1, 1 );
    memory.memorize_symbol( p2, 2 );
    memory.memorize_symbol( p2, 3 );
    CHECK( memory.get_symbol( p1 ) == 1 );
    CHECK( memory.get_symbol( p2 ) == 3 );
}

// TODO: map memory save / load

#include <chrono>

TEST_CASE( "lru_cache_perf", "[.]" )
{
    constexpr int max_size = 1000000;
    lru_cache<tripoint, int> symbol_cache;
    const auto start1 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < 1000000; ++i ) {
        for( int j = -60; j <= 60; ++j ) {
            symbol_cache.insert( max_size, { i, j, 0 }, 1 );
        }
    }
    const auto end1 = std::chrono::high_resolution_clock::now();
    const long long diff1 = std::chrono::duration_cast<std::chrono::microseconds>
                            ( end1 - start1 ).count();
    cata_printf( "completed %d insertions in %lld microseconds.\n", max_size, diff1 );
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
// The partitions are defined by x_partition and y_partition
// Each partition has an expected value, and should be homogenous.
static void check_quadrants( std::bitset<MAPSIZE *SEEX *MAPSIZE *SEEY> &test_cache,
                             size_t x_partition, size_t y_partition,
                             bool first_val, bool second_val, bool third_val, bool fourth_val )
{
    size_t y = 0;
    for( ; y < y_partition; ++y ) {
        size_t y_offset = y * SEEX * MAPSIZE;
        size_t x = 0;
        for( ; x < x_partition; ++x ) {
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
        size_t x = 0;
        for( ; x < x_partition; ++x ) {
            INFO( x << " " << y );
            CHECK( third_val == test_cache[ y_offset + x ] );
        }
        for( ; x < SEEX * MAPSIZE; ++x ) {
            INFO( x << " " << y );
            CHECK( fourth_val == test_cache[ y_offset + x ] );
        }
    }
}

constexpr size_t first_twelve = SEEX;
constexpr size_t last_twelve = ( SEEX *MAPSIZE ) - SEEX;

TEST_CASE( "shift_map_memory_seen_cache" )
{
    std::bitset<MAPSIZE *SEEX *MAPSIZE *SEEY> test_cache;

    GIVEN( "all bits are set" ) {
        test_cache.set();
        WHEN( "positive x shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_east );
            THEN( "last 12 columns are 0, rest are 1" ) {
                check_quadrants( test_cache, last_twelve, 0,
                                 true, false, true, false );
            }
        }
        WHEN( "negative x shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_west );
            THEN( "first 12 columns are 0, rest are 1" ) {
                check_quadrants( test_cache, first_twelve, 0,
                                 false, true, false, true );
            }
        }
        WHEN( "positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_south );
            THEN( "last 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, 0, last_twelve,
                                 true, true, false, false );
            }
        }
        WHEN( "negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_north );
            THEN( "first 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, 0, first_twelve,
                                 false, false, true, true );
            }
        }
        WHEN( "positive x, positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_south_east );
            THEN( "last 12 columns and rows are 0, rest are 1" ) {
                check_quadrants( test_cache, last_twelve, last_twelve,
                                 true, false, false, false );
            }
        }
        WHEN( "positive x, negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_north_east );
            THEN( "last 12 columns and first 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, last_twelve, first_twelve,
                                 false, false, true, false );
            }
        }
        WHEN( "negative x, positive y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_south_west );
            THEN( "first 12 columns and last 12 rows are 0, rest are 1" ) {
                check_quadrants( test_cache, first_twelve, last_twelve,
                                 false, true, false, false );
            }
        }
        WHEN( "negative x, negative y shift" ) {
            shift_bitset_cache<MAPSIZE_X, SEEX>( test_cache, point_north_west );
            THEN( "first 12 columns and rows are 0, rest are 1" ) {
                check_quadrants( test_cache, first_twelve, first_twelve,
                                 false, false, false, true );
            }
        }
    }
}
