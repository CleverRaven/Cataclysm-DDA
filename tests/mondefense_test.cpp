#include "catch/catch.hpp"
#include "item.h"
#include "mondefense.h"
#include "monster.h"
#include "npc.h"
#include "projectile.h"
#include "creature.h"
#include "type_id.h"

static void test_zapback( Creature &attacker, const bool expect_damage,
                          const dealt_projectile_attack *proj = nullptr )
{
    monster defender( mtype_id( "mon_zombie_electric" ) );
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
    item rock( "rock" );
    attacker.wield( rock );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_nonconductive_unarmed_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    // AFAICT this is the only nonconductive unarmed weapon.
    item knuckle_nail( "knuckle_nail" );
    attacker.wield( knuckle_nail );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_reach_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item pike( "pike" );
    attacker.wield( pike );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_npc_ranged_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item gun( "glock_19" );
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
    item ranged_reach_weapon( "reach_bow" );
    attacker.wield( ranged_reach_weapon );
    dealt_projectile_attack attack;
    test_zapback( attacker, false, &attack );
}

TEST_CASE( "zapback_npc_meleeattack_ranged_reach_weapon", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    item ranged_reach_weapon( "reach_bow" );
    REQUIRE( ranged_reach_weapon.gun_set_mode( gun_mode_id( "MELEE" ) ) );
    attacker.wield( ranged_reach_weapon );
    test_zapback( attacker, true );
}

TEST_CASE( "zapback_npc_electricity_immune", "[mondefense]" )
{
    standard_npc attacker( "Attacker" );
    attacker.add_bionic( bionic_id( "bio_power_storage" ) );
    attacker.add_bionic( bionic_id( "bio_faraday" ) );
    attacker.charge_power( 100 );
    // Don't forget to turn it on...
    test_zapback( attacker, true );
    // Wow this is a raw index?
    attacker.activate_bionic( 0 );
    test_zapback( attacker, false );
}

TEST_CASE( "zapback_monster", "[mondefense]" )
{
    monster attacker( mtype_id( "mon_zombie" ) );
    test_zapback( attacker, true );
}
