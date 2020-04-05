#include "avatar.h"
#include "game_constants.h"
#include "options.h"

#include "catch/catch.hpp"

// Return the `kcal_ratio` needed to reach the given `bmi`
//   BMI = 13 + (12 * kcal_ratio)
//   kcal_ratio = (BMI - 13) / 12
static float bmi_to_kcal_ratio( float bmi )
{
    return ( bmi - 13 ) / 12;
}

// Set enough stored calories to reach target BMI
static void set_player_bmi( player &dummy, float bmi )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * bmi_to_kcal_ratio( bmi ) );
    REQUIRE( dummy.get_bmi() == Approx( bmi ).margin( 0.001f ) );
}

// Return the player's `get_bmi` at `kcal_percent` (actually a ratio of stored_kcal to healthy_kcal)
static float bmi_at_kcal_ratio( player &dummy, float kcal_percent )
{
    dummy.set_stored_kcal( dummy.get_healthy_kcal() * kcal_percent );
    REQUIRE( dummy.get_kcal_percent() == Approx( kcal_percent ).margin( 0.001f ) );

    return dummy.get_bmi();
}

// Return the player's `get_max_healthy` value at the given body mass index
static int max_healthy_at_bmi( player &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.get_max_healthy();
}

// Return the player's `get_weight_string` at the given body mass index
static std::string weight_string_at_bmi( player &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.get_weight_string();
}

// Return `bodyweight` in kilograms for a player at the given body mass index.
static float bodyweight_kg_at_bmi( player &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return to_kilogram( dummy.bodyweight() );
}

// Return player `metabolic_rate_base` with a given mutation
static float metabolic_rate_with_mutation( player &dummy, std::string trait_name )
{
    dummy.empty_traits();
    dummy.toggle_trait( trait_id( trait_name ) );
    REQUIRE( dummy.has_trait( trait_id( trait_name ) ) );

    return dummy.metabolic_rate_base();
}

TEST_CASE( "body mass index determines weight description", "[biometrics][bmi][weight]" )
{
    avatar dummy;

    CHECK( weight_string_at_bmi( dummy, 13.0f ) == "Skeletal" );
    CHECK( weight_string_at_bmi( dummy, 13.9f ) == "Skeletal" );
    // 14
    CHECK( weight_string_at_bmi( dummy, 14.1f ) == "Emaciated" );
    CHECK( weight_string_at_bmi( dummy, 15.9f ) == "Emaciated" );
    // 16
    CHECK( weight_string_at_bmi( dummy, 16.1f ) == "Underweight" );
    CHECK( weight_string_at_bmi( dummy, 18.4f ) == "Underweight" );
    // 18.5
    CHECK( weight_string_at_bmi( dummy, 18.6f ) == "Normal" );
    CHECK( weight_string_at_bmi( dummy, 24.9f ) == "Normal" );
    // 25
    CHECK( weight_string_at_bmi( dummy, 25.1f ) == "Overweight" );
    CHECK( weight_string_at_bmi( dummy, 29.9f ) == "Overweight" );
    // 30
    CHECK( weight_string_at_bmi( dummy, 30.1f ) == "Obese" );
    CHECK( weight_string_at_bmi( dummy, 34.9f ) == "Obese" );
    // 35
    CHECK( weight_string_at_bmi( dummy, 35.1f ) == "Very Obese" );
    CHECK( weight_string_at_bmi( dummy, 39.9f ) == "Very Obese" );
    // 40
    CHECK( weight_string_at_bmi( dummy, 40.1f ) == "Morbidly Obese" );
    CHECK( weight_string_at_bmi( dummy, 41.0f ) == "Morbidly Obese" );
}

TEST_CASE( "stored kcal ratio determines body mass index", "[biometrics][kcal][bmi]" )
{
    avatar dummy;

    // Skeletal (<14)
    CHECK( bmi_at_kcal_ratio( dummy, 0.01f ) == Approx( 13.12f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.05f ) == Approx( 13.6f ).margin( 0.01f ) );
    // Emaciated (14)
    CHECK( bmi_at_kcal_ratio( dummy, 0.1f ) == Approx( 14.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.2f ) == Approx( 15.4f ).margin( 0.01f ) );
    // Underweight (16)
    CHECK( bmi_at_kcal_ratio( dummy, 0.3f ) == Approx( 16.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.4f ) == Approx( 17.8f ).margin( 0.01f ) );
    // Normal (18.5)
    CHECK( bmi_at_kcal_ratio( dummy, 0.5f ) == Approx( 19.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.6f ) == Approx( 20.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.7f ) == Approx( 21.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.8f ) == Approx( 22.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 0.9f ) == Approx( 23.8f ).margin( 0.01f ) );
    // Overweight (25)
    CHECK( bmi_at_kcal_ratio( dummy, 1.0f ) == Approx( 25.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.1f ) == Approx( 26.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.2f ) == Approx( 27.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.3f ) == Approx( 28.6f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.4f ) == Approx( 29.8f ).margin( 0.01f ) );
    // Obese (30)
    CHECK( bmi_at_kcal_ratio( dummy, 1.5f ) == Approx( 31.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.6f ) == Approx( 32.2f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.7f ) == Approx( 33.4f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 1.8f ) == Approx( 34.6f ).margin( 0.01f ) );
    // Very obese (35)
    CHECK( bmi_at_kcal_ratio( dummy, 1.9f ) == Approx( 35.8f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 2.0f ) == Approx( 37.0f ).margin( 0.01f ) );
    // Morbidly obese (40)
    CHECK( bmi_at_kcal_ratio( dummy, 2.25f ) == Approx( 40.0f ).margin( 0.01f ) );
    CHECK( bmi_at_kcal_ratio( dummy, 2.5f ) == Approx( 43.0f ).margin( 0.01f ) );
}

TEST_CASE( "body mass index determines maximum healthiness", "[biometrics][bmi][max]" )
{
    avatar dummy;

    // Skeletal (<14)
    CHECK( max_healthy_at_bmi( dummy, 8.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 9.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 10.0f ) == -183 );
    CHECK( max_healthy_at_bmi( dummy, 11.0f ) == -115 );
    CHECK( max_healthy_at_bmi( dummy, 12.0f ) == -53 );
    CHECK( max_healthy_at_bmi( dummy, 13.0f ) == 2 );
    // Emaciated (14)
    CHECK( max_healthy_at_bmi( dummy, 14.0f ) == 51 );
    CHECK( max_healthy_at_bmi( dummy, 15.0f ) == 95 );
    // Underweight (16)
    CHECK( max_healthy_at_bmi( dummy, 16.0f ) == 133 );
    CHECK( max_healthy_at_bmi( dummy, 17.0f ) == 164 );
    CHECK( max_healthy_at_bmi( dummy, 18.0f ) == 189 );
    // Normal (18.5)
    CHECK( max_healthy_at_bmi( dummy, 19.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 20.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 21.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 22.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 23.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 24.0f ) == 200 );
    // Overweight (25)
    CHECK( max_healthy_at_bmi( dummy, 25.0f ) == 200 );
    CHECK( max_healthy_at_bmi( dummy, 26.0f ) == 178 );
    CHECK( max_healthy_at_bmi( dummy, 27.0f ) == 149 );
    CHECK( max_healthy_at_bmi( dummy, 28.0f ) == 115 );
    CHECK( max_healthy_at_bmi( dummy, 29.0f ) == 74 );
    // Obese (30)
    CHECK( max_healthy_at_bmi( dummy, 30.0f ) == 28 );
    CHECK( max_healthy_at_bmi( dummy, 31.0f ) == -25 );
    CHECK( max_healthy_at_bmi( dummy, 32.0f ) == -83 );
    CHECK( max_healthy_at_bmi( dummy, 33.0f ) == -148 );
    CHECK( max_healthy_at_bmi( dummy, 34.0f ) == -200 );
    // Very obese (35)
    CHECK( max_healthy_at_bmi( dummy, 35.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 36.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 37.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 38.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 39.0f ) == -200 );
    // Morbidly obese (40)
    CHECK( max_healthy_at_bmi( dummy, 40.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 41.0f ) == -200 );
    CHECK( max_healthy_at_bmi( dummy, 42.0f ) == -200 );
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

