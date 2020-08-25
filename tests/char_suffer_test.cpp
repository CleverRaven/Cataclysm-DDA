#include "catch/catch.hpp"

#include "avatar.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

// Tests for Character suffering
//
// Covers functions:
// - Character::suffer_from_albinism
// - Character::suffer_from_sunburn

// Make character suffer for num_turns and return the amount of focus lost just from suffering
static int test_suffer_turns_focus_lost( Character &dummy, const int num_turns )
{
    int focus_lost = 0;
    for( int turn = 1; turn <= num_turns; turn++ ) {
        int focus_before = dummy.focus_pool;
        dummy.suffer();
        focus_lost += focus_before - dummy.focus_pool;
        dummy.set_moves( 100 );
        dummy.process_turn();
    }
    return focus_lost;
}

// Make character suffer for num_turns and return the amount of HP lost on a given bodypart
static int test_suffer_turns_hp_lost( Character &dummy, const int num_turns, const bodypart_id &bp )
{
    int hp_before = dummy.get_part_hp_cur( bp );
    for( int turn = 1; turn <= num_turns; turn++ ) {
        dummy.suffer();
        dummy.set_moves( 100 );
        dummy.process_turn();
    }
    return hp_before - dummy.get_part_hp_cur( bp );
}

// When suffering from albinism:
// - Only happens about once per minute: one_turn_in( 1_minutes )
// - Without sunglasses or blindfold, suffer 1 pain and 1 focus loss
//   gated again by a one_turn_in( 1_minutes ) chance, total of 1/3600
// - If any bodypart except eyes are exposed, suffer 1 pain and 1 focus loss
//   gated again by a one_turn_in( 1_minutes ) chance, total of 1/3600
//
TEST_CASE( "suffering from albinism", "[char][suffer][albino]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    clear_map();

    const int MAX_TURNS = 10000;
    int focus_lost = 0;

    // Need sunglasses to protect the eyes, no matter how covered the rest of the body is
    item shades( "test_sunglasses" );
    // Umbrella can protect completely
    item umbrella( "test_umbrella" );

    // hazmat suit has 100% coverage, but eyes and mouth are exposed
    item hazmat( "test_hazmat_suit" );
    // zentai has 100% coverage over all body parts
    item zentai( "test_zentai" );
    // long-sleeve shirt has 90% coverage on torso and arms
    item longshirt( "test_longshirt" );

    GIVEN( "avatar is in sunlight with the albino trait" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        REQUIRE( g->is_in_sunlight( dummy.pos() ) );

        dummy.toggle_trait( trait_id( "ALBINO" ) );
        REQUIRE( dummy.has_trait( trait_id( "ALBINO" ) ) );

        WHEN( "totally naked and exposed" ) {
            dummy.worn.clear();

            // 1 pain per hour for unshaded eyes
            // 1 pain per hour for exposed skin
            THEN( "they suffer pain and loss of focus" ) {
                focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                CHECK( dummy.get_pain() > 0 );
                CHECK( focus_lost > 1 );
            }

            WHEN( "wielding an umbrella and wearing sunglasses" ) {
                dummy.wield( umbrella );
                REQUIRE( dummy.weapon.has_flag( "RAIN_PROTECT" ) );

                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( "SUN_GLASSES" ) );

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }

        WHEN( "entire body is covered with clothing" ) {
            dummy.worn.clear();
            dummy.wear_item( zentai, false );

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( "SUN_GLASSES" ) );
                THEN( "they suffer some pain and loss of focus" ) {
                    focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                    CHECK( dummy.get_pain() > 0 );
                    CHECK( focus_lost > 1 );
                }
            }

            AND_WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( "SUN_GLASSES" ) );

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }

        WHEN( "only one body part is exposed" ) {
            dummy.worn.clear();
            dummy.wear_item( hazmat, false );
            dummy.wear_item( shades, false );

            THEN( "they suffer pain and loss of focus" ) {
                focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                CHECK( dummy.get_pain() > 0 );
                CHECK( focus_lost > 1 );
            }
        }

        WHEN( "multiple body parts are exposed" ) {
            dummy.worn.clear();
            dummy.wear_item( shades, false );
            dummy.wear_item( longshirt, false );

            THEN( "they suffer pain and loss of focus" ) {
                focus_lost = test_suffer_turns_focus_lost( dummy, MAX_TURNS );
                CHECK( dummy.get_pain() > 0 );
                CHECK( focus_lost > 1 );
            }
        }
    }
}

TEST_CASE( "suffering from sunburn", "[char][suffer][sunburn]" )
{
    Character &dummy = get_player_character();
    clear_avatar();
    clear_map();

    GIVEN( "avatar is in sunlight with the solar sensitivity trait" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        REQUIRE( g->is_in_sunlight( dummy.pos() ) );

        dummy.toggle_trait( trait_id( "SUNBURN" ) );
        REQUIRE( dummy.has_trait( trait_id( "SUNBURN" ) ) );

        WHEN( "totally naked and exposed" ) {
            dummy.worn.clear();

            int hp_lost = 0;

            THEN( "they suffer injury on every body part several times a minute" ) {
                hp_lost = test_suffer_turns_hp_lost( dummy, 60, bodypart_id( "torso" ) );
                CHECK( hp_lost > 0 );
            }
        }
    }
}

