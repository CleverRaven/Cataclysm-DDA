#include "catch/catch.hpp"

#include "item.h"
#include "worldfactory.h"

#include <algorithm>

TEST_CASE("Boat mod is loaded correctly or not at all") {
    const auto &mods = world_generator->active_world->active_mod_order;
    if( std::find( mods.begin(), mods.end(), "boats" ) != mods.end() ) {
        REQUIRE( item::type_is_defined( "inflatable_boat" ) );
    } else {
        REQUIRE( !item::type_is_defined( "inflatable_boat" ) );
    }
}
