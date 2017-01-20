#include "catch/catch.hpp"

#include "bionics.h"
#include "game.h"
#include "player.h"

// TODO: test: bio_battery bio_cable bio_furnace bio_reactor bio_advreactor ... anything with power_source

void give_and_activate( player &p, std::string const &bioid ) {
    p.add_bionic( bioid );
    INFO( "dummy has " + bioid + " bionic " );
    REQUIRE( p.has_bionic( bioid ) );

    // shortcut: "last added bionic"
    const int bioindex = p.num_bionics() - 1;
    const bionic &bio = p.my_bionics[bioindex];
    REQUIRE( bio.id == bioid );

    // activate if not on by default
    if( !( bionic_info( bioid ).activated ) ) {
        p.activate_bionic( bioindex );
        INFO( "bionic " + bioid + " with index " + std::to_string( bioindex ) + " is active " );
        // @todo: CHECK() when there's an interface
    }

    return;
}

TEST_CASE( "bionics", "[bionics] [item]" ) {
    player &dummy = g->u;
    //reset_bionics();
    // TODO: remove all bionics for multi-SECTION?

    // Could be a SECTION, but prerequisite for many tests.
    REQUIRE( dummy.max_power_level == 0 );
    dummy.add_bionic( "bio_power_storage" );
    REQUIRE( dummy.power_level == 0 );
    REQUIRE( dummy.max_power_level > 0 );

    SECTION( "bio_battery" ) {
        give_and_activate( dummy, "bio_battery" );
        // SECTION( "Can consume regular batteries" ) {
        //     INFO( "TODO" );
        // }
        // SECTION( "Can't consume UPS with charges" ) {
        //     INFO( "TODO" );
        // }
        // SECTION( "Can't consume car battery with charges" ) {
        //     INFO( "TODO" );
        // }
    }
}
