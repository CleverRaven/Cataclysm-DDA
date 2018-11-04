#include "catch/catch.hpp"

#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "mtype.h"
#include "options.h"
#include "player.h"

#include "map_helpers.h"

#include <fstream>
#include <string>
#include <vector>

// Destroying pavement with a pickaxe should not leave t_flat_roof.
// See issue #24707:
// https://github.com/CleverRaven/Cataclysm-DDA/issues/24707
// Behavior may depend on ZLEVELS being set.
TEST_CASE( "pavement_destroy", "[.]" )
{
    ter_id flat_roof_id = ter_id( "t_flat_roof" );
    REQUIRE( flat_roof_id != t_null );

    bool zlevels_set = get_option<bool>( "ZLEVELS" );
    INFO( "ZLEVELS is " << zlevels_set );
    clear_map();
    // Populate the map with pavement.
    tripoint pt( 0, 0, 0 );
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

    bool zlevels_set = get_option<bool>( "ZLEVELS" );
    INFO( "ZLEVELS is " << zlevels_set );

    clear_map();
    std::vector<ter_id> test_terrain_id = {
        ter_id( "t_dirt" ), ter_id( "t_grass" )
    };
    int idx = 0;
    int area_dim = 16;
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

    tripoint area_center( area_dim / 2, area_dim / 2, 0 );
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