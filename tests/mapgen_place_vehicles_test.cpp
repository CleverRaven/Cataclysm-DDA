#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const nested_mapgen_id
nested_mapgen_test_nested_place_shopping_cart( "test_nested_place_shopping_cart" );
static const update_mapgen_id
update_mapgen_test_update_place_shopping_cart( "test_update_place_shopping_cart" );

namespace
{
void update_test( map &m, tripoint_abs_omt const &loc )
{
    manual_mapgen( loc, manual_update_mapgen, update_mapgen_test_update_place_shopping_cart );
    REQUIRE( m.veh_at( m.get_bub( project_to<coords::ms>( loc ) ) ) );
}

void nested_test( map &m, tripoint_abs_omt const &loc )
{
    manual_mapgen( loc, manual_nested_mapgen, nested_mapgen_test_nested_place_shopping_cart );
    REQUIRE( m.veh_at( m.get_bub( project_to<coords::ms>( loc ) ) ) );
}
} // namespace

TEST_CASE( "mapgen_place_vehicles" )
{
    map &here = get_map();
    clear_map();
    clear_avatar();
    REQUIRE( here.get_vehicles().empty() );
    tripoint_abs_omt const this_test_omt = project_to<coords::omt>( get_avatar().pos_abs() );
    SECTION( "update mapgen" ) {
        update_test( here, this_test_omt );
        update_test( here, this_test_omt + tripoint::east );
    }
    SECTION( "nested mapgen" ) {
        nested_test( here, this_test_omt );
        nested_test( here, this_test_omt + tripoint::east );
    }
}
