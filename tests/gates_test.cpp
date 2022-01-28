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

/**
 * Assert door or window open at given position.
 *
 * @param expect terrain id to expect at given location after opening door.
 * @param where location of the door to open.
 * @param indoors whether to open the door from indoors.
 */
static void assert_open_gate( ter_id &expect, tripoint &where, bool indoors )
{
    map &here = get_map();
    REQUIRE( here.open_door( where, indoors, false ) );
    REQUIRE( here.ter( where ).obj().id == expect->id );
}

/**
 * Assert door or window fail to open at given position.
 *
 * @param expect terrain id to expect at given location after opening door.
 * @param where location of the door to open.
 * @param indoors whether to open the door from indoors.
 */
static void assert_open_gate_fail( ter_id &expect, tripoint &where, bool indoors )
{
    map &here = get_map();
    REQUIRE_FALSE( here.open_door( where, indoors, false ) );
    REQUIRE( here.ter( where ).obj().id == expect->id );
}

/**
 * Assert door or window close at given position.
 *
 * @param expect terrain id to expect at given location after closing door.
 * @param where location of the door to close.
 * @param indoors whether to close the door from indoors.
 */
static void assert_close_gate( ter_id &expect, tripoint &where, bool indoors )
{
    map &here = get_map();
    REQUIRE( here.close_door( where, indoors, false ) );
    REQUIRE( here.ter( where ).obj().id == expect->id );
}

/**
 * Assert door or window fail to close at given position.
 *
 * @param expect terrain id to expect at given location after closing door.
 * @param where location of the door to close.
 * @param indoors whether to close the door from indoors.
 */
static void assert_close_gate_fail( ter_id &expect, tripoint &where, bool indoors )
{
    map &here = get_map();
    REQUIRE_FALSE( here.close_door( where, indoors, false ) );
    REQUIRE( here.ter( where ).obj().id == expect->id );
}

/**
 * Assert changing terrain at given location.
 *
 * @param what terrain id to set to.
 * @param where location of terrain to set.
 */
static void assert_create_terrain( ter_id &what, tripoint &where )
{
    map &here = get_map();
    REQUIRE( here.ter_set( where, what ) );
    REQUIRE( here.ter( where ).obj().id == what->id );
}

TEST_CASE( "doors should be able to open and close", "[gates]" )
{
    clear_map();
    tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is unlocked" ) {
        // create closed door on tile next to player
        assert_create_terrain( t_door_c, pos );

        THEN( "the door should be able to open and close" ) {
            assert_open_gate( t_door_o, pos, true );
            assert_close_gate( t_door_c, pos, true );
        }
    }
    WHEN( "the door is locked" ) {
        // create locked door on tile next to player
        assert_create_terrain( t_door_locked, pos );

        THEN( "the door should not be able to open" ) {
            assert_close_gate_fail( t_door_locked, pos, true );
        }
    }
}

TEST_CASE( "windows should be able to open and close", "[gates]" )
{
    clear_map();
    tripoint pos = get_avatar().pos() + point_east;

    // create closed window on tile next to player
    assert_create_terrain( t_window_no_curtains, pos );

    WHEN( "the window is opened from the inside" ) {
        THEN( "the window should be able to open" ) {
            assert_open_gate( t_window_no_curtains_open, pos, true );
        }
    }
    WHEN( "the window is opened from the outside" ) {
        THEN( "the window should not be able to open" ) {
            assert_open_gate_fail( t_window_no_curtains, pos, false );
        }
    }
}

TEST_CASE( "doors and windows should make whoosh sound", "[gates]" )
{
    clear_map();
    clear_avatar();
    sounds::reset_sounds();

    tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is opened" ) {
        assert_create_terrain( t_door_c, pos );
        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );
        assert_open_gate( t_door_o, pos, true );

        THEN( "the door should make a swish sound" ) {
            REQUIRE_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the door is closed" ) {
        assert_create_terrain( t_door_o, pos );
        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );
        assert_close_gate( t_door_c, pos, true );

        THEN( "the door should make a swish sound" ) {
            REQUIRE_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is opened" ) {
        assert_create_terrain( t_window_no_curtains, pos );
        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );
        assert_open_gate( t_window_no_curtains_open, pos, true );

        THEN( "the window should make a swish sound" ) {
            REQUIRE_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is closed" ) {
        assert_create_terrain( t_window_no_curtains_open, pos );
        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );
        assert_close_gate( t_window_no_curtains, pos, true );

        THEN( "the window should make a swish sound" ) {
            REQUIRE_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
}

TEST_CASE( "character should lose moves when opening or closing doors or windows", "[gates]" )
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
    they.moves = 0;

    WHEN( "avatar opens door" ) {
        assert_create_terrain( t_door_c, pos );
        REQUIRE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should spend move points" ) {
            REQUIRE( they.moves == -open_move_cost );
        }
    }
    WHEN( "avatar fails to open locked door" ) {
        assert_create_terrain( t_door_locked, pos );
        REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should not spend move points" ) {
            REQUIRE( they.moves == 0 );
        }
    }
    GIVEN( "that avatar is outdoors" ) {
        REQUIRE( here.is_outside( pos ) );

        WHEN( "avatar fails to open window" ) {
            assert_create_terrain( t_window_no_curtains, pos );
            REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should spend move points" ) {
                REQUIRE( they.moves == 0 );
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
            assert_create_terrain( t_window_no_curtains, pos );
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should spend move points" ) {
                REQUIRE( they.moves == -open_move_cost );
            }
        }
    }
}
