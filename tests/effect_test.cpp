#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_id.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "damage.h"
#include "effect.h"
#include "effect_source.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"

class Creature;

static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_debugged( "debugged" );
static const efftype_id effect_grabbed( "grabbed" );
static const efftype_id effect_intensified( "intensified" );
static const efftype_id effect_max_effected( "max_effected" );
static const efftype_id effect_test_fatalism( "test_fatalism" );
static const efftype_id effect_test_int_remove( "test_int_remove" );
static const efftype_id effect_test_vitamineff( "test_vitamineff" );

static const itype_id itype_test_vitfood( "test_vitfood" );

static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const vitamin_id vitamin_test_vitv( "test_vitv" );
static const vitamin_id vitamin_test_vitx( "test_vitx" );

// Test `effect` class

// Effect initialization
// ---------------------
// Simple initialization sanity check, tests the constructor and a few accessors:
//
// effect::get_duration
// effect::get_bp
// effect::is_permanent
// effect::get_intensity
// effect::get_start_time
//
// Create an `effect` object with given parameters, and check they were initialized correctly.
static void check_effect_init( const std::string &eff_name, const time_duration &dur,
                               const std::string &bp_name, const bool permanent, const int intensity,
                               const time_point &start_time )
{
    const efftype_id eff_id( eff_name );
    const effect_type &type = eff_id.obj();
    const bodypart_str_id bp( bp_name );

    effect effect_obj( effect_source::empty(), &type, dur, bp, permanent, intensity, start_time );

    CHECK( dur == effect_obj.get_duration() );
    CHECK( bp.id() == effect_obj.get_bp() );
    CHECK( permanent == effect_obj.is_permanent() );
    CHECK( intensity == effect_obj.get_intensity() );
    CHECK( start_time == effect_obj.get_start_time() );
}

TEST_CASE( "effect_initialization_test", "[effect][init]" )
{
    // "debugged" effect is defined in data/mods/TEST_DATA/effects.json
    check_effect_init( "debugged", 1_days, "head", false, 5, calendar::turn_zero );
    check_effect_init( "bite", 1_minutes, "torso", true, 2, calendar::turn );
    check_effect_init( "grabbed", 1_turns, "arm_r", false, 1, calendar::turn + 1_hours );
}

// Effect duration
// ---------------
// effect::get_max_duration
// effect::get_duration
// effect::set_duration
// TODO:
// effect::get_start_time
// effect::mod_duration
// effect::mult_duration
//
TEST_CASE( "effect_duration", "[effect][duration]" )
{
    // "debugged" and "intensified" effects come from JSON effect data (data/mods/TEST_DATA/effects.json)
    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_turns,
                         bodypart_str_id::NULL_ID(),
                         false, 1, calendar::turn );

    // Current duration from effect initialization
    REQUIRE( eff_debugged.get_duration() == 1_turns );
    REQUIRE( eff_debugged.get_max_duration() == 1_hours );

    SECTION( "duration can be set to a negative value" ) {
        eff_debugged.set_duration( -1_turns );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == -1 );
    }

    SECTION( "set_duration is capped at maximum duration" ) {
        eff_debugged.set_duration( 2_hours );
        CHECK( eff_debugged.get_duration() == 1_hours );
    }

    // Example Effect (from EFFECTS_JSON.md):
    //
    // "id": "drunk",
    // "name": [ "Tipsy", "Drunk", "Trashed", "Wasted" ],
    // "max_intensity": 3,
    // "apply_message": "You feel lightheaded.",
    // "int_dur_factor": 1000,
    //
    // It has "int_dur_factor": 1000, meaning that its intensity will always be equal to its duration /
    // 1000 rounded up, and it has "max_intensity": 3 meaning the highest its intensity will go is 3 at
    // a duration of 3000 or higher.
    SECTION( "set_duration modifies intensity if effect is duration-based" ) {
        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(),
                            false, 1, calendar::turn );
        REQUIRE( eff_intense.get_int_dur_factor() == 1_minutes );

        // At zero duration, intensity is minimum (1)
        eff_intense.set_duration( 0_seconds );
        CHECK( eff_intense.get_intensity() == 1 );

        // At duration == int_dur_factor, intensity is 1
        eff_intense.set_duration( 1_minutes );
        CHECK( eff_intense.get_intensity() == 1 );

        // At duration == 2 * int_dur_factor, intensity is 2
        eff_intense.set_duration( 2_minutes );
        CHECK( eff_intense.get_intensity() == 2 );

        // At duration == (2m1s) * int_dur_factor, intensity is 3 (rounds up)
        eff_intense.set_duration( 2_minutes + 1_seconds );
        CHECK( eff_intense.get_intensity() == 3 );

        // At duration == 3 * int_dur_factor, intensity is still 3
        eff_intense.set_duration( 3_minutes );
        CHECK( eff_intense.get_intensity() == 3 );

        // At duration == 4 * int_dur_factor, intensity is still 3
        eff_intense.set_duration( 4_minutes );
        CHECK( eff_intense.get_intensity() == 3 );
    }
}

// Effect intensity
// ----------------
// effect::get_intensity
// effect::get_max_intensity
// effect::set_intensity
// effect::mod_intensity
//
// TODO:
// effect::get_dur_add_perc
// effect::get_int_dur_factor
// effect::get_int_add_val
//
TEST_CASE( "effect_intensity", "[effect][intensity]" )
{
    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 3_turns,
                         bodypart_str_id::NULL_ID(),
                         false, 1, calendar::turn );

    REQUIRE( eff_debugged.get_intensity() == 1 );
    REQUIRE( eff_debugged.get_max_intensity() == 10 );

    SECTION( "intensity cannot be set less than 0" ) {
        eff_debugged.set_intensity( 0 );
        CHECK( eff_debugged.get_intensity() == 0 );
        eff_debugged.set_intensity( -1 );
        CHECK( eff_debugged.get_intensity() == 0 );
    }

    SECTION( "intensity cannot be set greater than maximum" ) {
        // These don't go to 11
        eff_debugged.set_intensity( 11 );
        CHECK( eff_debugged.get_intensity() == 10 );
        // or 9+2
        eff_debugged.set_intensity( 9 );
        eff_debugged.mod_intensity( 2 );
        CHECK( eff_debugged.get_intensity() == 10 );
    }
}

TEST_CASE( "effect_intensity_removal", "[effect][intensity]" )
{
    effect eff_test_int_remove( effect_source::empty(), &effect_test_int_remove.obj(), 3_turns,
                                bodypart_str_id::NULL_ID(),
                                false, 1, calendar::turn );

    REQUIRE( eff_test_int_remove.get_intensity() == 1 );
    REQUIRE( eff_test_int_remove.get_int_decay_remove() == false );

    SECTION( "effects protected from intensity-based removal can't be set to less than 1 intensity" ) {
        eff_test_int_remove.set_intensity( 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
    }

}

TEST_CASE( "max_effective_intensity", "[effect][max][intensity]" )
{
    effect eff_maxed( effect_source::empty(), &effect_max_effected.obj(), 3_turns,
                      bodypart_str_id::NULL_ID(),
                      false, 1, calendar::turn );

    REQUIRE( eff_maxed.get_intensity() == 1 );
    REQUIRE( eff_maxed.get_max_effective_intensity() == 6 );
    REQUIRE( eff_maxed.get_max_intensity() == 10 );

    SECTION( "scaling_mods apply up to max_effective_intensity" ) {
        // No scaling_mod effects at baseline intensity (1)
        eff_maxed.set_intensity( 1 );
        CHECK( eff_maxed.get_mod( "INT" ) == 0 );
        // Adds 1 INT per intensity greater than 1, up to max_effective_intensity
        eff_maxed.set_intensity( 2 );
        CHECK( eff_maxed.get_mod( "INT" ) == 1 );
        eff_maxed.set_intensity( 3 );
        CHECK( eff_maxed.get_mod( "INT" ) == 2 );
        eff_maxed.set_intensity( 4 );
        CHECK( eff_maxed.get_mod( "INT" ) == 3 );
        eff_maxed.set_intensity( 5 );
        CHECK( eff_maxed.get_mod( "INT" ) == 4 );
        eff_maxed.set_intensity( 6 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        // The scaling_mods should stop applying above max_effective_intensity (6)
        eff_maxed.set_intensity( 7 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 8 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 9 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
        eff_maxed.set_intensity( 10 );
        CHECK( eff_maxed.get_mod( "INT" ) == 5 );
    }
}

// Effect decay
// ------------
// effect::decay
//
TEST_CASE( "effect_decay", "[effect][decay]" )
{
    std::vector<efftype_id> rem_ids;
    std::vector<bodypart_id> rem_bps;

    SECTION( "decay reduces effect duration by 1 turn and triggers intensity decay" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 2_turns,
                             bodypart_str_id::NULL_ID(),
                             false, 5, calendar::turn );
        // Ensure it will last 2 turns, and is not permanent/paused
        REQUIRE( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        REQUIRE_FALSE( eff_debugged.is_permanent() );

        // First decay - 1 turn left
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 1 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Second decay - 0 turns left
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 0 );
        // Effect still not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Third decay - 0 turns left
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 0 );
        // Effect is removed
        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );
        // Effect ID and body part are pushed to the arrays
        CHECK( rem_ids.front() == eff_debugged.get_id() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );
    }

    SECTION( "decay does not reduce paused/permanent effect duration" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 2_turns,
                             bodypart_str_id::NULL_ID(),
                             true, 1, calendar::turn );
        // Ensure it will last 2 turns, and is permanent/paused
        REQUIRE( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        REQUIRE( eff_debugged.is_permanent() );

        // Decay twice - should have no effect on duration
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
    }

    SECTION( "intensity decay triggers on the appropriate turns" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_hours,
                             bodypart_str_id::NULL_ID(),
                             false, 10, calendar::turn );
        // Ensure it has a decay tick  of 2 turns and a decay step of -1, and int removal is allwed
        // Also check max duration
        REQUIRE( eff_debugged.get_max_duration() == 1_hours );
        REQUIRE( eff_debugged.get_intensity() == 10 );
        REQUIRE( eff_debugged.get_int_decay_step() == -1 );
        REQUIRE( eff_debugged.get_int_decay_tick() == 2 );
        REQUIRE( eff_debugged.get_int_decay_remove() == true );
        // Reset time
        calendar::turn = calendar::start_of_cataclysm;

        // First decay - at max duration, no intensity decay
        CHECK( eff_debugged.get_duration() == eff_debugged.get_max_duration() );
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 10 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Progress turns, checking for the appropriate intensity and no removal

        // Turn 1, no intensity change
        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 10 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Turn 2, intensity decays
        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 9 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Turn 3, no intensity change
        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 9 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Skip the intermediate levels, set intensity to 1 manually before proceeding
        eff_debugged.set_intensity( 1 );
        calendar::turn += 1_turns;
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( eff_debugged.get_intensity() == 0 );
        // Effect is removed
        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );
        // Effect ID and body part are pushed to the arrays
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_ids.front() == eff_debugged.get_id() );
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );

    }

    SECTION( "int_decay_remove == false protects an effect from removal" ) {
        effect eff_test_int_remove( effect_source::empty(), &effect_test_int_remove.obj(), 3_turns,
                                    bodypart_str_id::NULL_ID(),
                                    false, 3, calendar::turn );
        // Ensure it has the -2 int decay step, is protected from removal and has a decay tick of 1
        REQUIRE( eff_test_int_remove.get_int_decay_step() == -2 );
        REQUIRE( eff_test_int_remove.get_int_decay_tick() == 1 );
        REQUIRE( eff_test_int_remove.get_int_decay_remove() == false );
        // Ensure it will last 3 turns, and is not permanent/paused
        REQUIRE( to_turns<int>( eff_test_int_remove.get_duration() ) == 3 );
        REQUIRE_FALSE( eff_test_int_remove.is_permanent() );

        // First decay - 2 turns left, intensity decay procs
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 2 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Second decay - 1 turns left, intensity stays 1 (but wouldn't proc anyway)
        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 1 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Third decay - 0 turn left, intensity decay procs but intensity is capped to 1
        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Fourth decay - 0 turns left, intensity stays 1 (but wouldn't proc anyway)
        calendar::turn += 1_turns;
        eff_test_int_remove.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_test_int_remove.get_duration() ) == 0 );
        CHECK( eff_test_int_remove.get_intensity() == 1 );
        // Effect is removed
        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );
        // Effect ID and body part are pushed to the arrays
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_ids.front() == eff_test_int_remove.get_id() );
        REQUIRE( !rem_ids.empty() );
        CHECK( rem_bps.front() == bodypart_id( "bp_null" ) );

    }
}

// Effect description
// ------------------
// effect::disp_short_desc
//
TEST_CASE( "display_short_description", "[effect][desc]" )
{
    effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_turns, body_part_arm_r,
                         false, 1,
                         calendar::turn );

    // TODO: Determine a case where `reduced` (true/false) makes a difference

    CHECK( eff_debugged.disp_short_desc( false ) == "You have been debugged!\n"
           "Everything is working perfectly now." );

    CHECK( eff_debugged.disp_short_desc( true ) == "You have been debugged!\n"
           "Everything is working perfectly now." );
}

// effect::disp_name
// effect::get_speed_name
//
// From EFFECTS_JSON.md:
//
// "name":
// """If "max_intensity" > 1 and the number of entries in "name" >= "max_intensity" then it will
// attempt to use the proper intensity name."""
//
// "speed_name":
// """This is the value used in the list of modifiers on a player's speed. It will default to the
// first entry in "name" if it doesn't exist, and if neither one exists or if "speed_name" is the
// empty string (""), then it will not appear in the list of modifiers on the players speed (though
// the effect might still have an effect)."""
//
TEST_CASE( "effect_display_and_speed_name_may_vary_with_intensity",
           "[effect][intensity][disp_name][speed_name]" )
{
    GIVEN( "effect with names for each intensity level" ) {
        // "intensified" effect (data/mods/TEST_DATA/effects.json) has 3 names:
        // "name": [ "Whoa", "Wut?", "Wow!" ]
        // "max_intensity": 3
        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(), false, 1, calendar::turn );
        REQUIRE( eff_intense.get_max_intensity() == 3 );

        // use_name_ints is true if there are names for each intensity
        const effect_type *etype_intense = eff_intense.get_effect_type();
        REQUIRE( etype_intense->use_name_ints() );

        THEN( "disp_name has the name for the current intensity" ) {
            eff_intense.set_intensity( 1 );
            CHECK( eff_intense.disp_name() == "Whoa" );
            eff_intense.set_intensity( 2 );
            CHECK( eff_intense.disp_name() == "Wut?" );
            eff_intense.set_intensity( 3 );
            CHECK( eff_intense.disp_name() == "Wow!" );
        }

        // For this effect, "speed_name" is not explicitly defined, so
        // get_speed_name is the same as disp_name for each intensity.
        THEN( "get_speed_name has the name for the current intensity" ) {
            eff_intense.set_intensity( 1 );
            CHECK( eff_intense.get_speed_name() == "Whoa" );
            eff_intense.set_intensity( 2 );
            CHECK( eff_intense.get_speed_name() == "Wut?" );
            eff_intense.set_intensity( 3 );
            CHECK( eff_intense.get_speed_name() == "Wow!" );
        }
    }

    GIVEN( "effect with a single name and explicit speed_name" ) {
        // "debugged" effect has only one name and a speed_name:
        // "name": [ "Debugged" ]
        // "speed_name": "Optimized"
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_minutes,
                             bodypart_str_id::NULL_ID(), false, 1, calendar::turn );

        THEN( "disp_name has the name, and current intensity if > 1" ) {
            eff_debugged.set_intensity( 1 );
            CHECK( eff_debugged.disp_name() == "Debugged" );
            eff_debugged.set_intensity( 2 );
            CHECK( eff_debugged.disp_name() == "Debugged [2]" );
            eff_debugged.set_intensity( 3 );
            CHECK( eff_debugged.disp_name() == "Debugged [3]" );
        }

        THEN( "get_speed_name is the speed name for all intensity levels" ) {
            eff_debugged.set_intensity( 1 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
            eff_debugged.set_intensity( 2 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
            eff_debugged.set_intensity( 3 );
            CHECK( eff_debugged.get_speed_name() == "Optimized" );
        }
    }
}

// Effect permanence
// -----------------
// effect::is_permanent
// effect::pause_effect
// effect::unpause_effect
//
TEST_CASE( "effect_permanence", "[effect][permanent]" )
{
    // Grab right arm, not permanent
    effect eff_grabbed( effect_source::empty(), &effect_grabbed.obj(), 1_turns, body_part_arm_r, false,
                        1,
                        calendar::turn );
    CHECK_FALSE( eff_grabbed.is_permanent() );
    // Pause makes it permanent
    eff_grabbed.pause_effect();
    CHECK( eff_grabbed.is_permanent() );
    // Pause again does nothing
    eff_grabbed.pause_effect();
    CHECK( eff_grabbed.is_permanent() );
    // Unpause removes permanence
    eff_grabbed.unpause_effect();
    CHECK_FALSE( eff_grabbed.is_permanent() );
    // Unpause again does nothing
    eff_grabbed.unpause_effect();
    CHECK_FALSE( eff_grabbed.is_permanent() );
}

// Effect body part
// ----------------
// effect::set_bp
// effect::get_bp
//
TEST_CASE( "effect_body_part", "[effect][bodypart]" )
{
    // Grab right arm, initially
    effect eff_grabbed( effect_source::empty(), &effect_grabbed.obj(), 1_turns, body_part_arm_r, false,
                        1,
                        calendar::turn );
    CHECK( eff_grabbed.get_bp() == body_part_arm_r.id() );
    // Switch to left arm
    eff_grabbed.set_bp( body_part_arm_l );
    CHECK( eff_grabbed.get_bp() == body_part_arm_l.id() );
    CHECK_FALSE( eff_grabbed.get_bp() == body_part_arm_r.id() );
    // Back to right arm
    eff_grabbed.set_bp( body_part_arm_r );
    CHECK( eff_grabbed.get_bp() == body_part_arm_r.id() );
    CHECK_FALSE( eff_grabbed.get_bp() == body_part_arm_l.id() );
}

// Effect modifiers
// ----------------
// TODO:
// effect::get_avg_mod
// effect::get_min_val
// effect::get_max_val
// effect::get_sizing
// effect::get_percentage
//
TEST_CASE( "effect_modifiers", "[effect][modifier]" )
{
    SECTION( "base_mods apply equally for any intensity" ) {
        effect eff_debugged( effect_source::empty(), &effect_debugged.obj(), 1_minutes,
                             bodypart_str_id::NULL_ID(),
                             false, 1, calendar::turn );

        CHECK( eff_debugged.get_mod( "STR" ) == 1 );
        CHECK( eff_debugged.get_mod( "DEX" ) == 2 );
        CHECK( eff_debugged.get_mod( "PER" ) == 3 );
        CHECK( eff_debugged.get_mod( "INT" ) == 4 );
        CHECK( eff_debugged.get_amount( "HEAL_RATE" ) == 2 );
        CHECK( eff_debugged.get_amount( "HEAL_HEAD" ) == 200 );
        CHECK( eff_debugged.get_amount( "HEAL_TORSO" ) == 150 );
    }

    // Scaling mods - vary based on intensity
    SECTION( "scaling_mods vary based on intensity" ) {
        effect eff_intense( effect_source::empty(), &effect_intensified.obj(), 1_turns,
                            bodypart_str_id::NULL_ID(),
                            false, 1, calendar::turn );
        REQUIRE( eff_intense.get_max_intensity() == 3 );

        // Subtracts 1 INT and 2 PER per intensity greater than 1
        eff_intense.set_intensity( 1 );
        CHECK( eff_intense.get_mod( "INT" ) == 0 );
        CHECK( eff_intense.get_mod( "PER" ) == 0 );
        eff_intense.set_intensity( 2 );
        CHECK( eff_intense.get_mod( "INT" ) == -1 );
        CHECK( eff_intense.get_mod( "PER" ) == -2 );
        eff_intense.set_intensity( 3 );
        CHECK( eff_intense.get_mod( "INT" ) == -2 );
        CHECK( eff_intense.get_mod( "PER" ) == -4 );
        // Max intensity is 3
        eff_intense.set_intensity( 4 );
        CHECK( eff_intense.get_mod( "INT" ) == -2 );
        CHECK( eff_intense.get_mod( "PER" ) == -4 );
    }
}

TEST_CASE( "bleed_effect_attribution", "[effect][bleed][monster]" )
{
    const auto spawn_npc = [&]( const point_bub_ms & p, const std::string & npc_class ) {
        const string_id<npc_template> test_guy( npc_class );
        const character_id model_id = get_map().place_npc( p, test_guy );
        g->load_npcs();

        npc *guy = g->find_npc( model_id );
        REQUIRE( guy != nullptr );
        CHECK( !guy->in_vehicle );
        return guy;
    };

    static const tripoint_bub_ms target_location{ 5, 0, 0 };
    clear_npcs();
    clear_vehicles();
    clear_map();
    clear_avatar();
    Character &player = get_player_character();
    const damage_instance cut_damage = damage_instance( damage_cut, 50, 50 );

    GIVEN( "player and monster" ) {
        clear_npcs();
        clear_creatures();
        monster &test_monster = spawn_test_monster( "mon_zombie", target_location );
        WHEN( "when player cuts monster" ) {
            REQUIRE( test_monster.get_hp() == test_monster.get_hp_max() );
            THEN( "bleed effect gets attributed to player" ) {
                test_monster.deal_damage( player.as_character(), bodypart_str_id::NULL_ID(), cut_damage );
                const effect &bleed = test_monster.get_effect( effect_bleed );
                CHECK( test_monster.get_hp() < test_monster.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == player.as_character() );
            }
        }
        WHEN( "when player cuts npc" ) {

            npc &test_npc = *spawn_npc( player.pos_bub().xy() + point::south_west, "thug" );
            test_npc.clear_worn(); // Ensure the victim doesn't generate with too much protection.
            REQUIRE( test_npc.get_hp() == test_npc.get_hp_max() );
            THEN( "bleed effect gets attributed to player" ) {
                test_npc.deal_damage( player.as_character(), body_part_torso, cut_damage );
                const effect &bleed = test_npc.get_effect( effect_bleed, body_part_torso );
                CHECK( test_npc.get_hp() < test_npc.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == player.as_character() );
            }
        }
    }
    GIVEN( "two npcs" ) {
        npc &npc_src = *spawn_npc( player.pos_bub().xy() + point::south, "bandit" );
        npc &npc_dst = *spawn_npc( player.pos_bub().xy() + point::south_east, "thug" );
        npc_dst.clear_worn(); // Ensure the victim doesn't generate with too much protection.
        WHEN( "when npc_src cuts npc_dst" ) {
            REQUIRE( npc_dst.get_hp() == npc_dst.get_hp_max() );
            THEN( "bleed effect gets attributed to npc_src" ) {
                const dealt_damage_instance dealt = npc_dst.deal_damage( npc_src.as_character(), body_part_torso,
                                                    cut_damage );
                REQUIRE( dealt.total_damage() > 0 );
                const effect &bleed = npc_dst.get_effect( effect_bleed, body_part_torso );
                CHECK( npc_dst.get_hp() < npc_dst.get_hp_max() );
                CHECK( !bleed.is_null() );
                const Creature *bleed_source = bleed.get_source().resolve_creature();
                CHECK( bleed_source == npc_src.as_character() );
            }
        }
    }
}

TEST_CASE( "Vitamin_Effects", "[effect][vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    subject.stomach.empty();
    subject.guts.empty();
    subject.clear_effects();

    // Our effect influencing vitamins, and the two vitamins it influences
    const efftype_id vits = effect_test_vitamineff;
    const vitamin_id vitv = vitamin_test_vitv;
    const vitamin_id vitx = vitamin_test_vitx;

    effect vitamin_effect( effect_source::empty(), &( *vits ), 5_hours, body_part_torso, false, 1,
                           calendar::turn );
    subject.add_effect( vitamin_effect );

    // A food rich in in vitamin x - we need 2 of them, for with/without the effect
    item food1( itype_test_vitfood );
    item food2( food1 );

    // Make sure they have none of these vitamins at the start
    // Except vitv, because the vitamin tick applies immediately when the effect is added
    subject.vitamin_set( vitv, 0 );
    REQUIRE( subject.vitamin_get( vitv ) == 0 );
    REQUIRE( subject.vitamin_get( vitx ) == 0 );

    // Track 5 hours under the effect, and register the affected vitamins after
    subject.consume( food1 );
    for( time_duration turn = 0_turns; turn < 5_hours; turn += 1_turns ) {
        calendar::turn += 1_turns;
        subject.update_body();
        subject.process_effects();
    }
    const int posteffect_vitv = subject.vitamin_get( vitv );
    const int posteffect_vitx = subject.vitamin_get( vitx );

    // Clear the avatar, and try 5 hours without the effect, under the same conditions
    clear_avatar();
    REQUIRE( subject.vitamin_get( vitv ) == 0 );
    REQUIRE( subject.vitamin_get( vitx ) == 0 );
    subject.consume( food2 );
    for( time_duration turn = 0_turns; turn < 5_hours; turn += 1_turns ) {
        calendar::turn += 1_turns;
        subject.update_body();
        subject.process_effects();
    }
    const int post_vitv = subject.vitamin_get( vitv );
    const int post_vitx = subject.vitamin_get( vitx );

    // The effect roughly halves the absorbed vitamin x
    CHECK( posteffect_vitx == Approx( 22 ).margin( 3 ) );
    CHECK( post_vitx == 46 );

    // Without the effect, no vitamin v is gained
    CHECK( posteffect_vitv == 120 );
    CHECK( post_vitv == 0 );
}

static void test_deadliness( const effect &applied, const int expected_dead, const int margin )
{
    creature_tracker &creatures = get_creature_tracker();
    clear_map();
    std::vector<monster *> mons;

    // Place a hundred debug monsters, our subjects
    for( int i = 0; i < 10; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            tripoint_bub_ms cursor( i + 20, j + 20, 0 );

            mons.push_back( g->place_critter_at( pseudo_debug_mon, cursor ) );
            // make sure they're there!
            CHECK( creatures.creature_at<Creature>( cursor ) != nullptr );
        }
    }

    for( monster *const mon : mons ) {
        mon->add_effect( applied );
    }

    // Let them suffer under it for 5 turns.
    for( int i = 0; i < 5; ++i ) {
        calendar::turn += 1_turns;
        for( monster *const mon : mons ) {
            mon->process_effects();
        }
    }

    // See how many remain
    int alive = 0;
    for( int i = 0; i < 10; ++i ) {
        for( int j = 0; j < 10; ++j ) {
            tripoint_bub_ms cursor( i + 20, j + 20, 0 );

            alive += creatures.creature_at<Creature>( cursor ) != nullptr;
        }
    }

    const int dead = 100 - alive;
    CHECK( dead == Approx( expected_dead ).margin( margin ) );
}

TEST_CASE( "Death_Effects", "[effect][death]" )
{
    effect placebo_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds,
                           body_part_torso, false, 1,
                           calendar::turn );
    effect deadly_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds,
                          body_part_torso, false, 2,
                          calendar::turn );
    effect fatal_effect( effect_source::empty(), &( *effect_test_fatalism ), 5_seconds, body_part_torso,
                         false, 3,
                         calendar::turn );

    test_deadliness( placebo_effect, 0, 0 );
    // Need a pretty big margin here, it's fairly random
    // Just make sure that not everone lives and not everyone dies
    test_deadliness( deadly_effect, 50, 25 );
    test_deadliness( fatal_effect, 100, 0 );
}
