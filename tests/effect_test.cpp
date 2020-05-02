#include "effect.h"

#include "catch/catch.hpp"

// Test `effect` class

// Create an `effect` object with given parameters, and check they were initialized correctly.
static void check_effect_init( const std::string eff_name, const time_duration dur,
                               const std::string bp_name, const bool permanent, const int intensity,
                               const time_point start_time )
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
    check_effect_init( "grabbed", 1_turns, "arm_r", true, 1, calendar::turn );
    check_effect_init( "bite", 1_minutes, "torso", false, 2, calendar::turn );
}

// Effect duration
// ---------------
// effect::get_start_time
// effect::get_max_duration
// effect::get_duration
// effect::set_duration
// effect::mod_duration
// effect::mult_duration
//
TEST_CASE( "effect duration", "[effect][duration]" )
{
    const efftype_id eff_id( "debugged" );
    effect effect_obj( &eff_id.obj(), 1_minutes, num_bp, false, 1, calendar::turn );

    // max_duration comes from JSON effect data (data/mods/TEST_DATA/effects.json)
    REQUIRE( effect_obj.get_duration() == 1_minutes );
    REQUIRE( effect_obj.get_max_duration() == 1_hours );

    SECTION( "duration can be set to a negative value" ) {
        effect_obj.set_duration( -1_turns );
        CHECK( to_turns<int>( effect_obj.get_duration() ) == -1 );
    }

    SECTION( "set_duration is capped at maximum duration" ) {
        effect_obj.set_duration( 2_hours );
        CHECK( effect_obj.get_duration() == 1_hours );
    }

    // set_duration
    // - forces intensity if it is duration based (int_dur_factor)
    // - (lowest intensity is 1 not 0)
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
    effect effect_obj( &eff_id.obj(), 3_turns, num_bp, false, 1, calendar::turn );

    REQUIRE( effect_obj.get_intensity() == 1 );
    REQUIRE( effect_obj.get_max_intensity() == 10 );

    SECTION( "intensity cannot be set less than 1" ) {
        effect_obj.set_intensity( 0 );
        CHECK( effect_obj.get_intensity() == 1 );
        effect_obj.set_intensity( -1 );
        CHECK( effect_obj.get_intensity() == 1 );
    }

    SECTION( "intensity cannot be set greater than maximum" ) {
        // These don't go to 11
        effect_obj.set_intensity( 11 );
        CHECK( effect_obj.get_intensity() == 10 );
        // or 9+2
        effect_obj.set_intensity( 9 );
        effect_obj.mod_intensity( 2 );
        CHECK( effect_obj.get_intensity() == 10 );
    }

    // From JSON:
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
        effect debugged( &eff_id.obj(), 2_turns, num_bp, false, 1, calendar::turn );
        // Ensure it will last 2 turns, and is not permanent/paused
        REQUIRE( to_turns<int>( debugged.get_duration() ) == 2 );
        REQUIRE_FALSE( debugged.is_permanent() );

        // First decay - 1 turn left
        debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( debugged.get_duration() ) == 1 );
        // Effect not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Second decay - 0 turns left
        debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( debugged.get_duration() ) == 0 );
        // Effect still not removed
        CHECK( rem_ids.empty() );
        CHECK( rem_bps.empty() );

        // Third decay - 0 turns left
        debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( debugged.get_duration() ) == 0 );
        // Effect is removed
        CHECK( rem_ids.size() == 1 );
        CHECK( rem_bps.size() == 1 );
        // Effect ID and body part are pushed to the arrays
        CHECK( rem_ids.front() == debugged.get_id() );
        CHECK( rem_bps.front() == num_bp );
    }

    SECTION( "decay does not reduce paused/permanent effect duration" ) {
        effect debugged( &eff_id.obj(), 2_turns, num_bp, true, 1, calendar::turn );
        // Ensure it will last 2 turns, and is permanent/paused
        REQUIRE( to_turns<int>( debugged.get_duration() ) == 2 );
        REQUIRE( debugged.is_permanent() );

        // Decay twice - should have no effect on duration
        debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( debugged.get_duration() ) == 2 );
        debugged.decay( rem_ids, rem_bps, calendar::turn, false );
        CHECK( to_turns<int>( debugged.get_duration() ) == 2 );
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
    const efftype_id eff_id( "grabbed" );
    const body_part arm_r = bodypart_id( "arm_r" )->token;
    effect grabbed( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );

    // TODO: Determine a case where `reduced` makes a difference

    CHECK( grabbed.disp_short_desc( false ) == "You have been grabbed by an attack.\n"
           "You are being held in place, and dodging and blocking are very difficult." );

    CHECK( grabbed.disp_short_desc( true ) == "You have been grabbed by an attack.\n"
           "You are being held in place, and dodging and blocking are very difficult." );
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
    effect grabbed( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );
    CHECK_FALSE( grabbed.is_permanent() );
    // Pause makes it permanent
    grabbed.pause_effect();
    CHECK( grabbed.is_permanent() );
    // Pause again does nothing
    grabbed.pause_effect();
    CHECK( grabbed.is_permanent() );
    // Unpause removes permanence
    grabbed.unpause_effect();
    CHECK_FALSE( grabbed.is_permanent() );
    // Unpause again does nothing
    grabbed.unpause_effect();
    CHECK_FALSE( grabbed.is_permanent() );
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
    effect grabbed( &eff_id.obj(), 1_turns, arm_r, false, 1, calendar::turn );
    CHECK( grabbed.get_bp() == arm_r );
    // Switch to left arm
    grabbed.set_bp( arm_l );
    CHECK( grabbed.get_bp() == arm_l );
    CHECK_FALSE( grabbed.get_bp() == arm_r );
    // Back to right arm
    grabbed.set_bp( arm_r );
    CHECK( grabbed.get_bp() == arm_r );
    CHECK_FALSE( grabbed.get_bp() == arm_l );
}

// Effect modifiers
// ----------------
// TODO:
// effect::get_mod
// effect::get_avg_mod
// effect::get_amount
// effect::get_min_val
// effect::get_max_val
// effect::get_sizing
// effect::get_percentage
//
TEST_CASE( "effect modifiers", "[effect][modifier]" )
{
}

