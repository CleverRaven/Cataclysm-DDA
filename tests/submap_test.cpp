#include "cata_catch.h"
#include "coordinates.h"
#include "map_scale_constants.h"
#include "point.h"
#include "submap.h"
#include "type_id.h"

TEST_CASE( "submap_rotation", "[submap]" )
{
    // Corners are labelled starting from the upper-left one, clockwise.
    constexpr point_sm_ms corner_1{};
    constexpr point_sm_ms corner_2 = { SEEX - 1, 0 };
    constexpr point_sm_ms corner_3 = { SEEX - 1, SEEY - 1 };
    constexpr point_sm_ms corner_4 = { 0, SEEY - 1 };

    constexpr point_sm_ms center_1 = { SEEX / 2 - 1, SEEY / 2 - 1 };
    constexpr point_sm_ms center_2 = { SEEX / 2, SEEY / 2 - 1 };
    constexpr point_sm_ms center_3 = { SEEX / 2, SEEY / 2 };
    constexpr point_sm_ms center_4 = { SEEX / 2 - 1, SEEY / 2 };

    GIVEN( "a submap with marks" ) {
        submap sm;

        sm.set_ter( corner_1, ter_id( 1 ) );
        sm.set_ter( corner_2, ter_id( 2 ) );
        sm.set_ter( corner_3, ter_id( 3 ) );
        sm.set_ter( corner_4, ter_id( 4 ) );

        sm.set_ter( center_1, ter_id( 1 ) );
        sm.set_ter( center_2, ter_id( 2 ) );
        sm.set_ter( center_3, ter_id( 3 ) );
        sm.set_ter( center_4, ter_id( 4 ) );

        WHEN( "it gets rotated for 0 turns (no rotation)" ) {
            sm.rotate( 0 );

            CHECK( sm.get_ter( corner_1 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( corner_2 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( corner_3 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( corner_4 ) == ter_id( 4 ) );

            CHECK( sm.get_ter( center_1 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( center_2 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( center_3 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( center_4 ) == ter_id( 4 ) );
        }

        WHEN( "it gets rotated for 1 turn" ) {
            sm.rotate( 1 );

            CHECK( sm.get_ter( corner_1 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( corner_2 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( corner_3 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( corner_4 ) == ter_id( 3 ) );

            CHECK( sm.get_ter( center_1 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( center_2 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( center_3 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( center_4 ) == ter_id( 3 ) );
        }

        WHEN( "it gets rotated for 2 turns" ) {
            sm.rotate( 2 );

            CHECK( sm.get_ter( corner_1 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( corner_2 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( corner_3 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( corner_4 ) == ter_id( 2 ) );

            CHECK( sm.get_ter( center_1 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( center_2 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( center_3 ) == ter_id( 1 ) );
            CHECK( sm.get_ter( center_4 ) == ter_id( 2 ) );
        }

        WHEN( "it gets rotated for 3 turns" ) {
            sm.rotate( 3 );

            CHECK( sm.get_ter( corner_1 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( corner_2 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( corner_3 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( corner_4 ) == ter_id( 1 ) );

            CHECK( sm.get_ter( center_1 ) == ter_id( 2 ) );
            CHECK( sm.get_ter( center_2 ) == ter_id( 3 ) );
            CHECK( sm.get_ter( center_3 ) == ter_id( 4 ) );
            CHECK( sm.get_ter( center_4 ) == ter_id( 1 ) );
        }
    }
}

TEST_CASE( "submap_rotation2", "[submap]" )
{
    int rotation_turns = GENERATE( 0, 1, 2, 3 );
    CAPTURE( rotation_turns );
    submap sm;
    submap sm_copy;

    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            sm.set_radiation( {x, y}, x + y * SEEX );
            sm_copy.set_radiation( {x, y}, x + y * SEEX );
        }
    }

    sm.rotate( rotation_turns );

    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            point_sm_ms p( x, y );
            point_sm_ms p_after_rotation = p.rotate( rotation_turns, {SEEX, SEEY} );

            CAPTURE( p, p_after_rotation );
            CHECK( sm.get_radiation( p_after_rotation ) == sm_copy.get_radiation( p ) );
        }
    }
}
