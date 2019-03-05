#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player.h"

TEST_CASE( "destroy_grabbed_furniture" )
{
    clear_map();
    GIVEN( "Furniture grabbed by the player" ) {
        const tripoint test_origin( 60, 60, 0 );
        g->u.setpos( test_origin );
        const tripoint grab_point = test_origin + tripoint( 1, 0, 0 );
        g->m.furn_set( grab_point, furn_id( "f_chair" ) );
        g->u.grab( OBJECT_FURNITURE, grab_point );
        WHEN( "The furniture grabbed by the player is destroyed" ) {
            g->m.destroy( grab_point );
            THEN( "The player's grab is released" ) {
                CHECK( g->u.get_grab_type() == OBJECT_NONE );
                CHECK( g->u.grab_point == tripoint_zero );
            }
        }
    }
}

TEST_CASE( "map_bounds_checking" )
{
    map m;
    m.load( 0, 0, 0, false );
    for( int x = -1; x <= MAPSIZE_X; ++x ) {
        for( int y = -1; y <= MAPSIZE_Y; ++y ) {
            for( int z = -OVERMAP_DEPTH - 1; z <= OVERMAP_HEIGHT + 1; ++z ) {
                INFO( "( " << x << ", " << y << ", " << z << " )" );
                if( x < 0 || x >= MAPSIZE_X ||
                    y < 0 || y >= MAPSIZE_Y ||
                    z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
                    CHECK( !m.ter( { x, y, z } ) );
                } else {
                    CHECK( m.ter( { x, y, z } ) );
                }
            }
        }
    }
}

TEST_CASE( "tinymap_bounds_checking" )
{
    tinymap m;
    m.load( 0, 0, 0, false );
    for( int x = -1; x <= SEEX * 2; ++x ) {
        for( int y = -1; y <= SEEY * 2; ++y ) {
            for( int z = -OVERMAP_DEPTH - 1; z <= OVERMAP_HEIGHT + 1; ++z ) {
                INFO( "( " << x << ", " << y << ", " << z << " )" );
                if( x < 0 || x >= SEEX * 2 ||
                    y < 0 || y >= SEEY * 2 ||
                    z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
                    CHECK( !m.ter( { x, y, z } ) );
                } else {
                    CHECK( m.ter( { x, y, z } ) );
                }
            }
        }
    }
}
