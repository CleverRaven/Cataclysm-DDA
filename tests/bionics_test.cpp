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

        // Bottle with gasoline does not work
        item bottle = item( "bottle_plastic" );
        bottle.put_in( gasoline, item_pocket::pocket_type::CONTAINER );
        CHECK( !dummy.can_fuel_bionic_with( bottle ) );

        // Armor has no reason to work.
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
        item flashlight = item( "flashlight" );
        flashlight.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( flashlight.ammo_remaining() == 50 );
        CHECK( !dummy.can_fuel_bionic_with( flashlight ) );

    }

    clear_bionics( dummy );
    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}
