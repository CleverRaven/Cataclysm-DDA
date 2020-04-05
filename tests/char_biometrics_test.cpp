#include "avatar.h"
#include "game_constants.h"

#include "catch/catch.hpp"

// Return `bodyweight` in kilograms for a player at the given body mass index.
static float bodyweight_kg_at_bmi( player &dummy, float bmi )
{
    // Set enough stored calories to reach target BMI
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * ( bmi - 13 ) / 12 );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );

    return to_kilogram( dummy.bodyweight() );
}

TEST_CASE( "character height and body weight", "[biometrics][height][bodyweight]" )
{
    avatar dummy;

    // NOTE: As of this writing, Character `init_height` is hard-coded to 175 (cm) in the class
    // definition in character.h, as a protected member with no functions to access it. Calling
    // `height()` now is an indirect way to verify it before continuing.
    REQUIRE( dummy.height() == 175 );

    // Body weight is calculated as ( BMI * (height/100)^2 ). At any given height, body weight
    // varies based on body mass index (which in turn depends on the amount of stored calories).

    GIVEN( "character is normal-sized (medium)" ) {
        REQUIRE_FALSE( dummy.has_trait( trait_id( "SMALL" ) ) );
        REQUIRE_FALSE( dummy.has_trait( trait_id( "LARGE" ) ) );
        REQUIRE_FALSE( dummy.has_trait( trait_id( "HUGE" ) ) );
        REQUIRE( dummy.get_size() == MS_MEDIUM );

        THEN( "height is 175cm" ) {
            CHECK( dummy.height() == 175 );
        }
        THEN( "bodyweight varies from ~49-107kg" ) {
            // BMI [16-35] is "Emaciated/Underweight" to "Obese/Very Obese"
            CHECK( bodyweight_kg_at_bmi( dummy, 16.0 ) == Approx( 49.0 ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 25.0 ) == Approx( 76.6 ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 35.0 ) == Approx( 107.2 ).margin( 0.1f ) );
        }
    }

    GIVEN( "character is small" ) {
        dummy.toggle_trait( trait_id( "SMALL" ) );
        REQUIRE( dummy.has_trait( trait_id( "SMALL" ) ) );
        REQUIRE( dummy.get_size() == MS_SMALL );

        THEN( "height is 125cm" ) {
            CHECK( dummy.height() == 125 );
        }
        THEN( "bodyweight varies from ~25-55kg" ) {
            CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 25.0f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 39.1f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 54.7f ).margin( 0.1f ) );
        }
    }

    GIVEN( "character is large" ) {
        dummy.toggle_trait( trait_id( "LARGE" ) );
        REQUIRE( dummy.has_trait( trait_id( "LARGE" ) ) );
        REQUIRE( dummy.get_size() == MS_LARGE );

        THEN( "height is 225cm" ) {
            CHECK( dummy.height() == 225 );
        }
        THEN( "bodyweight varies from ~81-177kg" ) {
            CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 81.0f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 126.6f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 177.2f ).margin( 0.1f ) );
        }
    }

    GIVEN( "character is huge" ) {
        dummy.toggle_trait( trait_id( "HUGE" ) );
        REQUIRE( dummy.has_trait( trait_id( "HUGE" ) ) );
        REQUIRE( dummy.get_size() == MS_HUGE );

        THEN( "height is 275cm" ) {
            CHECK( dummy.height() == 275 );
        }
        THEN( "bodyweight varies from ~121-265kg" ) {
            CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 121.0f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 189.1f ).margin( 0.1f ) );
            CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 264.7f ).margin( 0.1f ) );
        }
    }
}

TEST_CASE( "character activity level", "[biometrics][activity]" )
{
    // Activity level is a PROTECTED floating-point number
    // but it is only set to discrete values(??)
    //
    // NO_EXERCISE = 1.2f;
    // LIGHT_EXERCISE = 1.375f;
    // MODERATE_EXERCISE = 1.55f;
    // ACTIVE_EXERCISE = 1.725f;
    // EXTRA_EXERCISE = 1.9f;

    // Functions:
    // activity_level_str (return string constant for each range)
    // reset_activity_level (to NO_EXERCISE)
    // increase_activity_level
    // decrease_activity_level

    avatar dummy;
    CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
}

TEST_CASE( "character basal metabolic rate", "[biometrics][bmr]" )
{
    avatar dummy;

    CHECK( dummy.height() == 175 );
    CHECK( dummy.bodyweight() == 76562500_milligram );
    CHECK( dummy.metabolic_rate_base() == 1.0f );
    CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
    CHECK( dummy.get_bmr() == 2087 );

    // metabolic_rate_base:
    // PLAYER_HUNGER_RATE option, metabolism_modifier (based on mutation)

    // metabolic_rate (filled with TODOs)
    // Based on four thresholds for hunger level(?)
    // Penalize fast survivors
    // Cold decreases metabolism

    // get_bmr:
    // bodyweight, height, metabolic_base_rate, activity_level
}

