#include <string>
#include <vector>

#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "mapdata.h"
#include "options.h"

// Destroying pavement with a pickaxe should not leave t_flat_roof.
// See issue #24707:
// https://github.com/CleverRaven/Cataclysm-DDA/issues/24707
// Behavior may depend on ZLEVELS being set.
TEST_CASE( "pavement_destroy", "[.]" )
{
    const ter_id flat_roof_id = ter_id( "t_flat_roof" );
    REQUIRE( flat_roof_id != t_null );

    const bool zlevels_set = get_option<bool>( "ZLEVELS" );
    INFO( "ZLEVELS is " << zlevels_set );
    clear_map_and_put_player_underground();
    // Populate the map with pavement.
    const tripoint pt( 0, 0, 0 );
    g->m.ter_set( pt, ter_id( "t_pavement" ) );

    // Destroy it
    g->m.destroy( pt, true );
    ter_id after_destroy = g->m.ter( pt );
    if( after_destroy == flat_roof_id ) {
        FAIL( flat_roof_id.obj().name() << " found after destroying pavement" );
    } else {
        INFO( "After destroy, ground is " << after_destroy.obj().name() );
    }
}

// Ground-destroying explosions on dirt or grass shouldn't leave t_flat_roof.
// See issue #23250:
// https://github.com/CleverRaven/Cataclysm-DDA/issues/23250
// Behavior may depend on ZLEVELS being set.
TEST_CASE( "explosion_on_ground", "[.]" )
{
    ter_id flat_roof_id = ter_id( "t_flat_roof" );
    REQUIRE( flat_roof_id != t_null );

    const bool zlevels_set = get_option<bool>( "ZLEVELS" );
    INFO( "ZLEVELS is " << zlevels_set );

    clear_map_and_put_player_underground();
    std::vector<ter_id> test_terrain_id = {
        ter_id( "t_dirt" ), ter_id( "t_grass" )
    };
    int idx = 0;
    const int area_dim = 16;
    // Populate map with various test terrain.
    for( int x = 0; x < area_dim; x++ ) {
        for( int y = 0; y < area_dim; y++ ) {
            g->m.ter_set( tripoint( x, y, 0 ), test_terrain_id[idx] );
            idx = ( idx + 1 ) % test_terrain_id.size();
        }
    }
    // Detonate an RDX keg item in the middle of the populated map space
    itype_id rdx_keg_typeid( "tool_rdx_charge_act" );
    REQUIRE( item::type_is_defined( rdx_keg_typeid ) );

    const tripoint area_center( area_dim / 2, area_dim / 2, 0 );
    item rdx_keg( rdx_keg_typeid );
    rdx_keg.charges = 0;
    rdx_keg.type->invoke( g->u, rdx_keg, area_center );

    // Check area to see if any t_flat_roof is present.
    for( int x = 0; x < area_dim; x++ ) {
        for( int y = 0; y < area_dim; y++ ) {
            tripoint pt( x, y, 0 );
            ter_id t_id = g->m.ter( pt );
            if( t_id == flat_roof_id ) {
                FAIL( "After explosion, " << t_id.obj().name() << " found at " << x << "," << y );
            }
        }
    }
}

// Ground-destroying explosions on t_floor with a t_rock_floor basement
// below should create some t_open_air, not just t_flat_roof (which is
// the defined roof of a t_rock-floor).
// Behavior depends on ZLEVELS being set.
TEST_CASE( "explosion_on_floor_with_rock_floor_basement", "[.]" )
{
    ter_id flat_roof_id = ter_id( "t_flat_roof" );
    ter_id floor_id = ter_id( "t_floor" );
    ter_id rock_floor_id = ter_id( "t_rock_floor" );
    ter_id open_air_id = ter_id( "t_open_air" );

    REQUIRE( flat_roof_id != t_null );
    REQUIRE( floor_id != t_null );
    REQUIRE( rock_floor_id != t_null );
    REQUIRE( open_air_id != t_null );

    const bool zlevels_set = get_option<bool>( "ZLEVELS" );
    INFO( "ZLEVELS is " << zlevels_set );

    clear_map_and_put_player_underground();

    const int area_dim = 24;
    for( int x = 0; x < area_dim; x++ ) {
        for( int y = 0; y < area_dim; y++ ) {
            g->m.ter_set( tripoint( x, y, 0 ), floor_id );
            g->m.ter_set( tripoint( x, y, -1 ), rock_floor_id );
        }
    }
    // Detonate an RDX keg item in the middle of the populated map space
    itype_id rdx_keg_typeid( "tool_rdx_charge_act" );
    REQUIRE( item::type_is_defined( rdx_keg_typeid ) );

    const tripoint area_center( area_dim / 2, area_dim / 2, 0 );
    item rdx_keg( rdx_keg_typeid );
    rdx_keg.charges = 0;
    rdx_keg.type->invoke( g->u, rdx_keg, area_center );

    // Check z0 for open air
    bool found_open_air = false;
    for( int x = 0; x < area_dim; x++ ) {
        for( int y = 0; y < area_dim; y++ ) {
            tripoint pt( x, y, 0 );
            ter_id t_id = g->m.ter( pt );
            INFO( "t " << t_id.obj().name() << " at " << x << "," << y );
            if( t_id == open_air_id ) {
                found_open_air = true;
                break;
            }
        }
        if( found_open_air ) {
            break;
        }
    }

    if( !found_open_air ) {
        FAIL( "After explosion, no open air was found" );
    }
}
