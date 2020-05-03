/*
 * Researching what seem to be generally agreed on values
 * for bow power required for guaranteed game kills, these are some breakpoints.
 * While many source use KE values, there is a broad consensus that momentum is related to damage.
 * FPS  ft-lbs  slug-ft-s  Game
 * 136  19.01  0.28  Turkeys
 * 153  24  0.315  Whitetails at close range
 * 195  39  0.401  Whitetails
 * 249  63.73  0.512  Elk
 * 278  79.44  0.572  Moose
 *
 * The concept is to bracket these threshods with various bows using standard hunting loadouts.
 */

#include <memory>
#include <string>

#include "catch/catch.hpp"
#include "damage.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "point.h"
#include "projectile.h"
#include "type_id.h"

// In short, a bow should never destroy a wall, pretty simple.
static void test_projectile_hitting_wall( const std::string &target_type, bool smashable,
        dealt_projectile_attack &attack, const std::string &weapon_type )
{
    static const tripoint target_point{ 5, 5, 0 };
    for( int i = 0; i < 10; ++i ) {
        projectile projectile_copy = attack.proj;
        g->m.set( target_point, ter_id( target_type ), furn_id( "f_null" ) );
        CAPTURE( projectile_copy.impact.total_damage() );
        g->m.shoot( target_point, projectile_copy, false );
        CAPTURE( target_type );
        CAPTURE( weapon_type );
        CAPTURE( ter_id( target_type ).obj().name() );
        CAPTURE( g->m.ter( target_point ).obj().name() );
        if( smashable ) {
            CHECK( g->m.ter( target_point ) != ter_id( target_type ) );
        } else {
            CHECK( g->m.ter( target_point ) == ter_id( target_type ) );
        }
    }
}

static void test_projectile_attack( std::string target_type, bool killable,
                                    dealt_projectile_attack &attack, std::string weapon_type )
{
    for( int i = 0; i < 10; ++i ) {
        monster target{ mtype_id( target_type ), tripoint_zero };
        target.deal_projectile_attack( nullptr, attack, false );
        CAPTURE( target_type );
        CAPTURE( target.get_hp() );
        CAPTURE( target.get_hp_max() );
        CAPTURE( attack.proj.impact.total_damage() );
        CAPTURE( weapon_type );
        CHECK( target.is_dead() == killable );
    }
}

static void test_archery_balance( std::string weapon_type, std::string ammo_type,
                                  std::string killable, std::string unkillable )
{
    item weapon( weapon_type );
    // The standard modern hunting arrow, make this a parameter if we extend to crossbows.
    weapon.ammo_set( ammo_type, 1 );

    projectile test_projectile;
    test_projectile.speed = 1000;
    test_projectile.impact = weapon.gun_damage();
    test_projectile.proj_effects = weapon.ammo_effects();
    test_projectile.critical_multiplier = weapon.ammo_data()->ammo->critical_multiplier;

    dealt_projectile_attack attack {
        test_projectile, nullptr, dealt_damage_instance(), tripoint_zero, accuracy_critical - 0.05
    };
    if( !killable.empty() ) {
        test_projectile_attack( killable, true, attack, weapon_type );
    }
    if( !unkillable.empty() ) {
        test_projectile_attack( unkillable, false, attack, weapon_type );
    }
    test_projectile_hitting_wall( "t_wall", false, attack, weapon_type );
    // Use "can't kill anything" as an indication that it can't break a window either.
    if( !killable.empty() ) {
        test_projectile_hitting_wall( "t_window", true, attack, weapon_type );
    }
}

TEST_CASE( "archery_damage_thresholds", "[balance],[archery]" )
{
    // Selfbow can't kill a turkey
    test_archery_balance( "selfbow", "arrow_metal", "", "mon_turkey" );
    test_archery_balance( "rep_crossbow", "bolt_steel", "", "mon_turkey" );
    // Shortbow can kill turkeys, but not deer
    test_archery_balance( "shortbow", "arrow_metal", "mon_turkey", "mon_deer" );
    test_archery_balance( "hand_crossbow", "bolt_steel", "mon_turkey", "mon_deer" );
    // Fiberglass recurve can kill deer, but not bear
    test_archery_balance( "recurbow", "arrow_metal", "mon_deer", "mon_bear" );
    test_archery_balance( "compositecrossbow", "bolt_steel", "mon_deer", "mon_bear" );
    // Medium setting compound bow can kill Bear
    test_archery_balance( "compbow", "arrow_metal", "mon_bear", "" );
    // High setting modern compund bow can kill Moose
    test_archery_balance( "compcrossbow", "bolt_steel", "mon_moose", "" );
    test_archery_balance( "compbow_high", "arrow_metal", "mon_moose", "" );
}
