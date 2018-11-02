#include "catch/catch.hpp"

#include "map_memory.h"

static const tripoint p1{ 0, 0, 1 };
static const tripoint p2{ 0, 0, 2 };
static const tripoint p3{ 0, 0, 3 };

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
