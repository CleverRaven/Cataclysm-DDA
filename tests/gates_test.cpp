#include "avatar.h"
#include "avatar_action.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "gates.h"
#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "sounds.h"
#include "type_id.h"

static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_window_no_curtains( "t_window_no_curtains" );
static const ter_str_id ter_t_window_no_curtains_open( "t_window_no_curtains_open" );

TEST_CASE( "doors_should_be_able_to_open_and_close", "[gates]" )
{
    map &here = get_map();
    clear_map();

    tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is unlocked" ) {
        // create closed door on tile next to player
        REQUIRE( here.ter_set( pos, ter_t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_c->id );

        THEN( "the door should be able to open and close" ) {
            CHECK( here.open_door( get_avatar(), pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == ter_t_door_o->id );

            CHECK( here.close_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == ter_t_door_c->id );
        }
    }
    WHEN( "the door is locked" ) {
        // create locked door on tile next to player
        REQUIRE( here.ter_set( pos, ter_t_door_locked ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_locked->id );

        THEN( "the door should not be able to open" ) {
            CHECK_FALSE( here.close_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == ter_t_door_locked->id );
        }
    }
}

TEST_CASE( "windows_should_be_able_to_open_and_close", "[gates]" )
{
    map &here = get_map();
    clear_map();

    tripoint pos = get_avatar().pos() + point_east;

    // create closed window on tile next to player
    REQUIRE( here.ter_set( pos, ter_t_window_no_curtains ) );
    REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );

    WHEN( "the window is opened from the inside" ) {
        THEN( "the window should be able to open" ) {
            CHECK( here.open_door( get_avatar(), pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == ter_t_window_no_curtains_open->id );
        }
    }
    WHEN( "the window is opened from the outside" ) {
        THEN( "the window should not be able to open" ) {
            CHECK_FALSE( here.close_door( pos, false, false ) );
            CHECK( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );
        }
    }
}

// Note that it is not intended for all doors and windows to make swish sound,
// some doors and windows might make other types of sounds in the future
// TODO: update test case when door and window sounds become defined in JSON.
TEST_CASE( "doors_and_windows_should_make_whoosh_sound", "[gates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();
    sounds::reset_sounds();

    tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is opened" ) {
        REQUIRE( here.ter_set( pos, ter_t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_c->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.open_door( get_avatar(), pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_o->id );

        THEN( "the door should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the door is closed" ) {
        REQUIRE( here.ter_set( pos, ter_t_door_o ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_o->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.close_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_c->id );

        THEN( "the door should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is opened" ) {
        REQUIRE( here.ter_set( pos, ter_t_window_no_curtains ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.open_door( get_avatar(), pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains_open->id );

        THEN( "the window should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is closed" ) {
        REQUIRE( here.ter_set( pos, ter_t_window_no_curtains_open ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains_open->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.close_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );

        THEN( "the window should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
}

TEST_CASE( "character_should_lose_moves_when_opening_or_closing_doors_or_windows", "[gates]" )
{
    avatar &they = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map();

    tripoint pos = get_avatar().pos() + point_east;

    // the movement cost for opening and closing gates
    // remember to update this if changing value in code
    const int open_move_cost = 100;

    // set move value to 0 so we know how many
    // move points were spent opening and closing gates
    they.set_moves( 0 );

    WHEN( "avatar opens door" ) {
        REQUIRE( here.ter_set( pos, ter_t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_c->id );

        REQUIRE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should spend move points" ) {
            CHECK( they.get_moves() == -open_move_cost );
        }
    }
    WHEN( "avatar fails to open locked door" ) {
        REQUIRE( here.ter_set( pos, ter_t_door_locked ) );
        REQUIRE( here.ter( pos ).obj().id == ter_t_door_locked->id );

        REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should not spend move points" ) {
            CHECK( they.get_moves() == 0 );
        }
    }
    GIVEN( "that avatar is outdoors" ) {
        REQUIRE( here.is_outside( pos ) );

        WHEN( "avatar fails to open window" ) {
            REQUIRE( here.ter_set( pos, ter_t_window_no_curtains ) );
            REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );

            REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should spend move points" ) {
                CHECK( they.get_moves() == 0 );
            }
        }
    }
    GIVEN( "that avatar is indoors" ) {
        ter_id ter_flat_roof = ter_id( "t_flat_roof" );
        ter_id ter_concrete_floor = ter_id( "t_thconc_floor" );
        ter_id ter_concrete_wall = ter_id( "t_concrete_wall" );

        // enclose the player in single tile room surrounded with
        // concrete floor and roof to test opening windows from indoors
        const std::vector<tripoint> room_walls{
            pos + point_south_east,
            pos + point_south,
            pos + point_south_west,
            pos + point_west,
            pos + point_north_west,
            pos + point_north,
            pos + point_north_east
        };
        for( tripoint point : room_walls ) {
            REQUIRE( here.ter_set( point, ter_concrete_wall ) );
        }
        REQUIRE( here.ter_set( pos, ter_concrete_floor ) );
        REQUIRE( here.ter_set( pos + tripoint_above, ter_flat_roof ) );

        // mark map cache as dirty and rebuild it so that map starts
        // recognizing that tile player is standing on is indoors
        here.set_outside_cache_dirty( pos.z );
        here.build_outside_cache( pos.z );
        REQUIRE_FALSE( here.is_outside( pos ) );

        WHEN( "avatar opens window" ) {
            REQUIRE( here.ter_set( pos, ter_t_window_no_curtains ) );
            REQUIRE( here.ter( pos ).obj().id == ter_t_window_no_curtains->id );

            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should spend move points" ) {
                CHECK( they.get_moves() == -open_move_cost );
            }
        }
    }
}
