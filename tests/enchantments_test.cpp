#include "avatar.h"
#include "cata_catch.h"
#include "field.h"
#include "item.h"
#include "item_location.h"
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
