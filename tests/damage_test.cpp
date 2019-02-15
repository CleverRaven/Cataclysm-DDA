#include "catch/catch.hpp"
#include "item.h"
#include "mondefense.h"
#include "monster.h"
#include "npc.h"
#include "projectile.h"
#include "json.h"
#include "item_factory.h"
#include "init.h"
#include "game.h"
#include "itype.h"

TEST_CASE( "ranged_bow_normal_basic_damage_legacy", "[damage][gun_damage]" )
{
    const itype *testbow = item_controller->find_template( "test_bow_1" );
    REQUIRE( testbow->gun->damage.damage_units.size() == 3 );
    for( damage_unit unit : testbow->gun->damage.damage_units ) {
        switch( unit.type ) {
            case damage_type::DT_STAB:
                REQUIRE( unit.amount == 4 );
                break;
            case damage_type::DT_CUT:
                REQUIRE( unit.amount == 0 );
                break;
            case damage_type::DT_BASH:
                REQUIRE( unit.amount == 0 );
                break;
            default:
                WARN( "Damage type " << unit.type << " should not be present!" );
                break;
        }
    }


    const itype *testarrow = item_controller->find_template( "test_arrow_1" );
    REQUIRE( testarrow->ammo->damage.damage_units.size() == 3 );
    for( damage_unit unit : testarrow->ammo->damage.damage_units ) {
        switch( unit.type ) {
            case damage_type::DT_STAB:
                REQUIRE( unit.amount == 4 );
                break;
            case damage_type::DT_CUT:
                REQUIRE( unit.amount == 0 );
                break;
            case damage_type::DT_BASH:
                REQUIRE( unit.amount == 0 );
                break;
            default:
                WARN( "Damage type " << unit.type << " should not be present!" );
                break;
        }
    }

    item bow( "test_bow_1" );
    bow.ammo_set( "test_arrow_1", 1 );
    item arrow( "test_arrow_1" );
    bow.fill_with( arrow, 1 );
    damage_instance dmg = bow.gun_damage();

    REQUIRE( dmg.total_damage() == 8.0f );
}

TEST_CASE( "ranged_bow_normal_basic_damage_modern", "[damage][gun_damage]" )
{
    const itype *testbow = item_controller->find_template( "test_bow_2" );
    REQUIRE( testbow->gun->damage.damage_units.size() == 1 );
    REQUIRE( testbow->gun->damage.damage_units[0].type == damage_type::DT_CUT );

    const itype *testarrow = item_controller->find_template( "test_arrow_2" );
    REQUIRE( testarrow->ammo->damage.damage_units.size() == 1 );
    REQUIRE( testarrow->ammo->damage.damage_units[0].type == damage_type::DT_CUT );

    item bow( "test_bow_2" );
    bow.ammo_set( "test_arrow_2", 1 );
    item arrow( "test_arrow_2" );
    bow.fill_with( arrow, 1 );
    damage_instance dmg = bow.gun_damage();

    REQUIRE( dmg.total_damage() == 8.0f );
}
