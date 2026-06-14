#include <array>
#include <cmath>
#include <limits>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "player_helpers.h"
#include "skill.h"
#include "type_id.h"

static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_smg( "smg" );

TEST_CASE( "focus" )
{
    clear_avatar();
    avatar &you = get_avatar();
    REQUIRE( you.get_morale_level() == 0 );
    int previous_focus = 0;
    SECTION( "equilibrium" ) {
        you.set_focus( 100 );
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.get_focus() == 100 );
        }
    }
    SECTION( "converges up" ) {
        you.set_focus( 0 );
        previous_focus = you.get_focus();
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.get_focus() >= previous_focus );
            previous_focus = you.get_focus();
        }
        CHECK( you.get_focus() > 50 );
    }
    SECTION( "converges down" ) {
        you.set_focus( 200 );
        previous_focus = you.get_focus();
        for( int i = 0; i < 100; ++i ) {
            you.update_mental_focus();
            CHECK( you.get_focus() <= previous_focus );
            previous_focus = you.get_focus();
        }
        CHECK( you.get_focus() < 150 );
    }
    SECTION( "drains with practice" ) {
        you.set_focus( 100 );
        previous_focus = you.get_focus();
        for( int i = 0; i < 600; ++i ) {
            you.practice( skill_fabrication, 1, 10, true );
            if( i % 60 == 0 ) {
                you.update_mental_focus();
            }
            CHECK( you.get_focus() <= previous_focus + 1 );
            previous_focus = you.get_focus();
        }
        CHECK( you.get_focus() < 50 );
    }
    SECTION( "nominal time to drain focus" ) {
        you.set_focus( 100 );
        previous_focus = you.get_focus();
        int current_threshold = 0;
        std::array<int, 9> drain_thresholds = {{
                0, 9, 23, 40, 64, 96, 147, 230, 418
            }
        };
        int i = 0;
        for( ; i < 5000 && previous_focus > 12; ++i ) {
            you.practice( skill_fabrication, 1, 10, true );
            if( i % 60 == 0 ) {
                you.update_mental_focus();
            }
            previous_focus = you.get_focus();
            if( previous_focus <= ( 10 - current_threshold ) * 10 ) {
                CAPTURE( previous_focus );
                CHECK( i == drain_thresholds[ current_threshold ] );
                current_threshold++;
            }
        }
        CHECK( previous_focus == 12 );
        CHECK( i == 1192 );
    }
    SECTION( "drains rapidly with large practice" ) {
        you.practice( skill_fabrication, 1000, 10, true );
        CHECK( you.get_focus() < 10 );
    }
    SECTION( "large practice on combat skills drains focus to minimum" ) {
        you.set_focus( 100 );
        REQUIRE( you.get_focus() == 100 );
        REQUIRE( skill_smg->is_combat_skill() );

        // This is basically ensuring there isn't UB when squaring 'amount'
        // So let's give a value that will definitely do that without handling
        // But not so large that less extreme manipulations will cause problems
        const int practice_amount = 2 * std::sqrt( std::numeric_limits<int>::max() );
        you.practice( skill_smg, practice_amount );

        // This still succeeds, even when the UB is triggered
        // that's fine, the real objective is to set off UBsan
        CHECK( you.get_focus() == 1 );
    }
    SECTION( "time to level" ) {
        REQUIRE( static_cast<int>( you.get_skill_level( skill_fabrication ) ) == 0 );
        std::array<int, 11> expected_practice_times = {{
                0, 173, 2137, 6304, 12137, 19637, 28804, 39637, 52137, 66304, 82137
            }
        };
        for( int lvl = 1; lvl <= 10; ++lvl ) {
            int turns = 0;
            you.set_focus( 100 );
            while( static_cast<int>( you.get_skill_level( skill_fabrication ) ) < lvl ) {
                you.practice( skill_fabrication, 1, lvl, true );
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
