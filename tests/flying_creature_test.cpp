#include <memory>
#include <set>

#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const mtype_id mon_crow( "mon_crow" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_wall( "t_wall" );

// Floor at z=0, single open-air column straight up; walls everywhere else on
// the 3x3 z=1,2 footprint. Aloft fliers have no lateral candidates here.
static void build_narrow_shaft( map &here, const point_bub_ms &origin )
{
    for( int dx = -1; dx <= 1; ++dx ) {
        for( int dy = -1; dy <= 1; ++dy ) {
            const bool center = dx == 0 && dy == 0;
            const tripoint_bub_ms p0( origin.x() + dx, origin.y() + dy, 0 );
            const tripoint_bub_ms p1( origin.x() + dx, origin.y() + dy, 1 );
            const tripoint_bub_ms p2( origin.x() + dx, origin.y() + dy, 2 );
            here.ter_set( p0, ter_t_floor.id() );
            here.ter_set( p1, center ? ter_t_open_air.id() : ter_t_wall.id() );
            here.ter_set( p2, center ? ter_t_open_air.id() : ter_t_wall.id() );
        }
    }
    for( int z = 0; z <= 2; ++z ) {
        here.invalidate_map_cache( z );
        here.build_map_cache( z, true );
    }
}

// Force MATT_FLEE attitude toward the player.
static void force_flee_state( monster &mon )
{
    mon.aggro_character = true;
    mon.anger = 0;
    mon.morale = -100;
}

static std::set<int> run_and_collect_z( monster &mon, int turns )
{
    std::set<int> visited_z;
    visited_z.insert( mon.posz() );
    for( int turn = 0; turn < turns; ++turn ) {
        mon.mod_moves( mon.get_speed() );
        while( mon.get_moves() > 0 ) {
            const int prev = mon.posz();
            mon.move();
            visited_z.insert( mon.posz() );
            if( mon.posz() == prev && mon.get_moves() <= 0 ) {
                break;
            }
        }
    }
    return visited_z;
}

TEST_CASE( "aloft_idle_flier_settles_downward", "[monster][z-level][stumble]" )
{
    clear_avatar();
    clear_map( 0, 2 );
    map &here = get_map();
    Character &player = get_player_character();
    player.setpos( here, tripoint_bub_ms( 100, 100, 0 ) );

    const point_bub_ms origin( 30, 30 );
    build_narrow_shaft( here, origin );

    const tripoint_bub_ms spawn_pos( origin.x(), origin.y(), 2 );
    monster &mon = spawn_test_monster( mon_crow.str(), spawn_pos, false );
    mon.anger = 0;
    mon.morale = 0;
    REQUIRE( mon.flies() );
    REQUIRE( mon.is_wandering() );

    constexpr int budget_turns = 1500;
    const std::set<int> visited = run_and_collect_z( mon, budget_turns );

    g->remove_zombie( mon );

    CAPTURE( visited );
    CHECK( visited.count( 0 ) + visited.count( 1 ) >= 1 );
}

TEST_CASE( "aloft_flier_descends_when_fleeing", "[monster][z-level][flee]" )
{
    clear_avatar();
    clear_map( 0, 2 );
    map &here = get_map();
    Character &player = get_player_character();

    const point_bub_ms origin( 30, 30 );
    build_narrow_shaft( here, origin );

    player.setpos( here, tripoint_bub_ms( origin.x(), origin.y(), 0 ) );
    const tripoint_bub_ms spawn_pos( origin.x(), origin.y(), 2 );
    monster &mon = spawn_test_monster( mon_crow.str(), spawn_pos, false );
    REQUIRE( mon.flies() );
    force_flee_state( mon );

    constexpr int budget_turns = 600;
    const std::set<int> visited = run_and_collect_z( mon, budget_turns );
    const int final_z = mon.posz();

    g->remove_zombie( mon );

    CAPTURE( visited );
    CHECK( ( visited.count( 0 ) == 1 || visited.count( 1 ) == 1 ) );
    CHECK( final_z < 2 );
}

TEST_CASE( "grounded_flier_takes_off_when_fleeing", "[monster][z-level][flee]" )
{
    clear_avatar();
    clear_map( 0, 2 );
    map &here = get_map();
    Character &player = get_player_character();

    const point_bub_ms origin( 30, 30 );
    build_narrow_shaft( here, origin );

    const tripoint_bub_ms spawn_pos( origin.x(), origin.y(), 0 );
    player.setpos( here, tripoint_bub_ms( origin.x() + 1, origin.y(), 0 ) );
    monster &mon = spawn_test_monster( mon_crow.str(), spawn_pos, false );
    REQUIRE( mon.flies() );
    force_flee_state( mon );

    constexpr int budget_turns = 600;
    const std::set<int> visited = run_and_collect_z( mon, budget_turns );

    g->remove_zombie( mon );

    CAPTURE( visited );
    CHECK( visited.count( 1 ) == 1 );
}

TEST_CASE( "skittish_flier_ignores_distant_z_threat", "[monster][z-level][skittish]" )
{
    clear_avatar();
    clear_map( 0, 2 );
    map &here = get_map();
    Character &player = get_player_character();

    const point_bub_ms origin( 30, 30 );
    build_narrow_shaft( here, origin );

    // xy dist 0, z dist 2: unscaled would trip skittish flee at <=4;
    // scaled effective is 8.
    player.setpos( here, tripoint_bub_ms( origin.x(), origin.y(), 0 ) );
    const tripoint_bub_ms spawn_pos( origin.x(), origin.y(), 2 );
    monster &mon = spawn_test_monster( mon_crow.str(), spawn_pos, false );
    REQUIRE( mon.flies() );
    // attitude() returns MATT_FOLLOW when morale < 0 and morale + anger > 0.
    mon.aggro_character = true;
    mon.anger = 5;
    mon.morale = -3;

    CHECK_FALSE( mon.is_fleeing( player ) );

    g->remove_zombie( mon );
    monster &mon_close = spawn_test_monster( mon_crow.str(),
                         tripoint_bub_ms( origin.x() + 1, origin.y(), 0 ), false );
    mon_close.aggro_character = true;
    mon_close.anger = 5;
    mon_close.morale = -3;
    CHECK( mon_close.is_fleeing( player ) );
    g->remove_zombie( mon_close );
}
