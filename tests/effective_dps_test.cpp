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
static double weapon_dps_trials( player &attacker, monster &defender, item &weapon )
{
    constexpr int trials = 10000;

    int total_damage = 0;
    int total_moves = 0;

    clear_character( attacker );
    attacker.weapon = weapon;

    for( int i = 0; i < trials; i++ ) {
        // Keep track of attacker's moves and defender's HP
        const int before_moves = attacker.get_moves();
        const int starting_hp = defender.get_hp();
        // Attack once
        attacker.melee_attack( defender, false );
        // Tally total damage and moves
        total_damage += std::max( 0, starting_hp - defender.get_hp() );
        total_moves += std::abs( attacker.get_moves() - before_moves );
        // Reset for the next attack
        clear_character( attacker );    // Don't learn
        defender.set_hp( starting_hp ); // Don't die
    }

    // Scale damage-per-move to damage-per-second
    // 1 second == 100 moves
    return 100.0f * total_damage / total_moves;
}

TEST_CASE( "effective damage per second from STR and DEX", "[effective][dps][str][dex]" )
{
    player &dummy = g->u;
    monster mummy = mtype_id( "debug_mon" );

    clear_character( dummy );

    item rock( "test_rock" );
    item halligan( "test_halligan" );

    SECTION( "STR 6, DEX 6" ) {
        dummy.str_max = 6;
        dummy.dex_max = 6;

        CHECK( rock.effective_dps( dummy, mummy ) == Approx( 6.55f ).margin( 0.01f ) );
        CHECK( halligan.effective_dps( dummy, mummy ) == Approx( 9.33f ).margin( 0.01f ) );
    }

    SECTION( "STR 8, DEX 8" ) {
        dummy.str_max = 8;
        dummy.dex_max = 8;

        CHECK( rock.effective_dps( dummy, mummy ) == Approx( 8.84f ).margin( 0.01f ) );
        CHECK( halligan.effective_dps( dummy, mummy ) == Approx( 11.64f ).margin( 0.01f ) );
    }

    SECTION( "STR 10, DEX 10" ) {
        dummy.str_max = 10;
        dummy.dex_max = 10;

        CHECK( rock.effective_dps( dummy, mummy ) == Approx( 10.42f ).margin( 0.01f ) );
        CHECK( halligan.effective_dps( dummy, mummy ) == Approx( 12.80f ).margin( 0.01f ) );
    }
}

TEST_CASE( "effective vs actual damage per second", "[actual][dps][!mayfail]" )
{
    player &dummy = g->u;
    monster mummy = mtype_id( "debug_mon" );

    clear_character( dummy );

    double expect_dps;
    double actual_dps;

    SECTION( "rock damage per second" ) {
        item rock( "test_rock" );
        expect_dps = rock.effective_dps( dummy, mummy );
        actual_dps = weapon_dps_trials( dummy, mummy, rock );

        // FIXME: 3.4568 != 8.8445
        CHECK( actual_dps == Approx( expect_dps ).margin( 0.01f ) );
    }

    SECTION( "halligan bar damage per second" ) {
        item halligan( "test_halligan" );
        expect_dps = halligan.effective_dps( dummy, mummy );
        actual_dps = weapon_dps_trials( dummy, mummy, halligan );

        // FIXME: 3.4909 != 11.6390
        CHECK( actual_dps == Approx( expect_dps ).margin( 0.01f ) );
    }
}

