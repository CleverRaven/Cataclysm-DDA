#include <sstream>

#include "catch/catch.hpp"
#include "map_memory.h"
#include "json.h"

static constexpr tripoint p1{ 0, 0, 1 };
static constexpr tripoint p2{ 0, 0, 2 };
static constexpr tripoint p3{ 0, 0, 3 };

TEST_CASE( "map_memory_defaults", "[map_memory]" )
{
    map_memory memory;
    CHECK( memory.get_symbol( p1 ) == 0 );
    memorized_terrain_tile default_tile = memory.get_tile( p1 );
    CHECK( default_tile.tile.empty() );
    CHECK( default_tile.subtile == 0 );
    CHECK( default_tile.rotation == 0 );
}

TEST_CASE( "map_memory_remembers", "[map_memory]" )
{
    map_memory memory;
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p2, 2 );
    CHECK( memory.get_symbol( p1 ) == 1 );
    CHECK( memory.get_symbol( p2 ) == 2 );
}

TEST_CASE( "map_memory_limited", "[map_memory]" )
{
    lru_cache<long> symbol_cache;
    symbol_cache.insert( 2, p1, 1 );
    symbol_cache.insert( 2, p2, 1 );
    symbol_cache.insert( 2, p3, 1 );
    CHECK( symbol_cache.get( p1, 0 ) == 0 );
}

TEST_CASE( "map_memory_overwrites", "[map_memory]" )
{
    map_memory memory;
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p2, 2 );
    memory.memorize_symbol( 2, p2, 3 );
    CHECK( memory.get_symbol( p1 ) == 1 );
    CHECK( memory.get_symbol( p2 ) == 3 );
}

TEST_CASE( "map_memory_erases_lru", "[map_memory]" )
{
    lru_cache<long> symbol_cache;
    symbol_cache.insert( 2, p1, 1 );
    symbol_cache.insert( 2, p2, 2 );
    symbol_cache.insert( 2, p1, 1 );
    symbol_cache.insert( 2, p3, 3 );
    CHECK( symbol_cache.get( p1, 0 ) == 1 );
    CHECK( symbol_cache.get( p2, 0 ) == 0 );
    CHECK( symbol_cache.get( p3, 0 ) == 3 );
}

TEST_CASE( "map_memory_survives_save_lod", "[map_memory]" )
{
    map_memory memory;
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p2, 2 );

    // Save and reload
    std::ostringstream jsout_s;
    JsonOut jsout( jsout_s );
    memory.store( jsout );

    INFO( "Json was: " << jsout_s.str() );
    std::istringstream jsin_s( jsout_s.str() );
    JsonIn jsin( jsin_s );
    map_memory memory2;
    memory2.load( jsin );

    memory.memorize_symbol( 2, p3, 3 );
    memory2.memorize_symbol( 2, p3, 3 );
    CHECK( memory.get_symbol( p1 ) == memory2.get_symbol( p1 ) );
    CHECK( memory.get_symbol( p2 ) == memory2.get_symbol( p2 ) );
    CHECK( memory.get_symbol( p3 ) == memory2.get_symbol( p3 ) );
}

#include <chrono>

TEST_CASE( "lru_cache_perf", "[.]" )
{
    constexpr int max_size = 1000000;
    lru_cache<long> symbol_cache;
    const auto start1 = std::chrono::high_resolution_clock::now();
    for( int i = 0; i < 1000000; ++i ) {
        for( int j = -60; j <= 60; ++j ) {
            symbol_cache.insert( max_size, { i, j, 0 }, 1 );
        }
    }
    const auto end1 = std::chrono::high_resolution_clock::now();
    const long diff1 = std::chrono::duration_cast<std::chrono::microseconds>( end1 - start1 ).count();
    printf( "completed %d insertions in %ld microseconds.\n", max_size, diff1 );
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
