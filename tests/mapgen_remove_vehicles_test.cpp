#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mapgen_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"

static const nested_mapgen_id
nested_mapgen_test_nested_remove_all_1x1( "test_nested_remove_all_1x1" );
static const nested_mapgen_id
nested_mapgen_test_nested_remove_shopping_cart( "test_nested_remove_shopping_cart" );
static const update_mapgen_id
update_mapgen_test_update_place_shopping_cart( "test_update_place_shopping_cart" );
static const update_mapgen_id
update_mapgen_test_update_remove_shopping_cart( "test_update_remove_shopping_cart" );
static const vproto_id vehicle_prototype_motorcycle_cross( "motorcycle_cross" );
static const vproto_id vehicle_prototype_shopping_cart( "shopping_cart" );
namespace
{
void check_vehicle_still_works( vehicle &veh )
{
    map &here = get_map();
    REQUIRE( here.veh_at( get_avatar().pos_bub() ).has_value() );
    REQUIRE( veh.player_in_control( here, get_avatar() ) );
    veh.engine_on = true;
    veh.velocity = 1000;
    veh.cruise_velocity = veh.velocity;
    tripoint_bub_ms const startp = veh.pos_bub( here );
    here.vehmove();
    REQUIRE( veh.pos_bub( here ) != startp );

    here.displace_vehicle( veh, startp - veh.pos_bub( here ) );
}

vehicle *add_test_vehicle( map &m, tripoint_bub_ms loc )
{
    return m.add_vehicle( vehicle_prototype_shopping_cart, loc, -90_degrees, 100, 0 );
}

void remote_add_test_vehicle( map &m )
{
    wipe_map_terrain( &m );
    m.clear_traps();
    REQUIRE( m.get_vehicles().empty() );
    add_test_vehicle( m, tripoint_bub_ms::zero );
    REQUIRE( m.get_vehicles().size() == 1 );
    REQUIRE( m.veh_at( tripoint_bub_ms::zero ).has_value() );
}

template<typename F, typename ID>
void local_test( vehicle *veh, tripoint_bub_ms const &test_loc, F const &fmg, ID const &id )
{
    map &here = get_map();
    tripoint_abs_omt const this_test_omt =
        project_to<coords::omt>( get_map().get_abs( test_loc ) );
    tripoint_bub_ms const this_test_loc = test_loc + point::east;
    vehicle *const veh2 = add_test_vehicle( here, this_test_loc );
    REQUIRE( here.veh_at( this_test_loc ).has_value() );
    REQUIRE( here.get_vehicles().size() == 2 );
    REQUIRE( veh->pos_abs() == veh2->pos_abs() - point::east );
    REQUIRE( veh->sm_pos == veh2->sm_pos );

    manual_mapgen( this_test_omt, fmg, id );
    REQUIRE( here.get_vehicles().size() == 1 );
    check_vehicle_still_works( *veh );
}

template<typename F, typename ID>
void remote_test( vehicle *veh, tripoint_bub_ms const &test_loc, F const &fmg, ID const &id )
{
    map &here = get_map();
    tripoint_abs_omt const this_test_omt =
        project_to<coords::omt>( here.get_abs( test_loc ) ) + tripoint{ 20, 20, 0 };
    manual_mapgen( this_test_omt, fmg, id, remote_add_test_vehicle );
    REQUIRE( here.get_vehicles().size() == 1 );
    check_vehicle_still_works( *veh );

    tinymap tm;
    tm.load( this_test_omt, true );
    REQUIRE( tm.get_vehicles().empty() );
}

} // namespace

TEST_CASE( "mapgen_remove_vehicles" )
{
    map &here = get_map();
    clear_avatar();
    // this position should prevent pointless mapgen
    tripoint_bub_ms const start_loc( HALF_MAPSIZE_X + SEEX - 2, HALF_MAPSIZE_Y + SEEY - 1, 0 );
    get_avatar().setpos( here, start_loc );
    clear_map();
    REQUIRE( here.get_vehicles().empty() );

    vehicle *const veh =
        here.add_vehicle( vehicle_prototype_motorcycle_cross, start_loc, -90_degrees, 100, 0 );
    veh->set_owner( get_avatar() );
    REQUIRE( here.get_vehicles().size() == 1 );
    here.board_vehicle( start_loc, &get_avatar() );
    veh->start_engines( here, &get_avatar(), true );

    tripoint_bub_ms const test_loc = get_avatar().pos_bub();
    check_vehicle_still_works( *veh );

    SECTION( "nested remove_vehicle outside of main map" ) {
        remote_test( veh, test_loc, manual_nested_mapgen, nested_mapgen_test_nested_remove_shopping_cart );
    }

    SECTION( "nested remove_vehicle on main map" ) {
        local_test( veh, test_loc, manual_nested_mapgen, nested_mapgen_test_nested_remove_shopping_cart );
    }

    SECTION( "update with remove_vehicle outside of main map" ) {
        remote_test( veh, test_loc, manual_update_mapgen, update_mapgen_test_update_remove_shopping_cart );
    }

    SECTION( "update with remove_vehicle on main map" ) {
        local_test( veh, test_loc, manual_update_mapgen, update_mapgen_test_update_remove_shopping_cart );
    }

    SECTION( "update place then nested remove all on main map" ) {
        tripoint_abs_omt const this_test_omt =
            project_to<coords::omt>( get_map().get_abs( test_loc ) );
        tripoint_bub_ms const this_test_loc = get_map().get_bub( project_to<coords::ms>
                                              ( this_test_omt ) );
        manual_mapgen( this_test_omt, manual_update_mapgen, update_mapgen_test_update_place_shopping_cart );
        REQUIRE( here.get_vehicles().size() == 2 );
        REQUIRE( here.veh_at( this_test_loc ).has_value() );
        check_vehicle_still_works( *veh );

        manual_mapgen( this_test_omt, manual_nested_mapgen, nested_mapgen_test_nested_remove_all_1x1 );
        REQUIRE( here.get_vehicles().size() == 1 );
        REQUIRE( !here.veh_at( this_test_loc ).has_value() );
        check_vehicle_still_works( *veh );
    }
}
