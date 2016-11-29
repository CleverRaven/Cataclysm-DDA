#include "catch/catch.hpp"

#include "rng.h"

TEST_CASE( "erfinv" ) {
    for( double x = -0.99; x <= 0.99; x += 0.005 ) {
        REQUIRE( std::erf( erfinv( x ) ) == Approx( x ).epsilon( 1e-5 ) );
    }

    for( double z = -5.0; z <= 5.0; z += 0.05 ) {
        REQUIRE( erfinv( std::erf( z ) ) == Approx( z ).epsilon( 1e-5 ) );
    }
}
