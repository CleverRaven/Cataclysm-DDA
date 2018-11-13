#include "catch/catch.hpp"

#include "map_memory.h"
#include "json.h"

#include <sstream>

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
    map_memory memory;
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p2, 2 );
    memory.memorize_symbol( 2, p3, 3 );
    CHECK( memory.get_symbol( p1 ) == 0 );
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
    map_memory memory;
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p2, 2 );
    memory.memorize_symbol( 2, p1, 1 );
    memory.memorize_symbol( 2, p3, 3 );
    CHECK( memory.get_symbol( p1 ) == 1 );
    CHECK( memory.get_symbol( p2 ) == 0 );
    CHECK( memory.get_symbol( p3 ) == 3 );
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
    JsonObject json = jsin.get_object();
    memory2.load( json );

    memory.memorize_symbol( 2, p3, 3 );
    memory2.memorize_symbol( 2, p3, 3 );
    CHECK( memory.get_symbol( p1 ) == memory2.get_symbol( p1 ) );
    CHECK( memory.get_symbol( p2 ) == memory2.get_symbol( p2 ) );
    CHECK( memory.get_symbol( p3 ) == memory2.get_symbol( p3 ) );
}
