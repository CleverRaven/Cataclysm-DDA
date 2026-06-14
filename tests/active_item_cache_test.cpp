#include <set>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "point.h"
#include "type_id.h"

static const itype_id itype_firecracker_act( "firecracker_act" );

TEST_CASE( "place_active_item_at_various_coordinates", "[item]" )
{
    clear_map( -OVERMAP_DEPTH, OVERMAP_HEIGHT );
    map &here = get_map();
    REQUIRE( here.get_submaps_with_active_items().empty() );
    // An arbitrary active item.
    item active( itype_firecracker_act, calendar::turn_zero, item::default_charges_tag() );
    active.activate();

    // For each space in a wide area place the item and check if the cache has been updated.
    int z = 0;
    for( int x = 0; x < MAPSIZE_X; ++x ) {
        for( int y = 0; y < MAPSIZE_Y; ++y ) {
            REQUIRE( here.i_at( tripoint_bub_ms{ x, y, z } ).empty() );
            CAPTURE( x, y, z );
            tripoint_abs_sm abs_loc = here.get_abs_sub() + tripoint( x / SEEX, y / SEEY, z );
            CAPTURE( abs_loc );
            REQUIRE( here.get_submaps_with_active_items().empty() );
            REQUIRE( here.get_submaps_with_active_items().find( abs_loc ) ==
                     here.get_submaps_with_active_items().end() );
            item &item_ref = here.add_item( tripoint_bub_ms( x, y, z ), active );
            here.update_submaps_with_active_items();
            REQUIRE( item_ref.active );
            REQUIRE_FALSE( here.get_submaps_with_active_items().empty() );
            REQUIRE( here.get_submaps_with_active_items().find( abs_loc ) !=
                     here.get_submaps_with_active_items().end() );
            REQUIRE_FALSE( here.i_at( tripoint_bub_ms{ x, y, z } ).empty() );
            here.i_clear( tripoint_bub_ms{ x, y, z } );
            here.process_items();
        }
    }
}
