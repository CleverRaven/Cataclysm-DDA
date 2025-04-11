#include <algorithm>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "anatomy.h"
#include "ballistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character_attire.h"
#include "coordinates.h"
#include "creature.h"
#include "dispersion.h"
#include "game_constants.h"
#include "item.h"
#include "item_location.h"
#include "json.h"
#include "map.h"
#include "map_helpers.h"
#include "math_parser_diag_value.h"
#include "monster.h"
#include "npc.h"
#include "player_helpers.h"
#include "point.h"
#include "ranged.h"
#include "rng.h"
#include "test_statistics.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"

static const character_modifier_id
character_modifier_ranged_dispersion_manip_mod( "ranged_dispersion_manip_mod" );

static const itype_id itype_556_m855a1( "556_m855a1" );
static const itype_id itype_M24( "M24" );
static const itype_id itype_armguard_hard( "armguard_hard" );
static const itype_id itype_armguard_soft( "armguard_soft" );
static const itype_id itype_arrow_cf( "arrow_cf" );
static const itype_id itype_arrow_field_point_fletched( "arrow_field_point_fletched" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_balclava( "balclava" );
static const itype_id itype_bastsandals( "bastsandals" );
static const itype_id itype_beekeeping_gloves( "beekeeping_gloves" );
static const itype_id itype_bolt_cf( "bolt_cf" );
static const itype_id itype_bolt_makeshift( "bolt_makeshift" );
static const itype_id itype_bolt_steel( "bolt_steel" );
static const itype_id itype_bow_sight( "bow_sight" );
static const itype_id itype_bow_sight_pin( "bow_sight_pin" );
static const itype_id itype_choke( "choke" );
static const itype_id itype_cloak_wool( "cloak_wool" );
static const itype_id itype_compbow_high( "compbow_high" );
static const itype_id itype_compcrossbow( "compcrossbow" );
static const itype_id itype_compositecrossbow( "compositecrossbow" );
static const itype_id itype_cowboy_hat( "cowboy_hat" );
static const itype_id itype_crossbow( "crossbow" );
static const itype_id itype_debug_modular_m4_carbine( "debug_modular_m4_carbine" );
static const itype_id itype_fn_p90( "fn_p90" );
static const itype_id itype_footrags_wool( "footrags_wool" );
static const itype_id itype_glasses_safety( "glasses_safety" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_gloves_wraps_fur( "gloves_wraps_fur" );
static const itype_id itype_hk_mp5( "hk_mp5" );
static const itype_id itype_holo_sight( "holo_sight" );
static const itype_id itype_m14ebr( "m14ebr" );
static const itype_id itype_mask_guy_fawkes( "mask_guy_fawkes" );
static const itype_id itype_mossberg_590( "mossberg_590" );
static const itype_id itype_mossberg_930( "mossberg_930" );
static const itype_id itype_mp40semi( "mp40semi" );
static const itype_id itype_recurbow( "recurbow" );
static const itype_id itype_red_dot_sight( "red_dot_sight" );
static const itype_id itype_remington_870( "remington_870" );
static const itype_id itype_rifle_scope( "rifle_scope" );
static const itype_id itype_shortbow( "shortbow" );
static const itype_id itype_shot_00( "shot_00" );
static const itype_id itype_shot_bird( "shot_bird" );
static const itype_id itype_shotgun_s( "shotgun_s" );
static const itype_id itype_sw629( "sw629" );
static const itype_id itype_sw_619( "sw_619" );
static const itype_id itype_tele_sight( "tele_sight" );
static const itype_id itype_test_armor_chitin( "test_armor_chitin" );
static const itype_id itype_test_shot_00_fire_damage( "test_shot_00_fire_damage" );

using firing_statistics = statistics<bool>;

class Threshold
{
    public:
        Threshold( const double accuracy, const double chance )
            : _accuracy( accuracy ), _chance( chance ) {
        }
        double accuracy() const {
            return _accuracy;
        }
        double chance() const {
            return _chance;
        }
    private:
        double _accuracy;
        double _chance;
};

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

static firing_statistics firing_test( const dispersion_sources &dispersion,
                                      const int range, const Threshold &threshold )
{
    firing_statistics firing_stats( Z99_99 );
    bool threshold_within_confidence_interval = false;
    do {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        dispersion_sources local_dispersion = dispersion;
        // On each trip through the loop, grab a sample attack roll and add its results to
        // the stat object.  Keep sampling until our calculated confidence interval doesn't overlap
        // any thresholds we care about.  This is a mechanism to limit the number of samples
        // we have to accumulate before we declare that the true average is
        // either above or below the threshold.
        const projectile_attack_aim aim = projectile_attack_roll( local_dispersion, range, 0.5 );
        threshold_within_confidence_interval = false;
        firing_stats.add( aim.missed_by < threshold.accuracy() );
        if( firing_stats.n() < 100 ) {
            threshold_within_confidence_interval = true;
        }
        const double error = firing_stats.margin_of_error();
        const double avg = firing_stats.avg();
        if( avg + error > threshold.chance() && avg - error < threshold.chance() ) {
            threshold_within_confidence_interval = true;
        }
    } while( threshold_within_confidence_interval && firing_stats.n() < 10000000 );
    return firing_stats;
}

static std::vector<firing_statistics> firing_test( const dispersion_sources &dispersion,
        const int range, const std::vector< Threshold > &thresholds )
{
    std::vector<firing_statistics> firing_stats;
    firing_stats.reserve( thresholds.size() );
    for( const Threshold &pear : thresholds ) {
        firing_stats.push_back( firing_test( dispersion, range, pear ) );
    }
    return firing_stats;
}

static dispersion_sources get_dispersion( npc &shooter, const int aim_time, int range )
{
    item_location gun = shooter.get_wielded_item();
    dispersion_sources dispersion = shooter.get_weapon_dispersion( *gun );

    shooter.set_moves( aim_time );
    shooter.recoil = MAX_RECOIL;
    // Aim as well as possible within the provided time.
    shooter.aim( Target_attributes( range, 0.5, 0.0f, true ) );
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
        const dispersion_sources dispersion = get_dispersion( shooter, 0, min_quickdraw_range );
        std::vector<firing_statistics> minimum_stats = firing_test( dispersion, min_quickdraw_range, {
            Threshold( accuracy_grazing, 0.2 ),
            Threshold( accuracy_standard, 0.1 )
        } );
        INFO( dispersion );
        INFO( "Range: " << min_quickdraw_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.get_modifier( character_modifier_ranged_dispersion_manip_mod ) );
        CAPTURE( minimum_stats[0].n() );
        CAPTURE( minimum_stats[0].margin_of_error() );
        CAPTURE( minimum_stats[1].n() );
        CAPTURE( minimum_stats[1].margin_of_error() );
        CHECK( minimum_stats[0].avg() < 0.2 );
        CHECK( minimum_stats[1].avg() < 0.1 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 300, min_good_range );
        firing_statistics good_stats = firing_test( dispersion, min_good_range, Threshold( accuracy_goodhit,
                                       0.5 ) );
        INFO( dispersion );
        INFO( "Range: " << min_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.get_modifier( character_modifier_ranged_dispersion_manip_mod ) );
        CAPTURE( good_stats.n() );
        CAPTURE( good_stats.margin_of_error() );
        CHECK( good_stats.avg() > 0.0 );
    }
    {
        const dispersion_sources dispersion = get_dispersion( shooter, 500, max_good_range );
        firing_statistics good_stats = firing_test( dispersion, max_good_range, Threshold( accuracy_goodhit,
                                       0.1 ) );
        INFO( dispersion );
        INFO( "Range: " << max_good_range );
        INFO( "Max aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), MAX_RECOIL ) );
        INFO( "Min aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), shooter.recoil ) );
        CAPTURE( shooter.get_modifier( character_modifier_ranged_dispersion_manip_mod ) );
        CAPTURE( good_stats.n() );
        CAPTURE( good_stats.margin_of_error() );
        CHECK( good_stats.avg() < 0.1 );
    }
}

static void test_fast_shooting( npc &shooter, const int moves, float hit_rate )
{
    const int fast_shooting_range = 3;
    const float hit_rate_cap = hit_rate + 0.5f;
    const dispersion_sources dispersion = get_dispersion( shooter, moves, fast_shooting_range );
    firing_statistics fast_stats = firing_test( dispersion, fast_shooting_range,
                                   Threshold( accuracy_standard, hit_rate ) );
    firing_statistics fast_stats_upper = firing_test( dispersion, fast_shooting_range,
                                         Threshold( accuracy_standard, hit_rate_cap ) );
    INFO( dispersion );
    INFO( "Range: " << fast_shooting_range );
    INFO( "Max aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), MAX_RECOIL ) );
    INFO( "Min aim speed: " << shooter.aim_per_move( *shooter.get_wielded_item(), shooter.recoil ) );
    CAPTURE( shooter.get_modifier( character_modifier_ranged_dispersion_manip_mod ) );
    CAPTURE( shooter.get_wielded_item()->gun_skill().str() );
    CAPTURE( shooter.get_skill_level( shooter.get_wielded_item()->gun_skill() ) );
    CAPTURE( shooter.get_dex() );
    CAPTURE( to_milliliter( shooter.get_wielded_item()->volume() ) );
    CAPTURE( fast_stats.n() );
    CAPTURE( fast_stats.margin_of_error() );
    CHECK( fast_stats.avg() > hit_rate );
    CAPTURE( fast_stats_upper.n() );
    CAPTURE( fast_stats_upper.margin_of_error() );
    CHECK( fast_stats_upper.avg() <= hit_rate_cap );
}

static void assert_encumbrance( npc &shooter, int encumbrance )
{
    for( const bodypart_id &bp : shooter.get_all_body_parts() ) {
        INFO( "Body Part: " << body_part_name( bp ) );
        REQUIRE( shooter.encumb( bp ) == encumbrance );
    }
}

static constexpr tripoint_bub_ms shooter_pos( 60, 60, 0 );

// Test the aiming speed and accuracy of the weapon in the first shot

// When encountering an enemy suddenly, the pistol can fire the first shot quickly, but the pistol does not perform well at a long distance.
// There is no essential difference in the aiming speed of the first shot of submachine guns, shotguns, rifles, and crossbows.
// It is only related to the size of the gun.
// SMGs are not designed to fire semi-automatically, so in this test, they will not perform well.
// The advantage of shotguns can cause extremely high damage to unarmored targets in close-range shooting, even without precise aiming, so their performance in this test will not be very good.
// The crossbows are only alternative weapons, and their performance is worse than that of guns.
// Sniper rifles and carbines are tested separately.
// Sniper rifles have slow aiming speed but good accuracy, while carbine performance is close to SMGs.
// The aiming speed of the bowand arrow is very relevant to the skill, and an excellent archer can aim quickly

// To simulate the player who just started the game.
// The gun used for the test is the easiest for players to find.
TEST_CASE( "unskilled_shooter_accuracy", "[ranged] [balance] [slow]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 0, 8, 8, 8, 7 );
    shooter.set_body();
    shooter.worn.wear_item( shooter, item( itype_backpack ), false, false );
    equip_shooter( shooter, { itype_bastsandals, itype_armguard_hard, itype_armguard_soft, itype_test_armor_chitin, itype_beekeeping_gloves, itype_mask_guy_fawkes, itype_cowboy_hat } );
    assert_encumbrance( shooter, 10 );

    SECTION( "an unskilled shooter with a common pistol" ) {
        arm_shooter( shooter, itype_glock_19 );
        test_shooting_scenario( shooter, 4, 5, 17 );
        test_fast_shooting( shooter, 60, 0.15 );
    }
    SECTION( "an unskilled archer with a common bow" ) {
        arm_shooter( shooter, itype_shortbow, { itype_bow_sight_pin }, itype_arrow_field_point_fletched );
        test_shooting_scenario( shooter, 4, 4, 13 );
        test_fast_shooting( shooter, 90, 0.1 );
    }
    SECTION( "an unskilled archer with a common crossbow" ) {
        arm_shooter( shooter, itype_crossbow, {}, itype_bolt_makeshift );
        test_shooting_scenario( shooter, 4, 5, 17 );
        test_fast_shooting( shooter, 80, 0.15 );
    }
    SECTION( "an unskilled shooter with a common shotgun" ) {
        arm_shooter( shooter, itype_remington_870 );
        test_shooting_scenario( shooter, 4, 4, 19 );
        test_fast_shooting( shooter, 80, 0.15 );
    }
    SECTION( "an unskilled shooter with a common smg" ) {
        arm_shooter( shooter, itype_mp40semi );
        test_shooting_scenario( shooter, 4, 5, 18 );
        test_fast_shooting( shooter, 80, 0.04 );
    }
    SECTION( "an unskilled shooter with a common rifle" ) {
        arm_shooter( shooter, itype_debug_modular_m4_carbine );
        test_shooting_scenario( shooter, 5, 5, 25 );
        test_fast_shooting( shooter, 100, 0.15 );
    }
}

// To simulate players who already have sufficient proficiency and equipment
TEST_CASE( "competent_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 5, 10, 10, 10, 10 );
    shooter.set_body();
    equip_shooter( shooter, { itype_cloak_wool, itype_footrags_wool, itype_gloves_wraps_fur, itype_glasses_safety, itype_balclava } );
    assert_encumbrance( shooter, 5 );

    SECTION( "a skilled shooter with an accurate pistol" ) {
        arm_shooter( shooter, itype_sw_619, { itype_red_dot_sight } );
        test_shooting_scenario( shooter, 10, 12, 35 );
        test_fast_shooting( shooter, 40, 0.35 );
    }
    SECTION( "a skilled archer with an accurate bow" ) {
        arm_shooter( shooter, itype_recurbow, { itype_bow_sight } );
        test_shooting_scenario( shooter, 8, 10, 35 );
        test_fast_shooting( shooter, 70, 0.35 );
    }
    SECTION( "a skilled archer with an accurate crossbow" ) {
        arm_shooter( shooter, itype_compositecrossbow, { itype_tele_sight }, itype_bolt_steel );
        test_shooting_scenario( shooter, 9, 10, 35 );
        test_fast_shooting( shooter, 70, 0.35 );
    }
    SECTION( "a skilled shooter with a nice shotgun" ) {
        arm_shooter( shooter, itype_mossberg_590 );
        test_shooting_scenario( shooter, 9, 12, 35 );
        test_fast_shooting( shooter, 70, 0.35 );
    }
    SECTION( "a skilled shooter with a nice smg" ) {
        arm_shooter( shooter, itype_hk_mp5, { itype_red_dot_sight } );
        test_shooting_scenario( shooter, 9, 12, 35 );
        test_fast_shooting( shooter, 80, 0.3 );
    }
    SECTION( "a skilled shooter with a carbine" ) {
        arm_shooter( shooter, itype_debug_modular_m4_carbine, { itype_red_dot_sight }, itype_556_m855a1 );
        test_shooting_scenario( shooter, 10, 15, 48 );
        test_fast_shooting( shooter, 80, 0.3 );
    }
    SECTION( "a skilled shooter with an available sniper rifle" ) {
        arm_shooter( shooter, itype_M24 );
        test_shooting_scenario( shooter, 10, 10, 80 );
        test_fast_shooting( shooter, 80, 0.4 );
    }
}

// To simulate hero
TEST_CASE( "expert_shooter_accuracy", "[ranged] [balance]" )
{
    clear_map();
    standard_npc shooter( "Shooter", shooter_pos, {}, 10, 20, 20, 20, 20 );
    shooter.set_body();
    equip_shooter( shooter, { } );
    assert_encumbrance( shooter, 0 );

    SECTION( "an expert shooter with an excellent pistol" ) {
        arm_shooter( shooter, itype_sw629, { itype_holo_sight } );
        test_shooting_scenario( shooter, 18, 20, 140 );
        test_fast_shooting( shooter, 35, 0.5 );
    }
    SECTION( "an expert archer with an excellent bow" ) {
        arm_shooter( shooter, itype_compbow_high, { itype_holo_sight }, itype_arrow_cf );
        test_shooting_scenario( shooter, 12, 20, 80 );
        test_fast_shooting( shooter, 50, 0.4 );
    }
    SECTION( "an expert archer with an excellent crossbow" ) {
        arm_shooter( shooter, itype_compcrossbow, { itype_holo_sight }, itype_bolt_cf );
        test_shooting_scenario( shooter, 12, 20, 100 );
        test_fast_shooting( shooter, 50, 0.4 );
    }
    SECTION( "an expert shooter with an excellent shotgun" ) {
        arm_shooter( shooter, itype_mossberg_930, { itype_holo_sight } );
        test_shooting_scenario( shooter, 18, 24, 124 );
        test_fast_shooting( shooter, 60, 0.5 );
    }
    SECTION( "an expert shooter with an excellent smg" ) {
        arm_shooter( shooter, itype_fn_p90, { itype_holo_sight } );
        test_shooting_scenario( shooter, 20, 30, 190 );
        test_fast_shooting( shooter, 60, 0.5 );
    }
    SECTION( "an expert shooter with an excellent rifle with holo_sight" ) {
        arm_shooter( shooter, itype_m14ebr, { itype_holo_sight } );
        test_shooting_scenario( shooter, 30, 40, 500 );
        test_fast_shooting( shooter, 65, 0.5 );
    }
    SECTION( "an expert shooter with an excellent rifle with rifle_scope" ) {
        arm_shooter( shooter, itype_m14ebr, { itype_rifle_scope } );
        test_shooting_scenario( shooter, 25, 40, 1000 );
        test_fast_shooting( shooter, 65, 0.5 );
    }
}

static void range_test( const Threshold &test_threshold, bool write_data = false )
{
    std::vector <int> data;
    // Start at an absurdly high dispersion and count down.
    int prev_dispersion = 6000;
    for( int r = 1; r <= 60; ++r ) {
        int found_dispersion = -1;
        // We carry forward prev_dispersion because we never expet the next tier of range to hit the target accuracy level with a lower dispersion.
        for( int d = prev_dispersion; d >= 0; --d ) {
            firing_statistics stats = firing_test( dispersion_sources( d ), r, test_threshold );
            // Switch this from INFO to WARN to debug the scanning process itself.
            INFO( "Samples: " << stats.n() << " Range: " << r << " Dispersion: " << d <<
                  " avg hit rate: " << stats.avg() );
            if( stats.avg() > test_threshold.chance() ) {
                found_dispersion = d;
                prev_dispersion = d;
                break;
            }
            // The intent here is to skip over dispersion values proportionally to how far from converging we are.
            // As long as we check several adjacent dispersion values before a hit, we're good.
            d -= static_cast<int>( ( 1 - ( stats.avg() / test_threshold.chance() ) ) * 15 ) * 5;
        }
        if( found_dispersion == -1 ) {
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

        if( !similar_to_previous_test_results ) {
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
            WARN( "Didn't write.  Data too similar to previous test results." );
        }
        REQUIRE( similar_to_previous_test_results );
    }
}

// I added this to find inflection points where accuracy at a particular range crosses a threshold.
// I don't see any assertions we can make about these thresholds offhand.
TEST_CASE( "synthetic_range_test", "[.]" )
{
    SECTION( "quickdraw thresholds" ) {
        range_test( Threshold( accuracy_grazing, 0.1 ) );
    }
    SECTION( "max range thresholds" ) {
        range_test( Threshold( accuracy_goodhit, 0.1 ) );
    }
    SECTION( "good hit thresholds" ) {
        range_test( Threshold( accuracy_goodhit, 0.5 ), true );
    }
}

static void shoot_monster( const itype_id &gun_type, const std::vector<itype_id> &mods,
                           const itype_id &ammo_type, int range,
                           int expected_damage, const std::string &monster_type,
                           const std::function<bool ( const standard_npc &, const monster & )> &other_checks = nullptr )
{
    map &here = get_map();
    clear_map();
    statistics<int> damage;
    constexpr tripoint_bub_ms shooter_pos{ 60, 60, 0 };
    const tripoint_bub_ms monster_pos = shooter_pos + ( point::east * range );
    std::unique_ptr<standard_npc> shooter = std::make_unique<standard_npc>( "Shooter",
                                            shooter_pos,
                                            std::vector<itype_id>(), 5, 10, 10, 10, 10 );
    int other_check_success = 0;
    do {
        shooter->set_body();
        arm_shooter( *shooter, gun_type, mods, ammo_type );
        shooter->recoil = 0;
        monster &mon = spawn_test_monster( monster_type, monster_pos, false );
        const int prev_HP = mon.get_hp();
        shooter->fire_gun( here, monster_pos, 1, *shooter->get_wielded_item() );
        damage.add( prev_HP - mon.get_hp() );
        if( other_checks ) {
            other_check_success += other_checks( *shooter, mon );
        }
        if( damage.margin_of_error() < 0.05 && damage.n() > 500 ) {
            break;
        }
        mon.die( &here, nullptr );
    } while( damage.n() < 1000 ); // In fact, stable results can only be obtained when n reaches 10000
    const double avg = damage.avg();
    CAPTURE( damage.n() );
    CAPTURE( gun_type );
    CAPTURE( mods );
    CAPTURE( ammo_type );
    CAPTURE( range );
    CAPTURE( monster_type );
    CAPTURE( avg );
    CHECK( avg == Approx( expected_damage ).margin( 10 ) );
    if( other_checks ) {
        CHECK( other_check_success == Approx( damage.n() ).margin( 10 ) );
    }
}

TEST_CASE( "shot_features", "[gun]" "[slow]" )
{
    clear_map();
    // BIRDSHOT
    // Unarmored target
    // Minor damage at range.
    // More serious damage at close range.
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 5, 20, "mon_test_shotgun_0_bullet" );
    // Grevious damage at point blank.
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 1, 73, "mon_test_shotgun_0_bullet" );

    // Triviallly armored target (armor_bullet: 1)
    // Can rarely if ever inflict damage at range.
    // shoot_monster( itype_shot_bird, 10, 0, 5, "mon_zombie_tough" );
    // Can barely hurt at close range.
    // Can seriously injure trivially armored enemy at point blank,
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 1, 72, "mon_test_shotgun_1_bullet" );

    // Armored target (armor_bullet: 5)
    // Can't hurt at range
    // Can't hurt at close range.
    // Serioualy injure at point blank.
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 1, 65, "mon_test_shotgun_5_bullet" );
    // TODO: can't harm heavily armored enemies at point blank

    // Heavily Armored target (armor_bullet: 30)
    // Can't hurt at range,
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 12, 0, "mon_test_shotgun_30_bullet" );
    // Can't hurt at close range.
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 5, 0, "mon_test_shotgun_30_bullet" );
    // Barely injure at point blank.
    shoot_monster( itype_shotgun_s, {}, itype_shot_bird, 1, 30, "mon_test_shotgun_30_bullet" );
    // TODO: can't harm heavily armored enemies even at point blank.

    // BUCKSHOT
    // Unarmored target
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 18, 75, "mon_test_shotgun_0_bullet" );
    // Heavy damage at range.
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 12, 115, "mon_test_shotgun_0_bullet" );
    // More damage at close range.
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 5, 171, "mon_test_shotgun_0_bullet" );
    // Extreme damage at point blank range.
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 1, 86, "mon_test_shotgun_0_bullet" );

    // Lightly armored target (armor_bullet: 5)
    // Outcomes for lightly armored enemies are very similar.
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 18, 45, "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 12, 77, "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 5, 113, "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 1, 81, "mon_test_shotgun_5_bullet" );

    // Armored target (armor_bullet: 10)
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 18, 22, "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 12, 35, "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 5, 55, "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, {}, itype_shot_00, 1, 75, "mon_test_shotgun_10_bullet" );
}

TEST_CASE( "shot_features_with_choke", "[gun]" "[slow]" )
{
    clear_map_and_put_player_underground();
    // Unarmored target
    // This test result is difficult to converge
    // After more attempts, the average value is about 7
    // shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 18, 7, "mon_test_shotgun_0_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 12, 13,
                   "mon_test_shotgun_0_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 5, 20,
                   "mon_test_shotgun_0_bullet" );
    // All the results of tests at point blank are abonormal
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 1, 75,
                   "mon_test_shotgun_0_bullet" );

    // Triviallly armored target (armor_bullet: 1)
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 1, 74,
                   "mon_test_shotgun_1_bullet" );

    // Armored target (armor_bullet: 5)
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_bird, 1, 67,
                   "mon_test_shotgun_5_bullet" );

    // Unarmored target
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 18, 95,
                   "mon_test_shotgun_0_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 12, 145,
                   "mon_test_shotgun_0_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 5, 182,
                   "mon_test_shotgun_0_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 1, 87,
                   "mon_test_shotgun_0_bullet" );
    // Triviallly armored target (armor_bullet: 1)
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 18, 94,
                   "mon_test_shotgun_1_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 12, 135,
                   "mon_test_shotgun_1_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 5, 170,
                   "mon_test_shotgun_1_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 1, 87,
                   "mon_test_shotgun_1_bullet" );
    // Armored target (armor_bullet: 5)
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 18, 62,
                   "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 12, 95,
                   "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 5, 121,
                   "mon_test_shotgun_5_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 1, 80,
                   "mon_test_shotgun_5_bullet" );
    // Armored target (armor_bullet: 10)
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 18, 30,
                   "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 12, 44,
                   "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 5, 55,
                   "mon_test_shotgun_10_bullet" );
    shoot_monster( itype_shotgun_s, { itype_choke }, itype_shot_00, 1, 73,
                   "mon_test_shotgun_10_bullet" );
}

TEST_CASE( "shot_custom_damage_type", "[gun]" "[slow]" )
{
    clear_map();
    auto check_eocs = []( const standard_npc & src, const monster & tgt ) {
        return src.get_value( "general_dmg_type_test_test_fire" ) == "source" &&
               tgt.get_value( "general_dmg_type_test_test_fire" ) == "target";
    };
    // Check that ballistics damage processes weird damage types and on-hit EOCs
    // note: shotguns can miss one-shot critical hit rarely
    shoot_monster( itype_shotgun_s, {}, itype_test_shot_00_fire_damage, 1, 80,
                   "mon_test_zombie", check_eocs );
    shoot_monster( itype_shotgun_s, {}, itype_test_shot_00_fire_damage, 1, 80,
                   "mon_test_fire_resist", check_eocs );
    shoot_monster( itype_shotgun_s, {}, itype_test_shot_00_fire_damage, 1, 18,
                   "mon_test_fire_vresist", check_eocs );
    shoot_monster( itype_shotgun_s, {}, itype_test_shot_00_fire_damage, 1, 0,
                   "mon_test_fire_immune", check_eocs );
}

// Targeting graph tests
static constexpr float fudge_factor = 0.025;

template<typename T, typename W>
std::map<T, float> hit_distribution( const targeting_graph<T, W> &graph,
                                     std::optional<float> guess = std::nullopt, int iters = 100000 )
{
    std::map<T, float> hits;
    for( int i = 0; i < iters; ++i ) {
        typename std::map<T, float>::iterator it;
        if( guess ) {
            it = hits.emplace( graph.select( 0.0, 1.0, *guess ), 0.f ).first;
        } else {
            it = hits.emplace( graph.select( 0.0, 1.0, rng_float( 0, 1 ) ), 0.f ).first;
        }
        ++it->second;
    }

    for( std::pair<const T, float> &hit : hits ) {
        hit.second /= iters;
    }

    return hits;
}

TEST_CASE( "targeting_graph_linear_distribution", "[targeting_graph][random]" )
{
    // Represents a typical circle target with  N+1 equally spaced rings
    // The center circle is twice as wide as any ring
    // All chances assume hit selection is totally random
    // As a line:
    // 00->1->2->3->..->N
    // For N = 1:
    // 66% chance to hit center (0), 33% to hit 1
    // For N = 2:
    // 50% to hit center (0), 25% to hit 1, 25% to hit 2
    struct circle_target {
        static int connection( const int &i ) {
            if( i == 0 ) {
                return 0;
            }
            return i - 1;
        }

        static double weight( const int &i ) {
            if( i == 0 ) {
                return 2;
            }
            return 1;
        }
    };

    std::vector<int> parts = { 0, 1, 2 };
    targeting_graph<int, circle_target> graph;
    graph.generate( 0, parts );

    std::map<int, float> hits;
    SECTION( "With random hits, hit probability is evenly split between rings (with center counting as double" ) {
        hits = hit_distribution( graph );

        // Check that we're within 2.5% of the expected distribution for simple targets
        CHECK( hits[0] == Approx( 0.5 ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 0.25 ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 0.25 ).margin( fudge_factor ) );
    }

    SECTION( "With a perfectly accurate hit, only the center is hit" ) {
        hits = hit_distribution( graph, std::optional<float>( 0.0f ) );
        REQUIRE( hits[0] == 1.0f );
        CHECK( hits[1] == 0.0f );
        CHECK( hits[2] == 0.0f );
    }

    SECTION( "With the least accurate hit, only the end is hit" ) {
        hits = hit_distribution( graph, std::optional<float>( 1.0f ) );
        REQUIRE( hits[2] == 1.0f );
        CHECK( hits[0] == 0.0f );
        CHECK( hits[1] == 0.0f );
    }

    SECTION( "With a middling hit, only the middle ring is hit" ) {
        hits = hit_distribution( graph, std::optional<float>( 0.6f ) );
        REQUIRE( hits[1] == 1.0f );
        CHECK( hits[0] == 0.0f );
        CHECK( hits[2] == 0.0f );
    }
}

TEST_CASE( "targeting_graph_simple_limb_distribution", "[targeting_graph][random]" )
{
    // A target with a center and N equally sized limbs connecting to it
    // All chances assume hit selection is totally random
    // Hit chance:
    // 0 -> 1 || 2 || .. || N
    // For N = 1:
    // 50% to hit center (0), 50% to hit 1
    // For N = 2:
    // 50% to hit center (0), 25% to hit 1, 25% to hit 2
    struct center_target {
        static int connection( const int & ) {
            return 0;
        }

        static double weight( const int & ) {
            return 1;
        }
    };

    SECTION( "With two options, and no hits hitting center, hits are equally split" ) {
        std::vector<int> parts = { 0, 1, 2 };
        targeting_graph<int, center_target> graph;
        graph.generate( 0, parts );

        std::map<int, float> hits = hit_distribution( graph, std::optional<float>( 0.9f ) );

        // Check that we're within 2.5% of the expected distribution for simple targets
        // Nothing should hit the center, the hit is too inaccurate for that
        REQUIRE( hits[0] == 0.0f );
        // And as these two are of equal weight, hits between them should be equally likely
        CHECK( hits[1] == Approx( 0.50 ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 0.50 ).margin( fudge_factor ) );
    }

    SECTION( "With entirely random hits, the center is hit 50% of the time, and the remainder of hits are split equally" ) {
        std::vector<int> parts = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        targeting_graph<int, center_target> graph;
        graph.generate( 0, parts );
        std::map<int, float> hits = hit_distribution( graph );

        CHECK( hits[0] == Approx( 0.50 ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[3] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[5] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[6] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[7] == Approx( 0.0625 ).margin( fudge_factor ) );
        CHECK( hits[8] == Approx( 0.0625 ).margin( fudge_factor ) );
    }
}

TEST_CASE( "targeting_graph_complex_linear_distribution", "[targeting_graph][random]" )
{
    // A target with a center and N limbs connecting to it
    // N cannot be more than 9
    // For 9 limbs, with random hits the percent chance of each limb being hit is the weight
    // if the center is not hit.
    struct linear_target {
        static int connection( const int &i ) {
            if( i == 0 ) {
                return 0;
            }
            return i - 1;
        }

        static double weight( const int &i ) {
            switch( i ) {
                // *INDENT-OFF*
                case 0: return 10;
                case 1: return 40;
                case 2: return 8;
                case 3: return 9;
                case 4: return 2;
                case 6: return 10;
                case 5: return 1;
                case 7: return 10;
                case 8: return 4;
                case 9: return 6;
                // *INDENT-ON*
            }
            return 0;
        }
    };

    SECTION( "With random hits on a small target, hits follow weights" ) {
        std::vector<int> parts = { 0, 1, 2 };
        targeting_graph<int, linear_target> graph;
        graph.generate( 0, parts );

        std::map<int, float> hits = hit_distribution( graph );

        // 10 / 58
        CHECK( hits[0] == Approx( 10.0f / 58.0f ).margin( fudge_factor ) );
        // 40 / 58
        CHECK( hits[1] == Approx( 40.0f / 58.0f ).margin( fudge_factor ) );
        // 8 / 58
        CHECK( hits[2] == Approx( 8.0f / 58.0f ).margin( fudge_factor ) );
    }

    SECTION( "With a complete target, hits are split by weight" ) {
        std::vector<int> parts = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        targeting_graph<int, linear_target> graph;
        graph.generate( 0, parts );
        std::map<int, float> hits = hit_distribution( graph );

        CHECK( hits[0] == Approx( 0.10 ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 0.40 ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 0.08 ).margin( fudge_factor ) );
        CHECK( hits[3] == Approx( 0.09 ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 0.02 ).margin( fudge_factor ) );
        CHECK( hits[5] == Approx( 0.01 ).margin( fudge_factor ) );
        CHECK( hits[6] == Approx( 0.10 ).margin( fudge_factor ) );
        CHECK( hits[7] == Approx( 0.10 ).margin( fudge_factor ) );
        CHECK( hits[8] == Approx( 0.04 ).margin( fudge_factor ) );
        CHECK( hits[9] == Approx( 0.06 ).margin( fudge_factor ) );
    }
}

TEST_CASE( "targeting_graph_complex_limb_distribution", "[targeting_graph][random]" )
{
    // A target with a center and N limbs connecting to it
    // N cannot be more than 9
    // For 9 limbs, with random hits the percent chance of each limb being hit is the weight
    // if the center is not hit.
    struct center_target {
        static int connection( const int & ) {
            return 0;
        }

        static double weight( const int &i ) {
            switch( i ) {
                // *INDENT-OFF*
                case 0: return 10;
                case 1: return 40;
                case 2: return 8;
                case 3: return 9;
                case 4: return 7;
                case 7: return 10;
                case 5: return 3;
                case 6: return 10;
                case 8: return 8;
                case 9: return 5;
                // *INDENT-ON*
            }
            return 0;
        }
    };

    SECTION( "With two options, and no hits hitting center, hits follow weights" ) {
        std::vector<int> parts = { 0, 1, 2 };
        targeting_graph<int, center_target> graph;
        graph.generate( 0, parts );

        std::map<int, float> hits = hit_distribution( graph, std::optional<float>( 0.9f ) );

        // Nothing should hit the center, the hit is too inaccurate for that
        REQUIRE( hits[0] == 0.0f );
        // Weight 40 / 48
        CHECK( hits[1] == Approx( 5.0f / 6.0f ).margin( fudge_factor ) );
        // Weight 8 / 48
        CHECK( hits[2] == Approx( 1.0f / 6.0f ).margin( fudge_factor ) );
    }

    SECTION( "With all limbs, and no hits to center, hits are split by weight among limbs" ) {
        std::vector<int> parts = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        targeting_graph<int, center_target> graph;
        graph.generate( 0, parts );
        std::map<int, float> hits = hit_distribution( graph, std::optional<float>( 1.0f ) );

        REQUIRE( hits[0] == 0.0f );
        CHECK( hits[1] == Approx( 0.40 ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 0.08 ).margin( fudge_factor ) );
        CHECK( hits[3] == Approx( 0.09 ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 0.07 ).margin( fudge_factor ) );
        CHECK( hits[5] == Approx( 0.03 ).margin( fudge_factor ) );
        CHECK( hits[6] == Approx( 0.10 ).margin( fudge_factor ) );
        CHECK( hits[7] == Approx( 0.10 ).margin( fudge_factor ) );
        CHECK( hits[8] == Approx( 0.08 ).margin( fudge_factor ) );
        CHECK( hits[9] == Approx( 0.05 ).margin( fudge_factor ) );
    }

    SECTION( "With random hits, hits are split by weight among limbs and weight down branches" ) {
        // Weights 10, 40, 7, 3
        std::vector<int> parts = { 0, 1, 4, 5 };
        targeting_graph<int, center_target> graph;
        graph.generate( 0, parts );
        std::map<int, float> hits = hit_distribution( graph );

        /* Hit distribution expectations:
         * 80% of the time, the path chosen includes 1 -> 40 / 50
         * 14% of the time, the path chosen includes 4 ->  7 / 50
         *  6% of the time, the path chosen includes 5 ->  3 / 50
         *
         * When the path includes 1:
         * 20% of the time, 0 will be hit -> 10 / 50
         * 80% of the time, 1 will be hit -> 40 / 50
         *
         * When the path includes 4:
         * 59% of the time, 0 will be hit -> 10 / 17
         * 41% of the time, 4 will be hit ->  7 / 17
         *
         * When the path includes 5:
         * 77% of the time, 0 will be hit -> 10 / 13
         * 23% of the time, 5 will be hit ->  3 / 13
         *
         * In total:
         * 1 will be hit 80% * 80% = 64% of the time
         * 4 will be hit 41% * 14% = 5.7% of the time
         * 5 will be hit 23% * 6%  = 1.4% of the time
         * 0 will be hit 100% - 64% - 5.7% - 1.4% = 28.9% of the time
         */
        CHECK( hits[0] == Approx( 0.289 ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 0.640 ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 0.057 ).margin( fudge_factor ) );
        CHECK( hits[5] == Approx( 0.014 ).margin( fudge_factor ) );
    }
}

TEST_CASE( "targeting_graph_complex_target", "[targeting_graph][random]" )
{

    struct node {
        int val;
        int weight;
        const node *connect = nullptr;

        node() = default;
        node( int a, int b ) : val( a ), weight( b ) {}
        bool operator==( const node &rhs ) const {
            return val == rhs.val;
        }
        bool operator<( const node &rhs ) const {
            return val < rhs.val;
        }
    };

    struct graph_wrapper {
        static node connection( const node &nd ) {
            return *nd.connect;
        }

        static double weight( const node &nd ) {
            return nd.weight;
        }
    };

    /* Test complex construction and hit distribution
     * Here's the complex graph being represented, all connections are one-way and down
     * if an element has no connections, it connects to itself.
     *
     * 1   2    3
     *  \ /    |
     *   4     |
     *    \   /
     *      5
     * 6 7  |
     *  \|  |
     *   8  |  9
     *    \ | /
     *      0
     *
     * 5 is the biggest node
     */
    std::map<int, int> connections = {
        {1, 4},
        {2, 4},
        {3, 5},
        {4, 5},
        {5, 0},
        {6, 8},
        {7, 8},
        {8, 0},
        {9, 0},
        {0, 0}
    };

    SECTION( "Equally weighted nodes" ) {
        std::vector<node> nodes = {
            {0, 1},
            {1, 1},
            {2, 1},
            {3, 1},
            {4, 1},
            {5, 1},
            {6, 1},
            {7, 1},
            {8, 1},
            {9, 1}
        };
        for( node &nd : nodes ) {
            nd.connect = &nodes[connections[nd.val]];
        }

        targeting_graph<node, graph_wrapper> graph;
        graph.generate( nodes[5], nodes );

        std::map<node, float> node_hits = hit_distribution( graph );
        std::map<int, float> hits;
        for( const std::pair<const node, float> &entry : node_hits ) {
            hits.emplace( entry.first.val, entry.second );
        }

        /* Hit chances:
         * 5 connects to 0, 4, 3
         * 0 connects to 8, 9
         * 8 connects to 6, 7
         * 4 connects to 1, 2
         *
         * Potential paths:
         * 5 0 8 6
         * 5 0 8 7
         * 5 0 9
         * 5 4 1
         * 5 4 2
         * 5 3
         *
         * 1/3 of hits go down the 0 path:
         * 0 8 6
         * 0 8 7
         * 0 9
         * of those, 1/2 go down the 8 path:
         * 8 6
         * 8 7
         * and 1/2 of those go down the 6, 1/2 down the 7
         *
         * of those, 1/2 go down the 9 path
         *
         * 1/3 of hits go down the 4 path:
         * 4 1
         * 4 2
         * of those, 1/2 go down the 1 path, 1/2 go down the 2 path
         *
         * 1/3 of hits go down the 3 path:
         * 3
         *
         * So, we have these paths, with these probabilities of being selected
         * (1/12) 5 0 8 6
         * (1/12) 5 0 8 7
         * (1/ 6) 5 0 9
         * (1/ 6) 5 4 1
         * (1/ 6) 5 4 2
         * (1/ 3) 5 3
         *
         * Each individual element in a path has a 1/(length) chance of being selected
         * So, at the edges:
         * 6 has a 1/4 * 1/12 = 1/48 chance of being selected
         * 7 has a 1/4 * 1/12 = 1/48 chance of being selected
         * 9 has a 1/3 * 1/ 6 = 1/18 chance of being selected
         * 1 has a 1/3 * 1/ 6 = 1/18 chance of being selected
         * 2 has a 1/3 * 1/ 6 = 1/18 chance of being selected
         * 3 has a 1/2 * 1/ 3 = 1/ 6 chance of being selected
         *
         * 8 has a 2 * 1/4 * 1/12 = 1/24 chance of being selected
         * 4 has a 2 * 1/3 * 1/ 6 = 1/ 9 chance of being selected
         *
         * 0 has a 2 * 1/12 * 1/4
         *       + 1 * 1/ 6 * 1/3
         *       = 1/24 + 1/18
         *       = 7/72 chance of being selected
         *
         * 5 has a 2 * 1/12 * 1/4
         *       + 3 * 1/ 6 * 1/3
         *       + 1 * 1/ 3 * 1/2
         *       = 1/24 + 1/6 + 1/6
         *       = 3/8 chance of being selected
         */
        CHECK( hits[6] == Approx( 1.0f / 48.0f ).margin( fudge_factor ) );
        CHECK( hits[7] == Approx( 1.0f / 48.0f ).margin( fudge_factor ) );
        CHECK( hits[9] == Approx( 1.0f / 18.0f ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 1.0f / 18.0f ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 1.0f / 18.0f ).margin( fudge_factor ) );
        CHECK( hits[3] == Approx( 1.0f /  6.0f ).margin( fudge_factor ) );

        CHECK( hits[8] == Approx( 1.0f / 24.0f ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 1.0f /  9.0f ).margin( fudge_factor ) );

        CHECK( hits[0] == Approx( 7.0f / 72.0f ).margin( fudge_factor ) );

        CHECK( hits[5] == Approx( 3.0f /  8.0f ).margin( fudge_factor ) );
    }

    SECTION( "Non-equally weighted nodes" ) {
        std::vector<node> nodes = {
            {0, 12},
            {1, 2},
            {2, 4},
            {3, 8},
            {4, 12},
            {5, 16},
            {6, 1},
            {7, 1},
            {8, 4},
            {9, 1}
        };
        for( node &nd : nodes ) {
            nd.connect = &nodes[connections[nd.val]];
        }

        targeting_graph<node, graph_wrapper> graph;
        graph.generate( nodes[5], nodes );

        std::map<node, float> node_hits = hit_distribution( graph );
        std::map<int, float> hits;
        for( const std::pair<const node, float> &entry : node_hits ) {
            hits.emplace( entry.first.val, entry.second );
        }

        /* Hit chances:
         * 5 connects to 0, 4, 3
         * 0 connects to 8, 9
         * 8 connects to 6, 7
         * 4 connects to 1, 2
         *
         * Potential paths:
         * 5 0 8 6
         * 5 0 8 7
         * 5 0 9
         * 5 4 1
         * 5 4 2
         * 5 3
         *
         * 12/32 = 3/8 of hits go down the 0 path:
         * 0 8 6
         * 0 8 7
         * 0 9
         * of those 4/5 go down the 8 path:
         * 8 6
         * 8 7
         * then 1/2 of that goes to 6, and the other 1/2 to 7
         * so 1/5 go to 9
         *
         * 12/32 = 3/8 of hits go down the 4 path:
         * 4 1
         * 4 2
         * then 2/6 = 1/3 of that go to 1
         * and  4/6 = 2/3 of that go to 2
         *
         * 8/32 = 1/4 of hits go down the 3 path:
         * 3
         *
         * So, we have these paths, with these probabilities of being selected
         * (3/20) 5 0 8 6 [16 + 12 + 4 + 1 = 33]
         * (3/20) 5 0 8 7 [16 + 12 + 4 + 1 = 33]
         * (3/40) 5 0 9   [16 + 12 + 1     = 29]
         * (1/ 8) 5 4 1   [16 + 12 + 2     = 30]
         * (1/ 4) 5 4 2   [16 + 12 + 4     = 32]
         * (1/ 4) 5 3     [16 +  8         = 24]
         *
         * Each individual element in a path has a weight/(sum of weights) chance of being selected
         * So, at the edges:
         * 6 has a 3/20 * 1/33 = 1/ 220 chance of being selected
         * 7 has a 3/20 * 1/33 = 1/ 220 chance of being selected
         * 9 has a 3/40 * 1/29 = 1/1160 chance of being selected
         * 1 has a 1/ 8 * 2/30 = 1/ 120 chance of being selected
         * 2 has a 1/ 4 * 4/32 = 1/  32 chance of being selected
         * 3 has a 1/ 4 * 8/24 = 1/  12 chance of being selected
         *
         * 8 has a 2 * 3/20 *  4/33 = 2 / 55 chance of being selected
         * 4 has a 1 * 1/ 8 * 12/30
         *       + 1 * 1/ 4 * 12/32 = 23/160 chance of being selected
         *
         * 0 has a 2 * 3/20 * 12/33
         *       + 1 * 3/40 * 12/29
         *       = 447 / 3190 chance of being selected
         *
         * 5 has a 2 * 3/20 * 16/33
         *       + 1 * 3/40 * 16/29
         *       + 1 * 1/ 8 * 16/30
         *       + 1 * 1/ 4 * 16/32
         *       + 1 * 1/ 4 * 16/24
         *       = 20869/38280 chance of being selected
         */
        CHECK( hits[6] == Approx( 1.0f /  220.0f ).margin( fudge_factor ) );
        CHECK( hits[7] == Approx( 1.0f /  220.0f ).margin( fudge_factor ) );
        CHECK( hits[9] == Approx( 1.0f / 1160.0f ).margin( fudge_factor ) );
        CHECK( hits[1] == Approx( 1.0f /  120.0f ).margin( fudge_factor ) );
        CHECK( hits[2] == Approx( 1.0f /   32.0f ).margin( fudge_factor ) );
        CHECK( hits[3] == Approx( 1.0f /   12.0f ).margin( fudge_factor ) );

        CHECK( hits[8] == Approx( 2.0f /  55.0f ).margin( fudge_factor ) );
        CHECK( hits[4] == Approx( 23.0f / 160.0f ).margin( fudge_factor ) );

        CHECK( hits[0] == Approx( 447.0f / 3190.0f ).margin( fudge_factor ) );

        CHECK( hits[5] == Approx( 20869.0f / 38280.0f ).margin( fudge_factor ) );
    }
}

// Mostly just a targeting graph test, but under a layer of indirection
TEST_CASE( "Default_anatomy_body_part_hit_chances", "[targeting_graph][anatomy][random]" )
{
    anatomy_id tested( "human_anatomy" );
    std::map<bodypart_id, float> hits;
    // This is the fewest number of iterations to get values that are relatively close to expected
    // But it does make this test a little slow (few seconds)
    const int total_hits = 1000000;
    for( int i = 0; i < total_hits; ++i ) {
        auto it = hits.emplace( tested->select_body_part_projectile_attack( 0, 1, rng_float( 0, 1 ) ),
                                0.f ).first;
        ++it->second;
    }

    for( std::pair<const bodypart_id, float> &hit : hits ) {
        hit.second /= total_hits;
    }

    /* The body is:
     *
     * h   H     f   F
     * |   |     |   |
     * a   A     l   L
     * \   \     /   /
     *  \   \   /   /
     *   \--- t ---/
     *        |
     *        d
     *       / \
     *      m   e
     *
     * Where
     * h,H = hand_l,r at HS  1.5
     * a,A = arm_l,r  at HS 13
     * f,F = foot_l,r at HS  2
     * l,L = leg_l,r  at HS 13
     * t   = torso    at HS 36
     * d   = head     at HS  4
     * m   = mouth    at HS  0.5
     * e   = eyes     at HS  0.5
     *
     * Number crunching:
     * (13/56) t a h [36 + 13 + 1.5 = 50.5]
     * (13/56) t A H [36 + 13 + 1.5 = 50.5]
     * (13/56) t l f [36 + 13 + 2   = 51  ]
     * (13/56) t L F [36 + 13 + 2   = 51  ]
     * ( 1/28) t d m [36 +  4 + 0.5 = 40.5]
     * ( 1/28) t d e [36 +  4 + 0.5 = 40.5]
     *
     * h,H = 13/56 *  1.5/50.5 =  39/5656
     * f,F = 13/56 *  2  /51   =  13/1428
     * a,A = 13/56 * 13  /50.5 = 169/2828
     * l,L = 13/56 * 13  /51   = 169/2856
     * m   =  1/28 *  0.5/40.5 =   1/2268
     * e   =  1/28 *  0.5/40.5 =   1/2268
     * d   =  1/14 *  4  /40.5 =   4/ 567
     * t   = 2 * 13/56 * 36/50.5
     *     + 2 * 13/56 * 36/51
     *     + 2 *  1/28 * 36/40.5
     *     =                     78121/108171
     *
     * For:
     * hands:  0.69%
     * feet :  0.91%
     * arms :  5.98%
     * legs :  5.92%
     * mouth:  0.04%
     * eyes :  0.04%
     * head :  0.71%
     * torso: 72.22%
     */

    CHECK( hits.at( body_part_eyes ) == Approx( 1.0f / 2268.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_head ) == Approx( 4.0f / 567.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_mouth ) == Approx( 1.0f / 2268.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_torso ) == Approx( 78121.0f / 108171.0f ).margin( fudge_factor ) );
    CHECK( hits.at( body_part_arm_l ) == Approx( 169.0f / 2828.0f ).margin( fudge_factor ) );
    CHECK( hits.at( body_part_arm_r ) == Approx( 169.0f / 2828.0f ).margin( fudge_factor ) );
    CHECK( hits.at( body_part_hand_l ) == Approx( 39.0f / 5656.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_hand_r ) == Approx( 39.0f / 5656.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_leg_l ) == Approx( 169.0f / 2856.0f ).margin( fudge_factor ) );
    CHECK( hits.at( body_part_leg_r ) == Approx( 169.0f / 2856.0f ).margin( fudge_factor ) );
    CHECK( hits.at( body_part_foot_l ) == Approx( 13.0f / 1428.0f ).margin( fudge_factor / 10 ) );
    CHECK( hits.at( body_part_foot_r ) == Approx( 13.0f / 1428.0f ).margin( fudge_factor / 10 ) );
}

