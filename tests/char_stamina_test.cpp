#include "avatar.h"
#include "character.h"
#include "game.h"
#include "options.h"

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

TEST_CASE( "modify character stamina", "[stamina][modify]" )
{
    player &dummy = g->u;
    clear_character( dummy );
    REQUIRE_FALSE( dummy.is_npc() );
    REQUIRE_FALSE( dummy.has_effect( efftype_id( "winded" ) ) );

    GIVEN( "character has less than full stamina" ) {
        int lost_stamina = dummy.get_stamina_max() / 2;
        dummy.set_stamina( dummy.get_stamina_max() - lost_stamina );
        REQUIRE( dummy.get_stamina() + lost_stamina == dummy.get_stamina_max() );

        WHEN( "they regain only part of their lost stamina" ) {
            dummy.mod_stamina( lost_stamina / 2 );

            THEN( "stamina is less than maximum" ) {
                CHECK( dummy.get_stamina() < dummy.get_stamina_max() );
            }
        }

        WHEN( "they regain all of their lost stamina" ) {
            dummy.mod_stamina( lost_stamina );

            THEN( "stamina is at maximum" ) {
                CHECK( dummy.get_stamina() == dummy.get_stamina_max() );
            }
        }

        WHEN( "they regain more stamina than they lost" ) {
            dummy.mod_stamina( lost_stamina + 1 );

            THEN( "stamina is at maximum" ) {
                CHECK( dummy.get_stamina() == dummy.get_stamina_max() );
            }
        }

        WHEN( "they lose only part of their remaining stamina" ) {
            dummy.mod_stamina( -( dummy.get_stamina() / 2 ) );

            THEN( "stamina is above zero" ) {
                CHECK( dummy.get_stamina() > 0 );

                AND_THEN( "they do not become winded" ) {
                    REQUIRE_FALSE( dummy.has_effect( efftype_id( "winded" ) ) );
                }
            }
        }

        WHEN( "they lose all of their remaining stamina" ) {
            dummy.mod_stamina( -( dummy.get_stamina() ) );

            THEN( "stamina is at zero" ) {
                CHECK( dummy.get_stamina() == 0 );

                AND_THEN( "they do not become winded" ) {
                    REQUIRE_FALSE( dummy.has_effect( efftype_id( "winded" ) ) );
                }
            }
        }

        WHEN( "they lose more stamina than they have remaining" ) {
            dummy.mod_stamina( -( dummy.get_stamina() + 1 ) );

            THEN( "stamina is at zero" ) {
                CHECK( dummy.get_stamina() == 0 );

                AND_THEN( "they become winded" ) {
                    REQUIRE( dummy.has_effect( efftype_id( "winded" ) ) );
                }
            }
        }
    }
}

// burn_move_stamina (MODIFIES stamina)
// - Scaled by overburden percentage
// - Modified by bionic muscles
// - Running scales by 7x
// - Modifies stamina based on stamina_move_cost_modifier
// - Applies pain if overburdened with no stamina or BADBACK trait
//
TEST_CASE( "burn stamina for movement", "[stamina][burn][move]" )
{
    player &dummy = g->u;

    // Game-balance configured rate of stamina burned per move
    int burn_rate = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );
    int before_stam = 0;
    int after_stam = 0;

    GIVEN( "player is not overburdened" ) {
        clear_character( dummy );
        REQUIRE( dummy.weight_carried() < dummy.weight_capacity() );

        WHEN( "walking" ) {
            dummy.set_movement_mode( CMM_WALK );
            REQUIRE( dummy.movement_mode_is( CMM_WALK ) );

            before_stam = dummy.get_stamina();
            REQUIRE( before_stam == dummy.get_stamina_max() );

            THEN( "stamina cost is the normal rate per turn" ) {
                dummy.burn_move_stamina( to_moves<int>( 10_turns ) );
                after_stam = dummy.get_stamina();
                CHECK( after_stam == before_stam - 10 * burn_rate );
            }
        }

        WHEN( "crouching" ) {
            dummy.set_movement_mode( CMM_CROUCH );
            REQUIRE( dummy.movement_mode_is( CMM_CROUCH ) );

            before_stam = dummy.get_stamina();
            REQUIRE( before_stam == dummy.get_stamina_max() );

            THEN( "stamina cost is 1/2 the normal rate per turn" ) {
                dummy.burn_move_stamina( to_moves<int>( 10_turns ) );
                after_stam = dummy.get_stamina();
                CHECK( after_stam == before_stam - 5 * burn_rate );
            }
        }

        WHEN( "running" ) {
            dummy.set_movement_mode( CMM_RUN );
            REQUIRE( dummy.movement_mode_is( CMM_RUN ) );

            before_stam = dummy.get_stamina();
            REQUIRE( before_stam == dummy.get_stamina_max() );

            THEN( "stamina cost is 14 times the normal rate per turn" ) {
                dummy.burn_move_stamina( to_moves<int>( 10_turns ) );
                after_stam = dummy.get_stamina();
                CHECK( after_stam == before_stam - 140 * burn_rate );
            }
        }
    }
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
