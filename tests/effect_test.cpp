#include "effect.h"

#include "catch/catch.hpp"

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
    const body_part bp = bodypart_id( bp_name )->token;

    effect effect_obj( &type, dur, bp, permanent, intensity, start_time );

    CHECK( dur == effect_obj.get_duration() );
    CHECK( bp == effect_obj.get_bp() );
    CHECK( permanent == effect_obj.is_permanent() );
    CHECK( intensity == effect_obj.get_intensity() );
    CHECK( start_time == effect_obj.get_start_time() );
}

TEST_CASE( "effect initialization test", "[effect][init]" )
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
TEST_CASE( "effect duration", "[effect][duration]" )
{
    // "debugged" and "intensified" effects come from JSON effect data (data/mods/TEST_DATA/effects.json)
    const efftype_id eff_id( "debugged" );
    effect eff_debugged( &eff_id.obj(), 1_turns, num_bp, false, 1, calendar::turn );

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
    // "max_intensity": 4,
    // "apply_message": "You feel lightheaded.",
    // "int_dur_factor": 1000,
    //
    // It has "int_dur_factor": 1000, meaning that its intensity will always be equal to its duration /
    // 1000 rounded up, and it has "max_intensity": 4 meaning the highest its intensity will go is 4 at
    // a duration of 3000 or higher.
    SECTION( "set_duration modifies intensity if effect is duration-based" ) {
        const efftype_id eff_id( "intensified" );
        effect eff_intense( &eff_id.obj(), 1_turns, num_bp, false, 1, calendar::turn );
        REQUIRE( eff_intense.get_int_dur_factor() == 1_minutes );

        // At zero duration, intensity is minimum (1)
        eff_intense.set_duration( 0_seconds );
        CHECK( eff_intense.get_intensity() == 1 );

        // At duration == int_dur_factor, intensity is 2
        eff_intense.set_duration( 1_minutes );
        CHECK( eff_intense.get_intensity() == 2 );

        // At duration == 2 * int_dur_factor, intensity is 3
        eff_intense.set_duration( 2_minutes );
        CHECK( eff_intense.get_intensity() == 3 );

        // At duration == 3 * int_dur_factor, intensity is still 3
        eff_intense.set_duration( 3_minutes );
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
TEST_CASE( "effect intensity", "[effect][intensity]" )
{
    const efftype_id eff_id( "debugged" );
    effect eff_debugged( &eff_id.obj(), 3_turns, num_bp, false, 1, calendar::turn );

    REQUIRE( eff_debugged.get_intensity() == 1 );
    REQUIRE( eff_debugged.get_max_intensity() == 10 );

    SECTION( "intensity cannot be set less than 1" ) {
        eff_debugged.set_intensity( 0 );
        CHECK( eff_debugged.get_intensity() == 1 );
        eff_debugged.set_intensity( -1 );
        CHECK( eff_debugged.get_intensity() == 1 );
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

    // From EFFECTS_JSON.md:
    // max_intensity: Used for many later fields, defaults to 1
    // max_effective_intensity: How many intensity levels will apply effects. Other intensity levels
    //   will only increase duration.
    //
    // If "max_intensity" > 1 and the number of entries in "name" >= "max_intensity" then it will
    // attempt to use the proper intensity name.
}

// Effect decay
// ------------
// effect::decay
//
TEST_CASE( "effect decay", "[effect][decay]" )
{
    const efftype_id eff_id( "debugged" );

    std::vector<efftype_id> rem_ids;
    std::vector<body_part> rem_bps;

    SECTION( "decay reduces effect duration by 1 turn" ) {
        effect eff_debugged( &eff_id.obj(), 2_turns, num_bp, false, 1, calendar::turn );
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
        CHECK( rem_bps.front() == num_bp );
    }

    SECTION( "decay does not reduce paused/permanent effect duration" ) {
        effect eff_debugged( &eff_id.obj(), 2_turns, num_bp, true, 1, calendar::turn );
        // Ensure it will last 2 turns, and is permanent/paused
        REQUIRE( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        REQUIRE( eff_debugged.is_permanent() );

        // Decay twice - should have no effect on duration
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
        eff_debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( eff_debugged.get_duration() ) == 2 );
    }

    // TODO:
    // When intensity > 1
    // - and duration < max_duration
    // - and int_decay_tick is set (from effect JSON)
    // - and time % decay_tick == 0
    // Then:
    // - add int_decay_step to intensity (default -1)
}

// Effect description
// ------------------
// effect::disp_short_desc
//
TEST_CASE( "display short description", "[effect][desc]" )
{
    const efftype_id eff_id( "debugged" );
    const body_part arm_r = bodypart_id( "arm_r" )->token;
    effect eff_debugged( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );

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
TEST_CASE( "effect display and speed name may vary with intensity",
           "[effect][intensity][disp_name][speed_name]" )
{
    GIVEN( "effect with names for each intensity level" ) {
        // "intensified" effect (data/mods/TEST_DATA/effects.json) has 3 names:
        // "name": [ "Whoa", "Wut?", "Wow!" ]
        // "max_intensity": 3
        const efftype_id eid_intensified( "intensified" );
        effect eff_intense( &eid_intensified.obj(), 1_turns, num_bp, false, 1, calendar::turn );
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
        const efftype_id eid_debugged( "debugged" );
        effect eff_debugged( &eid_debugged.obj(), 1_minutes, num_bp, false, 1, calendar::turn );

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
TEST_CASE( "effect permanence", "[effect][permanent]" )
{
    const efftype_id eff_id( "grabbed" );
    const body_part arm_r = bodypart_id( "arm_r" )->token;

    // Grab right arm, not permanent
    effect eff_grabbed( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );
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
TEST_CASE( "effect body part", "[effect][bodypart]" )
{
    const efftype_id eff_id( "grabbed" );
    const body_part arm_r = bodypart_id( "arm_r" )->token;
    const body_part arm_l = bodypart_id( "arm_l" )->token;

    // Grab right arm, initially
    effect eff_grabbed( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );
    CHECK( eff_grabbed.get_bp() == arm_r );
    // Switch to left arm
    eff_grabbed.set_bp( arm_l );
    CHECK( eff_grabbed.get_bp() == arm_l );
    CHECK_FALSE( eff_grabbed.get_bp() == arm_r );
    // Back to right arm
    eff_grabbed.set_bp( arm_r );
    CHECK( eff_grabbed.get_bp() == arm_r );
    CHECK_FALSE( eff_grabbed.get_bp() == arm_l );
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
TEST_CASE( "effect modifiers", "[effect][modifier]" )
{
    SECTION( "base_mods apply equally for any intensity" ) {
        const efftype_id eff_id( "debugged" );
        effect eff_debugged( &eff_id.obj(), 1_minutes, num_bp, false, 1, calendar::turn );

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
        const efftype_id eff_id( "intensified" );
        effect eff_intense( &eff_id.obj(), 1_turns, num_bp, false, 1, calendar::turn );
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

