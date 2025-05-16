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
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "cata_catch.h"
#include "coordinates.h"
#include "damage.h"
#include "game_constants.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "point.h"
#include "projectile.h"
#include "type_id.h"
#include "value_ptr.h"

class Creature;

static const itype_id itype_arrow_metal( "arrow_metal" );
static const itype_id itype_arrow_wood_heavy( "arrow_wood_heavy" );
static const itype_id itype_bolt_makeshift( "bolt_makeshift" );
static const itype_id itype_bolt_steel( "bolt_steel" );
static const itype_id itype_compbow( "compbow" );
static const itype_id itype_compbow_high( "compbow_high" );
static const itype_id itype_compcrossbow( "compcrossbow" );
static const itype_id itype_compositecrossbow( "compositecrossbow" );
static const itype_id itype_hand_crossbow( "hand_crossbow" );
static const itype_id itype_recurbow( "recurbow" );
static const itype_id itype_rep_crossbow( "rep_crossbow" );
static const itype_id itype_selfbow( "selfbow" );
static const itype_id itype_shortbow( "shortbow" );

// In short, a bow should never destroy a wall, pretty simple.
static void test_projectile_hitting_wall( const std::string &target_type, bool smashable,
        dealt_projectile_attack &attack, const itype_id &weapon_type )
{
    static const tripoint_bub_ms target_point{ 5, 5, 0 };
    map &here = get_map();
    for( int i = 0; i < 10; ++i ) {
        projectile projectile_copy = attack.proj;
        here.set( target_point, ter_id( target_type ), furn_id( "f_null" ) );
        CAPTURE( projectile_copy.impact.total_damage() );
        here.shoot( target_point, projectile_copy, false );
        CAPTURE( target_type );
        CAPTURE( weapon_type.c_str() );
        CAPTURE( ter_id( target_type ).obj().name() );
        CAPTURE( here.ter( target_point ).obj().name() );
        if( smashable ) {
            CHECK( here.ter( target_point ) != ter_id( target_type ) );
        } else {
            CHECK( here.ter( target_point ) == ter_id( target_type ) );
        }
    }
}

static void test_projectile_attack( const std::string &target_type, bool killable,
                                    dealt_projectile_attack &attack, const itype_id &weapon_type,
                                    const bool headshot )
{
    for( int i = 0; i < 10; ++i ) {
        monster target{ mtype_id( target_type ), tripoint_bub_ms::zero };
        //the missed_by field is modified by deal_projectile_attack() and must be reset
        attack.missed_by = headshot ? accuracy_headshot * 0.75 : accuracy_critical;
        target.deal_projectile_attack( &get_map(), nullptr, attack, attack.missed_by, false );
        CAPTURE( target_type );
        CAPTURE( target.get_hp() );
        CAPTURE( target.get_hp_max() );
        CAPTURE( attack.proj.impact.total_damage() );
        CAPTURE( weapon_type.c_str() );
        CHECK( target.is_dead() == killable );
    }
}

static void test_archery_balance( const itype_id &weapon_type, const itype_id &ammo_type,
                                  const std::string &killable, const std::string &unkillable,
                                  const bool headshot )
{
    item weapon( weapon_type );
    weapon.ammo_set( ammo_type, 1 );

    projectile test_projectile;
    test_projectile.speed = 1000;
    test_projectile.impact = weapon.gun_damage();
    test_projectile.proj_effects = weapon.ammo_effects();
    test_projectile.critical_multiplier = weapon.ammo_data()->ammo->critical_multiplier;
    std::map<Creature *, std::pair<int, int>> targets_hit;

    dealt_projectile_attack attack {
        test_projectile, nullptr, dealt_damage_instance(), tripoint_bub_ms::zero, accuracy_critical * 0.75,
        false, false, targets_hit
    };
    if( !killable.empty() ) {
        test_projectile_attack( killable, true, attack, weapon_type, headshot );
    }
    if( !unkillable.empty() ) {
        test_projectile_attack( unkillable, false, attack, weapon_type, headshot );
    }
    test_projectile_hitting_wall( "t_wall", false, attack, weapon_type );
    // Use "can't kill anything" as an indication that it can't break a window either.
    if( !killable.empty() ) {
        test_projectile_hitting_wall( "t_window", true, attack, weapon_type );
    }
}

TEST_CASE( "archery_damage_thresholds", "[balance],[archery]" )
{
    // undodgable bolt fired with a guaranteed crit 10 times

    // Selfbow can't kill a turkey
    test_archery_balance( itype_selfbow, itype_arrow_wood_heavy, "", "test_mon_turkey", true );
    test_archery_balance( itype_rep_crossbow, itype_bolt_makeshift, "", "test_mon_turkey", true );
    // Shortbow can kill turkeys, but not deer
    test_archery_balance( itype_shortbow, itype_arrow_metal, "test_mon_turkey", "test_mon_deer", true );
    test_archery_balance( itype_hand_crossbow, itype_bolt_steel, "test_mon_turkey", "test_mon_deer",
                          true );
    // Fiberglass recurve can kill deer with an accurate hit, but not bear
    test_archery_balance( itype_recurbow, itype_arrow_metal, "", "test_mon_deer", false );
    test_archery_balance( itype_compositecrossbow, itype_bolt_steel, "", "test_mon_deer", false );
    test_archery_balance( itype_recurbow, itype_arrow_metal, "test_mon_deer", "test_mon_bear", true );
    test_archery_balance( itype_compositecrossbow, itype_bolt_steel, "test_mon_deer", "test_mon_bear",
                          true );
    // Medium setting compound bow can kill Bear
    test_archery_balance( itype_compbow, itype_arrow_metal, "", "test_mon_bear", false );
    test_archery_balance( itype_compbow, itype_arrow_metal, "test_mon_bear", "", true );
    // High setting modern compund bow can kill Moose
    // Use a 3x damage weakpoint to keep the tests functional.
    test_archery_balance( itype_compcrossbow, itype_bolt_steel, "", "test_mon_moose", false );
    test_archery_balance( itype_compbow_high, itype_arrow_metal, "", "test_mon_moose", false );
    test_archery_balance( itype_compcrossbow, itype_bolt_steel, "test_mon_moose", "", true );
    test_archery_balance( itype_compbow_high, itype_arrow_metal, "test_mon_moose", "", true );
}
