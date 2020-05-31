#include <climits>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "pimpl.h"
#include "player.h"
#include "player_helpers.h"
#include "type_id.h"
#include "units.h"

static void clear_bionics( player &p )
{
    p.my_bionics->clear();
    p.set_power_level( 0_kJ );
    p.set_max_power_level( 0_kJ );
}

static void test_consumable_charges( player &p, std::string &itemname, bool when_none,
                                     bool when_max )
{
    item it = item( itemname, 0, 0 );

    INFO( "\'" + it.tname() + "\' is count-by-charges" );
    CHECK( it.count_by_charges() );

    it.charges = 0;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_none );

    it.charges = INT_MAX;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_max );
}

static void test_consumable_ammo( player &p, std::string &itemname, bool when_empty,
                                  bool when_full )
{
    item it = item( itemname, 0, 0 );

    it.ammo_unset();
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_empty );

    if( !it.magazine_default().is_null() ) {
        item mag( it.magazine_default() );
        mag.ammo_set( mag.ammo_default() );
        it.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
    } else if( !it.ammo_default().is_null() ) {
        it.ammo_set( it.ammo_default() ); // fill
    }

    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( p.can_consume( it ) == when_full );
}

TEST_CASE( "bionics", "[bionics] [item]" )
{
    avatar &dummy = g->u;
    clear_avatar();

    // one section failing shouldn't affect the rest
    clear_bionics( dummy );

    // Could be a SECTION, but prerequisite for many tests.
    INFO( "no power capacity at first" );
    CHECK( !dummy.has_max_power() );

    dummy.add_bionic( bionic_id( "bio_power_storage" ) );

    INFO( "adding Power Storage CBM only increases capacity" );
    CHECK( !dummy.has_power() );
    REQUIRE( dummy.has_max_power() );

    SECTION( "bio_advreactor" ) {
        give_and_activate_bionic( dummy, bionic_id( "bio_advreactor" ) );

        static const std::list<std::string> always = {
            "plut_cell",  // solid
            "plut_slurry" // uncontained liquid! not shown in game menu
        };
        for( auto it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> never = {
            "light_atomic_battery_cell", // TOOLMOD, no ammo actually
            "rm13_armor"      // TOOL_ARMOR
        };
        for( auto it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    clear_bionics( dummy );

    SECTION( "bio_batteries" ) {
        give_and_activate_bionic( dummy, bionic_id( "bio_batteries" ) );

        static const std::list<std::string> always = {
            "battery" // old-school
        };
        for( auto it : always ) {
            test_consumable_charges( dummy, it, true, true );
        }

        static const std::list<std::string> never = {
            "flashlight",  // !is_magazine()
            "laser_rifle", // NO_UNLOAD, uses ups_charges
            "UPS_off",     // NO_UNLOAD, !is_magazine()
            "battery_car"  // NO_UNLOAD, is_magazine()
        };
        for( auto it : never ) {
            test_consumable_ammo( dummy, it, false, false );
        }
    }

    clear_bionics( dummy );
    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}
