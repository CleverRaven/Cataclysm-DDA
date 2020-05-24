#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "creature.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "string_id.h"
#include "test_statistics.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

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
        for( const tripoint &monster_position : closest_tripoints_first( origin, range ) ) {
            if( rl_dist( monster_position, origin ) != range ) {
                continue;
            }
            num_subjects++;
            num_subjects_this_time++;
            spawn_test_monster( "mon_zombie", monster_position );
        }
        // Set off an explosion
        item grenade( explosive_id );
        grenade.charges = 0;
        grenade.type->invoke( g->u, grenade, origin );
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
    part_hp.reserve( veh->parts.size() );
    for( vehicle_part &part : veh->parts ) {
        part_hp.push_back( part.hp() );
    }
    return part_hp;
}

static void check_vehicle_damage( const std::string &explosive_id, const std::string &vehicle_id,
                                  const int range )
{
    // Clear map
    clear_map_and_put_player_underground();
    tripoint origin( 30, 30, 0 );

    vehicle *target_vehicle = g->m.add_vehicle( vproto_id( vehicle_id ), origin, 0, -1, 0 );
    std::vector<int> before_hp = get_part_hp( target_vehicle );

    while( g->m.veh_at( origin ) ) {
        origin.x++;
    }
    origin.x += range;

    // Set off an explosion
    item grenade( explosive_id );
    grenade.charges = 0;
    grenade.type->invoke( g->u, grenade, origin );

    std::vector<int> after_hp = get_part_hp( target_vehicle );

    // We don't expect any destroyed parts.
    REQUIRE( before_hp.size() == after_hp.size() );
    for( size_t i = 0; i < before_hp.size(); ++i ) {
        CAPTURE( i );
        INFO( target_vehicle->parts[ i ].name() );
        if( target_vehicle->parts[ i ].info().get_id() == vpart_battery_car ||
            target_vehicle->parts[ i ].info().get_id() == vpart_headlight ||
            target_vehicle->parts[ i ].info().get_id() == vpart_windshield ) {
            CHECK( before_hp[ i ] >= after_hp[ i ] );
        } else if( !( target_vehicle->parts[ i ].info().get_id() == vpart_vehicle_clock ) ) {
            CHECK( before_hp[ i ] == after_hp[ i ] );
        }
    }
}

TEST_CASE( "grenade_lethality", "[grenade],[explosion],[balance],[slow]" )
{
    check_lethality( "grenade_act", 5, 0.95, 0.06, outcome_type::Kill );
    check_lethality( "grenade_act", 15, 0.40, 0.06, outcome_type::Casualty );
}

TEST_CASE( "grenade_vs_vehicle", "[grenade],[explosion],[balance]" )
{
    check_vehicle_damage( "grenade_act", "car", 5 );
}
