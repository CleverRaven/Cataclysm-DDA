#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "options.h"
#include "player.h"
#include "type_id.h"

// Character::healing_rate floating-point `at_rest_quality` for awake/asleep states
static const float awake_rest = 0.0f;
static const float sleep_rest = 1.0f;

// Empty `dummy` of all traits, and give them a single trait with name `trait_name`
static void give_one_trait( player &dummy, const std::string trait_name )
{
    const trait_id trait( trait_name );
    dummy.clear_mutations();
    dummy.toggle_trait( trait );
    REQUIRE( dummy.has_trait( trait ) );
}

// Return the Character's `healing_rate` at the given healthy value and rest quality.
static float healing_rate_at_health( Character &dummy, const int healthy_value,
                                     const float rest_quality )
{
    dummy.set_healthy( healthy_value );
    return dummy.healing_rate( rest_quality );
}

TEST_CASE( "traits and mutations affecting healing rate", "[heal][trait][mutation]" )
{
    avatar dummy;

    // Normal healing rate
    const float normal = get_option<float>( "PLAYER_HEALING_RATE" );
    REQUIRE( normal > 0.0f );

    // Tolerance for error - below this amount, healing_rate rounds to 0
    const float t = 0.000007f;

    // Ensure baseline hidden health stat
    REQUIRE( dummy.get_healthy() == 0 );

    GIVEN( "no traits" ) {
        dummy.clear_mutations();

        THEN( "there are no healing modifiers from mutations" ) {
            CHECK( dummy.mutation_value( "healing_resting" ) == 0.0f );
            CHECK( dummy.mutation_value( "healing_awake" ) == 0.0f );
        }

        THEN( "healing rate is zero when awake" ) {
            CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        }

        THEN( "healing rate is normal when asleep" ) {
            CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( t ) );
        }
    }

    SECTION( "Regeneration" ) {
        give_one_trait( dummy, "REGEN" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 2.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 1.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 2.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 4.5f ).margin( t ) );
    }

    SECTION( "Rapid Metabolism" ) {
        give_one_trait( dummy, "MET_RAT" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.2f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.2f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 1.7f ).margin( t ) );
    }

    SECTION( "Very Fast Healer" ) {
        give_one_trait( dummy, "FASTHEALER2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.66f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.66f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 2.2f ).margin( t ) );
    }

    SECTION( "Fast Healer" ) {
        give_one_trait( dummy, "FASTHEALER" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.20f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.20f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 1.7f ).margin( t ) );
    }

    SECTION( "Weakening" ) {
        give_one_trait( dummy, "ROT1" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.002f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.25f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.75f ).margin( t ) );
    }

    SECTION( "Slow Healer" ) {
        give_one_trait( dummy, "SLOWHEALER" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.25f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.75f ).margin( t ) );
    }

    SECTION( "Poor Healer" ) {
        give_one_trait( dummy, "SLOWHEALER2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.66f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.33f ).margin( t ) );
    }

    SECTION( "Imperceptive Healer" ) {
        give_one_trait( dummy, "SLOWHEALER3" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.9f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.10f ).margin( t ) );
    }

    SECTION( "Deterioration" ) {
        give_one_trait( dummy, "ROT2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.02f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.0f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( t ) );
    }

    SECTION( "Disintegration" ) {
        give_one_trait( dummy, "ROT3" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.08f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.0f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * -0.1f ).margin( t ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( t ) );
    }
}

TEST_CASE( "health effects on healing rate", "[heal][health]" )
{
    avatar dummy;

    // Normal healing rate
    const float normal = get_option<float>( "PLAYER_HEALING_RATE" );
    REQUIRE( normal > 0.0f );

    // Tolerance for error - below this amount, healing_rate rounds to 0
    const float t = 0.000007f;

    SECTION( "normal health gives normal healing when asleep" ) {
        CHECK( healing_rate_at_health( dummy, 0, sleep_rest ) == Approx( normal ).margin( t ) );
    }

    SECTION( "bad health degrades healing when asleep" ) {
        // Poor health reduces healing linearly
        CHECK( healing_rate_at_health( dummy, -50, sleep_rest ) == Approx( 0.75f * normal ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, -100, sleep_rest ) == Approx( 0.5f * normal ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, -150, sleep_rest ) == Approx( 0.25f * normal ).margin( t ) );
        // Worst possible health: no healing even while asleep!
        CHECK( healing_rate_at_health( dummy, -200, sleep_rest ) == Approx( 0.0f * normal ).margin( t ) );
    }

    SECTION( "good health improves healing when asleep" ) {
        // Good health increases healing linearly
        CHECK( healing_rate_at_health( dummy, 50, sleep_rest ) == Approx( 1.25f * normal ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, 100, sleep_rest ) == Approx( 1.5f * normal ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, 150, sleep_rest ) == Approx( 1.75f * normal ).margin( t ) );
        // Best possible health: double healing!
        CHECK( healing_rate_at_health( dummy, 200, sleep_rest ) == Approx( 2.0f * normal ).margin( t ) );
    }

    SECTION( "health has no effect on healing while awake" ) {
        CHECK( healing_rate_at_health( dummy, -200, awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, -100, awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, 0, awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, 100, awake_rest ) == Approx( 0.0f ).margin( t ) );
        CHECK( healing_rate_at_health( dummy, 200, awake_rest ) == Approx( 0.0f ).margin( t ) );
    }
}

