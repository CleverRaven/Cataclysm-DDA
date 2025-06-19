#include <string>

#include "cata_catch.h"
#include "creature.h"
#include "item.h"
#include "mondefense.h"
#include "monster.h"
#include "npc.h"
#include "projectile.h"
#include "type_id.h"
#include "units.h"

static const bionic_id bio_faraday( "bio_faraday" );
static const bionic_id bio_power_storage( "bio_power_storage" );

static const gun_mode_id gun_mode_MELEE( "MELEE" );

static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_knuckle_nail( "knuckle_nail" );
static const itype_id itype_pike( "pike" );
static const itype_id itype_reach_bow( "reach_bow" );
static const itype_id itype_rock( "rock" );

static const mtype_id mon_zombie_electric( "mon_zombie_electric" );
static const mtype_id mon_zomborg( "mon_zomborg" );

static void test_zapback( Creature &attacker, const bool expect_damage,
                          const dealt_projectile_attack *proj = nullptr )
{
    monster defender( mon_zombie_electric );
    int prev_hp = attacker.get_hp();
    mdefense::zapback( defender, &attacker, proj );

    if( expect_damage ) {
        CHECK( attacker.get_hp() < prev_hp );
    } else {
        CHECK( attacker.get_hp() == prev_hp );
    }
}

TEST_CASE( "zapback_npc_unarmed", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    test_zapback( attacker, true );
}

TEST_CASE( "zapback_npc_nonconductive_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item rock( itype_rock );
    attacker.wield( rock );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_nonconductive_unarmed_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    // AFAICT this is the only nonconductive unarmed weapon.
    item knuckle_nail( itype_knuckle_nail );
    attacker.wield( knuckle_nail );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_reach_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item pike( itype_pike );
    attacker.wield( pike );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_ranged_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item gun( itype_glock_19 );
    attacker.wield( gun );
    dealt_projectile_attack attack;
    test_zapback( attacker, false, &attack );
}

TEST_CASE( "zapback_npc_thrown_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    dealt_projectile_attack attack;
    test_zapback( attacker, false, &attack );
}

TEST_CASE( "zapback_npc_firing_ranged_reach_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item ranged_reach_weapon( itype_reach_bow );
    attacker.wield( ranged_reach_weapon );
    dealt_projectile_attack attack;
    test_zapback( attacker, false, &attack );
}

TEST_CASE( "zapback_npc_meleeattack_ranged_reach_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item ranged_reach_weapon( itype_reach_bow );
    REQUIRE( ranged_reach_weapon.gun_set_mode( gun_mode_MELEE ) );
    attacker.wield( ranged_reach_weapon );
    test_zapback( attacker, true );
}

TEST_CASE( "zapback_npc_electricity_immune", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    attacker.add_bionic( bio_power_storage );
    attacker.set_power_level( attacker.get_max_power_level() );
    attacker.add_bionic( bio_faraday );
    // Don't forget to turn it on...
    test_zapback( attacker, true );
    // Wow this is a raw index?
    attacker.activate_bionic( attacker.bionic_at_index( attacker.num_bionics() - 1 ) );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_monster", "[mondefense]" )
{
    monster attacker( mon_zomborg );
    test_zapback( attacker, true );
}
