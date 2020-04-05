#include "avatar.h"
#include "game_constants.h"
#include "options.h"

#include "catch/catch.hpp"

// Return `bodyweight` in kilograms for a player at the given body mass index.
static float bodyweight_kg_at_bmi( player &dummy, float bmi )
{
    // Set enough stored calories to reach target BMI
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * ( bmi - 13 ) / 12 );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );

    return to_kilogram( dummy.bodyweight() );
}

// Return player `metabolic_rate_base` with a given mutation
static float metabolic_rate_with_mutation( player &dummy, std::string trait_name )
{
    dummy.empty_traits();
    dummy.toggle_trait( trait_id( trait_name ) );

    return dummy.metabolic_rate_base();
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
    // Activity level is a floating-point number, but only set to discrete values:
    //
    // NO_EXERCISE = 1.2f;
    // LIGHT_EXERCISE = 1.375f;
    // MODERATE_EXERCISE = 1.55f;
    // ACTIVE_EXERCISE = 1.725f;
    // EXTRA_EXERCISE = 1.9f;

    // Functions tested:
    // activity_level_str (return string constant for each range)
    // reset_activity_level (to NO_EXERCISE)
    // increase_activity_level (only if greater than current)
    // decrease_activity_level (only if less than current)

    avatar dummy;

    SECTION( "reset activity level to NO_EXERCISE" ) {
        // Start at no exercise
        dummy.reset_activity_level();
        CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
        // Increase and confirm
        dummy.increase_activity_level( MODERATE_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        // Reset back and ensure it worked
        dummy.reset_activity_level();
        CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
    }

    SECTION( "increase_activity_level" ) {
        // Start at the lowest level
        dummy.reset_activity_level();
        REQUIRE( dummy.activity_level_str() == "NO_EXERCISE" );

        // Increase level a couple times
        dummy.increase_activity_level( LIGHT_EXERCISE );
        CHECK( dummy.activity_level_str() == "LIGHT_EXERCISE" );
        dummy.increase_activity_level( MODERATE_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        // Cannot 'increase' to lower level
        dummy.increase_activity_level( LIGHT_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        dummy.increase_activity_level( NO_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        // Increase to highest level
        dummy.increase_activity_level( ACTIVE_EXERCISE );
        CHECK( dummy.activity_level_str() == "ACTIVE_EXERCISE" );
        dummy.increase_activity_level( EXTRA_EXERCISE );
        CHECK( dummy.activity_level_str() == "EXTRA_EXERCISE" );
        // Cannot increase beyond the highest
        dummy.increase_activity_level( EXTRA_EXERCISE );
        CHECK( dummy.activity_level_str() == "EXTRA_EXERCISE" );

    }

    SECTION( "decrease_activity_level" ) {
        // Start at the highest level
        dummy.reset_activity_level();
        dummy.increase_activity_level( EXTRA_EXERCISE );
        REQUIRE( dummy.activity_level_str() == "EXTRA_EXERCISE" );

        // Decrease level a couple times
        dummy.decrease_activity_level( ACTIVE_EXERCISE );
        CHECK( dummy.activity_level_str() == "ACTIVE_EXERCISE" );
        dummy.decrease_activity_level( MODERATE_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        // Cannot 'decrease' to higher level
        dummy.decrease_activity_level( EXTRA_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        dummy.decrease_activity_level( ACTIVE_EXERCISE );
        CHECK( dummy.activity_level_str() == "MODERATE_EXERCISE" );
        // Decrease to lowest level
        dummy.decrease_activity_level( LIGHT_EXERCISE );
        CHECK( dummy.activity_level_str() == "LIGHT_EXERCISE" );
        dummy.decrease_activity_level( NO_EXERCISE );
        CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
        // Cannot decrease below lowest
        dummy.decrease_activity_level( NO_EXERCISE );
        CHECK( dummy.activity_level_str() == "NO_EXERCISE" );
    }
}

TEST_CASE( "character metabolic rate", "[biometrics][metabolism]" )
{
    avatar dummy;

    // Metabolic base rate uses PLAYER_HUNGER_RATE from game_balance.json, described as "base hunger
    // rate per 5 minutes". With no metabolism-affecting mutations, metabolism should be this value.
    const float normal_metabolic_rate = get_option<float>( "PLAYER_HUNGER_RATE" );
    dummy.empty_traits();
    CHECK( dummy.metabolic_rate_base() == normal_metabolic_rate );

    // The remaining checks assume the configured base rate is 1.0; if this is ever changed in the
    // game balance JSON, the below tests are likely to fail and need adjustment.
    REQUIRE( normal_metabolic_rate == 1.0f );

    // Mutations with a "metabolism_modifier" in mutations.json add/subtract to the base rate.
    // For example the rapid / fast / very fast / extreme metabolisms:
    //
    //     MET_RAT (+0.333), HUNGER (+0.5), HUNGER2 (+1.0), HUNGER3 (+2.0)
    //
    // And the light eater / heat dependent / cold blooded spectrum:
    //
    //     LIGHTEATER (-0.333), COLDBLOOD (-0.333), COLDBLOOD2/3/4 (-0.5)
    //
    // If metabolism modifiers are changed, the below check(s) need to be adjusted as well.

    CHECK( metabolic_rate_with_mutation( dummy, "MET_RAT" ) == Approx( 1.333f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER" ) == Approx( 1.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER2" ) == Approx( 2.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "HUNGER3" ) == Approx( 3.0f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "LIGHTEATER" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD" ) == Approx( 0.667f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD2" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD3" ) == Approx( 0.5f ) );
    CHECK( metabolic_rate_with_mutation( dummy, "COLDBLOOD4" ) == Approx( 0.5f ) );
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

