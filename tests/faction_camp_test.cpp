#include <map>
#include <memory>
#include <optional>
#include <string>

#include "basecamp.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "character.h"
#include "clzones.h"
#include "coordinates.h"
#include "faction.h"
#include "item.h"
#include "item_components.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "point.h"
#include "stomach.h"
#include "type_id.h"
#include "value_ptr.h"

static const itype_id itype_test_100_kcal( "test_100_kcal" );
static const itype_id itype_test_200_kcal( "test_200_kcal" );
static const itype_id itype_test_500_kcal( "test_500_kcal" );

static const vitamin_id vitamin_mutagen( "mutagen" );
static const vitamin_id vitamin_mutant_toxin( "mutant_toxin" );

static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

TEST_CASE( "camp_calorie_counting", "[camp]" )
{
    clear_avatar();
    clear_map();
    map &m = get_map();
    const tripoint_abs_ms zone_loc = m.get_abs( tripoint_bub_ms{ 5, 5, 0 } );
    REQUIRE( m.inbounds( zone_loc ) );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_FOOD, your_fac, {},
                       "food" );
    mapgen_place_zone( zone_loc, zone_loc, zone_type_CAMP_STORAGE, your_fac, {},
                       "storage" );
    faction *camp_faction = get_player_character().get_faction();
    const tripoint_abs_omt this_omt = project_to<coords::omt>( zone_loc );
    m.add_camp( this_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( this_omt.xy() );
    REQUIRE( !!bcp );
    basecamp *test_camp = *bcp;
    test_camp->set_owner( your_fac );
    WHEN( "a base item is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_100_kcal );
        REQUIRE( m.has_items( zone_local ) );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply().kcal() == 100 );
    }

    WHEN( "an item with inherited components is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        item test_200_kcal( itype_test_200_kcal );
        item_components made_of;
        made_of.add( test_100_kcal );
        made_of.add( test_100_kcal );
        // Setting the actual components. This will return 185 unless it's actually made up of two 100kcal components!
        test_200_kcal.components = made_of;
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_200_kcal );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply().kcal() == 200 );
    }

    WHEN( "an item with vitamins is added to larder" ) {
        camp_faction->empty_food_supply();
        item test_500_kcal( itype_test_500_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_500_kcal );
        test_camp->distribute_food();
        REQUIRE( camp_faction->food_supply().kcal() == 500 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
    }

    WHEN( "a larder with stored calories and vitamins has food withdrawn" ) {
        camp_faction->empty_food_supply();
        std::map<time_point, nutrients> added_food;
        added_food[calendar::turn_zero].calories = 100000;
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutant_toxin, 100 );
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutagen, 200 );
        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
        // Now withdraw 15% of the total calories, this should also draw out 15% of the stored vitamins.
        test_camp->camp_food_supply( -15 );
        CHECK( camp_faction->food_supply().kcal() == 85 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 85 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 170 );
    }

    WHEN( "a larder with perishable food passes the expiry date" ) {
        restore_on_out_of_scope restore_calendar_turn( calendar::turn );
        camp_faction->empty_food_supply();
        // non-perishable food
        std::map<time_point, nutrients> added_food;
        added_food[calendar::turn_zero].calories = 100000;
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutant_toxin, 100 );
        added_food[calendar::turn_zero].set_vitamin( vitamin_mutagen, 200 );

        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );

        // remove non-perishable from added
        added_food.erase( added_food.begin() );
        // perishable food
        added_food[calendar::turn + 7_days].calories = 150000;
        added_food[calendar::turn + 7_days].set_vitamin( vitamin_mutant_toxin, 200 );
        added_food[calendar::turn + 7_days].set_vitamin( vitamin_mutagen, 100 );

        camp_faction->add_to_food_supply( added_food );
        REQUIRE( camp_faction->food_supply().kcal() == 250 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 300 );
        REQUIRE( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 300 );

        // advance time
        calendar::turn += 14_days;

        CHECK( camp_faction->food_supply().kcal() == 100 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutant_toxin ) == 100 );
        CHECK( camp_faction->food_supply().get_vitamin( vitamin_mutagen ) == 200 );
    }

    WHEN( "an item that expires is added to larder" ) {
        restore_on_out_of_scope restore_calendar_turn( calendar::turn );
        camp_faction->empty_food_supply();
        item test_100_kcal( itype_test_100_kcal );
        tripoint_bub_ms zone_local = m.get_bub( zone_loc );
        m.i_clear( zone_local );
        m.add_item_or_charges( zone_local, test_100_kcal );
        test_camp->distribute_food();
        REQUIRE( camp_faction->food_supply().kcal() == 100 );
        REQUIRE( test_100_kcal.type->comestible != nullptr );
        REQUIRE( test_100_kcal.type->comestible->spoils == 360_days );

        calendar::turn += 365_days;

        CHECK( camp_faction->food_supply().kcal() == 0 );
    }
    overmap_buffer.clear_camps( this_omt.xy() );
}
// TODO: Tests for: Check calorie display at various activity levels, camp crafting works as expected (consumes inputs, returns outputs+byproducts, costs calories)
