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
//     [ 1/2 BMR ]                    // Basal Metabolic Rate, varies with activity and body size
//     + [ Health ]                   // Hidden health modifier, -200..+200
//     + [ Proficiency modifiers ]    // NOT IMPLEMENTED
//     + [ Cardio_Accumulator ]       // Adjustment based on daily activity, starting at 1/2 BMR
//     + 10 * [ Athletics skill ]     // Formerly swimming skill, now more versatile and valuable
//   ) * [ Trait modifiers ]          // Good/bad cardio mutations like Languorous and Hyperactive
//
// For NPCs (having no cardio), the formula is simply 2 * BMR.
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

// Base BMR for default character
static const int base_bmr = 1738;
static const int base_cardio = base_bmr;
// Base stamina
// MAX_STAMINA_BASE + CARDIOFIT_STAMINA_SCALING * base_bmr == 3500 + 3*1738
static const int base_stamina = 8714;

// Ensure the configured options from game_balance.json are what the tests assume they are
static void verify_default_cardio_options()
{
    const int max_stamina_base = get_option<int>( "PLAYER_MAX_STAMINA_BASE" );
    const int cardiofit_stamina_scaling = get_option<int>( "PLAYER_CARDIOFIT_STAMINA_SCALING" );
    REQUIRE( max_stamina_base == 3500 );
    REQUIRE( cardiofit_stamina_scaling == 3 );
}

// Count the number of steps (tiles) until character runs out of stamina or becomes winded.
static int running_steps( Character &they, const ter_id &terrain = t_pavement )
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
    they.moves = last_moves;
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
        REQUIRE( they.moves < last_moves );
        const int move_cost = last_moves - they.moves;
        // When moves run out, one turn has passed
        if( they.moves <= 0 ) {
            // Get "speed" moves back each turn
            they.moves += they.get_speed();
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
        last_moves = they.moves;
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
    GIVEN( "trait: " + trait_name ) {
        CHECK( they.get_cardiofit() == Approx( expect_cardio_fit ).margin( 2 ) );
        CHECK( they.get_stamina_max() == Approx( expect_stamina_max ).margin( 5 ) );
        //CHECK( they.get_stamina_max() == Approx( 3500 + 3 * expect_cardio_fit ).margin( 2 ) );
        CHECK( running_steps( they ) == Approx( expect_run_tiles ).margin( 3 ) );
    }
}


// Baseline character with default attributes and no traits
TEST_CASE( "base cardio", "[cardio][base]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map();
    clear_avatar();

    // Ensure no initial effects that would affect cardio
    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( they.get_skill_level( skill_swimming ) == 0 );
    // Ensure base_bmr and starting cardio are what we expect
    REQUIRE( they.base_bmr() == base_bmr );
    REQUIRE( they.get_cardiofit() == base_cardio );
    REQUIRE( 2 * they.get_cardio_acc() == base_bmr );

    SECTION( "Base character with no traits" ) {
        // pre-Cardio, could run 96 steps
        // post-Cardio, can run 84 steps in-game, test case reached 87
        // correctly counting moves/steps instead of turns, test case reaches 83
        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 83 );
    }
}

// Trait Modifiers
// ---------------
// Mutations/traits can influence cardio fitness level in a few ways:
//
// - Mutation affecting total body size, which affects base BMR and thus cardio
//   - Little / Tiny: Smaller body has lower BMR, less cardio, cannot run as far
//   - Large / Huge: Larger body has higher BMR, more cardio, can run further
// - metabolism_modifier: Affects metabolic_rate_base
//   - Heat Dependent / Cold Blooded: Decreased BMR and cardio, less running
//   - Fast/Rapid/Extreme Metabolism: Increased BMR and cardio, more running
//
// Some traits affect cardio fitness directly:
//
// - cardio_multiplier: Multiplies maximum cardio
//   - Languorous: Bad cardio, less total stamina
//   - Indefatigable, Hyperactive: Good cardio, more total stamina
//
// Some traits affect stamina regen and total running distance without affecting cardio:
//
// - stamina_regen_modifier
//   - Fast Metabolism, Persistence Hunter: Increased stamina regeneration
//
TEST_CASE( "cardio is affected by certain traits", "[cardio][traits]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();

    clear_map();
    clear_avatar();

    // Ensure no initial effects that would affect cardio
    REQUIRE( they.get_lifestyle() == 0 );
    REQUIRE( they.get_skill_level( skill_swimming ) == 0 );
    // Ensure base_bmr and starting cardio are what we expect
    REQUIRE( they.base_bmr() == base_bmr );
    REQUIRE( they.get_cardiofit() == base_cardio );
    REQUIRE( 2 * they.get_cardio_acc() == base_bmr );

    // Values trailing after check //123 are pre-Cardio in-game running distances, a high-level
    // metric for balancing traits relative to how they were before cardio. Some traits were buffed
    // by the change to cardio; the fast metabolism, persistence hunger, and cold-blooded traits got
    // a 20-30% boost in total running distance. The old values are preserved here for possible
    // future rebalancing and comparison.

    SECTION( "Base character with no traits" ) {
        // pre-Cardio, could run 96 steps
        check_trait_cardio_stamina_run( they, "", base_cardio, base_stamina, 83 ); //96
    }

    // Sprint distance for each body size is only slightly different than it was before cardio
    SECTION( "Traits affecting body size" ) {
        // Body size determines BMR, which affects base cardio fitness
        // Pre-Cardio, body size did not affect how many steps you could run
        check_trait_cardio_stamina_run( they, "SMALL2", 1088, 6764, 84 ); //97
        // FIXME: But why the heck can SMALL2 run further than SMALL?
        check_trait_cardio_stamina_run( they, "SMALL", 1376, 7628, 78 ); //97
        check_trait_cardio_stamina_run( they, "LARGE", 2162, 9986, 93 ); //97
        check_trait_cardio_stamina_run( they, "HUGE", 2663, 11489, 106 ); //97
    }

    SECTION( "Traits with cardio_multiplier" ) {
        // These traits were formerly implemented by max_stamina_modifier, which multiplied
        // maximum stamina. Now that cardio fitness is actually implemented, these traits
        // directly affect total cardio fitness, and thus maximum stamina (and running distance).
        // Languorous
        check_trait_cardio_stamina_run( they, "BADCARDIO", 0.7 * base_cardio, 7148, 66 ); //70
        // Indefatigable
        check_trait_cardio_stamina_run( they, "GOODCARDIO", 1.3 * base_cardio, 10277, 103 ); //126
        // Hyperactive
        check_trait_cardio_stamina_run( they, "GOODCARDIO2", 1.6 * base_cardio, 11840, 125 ); //145
    }

    // FIXME: These traits need a significant nerf (-8 to -32) to reach their pre-Cardio balance
    SECTION( "Traits with metabolism_modifier AND stamina_regen_modifier" ) {
        // Fast Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER", 2173, 10019, 95 ); //76
        // Very Fast Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER2", 2608, 11324, 108 ); //87
        // Extreme Metabolism
        check_trait_cardio_stamina_run( they, "HUNGER3", 3477, 13931, 133 ); //107
    }

    // FIXME: These traits need a significant nerf (-20) to reach their pre-Cardio balance
    SECTION( "Traits with ONLY stamina_regen_modifier" ) {
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER", base_cardio, base_stamina, 85 ); //68
        check_trait_cardio_stamina_run( they, "PERSISTENCE_HUNTER2", base_cardio, base_stamina, 86 ); //69
    }

    // FIXME: These traits need a significant nerf (-20) to reach their pre-Cardio balance
    SECTION( "Traits with ONLY metabolism_modifier" ) {
        check_trait_cardio_stamina_run( they, "COLDBLOOD", 1449, 7847, 78 ); //63
        check_trait_cardio_stamina_run( they, "COLDBLOOD2", 1304, 7412, 77 ); //62
        check_trait_cardio_stamina_run( they, "COLDBLOOD3", 1304, 7412, 81 ); //62
        check_trait_cardio_stamina_run( they, "COLDBLOOD4", 1304, 7412, 81 ); //62
        check_trait_cardio_stamina_run( they, "LIGHTEATER", 1449, 7847, 78 ); //63
        check_trait_cardio_stamina_run( they, "MET_RAT", 2028, 9584, 90 ); //72
    }
}

TEST_CASE( "cardio affects stamina regeneration", "[cardio][stamina]" )
{
    // With baseline cardio, stamina regen is X
    // With low cardio, stamina regen is X--
    // With high cardio, stamina regen is X++
}

TEST_CASE( "cardio affects weariness", "[cardio][weariness]" )
{
    // Weariness threshold is 1/2 cardio
}

TEST_CASE( "cardio is affected by activity level each day", "[cardio][activity]" )
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

TEST_CASE( "cardio is affected by character height", "[cardio][height]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    REQUIRE( they.size_class == creature_size::medium );

    SECTION( "Within a size class, greater height means greater cardio" ) {
        they.set_base_height( Character::default_height( they.size_class ) );
        CHECK( they.get_cardiofit() == base_cardio ); //1739
        they.set_base_height( Character::max_height( they.size_class ) );
        CHECK( they.get_cardiofit() == Approx( base_cardio + 196 ).margin( 5 ) ); //1935
        they.set_base_height( Character::min_height( they.size_class ) );
        CHECK( they.get_cardiofit() == Approx( base_cardio - 214 ).margin( 5 ) ); //1525
    }
}

TEST_CASE( "cardio is affected by character weight", "[cardio][weight]" )
{
    // Underweight, overweight
}

TEST_CASE( "cardio is affected by character health", "[cardio][health]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Hidden health stat adds directly to cardio fitness" ) {
        they.set_lifestyle( 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_lifestyle( 200 );
        CHECK( they.get_cardiofit() == base_cardio + 200 );
        they.set_lifestyle( -200 );
        CHECK( they.get_cardiofit() == base_cardio - 200 );
    }
}

TEST_CASE( "cardio is affected by athletics skill", "[cardio][athletics]" )
{
    verify_default_cardio_options();
    Character &they = get_player_character();
    clear_avatar();

    SECTION( "Athletics skill adds 10 per level to cardio fitness" ) {
        they.set_skill_level( skill_swimming, 0 );
        CHECK( they.get_cardiofit() == base_cardio );
        they.set_skill_level( skill_swimming, 1 );
        CHECK( they.get_cardiofit() == base_cardio + 10 );
        they.set_skill_level( skill_swimming, 2 );
        CHECK( they.get_cardiofit() == base_cardio + 20 );
        they.set_skill_level( skill_swimming, 3 );
        CHECK( they.get_cardiofit() == base_cardio + 30 );
        they.set_skill_level( skill_swimming, 4 );
        CHECK( they.get_cardiofit() == base_cardio + 40 );
        they.set_skill_level( skill_swimming, 5 );
        CHECK( they.get_cardiofit() == base_cardio + 50 );
        they.set_skill_level( skill_swimming, 6 );
        CHECK( they.get_cardiofit() == base_cardio + 60 );
        they.set_skill_level( skill_swimming, 7 );
        CHECK( they.get_cardiofit() == base_cardio + 70 );
        they.set_skill_level( skill_swimming, 8 );
        CHECK( they.get_cardiofit() == base_cardio + 80 );
        they.set_skill_level( skill_swimming, 9 );
        CHECK( they.get_cardiofit() == base_cardio + 90 );
        they.set_skill_level( skill_swimming, 10 );
        CHECK( they.get_cardiofit() == base_cardio + 100 );
    }
}
