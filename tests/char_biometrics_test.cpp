#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "creature.h"
#include "game_constants.h"
#include "options.h"
#include "player.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"

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

// Return the player's `kcal_speed_penalty` value at the given body mass index
static int kcal_speed_penalty_at_bmi( player &dummy, float bmi )
{
    set_player_bmi( dummy, bmi );
    return dummy.kcal_speed_penalty();
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

// Clear player traits and give them a single trait by name
static void set_single_trait( player &dummy, std::string trait_name )
{
    dummy.clear_mutations();
    dummy.toggle_trait( trait_id( trait_name ) );
    REQUIRE( dummy.has_trait( trait_id( trait_name ) ) );
}

// Return player `metabolic_rate_base` with a given mutation
static float metabolic_rate_with_mutation( player &dummy, std::string trait_name )
{
    set_single_trait( dummy, trait_name );
    return dummy.metabolic_rate_base();
}

// Return player `get_bmr` (basal metabolic rate) at the given activity level.
static int bmr_at_act_level( player &dummy, float activity_level )
{
    dummy.reset_activity_level();
    dummy.increase_activity_level( activity_level );

    return dummy.get_bmr();
}

// Return player `height()` with a given base height and size trait (SMALL, MEDIUM, LARGE, HUGE).
static int height_with_base_and_size( player &dummy, int base_height, std::string size_trait )
{
    clear_character( dummy );
    dummy.mod_base_height( base_height - dummy.base_height() );

    // MEDIUM is not an actual trait; just ignore it
    if( size_trait == "SMALL" || size_trait == "LARGE" || size_trait == "HUGE" ) {
        dummy.toggle_trait( trait_id( size_trait ) );
    }

    return dummy.height();
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
    // "BMIs under 20 and over 25 have been associated with higher all-causes mortality,
    // with the risk increasing with distance from the 20–25 range."
    //
    // https://en.wikipedia.org/wiki/Body_mass_index

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
    CHECK( max_healthy_at_bmi( dummy, 25.0f ) == 200 );
    // Overweight (25)
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

TEST_CASE( "character height and body size mutations", "[biometrics][height][mutation]" )
{
    // If character is 175cm starting height, they are 15cm shorter than the upper threshold for
    // MEDIUM size (which is 190cm). If they mutate to LARGE size, their height will be at the same
    // relationship to the upper threshold, 240cm - 15cm = 225cm.
    //
    // In general, mutating to another size simply adjusts the return value of `Character::height()`
    // in relationship to the original starting height.

    avatar dummy;
    clear_character( dummy );

    GIVEN( "character height started at 175cm" ) {
        int init_height = 175;
        dummy.mod_base_height( init_height - dummy.base_height() );
        REQUIRE( dummy.base_height() == init_height );

        WHEN( "they are normal-sized (MEDIUM)" ) {
            REQUIRE( dummy.get_size() == MS_MEDIUM );

            THEN( "height is initial height" ) {
                CHECK( dummy.height() == init_height );
            }
        }

        WHEN( "they become SMALL" ) {
            set_single_trait( dummy, "SMALL" );
            REQUIRE( dummy.get_size() == MS_SMALL );

            THEN( "they are 50cm shorter" ) {
                CHECK( dummy.height() == init_height - 50 );
            }
        }

        WHEN( "they become LARGE" ) {
            set_single_trait( dummy, "LARGE" );
            REQUIRE( dummy.get_size() == MS_LARGE );

            THEN( "they are 50cm taller" ) {
                CHECK( dummy.height() == init_height + 50 );
            }
        }

        WHEN( "they become HUGE" ) {
            set_single_trait( dummy, "HUGE" );
            REQUIRE( dummy.get_size() == MS_HUGE );

            THEN( "they are 100cm taler" ) {
                CHECK( dummy.height() == init_height + 100 );
            }
        }
    }

    // More generally

    GIVEN( "character height strarted at 160cm" ) {
        CHECK( height_with_base_and_size( dummy, 160, "MEDIUM" ) == 160 );
        // Always 30 cm shorter than max for each size class
        CHECK( height_with_base_and_size( dummy, 160, "SMALL" ) == 110 );
        CHECK( height_with_base_and_size( dummy, 160, "LARGE" ) == 210 );
        CHECK( height_with_base_and_size( dummy, 160, "HUGE" ) == 260 );
    }

    SECTION( "character height started at 190cm" ) {
        CHECK( height_with_base_and_size( dummy, 190, "MEDIUM" ) == 190 );
        // Always at maximum height for each size class
        CHECK( height_with_base_and_size( dummy, 190, "SMALL" ) == 140 );
        CHECK( height_with_base_and_size( dummy, 190, "LARGE" ) == 240 );
        CHECK( height_with_base_and_size( dummy, 190, "HUGE" ) == 290 );
    }
}

TEST_CASE( "size and height determine body weight", "[biometrics][bodyweight]" )
{
    avatar dummy;
    clear_character( dummy );

    // Body weight is calculated as ( BMI * (height/100)^2 ). At any given height, body weight
    // varies based on body mass index (which in turn depends on the amount of stored calories).

    // Check bodyweights at each size for two different heights:
    // 175cm (original default) and 190cm (maximum for MEDIUM sized human)

    GIVEN( "character height started at 175cm" ) {
        dummy.mod_base_height( 175 - dummy.base_height() );
        REQUIRE( dummy.base_height() == 175 );

        WHEN( "character is normal-sized (medium)" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SMALL" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "LARGE" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "HUGE" ) ) );
            REQUIRE( dummy.get_size() == MS_MEDIUM );


            THEN( "bodyweight varies from ~49-107kg" ) {
                // BMI [16-35] is "Emaciated/Underweight" to "Obese/Very Obese"
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0 ) == Approx( 49.0 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0 ) == Approx( 76.6 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0 ) == Approx( 107.2 ).margin( 0.1f ) );
            }
        }

        WHEN( "character is small" ) {
            set_single_trait( dummy, "SMALL" );
            REQUIRE( dummy.get_size() == MS_SMALL );

            THEN( "bodyweight varies from ~25-55kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 25.0f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 39.1f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 54.7f ).margin( 0.1f ) );
            }
        }

        WHEN( "character is large" ) {
            set_single_trait( dummy, "LARGE" );
            REQUIRE( dummy.get_size() == MS_LARGE );

            THEN( "bodyweight varies from ~81-177kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 81.0f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 126.6f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 177.2f ).margin( 0.1f ) );
            }
        }

        WHEN( "character is huge" ) {
            set_single_trait( dummy, "HUGE" );
            REQUIRE( dummy.get_size() == MS_HUGE );

            THEN( "bodyweight varies from ~121-265kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 121.0f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 189.1f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 264.7f ).margin( 0.1f ) );
            }
        }
    }

    GIVEN( "character height started at 190cm" ) {
        dummy.mod_base_height( 190 - dummy.base_height() );
        REQUIRE( dummy.base_height() == 190 );

        WHEN( "character is normal-sized (medium)" ) {
            REQUIRE_FALSE( dummy.has_trait( trait_id( "SMALL" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "LARGE" ) ) );
            REQUIRE_FALSE( dummy.has_trait( trait_id( "HUGE" ) ) );
            REQUIRE( dummy.get_size() == MS_MEDIUM );


            THEN( "bodyweight varies from ~57-126kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0 ) == Approx( 57.8 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0 ) == Approx( 90.3 ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0 ) == Approx( 126.3 ).margin( 0.1f ) );
            }
        }

        WHEN( "character is small" ) {
            set_single_trait( dummy, "SMALL" );
            REQUIRE( dummy.get_size() == MS_SMALL );

            THEN( "bodyweight varies from ~31-68kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 31.4f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 49.0f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 68.6f ).margin( 0.1f ) );
            }
        }

        WHEN( "character is large" ) {
            set_single_trait( dummy, "LARGE" );
            REQUIRE( dummy.get_size() == MS_LARGE );

            THEN( "bodyweight varies from ~92-201kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 92.16f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 144.0f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 201.6f ).margin( 0.1f ) );
            }
        }

        WHEN( "character is huge" ) {
            set_single_trait( dummy, "HUGE" );
            REQUIRE( dummy.get_size() == MS_HUGE );

            THEN( "bodyweight varies from ~134-294kg" ) {
                CHECK( bodyweight_kg_at_bmi( dummy, 16.0f ) == Approx( 134.6f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 25.0f ) == Approx( 210.2f ).margin( 0.1f ) );
                CHECK( bodyweight_kg_at_bmi( dummy, 35.0f ) == Approx( 294.3f ).margin( 0.1f ) );
            }
        }
    }

}

TEST_CASE( "activity level reset, increase and decrease", "[biometrics][activity]" )
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

TEST_CASE( "mutations may affect character metabolic rate", "[biometrics][metabolism]" )
{
    avatar dummy;

    // Metabolic base rate uses PLAYER_HUNGER_RATE from game_balance.json, described as "base hunger
    // rate per 5 minutes". With no metabolism-affecting mutations, metabolism should be this value.
    const float normal_metabolic_rate = get_option<float>( "PLAYER_HUNGER_RATE" );
    dummy.clear_mutations();
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

TEST_CASE( "basal metabolic rate with various size and metabolism", "[biometrics][bmr]" )
{
    avatar dummy;

    // Basal metabolic rate depends on size (height), bodyweight (BMI), and activity level
    // scaled by metabolic base rate. Assume default metabolic rate.
    REQUIRE( dummy.metabolic_rate_base() == 1.0f );

    // To keep things simple, use normal BMI for al tests
    set_player_bmi( dummy, 25.0f );
    REQUIRE( dummy.get_bmi() == Approx( 25.0f ).margin( 0.001f ) );

    // Tests cover:
    // - normal, very fast, and cold-blooded metabolisms for normal body size
    // - three different levels of exercise (none, moderate, extra)

    // CHECK expressions have expected value on the left hand side for better readability.

    SECTION( "normal body size" ) {
        REQUIRE( dummy.get_size() == MS_MEDIUM );

        SECTION( "normal metabolism" ) {
            CHECK( 2087 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 2696 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 3304 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very fast metabolism" ) {
            set_single_trait( dummy, "HUNGER2" );
            REQUIRE( dummy.metabolic_rate_base() == 2.0f );

            CHECK( 4174 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 5391 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 6608 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }

        SECTION( "very slow (cold-blooded) metabolism" ) {
            set_single_trait( dummy, "COLDBLOOD3" );
            REQUIRE( dummy.metabolic_rate_base() == 0.5f );

            CHECK( 1044 == bmr_at_act_level( dummy, NO_EXERCISE ) );
            CHECK( 1348 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
            CHECK( 1652 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
        }
    }

    SECTION( "small body size" ) {
        set_single_trait( dummy, "SMALL" );
        REQUIRE( dummy.get_size() == MS_SMALL );

        CHECK( 1262 == bmr_at_act_level( dummy, NO_EXERCISE ) );
        CHECK( 1630 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
        CHECK( 1998 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
    }

    SECTION( "large body size" ) {
        set_single_trait( dummy, "LARGE" );
        REQUIRE( dummy.get_size() == MS_LARGE );

        CHECK( 3062 == bmr_at_act_level( dummy, NO_EXERCISE ) );
        CHECK( 3955 == bmr_at_act_level( dummy, MODERATE_EXERCISE ) );
        CHECK( 4848 == bmr_at_act_level( dummy, EXTRA_EXERCISE ) );
    }
}

TEST_CASE( "body mass effect on speed", "[bmi][speed]" )
{
    avatar dummy;

    // Practically dead
    CHECK( kcal_speed_penalty_at_bmi( dummy, 0.1f ) == -90 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 1.0f ) == -87 );
    // Skeletal (<14)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 8.0f ) == -67 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 9.0f ) == -64 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 10.0f ) == -61 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 11.0f ) == -59 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 12.0f ) == -56 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 13.0f ) == -53 );
    // Emaciated (14)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 14.0f ) == -50 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 14.5f ) == -44 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 15.0f ) == -38 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 15.5f ) == -31 );
    // Underweight (16)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 16.0f ) == -25 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 16.5f ) == -20 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 17.0f ) == -15 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 17.5f ) == -10 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 18.0f ) == -5 );
    // Normal (18.5)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 18.5f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 19.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 20.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 21.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 22.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 23.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 24.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 25.0f ) == 0 );
    // Overweight (25)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 26.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 27.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 28.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 29.0f ) == 0 );
    // Obese (30)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 30.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 31.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 32.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 33.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 34.0f ) == 0 );
    // Very obese (35)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 35.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 36.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 37.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 38.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 39.0f ) == 0 );
    // Morbidly obese (40)
    CHECK( kcal_speed_penalty_at_bmi( dummy, 40.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 41.0f ) == 0 );
    CHECK( kcal_speed_penalty_at_bmi( dummy, 42.0f ) == 0 );
}

