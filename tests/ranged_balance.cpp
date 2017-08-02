#include "catch/catch.hpp"

#include "ballistics.h"
#include "dispersion.h"
#include "game.h"
#include "monattack.h"
#include "monster.h"
#include "npc.h"

#include "test_statistics.h"
#include "map_helpers.h"

static void equip_shooter( npc &shooter, itype_id gun_id )
{
    tripoint shooter_pos( 60, 60, 0 );
    shooter.setpos( shooter_pos );
    shooter.worn.clear();
    shooter.inv.clear();
    shooter.remove_weapon();
    // So we don't drop anything.
    shooter.wear_item( item( "backpack" ) );

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
    shooter.wield( gun );
}

std::array<double, 5> accuracy_levels = {{ accuracy_grazing, accuracy_standard, accuracy_goodhit, accuracy_critical, accuracy_headshot }};

static std::array<statistics, 5> firing_test( npc &shooter, std::string gun_type, float aim_ratio,
        int range, std::array<double, 5> thresholds )
{
    equip_shooter( shooter, itype_id( gun_type ) );

    // Spawn a target.
    // Maybe change this to a random direction, or even move the target around during the test.
    tripoint target_pos( shooter.pos() + tripoint( range, 0, 0 ) );
    monster &test_target = spawn_test_monster( "debug_mon", target_pos );
    REQUIRE( test_target.get_size() == MS_MEDIUM );

    item &gun = shooter.weapon;

    dispersion_sources dispersion =
        shooter.get_weapon_dispersion( gun, rl_dist( shooter.pos(), test_target.pos() ) );
    dispersion.add_range( std::max( static_cast<double>( gun.sight_dispersion() ),
                                    MAX_RECOIL * ( 1.0 - aim_ratio ) ) );

    std::array<statistics, 5> firing_stats;
    bool threshold_within_confidence_interval = false;
    do {
        projectile_attack_aim aim = projectile_attack_roll( dispersion, range,
                                    test_target.ranged_target_size() );
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

static void test_shooting_scenario( npc &shooter, std::string gun_type,
                                    int min_quickdraw_range, int min_good_range, int max_good_range )
{
    clear_map();

    {
        std::array<statistics, 5> minimum_stats = firing_test( shooter, gun_type, 0.0, min_quickdraw_range, {{ 0.01, -1, -1, -1, -1 }} );
        INFO( "Accumulated " << minimum_stats[0].n() << " samples." );
        CHECK( minimum_stats[0].avg() < 0.01 );
    }
    {
        std::array<statistics, 5> good_stats = firing_test( shooter, gun_type, 0.9, min_good_range, {{ -1, -1, 0.5, -1, -1 }} );
        INFO( "Accumulated " << good_stats[0].n() << " samples." );
        CHECK( good_stats[2].avg() > 0.5 );
    }
    {
        std::array<statistics, 5> good_stats = firing_test( shooter, gun_type, 1.0, max_good_range, {{ -1, -1, 0.01, -1, -1 }} );
        INFO( "Accumulated " << good_stats[0].n() << " samples." );
        CHECK( good_stats[2].avg() < 0.01 );
    }
}

TEST_CASE( "unskilled_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 0, 8, 8, 8, 8 );
    SECTION( "an unskilled shooter with an inaccurate pistol" ) {
        test_shooting_scenario( shooter, "glock_19", 3, 3, 5 );
    }
    SECTION( "an unskilled shooter with an inaccurate smg" ) {
        test_shooting_scenario( shooter, "mac_10", 3, 5, 8 );
    }
    SECTION( "an unskilled shooter with an inaccurate rifle" ) {
        test_shooting_scenario( shooter, "m1918", 3, 9, 15 );
    }
}

TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 5, 10, 10, 10, 10 );
    SECTION( "a skilled shooter with an accurate pistol" ) {
        test_shooting_scenario( shooter, "sw_619", 4, 7, 10 );
    }
    SECTION( "a skilled shooter with an accurate smg" ) {
        test_shooting_scenario( shooter, "uzi", 4, 10, 15 );
    }
    SECTION( "a skilled shooter with an accurate rifle" ) {
        test_shooting_scenario( shooter, "ruger_mini", 4, 15, 45 );
    }
}

TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 10, 18, 18, 18, 18 );
    SECTION( "an expert shooter with an excellent pistol" ) {
        test_shooting_scenario( shooter, "sw629", 5, 10, 15 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        test_shooting_scenario( shooter, "ppsh", 5, 20, 30 );
    }
    SECTION( "an expert shooter with an excellent rifle" ) {
        test_shooting_scenario( shooter, "browning_blr", 5, 30, 60 );
    }
}
