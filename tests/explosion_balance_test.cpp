#include <cstddef>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "creature.h"
#include "explosion.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "point.h"
#include "test_statistics.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const vpart_id vpart_battery_car( "battery_car" );
static const vpart_id vpart_headlight( "headlight" );
static const vpart_id vpart_vehicle_clock( "vehicle_clock" );
static const vpart_id vpart_windshield( "windshield" );

enum class outcome_type {
    Kill, Casualty
};

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
    do {
        clear_creatures();
        // Spawn some monsters in a circle.
        tripoint origin( 30, 30, 0 );
        int num_subjects_this_time = 0;
        for( const tripoint &monster_position : closest_points_first( origin, range ) ) {
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
        grenade.charges = 0;
        grenade.type->invoke( get_avatar(), grenade, origin );
        explosion_handler::process_explosions();
        // see how many monsters survive
        std::vector<Creature *> survivors = g->get_creatures_if( []( const Creature & critter ) {
            return critter.is_monster();
        } );
        num_survivors += survivors.size();
        for( Creature *survivor : survivors ) {
            survivor_stats << survivor->pos() << " " << survivor->get_hp() << ", ";
            bool wounded = survivor->get_hp() < survivor->get_hp_max();
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
    grenade.charges = 0;
    grenade.type->invoke( get_avatar(), grenade, origin );
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

TEST_CASE( "grenade_lethality", "[grenade],[explosion],[balance],[slow]" )
{
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
        check_vehicle_damage( "grenade_act", "motorcycle", 0, 0.975, 0.9975 );
        check_vehicle_damage( "grenade_act", "humvee", 5, 1 );
    }
}
