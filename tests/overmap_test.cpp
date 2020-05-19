#include <algorithm>
#include <memory>
#include <vector>

#include "calendar.h"
#include "catch/catch.hpp"
#include "common_types.h"
#include "enums.h"
#include "game_constants.h"
#include "omdata.h"
#include "overmap.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "point.h"
#include "type_id.h"

TEST_CASE( "set_and_get_overmap_scents" )
{
    std::unique_ptr<overmap> test_overmap = std::make_unique<overmap>( point_zero );

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

TEST_CASE( "default_overmap_generation_always_succeeds", "[slow]" )
{
    int overmaps_to_construct = 10;
    for( const point &candidate_addr : closest_points_first( point_zero, 10 ) ) {
        // Skip populated overmaps.
        if( overmap_buffer.has( candidate_addr ) ) {
            continue;
        }
        overmap_special_batch test_specials = overmap_specials::get_default_batch( candidate_addr );
        overmap_buffer.create_custom_overmap( candidate_addr, test_specials );
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

TEST_CASE( "default_overmap_generation_has_non_mandatory_specials_at_origin", "[slow]" )
{
    const point origin = point_zero;

    overmap_special mandatory;
    overmap_special optional;

    // Get some specific overmap specials so we can assert their presence later.
    // This should probably be replaced with some custom specials created in
    // memory rather than tying this test to these, but it works for now...
    for( const auto &elem : overmap_specials::get_all() ) {
        if( elem.id == overmap_special_id( "Cabin" ) ) {
            optional = elem;
        } else if( elem.id == overmap_special_id( "Lab" ) ) {
            mandatory = elem;
        }
    }

    // Make this mandatory special impossible to place.
    mandatory.city_size.min = 999;

    // Construct our own overmap_special_batch containing only our single mandatory
    // and single optional special, so we can make some assertions.
    std::vector<const overmap_special *> specials;
    specials.push_back( &mandatory );
    specials.push_back( &optional );
    overmap_special_batch test_specials = overmap_special_batch( origin, specials );

    // Run the overmap creation, which will try to place our specials.
    overmap_buffer.create_custom_overmap( origin, test_specials );

    // Get the origin overmap...
    overmap *test_overmap = overmap_buffer.get_existing( origin );

    // ...and assert that the optional special exists on this map.
    bool found_optional = false;
    for( int x = 0; x < OMAPX; ++x ) {
        for( int y = 0; y < OMAPY; ++y ) {
            const oter_id t = test_overmap->ter( { x, y, 0 } );
            if( t->id == "cabin" ||
                t->id == "cabin_north" || t->id == "cabin_east" ||
                t->id == "cabin_south" || t->id == "cabin_west" ) {
                found_optional = true;
            }
        }
    }

    INFO( "Failed to place optional special on origin " );
    CHECK( found_optional == true );
}

TEST_CASE( "is_ot_match", "[overmap][terrain]" )
{
    SECTION( "exact match" ) {
        // Matches the complete string
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::exact ) );
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::exact ) );

        // Does not exactly match if rotation differs
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sub_station_north" ), ot_match_type::exact ) );
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sub_station_south" ), ot_match_type::exact ) );
    }

    SECTION( "type match" ) {
        // Matches regardless of rotation
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_north" ), ot_match_type::type ) );
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_south" ), ot_match_type::type ) );
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_east" ), ot_match_type::type ) );
        CHECK( is_ot_match( "sub_station", oter_id( "sub_station_west" ), ot_match_type::type ) );

        // Does not match if base type does not match
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab" ), ot_match_type::type ) );
        CHECK_FALSE( is_ot_match( "sub_station", oter_id( "sewer_sub_station" ), ot_match_type::type ) );
    }

    SECTION( "prefix match" ) {
        // Matches the complete string
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::prefix ) );

        // Prefix matches when an underscore separator exists
        CHECK( is_ot_match( "central", oter_id( "central_lab" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );

        // Prefix itself may contain underscores
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );
        CHECK( is_ot_match( "central_lab_train", oter_id( "central_lab_train_depot" ),
                            ot_match_type::prefix ) );

        // Prefix does not match without an underscore separator
        CHECK_FALSE( is_ot_match( "fore", oter_id( "forest" ), ot_match_type::prefix ) );
        CHECK_FALSE( is_ot_match( "fore", oter_id( "forest_thick" ), ot_match_type::prefix ) );

        // Prefix does not match the middle or end
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab" ), ot_match_type::prefix ) );
        CHECK_FALSE( is_ot_match( "lab", oter_id( "central_lab_stairs" ), ot_match_type::prefix ) );
    }

    SECTION( "contains match" ) {
        // Matches the complete string
        CHECK( is_ot_match( "forest", oter_id( "forest" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "central_lab", oter_id( "central_lab" ), ot_match_type::contains ) );

        // Matches the beginning/middle/end of an underscore-delimited id
        CHECK( is_ot_match( "central", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "lab", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "stairs", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );

        // Matches the beginning/middle/end without undercores as well
        CHECK( is_ot_match( "cent", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "ral_lab", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "_lab_", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );
        CHECK( is_ot_match( "airs", oter_id( "central_lab_stairs" ), ot_match_type::contains ) );

        // Does not match if substring is not contained
        CHECK_FALSE( is_ot_match( "forest", oter_id( "central_lab" ), ot_match_type::contains ) );
        CHECK_FALSE( is_ot_match( "forestry", oter_id( "forest" ), ot_match_type::contains ) );
    }
}

