#include "avatar.h"
#include "character.h"
#include "game.h"

#include "catch/catch.hpp"
#include "player_helpers.h"

// options:
// PLAYER_MAX_STAMINA (10000)
// PLAYER_BASE_STAMINA_REGEN_RATE (20)
// PLAYER_BASE_STAMINA_BURN_RATE (15)

// Functions in character.cpp to cover:
//
// get_stamina_max
// - Start with PLAYER_MAX_STAMINA
// - Multiply by Character::mutation_value( "max_stamina_modifier" )
//
// mod_stamina
// - Modifies stamina by positive or negative amount
// - Adds effect_winded for 10 turns if stamina < 0



// Return `stamina_move_cost_modifier` in the given move_mode with [0,1] stamina remaining
float move_cost_mod( player &dummy, character_movemode move_mode, float stamina_proportion = 1.0 )
{
    clear_character( dummy );

    dummy.set_movement_mode( move_mode );
    REQUIRE( dummy.movement_mode_is( move_mode ) );

    int new_stamina = static_cast<int>( stamina_proportion * dummy.get_stamina_max() );

    dummy.set_stamina( new_stamina );
    REQUIRE( dummy.get_stamina() == new_stamina );

    return dummy.stamina_move_cost_modifier();
}

TEST_CASE( "stamina movement cost modifier", "[stamina][cost]" )
{
    player &dummy = g->u;

    SECTION( "running cost is double walking cost for the same stamina level" ) {
        CHECK( move_cost_mod( dummy, CMM_RUN, 1.0 ) == 2 * move_cost_mod( dummy, CMM_WALK, 1.0 ) );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.5 ) == 2 * move_cost_mod( dummy, CMM_WALK, 0.5 ) );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.0 ) == 2 * move_cost_mod( dummy, CMM_WALK, 0.0 ) );
    }

    SECTION( "walking cost is double crouching cost for the same stamina level" ) {
        CHECK( move_cost_mod( dummy, CMM_WALK, 1.0 ) == 2 * move_cost_mod( dummy, CMM_CROUCH, 1.0 ) );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.5 ) == 2 * move_cost_mod( dummy, CMM_CROUCH, 0.5 ) );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.0 ) == 2 * move_cost_mod( dummy, CMM_CROUCH, 0.0 ) );
    }

    SECTION( "running cost goes from 2.0 to 1.0 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, CMM_RUN, 1.00 ) == 2.00 );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.75 ) == 1.75 );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.50 ) == 1.50 );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.25 ) == 1.25 );
        CHECK( move_cost_mod( dummy, CMM_RUN, 0.00 ) == 1.00 );
    }

    SECTION( "walking cost goes from 1.0 to 0.5 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, CMM_WALK, 1.00 ) == 1.000 );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.75 ) == 0.875 );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.50 ) == 0.750 );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.25 ) == 0.625 );
        CHECK( move_cost_mod( dummy, CMM_WALK, 0.00 ) == 0.500 );
    }

    SECTION( "crouching cost goes from 0.5 to 0.25 as stamina goes to zero" ) {
        CHECK( move_cost_mod( dummy, CMM_CROUCH, 1.00 ) == 0.5000 );
        CHECK( move_cost_mod( dummy, CMM_CROUCH, 0.75 ) == 0.4375 );
        CHECK( move_cost_mod( dummy, CMM_CROUCH, 0.50 ) == 0.3750 );
        CHECK( move_cost_mod( dummy, CMM_CROUCH, 0.25 ) == 0.3125 );
        CHECK( move_cost_mod( dummy, CMM_CROUCH, 0.00 ) == 0.2500 );
    }
}

// burn_move_stamina (MODIFIES stamina)
// - Scaled by overburden percentage
// - Modified by bionic muscles
// - Running scales by 7x
// - Modifies stamina based on stamina_move_cost_modifier
// - Applies pain if overburdened with no stamina or BADBACK trait
//
TEST_CASE( "burning stamina", "[stamina][burn]" )
{
}


// update_stamina (REFRESHES stamina status)
// - Considers PLAYER_BASE_STAMINA_REGEN_RATE
// - Multiplies by mutation_value( "stamina_regen_modifier" )
// (from comments):
// - Mouth encumbrance interferes, even with mutated stamina
// - Stim recovers stamina
// - "Affect(sic) it less near 0 and more near full"
// - Negative stim kill at -200 (???)
// - At -100 stim it inflicts -20 malus to regen (???)
// - at 100% stamina, effectively countering stamina gain of default 20 (???)
// - at 50% stamina it is -10 (50%), cuts by 25% at 25% stamina (???)
// - If "bio_gills" and enough power, "the effective recovery is up to 5x default" (???)
// !!! calls mod_stamina with roll_remainder (RNG) for final result, then caps it at max
//
TEST_CASE( "update stamina", "[stamina][update]" )
{
}
