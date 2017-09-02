#include "catch/catch.hpp"

#include "ballistics.h"
#include "dispersion.h"
#include "game.h"
#include "monattack.h"
#include "monster.h"
#include "npc.h"

#include "test_statistics.h"
#include "map_helpers.h"

#include <vector>

template < class T >
std::ostream &operator <<( std::ostream &os, const std::vector<T> &v )
{
    os << "[";
    for( typename std::vector<T>::const_iterator ii = v.begin(); ii != v.end(); ++ii ) {
        os << " " << *ii;
    }
    os << " ]";
    return os;
}

std::ostream &operator<<( std::ostream &stream, const dispersion_sources &sources )
{
    if( !sources.normal_sources.empty() ) {
        stream << "Normal: " << sources.normal_sources << std::endl;
    }
    if( !sources.linear_sources.empty() ) {
        stream << "Linear: " << sources.linear_sources << std::endl;
    }
    if( !sources.multipliers.empty() ) {
        stream << "Mult: " << sources.multipliers << std::endl;
    }
    return stream;
}

static void arm_shooter( npc &shooter, std::string gun_type, std::vector<std::string> mods = {} )
{
    shooter.remove_weapon();

    itype_id gun_id( gun_type );
    // Give shooter a loaded gun of the requested type.
    item &gun = shooter.i_add( item( gun_id ) );
    const itype_id ammo_id = gun.ammo_default();
    if( gun.magazine_integral() ) {
        item &ammo = shooter.i_add( item( ammo_id, calendar::turn, gun.ammo_capacity() ) );
        REQUIRE( gun.is_reloadable_with( ammo_id ) );
        REQUIRE( shooter.can_reload( gun, ammo_id ) );
        gun.reload( shooter, item_location( shooter, &ammo ), gun.ammo_capacity() );
    } else {
        const itype_id magazine_id = gun.magazine_default();
        item &magazine = shooter.i_add( item( magazine_id ) );
        item &ammo = shooter.i_add( item( ammo_id, calendar::turn, magazine.ammo_capacity() ) );
        REQUIRE( magazine.is_reloadable_with( ammo_id ) );
        REQUIRE( shooter.can_reload( magazine, ammo_id ) );
        magazine.reload( shooter, item_location( shooter, &ammo ), magazine.ammo_capacity() );
        gun.reload( shooter, item_location( shooter, &magazine ), magazine.ammo_capacity() );
    }
    for( auto mod : mods ) {
        gun.contents.push_back( item( itype_id( mod ) ) );
    }
    shooter.wield( gun );
}

static void equip_shooter( npc &shooter, std::vector<std::string> apparel )
{
    tripoint shooter_pos( 60, 60, 0 );
    shooter.setpos( shooter_pos );
    shooter.worn.clear();
    shooter.inv.clear();
    for( const std::string article : apparel ) {
        shooter.wear_item( item( article ) );
    }
}

std::array<double, 5> accuracy_levels = {{ accuracy_grazing, accuracy_standard, accuracy_goodhit, accuracy_critical, accuracy_headshot }};

static std::array<statistics, 5> firing_test( dispersion_sources dispersion, int range,
        std::array<double, 5> thresholds )
{
    std::array<statistics, 5> firing_stats;
    bool threshold_within_confidence_interval = false;
    do {
        projectile_attack_aim aim = projectile_attack_roll( dispersion, range, 0.5 );
        threshold_within_confidence_interval = false;
        for( int i = 0; i < accuracy_levels.size(); ++i ) {
            firing_stats[i].add( aim.missed_by < accuracy_levels[i] );
            if( thresholds[i] == -1 ) {
                continue;
            }
            if( firing_stats[i].n() < 100 ) {
                threshold_within_confidence_interval = true;
            }
            double error = firing_stats[i].adj_wald_error();
            double avg = firing_stats[i].avg();
            double threshold = thresholds[i];
            if( avg + error > threshold && avg - error < threshold ) {
                threshold_within_confidence_interval = true;
            }
        }
    } while( threshold_within_confidence_interval && firing_stats[0].n() < 100000 );
    return firing_stats;
}

static dispersion_sources get_dispersion( npc &shooter, int aim_time )
{
    item &gun = shooter.weapon;
    dispersion_sources dispersion = shooter.get_weapon_dispersion( gun );

    // The 10 is an arbitrary amount under which NPCs refuse to spend moves on aiming.
    shooter.moves = 10 + aim_time;
    shooter.recoil = MAX_RECOIL;
    // Aim as well as possible within the provided time.
    shooter.aim();
    if( aim_time > 0 ) {
        REQUIRE( shooter.recoil < MAX_RECOIL );
    }
    dispersion.add_range( shooter.recoil );

    return dispersion;
}

static void test_shooting_scenario( npc &shooter, int min_quickdraw_range,
                                    int min_good_range, int max_good_range )
{
    {
        dispersion_sources dispersion = get_dispersion( shooter, 0 );
        std::array<statistics, 5> minimum_stats = firing_test( dispersion, min_quickdraw_range, {{ 0.2, 0.1, -1, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_quickdraw_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CHECK( minimum_stats[0].avg() < 0.2 );
        CHECK( minimum_stats[1].avg() < 0.1 );
    }
    {
        dispersion_sources dispersion = get_dispersion( shooter, 190 );
        std::array<statistics, 5> good_stats = firing_test( dispersion, min_good_range, {{ -1, -1, 0.5, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CHECK( good_stats[2].avg() > 0.5 );
    }
    {
        dispersion_sources dispersion = get_dispersion( shooter, 300 );
        std::array<statistics, 5> good_stats = firing_test( dispersion, max_good_range, {{ -1, -1, 0.1, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << max_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CHECK( good_stats[2].avg() < 0.1 );
    }
}

void assert_encumbrance( npc &shooter, int encumbrance )
{
    for( body_part bp : bp_aBodyPart ) {
        INFO( "Body Part: " << body_part_name( bp ) );
        REQUIRE( shooter.encumb( bp ) == encumbrance );
    }
}

TEST_CASE( "unskilled_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 0, 8, 8, 8, 8 );
    equip_shooter( shooter, { "bastsandals", "armguard_chitin", "armor_chitin", "beekeeping_gloves", "fencing_mask" } );
    assert_encumbrance( shooter, 10 );

    SECTION( "an unskilled shooter with an inaccurate pistol" ) {
        arm_shooter( shooter, "glock_19" );
        test_shooting_scenario( shooter, 4, 3, 5 );
    }
    SECTION( "an unskilled shooter with an inaccurate smg" ) {
        arm_shooter( shooter, "tommygun", { "holo_sight", "tuned_mechanism" } );
        test_shooting_scenario( shooter, 4, 5, 8 );
    }
    SECTION( "an unskilled shooter with an inaccurate rifle" ) {
        arm_shooter( shooter, "m1918", { "rifle_scope", "tuned_mechanism" } );
        test_shooting_scenario( shooter, 4, 9, 15 );
    }
}

TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 5, 10, 10, 10, 10 );
    equip_shooter( shooter, { "cloak_wool", "footrags_wool", "gloves_wraps_fur", "veil_wedding" } );
    assert_encumbrance( shooter, 5 );

    SECTION( "a skilled shooter with an accurate pistol" ) {
        arm_shooter( shooter, "sw_619", { "holo_sight", "pistol_grip", "tuned_mechanism" } );
        test_shooting_scenario( shooter, 5, 7, 15 );
    }
    SECTION( "a skilled shooter with an accurate smg" ) {
        arm_shooter( shooter, "hk_mp5", { "pistol_scope", "barrel_big", "match_trigger", "adjustable_stock" } );
        test_shooting_scenario( shooter, 5, 10, 15 );
    }
    SECTION( "a skilled shooter with an accurate rifle" ) {
        arm_shooter( shooter, "ruger_mini", { "rifle_scope", "tuned_mechanism" } );
        test_shooting_scenario( shooter, 5, 15, 45 );
    }
}

TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 10, 18, 18, 18, 18 );
    equip_shooter( shooter, { } );
    assert_encumbrance( shooter, 0 );

    SECTION( "an expert shooter with an excellent pistol" ) {
        arm_shooter( shooter, "sw629", { "holo_sight", "match_trigger" } );
        test_shooting_scenario( shooter, 6, 10, 25 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        arm_shooter( shooter, "ppsh", { "pistol_scope", "barrel_big" } );
        test_shooting_scenario( shooter, 6, 20, 45 );
    }
    SECTION( "an expert shooter with an excellent rifle" ) {
        arm_shooter( shooter, "browning_blr", { "rifle_scope" } );
        test_shooting_scenario( shooter, 6, 30, 60 );
    }
}

static void range_test( std::array<double, 5> test_thresholds )
{
    int index = 0;
    for( index = 0; index < accuracy_levels.size(); ++index ) {
        if( test_thresholds[index] >= 0 ) {
            break;
        }
    }
    // Start at an absurdly high dispersion and count down.
    int prev_dispersion = 6000;
    for( int r = 1; r <= 60; ++r ) {
        int found_dispersion = -1;
        // We carry forward prev_dispersion because we never expet the next tier of range to hit the target accuracy level with a lower dispersion.
        for( int d = prev_dispersion; d >= 0; --d ) {
            std::array<statistics, 5> stats = firing_test( dispersion_sources( d ), r, test_thresholds );
            // Switch this from INFO to WARN to debug the scanning process itself.
            INFO( "Samples: " << stats[index].n() << " Range: " << r << " Dispersion: " << d <<
                  " avg hit rate: " << stats[2].avg() );
            if( stats[index].avg() > test_thresholds[index] ) {
                found_dispersion = d;
                prev_dispersion = d;
                break;
            }
            // The intent here is to skip over dispersion values proportionally to how far from converging we are.
            // As long as we check several adjacent dispersion values before a hit, we're good.
            d -= int( ( test_thresholds[index] - stats[index].avg() ) * 10 ) * 10;
        }
        if( found_dispersion == -1.0 ) {
            WARN( "No matching dispersion found" );
        } else {
            WARN( "Range: " << r << " Dispersion: " << found_dispersion );
        }
    }
}

// I added this to find inflection points where accuracy at a particular range crosses a threshold.
// I don't see any assertions we can make about these thresholds offhand.
TEST_CASE( "synthetic_range_test", "[.]" )
{
    SECTION( "quickdraw thresholds" ) {
        range_test( {{ 0.1, -1, -1, -1, -1 }} );
    }
    SECTION( "max range thresholds" ) {
        range_test( {{ -1, -1, 0.1, -1, -1 }} );
    }
    SECTION( "good hit thresholds" ) {
        range_test( {{ -1, -1, 0.5, -1, -1 }} );
    }
}
