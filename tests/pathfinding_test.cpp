#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "field_type.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "pathfinding.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const itype_id itype_test_heavy_boulder( "test_heavy_boulder" );

static const ter_str_id ter_t_fence_barbed( "t_fence_barbed" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_pit( "t_pit" );
static const ter_str_id ter_t_reinforced_glass_shutter( "t_reinforced_glass_shutter" );
static const ter_str_id ter_t_wall( "t_wall" );

static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

static void build_open_area( map &here, const tripoint_bub_ms &center, int radius = 15 )
{
    for( int x = center.x() - radius; x <= center.x() + radius; x++ ) {
        for( int y = center.y() - radius; y <= center.y() + radius; y++ ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_floor );
        }
    }
}

static void build_wall_barrier( map &here, int y, int min_x, int max_x,
                                int gap_x = -1 )
{
    for( int x = min_x; x <= max_x; x++ ) {
        if( x != gap_x ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_wall );
        }
    }
}

static vehicle *setup_grabbed_cart( avatar &dummy, map &here,
                                    const tripoint_bub_ms &player_pos,
                                    const tripoint_rel_ms &grab_dir )
{
    dummy.setpos( here, player_pos );
    dummy.clear_destination();

    const tripoint_bub_ms cart_pos = player_pos + grab_dir;
    vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart,
                                      cart_pos, 0_degrees, 0, 0 );
    REQUIRE( cart != nullptr );
    cart->set_owner( dummy );
    dummy.grab( object_type::VEHICLE, grab_dir );
    REQUIRE( dummy.get_grab_type() == object_type::VEHICLE );
    REQUIRE( cart->get_points().size() == 1 );
    return cart;
}

TEST_CASE( "route_with_grab_avoids_sharp_terrain",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );
    const tripoint_bub_ms sharp_pos( 60, 65, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );
    build_wall_barrier( here, 65, 0, MAPSIZE_X - 1, 60 );
    here.ter_set( sharp_pos, ter_t_fence_barbed );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto blocked_route = route_with_grab( here, dummy,
                                          pathfinding_target::adjacent( dest_pos ) );
    CHECK( blocked_route.empty() );

    here.ter_set( sharp_pos, ter_t_floor );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto open_route = route_with_grab( here, dummy,
                                       pathfinding_target::adjacent( dest_pos ) );
    CHECK( !open_route.empty() );
}

TEST_CASE( "route_with_grab_fast_path_fallback",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );
    const tripoint_bub_ms sharp_pos( 60, 65, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );
    here.ter_set( sharp_pos, ter_t_fence_barbed );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    CHECK( !route.empty() );
}

TEST_CASE( "grab_aware_routing_integration",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    SECTION( "with grabbed single-tile vehicle" ) {
        setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );

        auto result = route_with_grab( here, dummy,
                                       pathfinding_target::point( dest_pos ) );
        CHECK( !result.empty() );
    }

    SECTION( "without grab, normal routing" ) {
        dummy.setpos( here, start_pos );
        dummy.clear_destination();
        dummy.grab( object_type::NONE );

        auto result = here.route( dummy.pos_bub(),
                                  pathfinding_target::point( dest_pos ),
                                  dummy.get_pathfinding_settings() );
        CHECK( !result.empty() );
    }
}

TEST_CASE( "route_with_grab_penalizes_dangerous_traps",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    const tripoint_bub_ms pit_pos( 60, 65, 0 );
    here.ter_set( pit_pos, ter_t_pit );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    REQUIRE( !route.empty() );

    const bool route_hits_pit = std::find( route.begin(), route.end(),
                                           pit_pos ) != route.end();
    CHECK_FALSE( route_hits_pit );

    here.ter_set( pit_pos, ter_t_floor );
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    dummy.setpos( here, start_pos + tripoint::north );
    dummy.setpos( here, start_pos );
    dummy.grab( object_type::VEHICLE, tripoint_rel_ms::south );

    auto direct_route = route_with_grab( here, dummy,
                                         pathfinding_target::adjacent( dest_pos ) );
    REQUIRE( !direct_route.empty() );
    CHECK( direct_route.size() <= route.size() );
}

TEST_CASE( "route_with_grab_rejects_open_air",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );
    build_wall_barrier( here, 65, 0, MAPSIZE_X - 1, 60 );
    here.ter_set( tripoint_bub_ms( 60, 65, 0 ), ter_t_open_air );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    CHECK( route.empty() );
}

TEST_CASE( "route_with_grab_rejects_reinforced_glass_shutters",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );
    build_wall_barrier( here, 65, 0, MAPSIZE_X - 1, 60 );
    here.ter_set( tripoint_bub_ms( 60, 65, 0 ), ter_t_reinforced_glass_shutter );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    CHECK( route.empty() );
}

TEST_CASE( "route_with_grab_avoids_dangerous_fields",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );
    const tripoint_bub_ms fire_pos( 60, 65, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );
    here.add_field( fire_pos, fd_fire, 1, 10_minutes );
    REQUIRE( here.get_field( fire_pos, fd_fire ) );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    REQUIRE( !route.empty() );

    const bool route_hits_fire = std::find( route.begin(), route.end(),
                                            fire_pos ) != route.end();
    CHECK_FALSE( route_hits_fire );
}

TEST_CASE( "route_with_grab_respects_avoid_callback",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );
    const tripoint_bub_ms blocked_pos( 60, 65, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );
    build_wall_barrier( here, 65, 0, MAPSIZE_X - 1, 60 );
    here.ter_set( blocked_pos, ter_t_floor );

    setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    // Avoid callback that rejects the only gap in the wall
    auto avoid = [&blocked_pos]( const tripoint_bub_ms & p ) {
        return p == blocked_pos;
    };

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ), avoid );
    CHECK( route.empty() );

    // Without the avoid callback, route should succeed
    auto route_no_avoid = route_with_grab( here, dummy,
                                           pathfinding_target::adjacent( dest_pos ) );
    CHECK( !route_no_avoid.empty() );
}

TEST_CASE( "route_with_grab_accounts_for_broken_wheels",
           "[pathfinding][vehicle]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms start_pos( 60, 60, 0 );
    const tripoint_bub_ms dest_pos( 60, 70, 0 );

    build_open_area( here, tripoint_bub_ms( 60, 65, 0 ) );

    vehicle *cart = setup_grabbed_cart( dummy, here, start_pos, tripoint_rel_ms::south );

    // Add heavy cargo so mass-based drag cap is meaningful
    const tripoint_bub_ms cart_pos = start_pos + tripoint::south;
    std::optional<vpart_reference> cargo = here.veh_at( cart_pos ).cargo();
    REQUIRE( cargo.has_value() );
    for( int i = 0; i < 200; i++ ) {
        if( !cargo->vehicle().add_item( here, cargo->part(),
                                        item( itype_test_heavy_boulder ) ) ) {
            break;
        }
    }

    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );

    const int healthy_req = cart->drag_str_req_at( here, cart_pos );

    // Remove the wheel -- simulates complete wheel destruction from collision.
    // Without wheels, drag_str_req_at returns the mass-based maximum.
    std::vector<int> wheels = cart->wheelcache;
    for( const int idx : wheels ) {
        cart->remove_part( cart->part( idx ) );
    }
    cart->part_removal_cleanup( here );
    REQUIRE( cart->wheelcache.empty() );

    const int broken_req = cart->drag_str_req_at( here, cart_pos );
    REQUIRE( broken_req > healthy_req );

    // Set strength between healthy and broken requirements
    dummy.set_str_base( healthy_req + 1 );
    dummy.set_str_bonus( 0 );
    REQUIRE( dummy.get_arm_str() >= healthy_req );
    REQUIRE( dummy.get_arm_str() < broken_req );

    auto route = route_with_grab( here, dummy,
                                  pathfinding_target::adjacent( dest_pos ) );
    CHECK( route.empty() );
}
