#include <cstdlib>

#include "avatar.h"
#include "calendar.h"
#include "creature.h"
#include "game.h"
#include "item.h"
#include "player.h"
#include "type_id.h"

#include "catch/catch.hpp"
#include "player_helpers.h"

// Run a large number of trials of a player attacking a monster with a given weapon,
// and return the average damage done per second.
static double weapon_dps_trials( avatar &attacker, monster &defender, item &weapon )
{
    constexpr int trials = 10000;

    int total_damage = 0;
    int total_moves = 0;

    clear_character( attacker );
    REQUIRE( attacker.can_wield( weapon ).success() );

    // FIXME: This is_wielding check always fails
    //REQUIRE( attacker.is_wielding( weapon ) );

    for( int i = 0; i < trials; i++ ) {
        // Reset and re-wield weapon before each attack to prevent skill-up during trials
        clear_character( attacker );
        attacker.wield( weapon );
        int before_moves = attacker.get_moves();

        // Keep the defender at maximum health
        const int starting_hp = defender.get_hp_max();
        defender.set_hp( starting_hp );

        // Attack once
        attacker.melee_attack( defender, false );

        // Tally total damage and moves
        total_damage += std::max( 0, starting_hp - defender.get_hp() );
        total_moves += std::abs( attacker.get_moves() - before_moves );
    }

    // Scale damage-per-move to damage-per-second
    // 1 second == 100 moves
    return 100.0f * total_damage / total_moves;
}

// Compare actual DPS with estimated effective DPS for an attacker/defender/weapon combo.
static void check_actual_dps( avatar &attacker, monster &defender, item &weapon )
{
    clear_character( attacker );
    double expect_dps = weapon.effective_dps( attacker, defender );
    double actual_dps = weapon_dps_trials( attacker, defender, weapon );
    CHECK( actual_dps == Approx( expect_dps ).margin( 0.01f ) );
}

TEST_CASE( "effective damage per second", "[effective][dps]" )
{
    avatar &dummy = g->u;
    clear_character( dummy );

    item rock( "test_rock" );
    item plank( "test_2x4" );
    item knife( "knife_trench" );

    SECTION( "against a debug monster with no armor or dodge" ) {
        monster mummy( mtype_id( "debug_mon" ) );

        CHECK( plank.effective_dps( dummy, mummy ) == Approx( 4.79f ).margin( 0.01f ) );
        CHECK( knife.effective_dps( dummy, mummy ) == Approx( 20.16f ).margin( 0.01f ) );
    }

    SECTION( "against an agile target" ) {
        monster smoker( mtype_id( "mon_zombie_smoker" ) );
        REQUIRE( smoker.get_dodge() >= 4 );

        CHECK( plank.effective_dps( dummy, smoker ) == Approx( 2.51f ).margin( 0.01f ) );
        CHECK( knife.effective_dps( dummy, smoker ) == Approx( 10.57f ).margin( 0.01f ) );
    }

    SECTION( "against an armored target" ) {
        monster soldier( mtype_id( "mon_zombie_soldier" ) );

        CHECK( plank.effective_dps( dummy, soldier ) == Approx( 1.11f ).margin( 0.01f ) );
        CHECK( knife.effective_dps( dummy, soldier ) == Approx( 2.22f ).margin( 0.01f ) );
    }

    SECTION( "effect of STR and DEX on damage per second" ) {
        monster mummy( mtype_id( "debug_mon" ) );

        SECTION( "STR 6, DEX 6" ) {
            dummy.str_max = 6;
            dummy.dex_max = 6;

            CHECK( rock.effective_dps( dummy, mummy ) == Approx( 6.55f ).margin( 0.01f ) );
            CHECK( plank.effective_dps( dummy, mummy ) == Approx( 3.60f ).margin( 0.01f ) );
            CHECK( knife.effective_dps( dummy, mummy ) == Approx( 16.66f ).margin( 0.01f ) );
        }

        SECTION( "STR 8, DEX 8" ) {
            dummy.str_max = 8;
            dummy.dex_max = 8;

            CHECK( rock.effective_dps( dummy, mummy ) == Approx( 8.84f ).margin( 0.01f ) );
            CHECK( plank.effective_dps( dummy, mummy ) == Approx( 4.79f ).margin( 0.01f ) );
            CHECK( knife.effective_dps( dummy, mummy ) == Approx( 20.16f ).margin( 0.01f ) );
        }

        SECTION( "STR 10, DEX 10" ) {
            dummy.str_max = 10;
            dummy.dex_max = 10;

            CHECK( rock.effective_dps( dummy, mummy ) == Approx( 10.42f ).margin( 0.01f ) );
            CHECK( plank.effective_dps( dummy, mummy ) == Approx( 5.55f ).margin( 0.01f ) );
            CHECK( knife.effective_dps( dummy, mummy ) == Approx( 21.61f ).margin( 0.01f ) );
        }
    }
}

TEST_CASE( "effective vs actual damage per second", "[actual][dps][!mayfail]" )
{
    avatar &dummy = g->u;
    clear_character( dummy );

    monster mummy( mtype_id( "debug_mon" ) );
    monster soldier( mtype_id( "mon_zombie_soldier" ) );
    monster smoker( mtype_id( "mon_zombie_smoker" ) );
    monster survivor( mtype_id( "mon_zombie_survivor" ) );

    item rock( "test_rock" );
    item plank( "test_2x4" );
    item knife( "knife_trench" );
    item halligan( "test_halligan" );

    SECTION( "debug monster" ) {
        check_actual_dps( dummy, mummy, rock );
        check_actual_dps( dummy, mummy, plank );
        check_actual_dps( dummy, mummy, knife );
        check_actual_dps( dummy, mummy, halligan );
    }

    SECTION( "soldier zombie" ) {
        check_actual_dps( dummy, soldier, rock );
        check_actual_dps( dummy, soldier, plank );
        check_actual_dps( dummy, soldier, knife );
        check_actual_dps( dummy, soldier, halligan );
    }

    SECTION( "smoker zombie" ) {
        check_actual_dps( dummy, smoker, rock );
        check_actual_dps( dummy, smoker, plank );
        check_actual_dps( dummy, smoker, knife );
        check_actual_dps( dummy, smoker, halligan );
    }

    SECTION( "survivor zombie" ) {
        check_actual_dps( dummy, survivor, rock );
        check_actual_dps( dummy, survivor, plank );
        check_actual_dps( dummy, survivor, knife );
        check_actual_dps( dummy, survivor, halligan );
    }
}

