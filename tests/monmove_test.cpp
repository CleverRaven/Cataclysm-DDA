#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "field_type.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "monster.h"
#include "mtype.h"
#include "player_helpers.h"
#include "point.h"
#include "scent_map.h"
#include "type_id.h"

static const ter_str_id ter_t_wall( "t_wall" );

static const efftype_id effect_immobilization( "immobilization" );
static const efftype_id effect_stunned( "stunned" );

static const mtype_id mon_mi_go( "mon_mi_go" );
static const mtype_id mon_turret( "mon_turret" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_dog( "mon_zombie_dog" );

static constexpr int MAX_MOVES_PER_TURN = 500;

static void boost_avatar_hp()
{
    avatar &u = get_avatar();
    for( const bodypart_id &bp : u.get_all_body_parts() ) {
        u.set_part_hp_cur( bp, 999 );
    }
}

static void run_monster_turns( monster &critter, int n )
{
    for( int i = 0; i < n && !critter.is_dead(); i++ ) {
        critter.set_moves( critter.get_speed() );
        critter.plan();
        int iters = 0;
        while( critter.get_moves() > 0 && !critter.is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
            critter.move();
        }
        calendar::turn += 1_turns;
    }
}

TEST_CASE( "monster_pursuit_on_open_map", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );
    boost_avatar_hp();

    const tripoint_bub_ms zombie_pos( 70, 60, 0 );
    monster &zed = spawn_test_monster( mon_zombie.str(), zombie_pos );
    zed.anger = 100;
    zed.set_dest( get_map().get_abs( player_pos ) );

    const int initial_dist = rl_dist( zed.pos_bub(), player_pos );

    run_monster_turns( zed, 15 );

    CHECK( rl_dist( zed.pos_bub(), player_pos ) < initial_dist );
}

TEST_CASE( "monster_constrained_state_prevents_movement", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms zombie_pos( 65, 60, 0 );
    monster &zed = spawn_test_monster( mon_zombie.str(), zombie_pos );
    zed.anger = 100;
    zed.set_dest( get_map().get_abs( player_pos ) );

    const tripoint_bub_ms start_pos = zed.pos_bub();

    SECTION( "stunned monster barely moves" ) {
        zed.add_effect( effect_stunned, 1_minutes );
        run_monster_turns( zed, 3 );
        // Stunned monsters stumble randomly; 3 turns can displace up to 3 tiles
        CHECK( rl_dist( zed.pos_bub(), start_pos ) <= 3 );
    }

    SECTION( "immobilized monster does not move" ) {
        zed.add_effect( effect_immobilization, 1_minutes );
        run_monster_turns( zed, 3 );
        CHECK( zed.pos_bub() == start_pos );
    }
}

TEST_CASE( "monster_avoids_dangerous_fields", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    map &here = get_map();

    const tripoint_bub_ms player_pos( 60, 70, 0 );
    g->place_player( player_pos );
    boost_avatar_hp();

    const tripoint_bub_ms migo_pos( 60, 50, 0 );
    monster &critter = spawn_test_monster( mon_mi_go.str(), migo_pos );
    critter.anger = 100;
    critter.set_dest( here.get_abs( player_pos ) );

    REQUIRE( !critter.is_immune_field( fd_fire ) );

    for( int x = 55; x <= 65; x++ ) {
        here.add_field( tripoint_bub_ms( x, 60, 0 ), fd_fire, 2, 1_hours );
    }

    const int initial_dist = rl_dist( critter.pos_bub(), player_pos );

    bool stepped_on_fire = false;
    for( int i = 0; i < 20 && !critter.is_dead(); i++ ) {
        critter.set_moves( critter.get_speed() );
        critter.plan();
        int iters = 0;
        while( critter.get_moves() > 0 && !critter.is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
            critter.move();
            if( here.get_field( critter.pos_bub(), fd_fire ) != nullptr ) {
                stepped_on_fire = true;
            }
        }
        calendar::turn += 1_turns;
    }

    CHECK_FALSE( stepped_on_fire );
    CHECK( rl_dist( critter.pos_bub(), player_pos ) < initial_dist );
}

TEST_CASE( "monster_attacks_when_adjacent", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );
    boost_avatar_hp();

    const tripoint_bub_ms zombie_pos( 61, 60, 0 );
    monster &zed = spawn_test_monster( mon_zombie.str(), zombie_pos );
    zed.anger = 100;
    zed.set_dest( get_map().get_abs( player_pos ) );

    zed.set_moves( zed.get_speed() );
    zed.plan();
    const int moves_before = zed.get_moves();
    int iters = 0;
    while( zed.get_moves() > 0 && !zed.is_dead() && iters++ < MAX_MOVES_PER_TURN ) {
        zed.move();
    }

    CHECK( zed.get_moves() < moves_before );
    CHECK( rl_dist( zed.pos_bub(), player_pos ) <= 1 );
}

TEST_CASE( "monster_scent_tracking", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    map &here = get_map();
    scent_map &scent = get_scent();

    const tripoint_bub_ms player_pos( 60, 65, 0 );
    g->place_player( player_pos );
    boost_avatar_hp();

    // Zombie dog has SMELLS flag and tracks sc_human scent.
    // No set_dest -- zombie must be wandering to trigger scent_move().
    const tripoint_bub_ms zombie_pos( 60, 50, 0 );
    monster &zed = spawn_test_monster( mon_zombie_dog.str(), zombie_pos );
    zed.anger = 100;

    // Wall at y=55 blocks LOS, gap at x=39 is the only path around
    for( int x = 40; x <= 80; x++ ) {
        here.ter_set( tripoint_bub_ms( x, 55, 0 ), ter_t_wall );
    }
    here.build_map_cache( 0 );

    REQUIRE( zed.type->has_flag( mon_flag_SMELLS ) );
    REQUIRE( !zed.sees( here, get_avatar() ) );

    const tripoint_bub_ms start_pos = zed.pos_bub();

    SECTION( "with scent trail, monster moves toward gap" ) {
        const scenttype_id player_scent = get_avatar().get_type_of_scent();

        // 2D scent field: westward gradient across a band y=48..52 so the
        // zombie can't escape the trail by drifting north/south. scent_move()
        // returns early when scent==0 on the current tile, so a 1-tile-wide
        // trail loses the zombie permanently on any random diagonal step.
        for( int y = 48; y <= 52; y++ ) {
            int sv = 50;
            for( int x = 70; x >= 39; x-- ) {
                scent.set( tripoint_bub_ms( x, y, 0 ), sv, player_scent );
                sv += 10;
            }
        }
        // South leg: gap at x=39, trail continues toward player
        int scent_val = 370;
        for( int y = 53; y <= 65; y++ ) {
            if( y != 55 ) {
                scent.set( tripoint_bub_ms( 39, y, 0 ), scent_val, player_scent );
            }
            scent_val += 10;
        }

        run_monster_turns( zed, 20 );

        // Trail leads west first; zombie should have followed it
        CHECK( zed.pos_bub().x() < start_pos.x() );
    }

    SECTION( "without scent, monster stays near start" ) {
        run_monster_turns( zed, 15 );

        // Random walk over 15 turns: expected displacement ~sqrt(15) but can
        // spike higher with unlucky seeds. Bound at 10 to avoid false failures.
        CHECK( rl_dist( zed.pos_bub(), start_pos ) <= 10 );
    }
}

TEST_CASE( "immobile_monster_stays_put", "[monmove]" )
{
    clear_map();
    clear_creatures();
    clear_avatar();
    set_time_to_day();

    const tripoint_bub_ms player_pos( 60, 60, 0 );
    g->place_player( player_pos );

    const tripoint_bub_ms turret_pos( 65, 60, 0 );
    monster &turret = spawn_test_monster( mon_turret.str(), turret_pos );

    run_monster_turns( turret, 5 );

    CHECK( turret.pos_bub() == turret_pos );
}
