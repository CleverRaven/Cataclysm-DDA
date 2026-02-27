#include <functional>
#include <initializer_list>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "clzones.h"
#include "coordinates.h"
#include "enums.h"
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
#include "visitable.h"
#include "vpart_position.h"

static const faction_id faction_your_followers( "your_followers" );

static const itype_id itype_556( "556" );
static const itype_id itype_ammolink223( "ammolink223" );
static const itype_id itype_backpack( "backpack" );
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
static const vproto_id vehicle_prototype_test_turret_rig( "test_turret_rig" );

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

// Grab-aware A* destination probing should find paths for single-tile vehicles.
// When player is at the source, items are picked up because route_length confirms
// the destination is reachable through grab-aware pathfinding.
// (process_activity can't simulate auto-move, so we verify pickup, not delivery.)
TEST_CASE( "zone_sorting_with_grabbed_single_tile_vehicle",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    // Player at source tile - no routing needed to reach source
    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Spawn shopping cart adjacent to player (east) and grab it
    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );

    // Source: LOOT_UNSORTED at player position
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );

    // Place a food item at the source tile
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );

    // Destination: LOOT_FOOD 5 tiles south (far enough that route_length exercises A*)
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 5, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );

    // Run the sort activity
    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item should have been picked up from source. Grab-aware A* found a valid
    // path to the destination, so route_length returned a real distance and
    // dropoff_coords was populated, allowing pickup to proceed.
    // (Item is in inventory/cart, not delivered yet - process_activity can't auto-move.)
    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 0 );
}

// Multi-tile vehicles should fall back to normal pathfinding instead of
// hanging or failing silently when grab-aware A* doesn't support them.
TEST_CASE( "zone_sorting_with_grabbed_multi_tile_vehicle",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    // Player at source tile
    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_abs_ms start_abs = here.get_abs( start_pos );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Spawn multi-tile vehicle (test_turret_rig) adjacent to player and grab it
    const tripoint_bub_ms veh_pos = start_pos + tripoint::east;
    vehicle *veh = here.add_vehicle( vehicle_prototype_test_turret_rig,
                                     veh_pos, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->get_points().size() > 1 );
    veh->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );

    // Source: LOOT_UNSORTED at player position
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, start_abs );
    here.add_item_or_charges( start_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 1 );

    // Destination: LOOT_FOOD 5 tiles south
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 0, 5, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food, start_abs ) == zone_type_LOOT_FOOD );

    // Run the sort activity - should use normal pathfinding fallback for
    // multi-tile vehicle (route_with_grab only supports single-tile).
    // Without Fix 1, route_length would return INT_MAX, dropoff_coords would
    // be empty, and the item would stay at source.
    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item picked up (normal pathfinding found destination reachable)
    CHECK( count_items_or_charges( start_pos, itype_test_apple, std::nullopt ) == 0 );
}

// When the grabbed cart sits on the source tile and inventory is full,
// the sort activity should terminate cleanly without hanging.
TEST_CASE( "zone_sorting_cart_on_source_full_inventory",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile is one tile south, where we'll also put the cart
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    // Spawn shopping cart at the source tile and grab it
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );

    // Create zones
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place food item at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    // Fill player inventory so items can't be picked up.
    // Limit iterations to avoid slow test if character has large capacity.
    for( int i = 0; i < 500 && dummy.can_add( item( itype_test_apple ) ); i++ ) {
        dummy.i_add( item( itype_test_apple ) );
    }
    REQUIRE( !dummy.can_add( item( itype_test_apple ) ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    // Run the sort activity
    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // With terrain UNSORTED zone, the ground item is sortable. Cart at source
    // provides storage even when inventory is full: ground item is picked up
    // into cart cargo for delivery. Activity terminates cleanly.
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}

// Virtual pickup: when grabbed cart sits on the source tile, items stay in
// cart cargo and are delivered to the adjacent destination without physically
// moving them to inventory first.
TEST_CASE( "zone_sorting_virtual_pickup_from_cart",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart at source tile (south of player)
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Zones: vehicle UNSORTED at source (items in cart cargo need vehicle zone),
    // LOOT_FOOD adjacent (east of player)
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place food in cart cargo
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Food delivered to destination, cart empty
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// Virtual pickup works even when player inventory is completely full.
TEST_CASE( "zone_sorting_virtual_pickup_full_inventory",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart at source (south)
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Vehicle UNSORTED zone (items in cart cargo need vehicle zone)
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place food in cart cargo
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    // Fill player inventory
    for( int i = 0; i < 500 && dummy.can_add( item( itype_test_apple ) ); i++ ) {
        dummy.i_add( item( itype_test_apple ) );
    }
    REQUIRE( !dummy.can_add( item( itype_test_apple ) ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Virtual pickup bypasses inventory capacity
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// Direct delivery: when grabbed cart has a destination zone, items from a
// different source tile are placed directly into cart cargo.
TEST_CASE( "zone_sorting_direct_delivery_to_cart",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source: ground tile south of player with UNSORTED zone
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Cart east of player with LOOT_FOOD zone at its position
    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs );

    // Place food on ground at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    zone_manager &zm = zone_manager::get_manager();
    item test_food( itype_test_apple );
    REQUIRE( zm.get_near_zone_type_for_item( test_food,
             here.get_abs( start_pos ) ) == zone_type_LOOT_FOOD );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Food delivered directly into cart cargo, source empty
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    CHECK( count_items_or_charges( cart_pos, itype_test_apple, cargo ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}

// When cart at source has items that already belong to a destination zone
// at that same tile, those items are skipped (not reshuffled).
TEST_CASE( "zone_sorting_cart_source_with_dest_zone",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart at source (south) with UNSORTED + LOOT_FOOD zones
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Vehicle zones: items in cart cargo need vehicle zone bindings to be
    // recognized as "at source" and "already at destination"
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, src_abs, true );

    // Place food in cart cargo - it already belongs to LOOT_FOOD at source
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Food stays in cart (sort_skip_item sees it's already at LOOT_FOOD destination)
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}

// When direct delivery target cart is full, fall back to normal pickup.
TEST_CASE( "zone_sorting_direct_delivery_cart_full",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south with UNSORTED
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Cart east with LOOT_FOOD zone
    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs );

    // Fill cart cargo to capacity (150L cart / 250ml apple = 600 apples)
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

    // Place food on ground at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item picked up from source via fallback (inventory), activity completed
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    // Fallback delivery: item dropped on ground at the LOOT_FOOD destination
    // (cart cargo was full, so the item goes on the map tile instead)
    CHECK( count_items_or_charges( cart_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );
}

// Virtual pickup with adjacent destination exercises both num_processed reset
// and the adjacent routing shortcut.
TEST_CASE( "zone_sorting_virtual_pickup_adjacent_dest",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart at source (south)
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Vehicle UNSORTED zone (items in cart cargo need vehicle zone)
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place multiple food items in cart cargo
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 3 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // All items delivered to adjacent destination
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 3 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// When destination is walled off (unreachable), virtual pickup items stay
// in cart and activity terminates cleanly.
TEST_CASE( "zone_sorting_virtual_pickup_unreachable_dest",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart at source (south)
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Vehicle UNSORTED zone (items in cart cargo need vehicle zone)
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );
    REQUIRE( zone_manager::get_manager().has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );

    // Destination walled off: LOOT_FOOD zone surrounded by walls
    const tripoint_bub_ms dest_pos = start_pos + tripoint( 5, 0, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Build walls around destination
    for( const tripoint &offset : {
             tripoint( 4, -1, 0 ), tripoint( 5, -1, 0 ), tripoint( 6, -1, 0 ),
             tripoint( 4, 0, 0 ), tripoint( 6, 0, 0 ),
             tripoint( 4, 1, 0 ), tripoint( 5, 1, 0 ), tripoint( 6, 1, 0 )
         } ) {
        here.ter_set( start_pos + offset, ter_t_wall );
    }

    // Place food in cart cargo
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Items stay in cart, activity terminates cleanly
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}

// Adjacent delivery works for plain ground tiles without any vehicle.
TEST_CASE( "zone_sorting_adjacent_ground_delivery",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south with UNSORTED zone
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Adjacent destination east with LOOT_FOOD zone
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place food on ground at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item delivered directly to adjacent destination, not in inventory
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.has_item_with( []( const item & it ) {
        return it.typeId() == itype_test_apple;
    } ) );
    CHECK( !dummy.activity );
}

// Terrain UNSORTED zone with grabbed cart at source: ground items get sorted,
// but the grabbed cart's cargo is protected (used for transport, not as source).
TEST_CASE( "zone_sorting_vehicle_on_terrain_unsorted_both_items",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    // Create terrain UNSORTED zone BEFORE spawning vehicle
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Spawn cart at source
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( !mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );

    // Destination: terrain LOOT_FOOD zone adjacent
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place items on ground AND in vehicle cargo
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Ground item sorted, vehicle cargo untouched
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    CHECK( !dummy.activity );
}

// Vehicle UNSORTED zone with grabbed cart: vehicle zone lets cart cargo be
// sorted (user explicitly marked the cart). Ground items also eligible since
// any unsorted zone makes all items at the tile sortable.
TEST_CASE( "zone_sorting_vehicle_unsorted_zone_ground_items_ignored",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    // Spawn cart at source, set owner
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Create vehicle UNSORTED zone
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs, true );

    zone_manager &mgr = zone_manager::get_manager();
    REQUIRE( !mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );

    // Destination: terrain LOOT_FOOD zone adjacent
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place items on ground AND in vehicle cargo
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Both vehicle cargo and ground items sorted (vehicle zone present,
    // so cart cargo is not protected; ground items always eligible)
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// Both terrain and vehicle UNSORTED zones at same position: all items sorted.
TEST_CASE( "zone_sorting_both_zones_at_same_position",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    // Create terrain UNSORTED zone BEFORE spawning vehicle
    create_tile_zone( "Unsorted Ground", zone_type_LOOT_UNSORTED, src_abs );

    // Spawn cart, set owner
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Create vehicle UNSORTED zone
    create_tile_zone( "Unsorted Vehicle", zone_type_LOOT_UNSORTED, src_abs, true );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );

    // Destination adjacent
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Items in both cargo and ground
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // All items sorted
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// Terrain-only UNSORTED zone with a non-grabbed vehicle (like a parked van):
// vehicle cargo should be sorted alongside ground items.
TEST_CASE( "zone_sorting_terrain_zone_sorts_non_grabbed_vehicle_cargo",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source tile south
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    // Create terrain UNSORTED zone BEFORE spawning vehicle
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Spawn a vehicle at source but do NOT grab it (simulates a parked van)
    vehicle *van = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                     src_pos, 0_degrees, 0, 0 );
    REQUIRE( van != nullptr );
    van->set_owner( dummy );
    // No grab - this is a parked vehicle

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();
    REQUIRE( mgr.has_terrain( zone_type_LOOT_UNSORTED, src_abs ) );
    REQUIRE( !mgr.has_vehicle( zone_type_LOOT_UNSORTED, src_abs ) );

    // Destination: terrain LOOT_FOOD zone adjacent
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Place items on ground AND in vehicle cargo
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );

    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Both ground and vehicle cargo items sorted (no grab, so no cart
    // protection - terrain zone sorts everything at the tile)
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 2 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 0 );
    CHECK( !dummy.activity );
}

// Unit test for has_terrain/has_vehicle/has helper methods.
TEST_CASE( "zone_sorting_has_terrain_has_vehicle_helpers",
           "[zones][activities][sorting]" )
{
    map &here = get_map();

    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms pos_a_bub( 60, 60, 0 );
    const tripoint_abs_ms pos_a = here.get_abs( pos_a_bub );
    here.ter_set( pos_a_bub, ter_t_floor );

    const tripoint_bub_ms pos_b_bub( 62, 60, 0 );
    const tripoint_abs_ms pos_b = here.get_abs( pos_b_bub );
    here.ter_set( pos_b_bub, ter_t_floor );

    // Create terrain zone at pos_a (before any vehicle)
    create_tile_zone( "Food Ground A", zone_type_LOOT_FOOD, pos_a );

    // Spawn vehicle at pos_a and create vehicle zone
    avatar &dummy = get_avatar();
    clear_avatar();
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      pos_a_bub, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    create_tile_zone( "Food Vehicle A", zone_type_LOOT_FOOD, pos_a, true );

    // Create terrain-only zone at pos_b (no vehicle there)
    create_tile_zone( "Food Ground B", zone_type_LOOT_FOOD, pos_b );

    zone_manager &mgr = zone_manager::get_manager();
    mgr.cache_vzones();

    // pos_a: both terrain and vehicle zones
    CHECK( mgr.has_terrain( zone_type_LOOT_FOOD, pos_a ) );
    CHECK( mgr.has_vehicle( zone_type_LOOT_FOOD, pos_a ) );
    CHECK( mgr.has( zone_type_LOOT_FOOD, pos_a ) );

    // pos_b: terrain only, no vehicle
    CHECK( mgr.has_terrain( zone_type_LOOT_FOOD, pos_b ) );
    CHECK( !mgr.has_vehicle( zone_type_LOOT_FOOD, pos_b ) );
    CHECK( mgr.has( zone_type_LOOT_FOOD, pos_b ) );
}

// Batching should consolidate pickups from nearby sources before delivering.
// Layout: player at S1 (UNSORTED, 10 apples), S2 one tile east (UNSORTED, 10
// apples), D ten tiles south (LOOT_FOOD). After picking up from S1, the sorter
// should detour to adjacent S2 (distance 1 < distance 10 to D) and pick up
// there too, delivering all 20 apples to D in one trip.
TEST_CASE( "zone_sorting_batches_nearby_sources",
           "[zones][items][activities][sorting][batching]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms s1_pos( 60, 60, 0 );
    dummy.setpos( here, s1_pos );
    dummy.clear_destination();
    // Backpack so can_stash succeeds for batching capacity check
    dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );

    // S1: player starts here, UNSORTED zone with 10 apples
    const tripoint_abs_ms s1_abs = here.get_abs( s1_pos );
    here.ter_set( s1_pos, ter_t_floor );
    create_tile_zone( "Unsorted S1", zone_type_LOOT_UNSORTED, s1_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s1_pos, item( itype_test_apple ) );
    }

    // S2: one tile east, UNSORTED zone with 10 apples
    const tripoint_bub_ms s2_pos = s1_pos + tripoint::east;
    const tripoint_abs_ms s2_abs = here.get_abs( s2_pos );
    here.ter_set( s2_pos, ter_t_floor );
    create_tile_zone( "Unsorted S2", zone_type_LOOT_UNSORTED, s2_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s2_pos, item( itype_test_apple ) );
    }

    // D: ten tiles south, LOOT_FOOD destination
    const tripoint_bub_ms dest_pos = s1_pos + tripoint( 0, 10, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Ensure clear floor along the path
    for( int y = s1_pos.y() + 1; y < dest_pos.y(); ++y ) {
        here.ter_set( tripoint_bub_ms( s1_pos.x(), y, 0 ), ter_t_floor );
    }

    REQUIRE( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 10 );
    REQUIRE( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 10 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Both sources should be empty - proves batching detoured to S2
    CHECK( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 0 );
    // Items are in player inventory (process_activity can't drive auto-move
    // to a non-adjacent dest, so delivery doesn't complete in the test harness).
    // The key assertion is that BOTH sources were emptied in one pass.
    int carried = 0;
    dummy.visit_items( [&carried]( const item * it, const item * ) {
        if( it->typeId() == itype_test_apple ) {
            carried++;
        }
        return VisitResponse::NEXT;
    } );
    CHECK( carried == 20 );
}

// Batching with a grabbed vehicle: the capacity check should consider the
// cart's free volume, not just the player's inventory. With 10 apples per
// source, items overflow into the cart when inventory fills up.
TEST_CASE( "zone_sorting_batches_into_grabbed_vehicle",
           "[zones][items][activities][sorting][batching][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms s1_pos( 60, 60, 0 );
    dummy.setpos( here, s1_pos );
    dummy.clear_destination();

    // Spawn shopping cart south of player and grab it
    const tripoint_bub_ms cart_pos = s1_pos + tripoint::south;
    here.ter_set( cart_pos, ter_t_floor );
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );

    // S1: player stands here, UNSORTED zone with 10 apples
    const tripoint_abs_ms s1_abs = here.get_abs( s1_pos );
    here.ter_set( s1_pos, ter_t_floor );
    create_tile_zone( "Unsorted S1", zone_type_LOOT_UNSORTED, s1_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s1_pos, item( itype_test_apple ) );
    }

    // S2: one tile east, UNSORTED zone with 10 apples
    const tripoint_bub_ms s2_pos = s1_pos + tripoint::east;
    const tripoint_abs_ms s2_abs = here.get_abs( s2_pos );
    here.ter_set( s2_pos, ter_t_floor );
    create_tile_zone( "Unsorted S2", zone_type_LOOT_UNSORTED, s2_abs );
    for( int i = 0; i < 10; i++ ) {
        here.add_item_or_charges( s2_pos, item( itype_test_apple ) );
    }

    // D: ten tiles south-west, LOOT_FOOD destination (far away)
    const tripoint_bub_ms dest_pos = s1_pos + tripoint( -5, 10, 0 );
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    REQUIRE( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 10 );
    REQUIRE( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 10 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Both sources should be empty - batching used cart volume for capacity
    CHECK( count_items_or_charges( s1_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( s2_pos, itype_test_apple, std::nullopt ) == 0 );
    // Items are in cart cargo and/or player inventory
    int total = 0;
    dummy.visit_items( [&total]( const item * it, const item * ) {
        if( it->typeId() == itype_test_apple ) {
            total++;
        }
        return VisitResponse::NEXT;
    } );
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    if( cargo.has_value() ) {
        total += count_items_or_charges( cart_pos, itype_test_apple, cargo );
    }
    CHECK( total == 20 );
}

// When both vehicle cargo and ground at the destination are full, items should
// stay at the source rather than overflowing to adjacent tiles.
TEST_CASE( "zone_sorting_adjacent_dest_both_full",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source south of player with UNSORTED zone
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Destination east of player with cart and LOOT_FOOD zone
    const tripoint_bub_ms dest_pos = start_pos + tripoint::east;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      dest_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Fill vehicle cargo to capacity
    std::optional<vpart_reference> cargo = here.veh_at( dest_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

    // Fill ground at destination to capacity (bypass capacity checks)
    const item filler_item( itype_test_apple );
    while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
        here.add_item( dest_pos, filler_item );
    }
    REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );

    // Place food at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item stays at source - dest was completely full
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );

    // No items spilled onto adjacent tiles (excluding src which has 1 apple)
    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            const tripoint_bub_ms neighbor = dest_pos + tripoint( dx, dy, 0 );
            if( neighbor == src_pos || neighbor == start_pos ) {
                continue;
            }
            CHECK( here.i_at( neighbor ).empty() );
        }
    }
}

// When a ground-only destination is full, items stay at source.
TEST_CASE( "zone_sorting_adjacent_ground_dest_full",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source south with UNSORTED zone
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Destination north with LOOT_FOOD zone (no vehicle)
    const tripoint_bub_ms dest_pos = start_pos + tripoint::north;
    const tripoint_abs_ms dest_abs = here.get_abs( dest_pos );
    here.ter_set( dest_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, dest_abs );

    // Fill ground at destination
    const item filler_item( itype_test_apple );
    while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
        here.add_item( dest_pos, filler_item );
    }
    REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );

    // Place food at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Item stays at source
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( !dummy.activity );

    // No spill to adjacent tiles
    for( int dx = -1; dx <= 1; dx++ ) {
        for( int dy = -1; dy <= 1; dy++ ) {
            if( dx == 0 && dy == 0 ) {
                continue;
            }
            const tripoint_bub_ms neighbor = dest_pos + tripoint( dx, dy, 0 );
            if( neighbor == src_pos || neighbor == start_pos ) {
                continue;
            }
            CHECK( here.i_at( neighbor ).empty() );
        }
    }
}

// Vehicle-only zone with full cargo: item stays at source, no ground fallback.
TEST_CASE( "zone_sorting_vehicle_dest_cargo_full_ground_fallback",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Source south with UNSORTED
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );

    // Cart east with vehicle-bound LOOT_FOOD zone, cargo full
    const tripoint_bub_ms cart_pos = start_pos + tripoint::east;
    const tripoint_abs_ms cart_abs = here.get_abs( cart_pos );
    here.ter_set( cart_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::east );

    // Vehicle-bound zone
    create_tile_zone( "Food", zone_type_LOOT_FOOD, cart_abs, true );

    // Fill cargo
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 700; i++ ) {
        item filler( itype_test_apple );
        if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
            break;
        }
    }
    REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

    // Place food at source
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );
    process_activity( dummy );

    // Vehicle-only zone with full cargo: item stays at source, no ground fallback
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( cart_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( !dummy.activity );
}

// When a terrain-bound zone (e.g. LOOT_FOOD) sits on a tile with a vehicle
// appliance (e.g. fridge/cart), items in vehicle cargo should be recognized as
// "already sorted" and not re-picked. Without the fix, sort_skip_item only
// checks has_terrain for ground items and has_vehicle for cargo items, so cargo
// items at a terrain-bound zone are never skipped, causing an infinite loop.
TEST_CASE( "zone_sorting_terrain_zone_vehicle_cargo_no_loop",
           "[zones][items][activities][sorting][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    // Cart south of player with terrain-bound UNSORTED + LOOT_FOOD zones
    const tripoint_bub_ms src_pos = start_pos + tripoint::south;
    const tripoint_abs_ms src_abs = here.get_abs( src_pos );
    here.ter_set( src_pos, ter_t_floor );

    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      src_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    // Terrain-bound zones (default veh=false). This is the bug scenario:
    // zone binding is terrain, but items are in vehicle cargo.
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, src_abs );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, src_abs );

    // Place food in cart cargo
    std::optional<vpart_reference> cargo = here.veh_at( src_pos ).cargo();
    REQUIRE( cargo.has_value() );
    cargo->vehicle().add_item( here, cargo->part(), item( itype_test_apple ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );

    // Bounded loop: process_activity would hang on the infinite loop bug
    int turns = 0;
    const int max_turns = 20;
    do {
        dummy.mod_moves( dummy.get_speed() );
        while( dummy.get_moves() > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
        turns++;
    } while( dummy.activity && turns < max_turns );

    REQUIRE( turns < max_turns );
    CHECK( !dummy.activity );

    // Food stays in cart cargo (recognized as already at LOOT_FOOD destination)
    CHECK( count_items_or_charges( src_pos, itype_test_apple, cargo ) == 1 );
    // No duplication on ground
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
}

// When source has items going to different zone types (FOOD vs DRINK), the
// activity should sort ALL of them across multiple internal passes, not just
// the first batch. The bug: after delivering one batch, num_processed is not
// reset when picked_up_stuff empties. The repopulated items list is shorter
// (delivered items removed), so the stale offset overshoots and the remaining
// items are never seen. The source gets erased and the activity terminates
// with "sorted out every item possible" while items still remain.
TEST_CASE( "zone_sorting_multi_dest_processes_all_items",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();
    zone_manager::get_manager().clear();

    // Layout: dest1 is 1 tile north of source, player 1 tile south.
    // When the sort picks up the first item (apple, destined for PFOOD
    // at dest1) and routes to adjacent(dest1), the A* finds source tile
    // (60,60) as the closest qualifying neighbor. The player teleports
    // there, drops the apple at dest1, and resumes stage_do still
    // adjacent to source. The stale num_processed offset (incremented
    // when the second item was skipped for having a different dest)
    // causes the items loop to start past the remaining item.
    //
    //   dest1 (PFOOD)    (60,59)
    //   source (UNSORTED) (60,60)   <-- A* endpoint: adj to dest1
    //   player start      (60,61)
    //
    //   dest2 (FOOD) far away at (55,60)

    const tripoint_bub_ms start_pos( 60, 61, 0 );
    dummy.setpos( here, start_pos );
    dummy.clear_destination();

    const tripoint_bub_ms src_pos( 60, 60, 0 );
    here.ter_set( src_pos, ter_t_floor );
    create_tile_zone( "Unsorted", zone_type_LOOT_UNSORTED, here.get_abs( src_pos ) );

    // PFOOD dest: 1 tile north of source. Route from player (60,61) to
    // adj(60,59) finds source (60,60) as closest qualifying tile (dist 1).
    // Player ends up at source after delivery -- still adjacent, no THINK reset.
    const tripoint_bub_ms pfood_pos( 60, 59, 0 );
    here.ter_set( pfood_pos, ter_t_floor );
    create_tile_zone( "PFood", zone_type_LOOT_PFOOD, here.get_abs( pfood_pos ) );

    // FOOD dest: far away so it's not adjacent from the delivery endpoint
    const tripoint_bub_ms food_pos( 55, 60, 0 );
    here.ter_set( food_pos, ter_t_floor );
    create_tile_zone( "Food", zone_type_LOOT_FOOD, here.get_abs( food_pos ) );

    // test_apple -> LOOT_PFOOD (perishable), test_bitter_almond -> LOOT_FOOD
    here.add_item_or_charges( src_pos, item( itype_test_apple ) );
    here.add_item_or_charges( src_pos, item( itype_test_bitter_almond ) );
    REQUIRE( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
    REQUIRE( count_items_or_charges( src_pos, itype_test_bitter_almond, std::nullopt ) == 1 );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.assign_activity( zone_sort_activity_actor() );

    // Simulate auto-move via teleport. route_to_destination nulls the activity
    // and sets destination_point; we teleport there and restart.
    int teleports = 0;
    const int max_teleports = 30;
    do {
        int turns = 0;
        while( dummy.activity && turns < 200 ) {
            dummy.mod_moves( dummy.get_speed() );
            while( dummy.get_moves() > 0 && dummy.activity ) {
                dummy.activity.do_turn( dummy );
            }
            turns++;
        }
        if( dummy.destination_point ) {
            dummy.setpos( here, here.get_bub( *dummy.destination_point ) );
            if( dummy.has_destination_activity() ) {
                dummy.start_destination_activity();
            } else {
                dummy.clear_destination();
            }
            teleports++;
        }
    } while( ( dummy.activity || dummy.destination_point ) && teleports < max_teleports );

    REQUIRE( teleports < max_teleports );

    // Both items should be gone from source - sorted to their destinations
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 0 );
    CHECK( count_items_or_charges( src_pos, itype_test_bitter_almond, std::nullopt ) == 0 );
}
