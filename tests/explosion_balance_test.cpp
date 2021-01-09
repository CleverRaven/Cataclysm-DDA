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
#include "iuse_actor.h"
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
    do {
        // Clear map
        clear_map_and_put_player_underground();
        // Spawn some monsters in a circle.
        tripoint origin( 30, 30, 0 );
        int num_subjects_this_time = 0;
        for( const tripoint &monster_position : closest_tripoints_first( origin, range ) ) {
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
        if( target_vehicle->parts[ i ].info().get_id() == vpart_id( "battery_car" ) ||
            target_vehicle->parts[ i ].info().get_id() == vpart_id( "headlight" ) ||
            target_vehicle->parts[ i ].info().get_id() == vpart_id( "windshield" ) ) {
            CHECK( before_hp[ i ] >= after_hp[ i ] );
        } else if( !( target_vehicle->parts[ i ].info().get_id() == vpart_id( "vehicle_clock" ) ) ) {
            CHECK( before_hp[ i ] == after_hp[ i ] );
        }
    }
}

TEST_CASE( "grenade_lethality", "[.],[grenade],[explosion],[balance],[slow]" )
{
    check_lethality( "grenade_act", 5, 0.95, 0.06, outcome_type::Kill );
    check_lethality( "grenade_act", 15, 0.40, 0.06, outcome_type::Casualty );
}

TEST_CASE( "grenade_vs_vehicle", "[grenade],[explosion],[balance]" )
{
    check_vehicle_damage( "grenade_act", "car", 5 );
}

TEST_CASE( "shrapnel behind wall", "[grenade],[explosion],[balance]" )
{
    clear_map_and_put_player_underground();
    tripoint origin( 30, 30, 0 );

    item grenade( "can_bomb_act" );
    grenade.charges = 0;
    REQUIRE( grenade.get_use( "explosion" ) != nullptr );
    const auto *actor = dynamic_cast<const explosion_iuse *>
                        ( grenade.get_use( "explosion" )->get_actor_ptr() );
    REQUIRE( actor != nullptr );
    REQUIRE( static_cast<bool>( actor->explosion.fragment ) );
    REQUIRE( actor->explosion.radius <= 0 );
    REQUIRE( actor->explosion.fragment->range > 2 );

    for( const tripoint &pt : closest_tripoints_first( origin, 2 ) ) {
        if( square_dist( origin, pt ) > 1 ) {
            g->m.ter_set( pt, t_wall_metal );
        }
    }

    // Not on the bomb because shrapnel always hits that square
    const monster &m_in_range = spawn_test_monster( "mon_zombie", origin + point( 1, 0 ) );
    const monster &m_behind_wall = spawn_test_monster( "mon_zombie", origin + point( 3, 0 ) );

    grenade.type->invoke( g->u, grenade, origin );

    CHECK( m_in_range.hp_percentage() < 100 );
    CHECK( m_behind_wall.hp_percentage() == 100 );
}

TEST_CASE( "shrapnel at huge range", "[grenade],[explosion]" )
{
    clear_map_and_put_player_underground();
    tripoint origin( 0, 0, 0 );

    item grenade( "debug_shrapnel_blast" );
    REQUIRE( grenade.get_use( "explosion" ) != nullptr );
    const auto *actor = dynamic_cast<const explosion_iuse *>
                        ( grenade.get_use( "explosion" )->get_actor_ptr() );
    REQUIRE( actor != nullptr );
    REQUIRE( static_cast<bool>( actor->explosion.fragment ) );
    REQUIRE( actor->explosion.radius <= 0 );
    REQUIRE( actor->explosion.fragment->range > MAPSIZE_X + MAPSIZE_Y );

    const monster &m = spawn_test_monster( "mon_zombie", tripoint( MAPSIZE_X - 1, MAPSIZE_Y - 1, 0 ) );

    grenade.type->invoke( g->u, grenade, origin );

    CHECK( m.is_dead_state() );
}

TEST_CASE( "shrapnel at max grenade range", "[grenade],[explosion]" )
{
    clear_map_and_put_player_underground();
    tripoint origin( 60, 60, 0 );

    item grenade( "can_bomb_act" );
    REQUIRE( grenade.get_use( "explosion" ) != nullptr );
    const auto *actor = dynamic_cast<const explosion_iuse *>
                        ( grenade.get_use( "explosion" )->get_actor_ptr() );
    REQUIRE( actor != nullptr );
    REQUIRE( static_cast<bool>( actor->explosion.fragment ) );
    REQUIRE( actor->explosion.radius <= 0 );
    REQUIRE( actor->explosion.fragment->range > 1 );

    const int range = actor->explosion.fragment->range;
    for( const tripoint &pt : closest_tripoints_first( origin, range + 1 ) ) {
        spawn_test_monster( "mon_zombie", pt );
    }

    grenade.charges = 0;
    grenade.type->invoke( g->u, grenade, origin );

    for( const tripoint &pt : closest_tripoints_first( origin, range + 1 ) ) {
        const monster *m = g->critter_at<monster>( pt );
        REQUIRE( m != nullptr );
        CAPTURE( m->pos() );
        CAPTURE( rl_dist( origin, m->pos() ) );
        if( rl_dist( origin, m->pos() ) <= range ) {
            CHECK( m->hp_percentage() < 100 );
        } else {
            CHECK( m->hp_percentage() == 100 );
        }
    }
}
