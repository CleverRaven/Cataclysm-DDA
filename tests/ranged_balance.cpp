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


static void arm_shooter( npc &shooter, itype_id gun_id )
{
    shooter.remove_weapon();

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

static dispersion_sources get_dispersion( npc &shooter, std::string gun_type, int aim_time )
{
    arm_shooter( shooter, itype_id( gun_type ) );

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

static void test_shooting_scenario( npc &shooter, std::string gun_type,
                                    int min_quickdraw_range, int min_good_range, int max_good_range )
{
    {
        dispersion_sources dispersion = get_dispersion( shooter, gun_type, 0 );
        std::array<statistics, 5> minimum_stats = firing_test( dispersion, min_quickdraw_range, {{ 0.1, -1, -1, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_quickdraw_range );
        CHECK( minimum_stats[0].avg() < 0.1 );
    }
    {
        dispersion_sources dispersion = get_dispersion( shooter, gun_type, 200 );
        std::array<statistics, 5> good_stats = firing_test( dispersion, min_good_range, {{ -1, -1, 0.5, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_good_range );
        CHECK( good_stats[2].avg() > 0.5 );
    }
    {
        dispersion_sources dispersion = get_dispersion( shooter, gun_type, 100 );
        std::array<statistics, 5> good_stats = firing_test( dispersion, max_good_range, {{ -1, -1, 0.01, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << max_good_range );
        CHECK( good_stats[2].avg() < 0.01 );
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
        test_shooting_scenario( shooter, "glock_19", 4, 3, 5 );
    }
    SECTION( "an unskilled shooter with an inaccurate smg" ) {
        test_shooting_scenario( shooter, "mac_10", 4, 5, 8 );
    }
    SECTION( "an unskilled shooter with an inaccurate rifle" ) {
        test_shooting_scenario( shooter, "m1918", 4, 9, 15 );
    }
}

TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 5, 10, 10, 10, 10 );
    equip_shooter( shooter, { "cloak_wool", "footrags_wool", "gloves_wraps_fur", "veil_wedding" } );
    assert_encumbrance( shooter, 5 );

    SECTION( "a skilled shooter with an accurate pistol" ) {
        test_shooting_scenario( shooter, "sw_619", 5, 7, 10 );
    }
    SECTION( "a skilled shooter with an accurate smg" ) {
        test_shooting_scenario( shooter, "uzi", 5, 10, 15 );
    }
    SECTION( "a skilled shooter with an accurate rifle" ) {
        test_shooting_scenario( shooter, "ruger_mini", 5, 15, 45 );
    }
}

TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    standard_npc shooter( "Shooter", {}, 10, 18, 18, 18, 18 );
    equip_shooter( shooter, { } );
    assert_encumbrance( shooter, 0 );

    SECTION( "an expert shooter with an excellent pistol" ) {
        test_shooting_scenario( shooter, "sw629", 6, 10, 15 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        test_shooting_scenario( shooter, "ppsh", 6, 20, 30 );
    }
    SECTION( "an expert shooter with an excellent rifle" ) {
        test_shooting_scenario( shooter, "browning_blr", 6, 30, 60 );
    }
}
