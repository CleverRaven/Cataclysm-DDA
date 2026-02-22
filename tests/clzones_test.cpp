#include <functional>
#include <initializer_list>
#include <list>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
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
#include "vpart_position.h"

class item_location;

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

// Terrain UNSORTED zone should only sort ground items, not vehicle cargo.
// Vehicle parked on the same tile should have its cargo left untouched.
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

// Vehicle UNSORTED zone should only sort vehicle cargo items, not ground items.
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

    // Vehicle cargo sorted, ground item untouched
    CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    CHECK( count_items_or_charges( src_pos, itype_test_apple, std::nullopt ) == 1 );
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

// When vehicle cargo is full but ground has room, items fall through to ground
// at the same tile. This documents the known limitation: cargo-to-ground fallback
// fires even for vehicle-bound zones.
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

// Direct tests for the overflow parameter on drop wrapper functions.
TEST_CASE( "zone_sorting_drop_overflow_disabled",
           "[zones][items][activities][sorting]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms pos( 60, 60, 0 );
    dummy.setpos( here, pos );
    here.ter_set( pos, ter_t_floor );

    const tripoint_bub_ms dest_pos = pos + tripoint::east;
    here.ter_set( dest_pos, ter_t_floor );

    SECTION( "drop_on_map with overflow=false on full tile returns empty" ) {
        // Fill destination
        const item filler_item( itype_test_apple );
        while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
            here.add_item( dest_pos, filler_item );
        }
        REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );

        std::vector<item_location> result = drop_on_map( dummy,
                                            item_drop_reason::deliberate,
        { item( itype_test_apple ) }, &here, dest_pos, false );
        CHECK( result.empty() );

        // Adjacent tiles have no spill
        for( int dx = -1; dx <= 1; dx++ ) {
            for( int dy = -1; dy <= 1; dy++ ) {
                if( dx == 0 && dy == 0 ) {
                    continue;
                }
                CHECK( here.i_at( dest_pos + tripoint( dx, dy, 0 ) ).empty() );
            }
        }
    }

    SECTION( "vehicle cargo full, overflow=false falls back to ground" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          dest_pos, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        cart->set_owner( dummy );

        // Fill cargo
        std::optional<vpart_reference> cargo = here.veh_at( dest_pos ).cargo();
        REQUIRE( cargo.has_value() );
        for( int i = 0; i < 700; i++ ) {
            item filler( itype_test_apple );
            if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
                break;
            }
        }
        REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

        std::vector<item_location> result = put_into_vehicle_or_drop_ret_locs( dummy,
                                            item_drop_reason::deliberate,
        { item( itype_test_apple ) }, &here, dest_pos, false, false );
        // Ground has room, so item goes there
        CHECK( !result.empty() );
        CHECK( count_items_or_charges( dest_pos, itype_test_apple, std::nullopt ) == 1 );
    }

    SECTION( "vehicle cargo and ground both full, overflow=false returns empty" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          dest_pos, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        cart->set_owner( dummy );

        // Fill cargo
        std::optional<vpart_reference> cargo = here.veh_at( dest_pos ).cargo();
        REQUIRE( cargo.has_value() );
        for( int i = 0; i < 700; i++ ) {
            item filler( itype_test_apple );
            if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
                break;
            }
        }
        REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

        // Fill ground
        const item filler_item( itype_test_apple );
        while( here.free_volume( dest_pos ) >= filler_item.volume() ) {
            here.add_item( dest_pos, filler_item );
        }
        REQUIRE( here.free_volume( dest_pos ) < filler_item.volume() );

        std::vector<item_location> result = put_into_vehicle_or_drop_ret_locs( dummy,
                                            item_drop_reason::deliberate,
        { item( itype_test_apple ) }, &here, dest_pos, false, false );
        CHECK( result.empty() );
    }

    SECTION( "count_by_charges item, cargo full, overflow=false - no charge duplication" ) {
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                          dest_pos, 0_degrees, 0, 0 );
        REQUIRE( cart != nullptr );
        cart->set_owner( dummy );

        // Fill cargo
        std::optional<vpart_reference> cargo = here.veh_at( dest_pos ).cargo();
        REQUIRE( cargo.has_value() );
        for( int i = 0; i < 700; i++ ) {
            item filler( itype_test_apple );
            if( !cargo->vehicle().add_item( here, cargo->part(), filler ) ) {
                break;
            }
        }
        REQUIRE( cargo->items().free_volume() < item( itype_test_apple ).volume() );

        // 556 ammo is count_by_charges
        item ammo( itype_556 );
        ammo.charges = 50;
        REQUIRE( ammo.count_by_charges() );
        const int original_charges = ammo.charges;

        // Count charges already in cargo (should be 0)
        int cargo_charges_before = 0;
        for( const item &it : cargo->items() ) {
            if( it.typeId() == itype_556 ) {
                cargo_charges_before += it.charges;
            }
        }

        std::vector<item_location> result = put_into_vehicle_or_drop_ret_locs( dummy,
                                            item_drop_reason::deliberate,
        { ammo }, &here, dest_pos, false, false );

        // Item should go to ground (cargo full, ground has room)
        CHECK( !result.empty() );

        // No partial charges in cargo - only on ground
        int cargo_charges_after = 0;
        for( const item &it : cargo->items() ) {
            if( it.typeId() == itype_556 ) {
                cargo_charges_after += it.charges;
            }
        }
        CHECK( cargo_charges_after == cargo_charges_before );

        // All charges landed on ground at dest
        int ground_charges = 0;
        for( const item &it : here.i_at( dest_pos ) ) {
            if( it.typeId() == itype_556 ) {
                ground_charges += it.charges;
            }
        }
        CHECK( ground_charges == original_charges );
    }
}
