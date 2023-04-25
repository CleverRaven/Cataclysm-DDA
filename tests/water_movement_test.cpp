#include <memory>

#include "avatar.h"
#include "cata_catch.h"
#include "creature.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "type_id.h"

TEST_CASE( "avatar diving", "[diving]" )
{
    const ter_id t_water_dp( "t_water_dp" );
    const ter_id t_water_cube( "t_water_cube" );
    const ter_id t_lake_bed( "t_lake_bed" );

    build_water_test_map( t_water_dp, t_water_cube, t_lake_bed );
    map &here = get_map();

    clear_avatar();
    Character &dummy = get_player_character();
    const tripoint test_origin( 60, 60, 0 );

    REQUIRE( here.ter( test_origin ) == t_water_dp );
    REQUIRE( here.ter( test_origin + tripoint_below ) == t_water_cube );
    REQUIRE( here.ter( test_origin + tripoint( 0, 0, -2 ) ) == t_lake_bed );

    GIVEN( "avatar is above water at z0" ) {
        dummy.set_underwater( false );
        dummy.setpos( test_origin );
        g->vertical_shift( 0 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is not underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( !dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z0" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin );
        g->vertical_shift( 0 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-1" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint_below );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_cube" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is not underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( !dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z-1" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin + tripoint_below );
        g->vertical_shift( -1 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-2" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint( 0, 0, -2 ) );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_lake_bed" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is underwater at z0" ) {
                REQUIRE( dummy.pos() == test_origin );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_dp" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }
    }

    GIVEN( "avatar is underwater at z-2" ) {
        dummy.set_underwater( true );
        dummy.setpos( test_origin + tripoint( 0, 0, -2 ) );
        g->vertical_shift( -2 );

        WHEN( "avatar dives down" ) {
            g->vertical_move( -1, false );

            THEN( "avatar is underwater at z-2" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint( 0, 0, -2 ) );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_lake_bed" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }

        WHEN( "avatar swims up" ) {
            g->vertical_move( 1, false );

            THEN( "avatar is underwater at z-1" ) {
                REQUIRE( dummy.pos() == test_origin + tripoint_below );
                REQUIRE( here.ter( dummy.pos() ) == ter_id( "t_water_cube" ) );
                REQUIRE( dummy.is_underwater() );
            }
        }
    }

    // Put us back at 0. We shouldn't have to do this but other tests are
    // making assumptions about what z-level they're on.
    g->vertical_shift( 0 );
}
