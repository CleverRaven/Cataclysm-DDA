#include <climits>
#include <iosfwd>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_pocket.h"
#include "npc.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static void clear_bionics( Character &you )
{
    you.my_bionics->clear();
    you.set_power_level( 0_kJ );
    you.set_max_power_level( 0_kJ );
}

static void test_consumable_charges( Character &you, std::string &itemname, bool should_work )
{
    item it = item( itemname, calendar::turn_zero, 0 );

    INFO( "\'" + it.tname() + "\' is count-by-charges" );
    CHECK( !it.count_by_charges() );

    it.charges = 0;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( you.can_fuel_bionic_with( it ) == false );

    it.charges = INT_MAX;
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.charges ) + " charges" );
    REQUIRE( you.can_fuel_bionic_with( it ) == should_work );
}

static void test_consumable_ammo( Character &you, std::string &itemname, bool when_empty,
                                  bool when_full )
{
    item it = item( itemname, calendar::turn_zero, 0 );

    it.ammo_unset();
    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( you.can_fuel_bionic_with( it ) == when_empty );

    if( !it.magazine_default().is_null() ) {
        item mag( it.magazine_default() );
        mag.ammo_set( mag.ammo_default() );
        it.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
    } else if( !it.ammo_default().is_null() ) {
        it.ammo_set( it.ammo_default() ); // fill
    }

    INFO( "consume \'" + it.tname() + "\' with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( you.can_fuel_bionic_with( it ) == when_full );
}

TEST_CASE( "bionics", "[bionics] [item]" )
{
    avatar &dummy = get_avatar();
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

    SECTION( "bio_fuel_cell_gasoline" ) {
        dummy.add_bionic( bionic_id( "bio_fuel_cell_gasoline" ) );

        item gasoline = item( "gasoline" );
        REQUIRE( gasoline.charges != 0 );
        CHECK( dummy.can_fuel_bionic_with( gasoline ) );


        item armor = item( "rm13_armor" );
        CHECK( !dummy.can_fuel_bionic_with( armor ) );
    }

    clear_bionics( dummy );

    SECTION( "bio_batteries" ) {
        dummy.add_bionic( bionic_id( "bio_batteries" ) );

        item battery = item( "light_battery_cell" );

        // Empty battery won't work
        battery.ammo_set( battery.ammo_default(), 0 );
        CHECK( !dummy.can_fuel_bionic_with( battery ) );

        // Full battery works
        battery.ammo_set( battery.ammo_default(), 50 );
        CHECK( dummy.can_fuel_bionic_with( battery ) );

        // Tool with battery won't work
        item light = item( "flashlight" );
        light.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        CHECK( !dummy.can_fuel_bionic_with( light ) );

    }

    clear_bionics( dummy );
    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}
