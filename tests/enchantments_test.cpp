#include "avatar.h"
#include "cata_catch.h"
#include "field.h"
#include "item.h"
#include "item_location.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"

static const bionic_id test_bio_ench( "test_bio_ench" );

static const efftype_id effect_blind( "blind" );
static const efftype_id effect_invisibility( "invisibility" );
static const trait_id trait_TEST_ENCH_MUTATION( "TEST_ENCH_MUTATION" );

static const mtype_id debug_mon("debug_mon");

static void advance_turn( Character &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
}

static void give_item( Character &guy, const std::string &item_id )
{
    guy.i_add( item( item_id ) );
    guy.recalculate_enchantment_cache();
}

struct enchant_test {
    int dex_before;
    int lie_before;
    int persuade_before;
    int intimidate_before;
};

static void test_generic_ench( avatar &p, enchant_test enc_test )
{
    // wait a turn for the effect to kick in
    p.process_turn();

    CHECK( p.get_dex() == enc_test.dex_before + p.get_dex_base() * 2 + 25 );
    CHECK( get_talker_for( p )->trial_chance_mod( "lie" ) == static_cast<int>( round( (
                enc_test.lie_before + 15 ) * 1.5 ) ) );
    CHECK( get_talker_for( p )->trial_chance_mod( "persuade" ) == static_cast<int>( round( (
                enc_test.persuade_before + 15 ) *
            1.5 ) ) );
    CHECK( get_talker_for( p )->trial_chance_mod( "intimidate" ) == static_cast<int>( (
                enc_test.intimidate_before + 1 )
            * 1.5 ) );

    CHECK( p.has_effect( effect_invisibility ) );

    const field &fields_here = get_map().field_at( p.pos() );
    CHECK( fields_here.find_field( field_type_id( "fd_shadow" ) ) != nullptr );

    // place a zombie next to the avatar
    const tripoint spot( 61, 60, 0 );
    clear_map();
    monster &zombie = spawn_test_monster( "mon_zombie", spot );

    p.on_hit( &zombie, bodypart_id( "torso" ), 0.0, nullptr );

    CHECK( zombie.has_effect( effect_blind ) );
}

static enchant_test set_enc_test( avatar &p )
{
    enchant_test enc_test;
    enc_test.dex_before = p.get_str();
    enc_test.lie_before = get_talker_for( p )->trial_chance_mod( "lie" );
    enc_test.persuade_before = get_talker_for( p )->trial_chance_mod( "persuade" );
    enc_test.intimidate_before = get_talker_for( p )->trial_chance_mod( "intimidate" );
    return enc_test;
}

TEST_CASE( "worn_enchantments", "[enchantments][worn][items]" )
{
    avatar p;
    clear_character( p );

    int str_before = p.get_str();

    // put on the ring
    item_location equipped_ring_strplus_one = p.i_add( item( "test_ring_strength_1" ) );
    p.wear( equipped_ring_strplus_one, false );

    // wait a turn for the effect to kick in
    p.recalculate_enchantment_cache();
    p.process_turn();

    CHECK( p.get_str() == str_before + 1 );

}

TEST_CASE( "bionic_enchantments", "[enchantments][bionics]" )
{
    avatar p;
    clear_character( p );

    enchant_test enc_test = set_enc_test( p );

    p.set_max_power_level( 100_kJ );
    p.set_power_level( 100_kJ );

    give_and_activate_bionic( p, test_bio_ench );

    test_generic_ench( p, enc_test );
}

TEST_CASE( "mutation_enchantments", "[enchantments][mutations]" )
{
    avatar p;
    clear_character( p );
    enchant_test enc_test = set_enc_test( p );

    p.toggle_trait( trait_TEST_ENCH_MUTATION );
    REQUIRE( p.has_trait( trait_TEST_ENCH_MUTATION ) );

    p.recalculate_enchantment_cache();

    test_generic_ench( p, enc_test );
}


TEST_CASE( "Enchantments_modify_speed", "[magic][enchantments][speed][WIP]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_avatar(), true );

    WHEN( "Character has no speed enchantment" ) {
        THEN( "Speed is default" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 100 );
            REQUIRE( guy.get_speed_bonus() == 0 );
        }
    }

    WHEN( "Character has speed enchantment" ) {
        give_item( guy, "test_relic_mods_speed" );
        guy.set_moves( 0 );
        advance_turn( guy );
        THEN( "Speed is changed accordingly" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 62 );
            REQUIRE( guy.get_speed_bonus() == -38 );
        }
    }

    WHEN( "Character loses speed enchantment" ) {
        clear_items( guy );
        guy.set_moves( 0 );
        advance_turn( guy );
        THEN( "Speed is default again" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 100 );
            REQUIRE( guy.get_speed_bonus() == 0 );
        }
    }
}


TEST_CASE( "Enchantment_SPEED_test", "[magic][enchantments][WIP]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_avatar(), true );

    WHEN( "Character has no speed enchantment" ) {
        THEN( "Speed is default" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 100 );
            REQUIRE( guy.get_speed_bonus() == 0 );
        }
    }

    WHEN( "Character has speed enchantment" ) {
        give_item( guy, "test_SPEED_ench_item" );
        guy.set_moves( 0 );
        advance_turn( guy );
        THEN( "Speed is changed accordingly" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 62 );
            REQUIRE( guy.get_speed_bonus() == -38 );
        }
    }

    WHEN( "Character loses speed enchantment" ) {
        clear_items( guy );
        guy.set_moves( 0 );
        advance_turn( guy );
        THEN( "Speed is default again" ) {
            REQUIRE( guy.get_speed_base() == 100 );
            REQUIRE( guy.get_speed() == 100 );
            REQUIRE( guy.get_speed_bonus() == 0 );
        }
    }
}

TEST_CASE( "Enchantment_ATTACK_SPEED_test", "[magic][enchantments][WIP]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_avatar(), true );
    g->place_critter_at( debug_mon, tripoint( 1, 1, 0 ) );

    WHEN( "Character attacks with no enchantment" ) {
        // 60 moves for the first attack, and 59 for each next, please don't ask why.
        THEN( "10 attacks cost 591 moves" ) {
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg("attack %i: amount of moves: %i", i, guy.get_moves());
                i++;
            }
            REQUIRE( guy.get_moves() == -591 );
        }
    }

    WHEN( "Character attacks with enchantment, that halves attack speed cost" ) {
        give_item( guy, "test_ATTACK_SPEED_ench_item" );
        advance_turn( guy );
        // 30 moves per attack
        THEN( "10 attacks cost only 300 moves" ) {
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg( "attack %i: amount of moves: %i", i, guy.get_moves() );
                i++;
            }
            REQUIRE( guy.get_moves() == -300 );
        }
    }

    WHEN( "Character attacks with enchantment, that adds 41 moves to each attack" ) {
        give_item( guy, "test_ATTACK_SPEED_ench_item_2" );
        advance_turn( guy );
        // 101 moves for first attack, next one are 100 moves
        THEN( "10 attacks cost 1001 moves" ) {
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg( "attack %i: amount of moves: %i", i, guy.get_moves() );
                i++;
            }
            REQUIRE( guy.get_moves() == -1001 );
        }
    }
}

TEST_CASE( "Enchantment_MELEE_STAMINA_CONSUMPTION_test", "[magic][enchantments][WIP]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_avatar(), true );
    g->place_critter_at( debug_mon, tripoint( 1, 1, 0 ) );

    WHEN( "Character attacks with no enchantment" ) {
        give_item( guy, "test_MELEE_STAMINA_CONSUMPTION_ench_item_0" );
        advance_turn( guy );
        THEN( "10 attacks cost 1750 stamina" ) {
            int stamina_start = guy.get_stamina();
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg( "attack %i: amount of stamina: %i", i, guy.get_stamina() );
                i++;
            }
            int stamina_spent = stamina_start - guy.get_stamina();
            REQUIRE( stamina_spent == 1750 );
        }
    }

    WHEN( "Character attacks with enchantment, that decreases stamina cost for 100" ) {
        clear_character( *guy.as_avatar(), true );
        give_item( guy, "test_MELEE_STAMINA_CONSUMPTION_ench_item_1" );
        advance_turn( guy );
        THEN( "10 attacks cost 750 stamina" ) {
            int stamina_start = guy.get_stamina();
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg( "attack %i: amount of stamina: %i", i, guy.get_stamina() );
                i++;
            }
            int stamina_spent = stamina_start - guy.get_stamina();
            REQUIRE( stamina_spent == 750 );
        }
    }

    WHEN( "Character attacks with enchantment, that double stamina cost" ) {
        clear_character( *guy.as_avatar(), true );
        give_item( guy, "test_MELEE_STAMINA_CONSUMPTION_ench_item_2" );
        advance_turn( guy );
        THEN( "10 attacks cost 3500 stamina" ) {
            int stamina_start = guy.get_stamina();
            int i = 0;
            while( i != 10 ) {
                guy.melee_attack_abstract( guy, false, matec_id( "" ) );
                //debugmsg( "attack %i: amount of stamina: %i", i, guy.get_stamina() );
                i++;
            }
            int stamina_spent = stamina_start - guy.get_stamina();
            REQUIRE( stamina_spent == 3500 );
        }
    }
}

TEST_CASE("Enchantment_BONUS_DODGE_test", "[magic][enchantments][WIP]")
{
    clear_map();
    Character& guy = get_player_character();
    clear_character(*guy.as_avatar(), true);

    WHEN("Character has default amount of dodges, not affected by enchantments") {
        THEN("1 dodge") {
            REQUIRE(guy.get_num_dodges() == 1);
        }
    }

    WHEN("Character has enchantment that gives +3 dodges") {
        give_item(guy, "test_BONUS_DODGE_ench_item_1");
        advance_turn(guy);
        THEN("4 dodges") {
            REQUIRE(guy.get_num_dodges() == 4);
        }
    }

    clear_character(*guy.as_avatar(), true);

    WHEN("Character has enchantment that gives +4 dodges, and then halves amount of dodges") {
        give_item(guy, "test_BONUS_DODGE_ench_item_2");
        advance_turn(guy);
        THEN("2.5 dodges, rounded down to 2") {
            REQUIRE(guy.get_num_dodges() == 2);
        }
    }
}
