#include "catch/catch.hpp"

#include "overmap.h"

TEST_CASE( "set_and_get_overmap_scents" ) {
    overmap test_overmap;

    // By default there are no scents set.
    for( int x = 0; x < 180; ++x ) {
        for( int y = 0; y < 180; ++y ) {
            for( int z = -10; z < 10; ++z ) {
                REQUIRE( test_overmap.scent_at( { x, y, z } ).creation_turn == -1 );
            }
        }
    }

    scent_trace test_scent( 50, 90 );
    test_overmap.set_scent( { 75, 85, 0 }, test_scent );
    REQUIRE( test_overmap.scent_at( { 75, 85, 0} ).creation_turn == 50 );
    REQUIRE( test_overmap.scent_at( { 75, 85, 0} ).initial_strength == 90 );
}
