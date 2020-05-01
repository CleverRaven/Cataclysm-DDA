#include "effect.h"

#include "catch/catch.hpp"

// Test `effect` class

// To cover:
//
// decay:
// - decay intensity and duration
// - add to removal list if duration <= 0
//
// Duration:
// get_start_time
// get_max_duration
// get_duration
// set_duration
// mod_duration
// mult_duration
//
// Intensity:
// get_intensity
// get_max_intensity
// set_intensity
// mod_intensity
// get_dur_add_perc
// get_int_dur_factor
// get_int_add_val
//
// Modifier type:
// get_mod
// get_avg_mod
// get_amount
// get_min_val
// get_max_val
// get_sizing
// get_percentage

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

