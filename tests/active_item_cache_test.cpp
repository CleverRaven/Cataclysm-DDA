#include <set>

#include "calendar.h"
#include "cata_catch.h"
#include "game_constants.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"

TEST_CASE( "place_active_item_at_various_coordinates", "[item]" )
{
    clear_map();
    map &here = get_map();
    for( int z = -OVERMAP_DEPTH; z < OVERMAP_HEIGHT; ++z ) {
        for( int x = 0; x < MAPSIZE_X; ++x ) {
            for( int y = 0; y < MAPSIZE_Y; ++y ) {
                here.i_clear( { x, y, z } );
            }
        }
    }
    REQUIRE( here.get_submaps_with_active_items().empty() );
    // An arbitrary active item.
    item active( "firecracker_act", calendar::turn_zero, item::default_charges_tag() );
    active.activate();

    // For each space in a wide area place the item and check if the cache has been updated.
    int z = 0;
    for( int x = 0; x < MAPSIZE_X; ++x ) {
        for( int y = 0; y < MAPSIZE_Y; ++y ) {
            REQUIRE( here.i_at( { x, y, z } ).empty() );
            CAPTURE( x, y, z );
            tripoint abs_loc = here.get_abs_sub() + tripoint( x / SEEX, y / SEEY, z );
            CAPTURE( abs_loc.x, abs_loc.y, abs_loc.z );
            REQUIRE( here.get_submaps_with_active_items().empty() );
            REQUIRE( here.get_submaps_with_active_items().find( abs_loc ) ==
                     here.get_submaps_with_active_items().end() );
            item &item_ref = here.add_item( { x, y, z }, active );
            REQUIRE( item_ref.active );
            REQUIRE_FALSE( here.get_submaps_with_active_items().empty() );
            REQUIRE( here.get_submaps_with_active_items().find( abs_loc ) !=
                     here.get_submaps_with_active_items().end() );
            REQUIRE_FALSE( here.i_at( { x, y, z } ).empty() );
            here.i_clear( { x, y, z } );
        }
    }
}
