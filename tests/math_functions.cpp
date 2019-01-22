#include <cmath>
#include <random>

#include "catch/catch.hpp"
#include "cata_utility.h"

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

    std::random_device rd;
    std::mt19937 mt( rd() );
    std::uniform_real_distribution<double> dist( -10.0, 10.0 );

    for( size_t i = 0; i != 1000; ++i ) {
        double val = dist( mt );
        REQUIRE( fast_floor( val ) == std::floor( val ) );
    }
}
