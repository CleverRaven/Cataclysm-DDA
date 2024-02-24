#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "creature.h"
#include "explosion.h"
#include "fragment_cloud.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "mtype.h"
#include "point.h"
#include "projectile.h"
#include "test_statistics.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const damage_type_id damage_bullet( "bullet" );

enum class outcome_type {
    Kill, Casualty
};

static float get_damage_vs_target( const std::string &target_id )
{
    projectile proj;
    proj.speed = 1000;
    // Arbitrary damage, we only care about scaling.
    proj.impact = damage_instance( damage_bullet, 10 );
    proj.proj_effects.insert( "NULL_SOURCE" );
    dealt_projectile_attack frag;
    frag.proj = proj;

    int damaging_hits = 0;
    int damage_taken = 0;
    tripoint monster_position( 30, 30, 0 );
    clear_map_and_put_player_underground();
    int hits = 10000;
    for( int i = 0; i < hits; ++i ) {
        clear_creatures();
        monster &target_monster = spawn_test_monster( target_id, monster_position );
        //REQUIRE( target_monster.type->armor_bullet == 0 );
        // This mirrors code in explosion::shrapnel() that scales hit rate with size and avoids crits.
        frag.missed_by = rng_float( 0.05, 1.0 / target_monster.ranged_target_size() );
        target_monster.deal_projectile_attack( nullptr, frag, false );
        if( frag.dealt_dam.total_damage() > 0 ) {
            damaging_hits++;
            damage_taken += frag.dealt_dam.total_damage();
        }
    }
    return static_cast<float>( damage_taken ) / static_cast<float>( damaging_hits );
}

static void check_lethality( const std::string &explosive_id, const int range, float lethality,
                             float margin, outcome_type expected_outcome )
{
    const epsilon_threshold target_lethality{ lethality, margin };
    int num_survivors = 0;
    int num_subjects = 0;
    int num_wounded = 0;
    statistics<bool> victims;
    std::stringstream survivor_stats;
    int total_hp = 0;
    clear_map_and_put_player_underground();
    tripoint origin( 30, 30, 0 );
    std::map<int, std::vector<tripoint>> circles;
    circles[0] = { origin };
    circles[5] = {
        { 25, 28, 0 }, { 25, 29, 0 }, { 25, 30, 0 }, { 25, 31, 0 }, { 25, 32, 0 },
        { 26, 26, 0 }, { 26, 34, 0 },
        { 27, 26, 0 }, { 27, 34, 0 },
        { 28, 25, 0 }, { 28, 35, 0 },
        { 29, 25, 0 }, { 29, 35, 0 },
        { 30, 25, 0 }, { 30, 35, 0 },
        { 31, 25, 0 }, { 31, 35, 0 },
        { 32, 25, 0 }, { 32, 35, 0 },
        { 33, 26, 0 }, { 33, 34, 0 },
        { 34, 26, 0 }, { 34, 34, 0 },
        { 35, 28, 0 }, { 35, 29, 0 }, { 35, 30, 0 }, { 35, 31, 0 }, { 35, 32, 0 }
    };
    circles[15] = closest_points_first( origin, range );
    do {
        clear_creatures();
        // Spawn some monsters in a circle.
        int num_subjects_this_time = 0;
        for( const tripoint &monster_position : circles[range] ) {
            if( rl_dist( monster_position, origin ) != range ) {
                continue;
            }
            num_subjects++;
            num_subjects_this_time++;
            monster &new_monster = spawn_test_monster( "mon_zombie", monster_position );
            new_monster.no_extra_death_drops = true;
        }
        // Set off an explosion
        item grenade( explosive_id );
        grenade.type->countdown_action.call( &get_avatar(), grenade, origin );
        explosion_handler::process_explosions();
        // see how many monsters survive
        std::vector<Creature *> survivors = g->get_creatures_if( []( const Creature & critter ) {
            return critter.is_monster();
        } );
        num_survivors += survivors.size();
        for( Creature *survivor : survivors ) {
            survivor_stats << survivor->pos() << " " << survivor->get_hp() << ", ";
            bool wounded = survivor->get_hp() < survivor->get_hp_max() * 0.75;
            num_wounded += wounded ? 1 : 0;
            total_hp += survivor->get_hp();
            if( expected_outcome == outcome_type::Casualty && wounded ) {
                victims.add( true );
            } else {
                victims.add( false );
            }
        }
        if( !survivors.empty() ) {
            survivor_stats << std::endl;
        }
        for( int i = survivors.size(); i < num_subjects_this_time; ++i ) {
            victims.add( true );
        }
    } while( victims.uncertain_about( target_lethality ) );
    CAPTURE( margin );
    INFO( explosive_id );
    item grenade( explosive_id );
    const explosion_data &ex = dynamic_cast<const explosion_iuse *>
                               ( grenade.type->countdown_action.get_actor_ptr() )->explosion;
    const shrapnel_data &shr = ex.shrapnel;
    const float fragment_velocity = explosion_handler::gurney_spherical( ex.power, shr.casing_mass );
    const float fragment_count = static_cast<float>( shr.casing_mass ) / shr.fragment_mass;
    const fragment_cloud cloud_at_target =
        shrapnel_calc( { fragment_velocity, fragment_count }, { 1.2, 1.0 }, std::max( 1, range ) );
    std::poisson_distribution<> d( cloud_at_target.density );
    int hits = d( rng_get_engine() );
    INFO( "Casing mass " << shr.casing_mass );
    INFO( "fragment mass " << shr.fragment_mass );
    INFO( "Total fragments " << fragment_count );
    INFO( "initial velocity " << fragment_velocity );
    INFO( "damage per fragment " << explosion_handler::ballistic_damage( cloud_at_target.velocity,
            shr.fragment_mass ) );
    INFO( "fragments expected " << cloud_at_target.density );
    INFO( "Sample fragment count " << hits );
    INFO( "range " << range );
    INFO( num_survivors << " survivors out of " << num_subjects << " targets." );
    INFO( survivor_stats.str() );
    INFO( "Wounded survivors: " << num_wounded );
    const int average_hp = num_survivors ? total_hp / num_survivors : 0;
    INFO( "average hp of survivors: " << average_hp );
    CHECK( victims.avg() == Approx( lethality ).margin( margin ) );
}

static std::vector<int> get_part_hp( vehicle *veh )
{
    std::vector<int> part_hp;
    part_hp.reserve( veh->part_count() );
    for( const vpart_reference &vpr : veh->get_all_parts() ) {
        part_hp.push_back( vpr.part().hp() );
    }
    return part_hp;
}

static void check_vehicle_damage( const std::string &explosive_id, const std::string &vehicle_id,
                                  const int range, const double damage_lower_bound, const double damage_upper_bound = 1.0 )
{
    // Clear map
    clear_map_and_put_player_underground();
    tripoint origin( 30, 30, 0 );

    vehicle *target_vehicle = get_map().add_vehicle( vproto_id( vehicle_id ), origin, 0_degrees,
                              -1, 0 );
    std::vector<int> before_hp = get_part_hp( target_vehicle );

    while( get_map().veh_at( origin ) ) {
        origin.x++;
    }
    origin.x += range;

    // Set off an explosion
    item grenade( explosive_id );
    grenade.type->countdown_action.call( &get_avatar(), grenade, origin );
    explosion_handler::process_explosions();

    std::vector<int> after_hp = get_part_hp( target_vehicle );

    // running sums of all parts, so we can do tests for hp range for the whole vehicle
    int before_hp_total = 0;
    int after_hp_total = 0;

    // We don't expect any destroyed parts.
    REQUIRE( before_hp.size() == after_hp.size() );
    for( size_t i = 0; i < before_hp.size(); ++i ) {
        before_hp_total += before_hp[ i ];
        after_hp_total += after_hp[ i ];
    }
    CAPTURE( before_hp_total );
    INFO( vehicle_id );
    CHECK( after_hp_total >= floor( before_hp_total * damage_lower_bound ) );
    CHECK( after_hp_total <= ceil( before_hp_total * damage_upper_bound ) );
}

TEST_CASE( "grenade_lethality_scaling_with_size", "[grenade],[explosion],[balance]" )
{
    // We want monsters of different sizes with the same armor to test that we aren't scaling damage with size.
    float tiny = get_damage_vs_target( "mon_spawn_raptor" );
    float small = get_damage_vs_target( "mon_dog_thing" );
    float medium = get_damage_vs_target( "mon_zombie" );
    float large = get_damage_vs_target( "mon_thing" );
    float huge = get_damage_vs_target( "mon_flying_polyp" );
    CHECK( tiny == Approx( small ).margin( 1.0 ) );
    CHECK( small == Approx( medium ).margin( 1.0 ) );
    CHECK( medium == Approx( large ).margin( 1.0 ) );
    CHECK( large == Approx( huge ).margin( 1.0 ) );

    float small_armored = get_damage_vs_target( "mon_skittering_plague" );
    float medium_armored = get_damage_vs_target( "mon_spider_trapdoor_giant" );
    float large_armored = get_damage_vs_target( "mon_mutant_evolved" );
    float huge_armored = get_damage_vs_target( "mon_jabberwock" );
    CHECK( small_armored == Approx( medium_armored ).margin( 1.0 ) );
    CHECK( medium_armored == Approx( large_armored ).margin( 1.0 ) );
    CHECK( large_armored == Approx( huge_armored ).margin( 1.0 ) );
}

TEST_CASE( "grenade_lethality", "[grenade],[explosion],[balance],[slow]" )
{
    check_lethality( "grenade_act", 0, 0.99, 0.06, outcome_type::Kill );
    check_lethality( "grenade_act", 5, 0.95, 0.06, outcome_type::Kill );
    check_lethality( "grenade_act", 15, 0.40, 0.06, outcome_type::Casualty );
}

TEST_CASE( "grenade_vs_vehicle", "[grenade],[explosion],[balance]" )
{
    /* as of test writing, car hp is 17653. 0.998 of that means the grenade
     * has to do more than 36 points of damage to 'fail', which isn't remotely
     * enough to disable a car even if it all happens to a single component
     *
     * also as of test writing:
     * motorcycle = 3173 hp, 0.997 is 3163 (so <= 10 points of damage)
     * at a range of 0, we expect motorcycles to take damage from a grenade.
     * should be between 3093 and 3166 (or 7 to 80 damage)
     *
     * humvee absolutely does not believe in a grenade damaging it.
     * this might change if we adjust the default armor loadout of a humvee,
     * but i would still expect less damage than with the car due to
     * heavy duty frames.
     */
    for( size_t i = 0; i <= 20 ; ++i ) {
        check_vehicle_damage( "grenade_act", "car", 5, 0.998 );
        check_vehicle_damage( "grenade_act", "motorcycle", 5, 0.997 );
        check_vehicle_damage( "grenade_act", "motorcycle", 0, 0.975, 0.9985 );
        check_vehicle_damage( "grenade_act", "humvee", 5, 1 );
    }
}
