﻿#include <algorithm>

#include "coordinate_conversions.h"
#include "catch/catch.hpp"
#include "coordinates.h"
#include "overmapbuffer.h"
#include "map_extras.h"

TEST_CASE( "mx_minefield real spawn", "[map_extra][overmap]" )
{
    // Pick a point in the middle of the overmap so we don't generate quite so
    // many overmaps when searching.
    const tripoint_abs_omt origin( 90, 90, 0 );

    // Find all of the bridgeheads within a 180 OMT radius of this location.
    omt_find_params find_params;
    find_params.types = {{"bridgehead_ground", ot_match_type::type}};
    find_params.search_range = 180;
    const std::vector<tripoint_abs_omt> bridges = overmap_buffer.find_all( origin, find_params );

    // The rest of this check is pointless if there are no bridges.
    REQUIRE( !bridges.empty() );

    // For every single bridge we found, run mapgen (which will select and apply a map extra).
    for( const tripoint_abs_omt &p : bridges ) {
        tinymap tm;
        tm.load( project_to<coords::sm>( p ), false );
    }

    // Get all of the map extras that have been generated.
    const std::vector<std::pair<point_abs_omt, string_id<map_extra>>> extras =
        overmap_buffer.get_all_extras( origin.z() );

    // Count the number of mx_minefield map extras that have been generated.
    const string_id<map_extra> mx_minefield( "mx_minefield" );
    int successes = std::count_if( extras.begin(),
    extras.end(), [&mx_minefield]( const std::pair<point_abs_omt, string_id<map_extra>> &e ) {
        return e.second == mx_minefield;
    } );

    // If at least one was generated, that's good enough.
    CHECK( successes > 0 );
}

TEST_CASE( "mx_minefield theoretical spawn", "[map_extra][overmap]" )
{
    overmap &om = overmap_buffer.get( point_abs_om() );

    const oter_id road( "road_ns" );
    const oter_id bridgehead( "bridgehead_ground_north" );
    const oter_id bridge( "bridge_north" );

    // The mx_minefield map extra expects to have a particular configuration with
    // three OMTs--a bridgehead, then a road, then a road once again.
    // It does this for four rotations, with the road on the north, south, east,
    // and west of the target point.
    const auto setup_terrain_and_generate = [&]( const tripoint_om_omt & center,
    om_direction::type bridge_direction ) {
        om.ter_set( center, bridgehead );
        om.ter_set( center + om_direction::displace( bridge_direction, 1 ), bridge );
        om.ter_set( center + om_direction::displace( om_direction::opposite( bridge_direction ), 1 ),
                    road );

        tinymap tm;
        tm.load( project_combine( om.pos(), project_to<coords::sm>( center ) ), false );

        const string_id<map_extra> mx_minefield( "mx_minefield" );
        const map_extra_pointer mx_func = MapExtras::get_function( mx_minefield.str() );

        return mx_func( tm, tm.get_abs_sub() );
    };

    // Pick a point in the middle of the overmap so we don't go out of bounds when setting up
    // our terrains.
    const tripoint_om_omt target( 90, 90, 0 );

    // Check that for each direction (north, south, east, west) the map extra generates successfully.
    for( om_direction::type dir : om_direction::all ) {
        CHECK( setup_terrain_and_generate( target, dir ) );
    }
}
