#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "player.h"

TEST_CASE( "destroy_grabbed_furniture" ) {
    GIVEN( "Furniture grabbed by the player" ) {
        tripoint test_origin( 60, 60, 0 );
        g->u.setpos( test_origin );
        tripoint grab_point = test_origin + tripoint( 1, 0, 0 );
        g->m.furn_set( grab_point, furn_id( "f_chair" ) );
        g->u.grab_type = OBJECT_FURNITURE;
        g->u.grab_point = grab_point;
        WHEN( "The furniture grabbed by the player is destroyed" ) {
            g->m.destroy( grab_point );
            THEN( "The player's grab is released" ) {
                CHECK( g->u.grab_type == OBJECT_NONE );
                CHECK( g->u.grab_point == tripoint_zero );
            }
        }
    }
}
