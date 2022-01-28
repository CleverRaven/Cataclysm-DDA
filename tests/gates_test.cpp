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

// Returns tile adjecent to avatar position along x axis.
static tripoint get_adjecent_tile()
{
    tripoint pos = get_avatar().pos();
    return tripoint( pos.x + 1, pos.y, pos.z );
}

TEST_CASE( "doors should be able to open and close", "[gates]" )
{
    clear_map();
    tripoint pos = get_adjecent_tile();

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
    tripoint pos = get_adjecent_tile();

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
