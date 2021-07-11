#include <iosfwd>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "creature.h"
#include "flag.h"
#include "game.h"
#include "item.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "test_statistics.h"
#include "type_id.h"

// Tests for Character suffering
//
// Covers functions:
// - Character::suffer_from_sunburn

// Make character suffer for a while
static void test_suffer( Character &dummy, const time_duration &dur )
{
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.suffer();
    }
}

// Return total focus lost just from suffering over a given time duration (may exceed maximum focus)
static int test_suffer_focus_lost( Character &dummy, const time_duration &dur )
{
    int focus_lost = 0;
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.set_focus( 100 );
        dummy.suffer();
        focus_lost += 100 - dummy.get_focus();
    }
    return focus_lost;
}

// Return total hit points lost for each body part over a given time duration (may exceed max HP)
static std::map<bodypart_id, int> test_suffer_bodypart_hp_lost( Character &dummy,
        const time_duration &dur )
{
    const std::vector<bodypart_id> body_parts_with_hp = dummy.get_all_body_parts(
                get_body_part_flags::only_main );
    // Total hit points lost for each body part
    std::map<bodypart_id, int> bp_hp_lost;
    for( const bodypart_id &bp : body_parts_with_hp ) {
        bp_hp_lost[bp] = 0;
    }
    // Every turn, heal completely, then suffer and accumulate HP loss for each body part
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.healall( 100 );
        dummy.suffer();
        for( const bodypart_id &bp : body_parts_with_hp ) {
            bp_hp_lost[bp] += dummy.get_part_hp_max( bp.id() ) - dummy.get_part_hp_cur( bp.id() );
        }
    }
    return bp_hp_lost;
}

// Return total pain points just from suffering over a given time duration
static int test_suffer_pain_felt( Character &dummy, const time_duration &dur )
{
    int pain_felt = 0;
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.set_pain( 0 );
        dummy.suffer();
        pain_felt += dummy.get_pain();
    }
    return pain_felt;
}

// Suffering from albinism (ALBINO trait)
//
// - Albinism suffering effects occur about once per minute (1/60 chance per turn)
// - If character is wielding an umbrella and wearing sunglasses, all effects are negated
// - If character has body parts more than 1% exposed, they suffer focus loss or pain
// - Most of the time (59/60 chance), 1 focus is lost, or 2 without sunglasses
// - Sometimes (1/60 chance), there is 1 pain instead, or 2 without sunglasses
//
TEST_CASE( "suffering from albinism", "[char][suffer][albino]" )
{
    clear_map();
    avatar &dummy = get_avatar();
    clear_character( dummy );
    g->reset_light_level();

    int focus_lost = 0;
    // FIXME: The random chance of pain is too unprectable to test reliably.
    // Code examples and THEN expectations are left in the tests below as documentation.
    //int pain_felt = 0;

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

            THEN( "they lose about 118 focus per hour" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( focus_lost == Approx( 118 ).margin( 60 ) );
            }

            THEN( "they suffer about 2 pain per hour" ) {
                // 1 pain per hour for unshaded eyes
                // 1 pain per hour for exposed skin
                // Without running a long test, chance of pain is too low to measure effectively
                // This assertion will pass when pain is between 0 and 12 in an hour
                //pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                //CHECK( pain_felt == Approx( 2 ).margin( 10 ) );
            }
        }

        WHEN( "wielding an umbrella and wearing sunglasses" ) {
            dummy.wield( umbrella );
            REQUIRE( dummy.weapon.has_flag( flag_RAIN_PROTECT ) );

            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            THEN( "they suffer no pain or loss of focus" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( dummy.get_pain() == 0 );
                CHECK( focus_lost == 0 );
            }
        }

        WHEN( "entire body is covered with clothing" ) {
            dummy.worn.clear();
            dummy.wear_item( zentai, false );

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer about 1 pain per hour" ) {
                    test_suffer( dummy, 1_hours );
                    // This will pass if pain is between 0 and 6 in an hour
                    CHECK( dummy.get_pain() == Approx( 1 ).margin( 5 ) );
                }
                THEN( "they lose about 59 focus per hour" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( focus_lost == Approx( 59 ).margin( 40 ) );
                }
            }

            AND_WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }
    }
}

// Suffering from Solar Sensitivity (SUNBURN trait)
//
// - Solar Sens. suffering effects occur about 3 times per minute (1/20 chance per turn)
// - If character is wielding an umbrella and wearing sunglasses, all effects are negated
// - If character has body parts more than 1% exposed, they suffer pain and loss of HP
// - Chance of pain and HP loss is directly proportional to skin exposure on each body part
// -
//
TEST_CASE( "suffering from sunburn", "[char][suffer][sunburn]" )
{
    clear_map();
    clear_avatar();
    Character &dummy = get_player_character();
    g->reset_light_level();
    const std::vector<bodypart_id> body_parts_with_hp = dummy.get_all_body_parts(
                get_body_part_flags::only_main );

    int focus_lost = 0;
    int pain_felt = 0;

    item shades( "test_sunglasses" );
    item umbrella( "test_umbrella" );
    item zentai( "test_zentai" );
    item longshirt( "test_longshirt" );

    GIVEN( "avatar is in sunlight with the solar sensitivity trait" ) {
        calendar::turn = calendar::turn_zero + 12_hours;
        REQUIRE( g->is_in_sunlight( dummy.pos() ) );

        dummy.toggle_trait( trait_id( "SUNBURN" ) );
        REQUIRE( dummy.has_trait( trait_id( "SUNBURN" ) ) );

        std::map<bodypart_id, int> bp_hp_lost;
        WHEN( "totally naked and exposed" ) {
            dummy.worn.clear();

            THEN( "they suffer injury on every body part several times a minute" ) {
                // Should lose an average of 6 HP per minute from each body part with hit points
                // (head, torso, both arms, both legs)
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 10_minutes );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    CHECK( bp_hp_lost[bp] == Approx( 60 ).margin( 40 ) );
                }
            }

            THEN( "they suffer pain several times a minute" ) {
                // This will pass if pain is between 0 and 90, but 3/minute is expected baseline
                //pain_felt = test_suffer_pain_felt( dummy, 10_minutes );
                //CHECK( pain_felt == Approx( 30 ).margin( 60 ) );
            }
        }

        WHEN( "naked and wielding an umbrella, with or without sunglasses" ) {
            dummy.worn.clear();
            dummy.wield( umbrella );
            REQUIRE( dummy.weapon.has_flag( flag_RAIN_PROTECT ) );

            // Umbrella completely shields the skin from exposure when wielded
            THEN( "they suffer no injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 10_minutes );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    CHECK( bp_hp_lost[bp] == 0 );
                }
            }

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );
                THEN( "they suffer pain" ) {
                    // Only about 3 pain per hour from exposed eyes
                    // This assertion will pass when pain is between 0 and 13 in an hour
                    //pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                    //CHECK( pain_felt == Approx( 3 ).margin( 10 ) );
                }
            }

            // Sunglasses protect from glare and pain
            WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no pain" ) {
                    pain_felt = test_suffer_pain_felt( dummy, 10_minutes );
                    CHECK( pain_felt == 0 );
                }
            }

        }

        WHEN( "torso and arms are 90% covered" ) {
            dummy.worn.clear();
            dummy.wear_item( longshirt, false );

            THEN( "damage to torso is 90% less than other parts" ) {
                time_duration t = 10_minutes;
                int num_turns = t / 1_turns;

                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, t );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    if( bp.id().str() == "torso" ) {
                        // Torso has only 10% chance losing 2 HP, 3x per minute
                        CHECK_THAT( bp_hp_lost[bp] / 2,
                                    IsBinomialObservation( num_turns, 1.0 / 200 ) );
                    } else if( bp.id().str() == "arm_l" || bp.id().str() == "arm_r" ) {
                        // Arms have 10% chance of losing 1 HP, 3x per minute (6 in 10m)
                        // But hands are exposed, and still lose 1 HP, 3x per minute (30 in 10m)
                        CHECK_THAT( bp_hp_lost[bp],
                                    IsBinomialObservation( num_turns, 1.0 / 200 + 1.0 / 20 ) );
                    } else {
                        // All other parts lose 1 HP, 3x per minute (30 in 10m)
                        // but legs+feet combine, and head+mouth combine (60 in 10m)
                        CHECK_THAT( bp_hp_lost[bp], IsBinomialObservation( num_turns, 2.0 / 20 ) );
                    }
                }
            }
        }

        WHEN( "entire body is covered" ) {
            dummy.worn.clear();
            dummy.wear_item( zentai, false );

            THEN( "they suffer no injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 10_minutes );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    CHECK( bp_hp_lost[bp] == 0 );
                }
            }

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer loss of focus" ) {
                    // Lose focus about 3x the rate of Albino, about 59 focus every 20 minutes
                    focus_lost = test_suffer_focus_lost( dummy, 20_minutes );
                    CHECK( focus_lost == Approx( 59 ).margin( 40 ) );
                }
                THEN( "they suffer pain" ) {
                    // Only about 3 pain per hour from exposed eyes
                    // This assertion will pass when pain is between 0 and 13 in an hour
                    //pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                    //CHECK( pain_felt == Approx( 3 ).margin( 10 ) );
                }
            }

            WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }
    }
}

