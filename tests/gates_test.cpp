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

TEST_CASE( "doors should be able to open and close", "[gates]" )
{
    map &here = get_map();
    clear_map();

    const tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is unlocked" ) {
        // create closed door on tile next to player
        REQUIRE( here.ter_set( pos, t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

        THEN( "the door should be able to open and close" ) {
            CHECK( here.open_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == t_door_o->id );

            CHECK( here.close_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == t_door_c->id );
        }
    }
    WHEN( "the door is locked" ) {
        // create locked door on tile next to player
        REQUIRE( here.ter_set( pos, t_door_locked ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_locked->id );

        THEN( "the door should not be able to open" ) {
            CHECK_FALSE( here.close_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == t_door_locked->id );
        }
    }
}

TEST_CASE( "windows should be able to open and close", "[gates]" )
{
    map &here = get_map();
    clear_map();

    const tripoint pos = get_avatar().pos() + point_east;

    // create closed window on tile next to player
    REQUIRE( here.ter_set( pos, t_window_no_curtains ) );
    REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains->id );

    WHEN( "the window is opened from the inside" ) {
        THEN( "the window should be able to open" ) {
            CHECK( here.open_door( pos, true, false ) );
            CHECK( here.ter( pos ).obj().id == t_window_no_curtains_open->id );
        }
    }
    WHEN( "the window is opened from the outside" ) {
        THEN( "the window should not be able to open" ) {
            CHECK_FALSE( here.close_door( pos, false, false ) );
            CHECK( here.ter( pos ).obj().id == t_window_no_curtains->id );
        }
    }
}

// Note that it is not intended for all doors and windows to make swish sound,
// some doors and windows might make other types of sounds in the future
// TODO: update test case when door and window sounds become defined in JSON.
TEST_CASE( "doors and windows should make whoosh sound", "[gates]" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();
    sounds::reset_sounds();

    const tripoint pos = get_avatar().pos() + point_east;

    WHEN( "the door is opened" ) {
        REQUIRE( here.ter_set( pos, t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.open_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_o->id );

        THEN( "the door should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the door is closed" ) {
        REQUIRE( here.ter_set( pos, t_door_o ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_o->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.close_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

        THEN( "the door should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is opened" ) {
        REQUIRE( here.ter_set( pos, t_window_no_curtains ) );
        REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.open_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains_open->id );

        THEN( "the window should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
    WHEN( "the window is closed" ) {
        REQUIRE( here.ter_set( pos, t_window_no_curtains_open ) );
        REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains_open->id );

        // make sure there is no sounds before action
        REQUIRE( sounds::get_monster_sounds().first.empty() );

        REQUIRE( here.close_door( pos, true, false ) );
        REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains->id );

        THEN( "the window should make a swish sound" ) {
            CHECK_FALSE( sounds::get_monster_sounds().first.empty() );
        }
    }
}

TEST_CASE( "character should lose moves when opening or closing doors or windows", "[gates]" )
{
    avatar &they = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map();

    const tripoint pos = get_avatar().pos() + point_east;

    // spend all moves so we know how many move points were spent
    they.moves = 0;

    WHEN( "avatar opens door" ) {
        REQUIRE( here.ter_set( pos, t_door_c ) );

        const ter_t door = here.ter( pos ).obj();
        REQUIRE( door.id == t_door_c->id );

        REQUIRE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should spend move points" ) {
            CHECK( they.moves == -door.open_cost );
        }
    }
    WHEN( "avatar fails to open locked door" ) {
        REQUIRE( here.ter_set( pos, t_door_locked ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_locked->id );

        REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

        THEN( "avatar should not spend move points" ) {
            CHECK( they.moves == 0 );
        }
    }
    GIVEN( "that avatar is outdoors" ) {
        REQUIRE( here.is_outside( pos ) );

        WHEN( "avatar fails to open window" ) {
            REQUIRE( here.ter_set( pos, t_window_no_curtains ) );
            REQUIRE( here.ter( pos ).obj().id == t_window_no_curtains->id );

            REQUIRE_FALSE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should not spend move points" ) {
                CHECK( they.moves == 0 );
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
            REQUIRE( here.ter_set( pos, t_window_no_curtains ) );

            const ter_t window = here.ter( pos ).obj();
            REQUIRE( window.id == t_window_no_curtains->id );

            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should spend move points" ) {
                CHECK( they.moves == -window.open_cost );
            }
        }
    }
}

TEST_CASE( "opening and closing doors should cost movement points", "[gates]" )
{
    avatar &they = get_avatar();
    map &here = get_map();

    clear_map();
    clear_avatar();

    const tripoint pos = they.pos() + tripoint_east;

    // spend all moves so we know how many move points were spent
    they.moves = 0;

    GIVEN( "that door is closed" ) {
        REQUIRE( here.ter_set( pos, t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

        WHEN( "avatar opens the door while walking" ) {
            REQUIRE( they.is_walking() );

            // movement cost of opening doors
            int open_cost = here.ter( pos ).obj().open_cost;

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -open_cost );
            }
        }
        WHEN( "avatar opens the door while running" ) {
            they.toggle_run_mode();
            REQUIRE( they.is_running() );

            // movement cost of opening doors
            int open_cost = here.ter( pos ).obj().open_cost;

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( they.run_cost( 100, false ) + open_cost / 2 ) );
            }
        }
        WHEN( "avatar opens the door while crouching" ) {
            they.toggle_crouch_mode();
            REQUIRE( they.is_crouching() );

            // movement cost of opening doors
            int open_cost = here.ter( pos ).obj().open_cost;

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( open_cost * 3 ) );
            }
        }
    }
    GIVEN( "that door is opened" ) {
        REQUIRE( here.ter_set( pos, t_door_o ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_o->id );

        WHEN( "avatar closes the door while walking" ) {
            REQUIRE( they.is_walking() );

            // movement cost of opening doors
            int close_cost = here.ter( pos ).obj().close_cost;
            doors::close_door( here, they, pos );
            REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -close_cost );
            }
        }
        WHEN( "avatar closes the door while running" ) {
            they.toggle_run_mode();
            REQUIRE( they.is_running() );

            // movement cost of opening doors
            int close_cost = here.ter( pos ).obj().close_cost;
            doors::close_door( here, they, pos );
            REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( close_cost / 2 ) );
            }
        }
        WHEN( "avatar closes the door while crouching" ) {
            they.toggle_crouch_mode();
            REQUIRE( they.is_crouching() );

            // movement cost of opening doors
            int close_cost = here.ter( pos ).obj().close_cost;
            doors::close_door( here, they, pos );
            REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( close_cost * 3 ) );
            }
        }
    }
    GIVEN( "that heavy door is closed" ) {
        REQUIRE( here.ter_set( pos, t_door_metal_c ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_metal_c->id );

        WHEN( "weak avatar opens the door" ) {
            REQUIRE( they.is_walking() );

            // make character physically weak
            they.mod_str_bonus( -4 );
            REQUIRE( they.get_str() == 4 );

            // movement cost of opening doors
            int open_cost = here.ter( pos ).obj().open_cost;
            REQUIRE( open_cost > 100 );

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( 100 + ( open_cost - 100 ) * 2.0 ) );
            }
        }
    }
    GIVEN( "that heavy door is open" ) {
        REQUIRE( here.ter_set( pos, t_door_metal_o ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_metal_o->id );

        WHEN( "weak avatar closes the door" ) {
            REQUIRE( they.is_walking() );

            // make character physically weak
            they.mod_str_bonus( -2 );
            REQUIRE( they.get_str() == 6 );

            // movement cost of opening doors
            int close_cost = here.ter( pos ).obj().close_cost;
            REQUIRE( close_cost > 100 );

            doors::close_door( here, they, pos );
            REQUIRE( here.ter( pos ).obj().id == t_door_metal_c->id );

            THEN( "avatar should lose movement points" ) {
                CHECK( they.moves == -( 100 + ( close_cost - 100 ) * 1.5 ) );
            }
        }
    }
}

TEST_CASE( "doors should be dashed through when running", "[gates]" )
{
    avatar &they = get_avatar();
    map &here = get_map();

    clear_map();
    clear_avatar();

    const tripoint pos = they.pos() + point_east;

    GIVEN( "that door is closed" ) {
        REQUIRE( here.ter_set( pos, t_door_c ) );
        REQUIRE( here.ter( pos ).obj().id == t_door_c->id );

        WHEN( "avatar runs through the door" ) {
            they.toggle_run_mode();
            REQUIRE( they.is_running() );

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should be dashing through" ) {
                // the player should move into the door frame
                CHECK( they.pos() == pos );
                // the player should still be running
                CHECK( they.is_running() );
            }
        }
        WHEN( "avatar opens the door while walking" ) {
            REQUIRE( they.is_walking() );

            // record player current position before moving
            const tripoint last_pos = they.pos();

            // move towards the door to open them
            REQUIRE( avatar_action::move( they, here, tripoint_east ) );

            THEN( "avatar should remain at the same position" ) {
                // the player should remain one tile away from the door
                CHECK( they.pos() == last_pos );
                // the player should still be walking
                CHECK( they.is_walking() );
            }
        }
    }
}
