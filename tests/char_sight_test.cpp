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
#include "player_helpers.h"
#include "type_id.h"

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
TEST_CASE( "light and fine_detail_vision_mod", "[character][sight][light][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();

    SECTION( "full daylight" ) {
        // Set clock to noon
        calendar::turn = calendar::turn_zero + 12_hours;
        // Build map cache including lightmap
        here.build_map_cache( 0, false );
        REQUIRE( g->is_in_sunlight( dummy.pos() ) );
        // ambient_light_at is 100.0 in full sun (this fails if lightmap cache is not built)
        REQUIRE( here.ambient_light_at( dummy.pos() ) == Approx( 100.0f ) );

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
        calendar::turn = calendar::turn_zero;
        here.build_map_cache( 0, false );
        REQUIRE_FALSE( g->is_in_sunlight( dummy.pos() ) );
        REQUIRE( here.ambient_light_at( dummy.pos() ) == Approx( LIGHT_AMBIENT_MINIMAL ) );

        // 7.3 is LIGHT_AMBIENT_MINIMAL, a dark cloudy night, unlit indoors
        CHECK( dummy.fine_detail_vision_mod() == Approx( 7.3f ) );
    }

    SECTION( "blindfolded" ) {
        dummy.wear_item( item( "blindfold" ) );
        REQUIRE( dummy.worn_with_flag( flag_BLIND ) );

        // 11.0 is zero light or blindness
        CHECK( dummy.fine_detail_vision_mod() == Approx( 11.0f ) );
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
TEST_CASE( "character sight limits", "[character][sight][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();

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
        dummy.add_effect( efftype_id( "boomered" ), 1_minutes );
        REQUIRE( dummy.has_effect( efftype_id( "boomered" ) ) );

        THEN( "impaired sight, with 1 tile of range" ) {
            dummy.recalc_sight_limits();
            CHECK( dummy.sight_impaired() );
            CHECK( dummy.unimpaired_range() == 1 );
        }
    }

    WHEN( "nearsighted" ) {
        dummy.toggle_trait( trait_id( "MYOPIC" ) );
        REQUIRE( dummy.has_trait( trait_id( "MYOPIC" ) ) );

        WHEN( "without glasses" ) {
            dummy.worn.clear();
            REQUIRE_FALSE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

            THEN( "impaired sight, with 4 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 4 );
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
        dummy.add_effect( efftype_id( "darkness" ), 1_minutes );
        REQUIRE( dummy.has_effect( efftype_id( "darkness" ) ) );

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
// Contrary to its name, the range returned by unimpaired_range() represents maximum visibility WITH
// IMPAIRMENTS (that is, affected by the same things that cause sight_impaired() to return true).
//
// The sight_max computed by recalc_sight_limits does not include is the Beer-Lambert light
// attenuation of a given light level; this is handled by sight_range(), which returns a value from
// [1 .. sight_max].
//
// FIXME: Rename unimpaired_range() to impaired_range()
// (it specifically includes all the things that impair visibility)
//
TEST_CASE( "ursine vision", "[character][ursine][vision]" )
{
    Character &dummy = get_player_character();
    map &here = get_map();

    clear_avatar();
    clear_map();
    g->reset_light_level();

    float light_here = 0.0f;

    GIVEN( "character with ursine eyes and no eyeglasses" ) {
        dummy.toggle_trait( trait_id( "URSINE_EYE" ) );
        REQUIRE( dummy.has_trait( trait_id( "URSINE_EYE" ) ) );

        dummy.worn.clear();
        REQUIRE_FALSE( dummy.worn_with_flag( flag_FIX_NEARSIGHT ) );

        WHEN( "under a new moon" ) {
            calendar::turn = calendar::turn_zero;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( LIGHT_AMBIENT_MINIMAL ) );

            THEN( "unimpaired sight, with 4 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 4 );
            }
        }

        WHEN( "under a half moon" ) {
            calendar::turn = calendar::turn_zero + 7_days;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( LIGHT_AMBIENT_DIM ).margin( 1.0f ) );

            THEN( "unimpaired sight, with 10 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 10 );
            }
        }

        WHEN( "under a full moon" ) {
            calendar::turn = calendar::turn_zero + 14_days;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( light_here == Approx( LIGHT_AMBIENT_LIT ) );

            THEN( "unimpaired sight, with 27 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK_FALSE( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 60 );
                CHECK( dummy.sight_range( light_here ) == 27 );
            }
        }

        WHEN( "under the noonday sun" ) {
            calendar::turn = calendar::turn_zero + 12_hours;
            here.build_map_cache( 0, false );
            light_here = here.ambient_light_at( dummy.pos() );
            REQUIRE( g->is_in_sunlight( dummy.pos() ) );
            REQUIRE( light_here == Approx( 100.0f ) );

            THEN( "impaired sight, with 4 tiles of range" ) {
                dummy.recalc_sight_limits();
                CHECK( dummy.sight_impaired() );
                CHECK( dummy.unimpaired_range() == 4 );
                CHECK( dummy.sight_range( light_here ) == 4 );
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

