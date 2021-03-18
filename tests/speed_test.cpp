#include "catch/catch.hpp"

#include "avatar.h"
#include "player_helpers.h"
#include "map.h"
#include "map_helpers.h"

static void advance_turn( Character &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
}

static player &prepare_player()
{
    clear_map();
    player &guy = *get_player_character().as_player();
    clear_character( *guy.as_player(), true );
    guy.set_moves( 0 );

    advance_turn( guy );

    REQUIRE( guy.get_speed_base() == 100 );
    REQUIRE( guy.get_speed() == 100 );
    REQUIRE( guy.get_moves() == 100 );

    guy.set_moves( 0 );

    return guy;
}

TEST_CASE( "Character regains moves each turn", "[speed]" )
{
    player &guy = prepare_player();

    advance_turn( guy );

    CHECK( guy.get_moves() == 100 );
}

static void pain_penalty_test( player &guy, int pain, int speed_exp )
{
    int penalty = 100 - speed_exp;

    guy.set_pain( pain );
    REQUIRE( guy.get_pain() == pain );
    guy.set_painkiller( 0 );
    REQUIRE( guy.get_painkiller() == 0 );
    REQUIRE( guy.get_perceived_pain() == pain );
    REQUIRE( guy.get_pain_penalty().speed == penalty );

    advance_turn( guy );

    CHECK( guy.get_speed_bonus() == -penalty );
    CHECK( guy.get_speed() == speed_exp );
    CHECK( guy.get_moves() == speed_exp );
}

TEST_CASE( "Character is slowed down by pain", "[speed][pain]" )
{
    player &guy = prepare_player();

    WHEN( "10 pain" ) {
        pain_penalty_test( guy, 10, 95 );
    }
    WHEN( "100 pain" ) {
        pain_penalty_test( guy, 100, 75 );
    }
    WHEN( "300 pain" ) {
        pain_penalty_test( guy, 300, 70 );
    }
}

static void carry_weight_test( Character &guy, int load_kg, int speed_exp )
{
    item item_1kg( "test_1kg_cube" );
    REQUIRE( item_1kg.weight() == 1_kilogram );
    REQUIRE( item_1kg.volume() == 10_ml );

    CAPTURE( load_kg, speed_exp );
    WHEN( "Character carries specified weight" ) {
        for( int i = 0; i < load_kg; i++ ) {
            guy.inv.add_item( item_1kg );
        }
        THEN( "No effect on speed" ) {
            CHECK( guy.get_speed() == 100 );
            AND_WHEN( "Turn passes" ) {
                advance_turn( guy );
                REQUIRE( guy.weight_carried() == 1_kilogram * load_kg + 633_gram );
                REQUIRE( guy.weight_capacity() == 45_kilogram );
                THEN( "Speed matches expected value" ) {
                    CHECK( guy.get_speed() == speed_exp );
                }
            }
        }
    }
}

TEST_CASE( "Character is slowed down while overburdened", "[speed]" )
{
    player &guy = prepare_player();

    item backpack( "test_backpack" );
    REQUIRE( backpack.get_storage() == 15_liter );
    REQUIRE( backpack.weight() == 633_gram );

    guy.clear_mutations();
    guy.weapon = backpack;
    guy.wear( guy.weapon, false );
    REQUIRE( guy.weight_capacity() == 45_kilogram );
    REQUIRE( guy.volume_capacity() == 15_liter );

    SECTION( "Carry weight under carry capacity" ) {
        // 94%, no penalty
        carry_weight_test( guy, 42, 100 );
    }
    SECTION( "Carry weight over carry capacity" ) {
        // 148% gives -12 speed (25 * 0.48)
        carry_weight_test( guy, 66, 88 );
    }
    SECTION( "Carry weight significantly over carry capacity" ) {
        // 228% gives -32 speed (25 * 1.28)
        carry_weight_test( guy, 102, 68 );
    }
}
