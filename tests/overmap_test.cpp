#include "catch/catch.hpp"

#include "map.h"
#include "overmap.h"
#include "overmapbuffer.h"

TEST_CASE( "set_and_get_overmap_scents" )
{
    std::unique_ptr<overmap> test_overmap = std::unique_ptr<overmap>( new overmap( 0, 0 ) );

    // By default there are no scents set.
    for( int x = 0; x < 180; ++x ) {
        for( int y = 0; y < 180; ++y ) {
            for( int z = -10; z < 10; ++z ) {
                REQUIRE( test_overmap->scent_at( { x, y, z } ).creation_time == -1 );
            }
        }
    }

    scent_trace test_scent( 50, 90 );
    test_overmap->set_scent( { 75, 85, 0 }, test_scent );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).creation_time == 50 );
    REQUIRE( test_overmap->scent_at( { 75, 85, 0} ).initial_strength == 90 );
}

TEST_CASE( "default_overmap_generation_always_succeeds" )
{
    int overmaps_to_construct = 10;
    for( point candidate_addr : closest_points_first( 10, { 0, 0 } ) ) {
        // Skip populated overmaps.
        if( overmap_buffer.has( candidate_addr.x, candidate_addr.y ) ) {
            continue;
        }
        overmap_special_batch test_specials = overmap_specials::get_default_batch( candidate_addr );
        overmap_buffer.create_custom_overmap( candidate_addr.x, candidate_addr.y, test_specials );
        std::stringstream remaining_specials;
        for( const auto &special_placement : test_specials ) {
            auto special = special_placement.special_details;
            INFO( "In attempt #" << overmaps_to_construct
                  << " failed to place " << special->id.str() );
            CHECK( special->occurrences.min <= special_placement.instances_placed );
        }
        if( --overmaps_to_construct <= 0 ) {
            break;
        }
    }
}
