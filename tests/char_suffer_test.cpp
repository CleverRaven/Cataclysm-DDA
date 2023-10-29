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
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "test_statistics.h"
#include "type_id.h"

static const efftype_id effect_grabbed( "grabbed" );

static const trait_id trait_ALBINO( "ALBINO" );
static const trait_id trait_SUNBURN( "SUNBURN" );

// Tests for Character suffering
//
// Covers functions:
// - Character::suffer_from_sunburn

// Make character suffer for a while
static void test_suffer( Character &dummy, const time_duration &dur, bool update_body = false )
{
    const int num_turns = to_turns<int>( dur );
    for( int turn = 0; turn < num_turns; ++turn ) {
        dummy.suffer();
        if( update_body ) {
            dummy.update_body( calendar::turn, calendar::turn + 1_turns );
        }
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
TEST_CASE( "suffering_from_albinism", "[char][suffer][albino]" )
{
    clear_map();
    avatar &dummy = get_avatar();
    clear_character( dummy );
    g->reset_light_level();

    int focus_lost = 0;
    // TODO: The random chance of pain is too unprectable to test reliably.
    // As a result any test with small non-zero values has been disabled.
    // The values should still be correct

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

        dummy.toggle_trait( trait_ALBINO );
        REQUIRE( dummy.has_trait( trait_ALBINO ) );

        WHEN( "totally naked and exposed" ) {
            dummy.clear_worn();

            // 60 times * 12 bodyparts * 0.25 chance for medium effect
            THEN( "they lose 80 to 280 focus per hour" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( focus_lost == Approx( 180 ).margin( 100 ) );
            }

            // THEN( "they suffer about 2 pain per hour" ) {
            // 1 pain per hour for unshaded eyes
            // 1 pain per hour for exposed skin
            // Without running a long test, chance of pain is too low to measure effectively
            // This assertion will pass when pain is between 0 and 12 in an hour
            //pain_felt = test_suffer_pain_felt( dummy, 1_hours );
            //CHECK( pain_felt == Approx( 2 ).margin( 10 ) );
            // }
        }

        WHEN( "wielding an umbrella and wearing sunglasses" ) {
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );

            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            THEN( "they suffer no pain or loss of focus" ) {
                focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                CHECK( dummy.get_pain() == 0 );
                CHECK( focus_lost == 0 );
            }
        }

        WHEN( "entire body is covered with clothing" ) {
            dummy.clear_worn();
            dummy.wear_item( zentai, false );

            // WHEN( "not wearing sunglasses" ) {
            //     REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            // 60 times * 1 bodyparts * 0.1 chance for severe effect
            // THEN( "they suffer about 6 pain per hour" ) {
            //     test_suffer( dummy, 3_hours );
            //     CHECK( dummy.get_pain() == Approx( 18 ).margin( 17 ) );
            // }
            // 60 times * 1 bodyparts * 0.25 chance for medium effect
            // THEN( "they lose about 15 focus per hour" ) {
            //     focus_lost = test_suffer_focus_lost( dummy, 1_hours );
            //     CHECK( focus_lost == Approx( 15 ).margin( 14 ) );
            // }
            // }

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

// Suffering from Solar Sensitivity (SUNBURN trait)
//
// - Solar Sens. suffering effects occur about 3 times per minute (1/20 chance per turn)
// - If character is wielding an umbrella and wearing sunglasses, all effects are negated
// - If character has body parts more than 1% exposed, they suffer pain and loss of HP
// - Chance of pain and HP loss is directly proportional to skin exposure on each body part
// -
//
TEST_CASE( "suffering_from_sunburn", "[char][suffer][sunburn]" )
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

        dummy.toggle_trait( trait_SUNBURN );
        REQUIRE( dummy.has_trait( trait_SUNBURN ) );

        std::map<bodypart_id, int> bp_hp_lost;
        WHEN( "totally naked and exposed, with sunglasses" ) {
            dummy.clear_worn();
            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            // THEN( "they suffer injuries on every body part" ) {
            // Should lose an average of 1 HP per minute from each body part with hit points
            // (head, torso, both arms, both legs)
            // 60 * 0.1 * 2 (as two body parts contribute to each part with HP, e.g. l. arm + l. hand both damage l. arm)
            // bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 1_hours );
            // for( const bodypart_id &bp : body_parts_with_hp ) {
            //     CAPTURE( bp.id().str() );
            //     CHECK( bp_hp_lost[bp] == Approx( 12 ).margin( 11 ) );
            // }
            // }

            THEN( "they suffer pain several times a minute" ) {
                // 60 * 0.25 * 11 (num body parts, excluding eyes)
                pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                CHECK( pain_felt == Approx( 15 * 11 ).margin( 14 * 11 ) );
            }
        }

        WHEN( "naked and wielding an umbrella, with sunglasses" ) {
            dummy.clear_worn();
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );
            dummy.wear_item( shades, false );
            REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

            // Umbrella completely shields the skin from exposure when wielded
            THEN( "they suffer no injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 1_hours );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    CHECK( bp_hp_lost[bp] == 0 );
                }
            }
            THEN( "they suffer no pain" ) {
                pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                CHECK( pain_felt == 0 );
            }
        }

        WHEN( "wielding an umbrella, without sunglasses" ) {
            dummy.clear_worn();
            dummy.wield( umbrella );
            REQUIRE( dummy.get_wielded_item()->has_flag( flag_RAIN_PROTECT ) );
            REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );
            THEN( "they suffer only head injury" ) {
                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 1_hours );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    // if( bp == bodypart_id( "head" ) ) {
                    // 60 * 0.1
                    // CHECK( bp_hp_lost[bp] == Approx( 6 ).margin( 4 ) );
                    // } else
                    if( bp != bodypart_id( "head" ) ) {
                        CHECK( bp_hp_lost[bp] == 0 );
                    }
                }
            }
            // THEN( "they suffer pain" ) {
            // 60 * 0.25
            // pain_felt = test_suffer_pain_felt( dummy, 1_hours );
            // CHECK( pain_felt == Approx( 15 ).margin( 14 ) );
            // }
        }

        WHEN( "torso and arms are 90% covered" ) {
            dummy.clear_worn();
            dummy.wear_item( longshirt, false );

            THEN( "damage to torso is 0 and halved for arms" ) {
                time_duration t = 1_hours;

                bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, t );
                for( const bodypart_id &bp : body_parts_with_hp ) {
                    CAPTURE( bp.id().str() );
                    if( bp.id().str() == "torso" ) {
                        CHECK( bp_hp_lost[bp] == 0 );
                    }
                    // else if( bp.id().str() == "arm_l" || bp.id().str() == "arm_r" ) {
                    // Hands are exposed
                    // 120 * 0.1 * 1
                    // CHECK( bp_hp_lost[bp] == Approx( 12 ).margin( 11 ) );
                    // } else {
                    // legs+feet combine, and head+mouth combine (doubled damage)
                    // 120 * 0.1 * 2
                    // CHECK( bp_hp_lost[bp] == Approx( 24 ).margin( 23 ) );
                    // }
                }
            }
        }

        WHEN( "entire body is covered" ) {
            dummy.clear_worn();
            dummy.wear_item( zentai, false );

            WHEN( "not wearing sunglasses" ) {
                REQUIRE_FALSE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer loss of focus" ) {
                    // Heavy and medium effects take priority.
                    // Although the chance for focus loss is written as 1.0 it is in reality 0.65 at 100% exposure
                    // 0.65 = 1.0 - 0.1 - 0.25
                    // 39 = 0.65 * 1 * 60
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( focus_lost == Approx( 39 ).margin( 30 ) );
                }
                // THEN( "they suffer pain" ) {
                // 60 * 0.25
                // from exposed eyes as they count as a fully exposed body part
                // pain_felt = test_suffer_pain_felt( dummy, 1_hours );
                // CHECK( pain_felt == Approx( 15 ).margin( 14 ) );
                // }
            }

            WHEN( "wearing sunglasses" ) {
                dummy.wear_item( shades, false );
                REQUIRE( dummy.worn_with_flag( flag_SUN_GLASSES ) );

                THEN( "they suffer no injury" ) {
                    bp_hp_lost = test_suffer_bodypart_hp_lost( dummy, 10_minutes );
                    for( const bodypart_id &bp : body_parts_with_hp ) {
                        CAPTURE( bp.id().str() );
                        CHECK( bp_hp_lost[bp] == 0 );
                    }
                }

                THEN( "they suffer no pain or loss of focus" ) {
                    focus_lost = test_suffer_focus_lost( dummy, 1_hours );
                    CHECK( dummy.get_pain() == 0 );
                    CHECK( focus_lost == 0 );
                }
            }
        }
    }
}

TEST_CASE( "suffering_from_asphyxiation", "[char][suffer][oxygen][grab]" )
{
    clear_map();
    clear_avatar();
    Character &dummy = get_player_character();
    dummy.set_underwater( false );
    dummy.oxygen = dummy.get_oxygen_max();
    REQUIRE( dummy.oxygen == 46 );

    GIVEN( "character is not grabbed or underwater" ) {
        REQUIRE( !dummy.is_underwater() );
        REQUIRE( !dummy.has_effect( effect_grabbed, body_part_torso ) );
        dummy.oxygen = 0;

        WHEN( "at full stamina" ) {
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

            THEN( "recover 5 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 15 );
            }
        }

        WHEN( "at 3/4 stamina" ) {
            dummy.set_stamina( ( dummy.get_stamina_max() * 3 ) / 4 );
            REQUIRE( dummy.get_stamina() == ( dummy.get_stamina_max() * 3 ) / 4 );

            THEN( "recover 3 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 9 );
            }
        }

        WHEN( "at 1/2 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 2 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 2 );

            THEN( "recover 2 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 6 );
            }
        }

        WHEN( "at 1/4 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 4 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 4 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }

        WHEN( "at 1/10 stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 10 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 10 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }

        WHEN( "at 0 stamina" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            THEN( "recover 1 stamina per turn" ) {
                test_suffer( dummy, 3_turns, true );
                CHECK( dummy.oxygen == 3 );
            }
        }
    }

    GIVEN( "character is underwater" ) {
        dummy.set_underwater( true );
        REQUIRE( dummy.is_underwater() );
        REQUIRE( dummy.oxygen == 46 );
        REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

        THEN( "they lose 1 oxygen per turn" ) {
            test_suffer( dummy, 10_turns, true );
            CHECK( dummy.oxygen == 36 );
        }
    }

    GIVEN( "character is grabbed" ) {
        REQUIRE( dummy.oxygen == 46 );
        REQUIRE( !dummy.is_underwater() );
        REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );
        // Always spawn the first two grabbers, no need for intensity checks
        spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_east );
        spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_west );
        dummy.add_effect( effect_grabbed, 20_turns, body_part_torso, false, 2, true );
        REQUIRE( dummy.has_effect( effect_grabbed, body_part_torso ) );
        WHEN( "two grabbers" ) {

            THEN( "they lose 0 or 1 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == Approx( 41 ).margin( 5 ) );
            }
        }

        WHEN( "four grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south );
            THEN( "they lose 1 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 36 );
            }
        }

        WHEN( "six grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south_west );
            THEN( "they lose 1 or 2 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == Approx( 31 ).margin( 5 ) );
            }
        }

        WHEN( "eight grabbers" ) {
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south_west );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_north_east );
            spawn_test_monster( "mon_debug_memory", dummy.pos() + tripoint_south_east );
            THEN( "they lose 2 oxygen per turn" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 26 );
            }
        }

        map &here = get_map();
        WHEN( "crushed against two walls by two grabbers" ) {
            here.ter_set( dummy.pos() + tripoint_south, t_rock_wall );
            here.ter_set( dummy.pos() + tripoint_north, t_rock_wall );
            REQUIRE( here.impassable( dummy.pos() + tripoint_south ) );
            REQUIRE( here.impassable( dummy.pos() + tripoint_north ) );

            THEN( "they lose 1 oxygen per turn, just like four grabbers" ) {
                test_suffer( dummy, 10_turns, true );
                CHECK( dummy.oxygen == 36 );
            }
        }
    }
}
