#include <iosfwd>
#include <vector>

#include "catch/catch.hpp"
#include "clzones.h"
#include "item.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "map_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );

static void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint pos )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_id( "your_followers" ), false, true, pos, pos );
}

// Comestibles sorting is a bit awkward. Unlike other loot, they're almost
// always inside of a container, and their sort zone changes based on their
// shelf life and whether the container prevents rotting.
TEST_CASE( "zone sorting comestibles ", "[zones][items][food][activities]" )
{
    clear_map();
    zone_manager &zm = zone_manager::get_manager();

    const tripoint &origin_pos = tripoint_zero;
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
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::unsealed );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.contents.get_all_contained_pockets().value().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::all_sealed );

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
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::unsealed );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( nonperishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.contents.get_all_contained_pockets().value().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::all_sealed );

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
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::unsealed );

                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_food, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.contents.get_all_contained_pockets().value().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::all_sealed );

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
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::unsealed );

                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( "test_watertight_open_sealed_container_250ml" );
                REQUIRE( container.put_in( perishable_drink, item_pocket::pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.contents.get_all_contained_pockets().value().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.contents.get_sealed_summary() == item_contents::sealed_summary::all_sealed );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }


        // MREs are under the food category but are not directly edible.
        GIVEN( "a non-comestible food" ) {
            item noncomestible_food( "mre_dessert" );
            REQUIRE( noncomestible_food.get_category_shallow().get_id() == item_category_id( "food" ) );
            REQUIRE_FALSE( noncomestible_food.is_comestible() );

            WHEN( "sorting" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( noncomestible_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }
    }
}
