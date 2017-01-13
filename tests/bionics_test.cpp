#include "catch/catch.hpp"

#include "bionics.h"
#include "game.h"
#include "player.h"

// TODO: use BDD-style instead? https://github.com/philsquared/Catch/blob/master/docs/tutorial.md#bdd-style
TEST_CASE( "bio_battery", "[bionic] [item]" ) {
    player &dummy = g->u;
    // TODO: give power storage bionic and tested bionic, turn it on - perhaps in a function
    INFO( "Dummy has bio_battery bionic: %s" + std::to_string( dummy.has_bionic( "bio_battery" ) ) );

        SECTION( "Can consume regular batteries" ) {
            INFO( "TODO" );
        }
        SECTION( "Can't consume UPS with charges" ) {
            INFO( "TODO" );
        }
        SECTION( "Can't consume car battery with charges" ) {
            INFO( "TODO" );
        }
}
