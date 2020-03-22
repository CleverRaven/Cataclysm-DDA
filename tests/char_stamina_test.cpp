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


// Return `stamina_move_cost_modifier` in the given move_mode with [0,1] stamina remaining
static float move_cost_mod( player &dummy, character_movemode move_mode,
                            float stamina_proportion = 1.0 )
{
    clear_character( dummy );
    dummy.remove_effect( efftype_id( "winded" ) );

    if( move_mode == CMM_RUN ) {
        REQUIRE( dummy.can_run() );
    }

    dummy.set_movement_mode( move_mode );
    REQUIRE( dummy.movement_mode_is( move_mode ) );

    int new_stamina = static_cast<int>( stamina_proportion * dummy.get_stamina_max() );

    dummy.set_stamina( new_stamina );
    REQUIRE( dummy.get_stamina() == new_stamina );

    return dummy.stamina_move_cost_modifier();
}

// Return amount of stamina burned per turn by `burn_move_stamina` in the given movement mode.
static int actual_burn_rate( player &dummy, character_movemode move_mode )
{
    // Set starting stamina to max to ensure enough left for 10 turns
    dummy.set_stamina( dummy.get_stamina_max() );
    dummy.remove_effect( efftype_id( "winded" ) );

    if( move_mode == CMM_RUN ) {
        REQUIRE( dummy.can_run() );
    }

    dummy.set_movement_mode( move_mode );
    REQUIRE( dummy.movement_mode_is( move_mode ) );

    int before_stam = dummy.get_stamina();
    dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
    int after_stam = dummy.get_stamina();
    REQUIRE( before_stam > after_stam );

    return before_stam - after_stam;
}

// Burden the player with a given proportion [0.0 .. inf) of weight
static void burden_player( player &dummy, float burden_proportion )
{
    clear_character( dummy, false );
    units::mass capacity = dummy.weight_capacity();

    // Add gold (5g/unit) to reach the desired weight capacity
    if( burden_proportion > 0.0 ) {
        int gold_units = static_cast<int>( capacity * burden_proportion / 5_gram );
        dummy.i_add( item( "gold_small", calendar::turn, gold_units ) );
    }

    // Might be off by a few grams
    REQUIRE( dummy.weight_carried() >= 0.999 * capacity * burden_proportion );
    REQUIRE( dummy.weight_carried() <= 1.001 * capacity * burden_proportion );
}

// Return amount of stamina burned per turn by `burn_move_stamina` in the given movement mode,
// while carrying the given proportion [0.0, inf) of maximum weight capacity.
static int burdened_burn_rate( player &dummy, character_movemode move_mode,
                               float burden_proportion = 0.0 )
{
    burden_player( dummy, burden_proportion );
    return actual_burn_rate( dummy, move_mode );
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


TEST_CASE( "stamina burn for movement", "[stamina][burn][move]" )
{
    player &dummy = g->u;

    // Game-balance configured "normal" rate of stamina burned per move
    int burn_rate = get_option<int>( "PLAYER_BASE_STAMINA_BURN_RATE" );

    GIVEN( "player is naked and unburdened" ) {
        THEN( "walking burns the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 0.0 ) == burn_rate );
        }

        THEN( "running burns 14 times the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 0.0 ) == burn_rate * 14 );
        }

        THEN( "crouching burns 1/2 the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 0.0 ) == burn_rate / 2 );
        }
    }

    GIVEN( "player is at their maximum weight capacity" ) {
        THEN( "walking burns the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 1.0 ) == burn_rate );
        }

        THEN( "running burns 14 times the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 1.0 ) == burn_rate * 14 );
        }

        THEN( "crouching burns 1/2 the normal amount of stamina per turn" ) {
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 1.0 ) == burn_rate / 2 );
        }
    }

    GIVEN( "player is overburdened" ) {
        THEN( "walking burn rate increases by 1 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 1.01 ) == burn_rate + 1 );
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 1.02 ) == burn_rate + 2 );
            //CHECK( burdened_burn_rate( dummy, CMM_WALK, 1.50 ) == burn_rate + 50 );
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 1.99 ) == burn_rate + 99 );
            CHECK( burdened_burn_rate( dummy, CMM_WALK, 2.00 ) == burn_rate + 100 );
        }

        THEN( "running burn rate increases by 14 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 1.01 ) == ( burn_rate + 1 ) * 14 );
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 1.02 ) == ( burn_rate + 2 ) * 14 );
            //CHECK( burdened_burn_rate( dummy, CMM_RUN, 1.50 ) == ( burn_rate + 50 ) * 14 );
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 1.99 ) == ( burn_rate + 99 ) * 14 );
            CHECK( burdened_burn_rate( dummy, CMM_RUN, 2.00 ) == ( burn_rate + 100 ) * 14 );
        }

        THEN( "crouching burn rate increases by 1/2 for each percent overburdened" ) {
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 1.01 ) == ( burn_rate + 1 ) / 2 );
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 1.02 ) == ( burn_rate + 2 ) / 2 );
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 1.50 ) == ( burn_rate + 50 ) / 2 );
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 1.99 ) == ( burn_rate + 99 ) / 2 );
            CHECK( burdened_burn_rate( dummy, CMM_CROUCH, 2.00 ) == ( burn_rate + 100 ) / 2 );
        }
    }
}

TEST_CASE( "burning stamina when overburdened may cause pain", "[stamina][burn][pain]" )
{
    player &dummy = g->u;
    int pain_before;
    int pain_after;

    GIVEN( "character is overburdened" ) {
        // As overburden percentage goes from (100% .. 350%),
        //           chance of pain goes from (1/25 .. 1/1)
        //
        // To guarantee pain when moving and ensure consistent test results,
        // set to 350% burden.
        burden_player( dummy, 3.5 );

        WHEN( "they have zero stamina left" ) {
            dummy.set_stamina( 0 );
            REQUIRE( dummy.get_stamina() == 0 );

            THEN( "they feel pain when carrying too much weight" ) {
                pain_before = dummy.get_pain();
                dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
                pain_after = dummy.get_pain();
                CHECK( pain_after > pain_before );
            }
        }

        WHEN( "they have a bad back" ) {
            dummy.toggle_trait( trait_id( "BADBACK" ) );
            REQUIRE( dummy.has_trait( trait_id( "BADBACK" ) ) );

            THEN( "they feel pain when carrying too much weight" ) {
                pain_before = dummy.get_pain();
                dummy.burn_move_stamina( to_moves<int>( 1_turns ) );
                pain_after = dummy.get_pain();
                CHECK( pain_after > pain_before );
            }
        }
    }
}

// TODO: stamina burn is modified by bionic muscles

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
