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
