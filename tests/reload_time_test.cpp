#include <game.h>
#include <memory>
#include <string>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "coordinates.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const itype_id itype_ammo_pouch( "ammo_pouch" );
static const itype_id itype_arrow_wood( "arrow_wood" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_long_duffelbag( "long_duffelbag" );
static const itype_id itype_longbow( "longbow" );
static const itype_id itype_pants( "pants" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_quiver( "quiver" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_slingshot( "slingshot" );
static const itype_id itype_staff_sling( "staff_sling" );
static const itype_id itype_stone_pouch( "stone_pouch" );
static const itype_id itype_tool_belt( "tool_belt" );

static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static void check_reload_time( const itype_id &weapon, const itype_id &ammo,
                               const itype_id &container, int expected_moves )
{
    map &here = get_map();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    const tripoint_bub_ms spot( 61, 60, 0 );
    clear_map();
    avatar &shooter = get_avatar();
    g->place_critter_at( pseudo_debug_mon, spot );
    shooter.setpos( here, test_origin );
    shooter.set_wielded_item( item( weapon, calendar::turn_zero, 0 ) );
    if( container.is_null() ) {
        get_map().add_item( test_origin, item( ammo ) );
    } else {
        shooter.worn.wear_item( shooter, item( container ), false, false );
        shooter.i_add( item( ammo, calendar::turn_zero, 1 ) );
    }

    REQUIRE( !!shooter.used_weapon() );
    // Spooky action at a distance to tell load_RAS_weapon where to find the ammo.
    item::reload_option opt = shooter.select_ammo( shooter.used_weapon(), false );
    shooter.ammo_location = opt.ammo;

    CAPTURE( opt.moves() );
    CAPTURE( opt.ammo.obtain_cost( shooter, 1 ) );
    CAPTURE( shooter.item_reload_cost( *shooter.used_weapon(), *opt.ammo, 1 ) );
    CAPTURE( shooter.used_weapon()->get_reload_time() );
    CAPTURE( shooter.item_handling_cost( *shooter.used_weapon(), true, 0 ) );
    CAPTURE( shooter.used_weapon()->get_reload_time() );
    aim_activity_actor act = aim_activity_actor::use_wielded();
    int moves_before = shooter.get_moves();
    REQUIRE( shooter.fire_gun( here, spot, 1, *shooter.used_weapon(), shooter.ammo_location ) );
    int moves_after = shooter.get_moves();
    int spent_moves = moves_before - moves_after;
    int expected_upper = expected_moves * 1.05;
    int expected_lower = expected_moves * 0.95;
    CHECK( spent_moves > expected_lower );
    CHECK( spent_moves < expected_upper );
}

TEST_CASE( "reload_from_inventory_times", "[reload],[inventory],[balance]" )
{
    // Build a list of gear loadouts ranked by expected firing speed.
    clear_avatar();
    SECTION( "reloading a slingshot" ) {
        SECTION( "from a backpack" ) {
            check_reload_time( itype_slingshot, itype_pebble, itype_backpack, 400 );
        }
        SECTION( "from an ammo pouch" ) {
            check_reload_time( itype_slingshot, itype_pebble, itype_ammo_pouch, 137 );
        }
        SECTION( "from a tool belt" ) {
            check_reload_time( itype_slingshot, itype_pebble, itype_tool_belt, 200 );
        }
        SECTION( "from a pocket" ) {
            check_reload_time( itype_slingshot, itype_pebble, itype_pants, 200 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( itype_slingshot, itype_pebble, itype_id::NULL_ID(), 180 );
        }
    }
    SECTION( "reloading a staff sling" ) {
        SECTION( "from a backpack" ) {
            check_reload_time( itype_staff_sling, itype_rock, itype_backpack, 595 );
        }
        SECTION( "from a stone pouch" ) {
            check_reload_time( itype_staff_sling, itype_rock, itype_stone_pouch, 315 );
        }
        SECTION( "from a tool belt" ) {
            check_reload_time( itype_staff_sling, itype_rock, itype_tool_belt, 380 );
        }
        SECTION( "from a pocket" ) {
            check_reload_time( itype_staff_sling, itype_rock, itype_pants, 395 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( itype_staff_sling, itype_rock, itype_id::NULL_ID(), 370 );
        }
    }
    SECTION( "reloading a bow" ) {
        SECTION( "from a duffel bag" ) {
            check_reload_time( itype_longbow, itype_arrow_wood, itype_long_duffelbag, 410 );
        }
        SECTION( "from a quiver" ) {
            check_reload_time( itype_longbow, itype_arrow_wood, itype_quiver, 130 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( itype_longbow, itype_arrow_wood, itype_id::NULL_ID(), 190 );
        }
    }
}
