#include <memory>
#include <string>

#include "ballistics.h"
#include "cata_catch.h"
#include "creature.h"
#include "damage.h"
#include "dispersion.h"
#include "map_helpers.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "projectile.h"
#include "ranged.h"

static const efftype_id effect_bite( "bite" );
static const efftype_id effect_sleep( "sleep" );

static const flag_id json_flag_FILTHY( "FILTHY" );

static const mtype_id mon_manhack( "mon_manhack" );

static const int num_iters = 10000;
static constexpr tripoint dude_pos( HALF_MAPSIZE_X + 4, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint mon_pos( HALF_MAPSIZE_X + 3, HALF_MAPSIZE_Y, 0 );
static constexpr tripoint badguy_pos( HALF_MAPSIZE_X + 1, HALF_MAPSIZE_Y, 0 );

static void check_near( std::string subject, float actual, const float expected,
                        const float tolerance )
{
    THEN( string_format( "%s is about %.1f (+/- %.1f)", subject, expected, tolerance ) ) {
        CHECK( actual == Approx( expected ).margin( tolerance ) );
    }
}

static float get_avg_melee_dmg( std::string clothing_id, bool infect_risk = false )
{
    monster zed( mon_manhack, mon_pos );
    standard_npc dude( "TestCharacter", dude_pos, {}, 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    if( infect_risk ) {
        cloth.set_flag( json_flag_FILTHY );
    }
    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( dude, true );
        dude.setpos( dude_pos );
        dude.wear_item( cloth, false );
        dude.add_effect( effect_sleep, 1_hours );
        if( zed.melee_attack( dude ) ) {
            num_hits++;
        }
        cloth.set_damage( cloth.min_damage() );
        if( !infect_risk ) {
            dam_acc += dude.get_hp_max() - dude.get_hp();
        } else if( dude.has_effect( effect_bite ) ) {
            dam_acc++;
        }
        if( dude.is_dead() ) {
            break;
        }
    }
    CAPTURE( dude.is_dead() );
    const std::string ret_type = infect_risk ? "infections" : "damage total";
    INFO( string_format( "%s landed %d hits on character, causing %d %s.", zed.get_name(), num_hits,
                         dam_acc, ret_type ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

static float get_avg_bullet_dmg( std::string clothing_id )
{
    clear_map();
    std::unique_ptr<standard_npc> badguy = std::make_unique<standard_npc>( "TestBaddie", badguy_pos,
                                           std::vector<std::string>(), 0, 8, 8, 8, 8 );
    std::unique_ptr<standard_npc> dude = std::make_unique<standard_npc>( "TestCharacter", dude_pos,
                                         std::vector<std::string>(), 0, 8, 8, 8, 8 );
    item cloth( clothing_id );
    projectile proj;
    proj.speed = 1000;
    proj.impact = damage_instance( damage_type::BULLET, 20 );
    proj.range = 30;
    proj.proj_effects = std::set<std::string>();
    proj.critical_multiplier = 1;

    int dam_acc = 0;
    int num_hits = 0;
    for( int i = 0; i < num_iters; i++ ) {
        clear_character( *dude, true );
        dude->setpos( dude_pos );
        dude->wear_item( cloth, false );
        dude->add_effect( effect_sleep, 1_hours );
        dealt_projectile_attack atk = projectile_attack( proj, badguy_pos, dude_pos, dispersion_sources(),
                                      &*badguy );
        dude->deal_projectile_attack( &*badguy, atk, false );
        if( atk.missed_by < 1.0 ) {
            num_hits++;
        }
        cloth.set_damage( cloth.min_damage() );
        dam_acc += dude->get_hp_max() - dude->get_hp();
        if( dude->is_dead() ) {
            break;
        }
    }
    CAPTURE( dude->is_dead() );
    INFO( string_format( "%s landed %d hits on character, causing %d damage total.",
                         badguy->disp_name( false, true ), num_hits, dam_acc ) );
    num_hits = num_hits ? num_hits : 1;
    return static_cast<float>( dam_acc ) / num_hits;
}

TEST_CASE( "Infections from filthy clothing", "[coverage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( "test_zentai", true );
        check_near( "Infection chance", chance, 0.35f, 0.05f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float chance = get_avg_melee_dmg( "test_zentai_nomelee", true );
        check_near( "Infection chance", chance, 0.0f, 0.0001f );
    }
}

TEST_CASE( "Melee coverage vs. melee damage", "[coverage] [melee] [damage]" )
{
    SECTION( "Full melee and ranged coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( "test_hazmat_suit" );
        check_near( "Average damage", dmg, 7.8f, 0.2f );
    }

    SECTION( "No melee coverage vs. melee attack" ) {
        const float dmg = get_avg_melee_dmg( "test_hazmat_suit_nomelee" );
        check_near( "Average damage", dmg, 14.5f, 0.2f );
    }
}

TEST_CASE( "Ranged coverage vs. bullet", "[coverage] [ranged]" )
{
    SECTION( "Full melee and ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( "test_hazmat_suit" );
        check_near( "Average damage", dmg, 13.6f, 0.2f );
    }

    SECTION( "No ranged coverage vs. ranged attack" ) {
        const float dmg = get_avg_bullet_dmg( "test_hazmat_suit_noranged" );
        check_near( "Average damage", dmg, 17.2f, 0.2f );
    }
}

TEST_CASE( "Proportional armor material resistances", "[material]" )
{
    SECTION( "Mostly steel armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( "test_swat_mostly_steel" );
        check_near( "Average damage", dmg, 3.3f, 0.2f );
    }

    SECTION( "Mostly cotton armor vs. melee" ) {
        const float dmg = get_avg_melee_dmg( "test_swat_mostly_cotton" );
        check_near( "Average damage", dmg, 12.2f, 0.2f );
    }
}
