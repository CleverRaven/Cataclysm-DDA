#include "catch/catch.hpp"
#include "submap.h"
#include "game_constants.h"
#include "int_id.h"
#include "point.h"
#include "type_id.h"

TEST_CASE( "submap rotation", "[submap]" )
{
    // Corners are labelled starting from the upper-left one, clockwise.
    constexpr auto corner_1 = point_zero;
    constexpr auto corner_2 = point{ SEEX - 1, 0 };
    constexpr auto corner_3 = point{ SEEX - 1, SEEY - 1 };
    constexpr auto corner_4 = point{ 0, SEEY - 1 };

    constexpr auto center_1 = point{ SEEX / 2 - 1, SEEY / 2 - 1 };
    constexpr auto center_2 = point{ SEEX / 2, SEEY / 2 - 1 };
    constexpr auto center_3 = point{ SEEX / 2, SEEY / 2 };
    constexpr auto center_4 = point{ SEEX / 2 - 1, SEEY / 2 };

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
