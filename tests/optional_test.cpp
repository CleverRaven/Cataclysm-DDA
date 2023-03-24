#include <new>
#include <optional>
#include <type_traits>

#include "cata_catch.h"

TEST_CASE( "optional_assignment_works", "[optional]" )
{
    std::optional<int> a( 1 );
    REQUIRE( a );
    CHECK( *a == 1 );

    std::optional<int> b( 2 );
    std::optional<int> unset;
    a = b;
    REQUIRE( a );
    CHECK( *a == 2 );
    a = unset;
    CHECK( !a );
    a = b;
    REQUIRE( a );
    CHECK( *a == 2 );
    a = unset;
    CHECK( !a );

    const std::optional<int> c( 3 );
    a = c;
    REQUIRE( a );
    CHECK( *a == 3 );

    int d = 4;
    a = d;
    REQUIRE( a );
    CHECK( *a == 4 );
    a = d;
    REQUIRE( a );
    CHECK( *a == 4 );

    const int e = 5;
    a = e;
    REQUIRE( a );
    CHECK( *a == 5 );
}

TEST_CASE( "optional_swap", "[optional]" )
{
    std::optional<int> a = GENERATE( std::optional<int>(), std::optional<int>( 1 ) );
    std::optional<int> b = GENERATE( std::optional<int>(), std::optional<int>( 2 ) );
    std::optional<int> a_orig = a;
    std::optional<int> b_orig = b;
    REQUIRE( a == a_orig );
    REQUIRE( b == b_orig );
    std::swap( a, b );
    CHECK( a == b_orig );
    CHECK( b == a_orig );
}
