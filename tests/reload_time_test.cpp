#include "cata_catch.h"

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "item.h"
#include "map.h"
#include "player_helpers.h"
#include "point.h"

#include <string>

static void check_reload_time( const std::string &weapon, const std::string &ammo,
                               const std::string &container, int expected_moves )
{
    const tripoint test_origin( 60, 60, 0 );
    avatar &shooter = get_avatar();
    shooter.setpos( test_origin );
    shooter.set_wielded_item( item( weapon, calendar::turn_zero, 0 ) );
    if( container.empty() ) {
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
    REQUIRE( act.load_RAS_weapon() );
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
            check_reload_time( "slingshot", "pebble", "backpack", 350 );
        }
        SECTION( "from an ammo pouch" ) {
            check_reload_time( "slingshot", "pebble", "ammo_pouch", 87 );
        }
        SECTION( "from a tool belt" ) {
            check_reload_time( "slingshot", "pebble", "tool_belt", 150 );
        }
        SECTION( "from a pocket" ) {
            check_reload_time( "slingshot", "pebble", "pants", 150 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( "slingshot", "pebble", "", 130 );
        }
    }
    SECTION( "reloading a staff sling" ) {
        SECTION( "from a backpack" ) {
            check_reload_time( "staff_sling", "rock", "backpack", 375 );
        }
        SECTION( "from a stone pouch" ) {
            check_reload_time( "staff_sling", "rock", "stone_pouch", 95 );
        }
        SECTION( "from a tool belt" ) {
            check_reload_time( "staff_sling", "rock", "tool_belt", 160 );
        }
        SECTION( "from a pocket" ) {
            check_reload_time( "staff_sling", "rock", "pants", 175 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( "staff_sling", "rock", "", 150 );
        }
    }
    SECTION( "reloading a bow" ) {
        SECTION( "from a duffel bag" ) {
            check_reload_time( "longbow", "arrow_wood", "long_duffelbag", 360 );
        }
        SECTION( "from a quiver" ) {
            check_reload_time( "longbow", "arrow_wood", "quiver", 80 );
        }
        SECTION( "from the ground" ) {
            check_reload_time( "longbow", "arrow_wood", "", 140 );
        }
    }
}
