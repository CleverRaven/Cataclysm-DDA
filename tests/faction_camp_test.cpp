#include <cstddef>
#include <iosfwd>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "calendar.h"
#include "cata_catch.h"
#include "clzones.h"
#include "coordinates.h"
#include "faction.h"
#include "map.h"
#include "map_helpers.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "player_helpers.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const recipe_id recipe_test_200_kcal( "test_200_kcal" );

static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );

static const zone_type_id zone_type_CAMP_FOOD( "CAMP_FOOD" );
static const zone_type_id zone_type_CAMP_STORAGE( "CAMP_STORAGE" );

static time_point midday = calendar::turn_zero + 12_hours;

// copied almost wholesale from the crafting test. This could probably be simpler, but we need a comestible that's inherited calories.
static void setup_test_craft( const recipe_id &rid )
{
    clear_avatar();
    set_time( midday ); // Ensure light for crafting
    avatar &player_character = get_avatar();
    const recipe &rec = rid.obj();
    REQUIRE( player_character.morale_crafting_speed_multiplier( rec ) == 1.0 );
    REQUIRE( player_character.lighting_craft_speed_multiplier( rec ) == 1.0 );
    REQUIRE( !player_character.activity );

    // This really shouldn't be needed, but for some reason the tests fail for mingw builds without it
    player_character.learn_recipe( &rec );
    const inventory &inv = player_character.crafting_inventory();
    REQUIRE( player_character.has_recipe( &rec, inv, player_character.get_crafting_helpers() ) );
    player_character.remove_weapon();
    REQUIRE( !player_character.is_armed() );
    player_character.make_craft( rid, 1 );
    REQUIRE( player_character.activity );
    REQUIRE( player_character.activity.id() == ACT_CRAFT );
    // Extra safety
    item_location wielded = player_character.get_wielded_item();
    REQUIRE( wielded );
}


TEST_CASE( "camp_calorie_counting", "[camp]" )
{
    clear_map();
    map &m = get_map();
    tripoint const zone_loc = m.getabs( tripoint{ 5, 5, 0 } );
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
        m.add_item( zone_loc, test_100_kcal );
        REQUIRE( m.has_items( zone_loc ) );
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply == 100 );
    }

    WHEN( "an item with inherited components is added to larder" ) {
        camp_faction->food_supply = 0;
        Character &player_character = get_player_character();
        player_character.toggle_trait( trait_DEBUG_CNF );
        player_character.setpos( zone_loc );
        item test_100_kcal( "test_100_kcal" );
        // Make sure we add exactly 2 for the recipe, and no more
        m.add_item( zone_loc, test_100_kcal, 1 );
        // Do crafting and drop on zone_loc.
        setup_test_craft( recipe_test_200_kcal );
        calendar::turn += 5_seconds;
        for( item_location loc : player_character.top_items_loc() ) {
            player_character.drop( loc, zone_loc );
        }
        test_camp->distribute_food();
        CHECK( camp_faction->food_supply == 200 );
    }
}
// TODO: Tests for: Check calorie display at various activity levels, camp crafting works as expected (consumes inputs, returns outputs+byproducts)