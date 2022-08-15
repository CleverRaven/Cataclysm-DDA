#include <iosfwd>
#include <vector>

#include "cata_catch.h"
#include "clzones.h"
#include "item.h"
#include "item_category.h"
#include "item_pocket.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const activity_id ACT_MOVE_LOOT( "ACT_MOVE_LOOT" );

static const faction_id faction_your_followers( "your_followers" );

static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_zone_unload_all( "zone_unload_all" );

static void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint pos )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_your_followers, false, true, pos, pos );
}

// Comestibles sorting is a bit awkward. Unlike other loot, they're almost
// always inside of a container, and their sort zone changes based on their
// shelf life and whether the container prevents rotting.
TEST_CASE( "zone sorting comestibles ", "[zones][items][food][activities]" )
{
    clear_map();
    zone_manager &zm = zone_manager::get_manager();

    const tripoint_abs_ms origin_pos;
    create_tile_zone( "Food", zone_type_LOOT_FOOD, tripoint_east );
    create_tile_zone( "Drink", zone_type_LOOT_DRINK, tripoint_west );

    SECTION( "without perishable zones" ) {
        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( "test_bitter_almond" );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( "test_wine" );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( "test_apple" );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( "test_milk" );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }

    SECTION( "with perishable zones" ) {
        create_tile_zone( "PFood", zone_type_LOOT_PFOOD, tripoint_north );
        create_tile_zone( "PDrink", zone_type_LOOT_PDRINK, tripoint_south );

        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( "test_bitter_almond" );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( "test_wine" );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( "test_apple" );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( "test_milk" );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_all_contained_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }
}

TEST_CASE( "zones_unload_basic", "[zones]" )
{
    clear_map();
    map &m = get_map();
    tripoint_abs_ms const zone_loc = get_player_character().get_location();
    tripoint_abs_ms const zone_unsorted_end = zone_loc + tripoint_north;
    tripoint_abs_ms const zone_unload_end = zone_loc + tripoint_north;

    item mossberg( "mossberg_590" );
    item shot_00( "shot_00" );
    item shoulder_strap( "shoulder_strap" );

    mossberg.force_insert_item( shoulder_strap, item_pocket::pocket_type::MOD );
    mossberg.force_insert_item( shot_00, item_pocket::pocket_type::MAGAZINE );

    mapgen_place_zone( zone_loc.raw(), zone_unsorted_end.raw(), zone_type_LOOT_UNSORTED, your_fac, {} );
    mapgen_place_unload_zone( zone_loc.raw(), zone_unload_end.raw(), zone_type_zone_unload_all,
                              your_fac, {}, true,
                              true, true );


    m.spawn_items( m.bub_from_abs( zone_loc ), { mossberg } );
    REQUIRE( !m.i_at( m.bub_from_abs( zone_loc ) ).empty() );
    REQUIRE( m.i_at( m.bub_from_abs( zone_loc ) ).size() == 1 );

    get_player_character().assign_activity( ACT_MOVE_LOOT );
    process_activity( get_player_character() );

    // after unloading more items are there
    CHECK( m.i_at( m.bub_from_abs( zone_loc ) ).size() == 3 );
}
