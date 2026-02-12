#include <optional>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "clzones.h"
#include "coordinates.h"
#include "item.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_556( "556" );
static const itype_id itype_ammolink223( "ammolink223" );
static const itype_id itype_belt223( "belt223" );
static const itype_id itype_test_apple( "test_apple" );
static const itype_id itype_test_bitter_almond( "test_bitter_almond" );
static const itype_id itype_test_milk( "test_milk" );
static const itype_id
itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );
static const itype_id itype_test_wine( "test_wine" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_wall( "t_wall" );

static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

static const zone_type_id zone_type_LOOT_DRINK( "LOOT_DRINK" );
static const zone_type_id zone_type_LOOT_FOOD( "LOOT_FOOD" );
static const zone_type_id zone_type_LOOT_PDRINK( "LOOT_PDRINK" );
static const zone_type_id zone_type_LOOT_PFOOD( "LOOT_PFOOD" );
static const zone_type_id zone_type_LOOT_UNSORTED( "LOOT_UNSORTED" );
static const zone_type_id zone_type_UNLOAD_ALL( "UNLOAD_ALL" );

namespace
{
template <class T>
int _count_items_or_charges( const T &items, const itype_id &id )
{
    int n = 0;
    for( const item &it : items ) {
        if( it.typeId() == id ) {
            n += it.count();
        }
    }
    return n;
}

int count_items_or_charges( const tripoint_bub_ms src, const itype_id &id,
                            const std::optional<vpart_reference> &vp )
{
    if( vp ) {
        return _count_items_or_charges( vp->vehicle().get_items( vp->part() ), id );
    }
    return _count_items_or_charges( get_map().i_at( src ), id );
}

void create_tile_zone( const std::string &name, const zone_type_id &zone_type, tripoint_abs_ms pos,
                       bool veh = false )
{
    zone_manager &zm = zone_manager::get_manager();
    zm.add( name, zone_type, faction_your_followers, false, true, pos, pos, nullptr, veh );
}

} // namespace

TEST_CASE( "zone_unloading_ammo_belts", "[zones][items][ammo_belt][activities][unload]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();
    std::optional<vpart_reference> vp;
    bool const in_vehicle = GENERATE( false, true );
    CAPTURE( in_vehicle );

    clear_avatar();
    clear_map_without_vision();

    tripoint_abs_ms const start = here.get_abs( tripoint_bub_ms::zero + tripoint::east );
    dummy.set_pos_abs_only( start );

    if( in_vehicle ) {
        REQUIRE( here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                   tripoint_bub_ms::zero + tripoint::east, 0_degrees, 0, 0 ) );
        vp = here.veh_at( start ).cargo();
        REQUIRE( vp );
        vp->vehicle().set_owner( dummy );
    }

    create_tile_zone( "Unload All", zone_type_UNLOAD_ALL, start, in_vehicle );

    item ammo_belt = item( itype_belt223, calendar::turn );
    ammo_belt.ammo_set( ammo_belt.ammo_default() );
    int belt_ammo_count_before_unload = ammo_belt.ammo_remaining( );

    REQUIRE( belt_ammo_count_before_unload > 0 );

    WHEN( "unloading ammo belts using UNLOAD_ALL " ) {
        if( in_vehicle ) {
            vp->vehicle().add_item( here, vp->part(), ammo_belt );
        } else {
            here.add_item_or_charges( tripoint_bub_ms( tripoint::east ), ammo_belt );
        }
        dummy.assign_activity( unload_loot_activity_actor() );
        CAPTURE( dummy.activity.id() );
        process_activity( dummy );

        THEN( "check that the ammo and linkages are both unloaded and the ammo belt is removed" ) {
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_belt223, vp ) == 0 );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east,
                                           itype_ammolink223, vp ) == belt_ammo_count_before_unload );
            CHECK( count_items_or_charges( tripoint_bub_ms::zero + tripoint::east, itype_556,
                                           vp ) == belt_ammo_count_before_unload );
        }
    }
}

// Comestibles sorting is a bit awkward. Unlike other loot, they're almost
// always inside of a container, and their sort zone changes based on their
// shelf life and whether the container prevents rotting.
TEST_CASE( "zone_sorting_comestibles_", "[zones][items][food][activities]" )
{
    clear_map_without_vision();
    zone_manager &zm = zone_manager::get_manager();

    const tripoint_abs_ms origin_pos;
    create_tile_zone( "Food", zone_type_LOOT_FOOD, tripoint_abs_ms::zero + tripoint::east );
    create_tile_zone( "Drink", zone_type_LOOT_DRINK, tripoint_abs_ms::zero + tripoint::west );

    SECTION( "without perishable zones" ) {
        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }

    SECTION( "with perishable zones" ) {
        create_tile_zone( "PFood", zone_type_LOOT_PFOOD, tripoint_abs_ms::zero + tripoint::north );
        create_tile_zone( "PDrink", zone_type_LOOT_PDRINK, tripoint_abs_ms::zero + tripoint::south );

        GIVEN( "a non-perishable food" ) {
            item nonperishable_food( itype_test_bitter_almond );
            REQUIRE_FALSE( nonperishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_food, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a non-perishable drink" ) {
            item nonperishable_drink( itype_test_wine );
            REQUIRE_FALSE( nonperishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( nonperishable_drink, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( nonperishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }

        GIVEN( "a perishable food" ) {
            item perishable_food( itype_test_apple );
            REQUIRE( perishable_food.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_food, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PFOOD );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_food, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the food zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_FOOD );
                }
            }
        }

        GIVEN( "a perishable drink" ) {
            item perishable_drink( itype_test_milk );
            REQUIRE( perishable_drink.goes_bad() );

            WHEN( "sorting without a container" ) {
                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( perishable_drink, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within an unsealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( !container.any_pockets_sealed() );

                THEN( "should put in the perishable drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_PDRINK );
                }
            }

            WHEN( "sorting within a sealed container" ) {
                item container( itype_test_watertight_open_sealed_container_250ml );
                REQUIRE( container.put_in( perishable_drink, pocket_type::CONTAINER ).success() );
                REQUIRE( container.seal() );
                REQUIRE( container.get_container_pockets().front()->spoil_multiplier() ==
                         0.0f );
                REQUIRE( container.all_pockets_sealed() );

                THEN( "should put in the drink zone" ) {
                    CHECK( zm.get_near_zone_type_for_item( container, origin_pos ) == zone_type_LOOT_DRINK );
                }
            }
        }
    }
}

// When all destination zone tiles are completely walled off (unreachable by A*),
// the sort activity should skip items rather than picking them up. Items must
// remain at the source tile.
TEST_CASE( "zone_sorting_skips_items_with_unreachable_destinations",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    // Player at east of origin (matching existing test pattern)
    const tripoint_bub_ms start_pos = tripoint_bub_ms::zero + tripoint::east;
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.set_pos_abs_only( start_abs );

    // Source: LOOT_UNSORTED zone at player position
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );

    // Place a food item at the source tile
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );

    // Destination: LOOT_FOOD zone at a tile 5 tiles east, with floor terrain
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    // Wall off all 8 neighbors of the destination tile
    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            here.ter_set( dest_pos + tripoint( dx, dy, 0 ), ter_t_wall );
        }
    }
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Verify the destination is recognized for sorting
    item test_food( itype_test_apple );
    zone_manager &zm = zone_manager::get_manager();
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );

    // Run the sort activity
    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item should remain at source tile (not picked up)
    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );
    // Player inventory should not contain the food item
    CHECK( dummy.charges_of( itype_test_apple ) == 0 );
    // Activity should have completed (no hang)
    CHECK( !dummy.activity );
}
