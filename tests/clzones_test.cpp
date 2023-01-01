#include <iosfwd>
#include <vector>

#include "activity_actor_definitions.h"
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

static const itype_id itype_556( "556" );
static const itype_id itype_ammolink223( "ammolink223" );
static const itype_id itype_belt223( "belt223" );

static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_zone_unload_all( "zone_unload_all" );

static int count_items_or_charges( const tripoint src, const itype_id &id )
{
    int n = 0;
    map_stack items = get_map().i_at( src );
    for( const item &it : items ) {
        if( it.typeId() == id ) {
            n += it.count();
        }
    }
    return n;
}

static void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint pos )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_your_followers, false, true, pos, pos );
}

TEST_CASE( "zone unloading ammo belts", "[zones][items][ammo_belt][activities][unload]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map();

    tripoint_abs_ms const start = here.getglobal( tripoint_east );
    bool const move_act = GENERATE( true, false );
    dummy.set_location( start );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start.raw() );
    create_tile_zone( "Unload All", zone_type_zone_unload_all, start.raw() );

    item ammo_belt = item( itype_belt223, calendar::turn );
    ammo_belt.ammo_set( ammo_belt.ammo_default() );
    int belt_ammo_count_before_unload = ammo_belt.ammo_remaining();

    REQUIRE( belt_ammo_count_before_unload > 0 );

    WHEN( "unloading ammo belts using zone_unload_all " ) {
        here.add_item_or_charges( tripoint_east, ammo_belt );
        if( move_act ) {
            dummy.assign_activity( player_activity( ACT_MOVE_LOOT ) );
        } else {
            dummy.assign_activity( player_activity( unload_loot_activity_actor() ) );
        }
        process_activity( dummy );

        THEN( "check that the ammo and linkages are both unloaded and the ammo belt is removed" ) {
            CHECK( count_items_or_charges( tripoint_east, itype_belt223 ) == 0 );
            CHECK( count_items_or_charges( tripoint_east,
                                           itype_ammolink223 ) == belt_ammo_count_before_unload );
            CHECK( count_items_or_charges( tripoint_east, itype_556 ) == belt_ammo_count_before_unload );
        }
    }
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
