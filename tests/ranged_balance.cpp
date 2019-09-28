#include <vector>
#include <array>
#include <list>
#include <ostream>
#include <string>

#include "catch/catch.hpp"
#include "cata_utility.h"
#include "ballistics.h"
#include "creature.h"
#include "dispersion.h"
#include "map_helpers.h"
#include "npc.h"
#include "test_statistics.h"
#include "units.h"
#include "bodypart.h"
#include "calendar.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "json.h"
#include "player.h"
#include "material.h"
#include "type_id.h"
#include "point.h"

using firing_statistics = statistics<bool>;

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

static void arm_shooter( npc &shooter, const std::string &gun_type,
                         const std::vector<std::string> &mods = {} )
{
    shooter.remove_weapon();

    const itype_id &gun_id( gun_type );
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
    for( const auto &mod : mods ) {
        gun.contents.push_back( item( itype_id( mod ) ) );
    }
    shooter.wield( gun );
}

static void equip_shooter( npc &shooter, const std::vector<std::string> &apparel )
{
    const tripoint shooter_pos( 60, 60, 0 );
    CHECK( !shooter.in_vehicle );
    shooter.setpos( shooter_pos );
    shooter.worn.clear();
    shooter.inv.clear();
    for( const std::string article : apparel ) {
        shooter.wear_item( item( article ) );
    }
}

std::array<double, 5> accuracy_levels = {{ accuracy_grazing, accuracy_standard, accuracy_goodhit, accuracy_critical, accuracy_headshot }};

static std::array<firing_statistics, 5> firing_test( const dispersion_sources &dispersion,
        const int range, const std::array<double, 5> &thresholds )
{
    std::array<firing_statistics, 5> firing_stats = {{ Z99_99, Z99_99, Z99_99, Z99_99, Z99_99 }};
    bool threshold_within_confidence_interval = false;
    do {
        // On each trip through the loop, grab a sample attack roll and add its results to
        // the stat object.  Keep sampling until our calculated confidence interval doesn't overlap
        // any thresholds we care about.  This is a mechanism to limit the number of samples
        // we have to accumulate before we declare that the true average is
        // either above or below the threshold.
        const projectile_attack_aim aim = projectile_attack_roll( dispersion, range, 0.5 );
        threshold_within_confidence_interval = false;
        for( int i = 0; i < static_cast<int>( accuracy_levels.size() ); ++i ) {
            firing_stats[i].add( aim.missed_by < accuracy_levels[i] );
            if( thresholds[i] == -1 ) {
                continue;
            }
            // If we've accumulated less than 100 or so samples we have a high risk
            // of reporting a bad result, so pretend we have high error if samples are low.
            if( firing_stats[i].n() < 100 ) {
                threshold_within_confidence_interval = true;
                continue;
            }
            const double error = firing_stats[i].margin_of_error();
            const double avg = firing_stats[i].avg();
            const double threshold = thresholds[i];
            if( avg + error > threshold && avg - error < threshold ) {
                threshold_within_confidence_interval = true;
            }
        }
    } while( threshold_within_confidence_interval && firing_stats[0].n() < 10000000 );
    return firing_stats;
}

static dispersion_sources get_dispersion( npc &shooter, const int aim_time )
{
    item &gun = shooter.weapon;
    dispersion_sources dispersion = shooter.get_weapon_dispersion( gun );

    shooter.moves = aim_time;
    shooter.recoil = MAX_RECOIL;
    // Aim as well as possible within the provided time.
    shooter.aim();
    if( aim_time > 0 ) {
        REQUIRE( shooter.recoil < MAX_RECOIL );
    }
    dispersion.add_range( shooter.recoil );

    return dispersion;
}

static void test_shooting_scenario( npc &shooter, const int min_quickdraw_range,
                                    const int min_good_range, const int max_good_range )
{
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 0 );
        std::array<firing_statistics, 5> minimum_stats = firing_test( dispersion, min_quickdraw_range, {{ 0.2, 0.1, -1, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_quickdraw_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CAPTURE( minimum_stats[0].n() );
        CAPTURE( minimum_stats[0].margin_of_error() );
        CAPTURE( minimum_stats[1].n() );
        CAPTURE( minimum_stats[1].margin_of_error() );
        CHECK( minimum_stats[0].avg() < 0.2 );
        CHECK( minimum_stats[1].avg() < 0.1 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 300 );
        std::array<firing_statistics, 5> good_stats = firing_test( dispersion, min_good_range, {{ -1, -1, 0.5, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << min_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CAPTURE( good_stats[2].n() );
        CAPTURE( good_stats[2].margin_of_error() );
        CHECK( good_stats[2].avg() > 0.5 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 500 );
        std::array<firing_statistics, 5> good_stats = firing_test( dispersion, max_good_range, {{ -1, -1, 0.1, -1, -1 }} );
        INFO( dispersion );
        INFO( "Range: " << max_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
        CAPTURE( good_stats[2].n() );
        CAPTURE( good_stats[2].margin_of_error() );
        CHECK( good_stats[2].avg() < 0.1 );
    }
}

static void test_fast_shooting( npc &shooter, const int moves, float hit_rate )
{
    const int fast_shooting_range = 3;
    const float hit_rate_cap = hit_rate + 0.3;
    const dispersion_sources dispersion = get_dispersion( shooter, moves );
    std::array<firing_statistics, 5> fast_stats = firing_test( dispersion, fast_shooting_range, {{ -1, hit_rate, -1, -1, -1 }} );
    std::array<firing_statistics, 5> fast_stats_upper = firing_test( dispersion, fast_shooting_range, {{ -1, hit_rate_cap, -1, -1, -1 }} );
    INFO( dispersion );
    INFO( "Range: " << fast_shooting_range );
    INFO( "Max aim speed: " << shooter.aim_per_move( shooter.weapon, MAX_RECOIL ) );
    INFO( "Min aim speed: " << shooter.aim_per_move( shooter.weapon, shooter.recoil ) );
    CAPTURE( shooter.weapon.gun_skill().str() );
    CAPTURE( shooter.get_skill_level( shooter.weapon.gun_skill() ) );
    CAPTURE( shooter.get_dex() );
    CAPTURE( to_milliliter( shooter.weapon.volume() ) );
    CAPTURE( fast_stats[1].n() );
    CAPTURE( fast_stats[1].margin_of_error() );
    CHECK( fast_stats[1].avg() > hit_rate );
    CAPTURE( fast_stats_upper[1].n() );
    CAPTURE( fast_stats_upper[1].margin_of_error() );
    CHECK( fast_stats_upper[1].avg() < hit_rate_cap );
}

static void assert_encumbrance( npc &shooter, int encumbrance )
{
    for( const body_part bp : all_body_parts ) {
        INFO( "Body Part: " << body_part_name( bp ) );
        REQUIRE( shooter.encumb( bp ) == encumbrance );
    }
}

TEST_CASE( "unskilled_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", {}, 0, 8, 8, 8, 8 );
    equip_shooter( shooter, { "bastsandals", "armguard_chitin", "armor_chitin", "beekeeping_gloves", "fencing_mask" } );
    assert_encumbrance( shooter, 10 );

    SECTION( "an unskilled shooter with an inaccurate pistol" ) {
        arm_shooter( shooter, "glock_19" );
        test_shooting_scenario( shooter, 4, 5, 15 );
        test_fast_shooting( shooter, 40, 0.3 );
    }
    SECTION( "an unskilled shooter with an inaccurate shotgun" ) {
        arm_shooter( shooter, "shotgun_d" );
        test_shooting_scenario( shooter, 4, 6, 17 );
        test_fast_shooting( shooter, 50, 0.3 );
    }
    SECTION( "an unskilled shooter with an inaccurate smg" ) {
        arm_shooter( shooter, "tommygun" );
        test_shooting_scenario( shooter, 4, 6, 18 );
        test_fast_shooting( shooter, 70, 0.3 );
    }
    SECTION( "an unskilled shooter with an inaccurate rifle" ) {
        arm_shooter( shooter, "m1918" );
        test_shooting_scenario( shooter, 5, 9, 25 );
        test_fast_shooting( shooter, 80, 0.2 );
    }
}

TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", {}, 5, 10, 10, 10, 10 );
    equip_shooter( shooter, { "cloak_wool", "footrags_wool", "gloves_wraps_fur", "glasses_safety", "balclava" } );
    assert_encumbrance( shooter, 5 );

    SECTION( "a skilled shooter with an accurate pistol" ) {
        arm_shooter( shooter, "sw_619", { "red_dot_sight" } );
        test_shooting_scenario( shooter, 10, 15, 33 );
        test_fast_shooting( shooter, 30, 0.5 );
    }
    SECTION( "a skilled shooter with an accurate shotgun" ) {
        arm_shooter( shooter, "ksg", { "red_dot_sight" } );
        test_shooting_scenario( shooter, 9, 15, 33 );
        test_fast_shooting( shooter, 50, 0.5 );
    }
    SECTION( "a skilled shooter with an accurate smg" ) {
        arm_shooter( shooter, "hk_mp5", { "tele_sight" } );
        test_shooting_scenario( shooter, 12, 18, 40 );
        test_fast_shooting( shooter, 40, 0.4 );
    }
    SECTION( "a skilled shooter with an accurate rifle" ) {
        arm_shooter( shooter, "ar15", { "tele_sight" } );
        test_shooting_scenario( shooter, 10, 22, 48 );
        test_fast_shooting( shooter, 85, 0.3 );
    }
}

TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", {}, 10, 20, 20, 20, 20 );
    equip_shooter( shooter, { } );
    assert_encumbrance( shooter, 0 );

    SECTION( "an expert shooter with an excellent pistol" ) {
        arm_shooter( shooter, "sw629", { "pistol_scope" } );
        test_shooting_scenario( shooter, 18, 20, 140 );
        test_fast_shooting( shooter, 20, 0.6 );
    }
    SECTION( "an expert shooter with an auto shotgun" ) {
        arm_shooter( shooter, "abzats", { "holo_sight" } );
        test_shooting_scenario( shooter, 18, 24, 124 );
        test_fast_shooting( shooter, 60, 0.5 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        arm_shooter( shooter, "ppsh", { "holo_sight" } );
        test_shooting_scenario( shooter, 20, 30, 190 );
        test_fast_shooting( shooter, 60, 0.5 );
    }
    SECTION( "an expert shooter with an excellent rifle" ) {
        arm_shooter( shooter, "browning_blr", { "rifle_scope" } );
        test_shooting_scenario( shooter, 25, 60, 900 );
        test_fast_shooting( shooter, 100, 0.4 );
    }
}

static void range_test( const std::array<double, 5> &test_thresholds, bool write_data = false )
{
    std::vector <int> data;
    int index = 0;
    for( index = 0; index < static_cast<int>( accuracy_levels.size() ); ++index ) {
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
            std::array<firing_statistics, 5> stats = firing_test( dispersion_sources( d ), r, test_thresholds );
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
            data.push_back( found_dispersion );
        }
    }
    if( write_data ) {
        const bool similar_to_previous_test_results =
            std::equal( data.begin(), data.end(),
                        Creature::dispersion_for_even_chance_of_good_hit.begin(),
                        Creature::dispersion_for_even_chance_of_good_hit.end(),
        []( const int a, const int b ) -> bool {
            return a > 0 && b > 0 && std::abs( static_cast<float>( a - b ) / b ) < 0.15;
        } );

        if( similar_to_previous_test_results == false ) {
            write_to_file( "./data/json/hit_range.json", [&]( std::ostream & fsa ) {
                JsonOut j_out( fsa );
                j_out.start_array();
                j_out.start_object();
                j_out.member( "type", "hit_range" );
                j_out.member( "even_good", data );
                j_out.end_object();
                j_out.end_array();
            }, _( "hit_range file" ) );
        } else {
            WARN( "Didn't write. Data too similar to previous test results." );
        }
        REQUIRE( similar_to_previous_test_results );
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
        range_test( {{ -1, -1, 0.5, -1, -1 }}, true );
    }
}
