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

static void test_generic_ench( avatar &p, int str_before )
{
    // wait a turn for the effect to kick in
    p.process_turn();

    CHECK( p.get_str() == str_before + p.get_str_base() * 2 + 25 );

    CHECK( p.has_effect( efftype_id( "invisibility" ) ) );

    const field &fields_here = get_map().field_at( p.pos() );
    CHECK( fields_here.find_field( field_type_id( "fd_shadow" ) ) != nullptr );

    // place a zombie next to the avatar
    const tripoint spot( 61, 60, 0 );
    clear_map();
    monster &zombie = spawn_test_monster( "mon_zombie", spot );

    p.on_hit( &zombie, bodypart_id( "torso" ), 0.0, nullptr );

    CHECK( zombie.has_effect( efftype_id( "blind" ) ) );
}

TEST_CASE( "worn enchantments", "[enchantments][worn][items]" )
{
    avatar p;
    clear_character( p );

    int str_before = p.get_str();

    // put on the ring
    item &equiped_ring_strplus_one = p.i_add( item( "test_ring_strength_1" ) );
    p.wear( item_location( *p.as_character(), &equiped_ring_strplus_one ), false );

    // wait a turn for the effect to kick in
    p.recalculate_enchantment_cache();
    p.process_turn();

    CHECK( p.get_str() == str_before + 1 );

}

TEST_CASE( "bionic enchantments", "[enchantments][bionics]" )
{
    avatar p;
    clear_character( p );

    int str_before = p.get_str();

    p.set_max_power_level( 100_kJ );
    p.set_power_level( 100_kJ );

    give_and_activate_bionic( p, bionic_id( "test_bio_ench" ) );

    test_generic_ench( p, str_before );
}

TEST_CASE( "mutation enchantments", "[enchantments][mutations]" )
{
    avatar p;
    clear_character( p );

    const trait_id test_ink( "TEST_ENCH_MUTATION" );
    int str_before = p.get_str();

    p.toggle_trait( test_ink );
    REQUIRE( p.has_trait( test_ink ) );

    p.recalculate_enchantment_cache();

    test_generic_ench( p, str_before );
}
