#include <cstddef>
#include <cmath>
#include <random>

#include "catch/catch.hpp"
#include "cata_utility.h"
#include "rng.h"

TEST_CASE( "fast_floor", "[math]" )
{
    REQUIRE( fast_floor( -2.0 ) == -2 );
    REQUIRE( fast_floor( -1.9 ) == -2 );
    REQUIRE( fast_floor( -1.1 ) == -2 );
    REQUIRE( fast_floor( -1.0 ) == -1 );
    REQUIRE( fast_floor( -0.9 ) == -1 );
    REQUIRE( fast_floor( -0.1 ) == -1 );
    REQUIRE( fast_floor( 0.0 ) ==  0 );
    REQUIRE( fast_floor( 0.1 ) ==  0 );
    REQUIRE( fast_floor( 0.9 ) ==  0 );
    REQUIRE( fast_floor( 1.0 ) ==  1 );
    REQUIRE( fast_floor( 1.1 ) ==  1 );
    REQUIRE( fast_floor( 1.9 ) ==  1 );
    REQUIRE( fast_floor( 2.0 ) ==  2 );
    REQUIRE( fast_floor( 2.1 ) ==  2 );
    REQUIRE( fast_floor( 2.9 ) ==  2 );

    for( size_t i = 0; i != 1000; ++i ) {
        double val = rng_float( -10, 10 );
        REQUIRE( fast_floor( val ) == std::floor( val ) );
    }
}
