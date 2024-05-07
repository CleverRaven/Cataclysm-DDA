#include "avatar.h"
#include "cata_catch.h"
#include "creature_tracker.h"
#include "field.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "talker.h"
#include "type_id.h"
#include "units.h"

static const bionic_id test_bio_ench( "test_bio_ench" );

static const efftype_id effect_blind( "blind" );
static const efftype_id effect_debug_no_staggered( "debug_no_staggered" );
static const efftype_id effect_invisibility( "invisibility" );

static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const skill_id skill_melee( "melee" );

static const trait_id trait_TEST_ENCH_MUTATION( "TEST_ENCH_MUTATION" );

static void advance_turn( Character &guy )
{
    guy.process_turn();
    calendar::turn += 1_turns;
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

    CHECK( p.get_dex() == ( enc_test.dex_before + 25 ) * 3 );
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

TEST_CASE( "Enchantments_change_stats", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();
    INFO( "Default character with 8 8 8 8 stats" );
    INFO( "When obtain item with stat enchantments" );
    guy.i_add( item( "test_STAT_ench_item_1" ) );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    INFO( "Stats change accordingly" );
    REQUIRE( guy.get_str() == 22 );
    REQUIRE( guy.get_dex() == 6 );
    REQUIRE( guy.get_int() == 5 );
    REQUIRE( guy.get_per() == 1 );
    INFO( "Item is removed" );
    clear_avatar();
    advance_turn( guy );
    INFO( "Stats change back" );
    REQUIRE( guy.get_str() == 8 );
    REQUIRE( guy.get_dex() == 8 );
    REQUIRE( guy.get_int() == 8 );
    REQUIRE( guy.get_per() == 8 );


    INFO( "Default character with 8 8 8 8 stats" );
    INFO( "When obtain two items with stat enchantments" );
    item backpack( "debug_backpack" );
    guy.wear_item( backpack );
    item enchantment_item( "test_STAT_ench_item_1" );
    guy.i_add( enchantment_item );
    guy.i_add( enchantment_item );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    INFO( "Stats change accordingly" );
    REQUIRE( guy.get_str() == 42 );
    REQUIRE( guy.get_dex() == 4 );
    REQUIRE( guy.get_int() == 0 );
    REQUIRE( guy.get_per() == 0 );
}

TEST_CASE( "Enchantment_SPEED_test", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();

    INFO( "Character has no speed enchantment" );
    INFO( "Speed is default" );
    REQUIRE( guy.get_speed_base() == 100 );
    REQUIRE( guy.get_speed() == 100 );
    REQUIRE( guy.get_speed_bonus() == 0 );


    INFO( "Character obtain speed enchantment" );
    guy.i_add( item( "test_SPEED_ench_item" ) );
    guy.recalculate_enchantment_cache();
    guy.set_moves( 0 );
    advance_turn( guy );
    INFO( "Speed is changed accordingly" );
    REQUIRE( guy.get_speed_base() == 100 );
    REQUIRE( guy.get_speed() == 62 );
    REQUIRE( guy.get_speed_bonus() == -38 );


    INFO( "Character loses speed enchantment" );
    clear_avatar();
    guy.set_moves( 0 );
    advance_turn( guy );
    INFO( "Speed is default again" );
    REQUIRE( guy.get_speed_base() == 100 );
    REQUIRE( guy.get_speed() == 100 );
    REQUIRE( guy.get_speed_bonus() == 0 );
}

static int test_melee_attack_attack_speed( Character &guy, Creature &mon )
{
    int i = 0;
    int prev_attack = 0;

    guy.set_skill_level( skill_melee, 10 );
    guy.add_effect( effect_debug_no_staggered, 100_seconds );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    guy.set_moves( 0 );

    while( i != 10 ) {
        prev_attack = guy.get_moves();
        guy.melee_attack_abstract( mon, false, matec_id( "" ) );
        add_msg( "attack %i: attack cost: %i, total amount of moves: %i", i, prev_attack - guy.get_moves(),
                 guy.get_moves() );
        guy.set_stamina( guy.get_stamina_max() ); //Reset reset!
        guy.set_sleepiness( 0 );
        i++;
    }

    return guy.get_moves();
}

TEST_CASE( "Enchantment_ATTACK_SPEED_test", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();
    g->place_critter_at( pseudo_debug_mon, tripoint_south );
    creature_tracker &creatures = get_creature_tracker();
    Creature &mon = *creatures.creature_at<Creature>( tripoint_south );
    int moves_spent_on_attacks = 0;


    INFO( "Character, melee skill lvl 10, attacks with no enchantment" );
    // 38 moves per attack
    INFO( "10 attacks cost 380 moves" );
    moves_spent_on_attacks = test_melee_attack_attack_speed( guy, mon );
    REQUIRE( moves_spent_on_attacks == -380 );


    INFO( "Character, melee skill lvl 10, attacks with enchantment, that halves attack speed cost" );
    guy.i_add( item( "test_ATTACK_SPEED_ench_item" ) );
    // 25 moves per attack
    INFO( "10 attacks cost only 250 moves" );
    moves_spent_on_attacks = test_melee_attack_attack_speed( guy, mon );
    REQUIRE( moves_spent_on_attacks == -250 );
    clear_avatar();


    INFO( "Character attacks with enchantment, that adds 62 moves to each attack" );
    guy.i_add( item( "test_ATTACK_SPEED_ench_item_2" ) );
    // 100 moves per attack
    INFO( "10 attacks cost 1000 moves" );
    moves_spent_on_attacks = test_melee_attack_attack_speed( guy, mon );
    REQUIRE( moves_spent_on_attacks == -1000 );
}


static int test_melee_attack_attack_stamina( Character &guy, Creature &mon )
{
    int i = 0;
    int stamina_prev = 0;
    guy.set_skill_level( skill_melee, 10 );
    guy.add_effect( effect_debug_no_staggered, 100_seconds );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );

    while( i != 10 ) {
        stamina_prev = guy.get_stamina();
        guy.melee_attack_abstract( mon, false, matec_id( "" ) );
        add_msg( "attack %i: stamina cost: %i, current amount of stamina: %i", i,
                 stamina_prev - guy.get_stamina(),
                 guy.get_stamina() );
        guy.set_sleepiness( 0 );
        i++;
    }

    return guy.get_stamina();
}


TEST_CASE( "Enchantment_MELEE_STAMINA_CONSUMPTION_test", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();
    g->place_critter_at( pseudo_debug_mon, tripoint_south );
    creature_tracker &creatures = get_creature_tracker();
    Creature &mon = *creatures.creature_at<Creature>( tripoint_south );
    int stamina_init = 0;
    int stamina_current = 0;
    int stamina_spent = 0;

    INFO( "Character attacks with no enchantment" );
    // item weight 2 kilo and has 2 L of volume
    guy.i_add( item( "test_MELEE_STAMINA_CONSUMPTION_ench_item_0" ) );
    // 165 stamina per attack
    INFO( "10 attacks cost 1650 stamina" );
    stamina_init = guy.get_stamina();
    stamina_current = test_melee_attack_attack_stamina( guy, mon );
    stamina_spent = stamina_init - stamina_current;
    REQUIRE( stamina_spent == 1650 );
    clear_avatar();


    INFO( "Character attacks with enchantment, that decreases stamina cost for 100" );
    guy.i_add( item( "test_MELEE_STAMINA_CONSUMPTION_ench_item_1" ) );
    // 65 stamina per attack
    INFO( "10 attacks cost 650 stamina" );
    stamina_init = guy.get_stamina();
    stamina_current = test_melee_attack_attack_stamina( guy, mon );
    stamina_spent = stamina_init - stamina_current;
    REQUIRE( stamina_spent == 650 );
    clear_avatar();


    INFO( "Character attacks with enchantment, that double stamina cost" );
    guy.i_add( item( "test_MELEE_STAMINA_CONSUMPTION_ench_item_2" ) );
    // 330 stamina per attack
    INFO( "10 attacks cost 3300 stamina" );
    stamina_init = guy.get_stamina();
    stamina_current = test_melee_attack_attack_stamina( guy, mon );
    stamina_spent = stamina_init - stamina_current;
    REQUIRE( stamina_spent == 3300 );
    clear_avatar();
}

TEST_CASE( "Enchantment_BONUS_DODGE_test", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();

    INFO( "Character has default amount of dodges, not affected by enchantments" );
    INFO( "1 dodge" );
    REQUIRE( guy.get_num_dodges() == 1 );


    INFO( "Character has enchantment that gives +3 dodges" );
    guy.i_add( item( "test_BONUS_DODGE_ench_item_1" ) );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    INFO( "4 dodges" );
    REQUIRE( guy.get_num_dodges() == 4 );

    clear_avatar();

    INFO( "Character has enchantment that gives +4 dodges, and then halves amount of dodges" );
    guy.i_add( item( "test_BONUS_DODGE_ench_item_2" ) );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    INFO( "2.5 dodges, rounded down to 2" );
    REQUIRE( guy.get_num_dodges() == 2 );
}

TEST_CASE( "Enchantment_PAIN_PENALTY_MOD_test", "[magic][enchantments]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_avatar();
    INFO( "Character has 50 pain, not affected by enchantments" );
    guy.set_pain( 50 );
    advance_turn( guy );
    INFO( "Stats are: 6 str, 5 dex, 3 int, 3 per, 85 speed" );
    REQUIRE( guy.get_str() == 6 );
    REQUIRE( guy.get_dex() == 5 );
    REQUIRE( guy.get_int() == 4 );
    REQUIRE( guy.get_per() == 4 );
    REQUIRE( guy.get_speed() == 85 );


    INFO( "Character has 50 pain, obtain enchantment" );
    guy.i_add( item( "test_PAIN_PENALTY_MOD_ench_item_1" ) );
    guy.recalculate_enchantment_cache();
    advance_turn( guy );
    INFO( "Stats are: 4 str, 7 dex, 7 int, 0 per, 89 speed" );
    REQUIRE( guy.get_str() == 4 );
    REQUIRE( guy.get_dex() == 7 );
    REQUIRE( guy.get_int() == 7 );
    REQUIRE( guy.get_per() == 0 );
    REQUIRE( guy.get_speed() == 89 );
}
