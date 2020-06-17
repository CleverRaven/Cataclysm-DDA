#include <string>

#include "avatar.h"
#include "effect.h"
#include "options.h"
#include "player.h"
#include "type_id.h"

#include "catch/catch.hpp"

// Tests for Character healing, including:
//
// - Character::healing_rate
// - Character::healing_rate_medicine
//
// These functions return HP restored per turn, depending on:
//
// - rest quality (awake or sleeping)
// - traits like "Imperceptive Healer" and "Rapid Metabolism"
// - healthy value (hidden health stat)
// - effects ot being bandaged and/or disinfected

// Character::healing_rate floating-point `at_rest_quality` for awake/asleep states
static const float awake_rest = 0.0f;
static const float sleep_rest = 1.0f;

// Equivalent of one hit point per day, in turns (aka seconds)
static const float hp_per_day = 1.0f / 86400;

// Tolerance for error - below this amount, healing_rate rounds to 0
static const float tol = 0.000007f;
static const float zero = 0.0f;

// Healing effects
static const efftype_id effect_bandaged( "bandaged" );
static const efftype_id effect_disinfected( "disinfected" );

// Empty `dummy` of all traits, and give them a single trait with name `trait_name`
static void give_one_trait( player &dummy, const std::string &trait_name )
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

// At baseline human defaults, with no treatment or traits, the character only heals while sleeping.
// Default as of this writing is is 0.0001, or 8.64 HP per day.
TEST_CASE( "baseline healing rate with no healing traits", "[heal][baseline]" )
{
    avatar dummy;

    // What is considered normal baseline healing rate comes from game_balance.json.
    const float normal = get_option<float>( "PLAYER_HEALING_RATE" );
    REQUIRE( normal > 1.0f * hp_per_day );

    // Ensure baseline hidden health stat
    REQUIRE( dummy.get_healthy() == 0 );

    GIVEN( "character with no healing traits" ) {
        dummy.clear_mutations();
        // Ensure there are no healing modifiers from traits/mutations
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );

        THEN( "healing rate is zero when awake" ) {
            CHECK( dummy.healing_rate( awake_rest ) == zero );
        }

        THEN( "healing rate is normal when asleep" ) {
            CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( tol ) );
        }
    }
}

// Healing rate may be affected by any of several traits/mutations, and the effects vary depending
// on whether the character is asleep or awake.
TEST_CASE( "traits and mutations affecting healing rate", "[heal][trait][mutation]" )
{
    avatar dummy;

    // TODO: Include `healing_rate_medicine` for trait-related healing effects, since many of these
    // affect healing while awake (which can only happen there), or have such small effects as to be
    // unmeasurable for `healing_rate` (ex. Deterioration)

    // Normal healing rate from game_balance.json
    const float normal = get_option<float>( "PLAYER_HEALING_RATE" );
    REQUIRE( normal > 1.0f * hp_per_day );

    // Ensure baseline hidden health stat
    REQUIRE( dummy.get_healthy() == 0 );

    // "Your flesh regenerates from wounds incredibly quickly."
    SECTION( "Regeneration" ) {
        give_one_trait( dummy, "REGEN" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 2.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 1.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 2.0f ).margin( tol ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 4.5f ).margin( tol ) );
    }

    // "You require more resources than most, but heal more rapidly as well.
    // Provides weak regeneration even when not asleep."
    SECTION( "Rapid Metabolism" ) {
        give_one_trait( dummy, "MET_RAT" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.2f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.2f ).margin( tol ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 1.7f ).margin( tol ) );
    }

    // "Your flesh regenerates slowly, and you will regain HP even when not sleeping."
    SECTION( "Very Fast Healer" ) {
        give_one_trait( dummy, "FASTHEALER2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.66f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.66f ).margin( tol ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 2.2f ).margin( tol ) );
    }

    // "You heal faster when sleeping and will even recover a small amount of HP when not sleeping."
    SECTION( "Fast Healer" ) {
        give_one_trait( dummy, "FASTHEALER" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.20f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.5f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * 0.20f ).margin( tol ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 1.7f ).margin( tol ) );
    }

    // "You feel as though you are slowly weakening and your body heals slower."
    SECTION( "Weakening" ) {
        give_one_trait( dummy, "ROT1" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.002f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.25f );

        CHECK( dummy.healing_rate( awake_rest ) == zero );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.75f ).margin( tol ) );
    }

    // "You heal a little slower than most; sleeping will heal less HP."
    SECTION( "Slow Healer" ) {
        give_one_trait( dummy, "SLOWHEALER" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.25f );

        CHECK( dummy.healing_rate( awake_rest ) == zero );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.75f ).margin( tol ) );
    }

    // "Your health recovery through sleeping is severely impaired and causes you to recover only a third of usual HP."
    SECTION( "Poor Healer" ) {
        give_one_trait( dummy, "SLOWHEALER2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.66f );

        CHECK( dummy.healing_rate( awake_rest ) == zero );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.33f ).margin( tol ) );
    }

    // "You recover barely any health through sleeping - it will heal only one tenth of usual HP."
    SECTION( "Imperceptive Healer" ) {
        give_one_trait( dummy, "SLOWHEALER3" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == 0.0f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == -0.9f );

        CHECK( dummy.healing_rate( awake_rest ) == zero );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal * 0.10f ).margin( tol ) );
    }

    // "Your body is very slowly wasting away."
    SECTION( "Deterioration" ) {
        give_one_trait( dummy, "ROT2" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.02f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.0f );

        CHECK( dummy.healing_rate( awake_rest ) == zero );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( tol ) );
    }

    // "Your body is slowly wasting away!"
    SECTION( "Disintegration" ) {
        give_one_trait( dummy, "ROT3" );

        REQUIRE( dummy.mutation_value( "healing_awake" ) == -0.08f );
        REQUIRE( dummy.mutation_value( "healing_resting" ) == 0.0f );

        CHECK( dummy.healing_rate( awake_rest ) == Approx( normal * -0.1f ).margin( tol ) );
        CHECK( dummy.healing_rate( sleep_rest ) == Approx( normal ).margin( tol ) );
    }
}

// The "hidden health" stat returned by Character::get_healthy ranges from [-200, 200] and
// influences healing rate significantly.
TEST_CASE( "health effects on healing rate", "[heal][health]" )
{
    avatar dummy;

    // Normal healing rate from game_balance.json
    const float normal = get_option<float>( "PLAYER_HEALING_RATE" );
    REQUIRE( normal > 1.0f * hp_per_day );

    SECTION( "normal health gives normal healing when asleep" ) {
        CHECK( healing_rate_at_health( dummy, 0, sleep_rest ) == Approx( normal ).margin( tol ) );
    }

    SECTION( "bad health degrades healing when asleep" ) {
        // Poor health reduces healing linearly
        CHECK( healing_rate_at_health( dummy, -50, sleep_rest ) == Approx( 0.75f * normal ).margin( tol ) );
        CHECK( healing_rate_at_health( dummy, -100, sleep_rest ) == Approx( 0.5f * normal ).margin( tol ) );
        CHECK( healing_rate_at_health( dummy, -150,
                                       sleep_rest ) == Approx( 0.25f * normal ).margin( tol ) );
        // Worst possible health: no healing even while asleep!
        CHECK( healing_rate_at_health( dummy, -200, sleep_rest ) == Approx( 0.0f * normal ).margin( tol ) );
    }

    SECTION( "good health improves healing when asleep" ) {
        // Good health increases healing linearly
        CHECK( healing_rate_at_health( dummy, 50, sleep_rest ) == Approx( 1.25f * normal ).margin( tol ) );
        CHECK( healing_rate_at_health( dummy, 100, sleep_rest ) == Approx( 1.5f * normal ).margin( tol ) );
        CHECK( healing_rate_at_health( dummy, 150, sleep_rest ) == Approx( 1.75f * normal ).margin( tol ) );
        // Best possible health: double healing!
        CHECK( healing_rate_at_health( dummy, 200, sleep_rest ) == Approx( 2.0f * normal ).margin( tol ) );
    }

    SECTION( "health has no effect on healing while awake" ) {
        // Note: This only considers baseline healing without treatment or traits
        CHECK( healing_rate_at_health( dummy, -200, awake_rest ) == zero );
        CHECK( healing_rate_at_health( dummy, -100, awake_rest ) == zero );
        CHECK( healing_rate_at_health( dummy, 0, awake_rest ) == zero );
        CHECK( healing_rate_at_health( dummy, 100, awake_rest ) == zero );
        CHECK( healing_rate_at_health( dummy, 200, awake_rest ) == zero );
    }
}

// Helper functions below are for `healing_rate_medicine` using bandaged/disinfected effects,
// using a local avatar instance to avoid any cross-contamination. Tests may be contagious!

// Return `healing_rate_medicine` for an untreated body part at a given rest quality
static float untreated_rate( const std::string &bp_name, const float rest_quality )
{
    avatar dummy;
    return dummy.healing_rate_medicine( rest_quality, bodypart_id( bp_name ) );
}

// Return `healing_rate_medicine` for a `bandaged` body part at a given rest quality
static float bandaged_rate( const std::string &bp_name, const float rest_quality )
{
    avatar dummy;
    const bodypart_id &bp = bodypart_id( bp_name );
    dummy.add_effect( effect_bandaged, 1_turns, bp->token );
    return dummy.healing_rate_medicine( rest_quality, bp );
}

// Return `healing_rate_medicine` for a `disinfected` body part at a given rest quality
static float disinfected_rate( const std::string &bp_name, const float rest_quality )
{
    avatar dummy;
    const bodypart_id &bp = bodypart_id( bp_name );
    dummy.add_effect( effect_disinfected, 1_turns, bp->token );
    return dummy.healing_rate_medicine( rest_quality, bp );
}

// Return `healing_rate_medicine` for a `bandaged` AND `disinfected` body part at a given rest quality
static float together_rate( const std::string &bp_name, const float rest_quality )
{
    avatar dummy;
    const bodypart_id &bp = bodypart_id( bp_name );
    dummy.add_effect( effect_bandaged, 1_turns, bp->token );
    dummy.add_effect( effect_disinfected, 1_turns, bp->token );
    return dummy.healing_rate_medicine( rest_quality, bp );
}

// Healing effects of bandages and disinfectant are calculated by Character::healing_rate_medicine.
//
// Without either treatment, healing_rate_medicine gives no healing for any body part, awake or asleep.
// Bandages or disinfectant alone will heal by 1-3 HP per day while awake, depending on body part.
// Both bandages and disinfectant together multiply this effect, giving 4-12 HP per day while awake.
// The torso gets the most benefit from treatment, while the head gets the least benefit.
// Healing rates from treatment are doubled while sleeping.
//
TEST_CASE( "healing_rate_medicine with bandages and/or disinfectant", "[heal][bandage][disinfect]" )
{
    // There are no healing effects from medicine if no medicine has been applied.
    SECTION( "no bandages or disinfectant" ) {
        SECTION( "awake" ) {
            CHECK( untreated_rate( "head", awake_rest ) == zero );
            CHECK( untreated_rate( "arm_l", awake_rest ) == zero );
            CHECK( untreated_rate( "arm_r", awake_rest ) == zero );
            CHECK( untreated_rate( "leg_l", awake_rest ) == zero );
            CHECK( untreated_rate( "leg_r", awake_rest ) == zero );
            CHECK( untreated_rate( "torso", awake_rest ) == zero );
        }

        SECTION( "asleep" ) {
            CHECK( untreated_rate( "head", sleep_rest ) == zero );
            CHECK( untreated_rate( "arm_l", sleep_rest ) == zero );
            CHECK( untreated_rate( "arm_r", sleep_rest ) == zero );
            CHECK( untreated_rate( "leg_l", sleep_rest ) == zero );
            CHECK( untreated_rate( "leg_r", sleep_rest ) == zero );
            CHECK( untreated_rate( "torso", sleep_rest ) == zero );
        }
    }

    // Bandages heal 1-3 HP per day while awake, 2-6 HP per day while asleep
    SECTION( "bandages only" ) {
        SECTION( "awake" ) {
            CHECK( bandaged_rate( "head", awake_rest ) == Approx( 1.0f * hp_per_day ) );
            CHECK( bandaged_rate( "arm_l", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( bandaged_rate( "arm_r", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( bandaged_rate( "leg_l", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( bandaged_rate( "leg_r", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( bandaged_rate( "torso", awake_rest ) == Approx( 3.0f * hp_per_day ) );
        }

        SECTION( "asleep" ) {
            CHECK( bandaged_rate( "head", sleep_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( bandaged_rate( "arm_l", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( bandaged_rate( "arm_r", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( bandaged_rate( "leg_l", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( bandaged_rate( "leg_r", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( bandaged_rate( "torso", sleep_rest ) == Approx( 6.0f * hp_per_day ) );
        }
    }

    // Disinfectant heals 1-3 HP per day while awake, 2-6 HP per day while asleep
    SECTION( "disinfectant only" ) {
        SECTION( "awake" ) {
            CHECK( disinfected_rate( "head", awake_rest ) == Approx( 1.0f * hp_per_day ) );
            CHECK( disinfected_rate( "arm_l", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( disinfected_rate( "arm_r", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( disinfected_rate( "leg_l", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( disinfected_rate( "leg_r", awake_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( disinfected_rate( "torso", awake_rest ) == Approx( 3.0f * hp_per_day ) );
        }

        SECTION( "asleep" ) {
            CHECK( disinfected_rate( "head", sleep_rest ) == Approx( 2.0f * hp_per_day ) );
            CHECK( disinfected_rate( "arm_l", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( disinfected_rate( "arm_r", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( disinfected_rate( "leg_l", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( disinfected_rate( "leg_r", sleep_rest ) == Approx( 4.0f * hp_per_day ) );
            CHECK( disinfected_rate( "torso", sleep_rest ) == Approx( 6.0f * hp_per_day ) );
        }
    }

    // Combined, healing is 2.5-7.5 HP per day while awake, 5-15 HP per day while asleep
    SECTION( "bandages and disinfectant together" ) {
        SECTION( "awake" ) {
            CHECK( together_rate( "head", awake_rest ) == Approx( 2.5f * hp_per_day ) );
            CHECK( together_rate( "arm_l", awake_rest ) == Approx( 5.0f * hp_per_day ) );
            CHECK( together_rate( "arm_r", awake_rest ) == Approx( 5.0f * hp_per_day ) );
            CHECK( together_rate( "leg_l", awake_rest ) == Approx( 5.0f * hp_per_day ) );
            CHECK( together_rate( "leg_r", awake_rest ) == Approx( 5.0f * hp_per_day ) );
            CHECK( together_rate( "torso", awake_rest ) == Approx( 7.5f * hp_per_day ) );
        }

        SECTION( "asleep" ) {
            CHECK( together_rate( "head", sleep_rest ) == Approx( 5.0f * hp_per_day ) );
            CHECK( together_rate( "arm_l", sleep_rest ) == Approx( 10.0f * hp_per_day ) );
            CHECK( together_rate( "arm_r", sleep_rest ) == Approx( 10.0f * hp_per_day ) );
            CHECK( together_rate( "leg_l", sleep_rest ) == Approx( 10.0f * hp_per_day ) );
            CHECK( together_rate( "leg_r", sleep_rest ) == Approx( 10.0f * hp_per_day ) );
            CHECK( together_rate( "torso", sleep_rest ) == Approx( 15.0f * hp_per_day ) );
        }
    }
}

