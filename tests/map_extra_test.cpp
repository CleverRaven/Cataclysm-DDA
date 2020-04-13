#include <algorithm>

#include "coordinate_conversions.h"
#include "catch/catch.hpp"
#include "overmapbuffer.h"
#include "map_extras.h"

// This test case is disabled because this map extra effectively doesn't spawn
// with the current real settings. In order for it to spawn, you have to:
//
// 1) roll of a map extra on a road
// 2) select the mx_minefield extra out of the pool for road map extras
// 3) have your current location be a road that is adjacent to a bridge
//
// and that particular combo is so rare that it might as well never happen.
TEST_CASE( "mx_minefield real spawn", "[map_extra][overmap][!mayfail]" )
{
    // Pick a point in the middle of the overmap so we don't generate quite so
    // many overmaps when searching.
    const tripoint origin = tripoint( 90, 90, 0 );;

    // Find all of the roads within a 180 OMT radius of this location.
    omt_find_params find_params;
    find_params.types = {{"road", ot_match_type::type}};
    find_params.search_range = 180;
    const std::vector<tripoint> roads = overmap_buffer.find_all( origin, find_params );

    // The rest of this check is pointless if there are no roads.
    REQUIRE( roads.size() > 0 );

    // For every single road we found, run mapgen (which will select and apply a map extra).
    for( const tripoint &p : roads ) {
        tinymap tm;
        tm.load( omt_to_sm_copy( p ), false );
    }

    // Get all of the map extras that have been generated.
    const std::vector<std::pair<point, string_id<map_extra>>> extras = overmap_buffer.get_all_extras(
                origin.z );

    // Count the number of mx_minefield map extras that have been generated.
    const string_id<map_extra> mx_minefield( "mx_minefield" );
    int successes = std::count_if( extras.begin(),
    extras.end(), [&mx_minefield]( std::pair<point, string_id<map_extra>> e ) {
        return e.second == mx_minefield;
    } );

    // If at least one was generated, that's good enough.
    CHECK( successes > 0 );
}

TEST_CASE( "mx_minefield theorertical spawn", "[map_extra][overmap]" )
{
    overmap &om = overmap_buffer.get( point_zero );

    const oter_id road( "road_ns" );
    const oter_id bridge( "bridge_north" );

    // The mx_minefield map extra expects to have a particular configuration with
    // three OMTs--a road, then not a bridge (aka a road is fine), then a bridge.
    // It does this for four rotations, with the bridge on the north, south, east,
    // and west of the target point.
    const auto setup_terrain_and_generate = [&]( const tripoint & center,
    om_direction::type bridge_direction ) {
        om.ter_set( center, road );
        om.ter_set( center + om_direction::displace( bridge_direction, 1 ), bridge );
        om.ter_set( center + om_direction::displace( om_direction::opposite( bridge_direction ), 1 ),
                    road );

        tinymap tm;
        tm.load( omt_to_sm_copy( center ), false );

        const string_id<map_extra> mx_minefield( "mx_minefield" );
        const map_extra_pointer mx_func = MapExtras::get_function( mx_minefield.str() );

        return mx_func( tm, tm.get_abs_sub() );
    };

    // Pick a point in the middle of the overmap so we don't go out of bounds when setting up
    // our terrains.
    const tripoint target( 90, 90, 0 );

    // Check that for each direction (north, south, east, west) the map extra generates successfully.
    for( om_direction::type dir : om_direction::all ) {
        CHECK( setup_terrain_and_generate( target, dir ) );
    }
}
