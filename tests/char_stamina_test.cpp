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
//
// stamina_move_cost_modifier
// - Both walk and run speed drop to half their maximums as stamina approaches 0
// - Modifier is 2x for running
// - Modifier is 1/2x for crouching
//
// burn_move_stamina (MODIFIES stamina)
// - Scaled by overburden percentage
// - Modified by bionic muscles
// - Running scales by 7x
// - Modifies stamina based on stamina_move_cost_modifier
// - Applies pain if overburdened with no stamina or BADBACK trait
//
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
//
// effect_winded
//
TEST_CASE( "stamina movement cost", "[stamina][move_cost]" )
{
    player &dummy = g->u;
    clear_character( dummy );

    GIVEN( "character is walking" ) {
        dummy.set_movement_mode( CMM_WALK );
        REQUIRE( dummy.movement_mode_is( CMM_WALK ) );

        THEN( "100%% movement speed at full stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

            CHECK( dummy.stamina_move_cost_modifier() == 1.0 );
        }

        WHEN( "75%% movement speed at half stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 2 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 2 );

            CHECK( dummy.stamina_move_cost_modifier() == 0.75 );
        }

        WHEN( "50%% movement speed at zero stamina" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            CHECK( dummy.stamina_move_cost_modifier() == 0.5 );
        }
    }

    WHEN( "character is running" ) {
        // Set max stamina to ensure they can run
        dummy.set_stamina( dummy.get_stamina_max() );
        dummy.set_movement_mode( CMM_RUN );
        REQUIRE( dummy.movement_mode_is( CMM_RUN ) );

        THEN( "200%% movement speed at full stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

            CHECK( dummy.stamina_move_cost_modifier() == 2.0 );
        }

        THEN( "150%% movement speed at half stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() / 2 );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() / 2 );

            CHECK( dummy.stamina_move_cost_modifier() == 1.5 );
        }

        THEN( "50%% movement speed at zero stamina" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            CHECK( dummy.stamina_move_cost_modifier() == 0.50 );
        }
    }

    WHEN( "character is crouching" ) {
        dummy.set_movement_mode( CMM_CROUCH );
        REQUIRE( dummy.movement_mode_is( CMM_CROUCH ) );

        THEN( "50%% movement speed at full stamina" ) {
            dummy.set_stamina( dummy.get_stamina_max() );
            REQUIRE( dummy.get_stamina() == dummy.get_stamina_max() );

            CHECK( dummy.stamina_move_cost_modifier() == 0.5 );
        }

        THEN( "25%% movement speed at zero stamina" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            CHECK( dummy.stamina_move_cost_modifier() == 0.25 );
        }
    }
}

