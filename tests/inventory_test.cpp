#include "cata_catch.h"
#include "inventory.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "type_id.h"

static const itype_id itype_rock( "rock" );
static const itype_id itype_stick( "stick" );
static const itype_id itype_string_36( "string_36" );

TEST_CASE( "inventory_form_from_map_bulk_batching", "[inventory][map]" )
{
    clear_map_without_vision();
    map &m = get_map();

    // Set up item positions
    tripoint_bub_ms p1( 0, 0, 0 );
    tripoint_bub_ms p2( 1, 0, 0 );
    tripoint_bub_ms p3( 2, 0, 0 );

    // Add items to the map
    item rock( itype_rock, calendar::turn );
    item stick( itype_stick, calendar::turn );
    item string( itype_string_36, calendar::turn );

    for( int i = 0; i < 5; ++i ) {
        m.add_item_or_charges( p1, rock );
        m.add_item_or_charges( p1, stick );
        m.add_item_or_charges( p1, stick ); // Extra stick
        m.add_item_or_charges( p2, stick );
        m.add_item_or_charges( p2, string );
        m.add_item_or_charges( p3, rock );
    }

    std::vector<tripoint_bub_ms> pts = { p1, p2, p3 };

    inventory inv;
    inv.form_from_map( m, pts, nullptr, false );

    // Check counts
    // Rocks: 5 at p1 + 5 at p3 = 10 rocks
    // Sticks: 10 at p1 + 5 at p2 = 15 sticks
    // Strings: 5 at p2 = 5 strings

    CHECK( inv.count_item( itype_rock ) == 10 );
    CHECK( inv.count_item( itype_stick ) == 15 );
    CHECK( inv.count_item( itype_string_36 ) == 5 );
}
