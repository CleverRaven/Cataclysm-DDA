#include "catch/catch.hpp"

#include "bionics.h"
#include "item.h"
#include "itype.h"
#include "game.h"
#include "player.h"

void give_and_activate( player &p, std::string const &bioid ) {
    INFO( "bionic " + bioid + " is valid" );
    REQUIRE( is_valid_bionic( bioid ) );
    
    p.add_bionic( bioid );
    INFO( "dummy has gotten " + bioid + " bionic " );
    REQUIRE( p.has_bionic( bioid ) );

    // shortcut: "last added bionic"
    // @todo: rework to allow "linked" bionics
    const int bioindex = p.num_bionics() - 1;
    const bionic &bio = p.bionic_at_index( bioindex );
    REQUIRE( bio.id == bioid );

    // turn on if possible
    if( bionic_info( bio.id ).toggled && !bio.powered ) {
        p.activate_bionic( bioindex );
        INFO( "bionic " + bio.id + " with index " + std::to_string( bioindex ) + " is active " );
        REQUIRE( p.has_active_bionic( bioid ) );
    }

    return;
}

void clear_bionics( player &p ) {
    p.my_bionics.clear();
    p.power_level = 0;
    p.max_power_level = 0;

    return;
}

void test_not_consumable( player &p, item &it ) {
    it.ammo_unset();
    INFO( "consume " + it.tname() + " with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( !p.can_consume( it ) );

    it.ammo_set( default_ammo( it.ammo_type() ), -1 ); // -1 -> full
    INFO( "consume " + it.tname() + " with " + std::to_string( it.ammo_remaining() ) + " charges" );
    REQUIRE( !p.can_consume( it ) );

    return;
}

TEST_CASE( "bionics", "[bionics] [item]" ) {
    player &dummy = g->u;

    // one section failing shouldn't affect the rest
    clear_bionics( dummy );

    // Could be a SECTION, but prerequisite for many tests.
    INFO( "no power capacity at first" );
    CHECK( dummy.max_power_level == 0 );

    dummy.add_bionic( "bio_power_storage" );

    INFO( "adding Power Storage CBM only increases capacity" );
    CHECK( dummy.power_level == 0 );
    REQUIRE( dummy.max_power_level > 0 );

    SECTION( "bio_batteries" ) {
        give_and_activate( dummy, "bio_batteries" );

        // Special case: old-school stackable.
        INFO( "consume old-school batteries" );
        REQUIRE( dummy.can_consume( item( "battery", 0, 1 ) ) );

        std::list<item> items;
        items.emplace_back( item( "UPS_off", 0, 0 ) );
        items.emplace_back( item( "battery_car", 0, 0 ) );

        for( auto it: items ) {
            test_not_consumable( dummy, it );
        }
    }

    // TODO: bio_cable bio_furnace bio_reactor bio_advreactor
    // TODO: (pick from stuff with power_source)
}
