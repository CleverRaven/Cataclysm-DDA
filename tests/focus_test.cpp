#include "catch/catch.hpp"

#include "avatar.h"

#include "player_helpers.h"

TEST_CASE( "focus" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.get_morale_level() == 0 );
    int previous_focus = 0;
    SECTION( "equilibrium" ) {
        you.focus_pool = 100;
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.focus_pool == 100 );
        }
    }
    SECTION( "converges up" ) {
        previous_focus = you.focus_pool = 0;
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.focus_pool >= previous_focus );
            previous_focus = you.focus_pool;
        }
        CHECK( you.focus_pool > 50 );
    }
    SECTION( "converges down" ) {
        previous_focus = you.focus_pool = 200;
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.focus_pool <= previous_focus );
            previous_focus = you.focus_pool;
        }
        CHECK( you.focus_pool < 150 );
    }
    SECTION( "drains with practice" ) {
        previous_focus = you.focus_pool = 100;
        for( int i = 0; i < 600; ++i ) {
            you.practice( skill_id( "fabrication" ), 1, 10, true );
            if( i % 60 == 0 ) {
                you.update_mental_focus();
            }
            CHECK( you.focus_pool <= previous_focus + 1 );
            previous_focus = you.focus_pool;
        }
        CHECK( you.focus_pool < 50 );
    }
    SECTION( "drains rapidly with large practice" ) {
        you.practice( skill_id( "fabrication" ), 1000, 10, true );
        CHECK( you.focus_pool < 10 );
    }
    SECTION( "time to level" ) {
        REQUIRE( you.get_skill_level( skill_id( "fabrication" ) ) == 0 );
        std::array<int, 11> expected_practice_times = {{
                0, 173, 2137, 6303, 12137, 19637, 28803, 39637, 52137, 66303, 82137
            }
        };
        for( int lvl = 1; lvl <= 10; ++lvl ) {
            int turns = 0;
            you.focus_pool = 100;
            while( you.get_skill_level( skill_id( "fabrication" ) ) < lvl ) {
                you.practice( skill_id( "fabrication" ), 1, lvl, true );
                if( turns % 60 == 0 ) {
                    you.update_mental_focus();
                }
                turns++;
            }
            CAPTURE( lvl );
            CHECK( turns == expected_practice_times[ lvl ] );
        }
    }
}
