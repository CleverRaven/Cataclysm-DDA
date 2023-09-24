#include <cstddef>
#include <iosfwd>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "cata_catch.h"
#include "clzones.h"
#include "coordinates.h"
#include "faction.h"
#include "map.h"
#include "map_helpers.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player_helpers.h"

static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

TEST_CASE( "camp_calorie_counting", "[camp]" )
{
    clear_avatar();
    clear_map();
    map &m = get_map();
    const tripoint zone_loc = m.getabs( tripoint{ 5, 5, 0 } );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {},
                       "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {},
                       "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( m.getglobal( zone_loc ) );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    basecamp *test_camp = *bcp;
    WHEN( "a base item is added to larder" ) {
        camp_faction->food_supply = 0;
        item test_100_kcal( "test_100_kcal" );
        m.i_clear( zone_loc );
        m.add_item_or_charges( zone_loc, test_100_kcal );
        REQUIRE( m.has_items( zone_loc ) );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply == 100 );
    }

    WHEN( "an item with inherited components is added to larder" ) {
        camp_faction->food_supply = 0;
        item test_100_kcal( "test_100_kcal" );
        item test_200_kcal( "test_200_kcal" );
        item_components made_of;
        made_of.add( test_100_kcal );
        made_of.add( test_100_kcal );
        // Setting the actual components. This will return 185 unless it's actually made up of two 100kcal components!
        test_200_kcal.components = made_of;
        m.i_clear( zone_loc );
        m.add_item_or_charges( zone_loc, test_200_kcal );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply == 200 );
    }
}
// TODO: Tests for: Check calorie display at various activity levels, camp crafting works as expected (consumes inputs, returns outputs+byproducts, costs calories)
