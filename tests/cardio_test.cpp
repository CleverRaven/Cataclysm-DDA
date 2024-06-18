#include "avatar.h"
#include "cata_catch.h"
#include "game.h"
#include "options.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

// Cardio Fitness
// --------------
// The hidden "Cardio Fitness" stat returned by Character::get_cardiofit() is the main factor in
// calculations related to cardiovascular health, including weariness, stamina, and stamina regen,
// more or less as described in the original issue #44370 "Rule #1. CARDIO".
//
// For player characters, cardio fitness is derived from a combination of several attributes:
//
//   (
//       [ Trait modifiers ]          // Good/bad cardio mutations like Languorous and Hyperactive
//                                    // Trait modifers defaults to 1.0f, so it is the baseline
//     + [ Health ] / 1000            // Hidden health modifier, -200..+200
//     + [ Proficiency modifiers ]    // NOT IMPLEMENTED
//     + [ Athletics skill ] / 100    // Formerly swimming skill, now more versatile and valuable
//   ) * [ Cardio_Accumulator ]       // Adjustment based on daily activity, starting at 1000
//
// For NPCs (having no cardio), it's 2000.
//
// Important functions:
// - Character::get_stamina
// - Character::set_stamina
// - Character::get_stamina_max
// - Character::get_cardiofit
// - Character::get_cardio_acc
// - Character::set_cardio_acc
// - avatar::update_cardio_acc <-- Critical

static const move_mode_id move_mode_run( "run" );

static const skill_id skill_swimming( "swimming" );

static const ter_str_id ter_t_pavement( "t_pavement" );

// Base cardio for default character
static const int base_cardio = 1000;
// Base stamina
// MAX_STAMINA_BASE + CARDIOFIT_STAMINA_SCALING * base_cardio == 3500 + 5*1000
static const int base_stamina = 8500;

// Ensure the configured options from game_balance.json are what the tests assume they are
static void verify_default_cardio_options()
{
    const int max_stamina_base = get_option<int>( "PLAYER_MAX_STAMINA_BASE" );
    const int cardiofit_stamina_scaling = get_option<int>( "PLAYER_CARDIOFIT_STAMINA_SCALING" );
    REQUIRE( max_stamina_base == 3500 );
    REQUIRE( cardiofit_stamina_scaling == 5 );
}

// Count the number of steps (tiles) until character runs out of stamina or becomes winded.
static int running_steps( Character &they, const ter_str_id &terrain = ter_t_pavement )
{
    map &here = get_map();
    // Please take off your shoes when entering, and no NPCs allowed
    REQUIRE_FALSE( they.is_wearing_shoes() );
    REQUIRE_FALSE( they.is_npc() );
    // You put your left foot in, you put your right foot in
    const tripoint left = they.pos();
    const tripoint right = left + tripoint_east;
    // You ensure two tiles of terrain to hokey-pokey in
    here.ter_set( left, terrain );
    here.ter_set( right, terrain );
    REQUIRE( here.ter( left ) == terrain );
    REQUIRE( here.ter( right ) == terrain );
    // Count how many steps (1-tile moves) it takes to become winded
    int steps = 0;
    int turns = 0;
    const int STOP_STEPS = 1000; // Safe exit in case of Superman
    // Track changes to moves and stamina
    int last_moves = they.get_speed();
    int last_stamina = they.get_stamina_max();
    // Take a deep breath and start running
    they.set_moves( last_moves );
    they.set_stamina( last_stamina );
    they.set_movement_mode( move_mode_run );
    // Run as long as possible
    while( they.can_run() && steps < STOP_STEPS ) {
        // Step right on even steps, left on odd steps
        if( steps % 2 == 0 ) {
            REQUIRE( they.pos() == left );
            REQUIRE( g->walk_move( right, false, false ) );
        } else {
            REQUIRE( they.pos() == right );
            REQUIRE( g->walk_move( left, false, false ) );
        }
        ++steps;

        // Ensure moves are decreasing, or else a turn will never pass
        REQUIRE( they.get_moves() < last_moves );
        const int move_cost = last_moves - they.get_moves();
        // When moves run out, one turn has passed
        if( they.get_moves() <= 0 ) {
            // Get "speed" moves back each turn
            they.mod_moves( they.get_speed() );
            calendar::turn += 1_turns;
            turns += 1;

            // Update body for stamina regen
            they.update_body();
            const int stamina_cost = last_stamina - they.get_stamina();
            // Total stamina must also be decreasing; if not, quit
            CAPTURE( turns );
            CAPTURE( steps );
            CAPTURE( move_cost );
            CAPTURE( stamina_cost );
            REQUIRE( they.get_stamina() < last_stamina );
            last_stamina = they.get_stamina();
        }
        last_moves = they.get_moves();
    }
    // Reset to starting position
    they.setpos( left );
    return steps;
}

// Give character a trait, and verify their expected cardio, max stamina, and running distance.
static void check_trait_cardio_stamina_run( Character &they, std::string trait_name,
        const int expect_cardio_fit, const int expect_stamina_max, const int expect_run_tiles )
{
    clear_avatar();
    if( trait_name.empty() ) {
        trait_name = "NONE";
    } else {
        set_single_trait( they, trait_name );
    }
    //it's possible suddenly becoming huge or tiny will cause BMI issues so we reset stored kcal to new normal
    they.set_stored_kcal( they.get_healthy_kcal() );
    GIVEN( "trait: " + trait_name ) {
        CHECK( they.get_cardiofit() == Approx( expect_cardio_fit ).margin( 2 ) );
        CHECK( they.get_stamina_max() == Approx( expect_stamina_max ).margin( 5 ) );
        //CHECK( they.get_stamina_max() == Approx( 3500 + 3 * expect_cardio_fit ).margin( 2 ) );
        CHECK( running_steps( they ) == Approx( expect_run_tiles ).margin( 3 ) );
    }
}

// Baseline character with default attributes and no traits
TEST_CASE( "base_cardio", "[cardio][base]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map();
    clear_avatar();

    // Ensure no initial effects that would affect cardio
    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( static_cast<int>( they.get_skill_level( skill_swimming ) ) == 0 );
    // Ensure starting cardio are what we expect
    REQUIRE( they.get_cardiofit() == 1000 );
    // Ensure that the character has the correct leg configuration
    they.recalc_limb_energy_usage();
    REQUIRE( they.get_working_leg_count() == 2 );
    REQUIRE( they.get_legs_power_use() == 0 );
    REQUIRE( they.get_legs_stam_mult() == 1 );

    SECTION( "Base character with no traits" ) {
        // pre-Cardio, could run 96 steps
        // post-Cardio, can run 84 steps in-game, test case reached 87
        // correctly counting moves/steps instead of turns, test case reaches 83
        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 83 );
    }
}

// Trait Modifiers
// ---------------
// Mutations/traits can influence cardio fitness level in a two ways:
//
// Some traits affect cardio fitness directly:
//
// - CARDIO_MULTIPLIER: Multiplies maximum cardio
//   - Languorous: Bad cardio, less total stamina
//   - Indefatigable, Hyperactive: Good cardio, more total stamina
//
// Some traits affect stamina regen and total running distance without affecting cardio:
//
// - STAMINA_REGEN_MOD
//   - Fast Metabolism, Persistence Hunter: Increased stamina regeneration
//
TEST_CASE( "cardio_is_and_is_not_affected_by_certain_traits", "[cardio][traits]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map();
    clear_avatar();

    // Ensure no initial effects that would affect cardio
    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( static_cast<int>( they.get_skill_level( skill_swimming ) ) == 0 );
    // Ensure starting cardio are what we expect
    REQUIRE( they.get_cardiofit() == 1000 );
    // Ensure that the character has the correct leg configuration
    they.recalc_limb_energy_usage();
    REQUIRE( they.get_working_leg_count() == 2 );
    REQUIRE( they.get_legs_power_use() == 0 );
    REQUIRE( they.get_legs_stam_mult() == 1 );

    SECTION( "Base character with no traits" ) {
        // pre-Cardio, could run 96 steps
        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 81 );
    }

    // Body Size has no effect on running distance.
    SECTION( "Traits affecting body size" ) {
        check_trait_cardio_stamina_run( they, "SMALL2", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "SMALL", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "LARGE", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "HUGE", base_cardio, base_stamina, 81 );
    }

    SECTION( "Traits with CARDIO_MULTIPLIER" ) {
        // These traits were formerly implemented by max_stamina_modifier, which multiplied
        // maximum stamina. Now that cardio fitness is actually implemented, these traits
        // directly affect total cardio fitness, and thus maximum stamina (and running distance).
        // Languorous
        check_trait_cardio_stamina_run( they, "BADCARDIO", 0.7 * base_cardio, 7000, 66 );
        // Indefatigable
        check_trait_cardio_stamina_run( they, "GOODCARDIO", 1.3 * base_cardio, 10000, 103 );
        // Hyperactive
        check_trait_cardio_stamina_run( they, "GOODCARDIO2", 1.6 * base_cardio, 11500, 121 );
    }

    SECTION( "Traits with metabolism_modifier AND STAMINA_REGEN_MOD" ) {
        // Fast Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER", base_cardio, base_stamina, 83 );
        // Very Fast Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER2", base_cardio, base_stamina, 85 );
        // Extreme Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER3", base_cardio, base_stamina, 88 );
    }

    SECTION( "Traits with ONLY STAMINA_REGEN_MOD" ) {
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER", base_cardio, base_stamina, 83 );
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER2", base_cardio, base_stamina, 84 );
    }

    SECTION( "Traits with ONLY metabolism_modifier" ) {
        check_trait_cardio_stamina_run( they, "COLDBLOOD", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD2", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD3", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "COLDBLOOD4", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "LIGHTEATER", base_cardio, base_stamina, 81 );
        check_trait_cardio_stamina_run( they, "MET_RAT", base_cardio, base_stamina, 81 );
    }
}

TEST_CASE( "cardio_affects_stamina_regeneration", "[cardio][stamina]" )
{
    // With baseline cardio, stamina regen is X
    // With low cardio, stamina regen is X--
    // With high cardio, stamina regen is X++
}

TEST_CASE( "cardio_affects_weariness", "[cardio][weariness]" )
{
    // Weariness threshold is 1/2 cardio
}

TEST_CASE( "cardio_is_affected_by_activity_level_each_day", "[cardio][activity]" )
{
    // When activity increases, cardio goes up
    // When activity decreases, cardio goes down
    // When activity stays the same, cardio stays the same

    // Given a starting character
    // When they get no exercise for a week
    // Then their cardio should decrease slightly
    // When they get moderate exercise for a week
    // Then their cardio should increase slightly
}

TEST_CASE( "cardio_is_not_affected_by_character_height", "[cardio][height]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    REQUIRE( they.size_class == creature_size::medium );

    SECTION( "No difference in cardio between heights" ) {
        they.set_base_height( Character::default_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_base_height( Character::max_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_base_height( Character::min_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio );
    }
}

TEST_CASE( "cardio_is_affected_by_character_weight", "[cardio][weight]" )
{
    // Underweight, overweight
}

TEST_CASE( "cardio_is_affected_by_character_health", "[cardio][health]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Hidden health stat adds directly to cardio fitness" ) {
        they.set_lifestyle( 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_lifestyle( 200 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.20f ) );
        they.set_lifestyle( -200 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 0.60f ) );
    }
}

TEST_CASE( "cardio_is_affected_by_athletics_skill", "[cardio][athletics]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Athletics skill adds 10 per level to cardio fitness" ) {
        they.set_skill_level( skill_swimming, 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_skill_level( skill_swimming, 1 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.01f ) );
        they.set_skill_level( skill_swimming, 2 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.02f ) );
        they.set_skill_level( skill_swimming, 3 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.03f ) );
        they.set_skill_level( skill_swimming, 4 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.04f ) );
        they.set_skill_level( skill_swimming, 5 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.05f ) );
        they.set_skill_level( skill_swimming, 6 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.06f ) );
        they.set_skill_level( skill_swimming, 7 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.07f ) );
        they.set_skill_level( skill_swimming, 8 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.08f ) );
        they.set_skill_level( skill_swimming, 9 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.09f ) );
        they.set_skill_level( skill_swimming, 10 );
        CHECK( they.get_cardiofit() == static_cast<int>( base_cardio * 1.10f ) );
    }
}
