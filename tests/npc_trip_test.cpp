#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "npc.h"
#include "npc_trip.h"
#include "npc_turn_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const faction_id faction_your_followers( "your_followers" );

static const field_type_str_id field_fd_fire( "fd_fire" );

static const ter_str_id ter_t_chainfence( "t_chainfence" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_ramp_down_low( "t_ramp_down_low" );
static const ter_str_id ter_t_ramp_up_high( "t_ramp_up_high" );
static const ter_str_id ter_t_stairs_down( "t_stairs_down" );
static const ter_str_id ter_t_stairs_up( "t_stairs_up" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

// follower with every need settled, so only following drives it
static npc &spawn_settled_follower( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.set_mission( NPC_MISSION_NULL );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    guy.set_hunger( 0 );
    guy.set_thirst( 0 );
    guy.set_sleepiness( 0 );
    guy.set_stored_kcal( guy.get_healthy_kcal() );
    guy.set_all_parts_temp_conv( BODYTEMP_NORM );
    guy.set_all_parts_temp_cur( BODYTEMP_NORM );
    return guy;
}

TEST_CASE( "npc_turn_helper_drives_a_follower_like_the_game", "[npc]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    avatar &u = get_avatar();
    u.setpos( here, tripoint_bub_ms( 65, 50, 0 ) );
    npc &guy = spawn_settled_follower( tripoint_bub_ms( 50, 50, 0 ) );
    REQUIRE( rl_dist( guy.pos_abs(), u.pos_abs() ) > guy.follow_distance() );

    const auto close_enough = [&u]( const npc & n ) {
        return rl_dist( n.pos_abs(), u.pos_abs() ) <= n.follow_distance();
    };
    run_npc_turns( guy, 30, close_enough );

    CHECK( close_enough( guy ) );
}

// non-ally NPC far from the player, so following and obligations stay out of it
static npc &spawn_loner( const tripoint_bub_ms &pos )
{
    map &here = get_map();
    get_avatar().setpos( here, tripoint_bub_ms( 20, 20, 0 ) );
    npc &guy = spawn_npc( pos.xy(), "test_talker" );
    clear_character( guy, true );
    guy.setpos( here, pos );
    REQUIRE_FALSE( guy.is_player_ally() );
    return guy;
}

TEST_CASE( "trip_to_disconnected_floor_is_not_walked", "[npc][movement]" )
{
    clear_map_without_vision( -2, 1 );
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    for( int x = 48; x <= 57; ++x ) {
        for( int y = 48; y <= 52; ++y ) {
            here.ter_set( tripoint_bub_ms( x, y, 1 ), ter_t_floor );
        }
    }
    const tripoint_bub_ms source( 51, 50, 1 );
    const npc_trip_spec spec = npc_trip_spec::within_range( source, 1, true );

    SECTION( "no way up: no walk, no step" ) {
        here.build_map_cache( 0 );
        CHECK_FALSE( guy.resolve_trip( spec ) );
        const tripoint_bub_ms before = guy.pos_bub();
        guy.set_moves( 100 );
        CHECK_FALSE( guy.move_to_and_verify( spec ) );
        CHECK( guy.pos_bub() == before );
    }
    SECTION( "stairs up: the walk climbs them" ) {
        const tripoint_bub_ms stairs( 55, 50, 0 );
        here.ter_set( stairs, ter_t_stairs_up );
        here.ter_set( stairs + tripoint::above, ter_t_stairs_down );
        here.build_map_cache( 0 );
        const std::optional<npc_trip> trip = guy.resolve_trip( spec );
        REQUIRE( trip );
        CHECK( trip->dest.z() == 1 );
        tripoint_bub_ms prev = guy.pos_bub();
        for( const tripoint_bub_ms &step : trip->path ) {
            CAPTURE( prev, step );
            if( step.z() != prev.z() ) {
                CHECK( prev.xy() == stairs.xy() );
            }
            prev = step;
        }
        // square_dist is 3D: the tile right below source counts as in range
        for( int turn = 0; turn < 20 && ( guy.posz() != 1 || square_dist( guy.pos_bub(), source ) > 1 );
             ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
        }
        CHECK( guy.posz() == 1 );
        CHECK( square_dist( guy.pos_bub(), source ) <= 1 );
    }
    SECTION( "stairs torn out mid-walk: it stays below" ) {
        const tripoint_bub_ms stairs( 55, 50, 0 );
        here.ter_set( stairs, ter_t_stairs_up );
        here.ter_set( stairs + tripoint::above, ter_t_stairs_down );
        here.build_map_cache( 0 );
        guy.set_moves( 100 );
        REQUIRE( guy.move_to_and_verify( spec ) );
        here.ter_set( stairs, ter_t_floor );
        here.build_map_cache( 0 );
        for( int turn = 0; turn < 20; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
            CAPTURE( turn );
            CHECK( guy.posz() == 0 );
        }
    }
}

// one step a turn; true once in range of spec.source
static bool walk_trip( npc &guy, const npc_trip_spec &spec, const int turns )
{
    for( int turn = 0; turn < turns; ++turn ) {
        if( guy.posz() == spec.source.z() && square_dist( guy.pos_bub(), spec.source ) <= spec.range ) {
            return true;
        }
        calendar::turn += 1_turns;
        guy.set_moves( 100 );
        guy.move_to_and_verify( spec );
    }
    return guy.posz() == spec.source.z() && square_dist( guy.pos_bub(), spec.source ) <= spec.range;
}

TEST_CASE( "self_care_walk_takes_any_level_change_it_routes", "[npc][movement]" )
{
    clear_map_without_vision( -2, 1 );
    clear_avatar();
    map &here = get_map();
    for( int x = 48; x <= 58; ++x ) {
        for( int y = 48; y <= 52; ++y ) {
            here.ter_set( tripoint_bub_ms( x, y, 1 ), ter_t_floor );
        }
    }
    const tripoint_bub_ms lower( 55, 50, 0 );

    SECTION( "stairs whose top is two tiles off" ) {
        const tripoint_bub_ms upper( 57, 50, 1 );
        here.ter_set( lower, ter_t_stairs_up );
        here.ter_set( upper, ter_t_stairs_down );
        here.build_map_cache( 0 );
        npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
        const npc_trip_spec spec = npc_trip_spec::within_range( tripoint_bub_ms( 51, 50, 1 ), 1, true );
        const std::optional<npc_trip> trip = guy.resolve_trip( spec );
        REQUIRE( trip );
        REQUIRE( std::find( trip->path.begin(), trip->path.end(), upper ) != trip->path.end() );
        CHECK( walk_trip( guy, spec, 30 ) );

        WHEN( "top is torn out mid-walk" ) {
            npc &late = spawn_loner( tripoint_bub_ms( 50, 51, 0 ) );
            late.set_moves( 100 );
            REQUIRE( late.move_to_and_verify( spec ) );
            here.ter_set( upper, ter_t_floor );
            here.build_map_cache( 0 );
            THEN( "it stays below" ) {
                CHECK_FALSE( walk_trip( late, spec, 20 ) );
                CHECK( late.posz() == 0 );
            }
        }
    }
    SECTION( "ramp up" ) {
        here.ter_set( lower, ter_t_ramp_up_high );
        here.ter_set( lower + tripoint::above, ter_t_ramp_down_low );
        here.build_map_cache( 0 );
        npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
        const npc_trip_spec spec = npc_trip_spec::within_range( tripoint_bub_ms( 51, 50, 1 ), 1, true );
        REQUIRE( guy.resolve_trip( spec ) );
        CHECK( walk_trip( guy, spec, 30 ) );

        WHEN( "ramp is torn out mid-walk" ) {
            npc &late = spawn_loner( tripoint_bub_ms( 50, 51, 0 ) );
            late.set_moves( 100 );
            REQUIRE( late.move_to_and_verify( spec ) );
            here.ter_set( lower, ter_t_floor );
            here.build_map_cache( 0 );
            THEN( "it stays below" ) {
                CHECK_FALSE( walk_trip( late, spec, 20 ) );
                CHECK( late.posz() == 0 );
            }
        }
    }
    SECTION( "ramp down" ) {
        here.ter_set( lower, ter_t_ramp_up_high );
        here.ter_set( lower + tripoint::above, ter_t_ramp_down_low );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 1 ) );
        REQUIRE( guy.posz() == 1 );
        const npc_trip_spec spec = npc_trip_spec::within_range( tripoint_bub_ms( 51, 50, 0 ), 1, true );
        REQUIRE( guy.resolve_trip( spec ) );
        CHECK( walk_trip( guy, spec, 30 ) );
    }
}

TEST_CASE( "adjacent_target_on_other_floor_is_not_in_place", "[npc][movement]" )
{
    clear_map_without_vision( -2, 1 );
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    const tripoint_bub_ms source( 50, 51, 1 );
    here.ter_set( source, ter_t_water_sh );
    here.build_map_cache( 0 );
    CHECK_FALSE( guy.resolve_trip( npc_trip_spec::within_range( source, 1, true ) ) );
}

TEST_CASE( "within_range_trip_ends_beside_unreachable_source", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    // nearest passable neighbor of the source is walled in; the far side is open
    const tripoint_bub_ms source( 55, 50, 0 );
    const tripoint_bub_ms pocket( 54, 50, 0 );
    here.ter_set( source, ter_t_wall_glass );
    for( const tripoint_bub_ms &p : here.points_in_radius( pocket, 1 ) ) {
        if( p != pocket ) {
            here.ter_set( p, ter_t_wall_glass );
        }
    }
    here.build_map_cache( 0 );
    REQUIRE( here.passable( pocket ) );
    REQUIRE( here.passable( tripoint_bub_ms( 56, 49, 0 ) ) );

    WHEN( "the walk may end on any tile next to the source" ) {
        const std::optional<npc_trip> trip =
            guy.resolve_trip( npc_trip_spec::within_range( source, 1, true ) );
        THEN( "it ends on the open side" ) {
            REQUIRE( trip );
            CHECK( trip->dest != pocket );
            CHECK( trip->dest.x() >= 56 );
            CHECK( square_dist( trip->dest, source ) == 1 );
        }
    }
    WHEN( "the walk must end on the nearest passable neighbor" ) {
        THEN( "there's no route to it" ) {
            CHECK_FALSE( guy.resolve_trip( npc_trip_spec::legacy_nearest_passable( source, true ) ) );
        }
    }
}

TEST_CASE( "stand_on_trip_to_impassable_spot_is_infeasible", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    const tripoint_bub_ms spot( 53, 50, 0 );
    here.build_map_cache( 0 );
    const std::optional<npc_trip> trip = guy.resolve_trip( npc_trip_spec::stand_on( spot, true ) );
    REQUIRE( trip );
    CHECK( trip->dest == spot );

    here.ter_set( spot, ter_t_wall_glass );
    here.build_map_cache( 0 );
    calendar::turn += 1_turns;
    CHECK_FALSE( guy.resolve_trip( npc_trip_spec::stand_on( spot, true ) ) );
}

TEST_CASE( "retained_water_trip_reuses_its_path", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    const tripoint_bub_ms water( 56, 50, 0 );
    here.ter_set( water, ter_t_water_sh );
    here.build_map_cache( 0 );
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );

    const int before = guy.trip_route_calls();
    guy.set_moves( 100 );
    REQUIRE( guy.move_to_and_verify( spec ) );
    const int after_first = guy.trip_route_calls();
    CHECK( after_first > before );
    REQUIRE_FALSE( guy.path.empty() );
    CHECK( square_dist( guy.path.back(), water ) <= 1 );

    SECTION( "same walk on later turns routes nothing" ) {
        for( int turn = 0; turn < 3; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            REQUIRE( guy.move_to_and_verify( spec ) );
        }
        CHECK( guy.trip_route_calls() == after_first );
    }
    SECTION( "someone further along the walk does not void it" ) {
        REQUIRE( guy.path.size() >= 3 );
        npc &other = spawn_npc( guy.path[2].xy(), "test_talker" );
        other.setpos( here, guy.path[2] );
        calendar::turn += 1_turns;
        guy.set_moves( 100 );
        REQUIRE( guy.move_to_and_verify( spec ) );
        CHECK( guy.trip_route_calls() == after_first );
    }
    SECTION( "fire under someone further along voids it" ) {
        REQUIRE( guy.path.size() >= 3 );
        const tripoint_bub_ms ahead = guy.path[2];
        npc &other = spawn_npc( ahead.xy(), "test_talker" );
        other.setpos( here, ahead );
        here.add_field( ahead, field_fd_fire, 1, 10_minutes );
        calendar::turn += 1_turns;
        guy.set_moves( 100 );
        guy.move_to_and_verify( spec );
        CHECK( guy.trip_route_calls() > after_first );
    }
    SECTION( "another source routes again" ) {
        const tripoint_bub_ms other( 50, 44, 0 );
        here.ter_set( other, ter_t_water_sh );
        here.build_map_cache( 0 );
        calendar::turn += 1_turns;
        guy.set_moves( 100 );
        REQUIRE( guy.move_to_and_verify( npc_trip_spec::within_range( other, 1, true ) ) );
        CHECK( guy.trip_route_calls() > after_first );
    }
}

TEST_CASE( "self_care_walk_respects_door_rules", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    guy.rules.set_flag( ally_rule::avoid_doors );
    // water in a closet whose only way in is door
    const tripoint_bub_ms water( 55, 50, 0 );
    const tripoint_bub_ms door( 53, 50, 0 );
    here.ter_set( water, ter_t_water_sh );
    for( int y = 48; y <= 52; ++y ) {
        here.ter_set( tripoint_bub_ms( 53, y, 0 ), ter_t_wall_glass );
        here.ter_set( tripoint_bub_ms( 57, y, 0 ), ter_t_wall_glass );
    }
    for( int x = 53; x <= 57; ++x ) {
        here.ter_set( tripoint_bub_ms( x, 48, 0 ), ter_t_wall_glass );
        here.ter_set( tripoint_bub_ms( x, 52, 0 ), ter_t_wall_glass );
    }
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );

    SECTION( "no new walk through a closed door" ) {
        here.ter_set( door, ter_t_door_c );
        here.build_map_cache( 0 );
        CHECK_FALSE( guy.resolve_trip( spec ) );
        for( int turn = 0; turn < 5; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
        }
        CHECK( here.ter( door ) == ter_t_door_c );
    }
    SECTION( "a door closed mid-walk stays shut" ) {
        here.ter_set( door, ter_t_door_o );
        here.build_map_cache( 0 );
        guy.set_moves( 100 );
        REQUIRE( guy.move_to_and_verify( spec ) );
        REQUIRE( std::find( guy.path.begin(), guy.path.end(), door ) != guy.path.end() );
        here.ter_set( door, ter_t_door_c );
        here.build_map_cache( 0 );
        for( int turn = 0; turn < 5; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
        }
        CHECK( here.ter( door ) == ter_t_door_c );
        CHECK( guy.pos_bub() != door );
    }
}

TEST_CASE( "self_care_walk_opens_doors_and_climbs", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    guy.rules.clear_flag( ally_rule::avoid_doors );
    // dexterity 20 slips on a climb 1/20
    guy.set_dex_base( 20 );
    // water in a closet whose only way in is way_in
    const tripoint_bub_ms water( 55, 50, 0 );
    const tripoint_bub_ms way_in( 53, 50, 0 );
    here.ter_set( water, ter_t_water_sh );
    for( int y = 48; y <= 52; ++y ) {
        here.ter_set( tripoint_bub_ms( 53, y, 0 ), ter_t_wall_glass );
        here.ter_set( tripoint_bub_ms( 57, y, 0 ), ter_t_wall_glass );
    }
    for( int x = 53; x <= 57; ++x ) {
        here.ter_set( tripoint_bub_ms( x, 48, 0 ), ter_t_wall_glass );
        here.ter_set( tripoint_bub_ms( x, 52, 0 ), ter_t_wall_glass );
    }
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );
    const auto walk_in = [&]() {
        for( int turn = 0; turn < 20 && square_dist( guy.pos_bub(), water ) > 1; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
        }
        return square_dist( guy.pos_bub(), water ) <= 1;
    };

    SECTION( "a closed door the NPC may open" ) {
        here.ter_set( way_in, ter_t_door_c );
        here.build_map_cache( 0 );
        REQUIRE( guy.resolve_trip( spec ) );
        CHECK( walk_in() );
        CHECK( here.ter( way_in ) != ter_t_door_c );
    }
    SECTION( "a fence the NPC may climb" ) {
        here.ter_set( way_in, ter_t_chainfence );
        here.build_map_cache( 0 );
        REQUIRE( guy.resolve_trip( spec ) );
        CHECK( walk_in() );
    }
}

TEST_CASE( "self_care_step_refuses_hazard_on_retained_path", "[npc][movement]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_loner( tripoint_bub_ms( 50, 50, 0 ) );
    const tripoint_bub_ms water( 56, 50, 0 );
    here.ter_set( water, ter_t_water_sh );
    here.build_map_cache( 0 );
    const npc_trip_spec spec = npc_trip_spec::within_range( water, 1, true );
    guy.set_moves( 100 );
    REQUIRE( guy.move_to_and_verify( spec ) );
    REQUIRE_FALSE( guy.path.empty() );
    const tripoint_bub_ms next = guy.path.front();
    here.add_field( next, field_fd_fire, 1, 10_minutes );
    REQUIRE( here.get_field( next, field_fd_fire ) != nullptr );

    WHEN( "there is a way around the fire" ) {
        for( int turn = 0; turn < 12 && square_dist( guy.pos_bub(), water ) > 1; ++turn ) {
            calendar::turn += 1_turns;
            guy.set_moves( 100 );
            guy.move_to_and_verify( spec );
            CAPTURE( turn, guy.pos_bub() );
            CHECK( guy.pos_bub() != next );
        }
        THEN( "it reaches the water" ) {
            CHECK( square_dist( guy.pos_bub(), water ) <= 1 );
        }
    }
    WHEN( "fire blocks the only way" ) {
        for( const tripoint_bub_ms &p : here.points_in_radius( guy.pos_bub(), 1 ) ) {
            if( p != guy.pos_bub() && p != next ) {
                here.ter_set( p, ter_t_wall_glass );
            }
        }
        here.build_map_cache( 0 );
        calendar::turn += 1_turns;
        const tripoint_bub_ms before = guy.pos_bub();
        guy.set_moves( 100 );
        THEN( "it doesn't step" ) {
            CHECK_FALSE( guy.move_to_and_verify( spec ) );
            CHECK( guy.pos_bub() == before );
            CHECK( guy.path.empty() );
        }
    }
}
