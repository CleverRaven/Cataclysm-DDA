#include <list>
#include <memory>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "lightmap.h"
#include "map.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "type_id.h"

static const efftype_id effect_boomered( "boomered" );
static const efftype_id effect_darkness( "darkness" );

static const trait_id trait_MYOPIC( "MYOPIC" );
static const trait_id trait_URSINE_EYE( "URSINE_EYE" );

// Tests of Character vision and sight
//
// Functions tested:
// Character::recalc_sight_limits
// Character::unimpaired_range
// Character::sight_impaired
// Character::fine_detail_vision_mod
//
// Other related / supporting functions:
// game::reset_light_level
// game::is_in_sunlight
// map::build_map_cache
// map::ambient_light_at

// Character::fine_detail_vision_mod() returns a floating-point number that acts as a multiplier for
// the time taken to perform tasks that require detail vision.  1.0 is ideal lighting conditions,
// while greater than 4.0 means these activities cannot be performed at all.
//
// According to the function docs:
// Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind).
//  1.0 is LIGHT_AMBIENT_LIT or brighter
//  4.0 is a dark clear night, barely bright enough for reading and crafting
//  6.0 is LIGHT_AMBIENT_DIM
//  7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
// 11.0 is zero light or blindness
//
// TODO: Test 'pos' (position) parameter to fine_detail_vision_mod
//
TEST_CASE( "light_and_fine_detail_vision_mod", "[character][sight][light][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    SECTION( "full daylight" ) {
        // Set clock to noon
        calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
        // Build map cache including lightmap
        here.build_map_cache( 0, false );
        REQUIRE( g->is_in_sunlight( dummy.pos() ) );
        // ambient_light_at is ~100.0 at this time of day (this fails if lightmap cache is not built)
        REQUIRE( here.ambient_light_at( dummy.pos() ) == Approx( 100.0f ).margin( 1 ) );

        // 1.0 is LIGHT_AMBIENT_LIT or brighter
        CHECK( dummy.fine_detail_vision_mod() == Approx( 1.0f ) );
    }

    SECTION( "wielding a bright lamp" ) {
        item lamp( "atomic_lamp" );
        dummy.wield( lamp );
        REQUIRE( dummy.active_light() == Approx( 15.0f ) );

        // 1.0 is LIGHT_AMBIENT_LIT or brighter
        CHECK( dummy.fine_detail_vision_mod() == Approx( 1.0f ) );
    }

    // TODO:
    // 4.0 is a dark clear night, barely bright enough for reading and crafting
    // 6.0 is LIGHT_AMBIENT_DIM

    SECTION( "midnight with a new moon" ) {
        // yes, surprisingly, we need to test for this
        tripoint const z_shift = GENERATE( tripoint_above, tripoint_zero );
        dummy.setpos( dummy.pos() + z_shift );
        CAPTURE( z_shift );

        calendar::turn = calendar::turn_zero;
        here.build_map_cache( 0, false );
        REQUIRE_FALSE( g->is_in_sunlight( dummy.pos() ) );
        REQUIRE( here.ambient_light_at( dummy.pos() ) == Approx( LIGHT_AMBIENT_MINIMAL ) );

        // 7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
        CHECK( dummy.fine_detail_vision_mod() == Approx( 7.3f ) );

        dummy.setpos( dummy.pos() - z_shift );
    }

    SECTION( "blindfolded" ) {
        dummy.wear_item( item( "blindfold" ) );
        REQUIRE( dummy.worn_with_flag( flag_BLIND ) );

        // 11.0 is zero light or blindness
        CHECK( dummy.fine_detail_vision_mod() == Approx( 11.0f ) );
    }
}

TEST_CASE( "npc_light_and_fine_detail_vision_mod", "[character][npc][sight][light][vision]" )
{
    Character &u = get_player_character();
    standard_npc n( "Mr. Testerman" );
    n.set_body();

    clear_avatar();
    clear_map();
    tripoint const u_shift = GENERATE( tripoint_zero, tripoint_above );
    CAPTURE( u_shift );
    u.setpos( u.pos() + u_shift );
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    time_point time_dst;
    float expected_vision{};
    if( GENERATE( true, false ) ) {
        time_dst = calendar::turn - time_past_midnight( calendar::turn ) + 12_hours;
        CAPTURE( "daylight" );
        expected_vision = 1.0;
    } else {
        time_dst = calendar::turn - time_past_midnight( calendar::turn );
        CAPTURE( "darkness" );
        expected_vision = 7.3;
    }

    set_time( time_dst );
    REQUIRE( u.fine_detail_vision_mod() == expected_vision );
    SECTION( "NPC on same z-level" ) {
        n.setpos( u.pos() + tripoint_east );
        CHECK( n.fine_detail_vision_mod() == u.fine_detail_vision_mod() );
    }
    SECTION( "NPC on a different z-level" ) {
        n.setpos( u.pos() + tripoint_above );
        // light map is not calculated outside the player character's z-level
        // even if fov_3d_z_range > 0, and building light map on multiple levels
        // could be expensive, so make NPCs able to see things in this case to
        // not interfere with NPC activity.
        CHECK( n.fine_detail_vision_mod() == Approx( 1.0 ) );
    }
}

// Sight limits
//
// Character::unimpaired_range() returns the maximum sight range, factoring in the effects of
// clothing (blindfold or corrective lenses), mutations (nearsightedness or ursine eyes), effects
// (boomered or darkness), or being underwater without suitable eye protection.
//
// Character::sight_impaired() returns true if sight is thus restricted.
//
TEST_CASE( "character_sight_limits", "[character][sight][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    GIVEN( "it is midnight with a new moon" ) {
        calendar::turn = calendar::turn_zero;
        here.build_map_cache( 0, false );
        REQUIRE_FALSE( g->is_in_sunlight( dummy.pos() ) );

        THEN( "sight limit is 60 tiles away" ) {
            dummy.recalc_sight_limits();
            CHECK( dummy.unimpaired_range() == 60 );
        }
    }

    WHEN( "blindfolded" ) {
        dummy.wear_item( item( "blindfold" ) );
        REQUIRE( dummy.worn_with_flag( flag_BLIND ) );

        THEN( "impaired sight, with 0 tiles of range" ) {
            dummy.recalc_sight_limits();
            CHECK( dummy.sight_impaired() );
            CHECK( dummy.unimpaired_range() == 0 );
        }
    }

    WHEN( "boomered" ) {
        dummy.add_effect( effect_boomered, 1_minutes );
        REQUIRE( dummy.has_effect( effect_boomered ) );

        THEN( "impaired sight, with 1 tile of range" ) {
            dummy.recalc_sight_limits();
            CHECK( dummy.sight_impaired() );
            CHECK( dummy.unimpaired_range() == 1 );
        }
    }

    WHEN( "nearsighted" ) {
        dummy.toggle_trait( trait_MYOPIC );
        REQUIRE( dummy.has_trait( trait_MYOPIC ) );

        WHEN( "without glasses" ) {
            dummy.clear_worn();
            REQUIRE_FALSE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

            THEN( "impaired sight, with 12 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 12 );
            }
        }

        WHEN( "wearing glasses" ) {
            dummy.wear_item( item( "glasses_eye" ) );
            REQUIRE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

            THEN( "unimpaired sight, with 60 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
            }
        }
    }

    GIVEN( "darkness effect" ) {
        dummy.add_effect( effect_darkness, 1_minutes );
        REQUIRE( dummy.has_effect( effect_darkness ) );

        THEN( "impaired sight, with 10 tiles of range" ) {
            dummy.recalc_sight_limits();
            CHECK( dummy.sight_impaired() );
            CHECK( dummy.unimpaired_range() == 10 );
        }
    }
}

// Special case of impaired sight - URSINE_EYES mutation causes severely reduced daytime vision
// equivalent to being nearsighted, which can be corrected with glasses. However, they have a
// nighttime vision range that exceeds that of normal characters.
//
// unimpaired_range() returns the range the character can see clearly once all impairments
// have taken their effect.
//
// The sight_max computed by recalc_sight_limits does not include the Beer-Lambert light
// attenuation of a given light level; this is handled by sight_range(), which returns a value from
// [1 .. sight_max].
//
// FIXME: Rename unimpaired_range() to impaired_range()
// (it specifically includes all the things that impair visibility)
//
TEST_CASE( "ursine_vision", "[character][ursine][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    float light_here = 0.0f;

    GIVEN( "character with ursine eyes and no eyeglasses" ) {
        dummy.toggle_trait( trait_URSINE_EYE );
        REQUIRE( dummy.has_trait( trait_URSINE_EYE ) );

        dummy.clear_worn();
        REQUIRE_FALSE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

        WHEN( "under a new moon" ) {
            calendar::turn = calendar::turn_zero;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( LIGHT_AMBIENT_MINIMAL ) );

            THEN( "unimpaired sight, with 7 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 7 );
            }
        }

        WHEN( "under a half moon" ) {
            calendar::turn = calendar::turn_zero + 7_days;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( LIGHT_AMBIENT_DIM ).margin( 1.0f ) );

            THEN( "unimpaired sight, with 8 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 8 );
            }
        }

        WHEN( "under a full moon" ) {
            calendar::turn = calendar::turn_zero + 14_days;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( 7 ) );

            THEN( "unimpaired sight, with 27 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 18 );
            }
        }

        WHEN( "under the noonday sun" ) {
            calendar::turn = calendar::turn_zero + 9_hours + 30_minutes;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( g->is_in_sunlight( dummy.pos() ) );
            REQUIRE( light_here == Approx( 100.0f ).margin( 1 ) );

            THEN( "impaired sight, with 12 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 12 );
                CHECK( dummy.sight_range( light_here ) == 12 );
            }

            // Glasses can correct Ursine Vision in bright light
            AND_WHEN( "wearing glasses" ) {
                dummy.wear_item( item( "glasses_eye" ) );
                REQUIRE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

                THEN( "unimpaired sight, with 87 tiles of range" ) {
                    dummy.recalc_sight_limits();
                    CHECK_FALSE( dummy.sight_impaired() );
                    CHECK( dummy.unimpaired_range() == 60 );
                    CHECK( dummy.sight_range( light_here ) == 87 );
                }
            }
        }
    }
}

